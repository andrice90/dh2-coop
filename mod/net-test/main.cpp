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
    }
    return "?";
}

void TestCodec(Tally& t) {
    using namespace net;
    t.Check(ChannelFor(MsgId::PlayerState) == kChannelState, "state channel");
    t.Check(ChannelFor(MsgId::Hello) == kChannelControl, "hello channel");
    t.Check(IsReliable(MsgId::Hello) && !IsReliable(MsgId::PlayerState), "reliability");

    Hello hello;
    hello.protoVersion = 1;
    hello.gameBuild = 434;
    SetName(hello.name, "Hunter-123456789012345678901234567890EXTRA");
    auto hb = Encode(hello);
    t.Check(hb.size() == kHelloBytes, "hello size");
    auto hd = Decode(hb.data(), hb.size());
    t.Check(hd.has_value() && std::holds_alternative<Hello>(*hd), "hello type");
    if (hd && std::holds_alternative<Hello>(*hd)) {
        const auto& m = std::get<Hello>(*hd);
        const std::string name = NameOf(m.name);
        t.Check(m.protoVersion == 1 && m.gameBuild == 434, "hello fields");
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
                                 RejectReason::BadHello}) {
        auto b = Encode(Reject{reason});
        auto d = Decode(b.data(), b.size());
        t.Check(b.size() == kRejectBytes && d && std::holds_alternative<Reject>(*d) && std::get<Reject>(*d).reason == reason,
                std::string("reject ") + ToString(reason));
    }

    PlayerJoined joined;
    joined.playerId = 3;
    SetName(joined.name, "P3");
    auto jb = Encode(joined);
    auto jd = Decode(jb.data(), jb.size());
    t.Check(jb.size() == kPlayerJoinedBytes && jd && std::holds_alternative<PlayerJoined>(*jd) &&
                std::get<PlayerJoined>(*jd).playerId == 3 && NameOf(std::get<PlayerJoined>(*jd).name) == "P3",
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
    }
    for (const auto& buf : good) {
        t.Check(Decode(buf.data(), buf.size()).has_value(), "full buffer decodes");
        for (size_t n = 0; n < buf.size(); ++n) {
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
        t.Check(back.has_value() && SameBytes(again, buf), "random exact packet is canonical");
    }

    const uint8_t reserved[] = {16, 32, 47, 48, 63, 64, 79, 80, 95, 96, 255};
    for (uint8_t id : reserved) {
        uint8_t buf[8] = {id, 1, 2, 3, 4, 5, 6, 7};
        t.Check(!Decode(buf, sizeof buf).has_value(), "reserved id rejected");
    }
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

    const char* names[] = {
        "codec round-trip",     "truncated buffers", "interp + yaw wrap", "handshake ids 0..3",
        "PlayerJoined fan-out", "PlayerState relay", "reject version",    "reject full",
        "PlayerLeft",           "host shutdown",     "enet_initialize once"};
    constexpr int kSections = 11;
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
