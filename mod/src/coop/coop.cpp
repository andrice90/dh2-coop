#include "coop/coop.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <algorithm>
#include <cctype>
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
#include "coop/puppet_models.h"
#include "coop/puppets.h"
#include "coop/test_fake.h"
#include "coop/vehicles.h"

namespace dh2::coop::story {
void DebugActivate(const char* name);  // story.cpp
}
#include "core/exports.h"
#include "core/hook.h"
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
    int model = 0;
    net::PlayerState last{};
    uint32_t received = 0;
};
std::map<uint8_t, RemotePlayer> g_remotes;

int* g_allow_background = nullptr;  // &cApplication::m_bAllowBackground (EngineDll7 static)

// Hello identity: two friends may only play when their dh2coop.asi files are identical (and passwords match).
uint32_t g_mod_build = 0;
uint32_t g_pass_hash = 0;
std::string g_notice;  // on-screen line (Snapshot().notice) until g_notice_until
uint32_t g_notice_until = 0;

// Lobby status line (Lobby()). A reject is followed by a disconnect: Failed keeps the reject's text.
LobbyState g_lobby_state = LobbyState::Offline;
std::string g_lobby_msg = "Not in a co-op game.";
std::string g_join_target;  // "host:port" of the current or last join
uint16_t g_join_port = 0;
uint32_t g_join_started_ms = 0;

// Host level recovery (HostRestartLevel / HostNextLevel, F5 / F6). Restart goes through the main menu: loading
// the map that is already loaded resets it in place, the same-map replay that crashed at the next teardown.
struct LevelJump {
    int armed = 0;              // 1 restart, 2 next level: the first press, waiting for the second
    uint32_t armed_until = 0;
    std::string restart_map;    // set while hopping through the main menu
    uint32_t restart_at = 0;    // menu loaded: load restart_map at this time
    bool f5_down = false;
    bool f6_down = false;
} g_jump;

void set_lobby(LobbyState state, std::string msg) {
    if (g_lobby_state != state || g_lobby_msg != msg) spdlog::info("coop: lobby -> {}", msg);
    g_lobby_state = state;
    g_lobby_msg = std::move(msg);
}

uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

uint32_t fnv1a(const void* data, size_t n, uint32_t h = 2166136261u) {
    const auto* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < n; ++i) h = (h ^ p[i]) * 16777619u;
    return h;
}

uint32_t hash_own_asi() {
    std::ifstream in(core::self_path(), std::ios::binary);
    if (!in) return 0;
    uint32_t h = 2166136261u;
    char buf[64 * 1024];
    while (in.read(buf, sizeof buf) || in.gcount() > 0) h = fnv1a(buf, static_cast<size_t>(in.gcount()), h);
    return h ? h : 1;
}

void show_notice(std::string text, uint32_t ms) {
    g_notice = std::move(text);
    g_notice_until = now_ms() + ms;
}

// This PC's addresses for the F8 panel and the host's menu line. Friends play over Tailscale: its adapter is named
// "Tailscale" and hands out 100.64.0.0/10 addresses. Adapters that are down, loopback and link-local are skipped.
// Refreshed every few seconds on the game thread (Tailscale may connect after the game starts).
struct LocalAddrs {
    std::string tailnet;  // first IPv4 of the Tailscale adapter ("" = Tailscale not up)
    std::string others;   // up to two other IPv4s (LAN, other VPNs)
};
LocalAddrs g_addrs;
uint32_t g_addrs_ms = 0;

LocalAddrs scan_local_addresses() {
    LocalAddrs out;
    ULONG size = 16 * 1024;
    std::vector<unsigned char> buf;
    ULONG r = ERROR_BUFFER_OVERFLOW;
    for (int tries = 0; tries < 3 && r == ERROR_BUFFER_OVERFLOW; ++tries) {
        buf.resize(size);
        r = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                                 nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data()), &size);
    }
    if (r != NO_ERROR) return out;
    int others = 0;
    for (auto* a = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data()); a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        const bool tailscale_name = (a->FriendlyName && wcsstr(a->FriendlyName, L"Tailscale")) ||
                                    (a->Description && wcsstr(a->Description, L"Tailscale"));
        for (auto* u = a->FirstUnicastAddress; u; u = u->Next) {
            const auto* in = reinterpret_cast<const sockaddr_in*>(u->Address.lpSockaddr);
            if (!in || in->sin_family != AF_INET) continue;
            const uint32_t ip = ntohl(in->sin_addr.s_addr);
            const uint32_t b0 = ip >> 24, b1 = (ip >> 16) & 0xff;
            if (ip == 0 || b0 == 127 || (b0 == 169 && b1 == 254)) continue;
            char text[INET_ADDRSTRLEN] = {};
            if (!inet_ntop(AF_INET, &in->sin_addr, text, sizeof text)) continue;
            const bool cgnat = b0 == 100 && (b1 & 0xc0) == 64;
            if (out.tailnet.empty() && (tailscale_name || cgnat)) {
                out.tailnet = text;
            } else if (others < 2) {
                out.others += (others++ ? ", " : "") + std::string(text);
            }
        }
    }
    return out;
}

