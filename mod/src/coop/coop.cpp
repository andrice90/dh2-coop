#include "coop/coop.h"

#include <windows.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>

#include <spdlog/spdlog.h>

#include "coop/player_tracker.h"
#include "coop/puppets.h"
#include "core/exports.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/console.h"
#include "engine/console_cmds.h"
#include "net/session.h"

namespace dh2::coop {
namespace {

CoopSettings g_settings;
net::Session g_session;

struct Pending {
    enum class Kind { None, Host, Join, Leave } kind = Kind::None;
    std::string host;
    uint16_t port = 0;
};
Pending g_pending;

struct RemotePlayer {
    std::string name;
    net::PlayerState last{};
    uint32_t received = 0;
};
std::map<uint8_t, RemotePlayer> g_remotes;

int* g_allow_background = nullptr;  // &cApplication::m_bAllowBackground (EngineDll7 static)

uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

// "host[:port]" -> host, port (default kept when absent or invalid).
void split_host_port(const char* arg, std::string& host, uint16_t& port) {
    host.clear();
    if (!arg) return;
    std::string s(arg);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
    const size_t colon = s.rfind(':');
    if (colon != std::string::npos) {
        const int p = std::atoi(s.c_str() + colon + 1);
        if (p > 0 && p < 65536) port = static_cast<uint16_t>(p);
        s.resize(colon);
    }
    host = s;
}

void load_settings() {
    const std::string ini = (core::self_dir() / "dh2coop.ini").string();
    char buf[64] = {};
    g_settings.allow_background = GetPrivateProfileIntA("Coop", "AllowBackground", 1, ini.c_str()) != 0;
    g_settings.disable_slowmo = GetPrivateProfileIntA("Coop", "DisableSlowMo", 1, ini.c_str()) != 0;
    g_settings.port = static_cast<uint16_t>(GetPrivateProfileIntA("Coop", "Port", net::kDefaultPort, ini.c_str()));
    GetPrivateProfileStringA("Coop", "PlayerName", "Hunter", buf, sizeof buf, ini.c_str());
    g_settings.player_name = buf;

    // Command line switches for automated tests (run-coop-local.ps1). The engine ignores unknown switches.
    const char* cmd = GetCommandLineA();
    if (const char* h = std::strstr(cmd, "-coophost")) {
        (void)h;
        g_settings.auto_role = "host";
    } else if (const char* j = std::strstr(cmd, "-coopjoin")) {
        j += std::strlen("-coopjoin");
        while (*j == ' ') ++j;
        std::string addr;
        while (*j && *j != ' ' && *j != '"') addr.push_back(*j++);
        g_settings.auto_role = "join";
        g_settings.auto_join_addr = addr.empty() ? "127.0.0.1" : addr;
    }
    spdlog::info("coop: settings background={} disable_slowmo={} port={} name='{}' auto_role='{}' auto_join='{}'",
                 g_settings.allow_background, g_settings.disable_slowmo, g_settings.port, g_settings.player_name,
                 g_settings.auto_role, g_settings.auto_join_addr);
}

// ---- console command handlers (__cdecl, called by cConsoleFunc::CallFunction on the game thread) ----

void __cdecl cmd_status() { spdlog::info("coop_status: {}", StatusLine()); }

void __cdecl cmd_host(const char* arg) {
    uint16_t port = g_settings.port;
    std::string ignored;
    const std::string a = core::safe_cstr(arg, 64);
    spdlog::info("coop_host received arg ptr={:#x} '{}'", reinterpret_cast<uintptr_t>(arg), a);
    if (arg && *arg) {
        const int p = std::atoi(a.c_str());
        if (p > 0 && p < 65536) port = static_cast<uint16_t>(p);
    }
    RequestHost(port);
}

void __cdecl cmd_join(const char* arg) {
    const std::string a = core::safe_cstr(arg, 128);
    spdlog::info("coop_join received arg ptr={:#x} '{}'", reinterpret_cast<uintptr_t>(arg), a);
    std::string host;
    uint16_t port = g_settings.port;
    split_host_port(arg ? a.c_str() : nullptr, host, port);
    if (host.empty()) {
        spdlog::warn("coop_join: usage coop_join <host[:port]>");
        return;
    }
    RequestJoin(host, port);
}

void __cdecl cmd_leave() { RequestLeave(); }

class CoopModule : public dh2::Module {
public:
    const char* Name() const override { return "coop"; }
    int Priority() const override { return 40; }

    void OnInit() override { load_settings(); }

    void OnEngineReady(HMODULE engine) override {
        console_cmds::Bind(engine);
        g_allow_background = core::find_export_as<int*>(engine, "?m_bAllowBackground@cApplication@@2HA");
        AssertBackground("engine ready");
    }

