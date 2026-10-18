#pragma once
// Co-op session glue: settings, console commands, and the per-frame net pump (game thread).
#include <cstdint>
#include <string>
#include <vector>

namespace net {
struct AnimalStates;
struct AnimalHit;
struct GameEvent;
}

namespace dh2::coop {

struct CoopSettings {
    bool allow_background = true;   // [Coop] AllowBackground: keep simulating when the window loses focus
    bool disable_slowmo = true;     // [Coop] DisableSlowMo
    std::string player_name = "Hunter";  // [Coop] PlayerName
    uint16_t port = 27420;          // [Coop] Port
    // [Coop] AutoRole / JoinAddress, overridden by the command line -coophost | -coopjoin <host[:port]>.
    // Applied after the first map load; a joiner retries every join_retry_s seconds while offline.
    std::string auto_role;          // "", "host", "join"
    std::string auto_join_addr;
    float join_retry_s = 5.f;       // [Coop] JoinRetrySeconds (0 = no retry)
    std::string password;           // [Coop] Password: host and clients must match ("" = none)
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
    std::string notice;  // shown on screen even with the overlay hidden (e.g. why the host refused this game)
    std::string tailnetIp;  // this PC's Tailscale IPv4 ("" = Tailscale not up)
    std::string otherIps;   // up to two other IPv4s of this PC
};
SessionView Snapshot();

// Deferred: applied on the next frame (game thread).
void RequestHost(uint16_t port);
void RequestJoin(const std::string& host, uint16_t port);
void RequestLeave();
std::string StatusLine();

// Game thread. Host broadcast / client hit report. False when this process has the wrong role.
bool BroadcastAnimalStates(const net::AnimalStates& states);
bool ReportAnimalHit(const net::AnimalHit& hit);
bool SendGameEvent(const net::GameEvent& ev);
// Host: a commit to one client only (catch-up after a join or a map change).
bool SendGameEventTo(uint8_t playerId, const net::GameEvent& ev);
// Host: ids of the clients in the session.
std::vector<uint8_t> PeerIds();
bool NetIsHost();
bool NetIsOnline();
// This process's player id (0 = host), net::kInvalidPlayerId (0xFF) before a client is welcomed.
uint8_t NetLocalId();

// Client: put the local hunter 1.5 m beside the host's last reported position, facing the host's way.
// False when there is no host position yet or no local hunter. Health and death state are not touched.
bool TeleportToHost(const char* why);
// Client: metres between the local hunter and the host's last reported position, or -1 if unknown.
float DistanceToHost();

}  // namespace dh2::coop
