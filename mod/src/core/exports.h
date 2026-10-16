#pragma once
// Export-table resolver: the engine DLLs relocate at runtime, so every address is obtained by
// GetProcAddress on the mangled export name (see re/exports/*.exports.txt) - never absolute.
#include <windows.h>

#include <cstdint>
#include <string>

namespace dh2::core {

inline constexpr uint32_t kNoTimeout = 0xFFFFFFFFu;

// Polls GetModuleHandleA(name) every poll_ms until the module is loaded (like the FOV fix does).
// Returns nullptr on timeout. Do NOT call from DllMain (loader lock).
HMODULE wait_for_module(const char* name, uint32_t timeout_ms = kNoTimeout, uint32_t poll_ms = 100);

// GetProcAddress by (mangled) export name; logs a warning if missing and log_missing is set.
void* find_export(HMODULE mod, const char* mangled_name, bool log_missing = true);

template <class T>
T find_export_as(HMODULE mod, const char* mangled_name, bool log_missing = true) {
    return reinterpret_cast<T>(find_export(mod, mangled_name, log_missing));
}

// Waits for `module` (e.g. "EngineDll7.dll") then resolves `mangled_name` in it.
void* wait_and_resolve(const char* module, const char* mangled_name, uint32_t timeout_ms = kNoTimeout,
                       uint32_t poll_ms = 100);

// "EngineDll7.dll+0x2fe07" for any address ("<unknown>+0x..." if not inside a module).
std::string describe_address(const void* addr);

// File name (no path) of a loaded module, "" on failure.
std::string module_base_name(HMODULE mod);

}  // namespace dh2::core
