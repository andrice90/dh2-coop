#include "net/session.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <enet/enet.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

namespace net {
namespace {

// Process-wide. Session has no threads; the game thread and the test are the only callers.
int g_refs = 0;
int g_inits = 0;

bool AcquireLib() {
    if (g_refs == 0) {
        if (enet_initialize() != 0) {
            spdlog::error("net: enet_initialize failed");
            return false;
        }
        ++g_inits;
        spdlog::info("net: enet {}.{}.{} initialized", ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH);
    }
    ++g_refs;
    return true;
}

void ReleaseLib() {
    if (g_refs == 0) return;
    --g_refs;
    if (g_refs == 0) {
        enet_deinitialize();
        spdlog::info("net: enet shut down");
    }
}

struct PacketGuard {
    ENetPacket* pkt = nullptr;
    ~PacketGuard() {
        if (pkt) enet_packet_destroy(pkt);
    }
};

struct Remote {
    ENetPeer* peer = nullptr;
    uint8_t playerId = kInvalidPlayerId;
    std::string name;
    bool active = false;
    bool localClose = false;
    bool waitHello = false;
    uint64_t bytesIn = 0;
    uint64_t bytesOut = 0;
    std::chrono::steady_clock::time_point helloDeadline{};
};

}  // namespace

struct Session::Impl {
    using Clock = std::chrono::steady_clock;

    ENetHost* host = nullptr;
    ENetPeer* server = nullptr;
    bool acquired = false;
    bool hosting = false;
    bool connected = false;
    bool inService = false;
    bool deferClose = false;
    bool closing = false;
    bool queueWarned = false;
    uint8_t localId = kInvalidPlayerId;
    int maxClients = kDefaultMaxClients;
    int maxPlayers = 0;
    uint16_t boundPort = 0;
    Identity identity{};
    Clock::time_point started{};
    std::vector<std::unique_ptr<Remote>> remotes;
    std::vector<Event> events;
    Handler handler;
    bool sendArmed = false;
    uint32_t lastSendMs = 0;

    ~Impl() { Close(); }

    void RequestClose() {
        if (inService) {
            deferClose = true;
            return;
        }
        Close();
    }

    void Close() {
        if (closing) return;
        closing = true;

        const bool wasLink = connected || server != nullptr || hosting;
        const uint8_t id = localId;

        if (host) {
            if (hosting) {
                for (auto& rp : remotes) {
                    if (!rp->peer) continue;
                    rp->localClose = true;
                    enet_peer_disconnect(rp->peer, kDisconnectShutdown);
                }
            } else if (server) {
                if (Remote* r = From(server)) r->localClose = true;
                enet_peer_disconnect(server, kDisconnectGraceful);
            }
            enet_host_flush(host);
            for (auto& rp : remotes) {
                if (rp->peer) rp->peer->data = nullptr;
            }
            enet_host_destroy(host);
            host = nullptr;
        }

        server = nullptr;
        remotes.clear();
        hosting = false;
        connected = false;
        localId = kInvalidPlayerId;
        maxPlayers = 0;
        boundPort = 0;

        if (wasLink) {
            Event ev;
            ev.type = EventType::Disconnected;
            ev.playerId = id;
            Push(ev);
        }

        if (acquired) {
            ReleaseLib();
            acquired = false;
        }
        closing = false;
    }

    Remote* From(ENetPeer* peer) const {
        if (!peer || !peer->data) return nullptr;
        return static_cast<Remote*>(peer->data);
    }

    int ActiveCount() const {
        int n = 0;
        for (const auto& rp : remotes)
            if (rp->active) ++n;
        return n;
    }

    uint8_t AllocId() const {
        bool used[kMaxPlayers] = {};
        used[0] = true;
        for (const auto& rp : remotes) {
            if (rp->active && rp->playerId < kMaxPlayers) used[rp->playerId] = true;
        }
        for (int id = 1; id <= maxClients; ++id) {
            if (!used[id]) return static_cast<uint8_t>(id);
        }
        return kInvalidPlayerId;
    }

