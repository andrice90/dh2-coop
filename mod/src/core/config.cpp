#include "core/config.h"

#include <windows.h>

#include <cstdio>

namespace dh2::core {

namespace {
bool get_bool(const char* sec, const char* key, bool def, const char* file) {
    return GetPrivateProfileIntA(sec, key, def ? 1 : 0, file) != 0;
}
int get_int(const char* sec, const char* key, int def, const char* file) {
    return static_cast<int>(GetPrivateProfileIntA(sec, key, def, file));
}
std::string get_str(const char* sec, const char* key, const std::string& def, const char* file) {
    char buf[128] = {};
    GetPrivateProfileStringA(sec, key, def.c_str(), buf, sizeof buf, file);
    return buf;
}
}  // namespace

Config load_config(const std::string& path) {
    Config c;
    const char* f = path.c_str();

    if (GetFileAttributesA(f) == INVALID_FILE_ATTRIBUTES) {
        // Write a default ini so the options are discoverable.
        auto w = [&](const char* sec, const char* key, const std::string& v) {
            WritePrivateProfileStringA(sec, key, v.c_str(), f);
        };
        w("General", "LogLevel", c.log_level);
        w("General", "PollMs", std::to_string(c.poll_ms));
        w("General", "ModuleWaitTimeoutMs", std::to_string(c.module_wait_timeout_ms));
        w("Probe", "Enabled", "0");
        w("Probe", "HookD3D", "1");
        w("Probe", "HookNet", "1");
        w("Probe", "HookMap", "1");
        w("Probe", "HookConsole", "1");
        w("Probe", "StateSeconds", std::to_string(c.probe_state_seconds));
        w("Coop", "AllowMultiInstance", "1");
        w("Console", "ExecAfterMap", "");
    }

    c.log_level = get_str("General", "LogLevel", c.log_level, f);
    c.poll_ms = get_int("General", "PollMs", c.poll_ms, f);
    c.module_wait_timeout_ms = get_int("General", "ModuleWaitTimeoutMs", c.module_wait_timeout_ms, f);

    c.probe_enabled = get_bool("Probe", "Enabled", c.probe_enabled, f);
    c.probe_hook_d3d = get_bool("Probe", "HookD3D", c.probe_hook_d3d, f);
    c.probe_hook_net = get_bool("Probe", "HookNet", c.probe_hook_net, f);
    c.probe_hook_map = get_bool("Probe", "HookMap", c.probe_hook_map, f);
    c.probe_hook_console = get_bool("Probe", "HookConsole", c.probe_hook_console, f);
    c.probe_state_seconds = get_int("Probe", "StateSeconds", c.probe_state_seconds, f);

    c.allow_multi_instance = get_bool("Coop", "AllowMultiInstance", c.allow_multi_instance, f);
    c.exec_after_map = get_str("Console", "ExecAfterMap", c.exec_after_map, f);
    return c;
}

namespace {
Config g_config;
}

const Config& config() { return g_config; }
void set_config(const Config& cfg) { g_config = cfg; }

}  // namespace dh2::core
