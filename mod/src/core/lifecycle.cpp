#include "core/lifecycle.h"

#include <windows.h>
#include <dbghelp.h>

#include <atomic>
#include <cstdio>
#include <cstring>

#include <spdlog/spdlog.h>

#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/console.h"
#include "engine/d3d8_hooks.h"
#include "probe/probe.h"

namespace dh2::core {
namespace {

safetyhook::InlineHook* g_mutex_hook = nullptr;
safetyhook::InlineHook* g_bsp_hook = nullptr;
bool g_allow_multi = true;
char g_mutex_name[64] = {};
char g_mutex_note[192] = {};
std::atomic<bool> g_mutex_noted{false};
std::atomic<bool> g_mutex_flushed{false};

wchar_t g_dump_path[MAX_PATH] = {};
LPTOP_LEVEL_EXCEPTION_FILTER g_prev_filter = nullptr;
std::atomic<bool> g_dump_written{false};

HANDLE g_stop = nullptr;
std::atomic<bool> g_shutdown_dispatched{false};

std::atomic<HMODULE> g_engine{nullptr};
std::atomic<HMODULE> g_game{nullptr};
std::atomic<bool> g_engine_done{false};
std::atomic<bool> g_game_done{false};

HANDLE WINAPI hk_CreateMutexA(LPSECURITY_ATTRIBUTES sa, BOOL owner, LPCSTR name) {
    LPCSTR used = name;
    if (g_allow_multi && name && std::strcmp(name, "GI UNIQUE MUTEX") == 0) {
        std::snprintf(g_mutex_name, sizeof g_mutex_name, "GI UNIQUE MUTEX %lu", GetCurrentProcessId());
        used = g_mutex_name;
        std::snprintf(g_mutex_note, sizeof g_mutex_note, "CreateMutexA '%s' -> '%s'", name, used);
        g_mutex_noted.store(true);
        if (spdlog::default_logger()) spdlog::info("{}", g_mutex_note);
    }
    return g_mutex_hook->unsafe_stdcall<HANDLE>(sa, owner, used);
}

bool g_bsp_loaded_in_start = false;

int __fastcall hk_BspLoad(void* self, void*, char* name, unsigned flags, int arg) {
    const ULONGLONG t0 = GetTickCount64();
    const int r = g_bsp_hook->unsafe_thiscall<int>(self, name, flags, arg);
    const std::string label = safe_cstr(name);
    spdlog::info("cBspTree::Load('{}', {:#x}, {}) -> {} in {}ms", label, flags, arg, r, GetTickCount64() - t0);
    // Probe runs saw success as 1 (runtime-spmap-probe.txt).
    if (r == 1) {
        g_bsp_loaded_in_start = true;
        modules::DispatchMapLoaded(label.c_str());
        console::OnMapLoaded(label.c_str());
    }
    return r;
}

// Main7 cSpServer::StartGame (RVA 0x1ed80, thiscall(server, const char* map, int)). spmap and menumap both
// end here, after StopServer has torn the world down. Starting the map that is already loaded (results
// screen "replay") resets the BSP instead of calling cBspTree::Load, so modules would keep pointers to
// destroyed entities: dispatch OnMapLoaded for that case too (re/notes/lead-mission.md, map flow).
safetyhook::InlineHook* g_start_hook = nullptr;

int __fastcall hk_StartGame(void* self, void*, const char* map, int arg) {
    g_bsp_loaded_in_start = false;
    const std::string name = safe_cstr(map, 64);
    const int r = g_start_hook->unsafe_thiscall<int>(self, map, arg);
    spdlog::info("cSpServer::StartGame('{}', {}) -> {} (bsp {})", name, arg, r,
                 g_bsp_loaded_in_start ? "loaded" : "reset, same map");
    if (!g_bsp_loaded_in_start && !name.empty()) {
        const std::string label = "Bsp\\" + name + ".bsp";
        modules::DispatchMapLoaded(label.c_str());
        console::OnMapLoaded(label.c_str());
    }
    g_bsp_loaded_in_start = false;
    return r;
}

// Main7 cSpServer::StopServer (RVA 0x1b060, thiscall(server) -> 1). spmap/menumap call it before StartGame;
// it destroys the world. Modules get OnMapUnloading first, while entities are still valid.
safetyhook::InlineHook* g_stop_hook = nullptr;

int __fastcall hk_StopServer(void* self, void*) {
    modules::DispatchMapUnloading();
    return g_stop_hook->unsafe_thiscall<int>(self);
}

void* checked_main7(uintptr_t rva, const char* label) {
    static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
    auto* exe = reinterpret_cast<uint8_t*>(GetModuleHandleA(nullptr));
    void* target = exe ? exe + rva : nullptr;
    uint8_t got[5] = {};
    if (!target || !safe_copy(target, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
        spdlog::error("{}: unexpected bytes at Main7+{:#x}; not hooked (wrong game build?)", label, rva);
        return nullptr;
    }
    return target;
}

void install_start_game_hook() {
    g_start_hook = install_hook("cSpServer::StartGame", checked_main7(0x1ed80, "cSpServer::StartGame"),
                                reinterpret_cast<void*>(&hk_StartGame));
    g_stop_hook = install_hook("cSpServer::StopServer", checked_main7(0x1b060, "cSpServer::StopServer"),
                               reinterpret_cast<void*>(&hk_StopServer));
}

struct LdrStr {
    USHORT Length, MaximumLength;
    PWSTR Buffer;
};
struct LdrNotifData {
    ULONG Flags;
    const LdrStr* FullDllName;
    const LdrStr* BaseDllName;
    PVOID DllBase;
    ULONG SizeOfImage;
};
using LdrNotifFn = VOID(NTAPI*)(ULONG reason, const LdrNotifData* data, PVOID ctx);
using LdrRegisterFn = LONG(NTAPI*)(ULONG flags, LdrNotifFn fn, PVOID ctx, PVOID* cookie);

bool base_name_is(const LdrNotifData* d, const wchar_t* want) {
    if (!d || !d->BaseDllName || !d->BaseDllName->Buffer) return false;
    return _wcsicmp(d->BaseDllName->Buffer, want) == 0;
}

VOID NTAPI on_ldr(ULONG reason, const LdrNotifData* d, PVOID) {
    if (reason != 1 || !d) return;
    if (base_name_is(d, L"EngineDll7.dll")) g_engine.store(static_cast<HMODULE>(d->DllBase));
    if (base_name_is(d, L"GameDll7.dll")) g_game.store(static_cast<HMODULE>(d->DllBase));
}

void note_engine(HMODULE m) {
    bool expected = false;
    if (!m || !g_engine_done.compare_exchange_strong(expected, true)) return;
    spdlog::info("EngineDll7.dll base={:#x} ({})", reinterpret_cast<uintptr_t>(m), module_base_name(m));
    console::Bind(m);
    void* load = find_export(m, "?Load@cBspTree@@QAEHPADIH@Z");
    g_bsp_hook = install_hook("cBspTree::Load", load, reinterpret_cast<void*>(&hk_BspLoad));
    install_start_game_hook();
    modules::DispatchEngineReady(m);
}

void note_game(HMODULE m) {
    bool expected = false;
    if (!m || !g_game_done.compare_exchange_strong(expected, true)) return;
    char path[MAX_PATH] = {};
    GetModuleFileNameA(m, path, MAX_PATH);
    spdlog::info("GameDll7.dll base={:#x} path={}", reinterpret_cast<uintptr_t>(m), path);
    modules::DispatchGameDll(m);
}

LONG WINAPI unhandled_filter(EXCEPTION_POINTERS* ep) {
    if (!ep || !ep->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
    bool expected = false;
    if (g_dump_written.compare_exchange_strong(expected, true) && g_dump_path[0]) {
        HANDLE file = CreateFileW(g_dump_path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION info{};
            info.ThreadId = GetCurrentThreadId();
            info.ExceptionPointers = ep;
            info.ClientPointers = FALSE;
            const BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal, &info,
                                               nullptr, nullptr);
            CloseHandle(file);
            if (!ok) g_dump_written.store(false);
        }
    }
    const EXCEPTION_RECORD* r = ep->ExceptionRecord;
    const std::string where = describe_address(r->ExceptionAddress);
    spdlog::error("unhandled exception {:#010x} at {} eip={:#x} dump={}", r->ExceptionCode, where,
                  ep->ContextRecord ? static_cast<uintptr_t>(ep->ContextRecord->Eip) : 0,
                  g_dump_written.load() ? "dh2coop-crash.dmp" : "not written");
    if (g_prev_filter) return g_prev_filter(ep);
    return EXCEPTION_CONTINUE_SEARCH;
}

LONG CALLBACK first_chance_logger(EXCEPTION_POINTERS* ep) {
    static thread_local bool busy = false;
    static volatile LONG count = 0;
    if (busy || !ep || !ep->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
    const DWORD code = ep->ExceptionRecord->ExceptionCode;
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_STACK_OVERFLOW:
        case EXCEPTION_PRIV_INSTRUCTION:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case 0xC0000409:
            break;
        default:
            return EXCEPTION_CONTINUE_SEARCH;
    }
    if (InterlockedIncrement(&count) > 16) return EXCEPTION_CONTINUE_SEARCH;
    busy = true;
    const CONTEXT* c = ep->ContextRecord;
    const EXCEPTION_RECORD* r = ep->ExceptionRecord;
    spdlog::error("first-chance exception {:#010x} at {} eip={:#x}", code, describe_address(r->ExceptionAddress),
                  c ? static_cast<uintptr_t>(c->Eip) : 0);
    busy = false;
    return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

void install_mutex_hook() {
    const std::string ini = (self_dir() / "dh2coop.ini").string();
    g_allow_multi = GetPrivateProfileIntA("Coop", "AllowMultiInstance", 1, ini.c_str()) != 0;
    HMODULE k = GetModuleHandleA("kernel32.dll");
    void* target = k ? reinterpret_cast<void*>(GetProcAddress(k, "CreateMutexA")) : nullptr;
    g_mutex_hook = install_hook("CreateMutexA", target, reinterpret_cast<void*>(&hk_CreateMutexA));
    if (!g_mutex_hook) {
        std::snprintf(g_mutex_note, sizeof g_mutex_note, "CreateMutexA hook NOT installed (AllowMultiInstance=%d)",
                      g_allow_multi ? 1 : 0);
        g_mutex_noted.store(true);
    }
}

void flush_mutex_log() {
    bool expected = false;
    if (!g_mutex_flushed.compare_exchange_strong(expected, true)) return;
    spdlog::info("AllowMultiInstance={}", g_allow_multi ? 1 : 0);
    if (g_mutex_noted.load() && g_mutex_note[0]) spdlog::info("{}", g_mutex_note);
    else spdlog::info("CreateMutexA hook armed; rename pending");
}

void install_crash_handlers() {
    const std::filesystem::path dump = self_dir() / "dh2coop-crash.dmp";
    wcsncpy_s(g_dump_path, dump.c_str(), _TRUNCATE);
    g_prev_filter = SetUnhandledExceptionFilter(unhandled_filter);
    AddVectoredExceptionHandler(0, first_chance_logger);
    spdlog::info("crash filter installed; dump path {}", dump.string());
}

void request_shutdown() {
    if (g_stop) SetEvent(g_stop);
}

void run_lifecycle() {
    g_stop = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (HMODULE nt = GetModuleHandleA("ntdll.dll")) {
        auto reg = reinterpret_cast<LdrRegisterFn>(GetProcAddress(nt, "LdrRegisterDllNotification"));
        PVOID cookie = nullptr;
        if (reg) {
            const LONG st = reg(0, on_ldr, nullptr, &cookie);
            spdlog::info("LdrRegisterDllNotification -> {:#x}", static_cast<std::uint32_t>(st));
        } else {
            spdlog::warn("LdrRegisterDllNotification missing");
        }
    }

    const Config& cfg = config();
    const DWORD poll = cfg.poll_ms > 0 ? static_cast<DWORD>(cfg.poll_ms) : 50;
    const ULONGLONG start = GetTickCount64();
    bool warned = false;

    for (;;) {
        d3d8::TryInstall();
        if (HMODULE eng = g_engine.load()) note_engine(eng);
        if (!g_engine_done.load()) {
            if (HMODULE m = GetModuleHandleA("EngineDll7.dll")) note_engine(m);
        }
        if (HMODULE game = g_game.load()) note_game(game);
        if (!g_game_done.load()) {
            if (HMODULE m = GetModuleHandleA("GameDll7.dll")) note_game(m);
        }
        if (g_engine_done.load()) d3d8::TryLateDevice(GetModuleHandleA("EngineDll7.dll"));
        probe::tick();

        if (!warned && GetTickCount64() - start > static_cast<ULONGLONG>(cfg.module_wait_timeout_ms)) {
            warned = true;
            if (!g_engine_done.load()) spdlog::warn("EngineDll7.dll still not seen");
            if (!g_game_done.load()) spdlog::warn("GameDll7.dll still not seen");
        }
        if (g_mutex_noted.load()) flush_mutex_log();
        if (WaitForSingleObject(g_stop, poll) == WAIT_OBJECT_0) break;
    }

    bool expected = false;
    if (g_shutdown_dispatched.compare_exchange_strong(expected, true)) {
        spdlog::info("lifecycle shutdown");
        modules::DispatchShutdown();
    }
}

}  // namespace dh2::core
