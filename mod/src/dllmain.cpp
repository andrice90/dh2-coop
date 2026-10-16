// dh2coop.asi entry point. Loaded by Ultimate ASI Loader (Bin\dinput8.dll) from the game's Bin folder.
// DllMain only spawns a worker thread: waiting/hooking inside DllMain would run under the loader lock.
#include <windows.h>

#include "core/config.h"
#include "core/exports.h"
#include "core/log.h"
#include "probe/probe.h"

namespace {

// Last-resort diagnostics: log the first few severe first-chance exceptions with module+RVA.
LONG CALLBACK crash_logger(EXCEPTION_POINTERS* ep) {
    static thread_local bool busy = false;
    static volatile LONG count = 0;
    if (busy) return EXCEPTION_CONTINUE_SEARCH;
    const DWORD code = ep->ExceptionRecord->ExceptionCode;
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_STACK_OVERFLOW:
        case EXCEPTION_PRIV_INSTRUCTION:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case 0xC0000409:  // STATUS_STACK_BUFFER_OVERRUN / fast-fail
            break;
        default:
            return EXCEPTION_CONTINUE_SEARCH;
    }
    if (InterlockedIncrement(&count) > 16) return EXCEPTION_CONTINUE_SEARCH;
    busy = true;
    const CONTEXT* c = ep->ContextRecord;
    const auto* r = ep->ExceptionRecord;
    spdlog::error(
        "first-chance exception {:#010x} at {} (eip={:#x}) info0={:#x} info1={:#x} eax={:#x} ecx={:#x} "
        "esp={:#x}",
        code, dh2::core::describe_address(r->ExceptionAddress), static_cast<uintptr_t>(c->Eip),
        r->NumberParameters > 0 ? r->ExceptionInformation[0] : 0,
        r->NumberParameters > 1 ? r->ExceptionInformation[1] : 0, static_cast<uintptr_t>(c->Eax),
        static_cast<uintptr_t>(c->Ecx), static_cast<uintptr_t>(c->Esp));
    busy = false;
    return EXCEPTION_CONTINUE_SEARCH;
}

DWORD WINAPI init_thread(LPVOID) {
    using namespace dh2::core;
    const Config cfg = load_config((self_dir() / "dh2coop.ini").string());
    log_init(cfg.log_level);
    spdlog::info("dh2coop {} loaded from {}", "0.1.0-probe", self_path().string());
    AddVectoredExceptionHandler(0, crash_logger);  // 0 = after other vectored handlers
    if (cfg.probe_enabled) dh2::probe::run(cfg);
    return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE self, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(self);
        dh2::core::set_self_module(self);
        if (HANDLE t = CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr)) CloseHandle(t);
    }
    return TRUE;
}