    uint32_t TickMs() const {
        if (!hosting) return 0;
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started).count();
        if (ms <= 0) return 0;
        return static_cast<uint32_t>(ms);
    }

    void Push(Event ev) {
        if (handler) {
            handler(ev);
            return;
        }
        if (events.size() >= 2048) {
            events.erase(events.begin());
            if (!queueWarned) {
                spdlog::warn("net: event queue full, dropping oldest");
                queueWarned = true;
            }
        }
        events.push_back(std::move(ev));
    }

    bool Send(Remote* remote, const std::vector<uint8_t>& bytes, uint8_t channel, bool reliable) {
        if (!remote || !remote->peer || !host) return false;
        const enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
        ENetPacket* pkt = enet_packet_create(bytes.data(), bytes.size(), flags);
        if (!pkt) {
            spdlog::error("net: packet alloc failed");
            return false;
        }
        if (enet_peer_send(remote->peer, channel, pkt) < 0) {
            enet_packet_destroy(pkt);
            return false;
        }
        remote->bytesOut += bytes.size();
        return true;
    }

    void SendHello(Remote* remote) {
        Hello hello;
        hello.protoVersion = identity.protoVersion;
        hello.gameBuild = identity.gameBuild;
        SetName(hello.name, identity.name);
        Send(remote, Encode(hello), kChannelControl, true);
    }

    void RejectPeer(Remote& remote, RejectReason reason) {
        Reject msg;
        msg.reason = reason;
        Send(&remote, Encode(msg), kChannelControl, true);
        remote.localClose = true;
        remote.waitHello = false;
        if (remote.peer) enet_peer_disconnect_later(remote.peer, kDisconnectReject);
        Event ev;
        ev.type = EventType::Reject;
        ev.reason = reason;
        Push(ev);
        spdlog::info("net: reject ({})", ToString(reason));
    }

    void Accept(Remote& remote, const Hello& hello, uint8_t id) {
        remote.playerId = id;
        remote.name = NameOf(hello.name);
        remote.active = true;
        remote.waitHello = false;

        Welcome welcome;
        welcome.playerId = id;
        welcome.hostTick = TickMs();
        welcome.maxPlayers = static_cast<uint8_t>(maxClients + 1);
        Send(&remote, Encode(welcome), kChannelControl, true);

        PlayerJoined hostJoin;
        hostJoin.playerId = 0;
        SetName(hostJoin.name, identity.name);
        Send(&remote, Encode(hostJoin), kChannelControl, true);

        for (auto& other : remotes) {
            if (other.get() == &remote || !other->active) continue;
            PlayerJoined existing;
            existing.playerId = other->playerId;
            SetName(existing.name, other->name);
            Send(&remote, Encode(existing), kChannelControl, true);
        }

        PlayerJoined neu;
        neu.playerId = id;
        SetName(neu.name, remote.name);
        const auto bytes = Encode(neu);
        for (auto& other : remotes) {
            if (other.get() == &remote || !other->active) continue;
            Send(other.get(), bytes, kChannelControl, true);
        }

        Event joined;
        joined.type = EventType::PlayerJoined;
        joined.playerId = id;
        joined.name = remote.name;
        Push(joined);

        Event link;
        link.type = EventType::Connected;
        link.playerId = id;
        link.name = remote.name;
        Push(link);

        spdlog::info("net: player {} '{}' joined", static_cast<int>(id), remote.name);
    }

    void On(Remote& remote, const Hello& hello) {
        Event ev;
        ev.type = EventType::Hello;
        ev.protoVersion = hello.protoVersion;
        ev.gameBuild = hello.gameBuild;
        ev.name = NameOf(hello.name);
        Push(ev);

        if (!hosting || remote.localClose || remote.active) return;
        if (hello.protoVersion != kProtocolVersion || hello.gameBuild != kGameBuild) {
            RejectPeer(remote, RejectReason::VersionMismatch);
            return;
        }
        if (ActiveCount() >= maxClients) {
            RejectPeer(remote, RejectReason::LobbyFull);
            return;
        }
        const uint8_t id = AllocId();
        if (id == kInvalidPlayerId) {
            RejectPeer(remote, RejectReason::LobbyFull);
            return;
        }
        Accept(remote, hello, id);
    }

