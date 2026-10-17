#pragma once

// Protocol v1 wire types. Little-endian. No ENet types here.
// Layout and reserved ranges: docs/net-protocol.md.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace net {

constexpr uint8_t kProtocolVersion = 1;
constexpr uint16_t kGameBuild = 434;
constexpr uint16_t kDefaultPort = 27420;
constexpr int kMaxPlayers = 4;
constexpr int kDefaultMaxClients = kMaxPlayers - 1;
constexpr size_t kNameBytes = 32;
constexpr uint8_t kInvalidPlayerId = 0xFF;

constexpr uint32_t kStateHz = 20;
constexpr uint32_t kStateIntervalMs = 1000 / kStateHz;

constexpr uint8_t kChannelControl = 0;  // reliable
constexpr uint8_t kChannelState = 1;    // unreliable sequenced
constexpr uint8_t kChannelEvents = 2;   // reliable: AnimalHit, GameEvent
constexpr uint8_t kChannelCount = 3;

// ENet disconnect data. 0 is a timeout (ENet forces 0) or the initiator's local event.
constexpr uint32_t kDisconnectGraceful = 1;
constexpr uint32_t kDisconnectReject = 2;
constexpr uint32_t kDisconnectShutdown = 3;
constexpr uint32_t kDisconnectHelloTimeout = 4;

constexpr uint32_t kHelloTimeoutMs = 2000;

enum class MsgId : uint8_t {
    Hello = 1,
    Welcome = 2,
    Reject = 3,
    PlayerJoined = 4,
    PlayerLeft = 5,
    PlayerState = 6,
    Ping = 7,
    Pong = 8,
    AnimalStates = 9,
    AnimalHit = 10,
    GameEvent = 48,
};

// Ids 11-15 are unused. Ranges below stay unused except 48 (GameEvent). Do not send them.
// See docs/net-protocol.md.
constexpr uint8_t kReservedControlBegin = 16;  // 16-31
constexpr uint8_t kAnimalStateBegin = 32;      // 32-47
constexpr uint8_t kAnimalEventBegin = 48;      // 48 assigned (GameEvent); 49-63 unused
constexpr uint8_t kHitRequestBegin = 64;       // 64-79
constexpr uint8_t kDamageEventBegin = 80;      // 80-95
constexpr uint8_t kReservedFutureBegin = 96;   // 96-255

constexpr uint8_t kMaxAnimals = 32;
constexpr uint8_t kMaxAnimalStateChars = 31;
constexpr size_t kAnimalStatesHeaderBytes = 8;  // id + seq + tickMs + count
constexpr size_t kAnimalStateFixedBytes = 24;   // one entry, empty name
constexpr size_t kAnimalHitBytes = 20;
constexpr size_t kGameEventHeaderBytes = 18;  // id through key length, empty key
constexpr uint8_t kMaxGameEventKeyChars = 31;

constexpr size_t kHelloBytes = 36;
constexpr size_t kWelcomeBytes = 7;
constexpr size_t kRejectBytes = 2;
constexpr size_t kPlayerJoinedBytes = 34;
constexpr size_t kPlayerLeftBytes = 2;
constexpr size_t kPlayerStateBytes = 44;
constexpr size_t kPingBytes = 5;
constexpr size_t kPongBytes = 5;

namespace player_flag {
constexpr uint16_t kCrouch = 1u << 0;
constexpr uint16_t kProne = 1u << 1;
constexpr uint16_t kAiming = 1u << 2;
constexpr uint16_t kFiring = 1u << 3;
constexpr uint16_t kDead = 1u << 4;
}

enum class RejectReason : uint8_t {
    None = 0,
    VersionMismatch = 1,
    LobbyFull = 2,
    BadHello = 3,
};

struct Hello {
    uint8_t protoVersion = kProtocolVersion;
    uint16_t gameBuild = kGameBuild;
    char name[kNameBytes] = {};
};

struct Welcome {
    uint8_t playerId = kInvalidPlayerId;
    uint32_t hostTick = 0;
    uint8_t maxPlayers = 0;
};

struct Reject {
    RejectReason reason = RejectReason::None;
};

struct PlayerJoined {
    uint8_t playerId = kInvalidPlayerId;
    char name[kNameBytes] = {};
};

struct PlayerLeft {
    uint8_t playerId = kInvalidPlayerId;
};

struct PlayerState {
    uint8_t playerId = kInvalidPlayerId;
    uint16_t seq = 0;
    uint32_t tickMs = 0;
    float pos[3] = {};
    float yaw = 0.f;
    float pitch = 0.f;
    float velocity[3] = {};
    uint16_t flags = 0;
    uint8_t weaponId = 0;
    uint8_t health = 0;
};

// PlayerState.flags bits (Phase 3 actions). Bits 8-11: shot counter (mod 16), bumps once per shot so a puppet
// replays the fire clip even when two shots fall between snapshots.
constexpr uint16_t kFlagCrouched = 1u << 0;
constexpr uint16_t kFlagReloading = 1u << 1;
constexpr uint16_t kFlagFiring = 1u << 2;
constexpr int kShotCounterShift = 8;
constexpr uint16_t kShotCounterMask = 0x0F00;
// PlayerState.weaponId values.
enum WeaponKind : uint8_t { kWeaponNone = 0, kWeaponHandgun = 1, kWeaponShotgun = 2, kWeaponRifle = 3, kWeaponOther = 4 };

