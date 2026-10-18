#include "net/interp.h"
#include "net/protocol.h"
#include "net/session.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <optional>
#include <random>
#include <string>
#include <variant>
#include <vector>

namespace {

struct Tally {
    int pass = 0;
    int fail = 0;
    std::vector<std::string> failures;

    void Check(bool ok, const std::string& what) {
        if (ok) {
            ++pass;
            return;
        }
        ++fail;
        if (failures.size() < 24) failures.push_back(what);
        std::fprintf(stderr, "FAIL %s\n", what.c_str());
    }
};

struct Section {
    const char* name = "";
    bool ok[2] = {false, false};
};

struct Digest {
    int ids[4] = {-1, -1, -1, -1};
    int relay[4][4] = {};
    int rejectV = 0;
    int rejectF = 0;
    int left = 0;
    int down = 0;
    int initDelta = 0;
    int fails = 0;
};

bool Same(const Digest& a, const Digest& b) {
    if (a.rejectV != b.rejectV || a.rejectF != b.rejectF || a.left != b.left || a.down != b.down) return false;
    if (a.initDelta != b.initDelta) return false;
    for (int i = 0; i < 4; ++i) {
        if (a.ids[i] != b.ids[i]) return false;
        for (int j = 0; j < 4; ++j)
            if (a.relay[i][j] != b.relay[i][j]) return false;
    }
    return true;
}

using Clock = std::chrono::steady_clock;

int ElapsedMs(Clock::time_point start) {
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count());
}

float AngDist(float a, float b) {
    float d = std::fmod(std::fabs(a - b), 360.f);
    if (d > 180.f) d = 360.f - d;
    return d;
}

// Independent of SnapshotBuffer, so a shared wrap bug cannot hide.
float ShortYaw(float a, float b, float t) {
    float d = b - a;
    while (d > 180.f) d -= 360.f;
    while (d < -180.f) d += 360.f;
    float r = a + d * t;
    while (r > 180.f) r -= 360.f;
    while (r <= -180.f) r += 360.f;
    return r;
}

bool Near(float a, float b, float eps = 0.001f) { return std::fabs(a - b) <= eps; }

bool SameBytes(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    return a.size() == b.size() && (a.empty() || std::memcmp(a.data(), b.data(), a.size()) == 0);
}