    void On(Remote& remote, const Welcome& welcome) {
        if (hosting) return;
        localId = welcome.playerId;
        maxPlayers = welcome.maxPlayers;
        connected = true;
        remote.active = true;
        remote.playerId = 0;

        Event link;
        link.type = EventType::Connected;
        link.playerId = localId;
        link.name = identity.name;
        Push(link);

        Event ev;
        ev.type = EventType::Welcome;
        ev.playerId = welcome.playerId;
        ev.hostTick = welcome.hostTick;
        ev.maxPlayers = welcome.maxPlayers;
        Push(ev);
        spdlog::info("net: welcomed as player {} (tick {})", static_cast<int>(localId), welcome.hostTick);
    }

    void On(Remote&, const Reject& reject) {
        Event ev;
        ev.type = EventType::Reject;
        ev.reason = reject.reason;
        ev.playerId = localId;
        Push(ev);
        spdlog::info("net: server rejected ({})", ToString(reject.reason));
    }

    void On(Remote& remote, const PlayerJoined& joined) {
        if (hosting) return;
        if (joined.playerId == 0) remote.name = NameOf(joined.name);
        Event ev;
        ev.type = EventType::PlayerJoined;
        ev.playerId = joined.playerId;
        ev.name = NameOf(joined.name);
        Push(ev);
    }

    void On(Remote&, const PlayerLeft& left) {
        if (hosting) return;
        Event ev;
        ev.type = EventType::PlayerLeft;
        ev.playerId = left.playerId;
        Push(ev);
    }

    void On(Remote& remote, const PlayerState& state) {
        PlayerState fixed = state;
        if (hosting) {
            if (!remote.active) return;
            fixed.playerId = remote.playerId;
        }
        Event ev;
        ev.type = EventType::PlayerState;
        ev.playerId = fixed.playerId;
        ev.state = fixed;
        Push(ev);
        if (!hosting) return;

        const auto bytes = Encode(fixed);
        for (auto& other : remotes) {
            if (other.get() == &remote || !other->active) continue;
            Send(other.get(), bytes, kChannelState, false);
        }
    }

    void On(Remote& remote, const Ping& ping) {
        Event ev;
        ev.type = EventType::Ping;
        ev.playerId = remote.playerId;
        ev.token = ping.token;
        Push(ev);
        Pong pong;
        pong.token = ping.token;
        Send(&remote, Encode(pong), kChannelControl, true);
    }

    void On(Remote& remote, const Pong& pong) {
        Event ev;
        ev.type = EventType::Pong;
        ev.playerId = remote.playerId;
        ev.token = pong.token;
        Push(ev);
    }

    void On(Remote& remote, const AnimalStates& states) {
        if (hosting) return;
        Event ev;
        ev.type = EventType::AnimalStates;
        ev.playerId = remote.playerId;
        ev.animalStates = states;
        Push(ev);
    }

    void On(Remote& remote, const AnimalHit& hit) {
        if (!hosting || !remote.active) return;
        AnimalHit fixed = hit;
        fixed.shooterId = remote.playerId;
        Event ev;
        ev.type = EventType::AnimalHit;
        ev.playerId = fixed.shooterId;
        ev.animalHit = fixed;
        Push(ev);
    }

    void On(Remote& remote, const GameEvent& evIn) {
        if (hosting) {
            if (!remote.active || evIn.commit()) return;
            GameEvent fixed = evIn;
            fixed.actorId = remote.playerId;
            fixed.flags = static_cast<uint8_t>(fixed.flags & ~game_event_flag::kCommit);
            Event ev;
            ev.type = EventType::GameEvent;
            ev.playerId = fixed.actorId;
            ev.gameEvent = std::move(fixed);
            Push(ev);
            return;
        }
        if (!evIn.commit()) return;
        Event ev;
        ev.type = EventType::GameEvent;
        ev.playerId = evIn.actorId;
        ev.gameEvent = evIn;
        Push(ev);
    }

