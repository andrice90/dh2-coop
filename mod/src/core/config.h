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
    bool probe_enabled = false;            // default off; event sources stay on either way
    bool probe_hook_d3d = true;            // D3D9 trace plus PresentCallback logging. D3D8 chain lives in engine/d3d8_hooks
    bool probe_hook_net = true;            // cNetServer / cNetClient pass-through trace
    bool probe_hook_map = true;            // extra cBspTree/cLevelMap/cHunterProfile logging (the map event is always on)
    bool probe_hook_console = true;        // cConsole::Exec/SetCheats/SetDeveloper, cCmdLine::Search, cApplication::Init
    int probe_state_seconds = 20;          // log engine static state (m_nDriverType, ...) once per second this long

    // [Coop]
    bool allow_multi_instance = true;      // rename "GI UNIQUE MUTEX" so a second process can start

    // [Console]
    std::string exec_after_map;            // executed once after the first successful cBspTree::Load; empty = off
};

// Loads <path>; if the file is missing, writes defaults there first.
Config load_config(const std::string& path);

// The config the ASI init thread loaded. Valid after set_config.
const Config& config();
void set_config(const Config& cfg);

}  // namespace dh2::core