bool IsAscii(const std::string& s) {
    for (unsigned char c : s) {
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

bool SameAnimal(const net::AnimalState& a, const net::AnimalState& b) {
    if (a.id != b.id || a.flags != b.flags || a.state != b.state) return false;
    if (std::memcmp(a.pos, b.pos, sizeof a.pos) != 0) return false;
    if (std::memcmp(&a.yaw, &b.yaw, sizeof a.yaw) != 0) return false;
    return std::memcmp(&a.health, &b.health, sizeof a.health) == 0;
}

bool SameAnimals(const net::AnimalStates& a, const net::AnimalStates& b) {
    if (a.seq != b.seq || a.tickMs != b.tickMs || a.animals.size() != b.animals.size()) return false;
    for (size_t i = 0; i < a.animals.size(); ++i) {
        if (!SameAnimal(a.animals[i], b.animals[i])) return false;
    }
    return true;
}

bool SameHit(const net::AnimalHit& a, const net::AnimalHit& b) {
    if (a.shooterId != b.shooterId || a.animalId != b.animalId) return false;
    if (std::memcmp(&a.damage, &b.damage, sizeof a.damage) != 0) return false;
    return std::memcmp(a.hitPos, b.hitPos, sizeof a.hitPos) == 0;
}

bool SameEvent(const net::GameEvent& a, const net::GameEvent& b) {
    if (a.flags != b.flags || a.actorId != b.actorId || a.seq != b.seq || a.kind != b.kind || a.subjectId != b.subjectId ||
        a.i0 != b.i0 || a.key != b.key) {
        return false;
    }
    return std::memcmp(&a.f0, &b.f0, sizeof a.f0) == 0;
}

net::AnimalStates WireForm(net::AnimalStates msg) {
    if (msg.animals.size() > net::kMaxAnimals) msg.animals.resize(net::kMaxAnimals);
    for (auto& animal : msg.animals) {
        if (animal.state.size() > net::kMaxAnimalStateChars) animal.state.resize(net::kMaxAnimalStateChars);
    }
    return msg;
}

net::AnimalStates MakeHerd() {
    net::AnimalStates many;
    many.seq = 9;
    many.tickMs = 123456;
    const std::string forty = "0123456789abcdefghijklmnopqrstuvwxyzABCD";
    for (int i = 0; i < 32; ++i) {
        net::AnimalState animal;
        animal.id = static_cast<uint16_t>(120 + i);
        animal.pos[0] = static_cast<float>(i) - 10.f;
        animal.pos[1] = 0.25f * static_cast<float>(i);
        animal.pos[2] = (i == 31) ? -10000.f : static_cast<float>(i);
        animal.yaw = 90.f * static_cast<float>(i % 4);
        animal.health = (i == 2) ? 0.f : 45.5f;
        animal.flags = static_cast<uint8_t>(i & 0x7);
        if (i == 0) animal.state = forty;
        else if (i == 1) animal.state.clear();
        else if (i % 2 == 0) animal.state = "Walk";
        else animal.state = "IdleAttack";
        many.animals.push_back(std::move(animal));
    }
    return many;
}

const char* EventName(net::EventType t) {
    switch (t) {
    case net::EventType::Connected: return "Connected";
    case net::EventType::Disconnected: return "Disconnected";
    case net::EventType::TimedOut: return "TimedOut";
    case net::EventType::Hello: return "Hello";
    case net::EventType::Welcome: return "Welcome";
    case net::EventType::Reject: return "Reject";
    case net::EventType::PlayerJoined: return "PlayerJoined";
    case net::EventType::PlayerLeft: return "PlayerLeft";
    case net::EventType::PlayerState: return "PlayerState";
    case net::EventType::Ping: return "Ping";
    case net::EventType::Pong: return "Pong";
    case net::EventType::AnimalStates: return "AnimalStates";
    case net::EventType::AnimalHit: return "AnimalHit";
    case net::EventType::GameEvent: return "GameEvent";
    }
    return "?";
}

void TestCodec(Tally& t) {
    using namespace net;
    t.Check(ChannelFor(MsgId::PlayerState) == kChannelState, "state channel");
    t.Check(ChannelFor(MsgId::Hello) == kChannelControl, "hello channel");
    t.Check(ChannelFor(MsgId::AnimalStates) == kChannelState, "animal state channel");
    t.Check(ChannelFor(MsgId::AnimalHit) == kChannelEvents, "animal hit channel");
    t.Check(ChannelFor(MsgId::GameEvent) == kChannelEvents, "game event channel");
    t.Check(IsReliable(MsgId::Hello) && !IsReliable(MsgId::PlayerState), "reliability");
    t.Check(!IsReliable(MsgId::AnimalStates) && IsReliable(MsgId::AnimalHit) && IsReliable(MsgId::GameEvent),
            "animal reliability");

    Hello hello;
    hello.protoVersion = 1;
    hello.gameBuild = 434;
    hello.modBuild = 0x12345678u;
    hello.passHash = 0x9ABCDEF0u;
    hello.model = 7;
    SetName(hello.name, "Hunter-123456789012345678901234567890EXTRA");
    auto hb = Encode(hello);
    t.Check(hb.size() == kHelloBytes, "hello size");
    auto hd = Decode(hb.data(), hb.size());
    t.Check(hd.has_value() && std::holds_alternative<Hello>(*hd), "hello type");
    if (hd && std::holds_alternative<Hello>(*hd)) {
        const auto& m = std::get<Hello>(*hd);
        const std::string name = NameOf(m.name);
        t.Check(m.protoVersion == 1 && m.gameBuild == 434 && m.modBuild == 0x12345678u && m.passHash == 0x9ABCDEF0u &&
                    m.model == 7,
                "hello fields");
        t.Check(name.size() == 31 && m.name[31] == '\0', "name truncated to 31 and nul terminated");
        t.Check(name == "Hunter-123456789012345678901234", "name keeps the first 31 bytes");
        t.Check(SameBytes(Encode(m), hb), "hello canonical");
    }

    Welcome welcome{2, 0x01020304u, 4};
    auto wb = Encode(welcome);
    t.Check(wb.size() == kWelcomeBytes, "welcome size");
    auto wd = Decode(wb.data(), wb.size());
    t.Check(wd && std::holds_alternative<Welcome>(*wd) && std::get<Welcome>(*wd).playerId == 2 &&
                std::get<Welcome>(*wd).hostTick == 0x01020304u && std::get<Welcome>(*wd).maxPlayers == 4,
            "welcome round trip");

    for (RejectReason reason : {RejectReason::None, RejectReason::VersionMismatch, RejectReason::LobbyFull,
                                 RejectReason::BadHello, RejectReason::BuildMismatch, RejectReason::BadPassword}) {
        auto b = Encode(Reject{reason, 0xA1B2C3D4u});
        auto d = Decode(b.data(), b.size());
        t.Check(b.size() == kRejectBytes && d && std::holds_alternative<Reject>(*d) && std::get<Reject>(*d).reason == reason &&
                    std::get<Reject>(*d).hostBuild == 0xA1B2C3D4u,
                std::string("reject ") + ToString(reason));
    }
    {
        // v1 Reject (2 bytes) from an old host still decodes.
        const uint8_t v1[] = {static_cast<uint8_t>(MsgId::Reject), static_cast<uint8_t>(RejectReason::BadHello)};
        auto d = Decode(v1, sizeof v1);
        t.Check(d && std::holds_alternative<Reject>(*d) && std::get<Reject>(*d).reason == RejectReason::BadHello &&
                    std::get<Reject>(*d).hostBuild == 0,
                "v1 reject decodes");
        // v1 Hello (36 bytes) from an old client decodes, so the host can answer VersionMismatch.
        Hello old;
        old.protoVersion = 1;
        SetName(old.name, "Old");
        auto ob = Encode(old);
        ob.resize(kHelloV1Bytes);
        auto od = Decode(ob.data(), ob.size());
        t.Check(od && std::holds_alternative<Hello>(*od) && std::get<Hello>(*od).protoVersion == 1 &&
                    std::get<Hello>(*od).modBuild == 0 && NameOf(std::get<Hello>(*od).name) == "Old",
                "v1 hello decodes");
        ob.push_back(0);
        t.Check(!Decode(ob.data(), ob.size()).has_value(), "37-byte hello rejected");
        // v2 Hello (44 bytes, no model) decodes with model 0.
        Hello v2;
        v2.protoVersion = 2;
        v2.modBuild = 0x01020304u;
        v2.model = 9;
        auto vb = Encode(v2);
        vb.resize(kHelloV2Bytes);
        auto vd = Decode(vb.data(), vb.size());
        t.Check(vd && std::holds_alternative<Hello>(*vd) && std::get<Hello>(*vd).protoVersion == 2 &&
                    std::get<Hello>(*vd).modBuild == 0x01020304u && std::get<Hello>(*vd).model == 0,
                "v2 hello decodes");
    }
    {
        VehicleStates vs;
        vs.tickMs = 123456;
        vs.mapSerial = 7;
        VehicleState v;
        v.id = 68;
        for (int i = 0; i < 9; ++i) v.rot[i] = static_cast<float>(i) * 0.5f;
        v.pos[0] = -312.5f;
        v.pos[1] = 0.65f;
        v.pos[2] = -64.25f;
        vs.vehicles.push_back(v);
        auto b = Encode(vs);
        auto d = Decode(b.data(), b.size());
        t.Check(b.size() == kVehicleStatesHeaderBytes + kVehicleStateBytes && d &&
                    std::holds_alternative<VehicleStates>(*d) && std::get<VehicleStates>(*d).vehicles.size() == 1 &&
                    std::get<VehicleStates>(*d).vehicles[0].id == 68 &&
                    std::get<VehicleStates>(*d).vehicles[0].rot[8] == 4.f &&
                    std::get<VehicleStates>(*d).vehicles[0].pos[2] == -64.25f && std::get<VehicleStates>(*d).mapSerial == 7,
                "vehicle states round trip");
        t.Check(!IsReliable(MsgId::VehicleStates) && ChannelFor(MsgId::VehicleStates) == kChannelState,
                "vehicle states unreliable on the state channel");
    }
    {
        HeliStates hs;
        hs.tickMs = 98765;
        hs.mapSerial = 3;
        HeliState h;
        h.id = 66;
        h.state = "Loop1";
        h.anim = "loop1";
        h.elapsed = 4.5f;
        h.speed = 1.f;
        h.length = 12.25f;
        h.pos[1] = 31.5f;
        h.flags = heli_flag::kVisible;
        hs.helis.push_back(h);
        auto b = Encode(hs);
        auto d = Decode(b.data(), b.size());
        t.Check(b.size() == kHeliStatesHeaderBytes + kHeliStateFixedBytes + 5 + 5 && d &&
                    std::holds_alternative<HeliStates>(*d) && std::get<HeliStates>(*d).helis.size() == 1 &&
                    std::get<HeliStates>(*d).helis[0].id == 66 && std::get<HeliStates>(*d).helis[0].state == "Loop1" &&
                    std::get<HeliStates>(*d).helis[0].anim == "loop1" &&
                    std::get<HeliStates>(*d).helis[0].elapsed == 4.5f &&
                    std::get<HeliStates>(*d).helis[0].length == 12.25f &&
                    std::get<HeliStates>(*d).helis[0].pos[1] == 31.5f &&
                    std::get<HeliStates>(*d).helis[0].flags == heli_flag::kVisible &&
                    std::get<HeliStates>(*d).mapSerial == 3 && std::get<HeliStates>(*d).tickMs == 98765,
                "heli states round trip");
        b.pop_back();
        t.Check(!Decode(b.data(), b.size()), "truncated heli states rejected");
        t.Check(!IsReliable(MsgId::HeliStates) && ChannelFor(MsgId::HeliStates) == kChannelState,
                "heli states unreliable on the state channel");
    }
    {
        AnimalStates s;
        s.mapSerial = 0xBEEF;
        auto b = Encode(s);
        auto d = Decode(b.data(), b.size());
        t.Check(d && std::holds_alternative<AnimalStates>(*d) && std::get<AnimalStates>(*d).mapSerial == 0xBEEF,
                "animal states map serial round trip");
    }

    PlayerJoined joined;
    joined.playerId = 3;
    SetName(joined.name, "P3");
    joined.model = 4;
    auto jb = Encode(joined);
    auto jd = Decode(jb.data(), jb.size());
    t.Check(jb.size() == kPlayerJoinedBytes && jd && std::holds_alternative<PlayerJoined>(*jd) &&
                std::get<PlayerJoined>(*jd).playerId == 3 && NameOf(std::get<PlayerJoined>(*jd).name) == "P3" &&
                std::get<PlayerJoined>(*jd).model == 4,
            "joined round trip");

    auto lb = Encode(PlayerLeft{2});
    auto ld = Decode(lb.data(), lb.size());
    t.Check(lb.size() == kPlayerLeftBytes && ld && std::holds_alternative<PlayerLeft>(*ld) &&
                std::get<PlayerLeft>(*ld).playerId == 2,
            "left round trip");

    PlayerState st;
    st.playerId = 1;
    st.seq = 65535;
    st.tickMs = 0xFFFFFFFFu;
    st.pos[0] = -123.456f;
    st.pos[1] = 0.001f;
    st.pos[2] = 50.f;
    st.yaw = 170.f;
    st.pitch = -12.5f;
    st.velocity[0] = 1.5f;
    st.velocity[1] = -2.25f;
    st.velocity[2] = 0.f;
    st.flags = static_cast<uint16_t>(player_flag::kCrouch | player_flag::kAiming | player_flag::kDead);
    st.weaponId = 255;
    st.health = 255;
    auto sb = Encode(st);
    auto sd = Decode(sb.data(), sb.size());
    t.Check(sb.size() == kPlayerStateBytes && sd && std::holds_alternative<PlayerState>(*sd), "state size");
    if (sd && std::holds_alternative<PlayerState>(*sd)) {
        const auto& m = std::get<PlayerState>(*sd);
        t.Check(m.playerId == 1 && m.seq == 65535 && m.tickMs == 0xFFFFFFFFu, "state ints");
        t.Check(std::memcmp(m.pos, st.pos, sizeof st.pos) == 0, "state pos bits");
        t.Check(std::memcmp(&m.yaw, &st.yaw, 4) == 0 && std::memcmp(&m.pitch, &st.pitch, 4) == 0, "state angles");
        t.Check(std::memcmp(m.velocity, st.velocity, sizeof st.velocity) == 0, "state vel bits");
        t.Check(m.flags == st.flags && m.weaponId == 255 && m.health == 255, "state tail");
        t.Check(SameBytes(Encode(m), sb), "state canonical");
    }

    auto pb = Encode(Ping{0xA1B2C3D4u});
    auto pd = Decode(pb.data(), pb.size());
    t.Check(pb.size() == kPingBytes && pd && std::holds_alternative<Ping>(*pd) && std::get<Ping>(*pd).token == 0xA1B2C3D4u,
            "ping round trip");
    auto qb = Encode(Pong{0xA1B2C3D4u});
    auto qd = Decode(qb.data(), qb.size());
    t.Check(qb.size() == kPongBytes && qd && std::holds_alternative<Pong>(*qd) && std::get<Pong>(*qd).token == 0xA1B2C3D4u,
            "pong round trip");

    Message mixed = st;
    auto mb = Encode(mixed);
    t.Check(SameBytes(mb, sb), "variant encode");

    AnimalStates emptyAnimals;
    emptyAnimals.seq = 1;
    emptyAnimals.tickMs = 50;
    auto emptyBytes = Encode(emptyAnimals);
    auto emptyDec = Decode(emptyBytes.data(), emptyBytes.size());
    t.Check(emptyBytes.size() == kAnimalStatesHeaderBytes && emptyBytes[0] == static_cast<uint8_t>(MsgId::AnimalStates) &&
                emptyBytes[kAnimalStatesHeaderBytes - 1] == 0,
            "empty animal list size");
    t.Check(emptyDec && std::holds_alternative<AnimalStates>(*emptyDec) && SameAnimals(std::get<AnimalStates>(*emptyDec), emptyAnimals),
            "empty animal list round trip");
    t.Check(emptyDec && std::holds_alternative<AnimalStates>(*emptyDec) &&
                SameBytes(Encode(std::get<AnimalStates>(*emptyDec)), emptyBytes),
            "empty animal list canonical");

    const AnimalStates herd = MakeHerd();
    t.Check(herd.animals.size() == 32 && herd.animals[0].state.size() == 40, "herd fixture is 32 with a 40-char name");
    bool herdAscii = true;
    for (const auto& animal : herd.animals) {
        if (!IsAscii(animal.state)) herdAscii = false;
    }
    t.Check(herdAscii, "herd state names are ASCII");
    auto herdBytes = Encode(herd);
    auto herdDec = Decode(herdBytes.data(), herdBytes.size());
    t.Check(herdBytes.size() > kAnimalStatesHeaderBytes && herdBytes[0] == static_cast<uint8_t>(MsgId::AnimalStates) &&
                herdBytes[kAnimalStatesHeaderBytes - 1] == 32,
            "32-entry count byte");
    t.Check(herdDec && std::holds_alternative<AnimalStates>(*herdDec), "32-entry type");
    if (herdDec && std::holds_alternative<AnimalStates>(*herdDec)) {
        const auto& m = std::get<AnimalStates>(*herdDec);
        const AnimalStates expect = WireForm(herd);
        t.Check(SameAnimals(m, expect), "32-entry fields");
        t.Check(m.animals[0].state.size() == 31 && m.animals[0].state == herd.animals[0].state.substr(0, 31),
                "40-char state truncated to 31");
        t.Check(m.animals[1].state.empty(), "empty state name round trip");
        t.Check(IsAscii(m.animals[0].state) && IsAscii(m.animals[3].state), "decoded state names stay ASCII");
        t.Check(SameBytes(Encode(m), herdBytes), "32-entry canonical");
        t.Check(SameBytes(Encode(Message{m}), herdBytes), "animal states variant encode");
    }
    AnimalStates over = herd;
    AnimalState extraAnimal;
    extraAnimal.id = 999;
    extraAnimal.state = "nope";
    over.animals.push_back(extraAnimal);
    t.Check(SameBytes(Encode(over), herdBytes), "encode drops the 33rd animal");

    AnimalState named;
    named.id = 120;
    named.state = std::string(31, 'a');
    AnimalStates oneNamed;
    oneNamed.animals.push_back(named);
    auto namedBytes = Encode(oneNamed);
    t.Check(namedBytes.size() == kAnimalStatesHeaderBytes + kAnimalStateFixedBytes + 31, "31-char entry size");

    AnimalHit hit;
    hit.shooterId = 0xFE;
    hit.animalId = 0xABCD;
    hit.damage = 12.5f;
    hit.hitPos[0] = 1.f;
    hit.hitPos[1] = 0.f;
    hit.hitPos[2] = -4.f;
    auto hitBytes = Encode(hit);
    auto hitDec = Decode(hitBytes.data(), hitBytes.size());
    t.Check(hitBytes.size() == kAnimalHitBytes && hitBytes[0] == static_cast<uint8_t>(MsgId::AnimalHit), "hit size");
    t.Check(hitBytes[1] == 0xFE && hitBytes[2] == 0xCD && hitBytes[3] == 0xAB, "hit little-endian");
    t.Check(hitDec && std::holds_alternative<AnimalHit>(*hitDec) && SameHit(std::get<AnimalHit>(*hitDec), hit), "hit round trip");
    t.Check(hitDec && std::holds_alternative<AnimalHit>(*hitDec) && SameBytes(Encode(std::get<AnimalHit>(*hitDec)), hitBytes),
            "hit canonical");
    AnimalHit zeros;
    auto zeroBytes = Encode(zeros);
    auto zeroDec = Decode(zeroBytes.data(), zeroBytes.size());
    t.Check(zeroDec && std::holds_alternative<AnimalHit>(*zeroDec) && SameHit(std::get<AnimalHit>(*zeroDec), zeros),
            "zero hit round trip");

    GameEvent gev;
    gev.flags = game_event_flag::kCommit;
    gev.actorId = 2;
    gev.seq = 0xBEEF;
    gev.kind = 99;
    gev.subjectId = 121;
    gev.i0 = 0x01020304u;
    gev.f0 = -2.5f;
    gev.key = std::string(40, 'k');
    auto gevBytes = Encode(gev);
    auto gevDec = Decode(gevBytes.data(), gevBytes.size());
    t.Check(gevBytes.size() == kGameEventHeaderBytes + kMaxGameEventKeyChars &&
                gevBytes[0] == static_cast<uint8_t>(MsgId::GameEvent),
            "game event size");
    t.Check(gevDec && std::holds_alternative<GameEvent>(*gevDec), "game event type");
    if (gevDec && std::holds_alternative<GameEvent>(*gevDec)) {
        const auto& m = std::get<GameEvent>(*gevDec);
        GameEvent expect = gev;
        expect.key.resize(kMaxGameEventKeyChars);
        t.Check(SameEvent(m, expect), "game event round trip");
        t.Check(m.key.size() == kMaxGameEventKeyChars, "game event key truncated to 31");
        t.Check(SameBytes(Encode(m), gevBytes), "game event canonical");
        t.Check(SameBytes(Encode(Message{m}), gevBytes), "game event variant encode");
    }
    GameEvent emptyKey;
    emptyKey.kind = 2;
    auto emptyEventBytes = Encode(emptyKey);
    auto emptyEventDec = Decode(emptyEventBytes.data(), emptyEventBytes.size());
    t.Check(emptyEventBytes.size() == kGameEventHeaderBytes, "empty game event key size");
    t.Check(emptyEventDec && std::holds_alternative<GameEvent>(*emptyEventDec) &&
                SameEvent(std::get<GameEvent>(*emptyEventDec), emptyKey),
            "empty game event key round trip");

    Session tick;
    t.Check(tick.ConsumeSendTick(0), "send tick first");
    t.Check(!tick.ConsumeSendTick(49), "send tick hold");
    t.Check(tick.ConsumeSendTick(50), "send tick 50ms");
    t.Check(!tick.ConsumeSendTick(99), "send tick 99");
    t.Check(tick.ConsumeSendTick(100), "send tick 100");
}

void TestTruncation(Tally& t) {
    using namespace net;
    std::vector<std::vector<uint8_t>> good;
    {
        Hello h;
        SetName(h.name, "A");
        good.push_back(Encode(h));
        good.push_back(Encode(Welcome{1, 5, 4}));
        good.push_back(Encode(Reject{RejectReason::LobbyFull}));
        PlayerJoined j;
        j.playerId = 1;
        SetName(j.name, "B");
        good.push_back(Encode(j));
        good.push_back(Encode(PlayerLeft{1}));
        PlayerState s;
        s.playerId = 1;
        s.pos[0] = 1.f;
        good.push_back(Encode(s));
        good.push_back(Encode(Ping{7}));
        good.push_back(Encode(Pong{7}));
        AnimalStates empty;
        empty.seq = 3;
        empty.tickMs = 9;
        good.push_back(Encode(empty));
        AnimalStates one;
        one.seq = 4;
        one.tickMs = 10;
        AnimalState deer;
        deer.id = 120;
        deer.pos[0] = 1.f;
        deer.pos[1] = 0.4f;
        deer.pos[2] = -2.f;
        deer.yaw = 90.f;
        deer.health = 45.f;
        deer.state = "Walk";
        one.animals.push_back(deer);
        good.push_back(Encode(one));
        AnimalHit shot;
        shot.shooterId = 1;
        shot.animalId = 122;
        shot.damage = 1.f;
        shot.hitPos[2] = 3.f;
        good.push_back(Encode(shot));
        GameEvent ev;
        ev.flags = game_event_flag::kCommit;
        ev.actorId = 1;
        ev.seq = 7;
        ev.kind = 1;
        ev.subjectId = 121;
        ev.f0 = 1.5f;
        ev.key = "map";
        good.push_back(Encode(ev));
    }
    for (const auto& buf : good) {
        t.Check(Decode(buf.data(), buf.size()).has_value(), "full buffer decodes");
        for (size_t n = 0; n < buf.size(); ++n) {
            // A v2 Hello / Reject cut to the v1 length is a valid v1 message.
            if (buf[0] == static_cast<uint8_t>(MsgId::Hello) && (n == kHelloV1Bytes || n == kHelloV2Bytes)) continue;
            if (buf[0] == static_cast<uint8_t>(MsgId::Reject) && n == kRejectV1Bytes) continue;
            t.Check(!Decode(buf.data(), n).has_value(), "truncated does not decode");
        }
        auto extra = buf;
        extra.push_back(0x5A);
        t.Check(!Decode(extra.data(), extra.size()).has_value(), "trailing byte rejected");
        for (size_t i = 0; i < buf.size(); ++i) {
            auto flipped = buf;
            flipped[i] = static_cast<uint8_t>(flipped[i] ^ 0xFFu);
            // Must not crash or read past the buffer. A flipped id or length fails; a flipped float still decodes.
            auto d = Decode(flipped.data(), flipped.size());
            if (d) {
                auto again = Encode(*d);
                t.Check(again.size() == flipped.size(), "flipped re-encode size");
            }
        }
    }
    t.Check(!Decode(nullptr, 0).has_value(), "null empty");
    t.Check(!Decode(nullptr, 8).has_value(), "null with length");

    std::mt19937 rng(0x27420u);
    for (int i = 0; i < 2000; ++i) {
        const size_t n = rng() % 96;
        std::vector<uint8_t> buf(n);
        for (auto& b : buf) b = static_cast<uint8_t>(rng() & 0xFF);
        auto d = Decode(buf.data(), buf.size());
        if (!d) continue;
        auto again = Encode(*d);
        auto back = Decode(again.data(), again.size());
        // Older Hello / Reject lengths decode on purpose (so the host can answer VersionMismatch) and re-encode at
        // the current size.
        const bool legacy = (buf[0] == static_cast<uint8_t>(MsgId::Hello) && (n == kHelloV1Bytes || n == kHelloV2Bytes)) ||
                            (buf[0] == static_cast<uint8_t>(MsgId::Reject) && n == kRejectV1Bytes);
        t.Check(back.has_value() && (legacy || SameBytes(again, buf)), "random exact packet is canonical");
    }

    const uint8_t reserved[] = {11, 15, 16, 32, 47, 49, 63, 64, 79, 80, 95, 96, 255};
    for (uint8_t id : reserved) {
        uint8_t buf[8] = {id, 1, 2, 3, 4, 5, 6, 7};
        t.Check(!Decode(buf, sizeof buf).has_value(), "reserved id rejected");
    }

    auto big = Encode(MakeHerd());
    t.Check(Decode(big.data(), big.size()).has_value(), "32-entry buffer decodes");
    for (size_t n = 0; n < big.size(); ++n) {
        t.Check(!Decode(big.data(), n).has_value(), "32-entry truncated");
    }
    auto bigExtra = big;
    bigExtra.push_back(0x5A);
    t.Check(!Decode(bigExtra.data(), bigExtra.size()).has_value(), "32-entry trailing byte rejected");

    std::vector<uint8_t> shortBody(kAnimalStatesHeaderBytes + 3, 0);
    shortBody[0] = static_cast<uint8_t>(MsgId::AnimalStates);
    shortBody[kAnimalStatesHeaderBytes - 1] = 8;
    t.Check(!Decode(shortBody.data(), shortBody.size()).has_value(), "count larger than remaining payload");

    std::vector<uint8_t> tooMany(kAnimalStatesHeaderBytes + 33, 0);
    tooMany[0] = static_cast<uint8_t>(MsgId::AnimalStates);
    tooMany[kAnimalStatesHeaderBytes - 1] = 33;
    t.Check(!Decode(tooMany.data(), tooMany.size()).has_value(), "count 33 rejected");

    AnimalState named;
    named.id = 120;
    named.state = std::string(31, 'a');
    AnimalStates oneNamed;
    oneNamed.animals.push_back(named);
    auto nameBytes = Encode(oneNamed);
    t.Check(nameBytes.size() == kAnimalStatesHeaderBytes + kAnimalStateFixedBytes + 31 && nameBytes[kAnimalStatesHeaderBytes + 23] == 31,
            "name length sits at the end of the fixed entry");
    nameBytes[kAnimalStatesHeaderBytes + 23] = 32;
    nameBytes.push_back('b');
    t.Check(!Decode(nameBytes.data(), nameBytes.size()).has_value(), "name length 32 rejected");
}

void TestInterp(Tally& t) {
    using namespace net;
    SnapshotBuffer empty;
    InterpSample sample;
    t.Check(!empty.Sample(0, sample), "empty sample");

    InterpConfig cfg;
    cfg.delayMs = 100;
    SnapshotBuffer move(cfg);
    for (uint32_t tm = 0; tm <= 2000; tm += 50) {
        PlayerState s;
        s.pos[0] = static_cast<float>(tm) / 1000.f * 10.f;
        s.velocity[0] = 10.f;
        s.yaw = 10.f;
        move.Push(tm, s);
    }
    t.Check(move.oldest_ms() == 1000 && move.newest_ms() == 2000, "history keeps about 1s");
    t.Check(move.size() == 21, "history count");

    float prev = -1.f;
    bool smooth = true;
    for (uint32_t render = 1100; render <= 2000; render += 10) {
        t.Check(move.Sample(render, sample), "sample in range");
        const float expect = static_cast<float>(render - 100) / 1000.f * 10.f;
        if (!Near(sample.pos[0], expect) || sample.extrapolated) smooth = false;
        if (prev >= 0.f && !Near(sample.pos[0] - prev, 0.1f)) smooth = false;
        prev = sample.pos[0];
    }
    t.Check(smooth, "moving target is linear at 10ms steps");

    InterpConfig noDelay;
    noDelay.delayMs = 0;
    SnapshotBuffer yawBuf(noDelay);
    PlayerState a;
    a.yaw = 170.f;
    a.pitch = 0.f;
    PlayerState b;
    b.yaw = -170.f;
    b.pitch = 10.f;
    b.flags = player_flag::kDead;
    a.flags = player_flag::kCrouch;
    yawBuf.Push(0, a);
    yawBuf.Push(100, b);
    struct YawCase {
        uint32_t at;
        float yawT;
    };
    const YawCase cases[] = {{0, 0.f}, {25, 0.25f}, {50, 0.5f}, {75, 0.75f}, {100, 1.f}};
    bool yawOk = true;
    for (const auto& c : cases) {
        t.Check(yawBuf.Sample(c.at, sample), "yaw sample");
        const float expect = ShortYaw(170.f, -170.f, c.yawT);
        if (AngDist(sample.yaw, expect) > 0.05f) yawOk = false;
        if (c.yawT < 1.f && sample.flags != player_flag::kCrouch) yawOk = false;
        if (c.yawT == 1.f && sample.flags != player_flag::kDead) yawOk = false;
    }
    t.Check(yawOk, "yaw takes the short arc across +/-180");
    t.Check(yawBuf.Sample(50, sample) && Near(sample.pitch, 5.f), "pitch lerp");
    // Must not pass through 0. The short arc is near +/-180.
    t.Check(yawBuf.Sample(50, sample) && std::fabs(sample.yaw) > 90.f, "mid yaw is not 0");

    PlayerState negA;
    negA.yaw = -170.f;
    PlayerState negB;
    negB.yaw = 170.f;
    SnapshotBuffer yawNeg(noDelay);
    yawNeg.Push(0, negA);
    yawNeg.Push(100, negB);
    t.Check(yawNeg.Sample(50, sample) && AngDist(sample.yaw, ShortYaw(-170.f, 170.f, 0.5f)) < 0.05f, "yaw short arc the other way");

    InterpConfig extCfg;
    extCfg.delayMs = 0;
    extCfg.extrapolateCapMs = 250;
    SnapshotBuffer ext(extCfg);
    PlayerState e0;
    e0.pos[0] = 0.f;
    e0.velocity[0] = 100.f;
    PlayerState e1 = e0;
    e1.pos[0] = 100.f;
    e1.yaw = 15.f;
    e1.pitch = -3.f;
    ext.Push(0, e0);
    ext.Push(1000, e1);
    t.Check(ext.Sample(1100, sample) && sample.extrapolated && Near(sample.pos[0], 110.f), "extrapolate 100ms");
    t.Check(ext.Sample(1500, sample) && sample.extrapolated && Near(sample.pos[0], 125.f) && Near(sample.yaw, 15.f) &&
                Near(sample.pitch, -3.f),
            "extrapolate capped at 250ms and angles hold");
    t.Check(ext.Sample(1000, sample) && !sample.extrapolated && Near(sample.pos[0], 100.f), "exact newest is not extrapolated");

    SnapshotBuffer stale;
    PlayerState s0;
    s0.pos[0] = 5.f;
    stale.Push(100, s0);
    PlayerState s1;
    s1.pos[0] = 1.f;
    stale.Push(50, s1);
    t.Check(stale.size() == 1 && stale.newest_ms() == 100, "older snapshot dropped");

    SnapshotBuffer one(extCfg);
    one.Push(0, e0);
    t.Check(one.Sample(0, sample) && !sample.extrapolated && Near(sample.pos[0], 0.f), "single snapshot holds");
    t.Check(one.Sample(400, sample) && sample.extrapolated && Near(sample.pos[0], 25.f), "single snapshot extrapolates to the cap");
}

struct Box {
    net::Session* session = nullptr;
    int index = 0;
    std::vector<net::Event> log;
    int stateCount[4] = {};
    bool gotSeq[4][60] = {};
    bool stateBad = false;
    bool spoofSeen = false;
    uint8_t spoofId = net::kInvalidPlayerId;
    float spoofPos = 0.f;
    int welcomes = 0;
    uint8_t welcomeId = net::kInvalidPlayerId;
    uint8_t welcomeMax = 0;
    std::vector<uint8_t> joins;
    std::string joinName[4];
    int rejectVersion = 0;
    int rejectFull = 0;
    bool left2 = false;
    int disconnected = 0;
    int timedOut = 0;
    bool pingSeen = false;
    bool pongSeen = false;
    uint32_t pongToken = 0;
    bool animalSeen = false;
    net::AnimalStates animalGot{};
    bool hitSeen = false;
    net::AnimalHit hitGot{};
    int gameEvents = 0;
    net::GameEvent gameGot{};
};

void OnEvent(Box& b, const net::Event& e) {
    b.log.push_back(e);
    using net::EventType;
    switch (e.type) {
    case EventType::Welcome:
        ++b.welcomes;
        b.welcomeId = e.playerId;
        b.welcomeMax = e.maxPlayers;
        break;
    case EventType::PlayerJoined:
        b.joins.push_back(e.playerId);
        if (e.playerId < 4) b.joinName[e.playerId] = e.name;
        break;
    case EventType::Reject:
        if (e.reason == net::RejectReason::VersionMismatch) ++b.rejectVersion;
        if (e.reason == net::RejectReason::LobbyFull) ++b.rejectFull;
        break;
    case EventType::PlayerLeft:
        if (e.playerId == 2) b.left2 = true;
        break;
    case EventType::Disconnected: ++b.disconnected; break;
    case EventType::TimedOut: ++b.timedOut; break;
    case EventType::PlayerState:
        if (e.state.seq == 65000) {
            b.spoofSeen = true;
            b.spoofId = e.state.playerId;
            b.spoofPos = e.state.pos[0];
        } else if (e.playerId < 4 && e.state.seq < 60) {
            if (e.state.pos[0] != static_cast<float>(e.state.seq) || e.state.pos[1] != static_cast<float>(e.playerId) ||
                e.state.tickMs != static_cast<uint32_t>(e.state.seq) * 50u) {
                b.stateBad = true;
            }
            if (!b.gotSeq[e.playerId][e.state.seq]) {
                b.gotSeq[e.playerId][e.state.seq] = true;
                ++b.stateCount[e.playerId];
            }
        }
        break;
    case EventType::Ping: b.pingSeen = true; break;
    case EventType::Pong:
        b.pongSeen = true;
        b.pongToken = e.token;
        break;
    case EventType::AnimalStates:
        b.animalSeen = true;
        b.animalGot = e.animalStates;
        break;
    case EventType::AnimalHit:
        b.hitSeen = true;
        b.hitGot = e.animalHit;
        break;
    case EventType::GameEvent:
        ++b.gameEvents;
        b.gameGot = e.gameEvent;
        break;
    default: break;
    }
}

void Pump(std::vector<Box>& boxes) {
    for (auto& b : boxes) {
        if (!b.session) continue;
        b.session->Service();
        for (const auto& e : b.session->DrainEvents()) OnEvent(b, e);
    }
}

template <class Pred>
bool PumpFor(int timeoutMs, std::vector<Box>& boxes, Pred pred) {
    const auto start = Clock::now();
    while (true) {
        Pump(boxes);
        if (pred()) return true;
        if (ElapsedMs(start) >= timeoutMs) return pred();
    }
}

void Dump(const Box& b) {
    std::fprintf(stderr, " box[%d] welcome=%d id=%u joins=", b.index, b.welcomes, b.welcomeId);
    for (uint8_t id : b.joins) std::fprintf(stderr, "%u ", id);
    std::fprintf(stderr, " rejV=%d rejF=%d left2=%d disc=%d timeout=%d states=%d,%d,%d,%d events=%zu\n", b.rejectVersion,
                 b.rejectFull, b.left2 ? 1 : 0, b.disconnected, b.timedOut, b.stateCount[0], b.stateCount[1], b.stateCount[2],
                 b.stateCount[3], b.log.size());
    const size_t n = b.log.size() < 12 ? b.log.size() : 12;
    for (size_t i = 0; i < n; ++i) {
        std::fprintf(stderr, "   %s id=%u\n", EventName(b.log[i].type), b.log[i].playerId);
    }
}

bool JoinVec(const std::vector<uint8_t>& got, std::initializer_list<uint8_t> expect) {
    return got.size() == expect.size() && std::equal(got.begin(), got.end(), expect.begin());
}

Digest TestLoopback(Tally& t, bool* sectionOk) {
    using namespace net;
    Digest digest;
    const int fails0 = t.fail;
    const int inits0 = Session::InitializeCount();

    std::optional<Session> host;
    host.emplace();
    Session::Identity hostId;
    hostId.name = "Host";
    uint16_t port = kDefaultPort;
    if (!host->StartHost(port, kDefaultMaxClients, hostId)) {
        t.Check(false, "bind 27420");
        host.reset();
        host.emplace();
        t.Check(host->StartHost(0, kDefaultMaxClients, hostId), "bind ephemeral");
        port = host->BoundPort();
    } else {
        t.Check(host->BoundPort() == kDefaultPort, "bound 27420");
    }
    t.Check(host->IsHost() && host->IsConnected() && host->LocalPlayerId() == 0, "host id 0");
    t.Check(host->MaxPlayers() == 4, "max players 4");
    t.Check(Session::InitializeCount() == inits0 + 1, "enet_initialize once for the host");
    digest.initDelta = Session::InitializeCount() - inits0;
    digest.ids[0] = host->LocalPlayerId();

    std::vector<Box> boxes(1);
    boxes[0].session = &(*host);
    boxes[0].index = 0;
    Pump(boxes);

    std::optional<Session> bad;
    bad.emplace();
    Session::Identity badId;
    badId.name = "Bad";
    badId.protoVersion = 99;
    t.Check(bad->Join("127.0.0.1", port, badId), "bad join starts");
    boxes.push_back({});
    boxes.back().session = &(*bad);
    boxes.back().index = 9;
    const bool versionDone = PumpFor(2000, boxes, [&] {
        return boxes.back().rejectVersion >= 1 && boxes.back().disconnected >= 1 && boxes[0].rejectVersion >= 1;
    });
    t.Check(versionDone, "version mismatch rejected");
    t.Check(bad->LocalPlayerId() == kInvalidPlayerId && !bad->IsConnected(), "bad client not in lobby");
    t.Check(Session::InitializeCount() == inits0 + 1, "second session did not re-init enet");
    bad.reset();
    boxes.pop_back();
    PumpFor(200, boxes, [] { return false; });

    std::optional<Session> clients[3];
    const char* names[3] = {"P1", "P2", "P3"};
    bool handshake = host->IsHost();
    for (int i = 0; i < 3; ++i) {
        clients[i].emplace();
        Session::Identity id;
        id.name = names[i];
        if (!clients[i]->Join("127.0.0.1", port, id)) handshake = false;
        boxes.push_back({});
        boxes.back().session = &(*clients[i]);
        boxes.back().index = i + 1;
        const int idx = static_cast<int>(boxes.size()) - 1;
        const bool up = PumpFor(2000, boxes, [&] {
            return clients[i]->LocalPlayerId() == static_cast<uint8_t>(i + 1) &&
                   static_cast<int>(host->Peers().size()) == i + 1;
        });
        if (!up) handshake = false;
        t.Check(up, std::string("client ") + names[i] + " welcomed");
        digest.ids[i + 1] = clients[i]->LocalPlayerId();
        t.Check(clients[i]->MaxPlayers() == 4 && boxes[idx].welcomes == 1 && boxes[idx].welcomeMax == 4, "welcome fields");
    }
    t.Check(handshake && digest.ids[1] == 1 && digest.ids[2] == 2 && digest.ids[3] == 3, "ids 0..3");
    sectionOk[3] = handshake && digest.ids[1] == 1 && digest.ids[2] == 2 && digest.ids[3] == 3 && t.fail == fails0;

    const bool fan = JoinVec(boxes[1].joins, {0, 2, 3}) && JoinVec(boxes[2].joins, {0, 1, 3}) &&
                     JoinVec(boxes[3].joins, {0, 1, 2}) && JoinVec(boxes[0].joins, {1, 2, 3}) &&
                     boxes[1].joinName[0] == "Host" && boxes[1].joinName[2] == "P2" && boxes[3].joinName[1] == "P1";
    t.Check(fan, "PlayerJoined fan-out");
    sectionOk[4] = fan;

    t.Check(clients[0]->SendPing(0xA1B2C3D4u), "send ping");
    const bool pingOk = PumpFor(1000, boxes, [&] { return boxes[0].pingSeen && boxes[1].pongSeen && boxes[1].pongToken == 0xA1B2C3D4u; });
    t.Check(pingOk, "ping/pong token");

    bool animalOk = false;
    if (!handshake) {
        t.Check(false, "animal loopback needs a lobby");
    } else {
        const bool roles = !clients[0]->SendAnimalStates(AnimalStates{}) && !host->SendAnimalHit(AnimalHit{});
        t.Check(roles, "animal send roles");

        AnimalStates snap;
        snap.seq = 42;
        snap.tickMs = 1000;
        AnimalState deer;
        deer.id = 120;
        deer.pos[0] = 10.5f;
        deer.pos[1] = 0.4f;
        deer.pos[2] = -3.25f;
        deer.yaw = 90.f;
        deer.health = 45.f;
        deer.state = "Walk";
        AnimalState puma;
        puma.id = 124;
        puma.pos[0] = -10000.f;
        puma.pos[1] = -10000.f;
        puma.pos[2] = -10000.f;
        puma.health = 100.f;
        puma.flags = animal_flag::kParked;
        puma.state = "Idle";
        AnimalState dead;
        dead.id = 121;
        dead.pos[0] = 3.f;
        dead.pos[1] = 1.f;
        dead.pos[2] = 4.f;
        dead.yaw = 180.f;
        dead.flags = static_cast<uint8_t>(animal_flag::kDead | animal_flag::kFrozenOnHost);
        dead.state = "fall";
        snap.animals = {deer, puma, dead};

        const bool sent = host->SendAnimalStates(snap);
        t.Check(sent, "host sends AnimalStates");
        const bool got = sent && PumpFor(1000, boxes, [&] {
            return boxes[1].animalSeen && boxes[2].animalSeen && boxes[3].animalSeen;
        });
        const bool same = got && !boxes[0].animalSeen && SameAnimals(boxes[1].animalGot, snap) &&
                          SameAnimals(boxes[2].animalGot, snap) && SameAnimals(boxes[3].animalGot, snap);
        t.Check(same, "clients received AnimalStates");

        AnimalHit hit;
        hit.shooterId = 9;
        hit.animalId = 122;
        hit.damage = 12.5f;
        hit.hitPos[0] = 1.25f;
        hit.hitPos[2] = -4.f;
        const bool hitSent = clients[0]->SendAnimalHit(hit);
        t.Check(hitSent, "client sends AnimalHit");
        const bool hitArrived = hitSent && PumpFor(1000, boxes, [&] { return boxes[0].hitSeen; });
        AnimalHit expect = hit;
        expect.shooterId = 1;
        const bool hitSame = hitArrived && clients[0]->LocalPlayerId() == 1 && SameHit(boxes[0].hitGot, expect) &&
                             !boxes[1].hitSeen && !boxes[2].hitSeen && !boxes[3].hitSeen;
        t.Check(hitSame, "host received AnimalHit with shooterId");
        animalOk = roles && same && hitSame;

        GameEvent forged;
        forged.flags = game_event_flag::kCommit;
        forged.kind = 1;
        forged.subjectId = 121;
        const bool commitRole = !clients[0]->SendGameEvent(forged);
        GameEvent hostReq;
        hostReq.kind = 2;
        const bool requestRole = !host->SendGameEvent(hostReq);
        t.Check(commitRole && requestRole, "game event send roles");

        for (auto& b : boxes) b.gameEvents = 0;
        GameEvent req;
        req.actorId = 9;
        req.kind = 2;
        req.subjectId = 0;
        req.i0 = 4;
        req.key = "HUNTMAP4";
        const bool reqSent = clients[0]->SendGameEvent(req);
        t.Check(reqSent, "client sends GameEvent request");
        const bool reqArrived = reqSent && PumpFor(1000, boxes, [&] { return boxes[0].gameEvents > 0; });
        GameEvent reqExpect = req;
        reqExpect.actorId = 1;
        const bool reqSame = reqArrived && SameEvent(boxes[0].gameGot, reqExpect) && boxes[1].gameEvents == 0 &&
                             boxes[2].gameEvents == 0 && boxes[3].gameEvents == 0;
        t.Check(reqSame, "host received GameEvent request with actorId");

        for (auto& b : boxes) b.gameEvents = 0;
        GameEvent commit;
        commit.flags = game_event_flag::kCommit;
        commit.actorId = 1;
        commit.seq = 3;
        commit.kind = 99;
        commit.subjectId = 121;
        commit.f0 = 0.f;
        commit.key = "objective.escort";
        const bool commitSent = host->SendGameEvent(commit);
        t.Check(commitSent, "host sends GameEvent commit");
        const bool commitArrived = commitSent && PumpFor(1000, boxes, [&] {
            return boxes[1].gameEvents > 0 && boxes[2].gameEvents > 0 && boxes[3].gameEvents > 0;
        });
        const bool commitSame = commitArrived && boxes[0].gameEvents == 0 && SameEvent(boxes[1].gameGot, commit) &&
                                 SameEvent(boxes[2].gameGot, commit) && SameEvent(boxes[3].gameGot, commit);
        t.Check(commitSame, "clients received unknown-kind GameEvent commit");
        sectionOk[12] = commitRole && requestRole && reqSame && commitSame;
    }
    sectionOk[11] = animalOk;

    const auto peers = host->Peers();
    t.Check(peers.size() == 3, "three peers");
    bool bytesOk = peers.size() == 3;
    for (const auto& row : peers) {
        std::printf("stats player %u rtt=%u ms loss=%.4f in=%llu out=%llu\n", row.playerId, row.rttMs,
                    static_cast<double>(row.packetLoss), static_cast<unsigned long long>(row.bytesIn),
                    static_cast<unsigned long long>(row.bytesOut));
        if (row.bytesIn == 0 || row.bytesOut == 0) bytesOk = false;
    }
    t.Check(bytesOk, "payload byte counters");

    PlayerState spoof;
    spoof.playerId = 9;
    spoof.seq = 65000;
    spoof.pos[0] = 42.f;
    t.Check(clients[0]->SendPlayerState(spoof), "spoof send");
    const bool spoofOk = PumpFor(1000, boxes, [&] {
        return boxes[0].spoofSeen && boxes[2].spoofSeen && boxes[3].spoofSeen && !boxes[1].spoofSeen;
    });
    t.Check(spoofOk && boxes[0].spoofId == 1 && boxes[2].spoofId == 1 && boxes[0].spoofPos == 42.f,
            "host rewrote spoofed player id");

    const auto waveStart = Clock::now();
    bool relay = true;
    for (int tick = 0; tick < 60 && relay; ++tick) {
        const auto due = waveStart + std::chrono::milliseconds(tick * 50);
        while (Clock::now() < due) Pump(boxes);
        for (int i = 0; i < 4; ++i) {
            PlayerState st;
            st.seq = static_cast<uint16_t>(tick);
            st.tickMs = static_cast<uint32_t>(tick * 50);
            st.pos[0] = static_cast<float>(tick);
            st.pos[1] = static_cast<float>(i);
            st.yaw = static_cast<float>(i);
            st.health = 100;
            Session* who = i == 0 ? &(*host) : &(*clients[i - 1]);
            if (!who->SendPlayerState(st)) relay = false;
        }
        const bool got = PumpFor(500, boxes, [&] {
            const int need = tick + 1;
            if (boxes[0].stateCount[1] < need || boxes[0].stateCount[2] < need || boxes[0].stateCount[3] < need) return false;
            for (int c = 1; c <= 3; ++c) {
                for (int s = 0; s < 4; ++s) {
                    if (s == c) continue;
                    if (boxes[c].stateCount[s] < need) return false;
                }
            }
            return true;
        });
        if (!got) relay = false;
    }
    for (int r = 0; r < 4; ++r) {
        for (int s = 0; s < 4; ++s) digest.relay[r][s] = boxes[r].stateCount[s];
    }
    bool matrix = !boxes[0].stateBad && !boxes[1].stateBad && !boxes[2].stateBad && !boxes[3].stateBad;
    for (int r = 0; r < 4; ++r) {
        for (int s = 0; s < 4; ++s) {
            const int expect = r == s ? 0 : 60;
            if (digest.relay[r][s] != expect) matrix = false;
        }
    }
    t.Check(relay && matrix, "every client got every other PlayerState");
    sectionOk[5] = relay && matrix;

    std::optional<Session> extra;
    extra.emplace();
    Session::Identity extraId;
    extraId.name = "Extra";
    t.Check(extra->Join("127.0.0.1", port, extraId), "fifth join starts");
    boxes.push_back({});
    boxes.back().session = &(*extra);
    boxes.back().index = 8;
    const bool full = PumpFor(2000, boxes, [&] {
        return boxes.back().rejectFull >= 1 && boxes.back().disconnected >= 1 && boxes[0].rejectFull >= 1 &&
               extra->LocalPlayerId() == kInvalidPlayerId;
    });
    t.Check(full, "fifth connection rejected full");
    t.Check(host->Peers().size() == 3, "lobby still 3 clients");
    digest.rejectF = boxes[0].rejectFull;
    digest.rejectV = boxes[0].rejectVersion;
    sectionOk[6] = versionDone;
    sectionOk[7] = full;
    extra.reset();
    boxes.pop_back();

    const int discBefore1 = boxes[1].disconnected;
    const int discBefore3 = boxes[3].disconnected;
    clients[1]->Disconnect();
    const bool left = PumpFor(2000, boxes, [&] {
        return boxes[0].left2 && boxes[1].left2 && boxes[3].left2 && boxes[2].disconnected >= 1 &&
               host->Peers().size() == 2;
    });
    t.Check(left, "PlayerLeft after client disconnect");
    t.Check(boxes[1].disconnected == discBefore1 && boxes[3].disconnected == discBefore3, "other clients stayed connected");
    t.Check(boxes[1].timedOut == 0 && boxes[3].timedOut == 0 && boxes[0].timedOut == 0, "leave was not a timeout");
    sectionOk[8] = left;
    digest.left = left ? 1 : 0;
    clients[1].reset();
    boxes[2].session = nullptr;

    host->Disconnect();
    boxes[0].session = nullptr;
    const bool down = PumpFor(2000, boxes, [&] {
        return !clients[0]->IsConnected() && !clients[2]->IsConnected() &&
               boxes[1].disconnected > discBefore1 && boxes[3].disconnected > discBefore3;
    });
    t.Check(down, "host shutdown disconnects clients");
    t.Check(boxes[1].timedOut == 0 && boxes[3].timedOut == 0, "shutdown was not a timeout");
    sectionOk[9] = down;
    digest.down = down ? 1 : 0;

    if (!handshake || !fan || !relay || !versionDone || !full || !left || !down) {
        for (const auto& b : boxes) Dump(b);
    }

    clients[0].reset();
    clients[2].reset();
    host.reset();
    t.Check(Session::InitializeCount() == inits0 + 1, "init count stable until the last session");
    sectionOk[10] = digest.initDelta == 1 && Session::InitializeCount() == inits0 + 1;
    digest.fails = t.fail - fails0;
    return digest;
}

}  // namespace

int main() {
    spdlog::set_level(spdlog::level::warn);
    spdlog::set_pattern("[%l] %v");
    // Every test socket stays on loopback, so a new build path never raises the Windows Firewall prompt.
    net::Session::SetBindAddress("127.0.0.1");

    const char* names[] = {
        "codec round-trip",     "truncated buffers", "interp + yaw wrap", "handshake ids 0..3",
        "PlayerJoined fan-out", "PlayerState relay", "reject version",    "reject full",
        "PlayerLeft",           "host shutdown",     "enet_initialize once", "animal loopback",
        "game event loopback"};
    constexpr int kSections = 13;
    Section sections[kSections];
    for (int i = 0; i < kSections; ++i) sections[i].name = names[i];

    Tally tally;
    Digest digests[2];
    for (int run = 0; run < 2; ++run) {
        const int before = tally.fail;
        TestCodec(tally);
        sections[0].ok[run] = tally.fail == before;
        const int afterCodec = tally.fail;
        TestTruncation(tally);
        sections[1].ok[run] = tally.fail == afterCodec;
        const int afterTrunc = tally.fail;
        TestInterp(tally);
        sections[2].ok[run] = tally.fail == afterTrunc;

        bool loopOk[kSections] = {};
        digests[run] = TestLoopback(tally, loopOk);
        for (int s = 3; s < kSections; ++s) sections[s].ok[run] = loopOk[s];
        std::printf("run %d failures-in-run=%d initDelta=%d\n", run + 1, digests[run].fails, digests[run].initDelta);
        std::printf("relay rows (host p1 p2 p3):\n");
        const char* rowName[] = {"host", "p1", "p2", "p3"};
        for (int r = 0; r < 4; ++r) {
            std::printf("  %-4s                 %4d %4d %4d %4d\n", rowName[r], digests[run].relay[r][0], digests[run].relay[r][1],
                        digests[run].relay[r][2], digests[run].relay[r][3]);
        }
    }

    const bool digestOk = Same(digests[0], digests[1]);
    std::printf("\ndh2net_test protocol v1\n");
    std::printf("%-24s %6s %6s\n", "check", "run1", "run2");
    for (const auto& s : sections) {
        std::printf("%-24s %6s %6s\n", s.name, s.ok[0] ? "pass" : "FAIL", s.ok[1] ? "pass" : "FAIL");
    }
    std::printf("%-24s %6s\n", "digest match", digestOk ? "pass" : "FAIL");
    std::printf("checks passed %d failed %d\n", tally.pass, tally.fail);
    if (!digestOk) {
        std::fprintf(stderr, "FAIL runs disagreed\n");
        ++tally.fail;
    }
    return tally.fail == 0 ? 0 : 1;
}
