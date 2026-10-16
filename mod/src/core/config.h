#pragma once
// dh2coop.ini (next to the ASI). Read with the Win32 profile API; missing keys keep defaults.
#include <string>

namespace dh2::core {

struct Config {
    // [General]
    std::string log_level = "info";        // trace|debug|info|warn|err
    int poll_ms = 50;                      // module wait poll interval
    int module_wait_timeout_ms = 180000;   // give up waiting for a module after this long

    // [Probe]
    bool probe_enabled = true;
    bool probe_hook_d3d = true;            // Direct3DCreate8/9 + device vtable (CreateDevice/Reset/Present/EndScene)
    bool probe_hook_net = true;            // cNetServer / cNetClient pass-through trace
    bool probe_hook_map = true;            // cBspTree::Load, cLevelMap::Load, cHunterProfile map getters
    bool probe_hook_console = true;        // cConsole::Exec/SetCheats/SetDeveloper, cCmdLine::Search, cApplication::Init
    int probe_state_seconds = 20;          // log engine static state (m_nDriverType, ...) once per second this long
};

// Loads <path>; if the file is missing, writes defaults there first.
Config load_config(const std::string& path);

}  // namespace dh2::core