    void OnMapLoaded(const char* bspName) override {
        if (!self_test_done_) {
            self_test_done_ = true;
            // Proves registration + argument passing end to end (handlers log what they receive).
            console::Exec("coop_status");
        }
        if (!auto_done_ && !g_settings.auto_role.empty()) {
            auto_done_ = true;
            if (g_settings.auto_role == "host") {
                RequestHost(g_settings.port);
            } else {
                std::string host;
                uint16_t port = g_settings.port;
                split_host_port(g_settings.auto_join_addr.c_str(), host, port);
                RequestJoin(host, port);
            }
        }
        spdlog::info("coop: map '{}' loaded; {}", bspName ? bspName : "", StatusLine());
    }

    void OnFrame(float dt) override {
        ++frames_;
        if (!commands_registered_ && console_cmds::Ready()) {
            commands_registered_ = true;
            console_cmds::AddCommand0("coop_status", &cmd_status);
            console_cmds::AddCommandStr("coop_host", &cmd_host);
            console_cmds::AddCommandStr("coop_join", &cmd_join);
            console_cmds::AddCommand0("coop_leave", &cmd_leave);
        }
        if (frames_ < 600 || (frames_ % 600) == 0) AssertBackground(nullptr);

        ApplyPending();
        g_session.Service();
        for (const net::Event& ev : g_session.DrainEvents()) HandleEvent(ev);
        SendLocalState(dt);

        log_accum_ += dt;
        if (log_accum_ >= 1.f) {
            log_accum_ = 0.f;
            LogRemotes();
        }
    }

    // No OnShutdown: it runs on the lifecycle thread and would race Service() on the game thread.
    // At process exit peers see a timeout instead of a graceful disconnect; `coop_leave` is graceful.

private:
    void AssertBackground(const char* why) {
        if (!g_settings.allow_background || !g_allow_background) return;
        int cur = 0;
        if (!core::safe_read(g_allow_background, cur)) return;
        if (cur != 1) {
            *g_allow_background = 1;
            spdlog::info("coop: cApplication::m_bAllowBackground {} -> 1{}{}", cur, why ? " at " : "", why ? why : "");
        }
    }

    void ApplyPending() {
        Pending p = g_pending;
        g_pending = {};
        net::Session::Identity id;
        id.name = g_settings.player_name;
        if (p.kind != Pending::Kind::None) puppets::RemoveAll();
        switch (p.kind) {
            case Pending::Kind::None:
                return;
            case Pending::Kind::Host:
                g_session.Disconnect();
                g_remotes.clear();
                spdlog::info("coop: hosting on UDP {} -> {}", p.port, g_session.StartHost(p.port, net::kDefaultMaxClients, id));
                return;
            case Pending::Kind::Join:
                g_session.Disconnect();
                g_remotes.clear();
                spdlog::info("coop: joining {}:{} -> {}", p.host, p.port, g_session.Join(p.host, p.port, id));
                return;
            case Pending::Kind::Leave:
                g_session.Disconnect();
                g_remotes.clear();
                spdlog::info("coop: left session");
                return;
        }
    }

    void HandleEvent(const net::Event& ev) {
        switch (ev.type) {
            case net::EventType::PlayerState: {
                RemotePlayer& r = g_remotes[ev.playerId];
                r.last = ev.state;
                ++r.received;
                puppets::OnRemoteState(ev.playerId, ev.state, now_ms());
                return;
            }
            case net::EventType::PlayerJoined:
                g_remotes[ev.playerId].name = ev.name;
                spdlog::info("coop: player {} '{}' joined", ev.playerId, ev.name);
                return;
            case net::EventType::PlayerLeft:
                g_remotes.erase(ev.playerId);
                puppets::OnRemoteLeft(ev.playerId);
                spdlog::info("coop: player {} left", ev.playerId);
                return;
            case net::EventType::Welcome:
                spdlog::info("coop: welcomed as player {} (max {})", ev.playerId, ev.maxPlayers);
                return;
            case net::EventType::Reject:
                spdlog::warn("coop: rejected ({})", net::ToString(ev.reason));
                return;
            case net::EventType::Disconnected:
            case net::EventType::TimedOut:
                spdlog::info("coop: {} player {}", ev.type == net::EventType::TimedOut ? "timeout" : "disconnect",
                             ev.playerId);
                if (!g_session.IsHost()) {
                    g_remotes.clear();
                    puppets::RemoveAll();
                }
                return;
            default:
                return;
        }
    }

