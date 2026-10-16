#include "engine/console_cmds.h"

#include <spdlog/spdlog.h>

#include "core/exports.h"
#include "core/safe_mem.h"

namespace dh2::console_cmds {
namespace {

// ?AddStaticFuncf@cConsole@@QAAPAVcConsoleFunc@@IPADP6AXXZZZ (EngineDll7 RVA 0x166bd0)
// cConsoleFunc* __cdecl AddStaticFuncf(cConsole* self, unsigned flags, char* name, void(__cdecl*)(), tag..., -1)
using AddStaticFuncfFn = void*(__cdecl*)(void* self, unsigned flags, const char* name, void(__cdecl* fn)(), ...);

constexpr int kTagString = 7;
constexpr int kTagEnd = -1;

void** g_console_slot = nullptr;
AddStaticFuncfFn g_add = nullptr;

void* console_instance() {
    void* p = nullptr;
    if (!g_console_slot || !core::safe_read(g_console_slot, p)) return nullptr;
    return p;
}

}  // namespace

bool Bind(HMODULE engine) {
    g_console_slot = core::find_export_as<void**>(engine, "?console@@3PAVcConsole@@A");
    g_add = core::find_export_as<AddStaticFuncfFn>(engine, "?AddStaticFuncf@cConsole@@QAAPAVcConsoleFunc@@IPADP6AXXZZZ");
    spdlog::info("console_cmds bind slot={:#x} AddStaticFuncf={}", reinterpret_cast<uintptr_t>(g_console_slot),
                 core::describe_address(reinterpret_cast<void*>(g_add)));
    return g_console_slot && g_add;
}

bool Ready() { return g_add != nullptr && console_instance() != nullptr; }

bool AddCommand0(const char* name, Handler0 fn, unsigned flags) {
    void* self = console_instance();
    if (!self || !g_add || !fn) return false;
    void* func = g_add(self, flags, name, fn, kTagEnd);
    spdlog::info("console command '{}' registered -> cConsoleFunc {:#x}", name, reinterpret_cast<uintptr_t>(func));
    return func != nullptr;
}

bool AddCommandStr(const char* name, HandlerStr fn, unsigned flags) {
    void* self = console_instance();
    if (!self || !g_add || !fn) return false;
    void* func = g_add(self, flags, name, reinterpret_cast<void(__cdecl*)()>(fn), kTagString, kTagEnd);
    spdlog::info("console command '{} <string>' registered -> cConsoleFunc {:#x}", name,
                 reinterpret_cast<uintptr_t>(func));
    return func != nullptr;
}

}  // namespace dh2::console_cmds
