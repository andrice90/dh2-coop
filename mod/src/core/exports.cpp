#include "core/exports.h"

#include <spdlog/spdlog.h>

namespace dh2::core {

HMODULE wait_for_module(const char* name, uint32_t timeout_ms, uint32_t poll_ms) {
    const ULONGLONG start = GetTickCount64();
    for (;;) {
        if (HMODULE m = GetModuleHandleA(name)) return m;
        if (timeout_ms != kNoTimeout && GetTickCount64() - start >= timeout_ms) return nullptr;
        Sleep(poll_ms);
    }
}

void* find_export(HMODULE mod, const char* mangled_name, bool log_missing) {
    void* p = mod ? reinterpret_cast<void*>(GetProcAddress(mod, mangled_name)) : nullptr;
    if (!p && log_missing) spdlog::warn("export not found: {}", mangled_name);
    return p;
}

void* wait_and_resolve(const char* module, const char* mangled_name, uint32_t timeout_ms, uint32_t poll_ms) {
    HMODULE m = wait_for_module(module, timeout_ms, poll_ms);
    if (!m) {
        spdlog::warn("module {} not loaded (timeout) while resolving {}", module, mangled_name);
        return nullptr;
    }
    return find_export(m, mangled_name);
}

std::string module_base_name(HMODULE mod) {
    char buf[MAX_PATH] = {};
    DWORD n = GetModuleFileNameA(mod, buf, MAX_PATH);
    if (!n) return {};
    const char* s = buf;
    for (const char* p = buf; *p; ++p)
        if (*p == '\\' || *p == '/') s = p + 1;
    return s;
}

std::string describe_address(const void* addr) {
    HMODULE m = nullptr;
    if (!addr || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                     reinterpret_cast<LPCSTR>(addr), &m)) {
        return fmt::format("<unknown>+{:#x}", reinterpret_cast<uintptr_t>(addr));
    }
    return fmt::format("{}+{:#x}", module_base_name(m),
                       reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(m));
}

}  // namespace dh2::core
