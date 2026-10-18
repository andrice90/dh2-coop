#pragma once
// Co-op session glue: settings, console commands, and the per-frame net pump (game thread).
#include <cstdint>
#include <string>
#include <vector>

namespace net {
struct AnimalStates;
struct AnimalHit;
struct GameEvent;
struct VehicleStates;
struct HeliStates;
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
    int model = 0;                  // puppet body others see (lobby; [Coop] PuppetModel)
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
    uint16_t flags = 0;  // last PlayerState flags (net::kFlag*)
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
    bool noticeIsBanner = false;  // the main-menu hint, not a timed message (the open lobby hides it)
    std::string tailnetIp;  // this PC's Tailscale IPv4 ("" = Tailscale not up)
    std::string otherIps;   // up to two other IPv4s of this PC
};
SessionView Snapshot();

// ---- Lobby (overlay window at the main menu, F8) ----
// What the lobby fields hold. Kept in dh2coop-lobby.ini beside the logs (game Bin, or %LOCALAPPDATA%\dh2coop for a
// game under Program Files); the first defaults come from dh2coop.ini.
struct LobbyPrefs {
    std::string name;
    int model = 0;  // coop/puppet_models.h index
    std::string join_host;
    uint16_t join_port = 27420;
    uint16_t host_port = 27420;
};
LobbyPrefs LoadLobbyPrefs();
void SaveLobbyPrefs(const LobbyPrefs& prefs);
std::string LobbyIniPath();  // dh2coop-lobby.ini beside the logs

enum class LobbyState { Offline, Hosting, Connecting, Connected, Failed };
struct LobbyPlayer {
    uint8_t id = 0xFF;
    std::string name;
    int model = 0;
    bool you = false;
    uint32_t rttMs = 0;  // 0 for yourself
};
struct LobbyView {
    LobbyState state = LobbyState::Offline;
    std::string message;  // status line: what is happening or what went wrong
    std::vector<LobbyPlayer> players;  // host (id 0) first
    std::string tailnetIp;
    std::string otherIps;
    uint16_t port = 0;  // hosting / joined port
    bool atMenu = false;  // this process is on the main menu map
    std::string map;      // current map name (upper case)
};
LobbyView Lobby();
// Name and model are applied before the session starts (they travel in Hello).
void LobbyHost(const std::string& name, int model, uint16_t port);
void LobbyJoin(const std::string& name, int model, const std::string& host, uint16_t port);
void LobbyLeave();

// ---- Join code and friend key (coop/lobby_invite.cpp) ----
// The host's lobby makes a join code (coop/joincode.h) for friends: this PC's tailnet IP, the host port, the saved
// friend key and the player name. The friend key is a reusable Tailscale auth key (tag:dh2friend) the host
// generates once; it is kept in dh2coop-lobby.ini key [Lobby] FriendKey as base64 of a DPAPI blob (current user),
// never in plain text, and never logged (logs say "friend key saved (N chars)").
// Save: false and `message` filled when the text is not a tskey-auth-... key.
bool SaveFriendKey(const std::string& key, std::string& message);
void ClearFriendKey();
int FriendKeyChars();  // length of the saved key, 0 = none (or not readable by this Windows account)
struct JoinCodeResult {
    std::string code;     // "" when it cannot be made (message says why)
    bool hasKey = false;  // the code carries the friend key
    std::string masked;   // "DH2-…(key hidden)" for logs
    std::string message;  // for the lobby: why not, or what the code holds
};
JoinCodeResult MakeJoinCode(uint16_t port, const std::string& playerName);
// Copy join code button: MakeJoinCode, then the clipboard (CF_UNICODETEXT). `message` is the lobby line to show.
bool CopyJoinCode(uint16_t port, const std::string& playerName, std::string& message);
// Tailscale setup help button: opens Tailscale's policy file and keys pages, copies the ACL snippet for this PC's
// tailnet IP and `port`.
bool OpenTailscaleHelp(uint16_t port, std::string& message);
std::string AclSnippet(const std::string& hostTailnetIp, uint16_t port);
// What the JOIN field makes of its text: a pasted join code fills address and port (the key is ignored there).
struct JoinInput {
    enum class Kind { Empty, Address, Code, BadCode } kind = Kind::Empty;
    std::string host;      // Code: the host's tailnet IP
    uint16_t port = 0;     // Code: the host's port
    std::string hostName;  // Code: the host's player name
    std::string message;   // Code: "Join code accepted: ..."; BadCode: why not
};
JoinInput ParseJoinInput(const std::string& text);
// Test twin of pasting into the JOIN field: the overlay takes this text on its next frame as if it was pasted.
void SetPendingJoinPaste(const std::string& text);
bool TakePendingJoinPaste(std::string& text);
void RegisterInviteCommands();  // coop_friendkey, coop_joincode, coop_joinfield, coop_tshelp

// Host recovery (F5 / F6 and the lobby buttons). The game has no checkpoints inside a level (its "Checkpoint
// reached" is the level exit), so these are the level-sized ones: restart the current level from its start, or go
// on to the next story level. The first call arms (on-screen "press again"), a second within 3 s does it; clients
// follow the map change. Returns false on a client or when there is nothing to do (main menu, last level).
bool HostRestartLevel();
bool HostNextLevel();
// Restart without the second press, for flow.cpp's Restart / replay menu choice (same main-menu hop as F5).
bool HostRestartNow();
// Next story level after `map` ("" after MAP12C or for hunt / bonus maps).
std::string NextStoryMap(const std::string& map);

// Deferred: applied on the next frame (game thread).
void RequestHost(uint16_t port);
void RequestJoin(const std::string& host, uint16_t port);
void RequestLeave();
std::string StatusLine();

// Game thread. Host broadcast / client hit report. False when this process has the wrong role.
bool BroadcastAnimalStates(const net::AnimalStates& states);
bool BroadcastVehicleStates(const net::VehicleStates& states);
bool BroadcastHeliStates(const net::HeliStates& states);
bool ReportAnimalHit(const net::AnimalHit& hit);
bool SendGameEvent(const net::GameEvent& ev);
// Host: a commit to one client only (catch-up after a join or a map change).
bool SendGameEventTo(uint8_t playerId, const net::GameEvent& ev);
// Host: ids of the clients in the session.
std::vector<uint8_t> PeerIds();
bool NetIsHost();
bool NetIsOnline();
// UDP port this process hosts on (the bound port); 0 when it is not hosting.
uint16_t NetHostPort();
// This process's player id (0 = host), net::kInvalidPlayerId (0xFF) before a client is welcomed.
uint8_t NetLocalId();
// Players in this session including this process (1 offline or while hosting alone). Game thread.
int ConnectedPlayers();

// Client: put the local hunter 1.5 m beside the host's last reported position, facing the host's way.
// False when there is no host position yet or no local hunter. Health and death state are not touched.
bool TeleportToHost(const char* why);
// Client: metres between the local hunter and the host's last reported position, or -1 if unknown.
float DistanceToHost();

}  // namespace dh2::coop
