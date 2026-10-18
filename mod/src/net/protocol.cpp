#include "net/protocol.h"

#include <cstring>
#include <utility>

namespace net {

const char* ToString(MsgId id) {
    switch (id) {
    case MsgId::Hello: return "Hello";
    case MsgId::Welcome: return "Welcome";
    case MsgId::Reject: return "Reject";
    case MsgId::PlayerJoined: return "PlayerJoined";
    case MsgId::PlayerLeft: return "PlayerLeft";
    case MsgId::PlayerState: return "PlayerState";
    case MsgId::Ping: return "Ping";
    case MsgId::Pong: return "Pong";
    case MsgId::AnimalStates: return "AnimalStates";
    case MsgId::AnimalHit: return "AnimalHit";
    case MsgId::VehicleStates: return "VehicleStates";
    case MsgId::GameEvent: return "GameEvent";
    }
    return "Unknown";
}

const char* ToString(RejectReason reason) {
    switch (reason) {
    case RejectReason::None: return "none";
    case RejectReason::VersionMismatch: return "version";
    case RejectReason::LobbyFull: return "full";
    case RejectReason::BadHello: return "bad-hello";
    case RejectReason::BuildMismatch: return "different mod build";
    case RejectReason::BadPassword: return "wrong password";
    }
    return "unknown";
}

void ByteWriter::U8(uint8_t v) { buf_.push_back(v); }

void ByteWriter::U16(uint16_t v) {
    buf_.push_back(static_cast<uint8_t>(v & 0xFF));
    buf_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

void ByteWriter::U32(uint32_t v) {
    buf_.push_back(static_cast<uint8_t>(v & 0xFF));
    buf_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf_.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buf_.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

void ByteWriter::F32(float v) {
    uint32_t bits = 0;
    static_assert(sizeof(float) == 4, "binary32");
    std::memcpy(&bits, &v, 4);
    U32(bits);
}

void ByteWriter::Bytes(const void* data, size_t n) {
    const auto* b = static_cast<const uint8_t*>(data);
    buf_.insert(buf_.end(), b, b + n);
}

void ByteWriter::Name(const char (&name)[kNameBytes]) { Bytes(name, kNameBytes); }

ByteReader::ByteReader(const uint8_t* data, size_t len) : p_(data), n_(len), i_(0), ok_(data != nullptr || len == 0) {
    if (len > 0 && data == nullptr) ok_ = false;
}

uint8_t ByteReader::U8() {
    if (!ok_ || i_ + 1 > n_) {
        ok_ = false;
        return 0;
    }
    return p_[i_++];
}

uint16_t ByteReader::U16() {
    if (!ok_ || i_ + 2 > n_) {
        ok_ = false;
        return 0;
    }
    const uint16_t v = static_cast<uint16_t>(p_[i_]) | (static_cast<uint16_t>(p_[i_ + 1]) << 8);
    i_ += 2;
    return v;
}

uint32_t ByteReader::U32() {
    if (!ok_ || i_ + 4 > n_) {
        ok_ = false;
        return 0;
    }
    const uint32_t v = static_cast<uint32_t>(p_[i_]) | (static_cast<uint32_t>(p_[i_ + 1]) << 8) |
                       (static_cast<uint32_t>(p_[i_ + 2]) << 16) | (static_cast<uint32_t>(p_[i_ + 3]) << 24);
    i_ += 4;
    return v;
}

float ByteReader::F32() {
    const uint32_t bits = U32();
    float v = 0.f;
    std::memcpy(&v, &bits, 4);
    return v;
}

void ByteReader::Bytes(void* dst, size_t n) {
    if (!ok_ || i_ + n > n_) {
        ok_ = false;
        if (dst != nullptr && n > 0) std::memset(dst, 0, n);
        return;
    }
    std::memcpy(dst, p_ + i_, n);
    i_ += n;
}

void ByteReader::Name(char (&name)[kNameBytes]) { Bytes(name, kNameBytes); }

namespace {

template <typename T, typename Fill>
std::optional<T> ParseExact(const uint8_t* data, size_t len, size_t expect, MsgId id, Fill fill) {
    if (data == nullptr || len != expect || data[0] != static_cast<uint8_t>(id)) return std::nullopt;
    ByteReader r(data + 1, len - 1);
    T msg{};
    fill(r, msg);
    if (!r.ok() || r.remaining() != 0) return std::nullopt;
    return msg;
}

std::optional<VehicleStates> ParseVehicleStates(const uint8_t* data, size_t len) {
    if (data == nullptr || len < kVehicleStatesHeaderBytes || data[0] != static_cast<uint8_t>(MsgId::VehicleStates)) {
        return std::nullopt;
    }
    ByteReader r(data + 1, len - 1);
    VehicleStates msg;
    msg.tickMs = r.U32();
    msg.mapSerial = r.U16();
    const uint8_t count = r.U8();
    if (!r.ok() || count > kMaxVehicles || len != kVehicleStatesHeaderBytes + count * kVehicleStateBytes) {
        return std::nullopt;
    }
    msg.vehicles.resize(count);
    for (auto& v : msg.vehicles) {
        v.id = r.U16();
        for (float& f : v.rot) f = r.F32();
        for (float& f : v.pos) f = r.F32();
    }
    if (!r.ok() || r.remaining() != 0) return std::nullopt;
    return msg;
}

std::optional<AnimalStates> ParseAnimalStates(const uint8_t* data, size_t len) {
    if (data == nullptr || len < kAnimalStatesHeaderBytes || data[0] != static_cast<uint8_t>(MsgId::AnimalStates)) {
        return std::nullopt;
    }
    ByteReader r(data + 1, len - 1);
    AnimalStates msg;
    msg.seq = r.U16();
    msg.tickMs = r.U32();
    msg.mapSerial = r.U16();
    const uint8_t count = r.U8();
    if (!r.ok() || count > kMaxAnimals || static_cast<size_t>(count) > r.remaining()) return std::nullopt;
    msg.animals.reserve(count);
    for (uint8_t i = 0; i < count; ++i) {
        AnimalState animal;
        animal.id = r.U16();
        animal.pos[0] = r.F32();
        animal.pos[1] = r.F32();
        animal.pos[2] = r.F32();
        animal.yaw = r.F32();
        animal.health = r.F32();
        animal.flags = r.U8();
        const uint8_t nameLen = r.U8();
        if (!r.ok() || nameLen > kMaxAnimalStateChars || static_cast<size_t>(nameLen) > r.remaining()) return std::nullopt;
        uint8_t raw[kMaxAnimalStateChars];
        r.Bytes(raw, nameLen);
        if (!r.ok()) return std::nullopt;
        animal.state.assign(reinterpret_cast<const char*>(raw), nameLen);
        const uint8_t armsLen = r.U8();
        if (!r.ok() || armsLen > kMaxAnimalStateChars || static_cast<size_t>(armsLen) > r.remaining()) return std::nullopt;
        r.Bytes(raw, armsLen);
        if (!r.ok()) return std::nullopt;
        animal.arms.assign(reinterpret_cast<const char*>(raw), armsLen);
        msg.animals.push_back(std::move(animal));
    }
    if (!r.ok() || r.remaining() != 0) return std::nullopt;
    return msg;
}

std::optional<GameEvent> ParseGameEvent(const uint8_t* data, size_t len) {
    if (data == nullptr || len < kGameEventHeaderBytes || data[0] != static_cast<uint8_t>(MsgId::GameEvent)) {
        return std::nullopt;
    }
    ByteReader r(data + 1, len - 1);
    GameEvent msg;
    msg.flags = r.U8();
    msg.actorId = r.U8();
    msg.seq = r.U16();
    msg.kind = r.U16();
    msg.subjectId = r.U16();
    msg.i0 = r.U32();
    msg.f0 = r.F32();
    const uint8_t keyLen = r.U8();
    if (!r.ok() || keyLen > kMaxGameEventKeyChars || static_cast<size_t>(keyLen) != r.remaining()) return std::nullopt;
    uint8_t raw[kMaxGameEventKeyChars];
    r.Bytes(raw, keyLen);
    if (!r.ok() || r.remaining() != 0) return std::nullopt;
    msg.key.assign(reinterpret_cast<const char*>(raw), keyLen);
    return msg;
}

}  // namespace

std::vector<uint8_t> Encode(const Hello& msg) {
    ByteWriter w;
    w.U8(static_cast<uint8_t>(MsgId::Hello));
    w.U8(msg.protoVersion);
    w.U16(msg.gameBuild);
    w.Name(msg.name);
    w.U32(msg.modBuild);
    w.U32(msg.passHash);
    w.U8(msg.model);
    return w.Take();
}

std::vector<uint8_t> Encode(const Welcome& msg) {
    ByteWriter w;
    w.U8(static_cast<uint8_t>(MsgId::Welcome));
    w.U8(msg.playerId);
    w.U32(msg.hostTick);
    w.U8(msg.maxPlayers);
    return w.Take();
}

std::vector<uint8_t> Encode(const Reject& msg) {
    ByteWriter w;
    w.U8(static_cast<uint8_t>(MsgId::Reject));
    w.U8(static_cast<uint8_t>(msg.reason));
    w.U32(msg.hostBuild);
    return w.Take();
}

std::vector<uint8_t> Encode(const PlayerJoined& msg) {
    ByteWriter w;
    w.U8(static_cast<uint8_t>(MsgId::PlayerJoined));
    w.U8(msg.playerId);
    w.Name(msg.name);
    w.U8(msg.model);
    return w.Take();
}

std::vector<uint8_t> Encode(const PlayerLeft& msg) {
    ByteWriter w;
    w.U8(static_cast<uint8_t>(MsgId::PlayerLeft));
    w.U8(msg.playerId);
    return w.Take();
}

std::vector<uint8_t> Encode(const PlayerState& msg) {
    ByteWriter w;
    w.U8(static_cast<uint8_t>(MsgId::PlayerState));
    w.U8(msg.playerId);
    w.U16(msg.seq);
    w.U32(msg.tickMs);
    w.F32(msg.pos[0]);
    w.F32(msg.pos[1]);
    w.F32(msg.pos[2]);
    w.F32(msg.yaw);
    w.F32(msg.pitch);
    w.F32(msg.velocity[0]);
    w.F32(msg.velocity[1]);
    w.F32(msg.velocity[2]);
    w.U16(msg.flags);
    w.U8(msg.weaponId);
    w.U8(msg.health);
    return w.Take();
}

std::vector<uint8_t> Encode(const Ping& msg) {
    ByteWriter w;
    w.U8(static_cast<uint8_t>(MsgId::Ping));
    w.U32(msg.token);
    return w.Take();
}

std::vector<uint8_t> Encode(const Pong& msg) {
    ByteWriter w;
    w.U8(static_cast<uint8_t>(MsgId::Pong));
    w.U32(msg.token);
    return w.Take();
}

std::vector<uint8_t> Encode(const AnimalStates& msg) {
    ByteWriter w;
    w.U8(static_cast<uint8_t>(MsgId::AnimalStates));
    w.U16(msg.seq);
    w.U32(msg.tickMs);
    w.U16(msg.mapSerial);
    const size_t count = msg.animals.size() > kMaxAnimals ? kMaxAnimals : msg.animals.size();
    w.U8(static_cast<uint8_t>(count));
    for (size_t i = 0; i < count; ++i) {
        const AnimalState& animal = msg.animals[i];
        w.U16(animal.id);
        w.F32(animal.pos[0]);
        w.F32(animal.pos[1]);
        w.F32(animal.pos[2]);
        w.F32(animal.yaw);
        w.F32(animal.health);
        w.U8(animal.flags);
        const size_t nameLen = animal.state.size() > kMaxAnimalStateChars ? kMaxAnimalStateChars : animal.state.size();
        w.U8(static_cast<uint8_t>(nameLen));
        if (nameLen > 0) w.Bytes(animal.state.data(), nameLen);
        const size_t armsLen = animal.arms.size() > kMaxAnimalStateChars ? kMaxAnimalStateChars : animal.arms.size();
        w.U8(static_cast<uint8_t>(armsLen));
        if (armsLen > 0) w.Bytes(animal.arms.data(), armsLen);
    }
    return w.Take();
}

std::vector<uint8_t> Encode(const AnimalHit& msg) {
    ByteWriter w;
    w.U8(static_cast<uint8_t>(MsgId::AnimalHit));
    w.U8(msg.shooterId);
    w.U16(msg.animalId);
    w.F32(msg.damage);
    w.F32(msg.hitPos[0]);
    w.F32(msg.hitPos[1]);
    w.F32(msg.hitPos[2]);
    return w.Take();
}

std::vector<uint8_t> Encode(const VehicleStates& msg) {
    ByteWriter w;
    w.U8(static_cast<uint8_t>(MsgId::VehicleStates));
    w.U32(msg.tickMs);
    w.U16(msg.mapSerial);
    const size_t n = msg.vehicles.size() < kMaxVehicles ? msg.vehicles.size() : kMaxVehicles;
    w.U8(static_cast<uint8_t>(n));
    for (size_t i = 0; i < n; ++i) {
        const VehicleState& v = msg.vehicles[i];
        w.U16(v.id);
        for (float f : v.rot) w.F32(f);
        for (float f : v.pos) w.F32(f);
    }
    return w.Take();
}

std::vector<uint8_t> Encode(const GameEvent& msg) {
    ByteWriter w;
    w.U8(static_cast<uint8_t>(MsgId::GameEvent));
    w.U8(msg.flags);
    w.U8(msg.actorId);
    w.U16(msg.seq);
    w.U16(msg.kind);
    w.U16(msg.subjectId);
    w.U32(msg.i0);
    w.F32(msg.f0);
    const size_t keyLen = msg.key.size() > kMaxGameEventKeyChars ? kMaxGameEventKeyChars : msg.key.size();
    w.U8(static_cast<uint8_t>(keyLen));
    if (keyLen > 0) w.Bytes(msg.key.data(), keyLen);
    return w.Take();
}

std::vector<uint8_t> Encode(const Message& msg) {
    return std::visit([](const auto& m) { return Encode(m); }, msg);
}

std::optional<Message> Decode(const uint8_t* data, size_t len) {
    if (data == nullptr || len == 0) return std::nullopt;
    switch (static_cast<MsgId>(data[0])) {
    case MsgId::Hello: {
        const bool v1 = len == kHelloV1Bytes;
        const bool v2 = len == kHelloV2Bytes;
        auto m = ParseExact<Hello>(data, len, v1 ? kHelloV1Bytes : (v2 ? kHelloV2Bytes : kHelloBytes), MsgId::Hello,
                                   [v1, v2](ByteReader& r, Hello& o) {
                                       o.protoVersion = r.U8();
                                       o.gameBuild = r.U16();
                                       r.Name(o.name);
                                       if (v1) return;
                                       o.modBuild = r.U32();
                                       o.passHash = r.U32();
                                       if (v2) return;
                                       o.model = r.U8();
                                   });
        if (!m) return std::nullopt;
        return Message{std::move(*m)};
    }
    case MsgId::Welcome: {
        auto m = ParseExact<Welcome>(data, len, kWelcomeBytes, MsgId::Welcome, [](ByteReader& r, Welcome& o) {
            o.playerId = r.U8();
            o.hostTick = r.U32();
            o.maxPlayers = r.U8();
        });
        if (!m) return std::nullopt;
        return Message{std::move(*m)};
    }
    case MsgId::Reject: {
        const bool v1 = len == kRejectV1Bytes;
        auto m = ParseExact<Reject>(data, len, v1 ? kRejectV1Bytes : kRejectBytes, MsgId::Reject,
                                    [v1](ByteReader& r, Reject& o) {
                                        o.reason = static_cast<RejectReason>(r.U8());
                                        if (!v1) o.hostBuild = r.U32();
                                    });
        if (!m) return std::nullopt;
        return Message{std::move(*m)};
    }
    case MsgId::PlayerJoined: {
        auto m = ParseExact<PlayerJoined>(data, len, kPlayerJoinedBytes, MsgId::PlayerJoined,
                                           [](ByteReader& r, PlayerJoined& o) {
                                               o.playerId = r.U8();
                                               r.Name(o.name);
                                               o.model = r.U8();
                                           });
        if (!m) return std::nullopt;
        return Message{std::move(*m)};
    }
    case MsgId::PlayerLeft: {
        auto m = ParseExact<PlayerLeft>(data, len, kPlayerLeftBytes, MsgId::PlayerLeft, [](ByteReader& r, PlayerLeft& o) {
            o.playerId = r.U8();
        });
        if (!m) return std::nullopt;
        return Message{std::move(*m)};
    }
    case MsgId::PlayerState: {
        auto m = ParseExact<PlayerState>(data, len, kPlayerStateBytes, MsgId::PlayerState,
                                          [](ByteReader& r, PlayerState& o) {
                                              o.playerId = r.U8();
                                              o.seq = r.U16();
                                              o.tickMs = r.U32();
                                              o.pos[0] = r.F32();
                                              o.pos[1] = r.F32();
                                              o.pos[2] = r.F32();
                                              o.yaw = r.F32();
                                              o.pitch = r.F32();
                                              o.velocity[0] = r.F32();
                                              o.velocity[1] = r.F32();
                                              o.velocity[2] = r.F32();
                                              o.flags = r.U16();
                                              o.weaponId = r.U8();
                                              o.health = r.U8();
                                          });
        if (!m) return std::nullopt;
        return Message{std::move(*m)};
    }
    case MsgId::Ping: {
        auto m = ParseExact<Ping>(data, len, kPingBytes, MsgId::Ping, [](ByteReader& r, Ping& o) { o.token = r.U32(); });
        if (!m) return std::nullopt;
        return Message{std::move(*m)};
    }
    case MsgId::Pong: {
        auto m = ParseExact<Pong>(data, len, kPongBytes, MsgId::Pong, [](ByteReader& r, Pong& o) { o.token = r.U32(); });
        if (!m) return std::nullopt;
        return Message{std::move(*m)};
    }
    case MsgId::AnimalStates: {
        auto m = ParseAnimalStates(data, len);
        if (!m) return std::nullopt;
        return Message{std::move(*m)};
    }
    case MsgId::AnimalHit: {
        auto m = ParseExact<AnimalHit>(data, len, kAnimalHitBytes, MsgId::AnimalHit, [](ByteReader& r, AnimalHit& o) {
            o.shooterId = r.U8();
            o.animalId = r.U16();
            o.damage = r.F32();
            o.hitPos[0] = r.F32();
            o.hitPos[1] = r.F32();
            o.hitPos[2] = r.F32();
        });
        if (!m) return std::nullopt;
        return Message{std::move(*m)};
    }
    case MsgId::VehicleStates: {
        auto m = ParseVehicleStates(data, len);
        if (!m) return std::nullopt;
        return Message{std::move(*m)};
    }
    case MsgId::GameEvent: {
        auto m = ParseGameEvent(data, len);
        if (!m) return std::nullopt;
        return Message{std::move(*m)};
    }
    }
    return std::nullopt;
}

}  // namespace net
