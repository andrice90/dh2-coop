#include "coop/coop.h"

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <vector>

#include <spdlog/spdlog.h>

#include "coop/animals.h"
#include "coop/events.h"
#include "coop/player_tracker.h"
#include "coop/puppets.h"

namespace dh2::coop::story {
void DebugActivate(const char* name);  // story.cpp
}
#include "core/exports.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/entity_api.h"
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
    GetPrivateProfileStringA("Coop", "AutoRole", "off", buf, sizeof buf, ini.c_str());
    const std::string role = buf;
    if (role == "host" || role == "join") g_settings.auto_role = role;
    GetPrivateProfileStringA("Coop", "JoinAddress", "127.0.0.1", buf, sizeof buf, ini.c_str());
    if (g_settings.auto_role == "join") g_settings.auto_join_addr = buf;
    g_settings.join_retry_s = static_cast<float>(GetPrivateProfileIntA("Coop", "JoinRetrySeconds", 5, ini.c_str()));

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

bool g_auto_stopped = false;  // coop_leave stops the AutoRole=join retry until the next coop_join/coop_host
bool g_respawn_requested = false;  // coop_respawn: same as F9, for test scripts that cannot hold the window in front

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
    g_auto_stopped = false;
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
    g_auto_stopped = false;
    RequestJoin(host, port);
}

void __cdecl cmd_leave() {
    g_auto_stopped = true;
    RequestLeave();
}

