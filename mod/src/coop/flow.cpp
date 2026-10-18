// Mission flow for every player (re/notes/flow.md). Every player leaves a level together to the same next map, fails
// together, retries together, and reaches the end of the game together:
// - Level exits (cObjTriggerLevel) are the host's. A client that reaches an armed exit asks the host (kLevelExit),
//   the host runs it for its own hunter, and its checkpoint message reaches the clients as kMissionEnd code 7.
// - Mission messages that do not pass cMission's 0x91c20 (a cObjGuide's death, a level exit, the task status report
//   0x96750) are caught where the HUD shows them (0xd8db0): the host commits them, a client shows only the host's.
// - The host's results/pause menu choice drives the clients (kLevelNext). A client that continues runs its own
//   mod_newlevel with its profile set to the host's next map, so it plays the same chapter movies and loads the same
//   map; a retry or the main menu is followed through kMapStart. A client's own menu choice waits for the host's.
// - Ammo-gated tasks (NEED_AMMO_FOR_COMPLETE, 0x2f530 reads the local player only) fail only when no living player
//   has ammo; clients report theirs (kAmmoState).
// - A client revived with F9 gets its mission status back (its death set it to failed, 0x965e0).
// - On every story map load the profile's mission index is set to that map, as the menus do before spmap, so a level
//   exit moves the profile to the map after this one (a -spmap test start leaves it at MAP1).
//
// GameDll7 v1.0 (build 434) RVAs; every hooked body starts `55 8b ec 6a ff` and is checked before hooking.
#include "coop/flow.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include "coop/coop.h"
#include "coop/events.h"
#include "coop/player_tracker.h"
#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/console.h"
#include "engine/console_cmds.h"
#include "net/protocol.h"