struct Ping {
    uint32_t token = 0;
};

struct Pong {
    uint32_t token = 0;
};

namespace animal_flag {
constexpr uint8_t kDead = 1u << 0;
constexpr uint8_t kParked = 1u << 1;
constexpr uint8_t kFrozenOnHost = 1u << 2;
constexpr uint8_t kNpc = 1u << 3;  // companion (obj_guide and other /npc/ models), not cScrAnimal
}

// One animal in an AnimalStates snapshot. `state` is a body-state name.
// Encode writes at most kMaxAnimalStateChars bytes. A longer string is truncated.
struct AnimalState {
    uint16_t id = 0;
    float pos[3] = {};
    float yaw = 0.f;
    float health = 0.f;
    uint8_t flags = 0;
    std::string state;
};

// Host → clients, unreliable. `animals` encodes as a u8 count, at most kMaxAnimals.
struct AnimalStates {
    uint16_t seq = 0;
    uint32_t tickMs = 0;
    std::vector<AnimalState> animals;
};

// Client → host, reliable. The host sets shooterId from the sending peer's player id.
struct AnimalHit {
    uint8_t shooterId = kInvalidPlayerId;
    uint16_t animalId = 0;
    float damage = 0.f;
    float hitPos[3] = {};
};

namespace game_event_flag {
constexpr uint8_t kCommit = 1u << 0;  // host → clients. Clear means client → host request.
}

// One reliable envelope for discrete world outcomes (death, map complete, later objectives).
// `kind` is a game-layer registry. The session does not interpret it. Unknown kinds still decode.
// `key` is a map-specific name (objective, item class). Encode keeps the first 31 bytes.
// A request's actorId is overwritten by the host from the sending peer. A commit keeps the actor
// the host wrote.
struct GameEvent {
    uint8_t flags = 0;
    uint8_t actorId = kInvalidPlayerId;
    uint16_t seq = 0;
    uint16_t kind = 0;
    uint16_t subjectId = 0;
    uint32_t i0 = 0;
    float f0 = 0.f;
    std::string key;

    bool commit() const { return (flags & game_event_flag::kCommit) != 0; }
};

using Message = std::variant<Hello, Welcome, Reject, PlayerJoined, PlayerLeft, PlayerState, Ping, Pong, AnimalStates,
                              AnimalHit, GameEvent>;

constexpr uint8_t ChannelFor(MsgId id) {
    if (id == MsgId::PlayerState || id == MsgId::AnimalStates) return kChannelState;
    if (id == MsgId::AnimalHit || id == MsgId::GameEvent) return kChannelEvents;
    return kChannelControl;
}

constexpr bool IsReliable(MsgId id) { return id != MsgId::PlayerState && id != MsgId::AnimalStates; }

inline void SetName(char (&dst)[kNameBytes], std::string_view src) {
    for (size_t i = 0; i < kNameBytes; ++i) dst[i] = 0;
    const size_t n = src.size() < (kNameBytes - 1) ? src.size() : (kNameBytes - 1);
    for (size_t i = 0; i < n; ++i) dst[i] = src[i];
}

inline std::string NameOf(const char (&src)[kNameBytes]) {
    size_t n = 0;
    while (n < kNameBytes && src[n] != '\0') ++n;
    return std::string(src, n);
}

const char* ToString(MsgId id);
const char* ToString(RejectReason reason);

// Grows a buffer. Never reads.
class ByteWriter {
public:
    void U8(uint8_t v);
    void U16(uint16_t v);
    void U32(uint32_t v);
    void F32(float v);
    void Bytes(const void* data, size_t n);
    void Name(const char (&name)[kNameBytes]);

    const std::vector<uint8_t>& Data() const { return buf_; }
    std::vector<uint8_t> Take() { return std::move(buf_); }

private:
    std::vector<uint8_t> buf_;
};

// Fails closed. A short span sets ok() and returns zeros. It does not read past `len`.
class ByteReader {
public:
    ByteReader(const uint8_t* data, size_t len);

    uint8_t U8();
    uint16_t U16();
    uint32_t U32();
    float F32();
    void Bytes(void* dst, size_t n);
    void Name(char (&name)[kNameBytes]);

    bool ok() const { return ok_; }
    size_t remaining() const { return ok_ && i_ <= n_ ? (n_ - i_) : 0; }

private:
    const uint8_t* p_ = nullptr;
    size_t n_ = 0;
    size_t i_ = 0;
    bool ok_ = false;
};

std::vector<uint8_t> Encode(const Hello& msg);
std::vector<uint8_t> Encode(const Welcome& msg);
std::vector<uint8_t> Encode(const Reject& msg);
std::vector<uint8_t> Encode(const PlayerJoined& msg);
std::vector<uint8_t> Encode(const PlayerLeft& msg);
std::vector<uint8_t> Encode(const PlayerState& msg);
std::vector<uint8_t> Encode(const Ping& msg);
std::vector<uint8_t> Encode(const Pong& msg);
std::vector<uint8_t> Encode(const AnimalStates& msg);
std::vector<uint8_t> Encode(const AnimalHit& msg);
std::vector<uint8_t> Encode(const GameEvent& msg);
std::vector<uint8_t> Encode(const Message& msg);

// Matching id, every field inside the span, no trailing bytes. Otherwise nullopt.
std::optional<Message> Decode(const uint8_t* data, size_t len);

}  // namespace net
