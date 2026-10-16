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

// Read-only view for UI/debug (game thread only; copy, do not cache pointers).
struct RemoteView {
    uint8_t id = 0xFF;
    std::string name;
    float pos[3] = {};
    float yaw = 0.f;
    uint32_t packets = 0;
    uint32_t rttMs = 0;
};
struct SessionView {
    std::string role;  // "offline" | "host" | "client"
    uint8_t localId = 0xFF;
    uint16_t port = 0;
    bool hunterFound = false;
    float hunterPos[3] = {};
    float hunterYaw = 0.f;
    int animals = 0;
    RemoteView remotes[4];
    int remoteCount = 0;
};
SessionView Snapshot();

// Deferred: applied on the next frame (game thread).
void RequestHost(uint16_t port);
void RequestJoin(const std::string& host, uint16_t port);
void RequestLeave();
std::string StatusLine();

}  // namespace dh2::coop