void refresh_local_addresses() {
    const uint32_t now = now_ms();
    if (g_addrs_ms != 0 && now - g_addrs_ms < 10000) return;
    const bool first = g_addrs_ms == 0;
    g_addrs_ms = now ? now : 1;
    LocalAddrs a = scan_local_addresses();
    if (first || a.tailnet != g_addrs.tailnet) {
        spdlog::info("coop: tailnet address {} (other addresses: {})", a.tailnet.empty() ? "none" : a.tailnet,
                     a.others.empty() ? "none" : a.others);
    }
    g_addrs = std::move(a);
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
    GetPrivateProfileStringA("Coop", "Password", "", buf, sizeof buf, ini.c_str());
    g_settings.password = buf;
    g_settings.model = static_cast<int>(GetPrivateProfileIntA("Coop", "PuppetModel", 0, ini.c_str()));
    static const char kSalt[] = "dh2coop-password";
    g_pass_hash = g_settings.password.empty()
                      ? 0
                      : fnv1a(g_settings.password.data(), g_settings.password.size(), fnv1a(kSalt, sizeof kSalt - 1));
    g_mod_build = hash_own_asi();

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
    spdlog::info(
        "coop: settings background={} disable_slowmo={} port={} name='{}' auto_role='{}' auto_join='{}' build={:08x} "
        "password={} protocol={}",
        g_settings.allow_background, g_settings.disable_slowmo, g_settings.port, g_settings.player_name,
        g_settings.auto_role, g_settings.auto_join_addr, g_mod_build, g_pass_hash ? "set" : "none",
        static_cast<int>(net::kProtocolVersion));
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

// On the host's own spot, 0.25 m up, facing the host's view yaw. 1.5 m to the side at the host's height dropped
// players through the map on slopes and ledges (two-PC test 2026-10-17); the host's spot is known ground, and the
// host's puppet no longer collides with players. Same calls the F9 revive used from the start: the
// "position"/"angle" keys move the script side and the body, SetPos/SetAngle/BspUpdate the entity.
entity::Vec3 place_near(void* hunter, const net::PlayerState& host) {
    const entity::Vec3 pos{host.pos[0], host.pos[1] + 0.25f, host.pos[2]};
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

// Test: coop_tp <x>:<y>:<z> moves the local hunter there (metres, the coordinates the logs print).
void __cdecl cmd_tp(const char* arg) {
    std::string a = core::safe_cstr(arg, 64);
    for (char& c : a) {
        if (c == ':' || c == ',') c = ' ';
    }
    float x = 0.f, y = 0.f, z = 0.f, yaw = 0.f;
    LocalPlayerSample lp;
    const int n = std::sscanf(a.c_str(), "%f %f %f %f", &x, &y, &z, &yaw);
    if (n < 3 || !SampleLocalNow(lp) || !lp.entity) {
        spdlog::warn("coop_tp: usage coop_tp <x>:<y>:<z>[:<yaw degrees>]");
        return;
    }
    net::PlayerState at{};
    at.pos[0] = x;
    at.pos[1] = y - 0.25f;  // place_near lifts by 0.25
    at.pos[2] = z;
    at.yaw = n >= 4 ? yaw : (lp.viewValid ? lp.viewYaw : lp.angle.y);
    const entity::Vec3 pos = place_near(lp.entity, at);
    spdlog::info("coop_tp: hunter to ({:.1f},{:.1f},{:.1f})", pos.x, pos.y, pos.z);
}

void __cdecl cmd_trigger(const char* arg) {
    const std::string name = core::safe_cstr(arg, 64);
    if (name.empty()) {
        spdlog::warn("coop_trigger: usage coop_trigger <trigger event name, e.g. cScrTask03>");
        return;
    }
    story::DebugActivate(name.c_str());
}

// Test: the lobby buttons without a mouse (name / model from dh2coop-lobby.ini). Console commands take one word,
// so: coop_lobby host | leave | <ip[:port]> (join).
void __cdecl cmd_lobby(const char* arg) {
    const std::string a = core::safe_cstr(arg, 96);
    const LobbyPrefs p = LoadLobbyPrefs();
    if (a == "host") {
        LobbyHost(p.name, p.model, p.host_port);
    } else if (a == "leave") {
        LobbyLeave();
    } else if (!a.empty()) {
        std::string host;
        uint16_t port = p.join_port;
        split_host_port(a.c_str(), host, port);
        LobbyJoin(p.name, p.model, host, port);
    } else {
        spdlog::warn("coop_lobby: usage coop_lobby host | leave | <ip[:port]>");
    }
}

// Test: F5 / F6 without the keyboard. coop_level restart | next (arms and confirms in one go).
void __cdecl cmd_level(const char* arg) {
    const std::string a = core::safe_cstr(arg, 32);
    if (a == "restart") {
        if (!HostRestartLevel()) HostRestartLevel();
    } else if (a == "next") {
        if (!HostNextLevel()) HostNextLevel();
    } else {
        spdlog::warn("coop_level: usage coop_level restart | next");
    }
}

// Test: draw remote player <id> with puppet model <index> on this machine only (model preview).
void __cdecl cmd_puppetmodel(const char* arg) {
    std::string a = core::safe_cstr(arg, 32);
    for (char& c : a) {
        if (c == ':' || c == ',') c = ' ';
    }
    int id = -1, model = -1;
    if (std::sscanf(a.c_str(), "%d %d", &id, &model) != 2 || id < 0 || id > 255 || model < 0 ||
        model >= PuppetModelCount()) {
        spdlog::warn("coop_puppetmodel: usage coop_puppetmodel <player id>:<model 0..{}>", PuppetModelCount() - 1);
        return;
    }
    puppets::SetModel(static_cast<uint8_t>(id), model);
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

class CoopModule;
CoopModule* g_coop = nullptr;
// Our frame work runs from IDirect3DDevice8::Present. cRenderDX::Flip skips Present while the device is lost
// (fullscreen alt-tab, minimized), so without this pump nothing serviced ENet and peers timed out (review H2).
safetyhook::InlineHook* g_flip_hook = nullptr;
int __fastcall hk_Flip(void* self, void*);

class CoopModule : public dh2::Module {
public:
    const char* Name() const override { return "coop"; }
    int Priority() const override { return 40; }

    void OnInit() override {
        g_coop = this;
        load_settings();
        g_cmd_epoch = std::filesystem::file_time_type::clock::now();
        events::SetHandler(events::kHostNotice, nullptr,
                           [](const net::GameEvent& ev) { show_notice(ev.key, 6000); });
    }

    void OnEngineReady(HMODULE engine) override {
        console_cmds::Bind(engine);
        g_allow_background = core::find_export_as<int*>(engine, "?m_bAllowBackground@cApplication@@2HA");
        AssertBackground("engine ready");
        if (void* flip = core::find_export(engine, "?Flip@cRenderDX@@UAEHXZ")) {
            g_flip_hook = core::install_hook("cRenderDX::Flip (net pump while no frame is presented)", flip,
                                             reinterpret_cast<void*>(&hk_Flip));
        }
    }

    void OnMapLoaded(const char* bspName) override {
        // Restart hop: the main menu is up, the level follows a moment later (OnFrame). Any other map ends the hop.
        if (!g_jump.restart_map.empty()) {
            std::string label = bspName ? bspName : "";
            for (char& c : label) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            const bool menu = label.find("menumap") != std::string::npos;
            if (menu && g_jump.restart_at == 0) {
                g_jump.restart_at = now_ms() + 1500;
            } else if (!menu) {
                g_jump.restart_map.clear();
                g_jump.restart_at = 0;
            }
        }
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
            console_cmds::AddCommandStr("coop_tp", &cmd_tp);
            console_cmds::AddCommandStr("coop_lobby", &cmd_lobby);
            console_cmds::AddCommandStr("coop_puppetmodel", &cmd_puppetmodel);
            console_cmds::AddCommandStr("coop_level", &cmd_level);
        }
        cmd_poll_ += dt;
        if (commands_registered_ && cmd_poll_ >= 0.5f) {
            cmd_poll_ = 0.f;
            PollCommandFile();
        }
        if (frames_ < 600 || (frames_ % 600) == 0) AssertBackground(nullptr);

        RetryAutoJoin(dt);
        refresh_local_addresses();
        ApplyPending();
        CheckJoinTimeout();
        PollLevelKeys();
        g_frame_thread = GetCurrentThreadId();
        g_last_frame_ms = now_ms();
        if (g_flip_pumping) {
            g_flip_pumping = false;
            spdlog::info("coop: frames are presented again; network back on the frame hook");
        }
        NetStep(dt);

        log_accum_ += dt;
        if (log_accum_ >= 1.f) {
            log_accum_ = 0.f;
            LogRemotes();
        }
        PollClientRespawn();
    }

    // No OnShutdown: it runs on the lifecycle thread and would race Service() on the game thread.
    // At process exit peers see a timeout instead of a graceful disconnect; `coop_leave` is graceful.

    // The network half of a frame. Also run from cRenderDX::Flip while no frame is presented (hk_Flip).
    void NetStep(float dt) {
        g_session.Service();
        const bool hosting = g_session.IsHost();
        const bool animals_active = hosting ? !g_session.Peers().empty() : g_session.IsConnected();
        animals::SetSessionActive(animals_active, hosting);
        for (const net::Event& ev : g_session.DrainEvents()) HandleEvent(ev);
        SendLocalState(dt);
        if (animals_active && hosting) animals::HostTick(now_ms());
        events::Pump();
    }

    static inline DWORD g_frame_thread = 0;
    static inline uint32_t g_last_frame_ms = 0;
    static inline bool g_flip_pumping = false;

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
        id.modBuild = g_mod_build;
        id.passHash = g_pass_hash;
        id.model = static_cast<uint8_t>(g_settings.model);
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
                DropSession();
            {
                const bool ok = g_session.StartHost(p.port, net::kDefaultMaxClients, id);
                spdlog::info("coop: hosting on UDP {} -> {}", p.port, ok);
                if (ok) {
                    set_lobby(LobbyState::Hosting, fmt::format("Lobby open on UDP port {}.", p.port));
                } else {
                    const std::string why = fmt::format(
                        "Could not host on UDP port {}. Another program (or another copy of the game) already uses "
                        "it.",
                        p.port);
                    set_lobby(LobbyState::Failed, why);
                    show_notice(why, 30000);
                }
                return;
            }
            case Pending::Kind::Join: {
                DropSession();
                g_join_target = fmt::format("{}:{}", p.host, p.port);
                g_join_port = p.port;
                g_join_started_ms = now_ms();
                const bool ok = g_session.Join(p.host, p.port, id);
                spdlog::info("coop: joining {}:{} -> {}", p.host, p.port, ok);
                if (ok) {
                    set_lobby(LobbyState::Connecting, fmt::format("Connecting to {} ...", g_join_target));
                } else {
                    set_lobby(LobbyState::Failed,
                              fmt::format("'{}' is not an address this PC can reach. Type the host's tailnet IP "
                                          "(100.x.y.z).",
                                          p.host));
                }
                return;
            }
            case Pending::Kind::Leave:
                DropSession();
                set_lobby(LobbyState::Offline, "Not in a co-op game.");
                spdlog::info("coop: left session");
                return;
        }
    }

    // Ends the current session and throws away its queued events: a late disconnect of an earlier attempt must not
    // land on the next one (it said "No answer from" the address that then connected, 2026-10-17 23:05).
    void DropSession() {
        g_session.Disconnect();
        const size_t dropped = g_session.DrainEvents().size();
        if (dropped) spdlog::info("coop: dropped {} event(s) of the previous session", dropped);
        g_remotes.clear();
    }

    // F5 restart / F6 next level (host; window in front), and the second half of a restart hop.
    void PollLevelKeys() {
        const bool f5 = (GetAsyncKeyState(VK_F5) & 0x8000) != 0;
        const bool f6 = (GetAsyncKeyState(VK_F6) & 0x8000) != 0;
        if (own_foreground()) {
            if (f5 && !g_jump.f5_down) HostRestartLevel();
            if (f6 && !g_jump.f6_down) HostNextLevel();
        }
        g_jump.f5_down = f5;
        g_jump.f6_down = f6;
        if (g_jump.restart_at != 0 && static_cast<int32_t>(now_ms() - g_jump.restart_at) >= 0) {
            const std::string cmd = "mod_spmap " + g_jump.restart_map;
            g_jump.restart_map.clear();
            g_jump.restart_at = 0;
            spdlog::info("coop: restart hop: {} -> {}", cmd, console::Exec(cmd.c_str()));
        }
    }

    // ENet keeps retrying an unanswered connect for longer than a player waits. The lobby gives up after 8 s.
    void CheckJoinTimeout() {
        if (g_lobby_state != LobbyState::Connecting || g_session.IsConnected() || g_session.IsHost()) return;
        if (now_ms() - g_join_started_ms < 8000) return;
        DropSession();
        set_lobby(LobbyState::Failed, fmt::format("No answer from {}. Check the IP and port, that the host pressed "
                                                  "HOST, and that both PCs are connected to Tailscale.",
                                                  g_join_target));
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
                g_remotes[ev.playerId].model = ev.model;
                puppets::SetModel(ev.playerId, ev.model);
                spdlog::info("coop: player {} '{}' joined (model {})", ev.playerId, ev.name, ev.model);
                return;
            case net::EventType::PlayerLeft:
                g_remotes.erase(ev.playerId);
                puppets::OnRemoteLeft(ev.playerId);
                events::OnPeerLeft(ev.playerId);
                spdlog::info("coop: player {} left", ev.playerId);
                return;
            case net::EventType::Welcome:
                spdlog::info("coop: welcomed as player {} (max {})", ev.playerId, ev.maxPlayers);
                set_lobby(LobbyState::Connected, fmt::format("Connected to {} as player {}.", g_join_target,
                                                             static_cast<int>(ev.playerId)));
                return;
            case net::EventType::AnimalStates:
                // Snapshots and kMapStart travel on different channels: after a host map change the first
                // new-map snapshot can arrive while this client is still on the old map with the same ids.
                if (!events::SnapshotIsCurrent(ev.animalStates.mapSerial)) return;
                animals::OnAnimalStates(ev.animalStates, now_ms());
                return;
            case net::EventType::AnimalHit:
                animals::OnAnimalHit(ev.animalHit);
                return;
            case net::EventType::VehicleStates:
                if (!events::SnapshotIsCurrent(ev.vehicleStates.mapSerial)) return;
                vehicles::OnStates(ev.vehicleStates, now_ms());
                return;
            case net::EventType::GameEvent:
                events::OnNet(ev.gameEvent);
                return;
            case net::EventType::Reject: {
                spdlog::warn("coop: rejected ({})", net::ToString(ev.reason));
                if (g_session.IsHost()) return;
                std::string why;
                switch (ev.reason) {
                    case net::RejectReason::BuildMismatch:
                        why = fmt::format("The host runs a different co-op kit (host {:08x}, you {:08x}). Install "
                                          "the same kit as the host.",
                                          ev.modBuild, g_mod_build);
                        break;
                    case net::RejectReason::VersionMismatch:
                        why = "The host runs a different co-op kit version. Install the same kit as the host.";
                        break;
                    case net::RejectReason::BadHello:
                        why = "The host runs an older co-op kit. Install the same kit as the host.";
                        break;
                    case net::RejectReason::BadPassword:
                        why = "Wrong co-op password: [Coop] Password in dh2coop.ini must match the host's.";
                        break;
                    case net::RejectReason::LobbyFull: why = "The host's game is full (4 players)."; break;
                    default: why = fmt::format("The host refused this game ({}).", net::ToString(ev.reason)); break;
                }
                show_notice(why, 30000);
                set_lobby(LobbyState::Failed, why);
                return;
            }
            case net::EventType::Disconnected:
            case net::EventType::TimedOut:
                spdlog::info("coop: {} player {}", ev.type == net::EventType::TimedOut ? "timeout" : "disconnect",
                             ev.playerId);
                if (!g_session.IsHost()) {
                    if (g_lobby_state == LobbyState::Connecting) {
                        set_lobby(LobbyState::Failed,
                                  fmt::format("No answer from {}. Check the IP and port, that the host pressed HOST, "
                                              "and that both PCs are connected to Tailscale.",
                                              g_join_target));
                    } else if (g_lobby_state == LobbyState::Connected) {
                        set_lobby(LobbyState::Failed, fmt::format("Lost the connection to the host ({}).", g_join_target));
                    }
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
        // +0xA4: parent entity id, set while the hunter sits in a vehicle (GetIn attaches it to a place_N tag).
        int dead = 0, in_sa = 0;
        int16_t parent = -1;
        if (s.entity) {
            core::safe_read(static_cast<char*>(s.entity) + 0x6CC, dead);
            core::safe_read(static_cast<char*>(s.entity) + 0x368, in_sa);
            core::safe_read(static_cast<char*>(s.entity) + 0xA4, parent);
        }
        st.flags = static_cast<uint16_t>((s.crouched ? net::kFlagCrouched : 0) | (s.reloading ? net::kFlagReloading : 0) |
                                         (s.firing ? net::kFlagFiring : 0) | (dead ? net::kFlagDead : 0) |
                                         (in_sa ? net::kFlagKnocked : 0) | (parent >= 0 ? net::kFlagRiding : 0) |
                                         ((s.shots << net::kShotCounterShift) & net::kShotCounterMask));
        st.weaponId = s.weapon;
        st.health = 100;
        testfake::Apply(st);  // no-op unless a coop_fake* test command is active
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

void flip_net_step(float dt) { g_coop->NetStep(dt); }

bool guarded_flip_net_step(float dt) {
    __try {
        flip_net_step(dt);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int __fastcall hk_Flip(void* self, void*) {
    const int r = g_flip_hook->unsafe_thiscall<int>(self);
    // Only on the game thread, and only while OnFrame has not run for a while (Present skipped).
    if (!g_coop || CoopModule::g_frame_thread == 0 || GetCurrentThreadId() != CoopModule::g_frame_thread) return r;
    static uint32_t last = 0;
    static bool fault_logged = false;
    const uint32_t now = now_ms();
    if (now - CoopModule::g_last_frame_ms < 250 || now - last < 15) return r;
    const float dt = (last != 0 && now - last < 250) ? static_cast<float>(now - last) / 1000.f : 0.016f;
    last = now;
    if (!CoopModule::g_flip_pumping) {
        CoopModule::g_flip_pumping = true;
        spdlog::info("coop: no frame presented for {} ms (device lost / minimized); network from cRenderDX::Flip",
                     now - CoopModule::g_last_frame_ms);
    }
    if (!guarded_flip_net_step(dt) && !fault_logged) {
        fault_logged = true;
        spdlog::warn("coop: fault in the Flip network pump");
    }
    return r;
}

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
        rv.flags = r.last.flags;
        // Host sees each client as a peer; a client has one peer (the host) whose rtt applies to everyone.
        auto it = rtt.find(id);
        rv.rttMs = it != rtt.end() ? it->second : (rtt.empty() ? 0 : rtt.begin()->second);
    }
    v.tailnetIp = g_addrs.tailnet;
    v.otherIps = g_addrs.others;
    if (!g_notice.empty() && static_cast<int32_t>(g_notice_until - now_ms()) > 0) {
        v.notice = g_notice;
    } else if (events::CurrentMap() == "MENUMAP") {
        // The kit starts everyone at the main menu: say what happens next.
        const size_t friends = g_remotes.size();
        if (g_session.IsHost()) {
            const std::string join_with = !g_addrs.tailnet.empty() ? g_addrs.tailnet : g_addrs.others;
            v.notice = fmt::format("Co-op: hosting on UDP {}, {} friend{} connected.{}{}\nStart a new game or load a "
                                   "save: everyone follows you.",
                                   g_settings.port, friends, friends == 1 ? "" : "s",
                                   join_with.empty() ? "" : "\nFriends join with: ", join_with);
        } else if (g_session.IsConnected()) {
            v.notice = "Co-op: connected to the host.\nStay in this menu: you follow the host into its game.";
        } else if (g_settings.auto_role == "join") {
            v.notice = fmt::format("Co-op: looking for the host at {} (retrying)...", g_settings.auto_join_addr);
        } else {
            v.notice = "Co-op: press F8 for the lobby (host or join).";
        }
        v.noticeIsBanner = true;
    }
    return v;
}

// ---- lobby ----

namespace {

// Next to the logs: the game's Bin, or %LOCALAPPDATA%\dh2coop for a game under Program Files. Each local test tree
// keeps its own names that way.
std::string lobby_ini_path() { return (core::log_dir() / "dh2coop-lobby.ini").string(); }

std::string clean_name(std::string s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
    if (s.size() > net::kNameBytes - 1) s.resize(net::kNameBytes - 1);
    return s.empty() ? std::string("Hunter") : s;
}

void apply_identity(const std::string& name, int model) {
    g_settings.player_name = clean_name(name);
    g_settings.model = (model >= 0 && model < PuppetModelCount()) ? model : 0;
    // The lobby replaces the launcher roles: no more automatic re-joins behind the player's back.
    g_settings.auto_role.clear();
    g_auto_stopped = true;
}

}  // namespace

LobbyPrefs LoadLobbyPrefs() {
    LobbyPrefs p;
    const std::string game_ini = (core::self_dir() / "dh2coop.ini").string();
    char buf[128] = {};
    GetPrivateProfileStringA("Coop", "JoinAddress", "", buf, sizeof buf, game_ini.c_str());
    const std::string ini = lobby_ini_path();
    p.name = g_settings.player_name;
    p.model = g_settings.model;
    p.join_host = (std::string(buf) == "127.0.0.1") ? std::string() : std::string(buf);
    p.join_port = g_settings.port;
    p.host_port = g_settings.port;
    GetPrivateProfileStringA("Lobby", "Name", p.name.c_str(), buf, sizeof buf, ini.c_str());
    p.name = buf;
    p.model = static_cast<int>(GetPrivateProfileIntA("Lobby", "Model", p.model, ini.c_str()));
    if (p.model < 0 || p.model >= PuppetModelCount()) p.model = 0;
    GetPrivateProfileStringA("Lobby", "JoinHost", p.join_host.c_str(), buf, sizeof buf, ini.c_str());
    p.join_host = buf;
    const int jp = static_cast<int>(GetPrivateProfileIntA("Lobby", "JoinPort", p.join_port, ini.c_str()));
    const int hp = static_cast<int>(GetPrivateProfileIntA("Lobby", "HostPort", p.host_port, ini.c_str()));
    if (jp > 0 && jp < 65536) p.join_port = static_cast<uint16_t>(jp);
    if (hp > 0 && hp < 65536) p.host_port = static_cast<uint16_t>(hp);
    return p;
}

void SaveLobbyPrefs(const LobbyPrefs& p) {
    const std::string ini = lobby_ini_path();
    WritePrivateProfileStringA("Lobby", "Name", clean_name(p.name).c_str(), ini.c_str());
    WritePrivateProfileStringA("Lobby", "Model", std::to_string(p.model).c_str(), ini.c_str());
    WritePrivateProfileStringA("Lobby", "JoinHost", p.join_host.c_str(), ini.c_str());
    WritePrivateProfileStringA("Lobby", "JoinPort", std::to_string(p.join_port).c_str(), ini.c_str());
    WritePrivateProfileStringA("Lobby", "HostPort", std::to_string(p.host_port).c_str(), ini.c_str());
}

LobbyView Lobby() {
    LobbyView v;
    v.state = g_lobby_state;
    v.message = g_lobby_msg;
    v.tailnetIp = g_addrs.tailnet;
    v.otherIps = g_addrs.others;
    v.map = events::CurrentMap();
    v.atMenu = v.map == "MENUMAP";
    // The session can drop without an event this module turned into a status (host stopped from the console).
    if (v.state == LobbyState::Hosting && !g_session.IsHost()) v.state = LobbyState::Offline;
    if (v.state == LobbyState::Connected && !g_session.IsConnected()) v.state = LobbyState::Offline;
    if (v.state == LobbyState::Offline && g_session.IsHost()) v.state = LobbyState::Hosting;
    if ((v.state == LobbyState::Offline || v.state == LobbyState::Connecting) && g_session.IsConnected())
        v.state = LobbyState::Connected;
    v.port = g_session.IsHost() ? g_session.BoundPort() : g_join_port;
    if (!g_session.IsHost() && !g_session.IsConnected()) return v;
    std::map<uint8_t, uint32_t> rtt;
    for (const auto& p : g_session.Peers()) rtt[p.playerId] = p.rttMs;
    LobbyPlayer me;
    me.id = g_session.IsHost() ? 0 : g_session.LocalPlayerId();
    me.name = g_settings.player_name;
    me.model = g_settings.model;
    me.you = true;
    v.players.push_back(me);
    for (const auto& [id, r] : g_remotes) {
        LobbyPlayer lp;
        lp.id = id;
        lp.name = r.name;
        lp.model = r.model;
        const auto it = rtt.find(id);
        lp.rttMs = it != rtt.end() ? it->second : (rtt.empty() ? 0 : rtt.begin()->second);
        v.players.push_back(lp);
    }
    std::sort(v.players.begin(), v.players.end(), [](const LobbyPlayer& a, const LobbyPlayer& b) { return a.id < b.id; });
    return v;
}

void LobbyHost(const std::string& name, int model, uint16_t port) {
    apply_identity(name, model);
    g_settings.port = port;
    spdlog::info("coop: lobby host as '{}' model {} port {}", g_settings.player_name, g_settings.model, port);
    RequestHost(port);
}

void LobbyJoin(const std::string& name, int model, const std::string& host, uint16_t port) {
    apply_identity(name, model);
    spdlog::info("coop: lobby join {}:{} as '{}' model {}", host, port, g_settings.player_name, g_settings.model);
    set_lobby(LobbyState::Connecting, fmt::format("Connecting to {}:{} ...", host, port));
    RequestJoin(host, port);
}

void LobbyLeave() {
    g_settings.auto_role.clear();
    g_auto_stopped = true;
    RequestLeave();
}

// ---- host level recovery ----

std::string NextStoryMap(const std::string& map) {
    static const char* const kStory[] = {"MAP1",  "MAP2A", "MAP2B", "MAP2C",  "MAP3A",  "MAP3B",  "MAP3C",
                                         "MAP4A", "MAP4B", "MAP5",  "MAP6",   "MAP7",   "MAP8",   "MAP9",
                                         "MAP10", "MAP11A", "MAP11B", "MAP11C", "MAP12A", "MAP12B", "MAP12C"};
    constexpr size_t n = sizeof kStory / sizeof kStory[0];
    for (size_t i = 0; i + 1 < n; ++i) {
        if (map == kStory[i]) return kStory[i + 1];
    }
    return {};
}

namespace {

// First call arms and says how to confirm; a second call of the same kind within 3 s returns true.
bool confirm_level_jump(int which, const std::string& what) {
    const uint32_t now = now_ms();
    if (g_jump.armed == which && static_cast<int32_t>(g_jump.armed_until - now) > 0) {
        g_jump.armed = 0;
        return true;
    }
    g_jump.armed = which;
    g_jump.armed_until = now + 3000;
    show_notice(fmt::format("Press {} again (or click again) to {}.", which == 1 ? "F5" : "F6", what), 3000);
    return false;
}

// A client cannot change the level; the host (or a solo player) can.
bool may_change_level() {
    if (!g_session.IsHost() && g_session.IsConnected()) {
        show_notice("Only the host can restart the level (F5) or skip to the next one (F6).", 4000);
        return false;
    }
    return true;
}

void tell_everyone(const std::string& text) {
    show_notice(text, 6000);
    if (g_session.IsHost())
        events::Send(events::kHostNotice, 0, 0, 0.f, text.substr(0, net::kMaxGameEventKeyChars), 0);
}

}  // namespace

bool HostRestartLevel() {
    if (!may_change_level()) return false;
    const std::string map = events::CurrentMap();
    if (map.empty() || map == "MENUMAP") return false;
    if (!confirm_level_jump(1, fmt::format("restart {} for everyone", map))) return false;
    tell_everyone(fmt::format("Host restarts {}", map));
    g_jump.restart_map = map;
    g_jump.restart_at = 0;
    const int r = console::Exec("menumap menumap");
    spdlog::info("coop: host restarts '{}' through the main menu -> {}", map, r);
    return true;
}

bool HostNextLevel() {
    if (!may_change_level()) return false;
    const std::string map = events::CurrentMap();
    if (map.empty() || map == "MENUMAP") return false;
    const std::string next = NextStoryMap(map);
    if (next.empty()) {
        show_notice(fmt::format("{} has no next story level (last level, hunt or bonus map). F5 restarts it.", map),
                    5000);
        return false;
    }
    if (!confirm_level_jump(2, fmt::format("skip to {} for everyone", next))) return false;
    tell_everyone(fmt::format("Host skips to {}", next));
    const std::string cmd = "mod_spmap " + next;
    spdlog::info("coop: host skips '{}' -> '{}': {} -> {}", map, next, cmd, console::Exec(cmd.c_str()));
    return true;
}

bool BroadcastVehicleStates(const net::VehicleStates& states) { return g_session.SendVehicleStates(states); }

bool BroadcastAnimalStates(const net::AnimalStates& states) {
    net::AnimalStates stamped = states;
    stamped.mapSerial = events::LocalMapSerial();
    return g_session.SendAnimalStates(stamped);
}

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

int ConnectedPlayers() {
    if (!NetIsOnline()) return 1;
    int n = 1;
    for (const auto& [id, r] : g_remotes) {
        if (id != g_session.LocalPlayerId()) ++n;
    }
    return n;
}

std::string StatusLine() {
    const char* role = g_session.IsHost() ? "host" : (g_session.IsConnected() ? "client" : "offline");
    const auto& lp = LocalPlayer();
    return fmt::format("role={} id={} peers={} remotes={} hunter={} animals={}", role,
                       static_cast<int>(g_session.LocalPlayerId()), g_session.Peers().size(), g_remotes.size(),
                       lp.valid ? "yes" : "no", AnimalCount());
}

}  // namespace dh2::coop