// GameDll cScr_ModHunter death (0x50350) calls this with "Death" on every model part.
// Part 0 can already read as Idle while the rest of the mesh is still playing Death.
void GuardedSetAllParts(void* ent, const char* state) {
    const HMODULE game = GetModuleHandleA("GameDll7.dll");
    if (!game || !ent || !state) return;
    auto* fn = reinterpret_cast<void(__thiscall*)(void*, const char*)>(reinterpret_cast<char*>(game) + 0x480f0);
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(fn);
    if (bytes[0] != 0x55 || bytes[1] != 0x8B || bytes[2] != 0xEC) return;
    __try {
        fn(ent, state);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

void GuardedEndCamera(void* ent) {
    using EndCamFn = void(__thiscall*)(void*);
    const HMODULE common = GetModuleHandleA("CommonDll7.dll");
    if (!common || !ent) return;
    const auto end_cam =
        reinterpret_cast<EndCamFn>(GetProcAddress(common, "?EndCamera@cObjScriptedEntity@@UAEXXZ"));
    if (!end_cam) return;
    __try {
        end_cam(ent);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

void GuardedProfileSet(void* profile, int attr, float value) {
    using SetAttrFn = void(__thiscall*)(void*, int, float);
    const HMODULE common = GetModuleHandleA("CommonDll7.dll");
    if (!common || !profile) return;
    const auto set = reinterpret_cast<SetAttrFn>(GetProcAddress(common, "?SetAttr@cProfile@@QAEXHM@Z"));
    if (!set) return;
    __try {
        set(profile, attr, value);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

// 1.5 m to the host's side, facing the host's view yaw. Same calls the F9 revive used from the start:
// the "position"/"angle" keys move the script side, SetPos/SetAngle/BspUpdate the entity.
entity::Vec3 place_near(void* hunter, const net::PlayerState& host) {
    const float yaw_rad = host.yaw * 3.14159265f / 180.f;
    const entity::Vec3 pos{host.pos[0] + std::cos(yaw_rad) * 1.5f, host.pos[1], host.pos[2] - std::sin(yaw_rad) * 1.5f};
    char buf[96];
    std::snprintf(buf, sizeof buf, "%.2f %.2f %.2f", pos.x * 100.f, pos.y * 100.f, pos.z * 100.f);
    entity::HandleKey(hunter, "position", buf);
    std::snprintf(buf, sizeof buf, "0 %.2f 0", host.yaw);
    entity::HandleKey(hunter, "angle", buf);
    entity::SetPos(hunter, pos);
    entity::SetAngle(hunter, entity::Vec3{0.f, host.yaw, 0.f});
    entity::BspUpdate(hunter);
    return pos;
}

// Several local test windows share one keyboard; only the window in front acts on F9.
bool own_foreground() {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

// Test: drop the local hunter. Does not end the level.
void __cdecl cmd_down() {
    LocalPlayerSample lp;
    if (!SampleLocalNow(lp) || !lp.entity) {
        spdlog::warn("coop_down: no hunter");
        return;
    }
    void* profile = nullptr;
    core::safe_read(static_cast<char*>(lp.entity) + 0x1F8, profile);
    GuardedProfileSet(profile, 9, 0.f);
    entity::SetBodyState(lp.entity, "Dying", true);
    spdlog::info("coop_down: hunter state='{}'", entity::BodyStateName(lp.entity));
}

// Test commands. coop_kill <id> [damage]: FuncInflictDamage on that animal. On the host or solo it is
// the real damage; on a client it hits the local mirror like a bullet would, so the mirror reports an
// AnimalHit to the host. coop_claim <id>: run the trophy-claim path as if the claim key had fired.
void __cdecl cmd_kill(const char* arg) {
    // The engine console passes exactly one token, so the damage rides as "<id>:<damage>".
    std::string a = core::safe_cstr(arg, 64);
    for (char& c : a) {
        if (c == ':' || c == ',') c = ' ';
    }
    int id = -1;
    float dmg = 1000.f;
    if (std::sscanf(a.c_str(), "%d %f", &id, &dmg) < 1 || id < 0) {
        spdlog::warn("coop_kill: usage coop_kill <animal id>[:damage]");
        return;
    }
    animals::DebugDamage(static_cast<uint16_t>(id), dmg);
}

void __cdecl cmd_respawn() { g_respawn_requested = true; }

void __cdecl cmd_tohost() { TeleportToHost("coop_tohost"); }

void __cdecl cmd_trigger(const char* arg) {
    const std::string name = core::safe_cstr(arg, 64);
    if (name.empty()) {
        spdlog::warn("coop_trigger: usage coop_trigger <trigger event name, e.g. cScrTask03>");
        return;
    }
    story::DebugActivate(name.c_str());
}

void __cdecl cmd_claim(const char* arg) {
    const int id = std::atoi(core::safe_cstr(arg, 32).c_str());
    if (id <= 0) {
        spdlog::warn("coop_claim: usage coop_claim <animal id>");
        return;
    }
    events::DebugClaim(static_cast<uint16_t>(id));
}

std::filesystem::file_time_type g_cmd_epoch{};  // set in OnInit: command files older than this are stale

// Automation channel: Bin\dh2coop.cmd, one console line per line. Read, deleted, then queued with
// cConsole::Exec (game thread). Lets test scripts drive coop_* and engine commands without typing.
void PollCommandFile() {
    const std::filesystem::path path = core::self_dir() / "dh2coop.cmd";
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) return;
    // A file left by an earlier (crashed) run must not drive this one: it once sent a fresh host to the menu.
    const auto written = std::filesystem::last_write_time(path, ec);
    if (!ec && written < g_cmd_epoch) {
        std::filesystem::remove(path, ec);
        spdlog::warn("coop: ignored stale {} (written before this process started)", path.string());
        return;
    }
    std::vector<std::string> lines;
    {
        std::ifstream in(path);
        std::string line;
        while (std::getline(in, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
            if (!line.empty()) lines.push_back(line);
        }
    }
    std::filesystem::remove(path, ec);
    for (const std::string& l : lines) {
        const int r = console::Exec(l.c_str());
        spdlog::info("coop: cmd file '{}' -> {}", l, r);
    }
}

class CoopModule : public dh2::Module {
public:
    const char* Name() const override { return "coop"; }
    int Priority() const override { return 40; }

    void OnInit() override {
        load_settings();
        g_cmd_epoch = std::filesystem::file_time_type::clock::now();
    }

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
            console_cmds::AddCommand0("coop_down", &cmd_down);
            console_cmds::AddCommandStr("coop_kill", &cmd_kill);
            console_cmds::AddCommandStr("coop_claim", &cmd_claim);
            console_cmds::AddCommandStr("coop_trigger", &cmd_trigger);
            console_cmds::AddCommand0("coop_respawn", &cmd_respawn);
            console_cmds::AddCommand0("coop_tohost", &cmd_tohost);
        }
        cmd_poll_ += dt;
        if (commands_registered_ && cmd_poll_ >= 0.5f) {
            cmd_poll_ = 0.f;
            PollCommandFile();
        }
        if (frames_ < 600 || (frames_ % 600) == 0) AssertBackground(nullptr);

        RetryAutoJoin(dt);
        ApplyPending();
        g_session.Service();
        const bool hosting = g_session.IsHost();
        const bool animals_active = hosting ? !g_session.Peers().empty() : g_session.IsConnected();
        animals::SetSessionActive(animals_active, hosting);
        for (const net::Event& ev : g_session.DrainEvents()) HandleEvent(ev);
        SendLocalState(dt);
        if (animals_active && hosting) animals::HostTick(now_ms());
        events::Pump();

        log_accum_ += dt;
        if (log_accum_ >= 1.f) {
            log_accum_ = 0.f;
            LogRemotes();
        }
        PollClientRespawn();
    }

    // No OnShutdown: it runs on the lifecycle thread and would race Service() on the game thread.
    // At process exit peers see a timeout instead of a graceful disconnect; `coop_leave` is graceful.

private:
    // AutoRole=join: the host may start later than the friend, or restart. Re-join while offline.
    void RetryAutoJoin(float dt) {
        const bool want = auto_done_ && !g_auto_stopped && g_settings.auto_role == "join" &&
                          g_settings.join_retry_s > 0.f && !g_session.IsHost() && !g_session.IsConnected();
        if (!want) {
            join_wait_ = 0.f;
            return;
        }
        join_wait_ += dt;
        // ENet's connect attempt needs a few seconds before it gives up; do not cut it short.
        if (join_wait_ < std::max(g_settings.join_retry_s, 6.f)) return;
        join_wait_ = 0.f;
        std::string host;
        uint16_t port = g_settings.port;
        split_host_port(g_settings.auto_join_addr.c_str(), host, port);
        spdlog::info("coop: auto-join retry {}:{}", host, port);
        RequestJoin(host, port);
    }

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
        if (p.kind == Pending::Kind::Host) spdlog::info("coop: host start port={}", p.port);
        else if (p.kind == Pending::Kind::Join) spdlog::info("coop: join start {}:{}", p.host, p.port);
        if (p.kind != Pending::Kind::None) {
            puppets::RemoveAll();
            animals::Reset();
            events::Reset();
        }
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
                // The new client asks for the map's events itself once its map is loaded (events kSyncRequest).
                g_remotes[ev.playerId].name = ev.name;
                spdlog::info("coop: player {} '{}' joined", ev.playerId, ev.name);
                return;
            case net::EventType::PlayerLeft:
                g_remotes.erase(ev.playerId);
                puppets::OnRemoteLeft(ev.playerId);
                events::OnPeerLeft(ev.playerId);
                spdlog::info("coop: player {} left", ev.playerId);
                return;
            case net::EventType::Welcome:
                spdlog::info("coop: welcomed as player {} (max {})", ev.playerId, ev.maxPlayers);
                return;
            case net::EventType::AnimalStates:
                animals::OnAnimalStates(ev.animalStates, now_ms());
                return;
            case net::EventType::AnimalHit:
                animals::OnAnimalHit(ev.animalHit);
                return;
            case net::EventType::GameEvent:
                events::OnNet(ev.gameEvent);
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
                    animals::Reset();
                    events::Reset();
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
        // Hunter +0x6CC: cObjHunter::OnDie. +0x368: inside a sync attack (an animal knocked this hunter down).
        int dead = 0, in_sa = 0;
        if (s.entity) {
            core::safe_read(static_cast<char*>(s.entity) + 0x6CC, dead);
            core::safe_read(static_cast<char*>(s.entity) + 0x368, in_sa);
        }
        st.flags = static_cast<uint16_t>((s.crouched ? net::kFlagCrouched : 0) | (s.reloading ? net::kFlagReloading : 0) |
                                         (s.firing ? net::kFlagFiring : 0) | (dead ? net::kFlagDead : 0) |
                                         (in_sa ? net::kFlagKnocked : 0) |
                                         ((s.shots << net::kShotCounterShift) & net::kShotCounterMask));
        st.weaponId = s.weapon;
        st.health = 100;
        g_session.SendPlayerState(st);
    }

    // F9 on a client: stand up at the host, with the health and stamina seen while alive.
    // Host death still ends the level. A client death does not.
    using SetAttrFn = void(__thiscall*)(void*, int, float);
    using GetAttrFn = float(__thiscall*)(void*, int);
    SetAttrFn set_attr_ = nullptr;
    GetAttrFn get_attr_ = nullptr;
    float full_health_ = 0.f;
    float full_stamina_ = 0.f;
    bool f9_down_ = false;

    void BindAttr() {
        if (set_attr_ && get_attr_) return;
        const HMODULE common = GetModuleHandleA("CommonDll7.dll");
        if (!common) return;
        set_attr_ = reinterpret_cast<SetAttrFn>(GetProcAddress(common, "?SetAttr@cProfile@@QAEXHM@Z"));
        get_attr_ = reinterpret_cast<GetAttrFn>(GetProcAddress(common, "?GetAttr@cProfile@@QAEMH@Z"));
    }

    static float GuardedGet(GetAttrFn fn, void* profile, int attr) {
        if (!fn || !profile) return 0.f;
        __try {
            return fn(profile, attr);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0.f;
        }
    }

    static bool GuardedSet(SetAttrFn fn, void* profile, int attr, float value) {
        if (!fn || !profile) return false;
        __try {
            fn(profile, attr, value);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    void RememberStats() {
        if (g_session.IsHost() || !g_session.IsConnected()) return;
        BindAttr();
        const LocalPlayerSample& lp = LocalPlayer();
        if (!lp.valid || !lp.entity) return;
        void* profile = nullptr;
        if (!core::safe_read(static_cast<char*>(lp.entity) + 0x1F8, profile) || !profile) return;
        const float health = GuardedGet(get_attr_, profile, 9);
        const float stamina = GuardedGet(get_attr_, profile, 5);
        if (health > full_health_) full_health_ = health;
        if (stamina > full_stamina_) full_stamina_ = stamina;
    }

    void PollClientRespawn() {
        RememberStats();
        const bool down = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        const bool pressed = down && !f9_down_;
        f9_down_ = down;
        if (pressed && own_foreground()) Respawn();
        if (g_respawn_requested) {
            g_respawn_requested = false;
            Respawn();
        }
    }

    void Respawn() {
        if (g_session.IsHost() || !g_session.IsConnected()) return;
        const auto host = g_remotes.find(0);
        if (host == g_remotes.end() || host->second.received == 0) {
            spdlog::info("coop: F9 respawn ignored, no host position yet");
            return;
        }
        LocalPlayerSample lp;
        if (!SampleLocalNow(lp) || !lp.entity) return;
        const net::PlayerState& host_state = host->second.last;

        void* profile = nullptr;
        core::safe_read(static_cast<char*>(lp.entity) + 0x1F8, profile);
        const float health = full_health_ > 1.f ? full_health_ : 100.f;
        const bool health_ok = GuardedSet(set_attr_, profile, 9, health);
        const bool stamina_ok = full_stamina_ > 1.f && GuardedSet(set_attr_, profile, 5, full_stamina_);

        const entity::Vec3 pos = place_near(lp.entity, host_state);
        // "Death" is applied to every part. Idle on part 0 leaves the mesh in the death pose.
        GuardedSetAllParts(lp.entity, "Idle");
        entity::SetAnim(lp.entity, "idlenormalneutral");
        // +0x6CC blocks the player update and the mouse look. +0xC1 bit 2 is the dead-entity bit.
        int dead_flag = -1;
        core::safe_read(static_cast<char*>(lp.entity) + 0x6CC, dead_flag);
        const int alive = 0;
        std::memcpy(static_cast<char*>(lp.entity) + 0x6CC, &alive, sizeof alive);
        unsigned char marks = 0;
        if (core::safe_read(static_cast<char*>(lp.entity) + 0xC1, marks)) {
            marks = static_cast<unsigned char>(marks & ~0x4);
            std::memcpy(static_cast<char*>(lp.entity) + 0xC1, &marks, sizeof marks);
        }
        GuardedEndCamera(lp.entity);
        spdlog::info(
            "coop: F9 respawn at ({:.1f},{:.1f},{:.1f}) health {:.1f} ok={} stamina={} deadflag={} parts='{},{},{},{}'",
            pos.x, pos.y, pos.z, health, health_ok, stamina_ok, dead_flag, entity::PartStateName(lp.entity, 0),
            entity::PartStateName(lp.entity, 1), entity::PartStateName(lp.entity, 2), entity::PartStateName(lp.entity, 3));
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
    float cmd_poll_ = 0.f;
    float join_wait_ = 0.f;
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

bool BroadcastAnimalStates(const net::AnimalStates& states) { return g_session.SendAnimalStates(states); }

bool ReportAnimalHit(const net::AnimalHit& hit) { return g_session.SendAnimalHit(hit); }

bool SendGameEvent(const net::GameEvent& ev) { return g_session.SendGameEvent(ev); }

bool SendGameEventTo(uint8_t playerId, const net::GameEvent& ev) { return g_session.SendGameEventTo(playerId, ev); }

std::vector<uint8_t> PeerIds() {
    std::vector<uint8_t> ids;
    if (!g_session.IsHost()) return ids;
    for (const auto& p : g_session.Peers()) ids.push_back(p.playerId);
    return ids;
}

bool TeleportToHost(const char* why) {
    if (g_session.IsHost() || !g_session.IsConnected()) return false;
    const auto host = g_remotes.find(0);
    if (host == g_remotes.end() || host->second.received == 0) {
        spdlog::info("coop: teleport to host ({}) skipped: no host position yet", why ? why : "");
        return false;
    }
    LocalPlayerSample lp;
    if (!SampleLocalNow(lp) || !lp.entity) return false;
    const entity::Vec3 pos = place_near(lp.entity, host->second.last);
    spdlog::info("coop: teleport to host ({}) from ({:.1f},{:.1f},{:.1f}) to ({:.1f},{:.1f},{:.1f})", why ? why : "",
                 lp.pos.x, lp.pos.y, lp.pos.z, pos.x, pos.y, pos.z);
    return true;
}

float DistanceToHost() {
    if (g_session.IsHost() || !g_session.IsConnected()) return -1.f;
    const auto host = g_remotes.find(0);
    const LocalPlayerSample& lp = LocalPlayer();
    if (host == g_remotes.end() || host->second.received == 0 || !lp.valid) return -1.f;
    const float dx = host->second.last.pos[0] - lp.pos.x;
    const float dy = host->second.last.pos[1] - lp.pos.y;
    const float dz = host->second.last.pos[2] - lp.pos.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool NetIsHost() { return g_session.IsHost(); }

bool NetIsOnline() { return g_session.IsHost() || g_session.IsConnected(); }

uint8_t NetLocalId() { return g_session.LocalPlayerId(); }

std::string StatusLine() {
    const char* role = g_session.IsHost() ? "host" : (g_session.IsConnected() ? "client" : "offline");
    const auto& lp = LocalPlayer();
    return fmt::format("role={} id={} peers={} remotes={} hunter={} animals={}", role,
                       static_cast<int>(g_session.LocalPlayerId()), g_session.Peers().size(), g_remotes.size(),
                       lp.valid ? "yes" : "no", AnimalCount());
}

}  // namespace dh2::coop
