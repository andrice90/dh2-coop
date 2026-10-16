#include "engine/console.h"

#include <cstring>

#include <spdlog/spdlog.h>

#include "core/config.h"
#include "core/exports.h"
#include "core/safe_mem.h"

namespace dh2::console {
namespace {

// int __thiscall cConsole::Exec(char* line, int flag)  — ?Exec@cConsole@@QAEHPADH@Z
using ExecFn = int(__thiscall*)(void* self, char* line, int flag);
// int __thiscall cConsole::RealExec(char*) — ?RealExec@cConsole@@QAEHPAD@Z
using RealExecFn = int(__thiscall*)(void* self, char* line);

void** g_slot = nullptr;  // address of the exported cConsole* global
ExecFn g_exec = nullptr;
RealExecFn g_real = nullptr;
bool g_exec_after_map_done = false;

void* instance() {
    void* p = nullptr;
    if (!g_slot || !core::safe_read(g_slot, p)) return nullptr;
    return p;
}

// Engine Exec copies into a 0x80-byte slot with no length check (RVA 0x15cd90). Stay under that.
bool copy_line(const char* line, char* buf, size_t cap) {
    if (!line || !buf || cap < 2) return false;
    size_t n = 0;
    while (line[n] != '\0') {
        if (n + 1 >= cap) return false;
        buf[n] = line[n];
        ++n;
    }
    buf[n] = '\0';
    return true;
}

}  // namespace

bool Bind(HMODULE engine) {
    g_slot = core::find_export_as<void**>(engine, "?console@@3PAVcConsole@@A");
    g_exec = core::find_export_as<ExecFn>(engine, "?Exec@cConsole@@QAEHPADH@Z");
    g_real = core::find_export_as<RealExecFn>(engine, "?RealExec@cConsole@@QAEHPAD@Z");
    void* cur = nullptr;
    if (g_slot) core::safe_read(g_slot, cur);
    spdlog::info("console bind slot={:#x} exec={:#x} realexec={:#x} current={:#x}",
                 reinterpret_cast<uintptr_t>(g_slot), reinterpret_cast<uintptr_t>(g_exec),
                 reinterpret_cast<uintptr_t>(g_real), reinterpret_cast<uintptr_t>(cur));
    return g_slot && g_exec && g_real;
}

bool Ready() { return instance() != nullptr && g_exec != nullptr; }

int Exec(const char* line) {
    void* self = instance();
    if (!self || !g_exec || !line) {
        spdlog::warn("console::Exec('{}') skipped; not ready", line ? line : "<null>");
        return -1;
    }
    char buf[128];
    if (!copy_line(line, buf, sizeof buf)) {
        spdlog::warn("console::Exec rejected; line must be shorter than 127 chars");
        return -1;
    }
    // Flag 0: queue the line unchanged. Non-zero prefixes '%' and hits the dev/cheat gates.
    const int r = g_exec(self, buf, 0);
    spdlog::info("console::Exec('{}') -> {}", buf, r);
    return r;
}

int RealExec(const char* line) {
    void* self = instance();
    if (!self || !g_real || !line) {
        spdlog::warn("console::RealExec('{}') skipped; not ready", line ? line : "<null>");
        return -1;
    }
    char buf[128];
    if (!copy_line(line, buf, sizeof buf)) {
        spdlog::warn("console::RealExec rejected; line must be shorter than 127 chars");
        return -1;
    }
    const int r = g_real(self, buf);
    spdlog::info("console::RealExec('{}') -> {}", buf, r);
    return r;
}

void OnMapLoaded(const char* bspName) {
    if (g_exec_after_map_done) return;
    const std::string& cmd = core::config().exec_after_map;
    if (cmd.empty()) return;
    if (!Ready()) {
        spdlog::warn("ExecAfterMap '{}' waiting; console not ready at map '{}'", cmd, bspName ? bspName : "");
        return;
    }
    const int r = Exec(cmd.c_str());
    spdlog::info("ExecAfterMap '{}' after '{}' -> {}", cmd, bspName ? bspName : "", r);
    if (r < 0) return;
    g_exec_after_map_done = true;
}

}  // namespace dh2::console
