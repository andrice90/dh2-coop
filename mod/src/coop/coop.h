#pragma once
// Co-op session glue: settings, console commands, and the per-frame net pump (game thread).
#include <cstdint>
#include <string>

namespace dh2::coop {

struct CoopSettings {
    bool allow_background = true;   // [Coop] AllowBackground: keep simulating when the window loses focus
    bool disable_slowmo = true;     // [Coop] DisableSlowMo
    std::string player_name = "Hunter";  // [Coop] PlayerName
    uint16_t port = 27420;          // [Coop] Port
    // Command line: -coophost  |  -coopjoin <host[:port]>   (applied after the first map load)
    std::string auto_role;          // "", "host", "join"
    std::string auto_join_addr;
};

const CoopSettings& Settings();

// Deferred: applied on the next frame (game thread).
void RequestHost(uint16_t port);
void RequestJoin(const std::string& host, uint16_t port);
void RequestLeave();
std::string StatusLine();

}  // namespace dh2::coop