    void SendLocalState(float dt) {
        (void)dt;
        if (!g_session.IsHost() && !g_session.IsConnected()) return;
        if (!g_session.ConsumeSendTick(now_ms())) return;
        LocalPlayerSample s;
        if (!SampleLocalNow(s)) return;
        net::PlayerState st{};
        st.seq = ++seq_;
        st.tickMs = now_ms();
        st.pos[0] = s.pos.x;
        st.pos[1] = s.pos.y;
        st.pos[2] = s.pos.z;
        // Yaw convention: degrees, atan2(dx, dz) — 0° = +z, 90° = +x. Prefer the render camera (live view, also
        // while standing still); the hunter's +0xF4.y heading only follows the view while moving (unwrapped).
        float yaw = std::fmod(s.viewValid ? s.viewYaw : s.angle.y, 360.f);
        if (yaw < 0.f) yaw += 360.f;
        st.yaw = yaw;
        st.pitch = s.viewValid ? s.viewPitch : 0.f;  // degrees, + = looking up
        if (have_prev_) {
            const float sec = static_cast<float>(st.tickMs - prev_tick_) / 1000.f;
            if (sec > 0.f) {
                st.velocity[0] = (s.pos.x - prev_.x) / sec;
                st.velocity[1] = (s.pos.y - prev_.y) / sec;
                st.velocity[2] = (s.pos.z - prev_.z) / sec;
            }
        }
        prev_ = s.pos;
        prev_tick_ = st.tickMs;
        have_prev_ = true;
        st.flags = static_cast<uint16_t>((s.crouched ? net::kFlagCrouched : 0) | (s.reloading ? net::kFlagReloading : 0) |
                                         (s.firing ? net::kFlagFiring : 0) |
                                         ((s.shots << net::kShotCounterShift) & net::kShotCounterMask));
        st.weaponId = s.weapon;
        st.health = 100;
        g_session.SendPlayerState(st);
    }

    void LogRemotes() {
        for (auto& [id, r] : g_remotes) {
            if (!r.received) continue;
            const auto& s = r.last;
            spdlog::info("coop: remote {} '{}' pos=({:.2f},{:.2f},{:.2f}) yaw={:.3f} seq={} rx={}", id, r.name,
                         s.pos[0], s.pos[1], s.pos[2], s.yaw, s.seq, r.received);
        }
    }

    std::uint64_t frames_ = 0;
    bool commands_registered_ = false;
    bool self_test_done_ = false;
    bool auto_done_ = false;
    float log_accum_ = 0.f;
    uint16_t seq_ = 0;
    Vec3 prev_{};
    uint32_t prev_tick_ = 0;
    bool have_prev_ = false;
};

DH2_MODULE(CoopModule)

}  // namespace

const CoopSettings& Settings() { return g_settings; }

void RequestHost(uint16_t port) {
    g_pending = {};
    g_pending.kind = Pending::Kind::Host;
    g_pending.port = port;
}

void RequestJoin(const std::string& host, uint16_t port) {
    g_pending = {};
    g_pending.kind = Pending::Kind::Join;
    g_pending.host = host;
    g_pending.port = port;
}

void RequestLeave() {
    g_pending = {};
    g_pending.kind = Pending::Kind::Leave;
}

SessionView Snapshot() {
    SessionView v;
    v.role = g_session.IsHost() ? "host" : (g_session.IsConnected() ? "client" : "offline");
    v.localId = g_session.LocalPlayerId();
    v.port = g_settings.port;
    const auto& lp = LocalPlayer();
    v.hunterFound = lp.valid;
    v.hunterPos[0] = lp.pos.x;
    v.hunterPos[1] = lp.pos.y;
    v.hunterPos[2] = lp.pos.z;
    v.hunterYaw = lp.angle.y;
    v.animals = AnimalCount();
    std::map<uint8_t, uint32_t> rtt;
    for (const auto& p : g_session.Peers()) rtt[p.playerId] = p.rttMs;
    for (const auto& [id, r] : g_remotes) {
        if (v.remoteCount >= 4) break;
        RemoteView& rv = v.remotes[v.remoteCount++];
        rv.id = id;
        rv.name = r.name;
        rv.pos[0] = r.last.pos[0];
        rv.pos[1] = r.last.pos[1];
        rv.pos[2] = r.last.pos[2];
        rv.yaw = r.last.yaw;
        rv.packets = r.received;
        // Host sees each client as a peer; a client has one peer (the host) whose rtt applies to everyone.
        auto it = rtt.find(id);
        rv.rttMs = it != rtt.end() ? it->second : (rtt.empty() ? 0 : rtt.begin()->second);
    }
    return v;
}

std::string StatusLine() {
    const char* role = g_session.IsHost() ? "host" : (g_session.IsConnected() ? "client" : "offline");
    const auto& lp = LocalPlayer();
    return fmt::format("role={} id={} peers={} remotes={} hunter={} animals={}", role,
                       static_cast<int>(g_session.LocalPlayerId()), g_session.Peers().size(), g_remotes.size(),
                       lp.valid ? "yes" : "no", AnimalCount());
}

}  // namespace dh2::coop