    void Dispatch(Remote& remote, const Message& msg) {
        std::visit([&](const auto& m) { On(remote, m); }, msg);
    }

    void OnConnect(ENetPeer* peer) {
        Remote* existing = From(peer);
        if (!existing) {
            auto remote = std::make_unique<Remote>();
            remote->peer = peer;
            remote->waitHello = hosting;
            if (hosting) remote->helloDeadline = Clock::now() + std::chrono::milliseconds(kHelloTimeoutMs);
            peer->data = remote.get();
            existing = remote.get();
            remotes.push_back(std::move(remote));
        }
        spdlog::info("net: link up");
        if (!hosting) SendHello(existing);
    }

    void Erase(Remote* remote) {
        if (!remote) return;
        if (remote->peer) {
            if (server == remote->peer) server = nullptr;
            remote->peer->data = nullptr;
        }
        remotes.erase(std::remove_if(remotes.begin(), remotes.end(),
                                     [&](const std::unique_ptr<Remote>& p) { return p.get() == remote; }),
                      remotes.end());
    }

    void OnDisconnect(ENetPeer* peer, enet_uint32 data) {
        Remote* remote = From(peer);
        if (!remote) {
            if (server == peer) {
                server = nullptr;
                connected = false;
            }
            return;
        }

        // ENet reports data 0 to the initiator and on timeout. A remote close keeps our code.
        const bool timeout = !remote->localClose && data == 0;
        const uint8_t id = remote->playerId;
        const bool active = remote->active;
        const bool wasServer = peer == server;

        if (hosting && active) {
            PlayerLeft left;
            left.playerId = id;
            const auto bytes = Encode(left);
            for (auto& other : remotes) {
                if (other.get() == remote || !other->active) continue;
                Send(other.get(), bytes, kChannelControl, true);
            }
            Event pl;
            pl.type = EventType::PlayerLeft;
            pl.playerId = id;
            Push(pl);
            spdlog::info("net: player {} left", static_cast<int>(id));
        }

        Event ev;
        ev.type = timeout ? EventType::TimedOut : EventType::Disconnected;
        ev.playerId = active ? id : kInvalidPlayerId;
        Push(ev);
        if (timeout) spdlog::info("net: peer timed out");

        if (wasServer) {
            server = nullptr;
            connected = false;
        }
        Erase(remote);
    }

    void OnReceive(ENetEvent& ev) {
        PacketGuard guard{ev.packet};
        ev.packet = nullptr;
        Remote* remote = From(guard.pkt ? ev.peer : nullptr);
        if (!remote || !guard.pkt || guard.pkt->data == nullptr) return;
        remote->bytesIn += guard.pkt->dataLength;

        const auto* bytes = reinterpret_cast<const uint8_t*>(guard.pkt->data);
        auto msg = Decode(bytes, guard.pkt->dataLength);
        if (!msg) {
            spdlog::warn("net: drop malformed packet ({} bytes)", guard.pkt->dataLength);
            if (hosting && !remote->active && !remote->localClose) RejectPeer(*remote, RejectReason::BadHello);
            return;
        }
        Dispatch(*remote, *msg);
    }

    void CheckHelloTimeouts() {
        if (!hosting) return;
        const auto now = Clock::now();
        std::vector<Remote*> late;
        for (auto& rp : remotes) {
            if (rp->waitHello && !rp->localClose && now > rp->helloDeadline) late.push_back(rp.get());
        }
        for (Remote* remote : late) {
            spdlog::warn("net: hello timeout");
            remote->localClose = true;
            remote->waitHello = false;
            if (remote->peer) enet_peer_disconnect(remote->peer, kDisconnectHelloTimeout);
        }
    }

