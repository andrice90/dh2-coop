// Discrete world events. Design: re/notes/p4c-events.md, engine map: re/notes/lead-mission.md.
// GameEvent is the only packet. Kinds are a registry so another map does not need a new message id.
#include "coop/events.h"

#include <windows.h>
#include <intrin.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include "coop/animals.h"
#include "coop/coop.h"
#include "coop/flow.h"
#include "coop/player_tracker.h"
#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/console.h"

namespace dh2::coop::story {
bool CatchupBusy();  // story.cpp: deferred fires, a cutscene camera, or a skip still pending
}

namespace dh2::coop::events {
namespace {

// A story map commits every trigger arm, fire, prop anim and PlaySA. MAP2A alone is past 60 by the bridge.
constexpr size_t kJournalCap = 1024;
// Catch-up: a client further than this from the host when it finishes is moved next to the host.
constexpr float kJoinTeleportMetres = 20.f;

// GameDll7 v1.0 (build 434). Every hooked body starts `55 8b ec 6a ff` (SEH frame); checked before hooking.
constexpr uintptr_t kRvaMission = 0x1869e8;         // cMission* singleton (scripts' GlobalsCheckMissionStatus reads it)
constexpr uintptr_t kRvaMissionMessage = 0x91c20;   // __thiscall(cMission*, char code, const char* text): msg 0x87
constexpr uintptr_t kRvaCheckMission = 0x94e50;     // __thiscall(cMission*): all tasks done -> success branch
constexpr uintptr_t kRvaHunterMissionEnd = 0x50760; // __thiscall(hunter, int 0): called in the success branch
constexpr uintptr_t kRvaClaimCallback = 0x1cd40;    // __cdecl(cObjAnimal*): clears +0x404, runs the script callback
constexpr uintptr_t kMissionStatus = 0x254;         // 1 running, 2 success, 3 failed
constexpr uintptr_t kMissionHunter = 0x258;
constexpr uintptr_t kHunterProfile = 0x1f8;
constexpr uintptr_t kAnimalClaimable = 0x404;
constexpr uintptr_t kAnimalHealth = 0x19C;
constexpr char kMsgSuccess = 5;
constexpr char kMsgFailed = 6;
constexpr char kMsgCheckpoint = 7;  // cObjTriggerLevel (level exit): "Checkpoint reached", then the autosave menu
constexpr int kExitHunterArg = 0x40000000;  // the exit's 0x50760(hunter, 2.0f), passed as the float's bits

struct Handlers {
    Handler request = nullptr;
    Handler commit = nullptr;
};

HMODULE g_game = nullptr;
safetyhook::InlineHook* g_msg_hook = nullptr;
safetyhook::InlineHook* g_check_hook = nullptr;
safetyhook::InlineHook* g_claim_hook = nullptr;
using SetMapIsCompleteFn = void(__thiscall*)(void* profile);
using HunterEndFn = void(__thiscall*)(void* hunter, int arg);
SetMapIsCompleteFn g_set_map_complete = nullptr;
HunterEndFn g_hunter_end = nullptr;
bool g_in_apply = false;
bool g_client_claim_script = true;  // [Animals] ClientClaimScript: a client runs the claim callback of a host claim
std::set<uint16_t> g_claim_ran;     // client: animals whose claim callback ran here (this map)
bool g_pending_end = false;
net::GameEvent g_pending_ev;
uint32_t g_check_swallowed = 0;
uint16_t g_seq = 0;
// Map flow (kMapStart). g_map is this process's map. The host announces each start once it is hosting.
std::string g_map;
uint16_t g_map_serial = 0;
bool g_map_committed = false;
std::string g_host_map;         // client: last map the host announced
bool g_client_seen_map = false;  // client: first announcement after joining only loads on a different map
uint16_t g_host_serial = 0;      // client: serial of that announcement (repeats of it are not new map starts)
bool g_loaded_alone = false;     // client: its current map was loaded without a host announcement for it
uint16_t g_followed_serial = 0;  // client: serial we issued spmap for, and when
uint32_t g_followed_ms = 0;
// Host: clients that got this map's journal (kSyncDone). Live commits go to them only; a client that is
// still loading gets everything in order when it asks, instead of commits for entities it does not have yet.
std::set<uint8_t> g_synced;
// Client side of the same handshake.
uint16_t g_sync_sent_serial = 0;  // local map serial the last kSyncRequest was for (0 = none)
uint32_t g_sync_sent_ms = 0;
bool g_synced_local = false;
bool g_catchup = false;
uint32_t g_catchup_done_ms = 0;
uint32_t g_recheck_ms = 0;  // a skipped cutscene can still place the hunter at its own spot: look once more
std::vector<net::GameEvent> g_journal;
std::vector<uint32_t> g_journal_ms;  // GetTickCount of each commit: a replayed fire carries its age in f0
std::map<uint16_t, Handlers> g_handlers;
std::map<uint16_t, bool> g_unknown_logged;

void* Mission() {
    if (!g_game) return nullptr;
    void* m = nullptr;
    core::safe_read(reinterpret_cast<char*>(g_game) + kRvaMission, m);
    return m;
}

void* Rva(uintptr_t rva) { return g_game ? reinterpret_cast<char*>(g_game) + rva : nullptr; }

// The RVAs are for one retail build. Refuse to hook anything that does not look like the expected prologue.
void* CheckedRva(uintptr_t rva, const char* label) {
    static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
    void* p = Rva(rva);
    uint8_t got[5] = {};
    if (!p || !core::safe_copy(p, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
        spdlog::error("events: {} at GameDll7+{:#x} has unexpected bytes; not hooked (wrong game build?)", label, rva);
        return nullptr;
    }
    return p;
}

bool guarded_message(void* mission, char code, const char* text) {
    __try {
        g_msg_hook->unsafe_thiscall<void>(mission, code, text);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_success_side_effects(void* mission) {
    __try {
        *reinterpret_cast<int*>(static_cast<char*>(mission) + kMissionStatus) = 2;
        void* hunter = *reinterpret_cast<void**>(static_cast<char*>(mission) + kMissionHunter);
        if (hunter) {
            void* profile = *reinterpret_cast<void**>(static_cast<char*>(hunter) + kHunterProfile);
            if (profile && g_set_map_complete) g_set_map_complete(profile);
            if (g_hunter_end) g_hunter_end(hunter, 0);
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// A level exit marks the map complete on the activator's profile before its checkpoint message (cObjTriggerLevel
// GameDll 0x6c420 / 0x6c610), then calls 0x50760(hunter, 2.0f). A client never runs its own exit (flow.cpp asks the
// host), so it gets the same here: its profile moves on to the next map like the host's.
bool guarded_checkpoint_side_effects(void* mission) {
    __try {
        void* hunter = *reinterpret_cast<void**>(static_cast<char*>(mission) + kMissionHunter);
        if (hunter) {
            void* profile = *reinterpret_cast<void**>(static_cast<char*>(hunter) + kHunterProfile);
            if (profile && g_set_map_complete) g_set_map_complete(profile);
            if (g_hunter_end) g_hunter_end(hunter, kExitHunterArg);
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_claim(void* animal) {
    __try {
        g_claim_hook->unsafe_ccall<void>(animal);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Kinds whose i0 is a per-subject serial (story.cpp): a second arm, OnHit or fire of the same trigger, or a prop anim
// / PlaySA played again, is a new entry rather than a repeat (audit-maps-c G8, review M5: MAPISLAND cScrTask09
// re-arms, a prop A -> B -> A, the same paired anim twice).
bool SerialKind(uint16_t kind) {
    return kind == kTriggerEvent || kind == kTriggerFire || kind == kTriggerHit || kind == kPropAnim || kind == kPlaySA;
}

int FindJournal(uint16_t kind, uint16_t subject, const std::string& key, uint32_t i0 = 0) {
    for (size_t i = 0; i < g_journal.size(); ++i) {
        const net::GameEvent& ev = g_journal[i];
        if (ev.kind == kind && ev.subjectId == subject && ev.key == key && (!SerialKind(kind) || ev.i0 == i0)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void NoteUnknown(uint16_t kind, const char* dir) {
    if (g_unknown_logged[kind]) return;
    g_unknown_logged[kind] = true;
    spdlog::warn("events: no {} handler for kind {} (ignored)", dir, kind);
}

// ---- kMissionEnd ----

// Client: replay what the host's mission did. Success mirrors cMission::CheckMissionStatus's success branch
// (status 2, SetMapIsComplete on this player's profile, the hunter call), then the message that ends the map.
bool ApplyMissionEnd(const net::GameEvent& ev) {
    void* mission = Mission();
    if (!mission || !g_msg_hook) return false;
    const char code = static_cast<char>(ev.i0);
    g_in_apply = true;
    bool ok = true;
    if (code == kMsgSuccess) ok = guarded_success_side_effects(mission);
    else if (code == kMsgFailed) *reinterpret_cast<int*>(static_cast<char*>(mission) + kMissionStatus) = 3;
    else if (code == kMsgCheckpoint) ok = guarded_checkpoint_side_effects(mission);
    // The HUD shows it a moment later (stream message 0x87); flow.cpp lets exactly this one through.
    flow::AllowMissionMessage(code);
    ok = guarded_message(mission, code, ev.key.c_str()) && ok;
    g_in_apply = false;
    spdlog::info("events: applied host mission end code={} text='{}' ok={}", static_cast<int>(code), ev.key, ok);
    return true;
}

void OnMissionEndCommit(const net::GameEvent& ev) {
    if (ApplyMissionEnd(ev)) return;
    g_pending_end = true;
    g_pending_ev = ev;
    spdlog::warn("events: mission end code={} waiting for cMission", ev.i0);
}

// ---- kTrophyClaim ----

void OnClaimRequest(const net::GameEvent& ev) {
    void* animal = animals::Find(ev.subjectId);
    if (!animal) {
        spdlog::warn("events: claim request id={} from player {}: no animal", ev.subjectId, ev.actorId);
        return;
    }
    int claimable = 0;
    float hp = 1.f;
    core::safe_read(static_cast<char*>(animal) + kAnimalClaimable, claimable);
    core::safe_read(static_cast<char*>(animal) + kAnimalHealth, hp);
    if (FindJournal(kTrophyClaim, ev.subjectId, "") >= 0) {
        spdlog::info("events: claim request id={} from player {}: already claimed", ev.subjectId, ev.actorId);
        Commit(kTrophyClaim, ev.subjectId, 0, 0.f, "", ev.actorId);
        return;
    }
    if (claimable == 0 || hp > 0.f) {
        spdlog::warn("events: claim request id={} from player {} rejected (claimable={} hp={:.1f})", ev.subjectId,
                     ev.actorId, claimable, hp);
        return;
    }
    g_in_apply = true;
    const bool ok = guarded_claim(animal);
    g_in_apply = false;
    spdlog::info("events: claim id={} by player {} ran on host ok={}", ev.subjectId, ev.actorId, ok);
    if (ok) Commit(kTrophyClaim, ev.subjectId, 0, 0.f, "", ev.actorId);
}

void OnClaimCommit(const net::GameEvent& ev) {
    // The host ran the claim script; how the mission ends arrives as kMissionEnd. The script's other effects are per
    // process (MAP2C cScrTask05: leave the boar respawn loop, cave_meat.Activate(), StartTask 5/6, stealth off;
    // audit-a G9), so this client runs the same callback once: the original 0x1cd40 (clears +0x404, runs the level
    // script's callback when one is set). Not under g_in_apply: CheckMissionStatus and mission messages the callback
    // reaches stay swallowed here.
    void* animal = animals::Find(ev.subjectId);
    bool ran = false, ok = false;
    if (animal) {
        if (g_client_claim_script && g_claim_hook && g_claim_ran.insert(ev.subjectId).second) {
            ok = guarded_claim(animal);
            ran = true;
        }
        *reinterpret_cast<int*>(static_cast<char*>(animal) + kAnimalClaimable) = 0;
    }
    spdlog::info("events: trophy id={} claimed by player {}{}", ev.subjectId, ev.actorId,
                 ran ? (ok ? "; claim script ran here" : "; claim script faulted here") : "");
}

// ---- kMapStart ----

std::string MapNameFromLabel(const char* label) {
    std::string s = label ? label : "";
    const size_t slash = s.find_last_of("\\/");
    if (slash != std::string::npos) s.erase(0, slash + 1);
    const size_t dot = s.rfind('.');
    if (dot != std::string::npos) s.resize(dot);
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

bool IsMenuMap(const std::string& name) { return name == "MENUMAP"; }

void OnMapStartCommit(const net::GameEvent& ev) {
    const std::string want = MapNameFromLabel(ev.key.c_str());
    // The name goes into a console command line: accept only plain map names (A-Z, 0-9, _), so a host cannot
    // smuggle separators or extra commands into this client's console.
    const bool plain = !want.empty() && want.size() <= 31 &&
                       std::all_of(want.begin(), want.end(), [](unsigned char c) { return std::isalnum(c) || c == '_'; });
    if (!plain) {
        spdlog::warn("events: rejected map name from host (len {})", want.size());
        return;
    }
    const bool first = !g_client_seen_map;
    const bool repeat = !first && ev.subjectId == g_host_serial;
    // This client loaded its current map by itself after the host's last announcement (it continued from its own
    // results screen at a mission end, as the host did). A new host serial for that same map is then the host
    // arriving, not the host replaying it: reloading would be the same-map replay that crashes at the next teardown.
    const bool arrived_alone = !first && want == g_map && g_loaded_alone && ev.subjectId != g_host_serial;
    g_client_seen_map = true;
    g_host_map = want;
    g_host_serial = ev.subjectId;
    if (want == g_map && (first || repeat || arrived_alone)) {
        // A join, the start of the host's catch-up reply for the map this client already followed, or a map this
        // client reached on its own.
        g_loaded_alone = false;
        spdlog::info("events: host map '{}' (serial {}) is already loaded here{}", want, ev.subjectId,
                     arrived_alone ? " (this client got here on its own)" : "");
        return;
    }
    // The catch-up reply repeats the announcement while our spmap is still loading: do not start it twice.
    if (want != g_map && g_followed_serial == ev.subjectId && GetTickCount() - g_followed_ms < 30000) {
        spdlog::info("events: host map '{}' (serial {}) already loading here", want, ev.subjectId);
        return;
    }
    g_followed_serial = ev.subjectId;
    g_followed_ms = GetTickCount();
    // This client continued with the host from its results screen (flow.cpp, kLevelNext): its own mod_newlevel plays
    // the chapter movies and loads that map. Starting another load would cut it short.
    if (want != g_map && flow::AlreadyFollowing(want)) {
        spdlog::info("events: host map '{}' (serial {}): this client is already on its way there", want, ev.subjectId);
        return;
    }
    // A new serial for the map we are on is the host replaying it ([Coop] FullReloadOnReplay makes that a full load).
    // From the main menu, spmap is fatal: it queues closemainmenu and loads the map first, and the deferred close then
    // unloads the menu's GUI models after their world is gone (cBoneStatic::UnlinkFromParent AV, 2026-10-17 22:30).
    // mod_spmap is the game's own menu loader (GameDll MOD_LoadLevel 0x36b0): it closes the main and game menus at
    // once (a results screen too), records the map as the profile's mission (its next map then matches the host's),
    // then queues spmap. Its mission table names are lower case. menumap is how the game itself enters the menu map.
    if (!IsMenuMap(g_map)) flow::BeforeFollow();
    std::string lower = want;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    const std::string cmd = IsMenuMap(want) ? "menumap " + want : "mod_spmap " + lower;
    const int r = console::Exec(cmd.c_str());
    spdlog::info("events: following host to '{}' (serial {}, here '{}'): {} -> {}", want, ev.subjectId, g_map, cmd, r);
}

// ---- late join / map change catch-up (kSyncRequest, kSyncDone) ----

net::GameEvent MapStartEvent() {
    for (const net::GameEvent& ev : g_journal) {
        if (ev.kind == kMapStart) return ev;
    }
    net::GameEvent ev;
    ev.flags = net::game_event_flag::kCommit;
    ev.actorId = 0;
    ev.kind = kMapStart;
    ev.subjectId = g_map_serial;
    ev.i0 = IsMenuMap(g_map) ? 1u : 0u;
    ev.key = g_map.substr(0, net::kMaxGameEventKeyChars);
    return ev;
}

// Host: the client has a map loaded and a hunter. Same map: the whole journal, in commit order, to that
// client only. Another map: the announcement, so it follows; it asks again once that map is up.
void OnSyncRequest(const net::GameEvent& ev) {
    const uint8_t who = ev.actorId;
    if (g_map.empty()) {
        spdlog::info("events: sync request from player {}: no map here yet", who);
        return;
    }
    SendGameEventTo(who, MapStartEvent());
    const std::string theirs = MapNameFromLabel(ev.key.c_str());
    if (theirs != g_map) {
        g_synced.erase(who);
        spdlog::info("events: sync request from player {} on '{}'; host is on '{}', sent the map start", who, theirs,
                     g_map);
        return;
    }
    // A replayed fire or OnHit carries its age (seconds) in f0, which they do not use otherwise: the client skips the
    // cutscenes of old ones and keeps the ones the host is watching right now.
    const uint32_t now = GetTickCount();
    uint32_t sent = 0;
    for (size_t i = 0; i < g_journal.size(); ++i) {
        net::GameEvent c = g_journal[i];
        if (c.kind == kMapStart) continue;
        if ((c.kind == kTriggerFire || c.kind == kTriggerHit) && i < g_journal_ms.size()) {
            c.f0 = static_cast<float>(now - g_journal_ms[i]) / 1000.f;
        }
        if (SendGameEventTo(who, c)) ++sent;
    }
    net::GameEvent done;
    done.flags = net::game_event_flag::kCommit;
    done.actorId = 0;
    done.kind = kSyncDone;
    done.subjectId = g_map_serial;
    done.i0 = sent;
    done.key = g_map.substr(0, net::kMaxGameEventKeyChars);
    SendGameEventTo(who, done);
    g_synced.insert(who);
    spdlog::info("events: sync player {} on '{}': replayed {} of {} commits", who, g_map, sent, g_journal.size());
}

void OnSyncDoneCommit(const net::GameEvent& ev) {
    const std::string map = MapNameFromLabel(ev.key.c_str());
    if (map != g_map) {
        spdlog::warn("events: sync done for '{}' but this process is on '{}'", map, g_map);
        return;
    }
    g_synced_local = true;
    g_catchup_done_ms = GetTickCount();
    spdlog::info("events: synced with host on '{}' ({} commits replayed)", map, ev.i0);
}

// Client, every frame: ask once the map and hunter are ready, then end the catch-up when the replayed
// story has settled. A client far from the host at that point (late join, slow load) is moved to the host.
void PumpClientSync() {
    if (NetIsHost() || !NetIsOnline() || g_map.empty()) return;
    const uint32_t now = GetTickCount();
    // At the main menu there is no hunter: ask anyway, so a friend who joins from the menu learns the host's map.
    if (!g_synced_local && (LocalPlayer().valid || IsMenuMap(g_map)) &&
        (g_sync_sent_serial != g_map_serial || now - g_sync_sent_ms > 10000)) {
        g_sync_sent_serial = g_map_serial;
        g_sync_sent_ms = now;
        g_catchup = true;
        Request(kSyncRequest, g_map_serial, 0, 0.f, g_map);
    }
    if (g_recheck_ms && static_cast<int32_t>(now - g_recheck_ms) >= 0) {
        g_recheck_ms = 0;
        const float dist = DistanceToHost();
        if (dist > kJoinTeleportMetres && !story::CatchupBusy()) {
            spdlog::info("events: host {:.1f} m away after the catch-up; moving again", dist);
            TeleportToHost("join re-check");
        }
    }
    if (!g_catchup || !g_synced_local) return;
    if (IsMenuMap(g_map)) {  // nobody to move at the menu
        g_catchup = false;
        return;
    }
    const uint32_t since = now - g_catchup_done_ms;
    if (since < 1000 || (story::CatchupBusy() && since < 15000)) return;
    const float dist = DistanceToHost();
    if (dist < 0.f && since < 15000) return;  // no host position yet
    g_catchup = false;
    const bool move = dist > kJoinTeleportMetres;
    spdlog::info("events: catch-up finished after {} ms; host {:.1f} m away{}", since, dist,
                 move ? ", moving to the host" : "");
    if (move) {
        TeleportToHost("join catch-up");
        g_recheck_ms = now + 4000;
    }
}

// ---- kAnimalDied ----

void OnAnimalDiedCommit(const net::GameEvent& ev) { animals::OnAnimalDied(ev); }

void OnAnimalDiedRequest(const net::GameEvent& ev) {
    static bool logged = false;
    if (logged) return;
    logged = true;
    spdlog::info("events: AnimalDied is host-simulated; ignored request subject={} actor={}", ev.subjectId,
                 ev.actorId);
}

// Per map: the journal only holds outcomes of the current map.
void ResetMap() {
    g_journal.clear();
    g_journal_ms.clear();
    g_seq = 0;
    g_pending_end = false;
    g_check_swallowed = 0;
    g_synced.clear();
    g_synced_local = false;
    g_sync_sent_serial = 0;
    g_recheck_ms = 0;
    g_claim_ran.clear();
}

// ---- hooks ----

void __fastcall hk_MissionMessage(void* self, void* /*edx*/, char code, const char* text) {
    const std::string t = core::safe_cstr(text, 64);
    if (g_in_apply || !NetIsOnline()) {
        g_msg_hook->unsafe_thiscall<void>(self, code, text);
        return;
    }
    const std::string caller = core::describe_address(_ReturnAddress());
    if (NetIsHost()) {
        spdlog::info("events: host mission message code={} text='{}' caller={}", static_cast<int>(code), t, caller);
        g_msg_hook->unsafe_thiscall<void>(self, code, text);
        Commit(kMissionEnd, 0, static_cast<uint8_t>(code), 0.f, t, 0);
        return;
    }
    spdlog::info("events: client mission message swallowed code={} text='{}' caller={}", static_cast<int>(code), t,
                 caller);
}

void __fastcall hk_CheckMission(void* self, void* /*edx*/) {
    if (g_in_apply || !NetIsOnline() || NetIsHost()) {
        g_check_hook->unsafe_thiscall<void>(self);
        return;
    }
    // Client: the host's mission decides. Scripts call this every frame once a hunt can end.
    if ((g_check_swallowed++ % 300) == 0) {
        spdlog::info("events: client CheckMissionStatus skipped (x{}) caller={}", g_check_swallowed,
                     core::describe_address(_ReturnAddress()));
    }
}

void __cdecl hk_ClaimCallback(void* animal) {
    if (g_in_apply || !NetIsOnline()) {
        g_claim_hook->unsafe_ccall<void>(animal);
        return;
    }
    const int id = animals::IdOf(animal);
    if (NetIsHost()) {
        g_claim_hook->unsafe_ccall<void>(animal);
        spdlog::info("events: host claimed trophy id={}", id);
        if (id >= 0) Commit(kTrophyClaim, static_cast<uint16_t>(id), 0, 0.f, "", 0);
        return;
    }
    if (animal) *reinterpret_cast<int*>(static_cast<char*>(animal) + kAnimalClaimable) = 0;
    spdlog::info("events: client claim id={} sent to host", id);
    if (id >= 0) Request(kTrophyClaim, static_cast<uint16_t>(id), 0, 0.f, "");
}

class EventsModule : public dh2::Module {
public:
    const char* Name() const override { return "events"; }
    int Priority() const override { return 41; }

    void OnInit() override {
        SetHandler(kAnimalDied, &OnAnimalDiedRequest, &OnAnimalDiedCommit);
        SetHandler(kMissionEnd, nullptr, &OnMissionEndCommit);
        SetHandler(kTrophyClaim, &OnClaimRequest, &OnClaimCommit);
        SetHandler(kMapStart, nullptr, &OnMapStartCommit);
        SetHandler(kSyncRequest, &OnSyncRequest, nullptr);
        SetHandler(kSyncDone, nullptr, &OnSyncDoneCommit);
        g_client_claim_script =
            GetPrivateProfileIntA("Animals", "ClientClaimScript", 1, (core::self_dir() / "dh2coop.ini").string().c_str()) != 0;
        spdlog::info("events: registry AnimalDied={} MissionEnd={} TrophyClaim={} MapStart={}", kAnimalDied,
                     kMissionEnd, kTrophyClaim, kMapStart);
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_game = game;
        g_set_map_complete =
            reinterpret_cast<SetMapIsCompleteFn>(core::find_export(game, "?SetMapIsComplete@cHunterProfile@@QAEXXZ"));
        g_hunter_end = reinterpret_cast<HunterEndFn>(CheckedRva(kRvaHunterMissionEnd, "hunter mission end"));
        g_msg_hook = core::install_hook("cMission message (0x91c20)", CheckedRva(kRvaMissionMessage, "mission message"),
                                        reinterpret_cast<void*>(&hk_MissionMessage));
        g_check_hook = core::install_hook("cMission::CheckMissionStatus (0x94e50)",
                                          CheckedRva(kRvaCheckMission, "check mission"),
                                          reinterpret_cast<void*>(&hk_CheckMission));
        g_claim_hook = core::install_hook("cObjAnimal::ClaimCallback (0x1cd40)",
                                          CheckedRva(kRvaClaimCallback, "claim callback"),
                                          reinterpret_cast<void*>(&hk_ClaimCallback));
    }

    void OnMapLoaded(const char* bspName) override {
        ResetMap();
        const std::string loaded = MapNameFromLabel(bspName);
        // A client on a map the host has not announced (yet): it got here itself, not by following.
        g_loaded_alone = !NetIsHost() && NetIsOnline() && g_client_seen_map && loaded != g_host_map;
        g_map = loaded;
        ++g_map_serial;
        g_map_committed = false;
        spdlog::info("events: journal cleared for map '{}' (serial {})", g_map, g_map_serial);
    }
};

DH2_MODULE(EventsModule)

}  // namespace

void SetHandler(uint16_t kind, Handler onRequest, Handler onCommit) {
    Handlers& row = g_handlers[kind];
    if (row.request || row.commit) {
        spdlog::info("events: replacing handler for kind {}", kind);
    }
    row.request = onRequest;
    row.commit = onCommit;
}

void Commit(uint16_t kind, uint16_t subject, uint32_t i0, float f0, std::string_view key, uint8_t actor) {
    if (!NetIsHost()) return;
    const std::string trimmed(key.substr(0, net::kMaxGameEventKeyChars));
    const auto send = [](const net::GameEvent& ev) {
        if (ev.kind == kMapStart) return SendGameEvent(ev);
        bool any = false;
        for (uint8_t id : g_synced) any = SendGameEventTo(id, ev) || any;
        return any;
    };
    const int existing = FindJournal(kind, subject, trimmed, i0);
    if (existing >= 0) {
        // Everyone synced already has it. A client asking again (its own trigger fired after ours) gets it back
        // alone: re-broadcasting each repeat flooded the other clients (MAP12C 2026-10-17 10:14, ~100/s).
        if (actor != 0 && g_synced.count(actor) != 0) SendGameEventTo(actor, g_journal[static_cast<size_t>(existing)]);
        return;
    }
    if (g_journal.size() >= kJournalCap) {
        spdlog::warn("events: journal full, dropped kind={} subject={}", kind, subject);
        return;
    }
    net::GameEvent ev;
    ev.flags = net::game_event_flag::kCommit;
    ev.actorId = actor;
    ev.seq = ++g_seq;
    ev.kind = kind;
    ev.subjectId = subject;
    ev.i0 = i0;
    ev.f0 = f0;
    ev.key = trimmed;
    g_journal.push_back(ev);
    g_journal_ms.push_back(GetTickCount());
    const bool sent = send(ev);
    spdlog::info("events: commit kind={} subject={} actor={} i0={} key='{}' seq={} sent={} journal={}", kind, subject,
                 actor, i0, trimmed, ev.seq, sent, g_journal.size());
}

void Send(uint16_t kind, uint16_t subject, uint32_t i0, float f0, std::string_view key, uint8_t actor) {
    if (!NetIsHost()) return;
    net::GameEvent ev;
    ev.flags = net::game_event_flag::kCommit;
    ev.actorId = actor;
    ev.kind = kind;
    ev.subjectId = subject;
    ev.i0 = i0;
    ev.f0 = f0;
    ev.key.assign(key.data(), key.size() > net::kMaxGameEventKeyChars ? net::kMaxGameEventKeyChars : key.size());
    for (uint8_t id : g_synced) SendGameEventTo(id, ev);
}

void Request(uint16_t kind, uint16_t subject, uint32_t i0, float f0, std::string_view key) {
    if (NetIsHost() || !NetIsOnline()) return;
    net::GameEvent ev;
    ev.kind = kind;
    ev.subjectId = subject;
    ev.i0 = i0;
    ev.f0 = f0;
    ev.key.assign(key.data(), key.size() > net::kMaxGameEventKeyChars ? net::kMaxGameEventKeyChars : key.size());
    const bool sent = SendGameEvent(ev);
    spdlog::info("events: request kind={} subject={} key='{}' sent={}", kind, subject, ev.key, sent);
}

void OnNet(const net::GameEvent& ev) {
    if (ev.commit()) {
        if (NetIsHost()) return;
        const auto it = g_handlers.find(ev.kind);
        if (it == g_handlers.end() || !it->second.commit) {
            NoteUnknown(ev.kind, "commit");
            return;
        }
        spdlog::info("events: apply commit kind={} subject={} actor={} i0={} key='{}'", ev.kind, ev.subjectId,
                     ev.actorId, ev.i0, ev.key);
        it->second.commit(ev);
        return;
    }
    if (!NetIsHost()) return;
    const auto it = g_handlers.find(ev.kind);
    if (it == g_handlers.end() || !it->second.request) {
        NoteUnknown(ev.kind, "request");
        return;
    }
    it->second.request(ev);
}

void Pump() {
    if (g_pending_end && !NetIsHost() && NetIsOnline() && ApplyMissionEnd(g_pending_ev)) g_pending_end = false;
    // The first map's load happens before hosting starts (auto-host runs on the next frame), so announce here.
    if (NetIsHost() && !g_map_committed && !g_map.empty()) {
        g_map_committed = true;
        Commit(kMapStart, g_map_serial, IsMenuMap(g_map) ? 1u : 0u, 0.f, g_map, 0);
    }
    PumpClientSync();
}

void Reset() {
    ResetMap();
    g_map_committed = false;
    g_host_map.clear();
    g_client_seen_map = false;
    g_host_serial = 0;
    g_followed_serial = 0;
    g_catchup = false;
}

void OnPeerLeft(uint8_t playerId) { g_synced.erase(playerId); }

bool CatchingUp() { return g_catchup; }

const std::string& CurrentMap() { return g_map; }

bool SameMapAsHost() {
    if (NetIsHost() || !NetIsOnline() || g_host_map.empty()) return true;
    return g_host_map == g_map;
}

uint16_t LocalMapSerial() { return g_map_serial; }

bool SnapshotIsCurrent(uint16_t hostMapSerial) {
    if (NetIsHost()) return true;
    // Nothing announced yet (fresh join): the ids may belong to another map.
    if (g_host_map.empty()) return false;
    return hostMapSerial == g_host_serial && g_host_map == g_map;
}

void CommitHudMissionMessage(int code, const char* text) {
    if (!NetIsHost() || !NetIsOnline()) return;
    const std::string t = core::safe_cstr(text, 64).substr(0, net::kMaxGameEventKeyChars);
    if (FindJournal(kMissionEnd, 0, t) >= 0) return;  // came through 0x91c20, already committed
    spdlog::info("events: host HUD mission message code={} text='{}' did not pass cMission; committing it", code, t);
    Commit(kMissionEnd, 0, static_cast<uint32_t>(code), 0.f, t, 0);
}

void DebugClaim(uint16_t animalId) {
    void* animal = animals::Find(animalId);
    spdlog::info("events: coop_claim id={} animal={:#x}", animalId, reinterpret_cast<uintptr_t>(animal));
    if (animal && g_claim_hook) hk_ClaimCallback(animal);
}

}  // namespace dh2::coop::events
