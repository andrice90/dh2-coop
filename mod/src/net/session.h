#pragma once

// Host-authoritative ENet session. One object, both roles, no thread of its own.
// Call Service() once per game frame on the game thread. See docs/net-protocol.md.

#include "net/protocol.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace net {

enum class EventType : uint8_t {
    Connected,
    Disconnected,
    TimedOut,
    Hello,
    Welcome,
    Reject,
    PlayerJoined,
    PlayerLeft,
    PlayerState,
    Ping,
    Pong,
    AnimalStates,
    AnimalHit,
    GameEvent,
};

struct Event {
    EventType type = EventType::Connected;
    uint8_t playerId = kInvalidPlayerId;
    RejectReason reason = RejectReason::None;
    uint8_t protoVersion = 0;
    uint16_t gameBuild = 0;
    uint32_t modBuild = 0;  // Hello: the peer's build. Reject on a client: the host's build
    uint8_t maxPlayers = 0;
    uint32_t hostTick = 0;
    uint32_t token = 0;
    std::string name;
    uint8_t model = 0;  // PlayerJoined: that player's puppet model index
    PlayerState state{};
    AnimalStates animalStates{};
    AnimalHit animalHit{};
    GameEvent gameEvent{};
};

struct PeerStats {
    uint8_t playerId = kInvalidPlayerId;
    std::string name;
    uint32_t rttMs = 0;
    float packetLoss = 0.f;  // ENet reliable loss, 0..1
    uint64_t bytesIn = 0;    // v1 payload bytes
    uint64_t bytesOut = 0;
};

class Session {
public:
    struct Identity {
        std::string name = "Hunter";
        uint8_t protoVersion = kProtocolVersion;
        uint16_t gameBuild = kGameBuild;
        uint32_t modBuild = 0;  // hash of dh2coop.asi; host and client must match
        uint32_t passHash = 0;  // 0 = no password
        uint8_t model = 0;      // puppet model index others draw this player with
    };

    using Handler = std::function<void(const Event&)>;

    Session();
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    // maxClients is clamped to 1..3 so the lobby stays at or under 4 players.
    bool StartHost(uint16_t port = kDefaultPort, int maxClients = kDefaultMaxClients, Identity id = {});
    bool Join(const std::string& host, uint16_t port = kDefaultPort, Identity id = {});
    void Disconnect();

    // Non-blocking. Drains the socket and may emit events.
    void Service();

    bool IsHost() const;
    bool IsConnected() const;
    uint8_t LocalPlayerId() const;
    uint16_t BoundPort() const;
    int MaxPlayers() const;
    // Milliseconds since StartHost. 0 on a client.
    uint32_t HostTickMs() const;

    std::vector<PeerStats> Peers() const;

    // Fills playerId with the local id. Host broadcasts; a client sends to the host.
    // The host rewrites a client's id before relaying, so the field cannot be spoofed.
    bool SendPlayerState(const PlayerState& state);
    bool SendPing(uint32_t token);

    // Host only. Broadcasts on the unreliable snapshot channel. Returns false on a client.
    bool SendAnimalStates(const AnimalStates& states);
    // Client only. Reliable to the host. Returns false on the host.
    // The host writes shooterId from the sending peer before the event is queued.
    bool SendAnimalHit(const AnimalHit& hit);

    // Reliable world event on channel 2.
    // A commit (flags bit0) is host → every client. Returns false on a client.
    // A request is client → host. Returns false on the host. The host writes actorId
    // from the sending peer and does not relay the request.
    bool SendGameEvent(const GameEvent& ev);
    // Host only: a commit to one client (late-join catch-up). False on a client or for an unknown id.
    bool SendGameEventTo(uint8_t playerId, const GameEvent& ev);

    // True at most once per kStateIntervalMs. The first call is true.
    // `nowMs` is a steady millisecond clock. Safe to call before connect.
    bool ConsumeSendTick(uint32_t nowMs);

    // If set, events go to the handler and are not queued. Do not call Service or
    // Disconnect from the handler. If unset, DrainEvents() is the consumer.
    void SetHandler(Handler handler);
    std::vector<Event> DrainEvents();

    // How many times this process has called enet_initialize. Overlapping sessions share one.
    static int InitializeCount();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace net