namespace dh2::coop::flow {
namespace {

// Level exits (re/notes/audit-maps-a.md F6). Both read the trigger's target "map3b,human2", drop the map token (the
// next map is the profile's), store the spawn point in rules +0xbc, SetMapIsComplete on the activator's profile,
// write HUD message code 7 straight into the server stream, then 0x50760(activator, 2.0f).
constexpr uintptr_t kRvaLevelActivate = 0x6c420;      // walk-in, __thiscall(trigger, activator)
constexpr uintptr_t kRvaLevelFastActivate = 0x6c610;  // ActivateTriggerLevel's immediate path, same arguments
constexpr uintptr_t kTrigLevelCooldown = 0x1e4;       // float, -5 after an exit ran
constexpr uintptr_t kTrigLevelArmed = 0x1e8;          // (Latent)ActivateTriggerLevel set it
constexpr uintptr_t kEntId = 0x04;
constexpr uintptr_t kEntTargetName = 0x198;           // char*: "exit_1"
constexpr uintptr_t kLevelTarget = 0x194;             // char*: "map3b,human2"
// HUD mission message: SvParseMessage (0x3140) hands every stream message 0x87 to this. Its update (0xc9ca0) then
// opens "succesmenu" (5), "failmenu" (6) or "gameAutosave" (7) once the fade is done.
constexpr uintptr_t kRvaHudMissionShow = 0xd8db0;     // __thiscall(hud msg, const char* text, float secs, int code)
constexpr uintptr_t kHudMsgCode = 0xd0;
// NEED_AMMO_FOR_COMPLETE: task CanBeCompleted (0x97440 base, 0x979f0 cTaskShoot) fails the task (state 5) when this
// returns 0: GetAmmoCount on the local player's profile.
constexpr uintptr_t kRvaAmmoCheck = 0x2f530;          // __fastcall(obj) -> int
constexpr uintptr_t kRvaAmmoObj = 0x186314;           // its argument (a global)
// Console commands the menus queue.
constexpr uintptr_t kRvaNewLevel = 0xc0080;           // mod_newlevel: outro/intro movies, spmap GetMapToBeStarted
constexpr uintptr_t kRvaEndMovie = 0xbe480;           // mod_playendfmv: Movies\omap12c (MAP12C success, continue)
// Game rules and profile.
constexpr uintptr_t kRvaSetMissionMap = 0xa800;       // __thiscall(rules, const char* map): profile mission index
constexpr uintptr_t kRvaRules = 0x1657f0;             // cMODApp*
constexpr uintptr_t kRulesSpawn = 0xbc;               // char[]: spawn point of the next map (an exit's 2nd token)
constexpr size_t kSpawnMax = 24;
constexpr uintptr_t kRvaProfile0 = 0x165888;          // cHunterProfile[8] (stride 0x4070); the game uses [0]
constexpr uintptr_t kProfileMapIndex = 0x23d8;        // uint8: mission table index, SetMapIsComplete adds 1 (story)
constexpr uintptr_t kRvaHuntMode = 0x186c98;          // int: 0 story, else a hunt/bonus mission (SetMapIsComplete's branch)
constexpr uintptr_t kRvaFurthest = 0x186eb0;          // int: furthest mission index reached (progress)
constexpr uintptr_t kRvaMission = 0x1869e8;           // cMission*
constexpr uintptr_t kMissionStatus = 0x254;           // 1 running, 2 success, 3 failed
constexpr uintptr_t kMissionHunter = 0x258;
constexpr uintptr_t kHunterDead = 0x6cc;              // cObjHunter::OnDie sets it; F9 clears it
constexpr uint32_t kAmmoFreshMs = 75000;  // clients repeat their report every 30 s

// GUI commands (GAMEMENU.SCR) that put the stats page back on a results menu.
constexpr int kCmSuccessPage = 13134;
constexpr int kCmFailPage = 13137;
using PostGuiFn = void(__cdecl*)(int cmd, int p1, int p2);  // CommonDll7 cGuiEvent::PostMessageA (static)
using SetMissionMapFn = void(__thiscall*)(void* rules, const char* map);
using GetMapFn = char*(__thiscall*)(void* profile);

enum Choice : uint32_t { kNone = 0, kContinue = 1, kEndMovie = 2, kRetry = 3, kMainMenu = 4, kOtherMap = 5 };

const char* ChoiceName(uint32_t c) {
    switch (c) {
        case kContinue: return "continue";
        case kEndMovie: return "end movie";
        case kRetry: return "retry";
        case kMainMenu: return "main menu";
        case kOtherMap: return "another map";
        default: return "?";
    }
}

// Script natives of the game menus (cModGameMenu exports, registered in 0xc2d40). Each closes the menus and queues
// the console command that loads the next thing.
struct Native {
    const char* name;
    uintptr_t rva;
    Choice choice;
    bool takes_name;  // GameLoadMission / GameLoadBonus take the mission name
    safetyhook::InlineHook* hook;
};
// Order matters: Detour() and coop_results index it.
Native g_natives[] = {
    {"GameLoadLevel", 0xbea20, kContinue, false, nullptr},  // mod_newlevel (success menu continue, autosave menu)
    {"GameLoadHunt", 0xbeb60, kOtherMap, false, nullptr},   // mod_newhunt
    {"GameLoadMission", 0xbeca0, kOtherMap, true, nullptr}, // mod_gameloadmission <name>
    {"GameLoadBonus", 0xbedf0, kOtherMap, true, nullptr},   // mod_gameloadbonus <name>
    {"RestartMission", 0xbe6a0, kRetry, false, nullptr},    // mod_restartmission (success menu replay)
    {"Restart", 0xc0770, kRetry, false, nullptr},           // sv_restart (fail menu, pause menu)
    {"Quit", 0xbef40, kMainMenu, false, nullptr},           // closegamemenu, sv_stop
};

HMODULE g_game = nullptr;
safetyhook::InlineHook* g_show_hook = nullptr;
safetyhook::InlineHook* g_level_hook = nullptr;
safetyhook::InlineHook* g_level_fast_hook = nullptr;
safetyhook::InlineHook* g_ammo_hook = nullptr;
safetyhook::InlineHook* g_newlevel_hook = nullptr;
safetyhook::InlineHook* g_endmovie_hook = nullptr;
SetMissionMapFn g_set_mission_map = nullptr;
GetMapFn g_get_map = nullptr;
PostGuiFn g_post_gui = nullptr;

// [Flow] ini keys.
bool g_exit_asks_host = true;   // ClientExitAsksHost
bool g_ammo_any = true;         // AmmoAnyPlayer
bool g_sync_profile = true;     // SyncProfileMap
bool g_follow_menu = true;      // FollowHostMenu
bool g_retry_menu = true;       // RetryViaMenu: Restart / replay go through the main menu, not the game's in-place restart

// Client state, reset on every map load.
uint32_t g_host_choice = kNone;  // the host left its results/pause menu with this (kLevelNext)
bool g_waiting = false;          // this client chose first: nothing ran here, it waits for the host
int g_show_allow[8] = {};        // HUD mission messages the host sent (ApplyMissionEnd), by code, not yet shown
int g_end_code = 0;              // the mission message this map ended with here (5, 6, 7), 0 while playing
void* g_hud_msg = nullptr;       // the HUD mission message element (hk_Show's this)
uint32_t g_hint_until = 0;
std::string g_hint;
std::unordered_map<int16_t, uint32_t> g_exit_asked;  // trigger id -> GetTickCount of the last kLevelExit request
int g_dead_prev = -1;            // local hunter +0x6cc last frame
// Client: following the host's continue / end movie. Survives the map load it causes.
bool g_follow_newlevel = false;  // the next mod_newlevel here is the host's
bool g_follow_endmovie = false;  // the next mod_playendfmv here is the host's
std::string g_follow_map;        // map that mod_newlevel loads (upper case)
uint32_t g_follow_ms = 0;
// Ammo. Client: what it last reported. Host: per player.
int g_ammo_sent = -1;
uint32_t g_ammo_sent_ms = 0;
uint32_t g_ammo_poll_ms = 0;
int g_ammo_fake = -1;            // coop_ammofake: -1 real, 0 dry, 1 has ammo (this process's own check)
struct AmmoReport {
    bool has = true;
    uint32_t ms = 0;
};
std::unordered_map<uint8_t, AmmoReport> g_ammo;
int g_ammo_override_logged = -1;  // host: last logged "a client still has ammo" decision
// Host: one level exit per map.
bool g_exit_done = false;
bool g_in_exit = false;
int g_in_native = 0;  // >0 while a host/test call runs a menu native's original
bool g_cmds = false;

void* Rva(uintptr_t rva) { return g_game ? reinterpret_cast<char*>(g_game) + rva : nullptr; }

void* CheckedRva(uintptr_t rva, const char* label) {
    static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
    void* p = Rva(rva);
    uint8_t got[5] = {};
    if (!p || !core::safe_copy(p, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
        spdlog::error("flow: {} at GameDll7+{:#x} has unexpected bytes; not hooked (wrong game build?)", label, rva);
        return nullptr;
    }
    return p;
}

bool IsClient() { return NetIsOnline() && !NetIsHost(); }
bool IsOnlineHost() { return NetIsOnline() && NetIsHost(); }

void* Rules() {
    void* rules = nullptr;
    return g_game && core::safe_read(Rva(kRvaRules), rules) ? rules : nullptr;
}

void* Mission() {
    void* m = nullptr;
    return g_game && core::safe_read(Rva(kRvaMission), m) ? m : nullptr;
}

std::string Lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string Upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

bool PlainName(const std::string& s, size_t max) {
    return !s.empty() && s.size() <= max &&
           std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isalnum(c) || c == '_'; });
}

bool IsStoryMap(const std::string& upper) { return upper == "MAP12C" || !NextStoryMap(upper).empty(); }

void Hint(const std::string& text) {
    g_hint = text;
    g_hint_until = GetTickCount() + 6000;
}

int16_t IdOf(void* ent) {
    int16_t id = -1;
    core::safe_read(static_cast<char*>(ent) + kEntId, id);
    return id;
}

std::string StrAt(void* ent, uintptr_t off) {
    char* s = nullptr;
    core::safe_read(static_cast<char*>(ent) + off, s);
    return s ? core::safe_cstr(s, 48) : std::string();
}

// ---- profile mission map ----

const char* guarded_get_map(void* profile) {
    __try {
        return g_get_map(profile);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

std::string ProfileMap() {
    if (!g_get_map || !g_game) return {};
    const char* m = guarded_get_map(Rva(kRvaProfile0));
    return m ? core::safe_cstr(m, 32) : std::string();
}

bool guarded_set_mission(void* rules, const char* map) {
    __try {
        g_set_mission_map(rules, map);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// As the menus do before every spmap (mod_spmap 0x36b0, mod_newlevel 0xc0080): rules +0x3c = map, and the profile's
// mission index points at that map in the mission table (names are lower case there).
std::string ProfileState();

bool SetProfileMap(const std::string& map_lower, const char* why) {
    void* rules = Rules();
    if (!rules || !g_set_mission_map) return false;
    const std::string before = ProfileMap();
    if (before == map_lower) return true;
    const bool ok = guarded_set_mission(rules, map_lower.c_str());
    spdlog::info("flow: profile mission map '{}' -> '{}' ({}) ok={} {}", before, ProfileMap(), why, ok, ProfileState());
    return ok;
}

void WriteSpawn(const std::string& spawn) {
    void* rules = Rules();
    if (!rules || spawn.empty() || spawn.size() >= kSpawnMax) return;
    __try {
        std::memcpy(static_cast<char*>(rules) + kRulesSpawn, spawn.c_str(), spawn.size() + 1);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

// "index=3 hunt_mode=0 furthest=3": where SetMapIsComplete will move the profile (log evidence for a wrong next map).
std::string ProfileState() {
    uint8_t index = 255;
    int hunt_mode = -1, furthest = -1;
    if (g_game) {
        core::safe_read(Rva(kRvaProfile0 + kProfileMapIndex), index);
        core::safe_read(Rva(kRvaHuntMode), hunt_mode);
        core::safe_read(Rva(kRvaFurthest), furthest);
    }
    return "index=" + std::to_string(index) + " hunt_mode=" + std::to_string(hunt_mode) +
           " furthest=" + std::to_string(furthest);
}

std::string ReadSpawn() {
    void* rules = Rules();
    if (!rules) return {};
    const std::string s = core::safe_cstr(static_cast<char*>(rules) + kRulesSpawn, kSpawnMax);
    return PlainName(s, kSpawnMax - 1) ? s : std::string();
}

void ClearHudMenuCode() {
    if (!g_hud_msg) return;
    __try {
        *reinterpret_cast<int*>(static_cast<char*>(g_hud_msg) + kHudMsgCode) = 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

// ---- the menu natives: the host's choice drives everyone ----

bool guarded_native(safetyhook::InlineHook* h, bool takes_name, const char* arg) {
    if (!h) return false;
    __try {
        if (takes_name) h->unsafe_ccall<void>(arg);
        else h->unsafe_ccall<void>();
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void OnNative(int i, const char* arg) {
    Native& n = g_natives[i];
    // Retry on the host (or offline): the game restarts the loaded map in place, and with the same-map cleanup
    // (lifecycle.cpp) the still open menu then faults in cGuiManager::Update (MAP2B, 2026-10-18). Close the menu and
    // take the main-menu hop F5 uses; the clients follow its two kMapStart commits.
    if (n.choice == kRetry && g_retry_menu && !g_in_native && (!NetIsOnline() || NetIsHost())) {
        const std::string cur = events::CurrentMap();
        if (!cur.empty() && cur != "MENUMAP") {
            if (NetIsOnline()) events::Send(events::kLevelNext, 0, n.choice, 0.f, cur, 0);
            spdlog::info("flow: {} chose retry ({}): restarting '{}' through the main menu", NetIsOnline() ? "host" : "player",
                         n.name, cur);
            ClearHudMenuCode();
            console::Exec("closegamemenu");
            if (HostRestartNow()) return;
        }
    }
    if (!NetIsOnline() || !g_follow_menu || g_in_native) {
        guarded_native(n.hook, n.takes_name, arg);
        return;
    }
    const std::string a = arg ? core::safe_cstr(arg, 32) : std::string();
    if (NetIsHost()) {
        // A continue announces itself from mod_newlevel (it knows the next map). The others: tell the clients now;
        // the map change itself reaches them as kMapStart.
        if (n.choice != kContinue) {
            events::Send(events::kLevelNext, 0, n.choice, 0.f, events::CurrentMap(), 0);
        }
        spdlog::info("flow: host chose {} ({}{}{})", ChoiceName(n.choice), n.name, a.empty() ? "" : " ", a);
        ++g_in_native;
        if (!guarded_native(n.hook, n.takes_name, arg)) spdlog::error("flow: {} faulted on the host", n.name);
        --g_in_native;
        return;
    }
    // Client: the host decides. Its choice arrives as kLevelNext and kMapStart.
    if (g_host_choice != kNone || !g_follow_map.empty()) {
        spdlog::info("flow: client {} ignored: already following the host's {}", n.name, ChoiceName(g_host_choice));
        return;
    }
    spdlog::info("flow: client chose {} ({}): not run here, waiting for the host's choice", ChoiceName(n.choice),
                 n.name);
    g_waiting = true;
    if (n.choice != kContinue) Hint("Only the host can restart the mission or leave it");
    if (g_end_code == 0) {
        // A pause menu (RESTART, QUIT): back to the game.
        console::Exec("closegamemenu");
        return;
    }
    // A results screen. Closing it would not help: the HUD opens it again on its next update. The script took its
    // page down before calling us, so put the stats page back and keep waiting.
    if (g_post_gui && (g_end_code == 5 || g_end_code == 6)) g_post_gui(g_end_code == 5 ? kCmSuccessPage : kCmFailPage, 0, 0);
}

template <int I>
void __cdecl hk_native() {
    OnNative(I, nullptr);
}
template <int I>
void __cdecl hk_native_name(const char* name) {
    OnNative(I, name);
}

void* Detour(int i) {
    switch (i) {
        case 0: return reinterpret_cast<void*>(&hk_native<0>);
        case 1: return reinterpret_cast<void*>(&hk_native<1>);
        case 2: return reinterpret_cast<void*>(&hk_native_name<2>);
        case 3: return reinterpret_cast<void*>(&hk_native_name<3>);
        case 4: return reinterpret_cast<void*>(&hk_native<4>);
        case 5: return reinterpret_cast<void*>(&hk_native<5>);
        case 6: return reinterpret_cast<void*>(&hk_native<6>);
        default: return nullptr;
    }
}

bool guarded_void(safetyhook::InlineHook* h) {
    __try {
        h->unsafe_ccall<void>();
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// mod_newlevel: the host's continue (results menu, autosave menu after an exit). The profile already points at the
// next map (SetMapIsComplete); the clients do the same with that map, so their movies and map are the host's.
void __cdecl hk_NewLevel() {
    if (!NetIsOnline() || !g_follow_menu) {
        guarded_void(g_newlevel_hook);
        return;
    }
    if (NetIsHost()) {
        const std::string next = ProfileMap();
        const std::string spawn = ReadSpawn();
        std::string key = next;
        if (!spawn.empty() && key.size() + 1 + spawn.size() <= net::kMaxGameEventKeyChars) key += "|" + spawn;
        events::Send(events::kLevelNext, 0, kContinue, 0.f, key, 0);
        spdlog::info("flow: host continues to '{}' (spawn '{}', {}); clients follow", next, spawn, ProfileState());
        guarded_void(g_newlevel_hook);
        return;
    }
    if (g_follow_newlevel) {
        g_follow_newlevel = false;
        spdlog::info("flow: client mod_newlevel with the host: profile map '{}', spawn '{}'", ProfileMap(), ReadSpawn());
        guarded_void(g_newlevel_hook);
        return;
    }
    spdlog::info("flow: client's own mod_newlevel dropped (profile map '{}'): the host continues for everyone",
                 ProfileMap());
}

// mod_playendfmv: the success menu queues it after MAP12C (then Quit to the main menu).
void __cdecl hk_EndMovie() {
    if (!NetIsOnline() || !g_follow_menu) {
        guarded_void(g_endmovie_hook);
        return;
    }
    if (NetIsHost()) {
        events::Send(events::kLevelNext, 0, kEndMovie, 0.f, events::CurrentMap(), 0);
        spdlog::info("flow: host plays the end movie; clients play it too");
        guarded_void(g_endmovie_hook);
        return;
    }
    if (g_follow_endmovie) {
        g_follow_endmovie = false;
        spdlog::info("flow: client plays the end movie with the host");
        guarded_void(g_endmovie_hook);
        spdlog::info("flow: client end movie done");
        return;
    }
    spdlog::info("flow: client's own end movie dropped: it plays when the host's does");
}

void OnLevelNext(const net::GameEvent& ev) {
    if (!events::SameMapAsHost()) {
        spdlog::info("flow: host chose {} for another map; ignored here", ChoiceName(ev.i0));
        return;
    }
    g_host_choice = ev.i0;
    g_waiting = false;
    spdlog::info("flow: host chose {} ('{}')", ChoiceName(ev.i0), ev.key);
    if (ev.i0 == kEndMovie) {
        g_follow_endmovie = true;
        console::Exec("mod_playendfmv");  // queued: the movie plays from the console, outside Present
        return;
    }
    if (ev.i0 != kContinue) return;  // retry, main menu, another map: kMapStart follows
    const size_t bar = ev.key.find('|');
    const std::string map = Lower(ev.key.substr(0, bar));
    const std::string spawn = bar == std::string::npos ? std::string() : ev.key.substr(bar + 1);
    if (!PlainName(map, 24) || (!spawn.empty() && !PlainName(spawn, kSpawnMax - 1))) {
        spdlog::warn("flow: host's next map '{}' rejected", ev.key);
        return;
    }
    // As the host's menu did: the profile points at the next map (the client's SetMapIsComplete normally did that
    // already), the exit's spawn point, then the game's own continue (GameLoadLevel 0xbea20 queues these three).
    SetProfileMap(map, "host continues");
    WriteSpawn(spawn);
    ClearHudMenuCode();
    g_follow_newlevel = true;
    g_follow_map = Upper(map);
    g_follow_ms = GetTickCount();
    console::Exec("closemainmenu");
    console::Exec("closegamemenu");
    const int r = console::Exec("mod_newlevel");
    spdlog::info("flow: following the host's continue to '{}' (spawn '{}') -> {}", map, spawn, r);
}

// Test: coop_results continue|hunt|replay|restart|quit runs that menu native as the GUI would.
void __cdecl cmd_results(const char* arg) {
    const std::string a = core::safe_cstr(arg, 32);
    int i = -1;
    if (a == "continue") i = 0;
    else if (a == "hunt") i = 1;
    else if (a == "replay") i = 4;
    else if (a == "restart") i = 5;
    else if (a == "quit") i = 6;
    if (i < 0 || !g_natives[i].hook) {
        spdlog::warn("flow: usage coop_results continue|hunt|replay|restart|quit");
        return;
    }
    spdlog::info("flow: coop_results {} -> {}", a, g_natives[i].name);
    OnNative(i, nullptr);
}

// ---- mission messages (HUD) ----

void __fastcall hk_Show(void* self, void*, const char* text, float secs, int code) {
    g_hud_msg = self;
    const bool mission = code == 5 || code == 6 || code == 7;
    if (!mission || !NetIsOnline()) {
        if (mission) g_end_code = code;
        g_show_hook->unsafe_thiscall<void>(self, text, secs, code);
        return;
    }
    if (NetIsHost()) {
        g_end_code = code;
        g_show_hook->unsafe_thiscall<void>(self, text, secs, code);
        events::CommitHudMissionMessage(code, text);
        return;
    }
    if (g_show_allow[code] > 0) {
        --g_show_allow[code];
        g_end_code = code;
        g_show_hook->unsafe_thiscall<void>(self, text, secs, code);
        return;
    }
    spdlog::info("flow: client's own mission message code={} '{}' dropped: the host decides", code,
                 core::safe_cstr(text, 64));
}

// ---- level exits (trigger_level): the host's ----

bool guarded_level(safetyhook::InlineHook* h, void* trigger, void* activator) {
    __try {
        h->unsafe_thiscall<void>(trigger, activator);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

float Cooldown(void* trigger) {
    float v = 0.f;
    core::safe_read(static_cast<char*>(trigger) + kTrigLevelCooldown, v);
    return v;
}

// Runs the exit here and says whether it went through (it resets its cooldown to -5 when it does).
bool RunExit(bool fast, void* self, void* activator) {
    safetyhook::InlineHook* h = fast ? g_level_fast_hook : g_level_hook;
    const float before = Cooldown(self);
    const bool ok = guarded_level(h, self, activator);
    const float after = Cooldown(self);
    return ok && after < 0.f && after != before;
}

void OnLevel(bool fast, void* self, void* activator) {
    if (!NetIsOnline() || g_in_exit) {
        guarded_level(fast ? g_level_fast_hook : g_level_hook, self, activator);
        return;
    }
    if (NetIsHost()) {
        if (RunExit(fast, self, activator)) {
            g_exit_done = true;
            spdlog::info("flow: host level exit '{}' (id {}, target '{}') ran; next map '{}', spawn '{}'",
                         StrAt(self, kEntTargetName), IdOf(self), StrAt(self, kLevelTarget), ProfileMap(), ReadSpawn());
        }
        return;
    }
    // This client's player reached an exit (walked in, a replayed host fire ran ActivateTriggerLevel, or a task end
    // hit it with the mission's hunter; on the pilot maps that player is not a cObjHunter). Its own checkpoint would
    // load its profile's next map for this player alone: the host runs the exit for everyone.
    const int16_t id = IdOf(self);
    const uint32_t now = GetTickCount();
    auto it = g_exit_asked.find(id);
    if (id < 0 || (it != g_exit_asked.end() && now - it->second < 5000)) return;
    g_exit_asked[id] = now;
    const std::string name = StrAt(self, kEntTargetName);
    if (g_exit_asks_host) events::Request(events::kLevelExit, static_cast<uint16_t>(id), fast ? 1u : 0u, 0.f, name);
    spdlog::info("flow: client's own level exit '{}' (id {}, target '{}') not run{}", name, id,
                 StrAt(self, kLevelTarget), g_exit_asks_host ? "; asked the host" : "; the host's exit decides");
    if (!fast) {
        Hint(g_exit_asks_host ? "Exit reached: the host takes everyone to the next map"
                              : "Exit reached: the map changes when the host gets there");
    }
}

void __fastcall hk_LevelActivate(void* self, void*, void* activator) { OnLevel(false, self, activator); }
void __fastcall hk_LevelFastActivate(void* self, void*, void* activator) { OnLevel(true, self, activator); }

void OnLevelExitRequest(const net::GameEvent& ev) {
    if (g_exit_done) {
        spdlog::info("flow: level exit request '{}' (id {}) from player {}: this map's exit already ran", ev.key,
                     ev.subjectId, ev.actorId);
        return;
    }
    void* ent = EntityById(ev.subjectId);
    // The mission's hunter (cMission +0x258) is the player the game's own task-end exits use; on the pilot maps it is
    // the cObjPilot the tracker may not know.
    void* hunter = nullptr;
    if (void* mission = Mission()) core::safe_read(static_cast<char*>(mission) + kMissionHunter, hunter);
    if (!hunter) hunter = LocalPlayer().entity;
    if (!ent || StrAt(ent, kEntTargetName) != ev.key || !hunter) {
        spdlog::warn("flow: level exit request id={} '{}' from player {}: no such exit here (or no hunter)",
                     ev.subjectId, ev.key, ev.actorId);
        return;
    }
    int armed = 0;
    core::safe_read(static_cast<char*>(ent) + kTrigLevelArmed, armed);
    if (armed == 0) {
        spdlog::warn("flow: level exit request '{}' (id {}) from player {} rejected: not armed on the host", ev.key,
                     ev.subjectId, ev.actorId);
        return;
    }
    g_in_exit = true;
    const bool ran = RunExit(ev.i0 != 0, ent, hunter);
    g_in_exit = false;
    if (ran) g_exit_done = true;
    spdlog::info("flow: level exit '{}' (id {}, target '{}') reached by player {}: ran for the host's hunter: {}; "
                 "next map '{}', spawn '{}'",
                 ev.key, ev.subjectId, StrAt(ent, kLevelTarget), ev.actorId, ran, ProfileMap(), ReadSpawn());
}

// ---- ammo-gated tasks ----

void* AmmoObj() {
    void* obj = nullptr;
    return g_game && core::safe_read(Rva(kRvaAmmoObj), obj) ? obj : nullptr;
}

int guarded_ammo(void* obj) {
    __try {
        return g_ammo_hook->unsafe_fastcall<int>(obj);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 1;
    }
}

// This process's own answer (coop_ammofake overrides it for tests).
int OwnAmmo(void* obj) {
    if (g_ammo_fake >= 0) return g_ammo_fake;
    return guarded_ammo(obj);
}

bool ClientHasAmmo(uint8_t* who) {
    const uint32_t now = GetTickCount();
    const SessionView view = Snapshot();
    for (int i = 0; i < view.remoteCount; ++i) {
        const RemoteView& r = view.remotes[i];
        if (r.id == 0 || (r.flags & net::kFlagDead) != 0) continue;
        const auto it = g_ammo.find(r.id);
        // No report (yet): count as armed, as the client's own check would pass before it reports.
        const bool has = it == g_ammo.end() || now - it->second.ms > kAmmoFreshMs || it->second.has;
        if (has) {
            if (who) *who = r.id;
            return true;
        }
    }
    return false;
}

int __fastcall hk_AmmoCheck(void* obj) {
    if (!NetIsOnline() || !g_ammo_any) return OwnAmmo(obj);
    if (!NetIsHost()) return 1;  // the host's tasks decide; a dry client fails nothing here
    const int own = OwnAmmo(obj);
    if (own != 0) {
        g_ammo_override_logged = -1;
        return own;
    }
    uint8_t who = 0xFF;
    const bool other = ClientHasAmmo(&who);
    if ((other ? 1 : 0) != g_ammo_override_logged) {
        g_ammo_override_logged = other ? 1 : 0;
        if (other) spdlog::info("flow: host is out of ammo for the task; player {} still has ammo: the task goes on", who);
        else spdlog::info("flow: no living player has ammo for the task: it fails");
    }
    return other ? 1 : 0;
}

void OnAmmoState(const net::GameEvent& ev) {
    AmmoReport& r = g_ammo[ev.actorId];
    if (r.has != (ev.i0 != 0) || r.ms == 0) spdlog::info("flow: player {} ammo {}", ev.actorId, ev.i0 ? "yes" : "none");
    r.has = ev.i0 != 0;
    r.ms = GetTickCount();
}

void PumpClientAmmo() {
    // Not LocalPlayer(): the ammo-gated maps (MAP11A/11C) fly a cObjPilot; the check finds the player itself.
    if (!IsClient() || !g_ammo_any || !g_ammo_hook || !events::SameMapAsHost() || !Mission()) return;
    const uint32_t now = GetTickCount();
    if (now - g_ammo_poll_ms < 1000) return;
    g_ammo_poll_ms = now;
    void* obj = AmmoObj();
    const int has = obj ? (OwnAmmo(obj) != 0 ? 1 : 0) : 1;
    if (has != g_ammo_sent || now - g_ammo_sent_ms > 30000) {
        g_ammo_sent = has;
        g_ammo_sent_ms = now;
        events::Request(events::kAmmoState, 0, static_cast<uint32_t>(has), 0.f, "");
    }
}

// ---- a client's revival ----

// A client's death ran cObjHunter::OnDie (0x50350) -> 0x965e0: mission status 3 (its MISSION FAILED was swallowed).
// When F9 brings it back and the host's mission goes on, its mission runs again.
void PumpClientRevive() {
    if (!IsClient()) {
        g_dead_prev = -1;
        return;
    }
    const LocalPlayerSample& lp = LocalPlayer();
    if (!lp.valid || !lp.entity) return;
    int dead = 0;
    core::safe_read(static_cast<char*>(lp.entity) + kHunterDead, dead);
    const int was = g_dead_prev;
    g_dead_prev = dead;
    if (was != 1 || dead != 0 || g_end_code != 0) return;
    void* mission = Mission();
    int status = 0;
    if (!mission || !core::safe_read(static_cast<char*>(mission) + kMissionStatus, status) || status != 3) return;
    __try {
        *reinterpret_cast<int*>(static_cast<char*>(mission) + kMissionStatus) = 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    spdlog::info("flow: client revived; its mission status 3 -> 1 (the host's mission goes on)");
}

// ---- test commands ----

// coop_ammofake -1|0|1: this process's own ammo check answers real / none / some.
void __cdecl cmd_ammofake(const char* arg) {
    g_ammo_fake = std::clamp(std::atoi(core::safe_cstr(arg, 8).c_str()), -1, 1);
    spdlog::info("flow: coop_ammofake {} (own ammo check {})", g_ammo_fake,
                 g_ammo_fake < 0 ? "real" : (g_ammo_fake ? "has ammo" : "dry"));
}

// coop_die: this process's hunter dies as a killed hunter does: cObjHunter::OnDie (0x50350, thiscall(hunter, killer),
// no killer = "Mission failed / Your hunt has ended!"): +0x6cc = 1, mission status 3 (0x965e0). coop_down only sets
// health 0 and a body state, so it never reaches this. Test for a client's death and F9.
using HunterDieFn = void(__thiscall*)(void* hunter, void* killer);
void __cdecl cmd_die() {
    const LocalPlayerSample& lp = LocalPlayer();
    if (!lp.valid || !lp.entity || !g_game) {
        spdlog::warn("flow: coop_die: no local hunter");
        return;
    }
    bool ok = false;
    __try {
        reinterpret_cast<HunterDieFn>(Rva(0x50350))(lp.entity, nullptr);
        ok = true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    int dead = -1, status = -1;
    core::safe_read(static_cast<char*>(lp.entity) + kHunterDead, dead);
    if (void* mission = Mission()) core::safe_read(static_cast<char*>(mission) + kMissionStatus, status);
    spdlog::info("flow: coop_die ok={} hunter dead flag={} mission status={}", ok, dead, status);
}

// coop_profilemap <map>: the profile's mission index points at <map> (what SetMapIsComplete does after the level
// before it), so a continue plays that map's chapter movie and loads it. Tests a map whose end is hard to reach.
void __cdecl cmd_profilemap(const char* arg) {
    const std::string map = Lower(core::safe_cstr(arg, 24));
    if (!PlainName(map, 24)) {
        spdlog::warn("flow: usage coop_profilemap <map from the mission table, lower case>");
        return;
    }
    const bool ok = SetProfileMap(map, "coop_profilemap");
    spdlog::info("flow: coop_profilemap '{}' -> ok={} profile map '{}' {}", map, ok, ProfileMap(), ProfileState());
}

// coop_ammocheck: run the task's ammo check as the game does and log the answer.
void __cdecl cmd_ammocheck() {
    void* obj = AmmoObj();
    const int r = obj && g_ammo_hook ? hk_AmmoCheck(obj) : -1;
    spdlog::info("flow: coop_ammocheck obj={:#x} -> {}", reinterpret_cast<uintptr_t>(obj), r);
}

// coop_npcdie <id>: the NPC's profile health to 0 and its OnDie (vt +0x154, cObjNPC::OnDie 0x5fde0 for guides), as
// a lethal hit does. Host test for a guide's death.
using NpcDieFn = void(__fastcall*)(void* npc);
bool guarded_npc_die(void* npc) {
    __try {
        void* profile = *reinterpret_cast<void**>(static_cast<char*>(npc) + 0x1F8);
        if (profile) *reinterpret_cast<float*>(static_cast<char*>(profile) + 0x54) = 0.f;
        void** vt = *reinterpret_cast<void***>(npc);
        reinterpret_cast<NpcDieFn>(vt[0x154 / 4])(npc);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void __cdecl cmd_npcdie(const char* arg) {
    const int id = std::atoi(core::safe_cstr(arg, 8).c_str());
    void* ent = id > 0 ? EntityById(id) : nullptr;
    if (!ent) {
        spdlog::warn("flow: coop_npcdie {}: no entity", id);
        return;
    }
    spdlog::info("flow: coop_npcdie id={} '{}' ok={}", id, StrAt(ent, kEntTargetName), guarded_npc_die(ent));
}

void __cdecl cmd_flow() {
    void* mission = Mission();
    int status = -1;
    if (mission) core::safe_read(static_cast<char*>(mission) + kMissionStatus, status);
    if (g_game) {
        // The mission table (GameDll+0x1653b8, 28 entries of 0x14 bytes: byte 0, name at +1, movie type at +0xb).
        std::string table;
        for (int i = 0; i < 28; ++i) {
            const char* e = static_cast<const char*>(Rva(0x1653b8)) + i * 0x14;
            uint8_t first = 0, type = 0;
            core::safe_read(e, first);
            core::safe_read(e + 0xb, type);
            table += std::to_string(i) + ":" + core::safe_cstr(e + 1, 10) + "/" + std::to_string(type) + "/" +
                     std::to_string(first) + " ";
        }
        spdlog::info("flow: mission table {}", table);
    }
    spdlog::info("flow: state map='{}' profile map='{}' {} spawn='{}' mission status={} end_code={} host_choice={} "
                 "waiting={} following='{}' exit_done={} banner='{}'",
                 events::CurrentMap(), ProfileMap(), ProfileState(), ReadSpawn(), status, g_end_code,
                 ChoiceName(g_host_choice), g_waiting, g_follow_map, g_exit_done, Banner());
}

class FlowModule : public dh2::Module {
public:
    const char* Name() const override { return "flow"; }
    int Priority() const override { return 46; }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        g_exit_asks_host = GetPrivateProfileIntA("Flow", "ClientExitAsksHost", 1, ini.c_str()) != 0;
        g_ammo_any = GetPrivateProfileIntA("Flow", "AmmoAnyPlayer", 1, ini.c_str()) != 0;
        g_sync_profile = GetPrivateProfileIntA("Flow", "SyncProfileMap", 1, ini.c_str()) != 0;
        g_follow_menu = GetPrivateProfileIntA("Flow", "FollowHostMenu", 1, ini.c_str()) != 0;
        g_retry_menu = GetPrivateProfileIntA("Flow", "RetryViaMenu", 1, ini.c_str()) != 0;
        events::SetHandler(events::kLevelExit, &OnLevelExitRequest, nullptr);
        events::SetHandler(events::kAmmoState, &OnAmmoState, nullptr);
        events::SetHandler(events::kLevelNext, nullptr, &OnLevelNext);
        spdlog::info("flow: ClientExitAsksHost={} AmmoAnyPlayer={} SyncProfileMap={} FollowHostMenu={} RetryViaMenu={}",
                     g_exit_asks_host, g_ammo_any, g_sync_profile, g_follow_menu, g_retry_menu);
    }

    void OnEngineReady(HMODULE) override {
        if (HMODULE common = core::wait_for_module("CommonDll7.dll", 10000))
            g_post_gui = core::find_export_as<PostGuiFn>(common, "?PostMessageA@cGuiEvent@@SAXHHH@Z");
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_game = game;
        g_get_map = core::find_export_as<GetMapFn>(game, "?GetMapToBeStarted@cHunterProfile@@QAEPADXZ");
        g_set_mission_map = reinterpret_cast<SetMissionMapFn>(CheckedRva(kRvaSetMissionMap, "set mission map"));
        for (int i = 0; i < static_cast<int>(std::size(g_natives)); ++i) {
            Native& n = g_natives[i];
            const std::string label = std::string("menu native ") + n.name;
            n.hook = core::install_hook(label.c_str(), CheckedRva(n.rva, n.name), Detour(i));
        }
        g_newlevel_hook = core::install_hook("mod_newlevel (0xc0080)", CheckedRva(kRvaNewLevel, "mod_newlevel"),
                                             reinterpret_cast<void*>(&hk_NewLevel));
        g_endmovie_hook = core::install_hook("mod_playendfmv (0xbe480)", CheckedRva(kRvaEndMovie, "mod_playendfmv"),
                                             reinterpret_cast<void*>(&hk_EndMovie));
        g_show_hook = core::install_hook("HUD mission message (0xd8db0)",
                                         CheckedRva(kRvaHudMissionShow, "HUD mission message"),
                                         reinterpret_cast<void*>(&hk_Show));
        g_level_hook = core::install_hook("cObjTriggerLevel walk-in (0x6c420)",
                                          CheckedRva(kRvaLevelActivate, "trigger_level walk-in"),
                                          reinterpret_cast<void*>(&hk_LevelActivate));
        g_level_fast_hook = core::install_hook("cObjTriggerLevel activate (0x6c610)",
                                               CheckedRva(kRvaLevelFastActivate, "trigger_level activate"),
                                               reinterpret_cast<void*>(&hk_LevelFastActivate));
        g_ammo_hook = core::install_hook("task ammo check (0x2f530)", CheckedRva(kRvaAmmoCheck, "ammo check"),
                                         reinterpret_cast<void*>(&hk_AmmoCheck));
    }

    void OnMapLoaded(const char* bspName) override {
        g_host_choice = kNone;
        g_waiting = false;
        std::fill(std::begin(g_show_allow), std::end(g_show_allow), 0);
        g_end_code = 0;
        g_exit_asked.clear();
        g_exit_done = false;
        g_dead_prev = -1;
        g_ammo.clear();
        g_ammo_sent = -1;
        g_ammo_override_logged = -1;
        g_follow_newlevel = false;
        g_follow_endmovie = false;
        g_follow_map.clear();
        g_hint_until = 0;
        std::string map = bspName ? bspName : "";
        const size_t slash = map.find_last_of("\\/");
        if (slash != std::string::npos) map.erase(0, slash + 1);
        const size_t dot = map.rfind('.');
        if (dot != std::string::npos) map.resize(dot);
        map = Upper(map);
        if (g_sync_profile && IsStoryMap(map)) SetProfileMap(Lower(map), "map loaded");
    }

    void OnFrame(float) override {
        if (!g_cmds && console_cmds::Ready()) {
            g_cmds = true;
            console_cmds::AddCommandStr("coop_results", &cmd_results);
            console_cmds::AddCommandStr("coop_ammofake", &cmd_ammofake);
            console_cmds::AddCommand0("coop_ammocheck", &cmd_ammocheck);
            console_cmds::AddCommandStr("coop_npcdie", &cmd_npcdie);
            console_cmds::AddCommandStr("coop_profilemap", &cmd_profilemap);
            console_cmds::AddCommand0("coop_die", &cmd_die);
            console_cmds::AddCommand0("coop_flow", &cmd_flow);
        }
        if (!NetIsOnline()) {
            g_host_choice = kNone;
            g_waiting = false;
        }
        PumpClientAmmo();
        PumpClientRevive();
    }
};

DH2_MODULE(FlowModule)

}  // namespace

std::string Banner() {
    if (!IsClient()) return {};
    switch (g_host_choice) {
        case kContinue: return "The host continues: loading the next map";
        case kEndMovie: return "The host watches the ending";
        case kRetry: return "The host restarts the mission";
        case kMainMenu: return "The host went to the main menu";
        case kOtherMap: return "The host loads another map";
        default: break;
    }
    if (g_hint_until && static_cast<int32_t>(GetTickCount() - g_hint_until) < 0) return g_hint;
    if (g_waiting) return "Waiting for the host to choose";
    if (g_end_code != 0) return "The host chooses what comes next";
    return {};
}

bool AlreadyFollowing(const std::string& map) {
    return !g_follow_map.empty() && g_follow_map == map && GetTickCount() - g_follow_ms < 180000;
}

void BeforeFollow() {
    ClearHudMenuCode();
    console::Exec("closegamemenu");
}

void AllowMissionMessage(int code) {
    if (code >= 0 && code < static_cast<int>(std::size(g_show_allow))) ++g_show_allow[code];
}

}  // namespace dh2::coop::flow
