// dh2cmd.asi: baseline stand-in for dh2coop.asi (re/notes/replay-crash.md). It installs no engine hook. It only
//  - renames "GI UNIQUE MUTEX" through Main7's import table, so other games can run beside it;
//  - keeps cApplication::m_bAllowBackground at 1 (an unfocused window keeps running frames);
//  - runs each line of Bin\dh2coop.cmd with cConsole::Exec(console, line, 0) on the game window's thread
//    (window subclass + SendMessage), the same channel dh2coop.asi offers to the test scripts.
// Log: Bin\dh2cmd.log. Build: tools-src\replaycmd\build.ps1.
#include <windows.h>

#include <share.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

char g_dir[MAX_PATH] = {};
FILE* g_log = nullptr;
CRITICAL_SECTION g_log_cs;
char g_mutex_name[64] = {};
HWND g_hwnd = nullptr;
WNDPROC g_prev = nullptr;
const UINT kExecMsg = WM_APP + 0x3d;

using ExecFn = int(__thiscall*)(void* self, char* line, int flag);
void** g_console = nullptr;
ExecFn g_exec = nullptr;
int* g_allow_bg = nullptr;

void logf(const char* fmt, ...) {
    if (!g_log) return;
    EnterCriticalSection(&g_log_cs);
    SYSTEMTIME t;
    GetLocalTime(&t);
    std::fprintf(g_log, "%02d:%02d:%02d.%03d ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(g_log, fmt, ap);
    va_end(ap);
    std::fputc('\n', g_log);
    std::fflush(g_log);
    LeaveCriticalSection(&g_log_cs);
}

HANDLE WINAPI my_CreateMutexA(LPSECURITY_ATTRIBUTES sa, BOOL owner, LPCSTR name) {
    if (name && std::strcmp(name, "GI UNIQUE MUTEX") == 0) {
        std::snprintf(g_mutex_name, sizeof g_mutex_name, "GI UNIQUE MUTEX %lu", GetCurrentProcessId());
        name = g_mutex_name;
    }
    return CreateMutexA(sa, owner, name);
}

// Replace every IAT slot of `mod` that holds `target`.
int patch_iat(HMODULE mod, void* target, void* repl) {
    auto* base = reinterpret_cast<BYTE*>(mod);
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    const IMAGE_DATA_DIRECTORY& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir.VirtualAddress) return 0;
    int n = 0;
    for (auto* d = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + dir.VirtualAddress); d->Name; ++d) {
        for (auto* th = reinterpret_cast<IMAGE_THUNK_DATA*>(base + d->FirstThunk); th->u1.Function; ++th) {
            if (reinterpret_cast<void*>(th->u1.Function) != target) continue;
            DWORD old = 0;
            VirtualProtect(&th->u1.Function, sizeof(void*), PAGE_READWRITE, &old);
            th->u1.Function = reinterpret_cast<ULONG_PTR>(repl);
            VirtualProtect(&th->u1.Function, sizeof(void*), old, &old);
            ++n;
        }
    }
    return n;
}

LRESULT CALLBACK sub_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == kExecMsg) {
        void* console = g_console ? *g_console : nullptr;
        const char* line = reinterpret_cast<const char*>(lp);
        if (!console || !g_exec || !line) return -1;
        char buf[128] = {};
        std::strncpy(buf, line, sizeof buf - 1);  // engine Exec copies into a 0x80 slot
        const int r = g_exec(console, buf, 0);
        logf("exec '%s' -> %d (thread %lu)", buf, r, GetCurrentThreadId());
        return r;
    }
    return CallWindowProcA(g_prev, h, msg, wp, lp);
}

BOOL CALLBACK find_wnd(HWND h, LPARAM lp) {
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid != GetCurrentProcessId() || !IsWindowVisible(h) || GetWindow(h, GW_OWNER)) return TRUE;
    RECT r{};
    GetWindowRect(h, &r);
    auto* best = reinterpret_cast<std::pair<HWND, long>*>(lp);
    const long area = (r.right - r.left) * (r.bottom - r.top);
    if (area > best->second) *best = {h, area};
    return TRUE;
}

void ensure_subclass() {
    if (g_hwnd && IsWindow(g_hwnd) &&
        reinterpret_cast<WNDPROC>(GetWindowLongPtrA(g_hwnd, GWLP_WNDPROC)) == &sub_proc)
        return;
    std::pair<HWND, long> best{nullptr, 0};
    EnumWindows(&find_wnd, reinterpret_cast<LPARAM>(&best));
    if (!best.first || best.second < 320 * 240) return;
    g_hwnd = best.first;
    g_prev = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrA(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&sub_proc)));
    logf("subclassed hwnd %p (area %ld, unicode %d)", g_hwnd, best.second, IsWindowUnicode(g_hwnd) ? 1 : 0);
}

void poll_file() {
    char path[MAX_PATH];
    std::snprintf(path, sizeof path, "%s\\dh2coop.cmd", g_dir);
    FILE* f = std::fopen(path, "rb");
    if (!f) return;
    std::vector<std::string> lines;
    char buf[256];
    while (std::fgets(buf, sizeof buf, f)) {
        std::string s(buf);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
        if (!s.empty()) lines.push_back(s);
    }
    std::fclose(f);
    DeleteFileA(path);
    for (const std::string& l : lines) {
        DWORD_PTR r = 0;
        const LRESULT ok = SendMessageTimeoutA(g_hwnd, kExecMsg, 0, reinterpret_cast<LPARAM>(l.c_str()), SMTO_NORMAL,
                                               60000, &r);
        logf("cmd file '%s' sent=%d result=%d", l.c_str(), ok ? 1 : 0, static_cast<int>(r));
    }
}

DWORD WINAPI worker(LPVOID) {
    HMODULE eng = nullptr;
    while (!(eng = GetModuleHandleA("EngineDll7.dll"))) Sleep(50);
    g_console = reinterpret_cast<void**>(GetProcAddress(eng, "?console@@3PAVcConsole@@A"));
    g_exec = reinterpret_cast<ExecFn>(GetProcAddress(eng, "?Exec@cConsole@@QAEHPADH@Z"));
    g_allow_bg = reinterpret_cast<int*>(GetProcAddress(eng, "?m_bAllowBackground@cApplication@@2HA"));
    logf("EngineDll7 %p console slot %p exec %p allowbg %p", eng, g_console, g_exec, g_allow_bg);
    for (;;) {
        Sleep(500);
        if (g_allow_bg && *g_allow_bg != 1) *g_allow_bg = 1;
        if (!g_console || !*g_console) continue;
        ensure_subclass();
        if (g_hwnd) poll_file();
    }
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE self, DWORD reason, LPVOID) {
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(self);
    InitializeCriticalSection(&g_log_cs);
    GetModuleFileNameA(self, g_dir, MAX_PATH);
    if (char* slash = std::strrchr(g_dir, '\\')) *slash = '\0';
    char path[MAX_PATH];
    std::snprintf(path, sizeof path, "%s\\dh2cmd.log", g_dir);
    g_log = _fsopen(path, "w", _SH_DENYWR);
    void* real = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateMutexA"));
    const int patched = patch_iat(GetModuleHandleA(nullptr), real, reinterpret_cast<void*>(&my_CreateMutexA));
    logf("dh2cmd loaded; cmdline: %s", GetCommandLineA());
    logf("Main7 CreateMutexA IAT slots patched: %d", patched);
    if (HANDLE t = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr)) CloseHandle(t);
    return TRUE;
}
