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
constexpr uint8_t kChannelEvents = 2;   // reliable, reserved
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
};

// Reserved. Do not send. See docs/net-protocol.md.
constexpr uint8_t kReservedControlBegin = 16;  // 16-31
constexpr uint8_t kAnimalStateBegin = 32;      // 32-47
constexpr uint8_t kAnimalEventBegin = 48;      // 48-63
constexpr uint8_t kHitRequestBegin = 64;       // 64-79
constexpr uint8_t kDamageEventBegin = 80;      // 80-95
constexpr uint8_t kReservedFutureBegin = 96;   // 96-255

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

struct Ping {
    uint32_t token = 0;
};

struct Pong {
    uint32_t token = 0;
};

using Message = std::variant<Hello, Welcome, Reject, PlayerJoined, PlayerLeft, PlayerState, Ping, Pong>;

constexpr uint8_t ChannelFor(MsgId id) {
    return id == MsgId::PlayerState ? kChannelState : kChannelControl;
}

constexpr bool IsReliable(MsgId id) { return id != MsgId::PlayerState; }

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
std::vector<uint8_t> Encode(const Message& msg);

// Exact size, matching id, all fields inside the span. Otherwise nullopt.
std::optional<Message> Decode(const uint8_t* data, size_t len);

}  // namespace net