    void Service() {
        if (!host || inService) return;
        inService = true;
        CheckHelloTimeouts();
        for (int i = 0; i < 256; ++i) {
            ENetEvent ev;
            const int rc = enet_host_service(host, &ev, 0);
            if (rc < 0) {
                spdlog::warn("net: host service failed");
                break;
            }
            if (rc == 0) break;
            switch (ev.type) {
            case ENET_EVENT_TYPE_CONNECT: OnConnect(ev.peer); break;
            case ENET_EVENT_TYPE_DISCONNECT: OnDisconnect(ev.peer, ev.data); break;
            case ENET_EVENT_TYPE_RECEIVE: OnReceive(ev); break;
            default: break;
            }
        }
        if (host) enet_host_flush(host);
        inService = false;
        if (deferClose) {
            deferClose = false;
            Close();
        }
    }

    bool SendState(const PlayerState& in) {
        if (!host || (!hosting && !connected)) return false;
        PlayerState state = in;
        state.playerId = localId;
        const auto bytes = Encode(state);
        if (hosting) {
            for (auto& rp : remotes) {
                if (!rp->active) continue;
                Send(rp.get(), bytes, kChannelState, false);
            }
            return true;
        }
        Remote* remote = From(server);
        if (!remote) return false;
        return Send(remote, bytes, kChannelState, false);
    }

    bool SendAnimals(const AnimalStates& states) {
        if (!host || !hosting) return false;
        const auto bytes = Encode(states);
        for (auto& rp : remotes) {
            if (!rp->active) continue;
            Send(rp.get(), bytes, kChannelState, false);
        }
        return true;
    }

    bool SendHit(const AnimalHit& hit) {
        if (!host || hosting || !connected) return false;
        Remote* remote = From(server);
        if (!remote) return false;
        return Send(remote, Encode(hit), kChannelEvents, true);
    }

    bool SendGame(const GameEvent& evIn) {
        if (!host || (!hosting && !connected)) return false;
        if (evIn.commit()) {
            if (!hosting) return false;
            const auto bytes = Encode(evIn);
            for (auto& rp : remotes) {
                if (!rp->active) continue;
                Send(rp.get(), bytes, kChannelEvents, true);
            }
            return true;
        }
        if (hosting) return false;
        Remote* remote = From(server);
        if (!remote) return false;
        GameEvent ev = evIn;
        ev.actorId = localId;
        return Send(remote, Encode(ev), kChannelEvents, true);
    }

    bool SendPing(uint32_t token) {
        if (!host || (!hosting && !connected)) return false;
        Ping ping;
        ping.token = token;
        const auto bytes = Encode(ping);
        if (hosting) {
            bool ok = false;
            for (auto& rp : remotes) {
                if (!rp->active) continue;
                ok = Send(rp.get(), bytes, kChannelControl, true) || ok;
            }
            return ok || ActiveCount() == 0;
        }
        Remote* remote = From(server);
        if (!remote) return false;
        return Send(remote, bytes, kChannelControl, true);
    }

    std::vector<PeerStats> Stats() const {
        std::vector<PeerStats> out;
        for (const auto& rp : remotes) {
            if (!rp->active) continue;
            PeerStats row;
            row.playerId = rp->playerId;
            row.name = rp->name;
            row.bytesIn = rp->bytesIn;
            row.bytesOut = rp->bytesOut;
            if (rp->peer) {
                row.rttMs = rp->peer->roundTripTime;
                row.packetLoss = static_cast<float>(rp->peer->packetLoss) / static_cast<float>(ENET_PEER_PACKET_LOSS_SCALE);
            }
            out.push_back(std::move(row));
        }
        return out;
    }
};

Session::Session() : impl_(std::make_unique<Impl>()) {}

Session::~Session() {
    if (impl_) impl_->Close();
}

bool Session::StartHost(uint16_t port, int maxClients, Identity id) {
    impl_->RequestClose();
    if (maxClients < 1) maxClients = 1;
    if (maxClients > kDefaultMaxClients) maxClients = kDefaultMaxClients;
    if (!AcquireLib()) return false;
    impl_->acquired = true;
    impl_->identity = std::move(id);
    impl_->maxClients = maxClients;

    ENetAddress addr;
    addr.host = ENET_HOST_ANY;
    addr.port = port;
    // Extra slots so a full lobby can still accept a connection long enough to Reject it.
    const size_t peers = static_cast<size_t>(maxClients) + 4;
    impl_->host = enet_host_create(&addr, peers, kChannelCount, 0, 0);
    if (!impl_->host) {
        spdlog::error("net: bind udp {} failed", static_cast<int>(port));
        ReleaseLib();
        impl_->acquired = false;
        return false;
    }
    impl_->boundPort = impl_->host->address.port;
    impl_->hosting = true;
    impl_->connected = true;
    impl_->localId = 0;
    impl_->maxPlayers = maxClients + 1;
    impl_->started = Impl::Clock::now();

    Event ev;
    ev.type = EventType::Connected;
    ev.playerId = 0;
    ev.name = impl_->identity.name;
    impl_->Push(ev);
    spdlog::info("net: hosting udp {} (max {} clients)", static_cast<int>(impl_->boundPort), maxClients);
    return true;
}

bool Session::Join(const std::string& host, uint16_t port, Identity id) {
    impl_->RequestClose();
    if (host.empty()) return false;
    if (!AcquireLib()) return false;
    impl_->acquired = true;
    impl_->identity = std::move(id);

    impl_->host = enet_host_create(nullptr, 1, kChannelCount, 0, 0);
    if (!impl_->host) {
        spdlog::error("net: client socket failed");
        ReleaseLib();
        impl_->acquired = false;
        return false;
    }

    ENetAddress addr;
    if (enet_address_set_host(&addr, host.c_str()) != 0) {
        spdlog::error("net: cannot resolve {}", host);
        enet_host_destroy(impl_->host);
        impl_->host = nullptr;
        ReleaseLib();
        impl_->acquired = false;
        return false;
    }
    addr.port = port;
    impl_->server = enet_host_connect(impl_->host, &addr, kChannelCount, 0);
    if (!impl_->server) {
        spdlog::error("net: connect to {}:{} failed", host, static_cast<int>(port));
        enet_host_destroy(impl_->host);
        impl_->host = nullptr;
        ReleaseLib();
        impl_->acquired = false;
        return false;
    }

    auto remote = std::make_unique<Remote>();
    remote->peer = impl_->server;
    impl_->server->data = remote.get();
    impl_->remotes.push_back(std::move(remote));
    impl_->boundPort = impl_->host->address.port;
    spdlog::info("net: connecting to {}:{}", host, static_cast<int>(port));
    return true;
}

void Session::Disconnect() { impl_->RequestClose(); }

void Session::Service() { impl_->Service(); }

bool Session::IsHost() const { return impl_->hosting; }

bool Session::IsConnected() const { return impl_->connected; }

uint8_t Session::LocalPlayerId() const { return impl_->localId; }

uint16_t Session::BoundPort() const { return impl_->boundPort; }

int Session::MaxPlayers() const { return impl_->maxPlayers; }

uint32_t Session::HostTickMs() const { return impl_->TickMs(); }

std::vector<PeerStats> Session::Peers() const { return impl_->Stats(); }

bool Session::SendPlayerState(const PlayerState& state) { return impl_->SendState(state); }

bool Session::SendPing(uint32_t token) { return impl_->SendPing(token); }

bool Session::SendAnimalStates(const AnimalStates& states) { return impl_->SendAnimals(states); }

bool Session::SendAnimalHit(const AnimalHit& hit) { return impl_->SendHit(hit); }

bool Session::SendGameEvent(const GameEvent& ev) { return impl_->SendGame(ev); }

bool Session::ConsumeSendTick(uint32_t nowMs) {
    if (!impl_->sendArmed || nowMs - impl_->lastSendMs >= kStateIntervalMs) {
        impl_->lastSendMs = nowMs;
        impl_->sendArmed = true;
        return true;
    }
    return false;
}

void Session::SetHandler(Handler handler) { impl_->handler = std::move(handler); }

std::vector<Event> Session::DrainEvents() {
    std::vector<Event> out;
    out.swap(impl_->events);
    return out;
}

int Session::InitializeCount() { return g_inits; }

}  // namespace net
