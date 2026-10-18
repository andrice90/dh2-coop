// Story mode: the host decides every script trigger (re/notes/lead-story-coop.md, re/notes/triggers.md).
// Arm (GameDll 0x68eb0, kTriggerEvent), OnHit (0x6b140, kTriggerHit) and fire (0x68130, kTriggerFire) of a
// cObjTriggerEvent are host commits; a client's own arm, touch or fire is a request. Cutscene skips, prop anims and
// paired anims (PlaySA) are replayed too. Also logs trigger volumes and CompleteTask.
#include <windows.h>
#include <intrin.h>

#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <spdlog/spdlog.h>

#include "coop/animals.h"
#include "coop/coop.h"
#include "coop/events.h"
#include "coop/player_tracker.h"
#include "coop/puppets.h"
#include "engine/console_cmds.h"
#include "engine/entity_api.h"
#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"

namespace dh2::coop::story {
namespace {

constexpr uintptr_t kEntId = 0x04;
constexpr uintptr_t kEntTargetName = 0x198;   // cObjBaseEntity target name (char*), as FindEntScrObject compares
constexpr uintptr_t kRvaTriggerEvent = 0x68eb0;  // GameDll7: fastcall(cObjTriggerEvent*) activation
constexpr uintptr_t kRvaCompleteTask = 0x95840;  // GameDll7: thiscall(cMission*, int task)
constexpr uintptr_t kRvaTriggerFire = 0x68130;   // GameDll7: thiscall(cObjTriggerEvent*, activator): script OnActivate
constexpr uintptr_t kRvaTriggerHit = 0x6b140;    // GameDll7: thiscall(trigger, activator, pos*, dir*), ret 0xc: OnHit
constexpr uintptr_t kRvaTriggerDisarm = 0x68fd0; // GameDll7: fastcall(trigger): +0x1e4 = 0, mission unregister
constexpr uintptr_t kRvaTriggerUpdate = 0x68800; // GameDll7: thiscall(trigger, float dt), ret 4 (vt +0x44)
constexpr uintptr_t kRvaFindEntity = 0x6d80;     // GameDll7: stdcall(const char*): first cObjBaseEntity of that name
constexpr uintptr_t kRvaTaskIndex = 0x94d10;     // GameDll7: thiscall(cMission*, int task id) -> list index or -1
constexpr uintptr_t kRvaTaskEnded = 0x956f0;     // GameDll7: thiscall(cMission*, int index) -> that task has ended
constexpr uintptr_t kRvaTriggerVt = 0x10afa0;    // cObjTriggerEvent
// cObjTriggerEvent fields (re/notes/audit-maps-b.md 0.1, re/notes/triggers.md).
constexpr uintptr_t kTrigBoxMin = 0xc4;          // float[3]; max at +0xd0 (inside test 0x68740)
constexpr uintptr_t kTrigBoxMax = 0xd0;
constexpr uintptr_t kTrigScrObj = 0x1b4;         // script object the trigger's functions run on
constexpr uintptr_t kTrigArmed = 0x1e4;
constexpr uintptr_t kTrigHitRan = 0x1e8;         // OnHit ran since the last arm
constexpr uintptr_t kTrigInside = 0x1f8;         // Update: someone is inside this frame, run UpdateTrigger
constexpr uintptr_t kTrigState = 0x1fc;          // 0 idle, 4 BeginCutScene next, 5 cutscene up, 3 delay, 2 fire next, 1 fired
constexpr uintptr_t kTrigWaitMask = 0x200;       // WaitTask bits: task ids that must have ended in this process's mission
constexpr uintptr_t kTrigFnOnHit = 0x208;
constexpr uintptr_t kTrigFnBeginCut = 0x210;
constexpr uintptr_t kTrigFnUpdate = 0x214;
constexpr uintptr_t kTrigActivateBy = 0x224;     // char*: ActivateBy name ("hunter", "car1", "helicopter"); null = a player
constexpr uintptr_t kEntPosition = 0xe8;
// Return addresses of the task-end OnHit calls (vt +0x8c with the mission hunter): cMissionTask::End (0x96da0) for
// an ended and for a failed task, and the forced end 0x98a50. Each process's own mission makes them.
constexpr uintptr_t kTaskEndHitReturns[] = {0x9705d, 0x970df, 0x98e54};

safetyhook::InlineHook* g_volume_hook = nullptr;
safetyhook::InlineHook* g_event_hook = nullptr;
safetyhook::InlineHook* g_task_hook = nullptr;
safetyhook::InlineHook* g_setanim_hook = nullptr;
safetyhook::InlineHook* g_playsa_hook = nullptr;
bool g_in_prop = false;
// Host: the last SetAnim / PlaySA per entity. A new key, or the same key again after a pause, is a new commit with the
// next serial (i0); a call repeated every frame is one commit (review M5).
struct LastAnim {
    std::string key;
    uint32_t ms = 0;
    uint32_t commits = 0;
};
std::unordered_map<int16_t, LastAnim> g_prop_last;
std::unordered_map<int16_t, LastAnim> g_playsa_last;
constexpr uint32_t kAnimRepeatGapMs = 2000;
constexpr uint32_t kMaxAnimCommits = 16;  // per entity per map, so a looping script cannot fill the journal
// Kept for the life of the map: PlaySA stores the anim name pointer on the partner mesh.
struct PlayStrings {
    std::string anim;
    std::string partner;
};
std::unordered_map<int16_t, PlayStrings> g_play_keep;
bool g_log = true;
// Triggers whose OnActivate (0x68130) this process actually ran since their last arm, and the host fire serial that
// run was (0: ran here before any host commit for it). The +0x1fc flag can be set without that call (MAP12C
// cScrTask01, 2026-10-16 22:45): the host fire was then skipped and the client stayed in the opening cutscene camera.
std::unordered_map<int16_t, uint32_t> g_ran_fire;
// Serials (audit-c G8, review M5). The host numbers every arm, OnHit and fire of a trigger, per trigger id and map,
// and sends the number in the commit's i0: a second arm or fire of the same trigger is a new journal entry. Clients
// keep the highest number they applied, so a commit the host hands back again (a repeated request) is not re-run.
std::unordered_map<int16_t, uint32_t> g_arm_serial, g_hit_serial, g_fire_serial;
std::unordered_map<int16_t, uint32_t> g_applied_arm, g_applied_hit, g_applied_fire;
// Client: host commits that cannot be applied yet, in arrival order (a cutscene camera, or this trigger's own
// BeginCutScene / cutscene still running here).
struct Deferred {
    net::GameEvent ev;
    uint32_t since = 0;
};
std::vector<Deferred> g_deferred;
// Host, audit-b G5: a client's fire request that arrives while the host's own sequence is pending (states 2-5) waits
// for the host's fire; only after this long is it forced, and the host's own later fire is then skipped once.
std::unordered_map<int16_t, uint32_t> g_fire_wait;    // trigger id -> GetTickCount of the first such request
std::unordered_set<int16_t> g_fired_for_request;     // OnActivate ran for a request; skip the host's own next fire
uint32_t g_force_fire_ms = 45000;                    // [Story] FireRequestForceSec
bool g_sync_onhit = true;                            // [Story] SyncOnHit
bool g_puppet_update = true;                         // [Story] PuppetUpdateTrigger

// Client cutscene watchdog. A cutscene that waits on NPC tasks (MAP8 cutscene 2: MoveToPos / LookToEntity on the
// tribesmen) never ends here, where NPC brains are frozen mirrors: the client sat in it 2.4 min until RETURN while
// the host's took 34 s (MAP8, 2026-10-18). Started by an applied host OnHit; skipped through the trigger's own skip.
// Two rules: the cutscene still runs StuckCutsceneSec after the host's fire arrived (the host's was real: its fire
// came 5 s+ after our start), or it runs StuckCutsceneMaxSec at all (the longest script waits sum to ~35 s).
struct CutWatch {
    uint32_t start = 0;
    uint32_t fire_at = 0;
    bool seen = false;
};
std::unordered_map<int16_t, CutWatch> g_cut_watch;
uint32_t g_stuck_after_fire_ms = 20000;   // [Story] StuckCutsceneSec
uint32_t g_stuck_max_ms = 90000;          // [Story] StuckCutsceneMaxSec

// ---- cutscene skip ----
// GameDll7 0x6b350 is the trigger's skip (the skip_cutscene key ends up here): fastcall(cObjTriggerEvent*). It sets
// +0x1f0, hides the skip HUD, and calls cScrTriggerEvent::SkipCutScene (0x6a940), which takes the script the
// trigger executed (cScrTriggerEvent +0x30, a class-manager index), sets that cScrCutScene's m_bSkip (+0x4c) and
// switches it to its "Skip" state: EndCamera, StopAllTask on the actors, DeactivateCutSceneMode.
// re/notes/lead-dropin.md. The host's skips are committed (kCutsceneSkip); a client catching up after a join skips
// the cutscenes of fires that are already old on the host.
constexpr uintptr_t kRvaTriggerSkip = 0x6b350;
constexpr uintptr_t kTriggerSkipped = 0x1f0;
constexpr uintptr_t kScrExecuted = 0x30;
constexpr uintptr_t kRvaCutSceneVt = 0x110998;  // cScrCutScene constructor 0x8e260
constexpr float kCatchupSkipAge = 3.f;          // seconds: a replayed fire older than this does not play its cutscene
uintptr_t g_game_base = 0;
safetyhook::InlineHook* g_skip_hook = nullptr;
using GetScrObjectFn = void*(__thiscall*)(void*);
using GetRecordFn = void*(__thiscall*)(void*, int);
GetScrObjectFn g_get_scr = nullptr;
GetRecordFn g_get_record = nullptr;
void** g_class_mgr = nullptr;  // &g_pClassManager (EngineDll7)
bool g_in_skip = false;
std::unordered_map<int16_t, uint32_t> g_skip_watch;  // trigger id -> GetTickCount after which we stop trying
// Depth of trigger OnActivate calls in progress. A cutscene skip during one is the game skipping by itself (the host
// profile's setting: two-PC test, every host skip came within 20 ms of its fire), never a player's key.
int g_onactivate_depth = 0;

// Opening cutscenes put the render camera on a scripted rail, metres from the hunter.
// Standing play keeps it about 1.7 m above the feet. MAP12C's cScrCutScene1 only calls
// EndCamera / DeactivateCutSceneMode after the yeti's PlaySA task, so a host fire applied
// during that wait replaces the task and the camera never releases.
bool PlayerInCutscene() {
    const LocalPlayerSample& p = LocalPlayer();
    if (!p.valid || !p.viewValid) return false;
    const float dx = p.viewPos.x - p.pos.x;
    const float dy = p.viewPos.y - p.pos.y;
    const float dz = p.viewPos.z - p.pos.z;
    return (dx * dx + dy * dy + dz * dz) > 16.f;
}

std::string Describe(void* ent) {
    if (!ent) return "null";
    int16_t id = -1;
    char* name = nullptr;
    core::safe_read(static_cast<char*>(ent) + kEntId, id);
    core::safe_read(static_cast<char*>(ent) + kEntTargetName, name);
    void* vt = nullptr;
    core::safe_read(ent, vt);
    return fmt::format("id={} name='{}' vt={}", id, name ? core::safe_cstr(name, 48) : std::string(),
                       core::describe_address(vt));
}

const char* Role() { return NetIsHost() ? "host" : (NetIsOnline() ? "client" : "solo"); }

void __fastcall hk_VolumeActivate(void* self, void*, void* activator) {
    if (g_log) {
        spdlog::info("story: [{}] volume activate {} by {} caller={}", Role(), Describe(self), Describe(activator),
                     core::describe_address(_ReturnAddress()));
    }
    g_volume_hook->unsafe_thiscall<void>(self, activator);
}

// ---- L2 trigger events: the host decides, clients replay (kTriggerEvent) ----
bool g_sync_events = true;  // [Story] SyncTriggerEvents
bool g_in_apply = false;

int16_t IdOf(void* ent) {
    int16_t id = -1;
    core::safe_read(static_cast<char*>(ent) + kEntId, id);
    return id;
}

// OnActivate ran on this process. Host and solo number it (the next fire serial of that trigger); a client records 0
// when no host commit asked for it.
uint32_t NoteFireRan(void* ent) {
    const int16_t id = IdOf(ent);
    if (id < 0) return 0;
    if (NetIsOnline() && !NetIsHost()) {
        g_ran_fire.emplace(id, 0u);
        return 0;
    }
    const uint32_t serial = ++g_fire_serial[id];
    g_ran_fire[id] = serial;
    return serial;
}

uint32_t SerialOf(const std::unordered_map<int16_t, uint32_t>& m, int16_t id) {
    const auto it = m.find(id);
    return it == m.end() ? 0u : it->second;
}

std::string NameOf(void* ent) {
    char* name = nullptr;
    core::safe_read(static_cast<char*>(ent) + kEntTargetName, name);
    return name ? core::safe_cstr(name, 48) : std::string();
}

int TrigInt(void* ent, uintptr_t off, int fallback = 0) {
    int v = fallback;
    core::safe_read(static_cast<char*>(ent) + off, v);
    return v;
}

void* TrigPtr(void* ent, uintptr_t off) {
    void* v = nullptr;
    core::safe_read(static_cast<char*>(ent) + off, v);
    return v;
}

void SetTrigInt(void* ent, uintptr_t off, int v) { std::memcpy(static_cast<char*>(ent) + off, &v, sizeof v); }

bool IsActive(void* ent) { return TrigInt(ent, kTrigArmed, -1) == 1; }

// The trigger really armed (0x68eb0 resets +0x1fc and +0x1e8): a new cycle. Returns the host's arm serial.
uint32_t NoteArmed(void* ent) {
    const int16_t id = IdOf(ent);
    if (id < 0) return 0;
    g_ran_fire.erase(id);
    g_fire_wait.erase(id);
    g_fired_for_request.erase(id);
    return ++g_arm_serial[id];
}

// A script fault inside the engine call must not escape into the module dispatcher: it disables the whole
// coop module (MAP12C 2026-10-17 10:14:28, a forced re-fire faulted in EngineDll and the client went offline).
bool guarded_activate(void* ent) {
    __try {
        g_event_hook->unsafe_fastcall<void>(ent);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void Activate(void* ent) {
    const bool was = IsActive(ent);
    g_in_apply = true;
    if (!guarded_activate(ent)) spdlog::error("story: trigger activate faulted id={}", IdOf(ent));
    g_in_apply = false;
    if (!was && IsActive(ent)) NoteArmed(ent);
}

// Client: its own swallowed arm/fire repeats every frame while the engine still sees the condition (the host's
// answer may be deferred behind a cutscene). One request per trigger per few seconds is enough.
std::unordered_map<int32_t, uint32_t> g_requested;  // (kind << 16 | id) -> GetTickCount of the last request

bool ShouldRequest(uint16_t kind, int16_t id) {
    const int32_t key = (static_cast<int32_t>(kind) << 16) | static_cast<uint16_t>(id);
    const uint32_t now = GetTickCount();
    const auto it = g_requested.find(key);
    if (it != g_requested.end() && now - it->second < 3000) return false;
    g_requested[key] = now;
    return true;
}

// The same BSP entity id must carry the same name on both sides; otherwise the maps differ and nothing is applied.
void* FindEvent(const net::GameEvent& ev) {
    void* ent = EntityById(ev.subjectId);
    if (!ent) return nullptr;
    return NameOf(ent) == ev.key ? ent : nullptr;
}

void* guarded_scr(void* ent) {
    __try {
        return g_get_scr(ent);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

void* guarded_record(int idx) {
    __try {
        return g_get_record(*g_class_mgr, idx);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool guarded_skip(void* trigger) {
    __try {
        g_skip_hook->unsafe_fastcall<void>(trigger);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// The cutscene script this trigger executed, or null. Only a real cScrCutScene instance counts: the skip writes
// m_bSkip and switches the script to a "Skip" state that other script classes do not have.
void* TriggerCutscene(void* trigger) {
    if (!g_get_scr || !g_get_record || !g_class_mgr || !*g_class_mgr || !g_game_base) return nullptr;
    void* scr = guarded_scr(trigger);
    int idx = -1;
    if (!scr || !core::safe_read(static_cast<char*>(scr) + kScrExecuted, idx) || idx < 0) return nullptr;
    void* rec = guarded_record(idx);
    void* inst = nullptr;
    if (!rec || !core::safe_read(static_cast<char*>(rec) + 8, inst) || !inst) return nullptr;
    uintptr_t vt = 0;
    if (!core::safe_read(inst, vt) || vt != g_game_base + kRvaCutSceneVt) return nullptr;
    return inst;
}

bool Skippable(void* trigger) {
    int skipped = 1;
    return trigger && core::safe_read(static_cast<char*>(trigger) + kTriggerSkipped, skipped) && skipped == 0 &&
           TriggerCutscene(trigger) != nullptr;
}

// Keep trying to skip this trigger's cutscene for a while: the fire may still be deferred, or the cutscene
// script may start a frame after OnActivate.
void WatchSkip(uint16_t id, const char* why) {
    if (!g_skip_hook) return;
    g_skip_watch[static_cast<int16_t>(id)] = GetTickCount() + 15000;
    spdlog::info("story: will skip the cutscene of trigger id {} ({})", id, why);
}

void PumpSkips() {
    if (g_skip_watch.empty()) return;
    const uint32_t now = GetTickCount();
    for (auto it = g_skip_watch.begin(); it != g_skip_watch.end();) {
        void* ent = EntityById(it->first);
        if (ent && Skippable(ent)) {
            g_in_skip = true;
            const bool ok = guarded_skip(ent);
            g_in_skip = false;
            spdlog::info("story: skipped cutscene of '{}' (id {}) ok={}", NameOf(ent), it->first, ok);
            it = g_skip_watch.erase(it);
        } else if (static_cast<int32_t>(now - it->second) > 0) {
            it = g_skip_watch.erase(it);
        } else {
            ++it;
        }
    }
}

// The skip key's input callback (0x6b600, cdecl(trigger, pressed)) is the only manual path into 0x6b350. The same
// function also runs inside a trigger's OnActivate when this profile skips cutscenes on its own (two-PC test
// 2026-10-17: every host cutscene "skipped" within 20 ms of its fire, MAP1 intro included). Forwarding those made
// every client skip every cutscene; only a key press is the host's choice.
constexpr uintptr_t kRvaSkipInput = 0x6b600;
safetyhook::InlineHook* g_skip_input_hook = nullptr;
bool g_manual_skip = false;

void __cdecl hk_SkipInput(void* trigger, int pressed) {
    g_manual_skip = pressed != 0;
    g_skip_input_hook->unsafe_ccall<void>(trigger, pressed);
    g_manual_skip = false;
}

void __fastcall hk_TriggerSkip(void* self, void*) {
    int before = 1;
    core::safe_read(static_cast<char*>(self) + kTriggerSkipped, before);
    g_skip_hook->unsafe_fastcall<void>(self);
    if (g_in_skip || before != 0 || !g_sync_events || !NetIsHost()) return;
    // The game's own skip runs inside the trigger's OnActivate. The skip-key callback alone did not tell them apart
    // (repro 15:59:15: a skip with no key sent still came through 0x6b600).
    if (g_onactivate_depth > 0) {
        spdlog::info("story: host cutscene of '{}' skipped by the game itself (key callback {}); clients keep theirs",
                     NameOf(self), g_manual_skip);
        return;
    }
    const int16_t id = IdOf(self);
    const std::string name = NameOf(self);
    spdlog::info("story: host skipped the cutscene of '{}' (id {})", name, id);
    if (id >= 0) events::Commit(events::kCutsceneSkip, static_cast<uint16_t>(id), 0, 0.f, name, 0);
}

void OnSkipCommit(const net::GameEvent& ev) {
    if (!FindEvent(ev)) {
        spdlog::warn("story: cutscene skip id={} '{}': no such trigger here", ev.subjectId, ev.key);
        return;
    }
    WatchSkip(ev.subjectId, "host skipped it");
}

void OnEventRequest(const net::GameEvent& ev) {
    void* ent = FindEvent(ev);
    if (!ent) {
        spdlog::warn("story: trigger request id={} '{}' from player {}: no such trigger here", ev.subjectId, ev.key,
                     ev.actorId);
        return;
    }
    const int16_t id = static_cast<int16_t>(ev.subjectId);
    if (!IsActive(ent)) {
        // A client's own pickup (Food1, cave meat, radio) arms its copy of a trigger the host may have used already.
        // Re-arming would reset the host's +0x1fc / +0x1e8 and let its story branch run a second time.
        if (const uint32_t armed_before = SerialOf(g_arm_serial, id)) {
            spdlog::info("story: trigger '{}' (id {}) requested by player {}: used on the host already (arm #{}), "
                         "not armed again", ev.key, id, ev.actorId, armed_before);
            return;
        }
        Activate(ent);
    }
    spdlog::info("story: trigger '{}' (id {}) requested by player {} -> active", ev.key, ev.subjectId, ev.actorId);
    events::Commit(events::kTriggerEvent, ev.subjectId, SerialOf(g_arm_serial, id), 0.f, ev.key, ev.actorId);
}

void ApplyEventCommit(const net::GameEvent& ev) {
    void* ent = FindEvent(ev);
    if (!ent) {
        spdlog::warn("story: trigger commit id={} '{}': no such trigger here", ev.subjectId, ev.key);
        return;
    }
    const int16_t id = static_cast<int16_t>(ev.subjectId);
    uint32_t& seen = g_applied_arm[id];
    if (ev.i0 != 0 && ev.i0 <= seen) {
        // The host handed an earlier arm back (a repeated request): arming a used trigger again would let it run twice.
        spdlog::info("story: host trigger '{}' (id {}) arm #{} applied already (#{})", ev.key, id, ev.i0, seen);
        return;
    }
    if (ev.i0 != 0) seen = ev.i0;
    if (IsActive(ent)) return;
    Activate(ent);
    spdlog::info("story: applied host trigger '{}' (id {}) arm #{}", ev.key, ev.subjectId, ev.i0);
}

// ---- trigger firing (cObjTriggerEvent::OnActivate, GameDll 0x68130): kTriggerFire ----
safetyhook::InlineHook* g_fire_hook = nullptr;

bool HasFired(void* ent) {
    int fired = 0;
    core::safe_read(static_cast<char*>(ent) + 0x1fc, fired);
    return fired != 0;
}

bool guarded_fire(void* ent, void* activator) {
    __try {
        ++g_onactivate_depth;
        g_fire_hook->unsafe_thiscall<void>(ent, activator);
        --g_onactivate_depth;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (g_onactivate_depth > 0) --g_onactivate_depth;
        return false;
    }
}

void Fire(void* ent) {
    void* activator = LocalPlayer().entity;  // the script callback ignores it; the engine only needs non-null
    if (!activator) activator = ent;
    g_in_apply = true;
    if (!guarded_fire(ent, activator)) spdlog::error("story: trigger OnActivate faulted id={}", IdOf(ent));
    g_in_apply = false;
}

// Host: OnActivate for a client's fire request, numbered and committed like the host's own fire.
void HostFireNow(void* ent, const net::GameEvent& ev, const char* why) {
    const int16_t id = static_cast<int16_t>(ev.subjectId);
    const int state = TrigInt(ent, kTrigState);
    if (state != 0) SetTrigInt(ent, kTrigState, 0);
    // Fire() calls the original through the trampoline, which skips hk_TriggerFire's NoteFireRan: record it here, or
    // every repeat of the client's request ran OnActivate again (repro 15:59:27 "ran here: false").
    const uint32_t serial = ++g_fire_serial[id];
    g_ran_fire[id] = serial;
    g_fire_wait.erase(id);
    Fire(ent);
    spdlog::info("story: [host] ran '{}' (id {}) for player {} ({}, state was {}) fire #{}", ev.key, id, ev.actorId, why,
                 state, serial);
    events::Commit(events::kTriggerFire, ev.subjectId, serial, 0.f, ev.key, ev.actorId);
}

void OnFireRequest(const net::GameEvent& ev) {
    void* ent = FindEvent(ev);
    if (!ent) {
        spdlog::warn("story: fire request id={} '{}' from player {}: no such trigger here", ev.subjectId, ev.key,
                     ev.actorId);
        return;
    }
    // Whether OnActivate ran *here*, not the +0x1fc flag: the flag can be set without the script having run (the
    // same case ApplyFireCommit handles on clients). 2026-10-17 11:40:15 a client's cScrTask12 found the host's flag
    // set, the host never ran it, the client did, and the story split (two-PC test, both players stuck).
    const int16_t id = static_cast<int16_t>(ev.subjectId);
    const auto ran = g_ran_fire.find(id);
    if (ran != g_ran_fire.end()) {
        spdlog::info("story: trigger '{}' (id {}) fired by player {}: ran here already (fire #{})", ev.key, id,
                     ev.actorId, ran->second);
        events::Commit(events::kTriggerFire, ev.subjectId, ran->second, 0.f, ev.key, ev.actorId);
        return;
    }
    const int state = TrigInt(ent, kTrigState);
    const bool armed = IsActive(ent);
    if (state == 1) {
        HostFireNow(ent, ev, "flag set, OnActivate not run");
        return;
    }
    // audit-b G5: states 2-5 are the host's own sequence under way (fire next frame, delay, BeginCutScene, cutscene).
    // Its fire commits by itself; running OnActivate now ran it twice. With OnHit sync an armed idle trigger is
    // waiting for the host's OnHit (the client's own OnHit became a request), so it waits too.
    const bool pending = (state >= 2 && state <= 5) || (g_sync_onhit && state == 0 && armed);
    if (pending) {
        const uint32_t now = GetTickCount();
        const auto it = g_fire_wait.emplace(id, now).first;
        const uint32_t waited = now - it->second;
        if (waited < g_force_fire_ms) {
            spdlog::info("story: fire request '{}' (id {}) from player {}: host state {}{}, waiting for the host's fire "
                         "({:.0f} s)", ev.key, id, ev.actorId, state, armed ? " armed" : "", waited / 1000.f);
            return;
        }
        // Stuck host sequence (a cutscene that never ends, MAP12C): run it once now, and skip the host's own fire if
        // its sequence still gets there.
        if (state >= 2) g_fired_for_request.insert(id);
        HostFireNow(ent, ev, "forced after the wait");
        return;
    }
    if (!armed && state == 0) {
        // audit-a G6: an alternative the host deactivated (MAP4B 15/16/17, MAP3A water 04/05/06) or never armed.
        spdlog::info("story: fire request '{}' (id {}) from player {} rejected: not armed on the host{}", ev.key, id,
                     ev.actorId, TrigInt(ent, kTrigHitRan) ? " (OnHit ran, no fire scheduled)" : "");
        return;
    }
    HostFireNow(ent, ev, "requested");
}

// ---- trigger OnHit (GameDll 0x6b140): kTriggerHit ----
// A script's OnHit runs only on the process whose activator touched the volume, and the trigger's fire then blocks
// every later OnHit (audit-b G1). The host decides: a client's OnHit becomes a request, the host runs OnHit with its
// own entity of the ActivateBy name and commits it; clients run the script OnHit as the engine does (disarm, script,
// +0x1e8 = 1), so their own state machine plays BeginCutScene, the cutscene, then fires (OnActivate comes from the
// host's fire commit).
safetyhook::InlineHook* g_hit_hook = nullptr;
safetyhook::InlineHook* g_update_hook = nullptr;
using DisarmFn = void(__fastcall*)(void* trigger);
using ScriptCallFn = void(__cdecl*)(void* mgr, void* fn, void* obj);  // class manager vt +0x28 (ADD ESP,0xc after)
using FindEntityFn = void*(__stdcall*)(const char* name);
using TaskIndexFn = int(__thiscall*)(void* mission, int task_id);
using TaskEndedFn = int(__thiscall*)(void* mission, int index);
DisarmFn g_disarm = nullptr;
FindEntityFn g_find_entity = nullptr;
TaskIndexFn g_task_index = nullptr;
TaskEndedFn g_task_ended = nullptr;
uint8_t g_hit_actor = 0;  // host: the player whose request is being run (0 = the host's own touch)
std::unordered_set<int16_t> g_task_end_logged;
void* MissionPtr();

bool guarded_onhit_script(void* trigger) {
    __try {
        char* t = static_cast<char*>(trigger);
        g_disarm(t);
        void* mgr = *g_class_mgr;
        void* fn = *reinterpret_cast<void**>(t + kTrigFnOnHit);
        void* obj = *reinterpret_cast<void**>(t + kTrigScrObj);
        reinterpret_cast<ScriptCallFn>((*reinterpret_cast<void***>(mgr))[0x28 / 4])(mgr, fn, obj);
        *reinterpret_cast<int*>(t + kTrigHitRan) = 1;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void ApplyHitCommit(const net::GameEvent& ev) {
    void* ent = FindEvent(ev);
    if (!ent) {
        spdlog::warn("story: OnHit commit id={} '{}': no such trigger here", ev.subjectId, ev.key);
        return;
    }
    const int16_t id = static_cast<int16_t>(ev.subjectId);
    uint32_t& seen = g_applied_hit[id];
    if (ev.i0 != 0 && ev.i0 <= seen) {
        spdlog::info("story: host OnHit '{}' (id {}) #{} applied already (#{})", ev.key, id, ev.i0, seen);
        return;
    }
    if (ev.i0 != 0) seen = ev.i0;
    const int state = TrigInt(ent, kTrigState);
    if (TrigInt(ent, kTrigHitRan) != 0) {
        spdlog::info("story: host OnHit '{}' (id {}): OnHit ran here already", ev.key, id);
        return;
    }
    if (state == 1) {  // the engine never runs OnHit after the fire
        spdlog::info("story: host OnHit '{}' (id {}): this trigger fired here already; OnHit skipped", ev.key, id);
        return;
    }
    if (!TrigPtr(ent, kTrigFnOnHit) || !g_disarm || !g_class_mgr || !*g_class_mgr) {
        SetTrigInt(ent, kTrigHitRan, 1);
        spdlog::info("story: host OnHit '{}' (id {}): no script OnHit here", ev.key, id);
        return;
    }
    g_in_apply = true;
    const bool ok = guarded_onhit_script(ent);
    g_in_apply = false;
    spdlog::info("story: applied host OnHit '{}' (id {}) #{} ok={} state {} -> {}", ev.key, id, ev.i0, ok, state,
                 TrigInt(ent, kTrigState));
    if (ok && TrigPtr(ent, kTrigFnBeginCut)) g_cut_watch[id] = CutWatch{GetTickCount(), 0, false};
}

void ApplyFireCommit(const net::GameEvent& ev) {
    void* ent = FindEvent(ev);
    if (!ent) {
        spdlog::warn("story: fire commit id={} '{}': no such trigger here", ev.subjectId, ev.key);
        return;
    }
    const int16_t id = static_cast<int16_t>(ev.subjectId);
    uint32_t& seen = g_applied_fire[id];
    if (ev.i0 != 0 && ev.i0 <= seen) {
        spdlog::info("story: host fire '{}' (id {}) #{} applied already (#{})", ev.key, id, ev.i0, seen);
        return;
    }
    if (ev.i0 != 0) seen = ev.i0;
    // Arm first only a trigger this client never armed and never used (its arm commit did not apply here). A trigger
    // that OnHit disarmed stays disarmed after its fire, as on the host.
    if (!IsActive(ent) && TrigInt(ent, kTrigHitRan) == 0 && TrigInt(ent, kTrigState) == 0 &&
        g_applied_arm.find(id) == g_applied_arm.end()) {
        Activate(ent);
    }
    const auto ran = g_ran_fire.find(id);
    if (ran != g_ran_fire.end() && (ran->second == 0 || ran->second >= ev.i0)) {
        ran->second = ev.i0;
        spdlog::info("story: host fire '{}' (id {}) already ran here", ev.key, id);
        return;
    }
    g_ran_fire[id] = ev.i0;
    if (HasFired(ent)) {
        spdlog::info("story: host fire '{}' (id {}) flag={} but OnActivate did not run; clearing it", ev.key, id,
                     TrigInt(ent, kTrigState));
        SetTrigInt(ent, kTrigState, 0);
    }
    Fire(ent);
    spdlog::info("story: applied host fire '{}' (id {}) #{}", ev.key, ev.subjectId, ev.i0);
}

// ---- client: apply the host's arm / OnHit / fire commits in order ----
constexpr uint32_t kOwnSequenceWaitMs = 30000;

const char* KindName(uint16_t kind) {
    switch (kind) {
        case events::kTriggerEvent: return "arm";
        case events::kTriggerHit: return "OnHit";
        case events::kTriggerFire: return "fire";
        default: return "event";
    }
}

bool Queued(uint16_t subject) {
    for (const Deferred& d : g_deferred) {
        if (d.ev.subjectId == subject) return true;
    }
    return false;
}

bool QueuedFire(uint16_t subject) {
    for (const Deferred& d : g_deferred) {
        if (d.ev.subjectId == subject && d.ev.kind == events::kTriggerFire) return true;
    }
    return false;
}

// Why this commit cannot run yet, or null. A cutscene camera holds every OnHit and fire (a fire applied during a
// cutscene's wait replaced its task and the camera never let go, MAP12C). A fire also waits while this trigger's own
// BeginCutScene or cutscene runs here (state 4/5): OnActivate comes after the cutscene, as on the host.
const char* NotReady(const Deferred& d, uint32_t now) {
    if (d.ev.kind == events::kTriggerEvent) return nullptr;
    if (PlayerInCutscene()) return "the local cutscene camera ends";
    if (d.ev.kind != events::kTriggerFire || now - d.since >= kOwnSequenceWaitMs) return nullptr;
    void* ent = FindEvent(d.ev);
    if (!ent || g_ran_fire.count(static_cast<int16_t>(d.ev.subjectId)) != 0) return nullptr;
    const int state = TrigInt(ent, kTrigState);
    if (state == 4) return "its BeginCutScene runs here";
    if (state == 5) return "its cutscene ends here";
    return nullptr;
}

void ApplyStory(const net::GameEvent& ev) {
    if (ev.kind == events::kTriggerEvent) ApplyEventCommit(ev);
    else if (ev.kind == events::kTriggerHit) ApplyHitCommit(ev);
    else if (ev.kind == events::kTriggerFire) ApplyFireCommit(ev);
}

void Submit(const net::GameEvent& ev) {
    const Deferred d{ev, GetTickCount()};
    // Arms run at once unless something of the same trigger waits (keeps arm -> OnHit -> fire in order).
    const bool behind = ev.kind == events::kTriggerEvent ? Queued(ev.subjectId) : !g_deferred.empty();
    const char* why = behind ? "earlier commits apply" : NotReady(d, d.since);
    if (!why) {
        ApplyStory(ev);
        return;
    }
    for (const Deferred& p : g_deferred) {
        if (p.ev.kind == ev.kind && p.ev.subjectId == ev.subjectId && p.ev.i0 == ev.i0 && p.ev.key == ev.key) return;
    }
    g_deferred.push_back(d);
    spdlog::info("story: defer host {} '{}' (id {}) until {}", KindName(ev.kind), ev.key, ev.subjectId, why);
}

void PumpDeferred() {
    const uint32_t now = GetTickCount();
    while (!g_deferred.empty()) {
        if (NotReady(g_deferred.front(), now)) return;
        const net::GameEvent ev = g_deferred.front().ev;
        g_deferred.erase(g_deferred.begin());
        ApplyStory(ev);
    }
}

void OnEventCommit(const net::GameEvent& ev) { Submit(ev); }

void OnHitCommit(const net::GameEvent& ev) {
    // Late join: an old OnHit's BeginCutScene still runs here (mission counts, items, actors), its cutscene is skipped.
    if (events::CatchingUp() && ev.f0 > kCatchupSkipAge) WatchSkip(ev.subjectId, "old OnHit during catch-up");
    Submit(ev);
}

void OnFireCommit(const net::GameEvent& ev) {
    // Late join: the host fired this a while ago, so its cutscene is long over there. Watch before deferring:
    // the cutscene holding the deferral may be this trigger's own (a map-start fire that ran before the join).
    if (events::CatchingUp() && ev.f0 > kCatchupSkipAge) WatchSkip(ev.subjectId, "old fire during catch-up");
    const auto w = g_cut_watch.find(static_cast<int16_t>(ev.subjectId));
    if (w != g_cut_watch.end() && w->second.fire_at == 0) w->second.fire_at = GetTickCount();
    Submit(ev);
}

// Client frame: see CutWatch.
void PumpStuckCutscenes() {
    if (g_cut_watch.empty() || NetIsHost() || !NetIsOnline()) return;
    const uint32_t now = GetTickCount();
    for (auto it = g_cut_watch.begin(); it != g_cut_watch.end();) {
        CutWatch& w = it->second;
        void* ent = EntityById(it->first);
        const int state = ent ? TrigInt(ent, kTrigState) : 0;
        if (state == 4 || state == 5) {
            w.seen = true;
            const bool host_over =
                w.fire_at != 0 && w.fire_at - w.start >= 5000 && now - w.fire_at >= g_stuck_after_fire_ms;
            if (host_over || now - w.start >= g_stuck_max_ms) {
                WatchSkip(static_cast<uint16_t>(it->first),
                          host_over ? "its cutscene still runs after the host's fire" : "its cutscene runs over the time limit");
                it = g_cut_watch.erase(it);
                continue;
            }
        } else if (!ent || w.seen || now - w.start > 5000) {
            it = g_cut_watch.erase(it);  // over, or never started one here
            continue;
        }
        ++it;
    }
}

// A cutscene gathers everyone (user, 2026-10-17): whoever set it off, everyone plays it, and when a client's
// cutscene camera lets go, a client far from the host is moved next to the host.
bool g_was_cutscene = false;
uint32_t g_gather_at = 0;

void PumpGather() {
    if (NetIsHost() || !NetIsOnline()) {
        g_was_cutscene = false;
        g_gather_at = 0;
        return;
    }
    const bool in_cutscene = PlayerInCutscene();
    const uint32_t now = GetTickCount();
    if (g_was_cutscene && !in_cutscene) g_gather_at = now + 1500;  // let the camera and the host's position settle
    g_was_cutscene = in_cutscene;
    if (!g_gather_at || static_cast<int32_t>(now - g_gather_at) < 0) return;
    g_gather_at = 0;
    const float dist = DistanceToHost();
    if (dist > 20.f) {
        spdlog::info("story: cutscene over, host {:.1f} m away: gathering", dist);
        TeleportToHost("after cutscene");
    }
}

// A client trigger waiting for the host calls Fire every frame: one FIRE line per trigger every 2 s is enough.
bool LogFireNow(void* self) {
    static std::unordered_map<void*, uint32_t> last;
    const uint32_t now = GetTickCount();
    auto it = last.find(self);
    if (it != last.end() && now - it->second < 2000) return false;
    last[self] = now;
    return true;
}

void __fastcall hk_TriggerFire(void* self, void*, void* activator) {
    if (g_log && (g_in_apply || NetIsHost() || LogFireNow(self))) {
        spdlog::info("story: [{}] trigger FIRE {} activator={}{}", Role(), Describe(self), Describe(activator),
                     g_in_apply ? " (host commit)" : "");
    }
    if (g_in_apply || !g_sync_events || !NetIsOnline() || !activator) {
        ++g_onactivate_depth;
        g_fire_hook->unsafe_thiscall<void>(self, activator);
        --g_onactivate_depth;
        NoteFireRan(self);
        return;
    }
    const int16_t id = IdOf(self);
    const std::string name = NameOf(self);
    if (NetIsHost()) {
        if (id >= 0 && g_fired_for_request.erase(id) != 0) {
            // audit-b G5: OnActivate already ran for a client's request after the host's sequence stalled.
            SetTrigInt(self, kTrigState, 1);
            spdlog::info("story: [host] '{}' (id {}) own fire skipped: OnActivate ran for a request already", name, id);
            return;
        }
        ++g_onactivate_depth;
        g_fire_hook->unsafe_thiscall<void>(self, activator);
        --g_onactivate_depth;
        const uint32_t serial = NoteFireRan(self);
        g_fire_wait.erase(id);
        if (id >= 0) events::Commit(events::kTriggerFire, static_cast<uint16_t>(id), serial, 0.f, name, 0);
        return;
    }
    // Already decided: the host's fire ran here, or it is waiting behind our cutscene camera.
    if (g_ran_fire.count(id) != 0) {
        // The trigger's update keeps calling Fire every frame while its state (+0x1fc) is still pending (2), which
        // flooded the client log with one FIRE line per frame (MAP4A cScrTask01, audit-b G10). It ran here: done.
        const int state = TrigInt(self, kTrigState);
        if (state != 1) {
            SetTrigInt(self, kTrigState, 1);
            spdlog::info("story: '{}' (id {}) ran here already; trigger state {} -> 1 (stops the per-frame Fire)", name,
                         id, state);
        }
        return;
    }
    if (QueuedFire(static_cast<uint16_t>(id))) return;
    if (id < 0 || !ShouldRequest(events::kTriggerFire, id)) return;
    events::Request(events::kTriggerFire, static_cast<uint16_t>(id), 0, 0.f, name);
    spdlog::info("story: client fire '{}' (id {}) sent to host", name, id);
}

void __fastcall hk_TriggerEvent(void* self, void*) {
    const bool active = IsActive(self);
    if (g_log && !active) {
        spdlog::info("story: [{}] trigger event activate {} caller={}{}", Role(), Describe(self),
                     core::describe_address(_ReturnAddress()), g_in_apply ? " (host commit)" : "");
    }
    if (active || g_in_apply || !g_sync_events || !NetIsOnline()) {
        g_event_hook->unsafe_fastcall<void>(self);
        if (!active && IsActive(self)) NoteArmed(self);
        return;
    }
    const int16_t id = IdOf(self);
    const std::string name = NameOf(self);
    if (NetIsHost()) {
        g_event_hook->unsafe_fastcall<void>(self);
        if (!IsActive(self)) return;
        const uint32_t serial = NoteArmed(self);
        if (id >= 0) events::Commit(events::kTriggerEvent, static_cast<uint16_t>(id), serial, 0.f, name, 0);
        return;
    }
    // Client: its own player or script reached this trigger first. The host decides; the commit activates it here.
    if (id < 0 || !ShouldRequest(events::kTriggerEvent, id)) return;
    events::Request(events::kTriggerEvent, static_cast<uint16_t>(id), 0, 0.f, name);
    spdlog::info("story: client trigger '{}' (id {}) sent to host", name, id);
}

// ---- OnHit hook ----

bool guarded_is_player(void* ent) {
    __try {
        using IsPlayerFn = int(__thiscall*)(void*);
        return reinterpret_cast<IsPlayerFn>((*reinterpret_cast<void***>(ent))[0xfc / 4])(ent) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int guarded_task_ended(void* mission, int task_id) {
    __try {
        const int index = g_task_index(mission, task_id);
        if (index < 0) return 1;  // no such task: the engine lets it pass
        return g_task_ended(mission, index) ? 1 : 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

// Every WaitTask task of this trigger has ended in this process's mission (the gate at the top of 0x6b140).
bool WaitTasksDone(void* trigger) {
    int mask = TrigInt(trigger, kTrigWaitMask);
    void* mission = MissionPtr();
    if (mask <= 0 || !mission || !g_task_index || !g_task_ended) return true;
    for (int task = 0; mask != 0; mask >>= 1, ++task) {
        if ((mask & 1) && guarded_task_ended(mission, task) == 0) return false;
    }
    return true;
}

// The tests of 0x6b140 without running anything: armed, activator name == ActivateBy (an activator without a name
// passes, as in the engine), or a player when there is no ActivateBy, not fired, a script OnHit, OnHit not run yet.
// with_tasks adds the WaitTask gate against this process's mission (the host's; clients leave it to the host).
bool WouldRunOnHit(void* trigger, void* activator, bool with_tasks) {
    if (!trigger || !activator || !IsActive(trigger)) return false;
    if (TrigInt(trigger, kTrigState) == 1 || !TrigPtr(trigger, kTrigFnOnHit) || TrigInt(trigger, kTrigHitRan) != 0)
        return false;
    if (const char* by = static_cast<const char*>(TrigPtr(trigger, kTrigActivateBy))) {
        const char* name = static_cast<const char*>(TrigPtr(activator, kEntTargetName));
        if (name && core::safe_cstr(name, 64) != core::safe_cstr(by, 64)) return false;
    } else if (!guarded_is_player(activator)) {
        return false;
    }
    return !with_tasks || WaitTasksDone(trigger);
}

bool IsTaskEndHit(uintptr_t ret) {
    for (uintptr_t r : kTaskEndHitReturns) {
        if (g_game_base && ret == g_game_base + r) return true;
    }
    return false;
}

// An entity this process does not drive: a mirrored animal or NPC (the host's copy touches the host's volume), or
// another player's puppet (that player reports its own touch).
bool IsRemoteDriven(void* ent) {
    if (puppets::Owns(ent)) return true;
    for (void* a : Animals()) {
        if (a == ent) return true;
    }
    for (void* n : Npcs()) {
        if (n == ent) return true;
    }
    return false;
}

std::string ActivateByOf(void* trigger) {
    const char* by = static_cast<const char*>(TrigPtr(trigger, kTrigActivateBy));
    return by ? core::safe_cstr(by, 48) : std::string();
}

void __fastcall hk_TriggerHit(void* self, void*, void* activator, void* pos, void* dir) {
    const uintptr_t ret = reinterpret_cast<uintptr_t>(_ReturnAddress());
    if (!g_sync_onhit || !g_sync_events || !NetIsOnline() || !self || !activator) {
        g_hit_hook->unsafe_thiscall<void>(self, activator, pos, dir);
        return;
    }
    const int16_t id = IdOf(self);
    const bool task_end = IsTaskEndHit(ret);
    if (NetIsHost()) {
        const bool will = id >= 0 && WouldRunOnHit(self, activator, true);
        const std::string name = will ? NameOf(self) : std::string();
        const uint8_t actor = g_hit_actor;
        g_hit_actor = 0;  // OnHits the script causes in turn are the host's own
        if (will) {
            // Committed before the script runs: OnHits and arms the script causes (a find that ends a task hits the
            // task's trigger) then follow it in the journal, in the order the host ran them.
            const uint32_t serial = ++g_hit_serial[id];
            spdlog::info("story: [host] OnHit '{}' (id {}) by {}{}{} #{}", name, id, Describe(activator),
                         task_end ? " (task end)" : "",
                         actor ? fmt::format(" for player {}", actor) : std::string(), serial);
            events::Commit(events::kTriggerHit, static_cast<uint16_t>(id), serial, 0.f, name, actor);
        }
        g_hit_hook->unsafe_thiscall<void>(self, activator, pos, dir);
        g_hit_actor = actor;
        if (will && TrigInt(self, kTrigHitRan) != 1) {
            spdlog::warn("story: [host] OnHit '{}' (id {}) committed but the engine did not run it", name, id);
        }
        return;
    }
    // Client. A task end is this process's own mission count: the host's mission ends the same task and commits.
    if (task_end) {
        if (WouldRunOnHit(self, activator, false) && g_task_end_logged.insert(id).second) {
            spdlog::info("story: client task-end OnHit '{}' (id {}) left to the host's mission", NameOf(self), id);
        }
        return;
    }
    if (!WouldRunOnHit(self, activator, false)) {
        g_hit_hook->unsafe_thiscall<void>(self, activator, pos, dir);  // runs nothing either: same tests
        return;
    }
    if (IsRemoteDriven(activator) || id < 0 || !ShouldRequest(events::kTriggerHit, id)) return;
    const std::string name = NameOf(self);
    events::Request(events::kTriggerHit, static_cast<uint16_t>(id), 0, 0.f, name);
    spdlog::info("story: client OnHit '{}' (id {}) by {} sent to host", name, id, Describe(activator));
}

bool guarded_hit(void* trigger, void* activator) {
    static const float zero[3] = {0.f, 0.f, 0.f};
    __try {
        hk_TriggerHit(trigger, nullptr, activator, const_cast<float*>(zero), const_cast<float*>(zero));
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void* guarded_find_entity(const char* name) {
    __try {
        return g_find_entity(name);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

// This process's entity the trigger waits for: its ActivateBy name ("hunter", "car1", "helicopter"), else the player.
void* ActivatorFor(void* trigger) {
    const std::string by = ActivateByOf(trigger);
    if (by.empty() || !g_find_entity) return LocalPlayer().entity;
    return guarded_find_entity(by.c_str());
}

// Host: a client's own activator touched the volume. Run OnHit with the host's entity of that name; the engine's
// tests (armed, WaitTask in the host's mission, not fired) decide, and the hook commits.
void OnHitRequest(const net::GameEvent& ev) {
    void* ent = FindEvent(ev);
    if (!ent) {
        spdlog::warn("story: OnHit request id={} '{}' from player {}: no such trigger here", ev.subjectId, ev.key,
                     ev.actorId);
        return;
    }
    const int16_t id = static_cast<int16_t>(ev.subjectId);
    if (TrigInt(ent, kTrigHitRan) != 0) {
        spdlog::info("story: OnHit request '{}' (id {}) from player {}: ran on the host already", ev.key, id,
                     ev.actorId);
        if (const uint32_t serial = SerialOf(g_hit_serial, id)) {
            events::Commit(events::kTriggerHit, ev.subjectId, serial, 0.f, ev.key, ev.actorId);
        }
        return;
    }
    if (!IsActive(ent)) {
        spdlog::info("story: OnHit request '{}' (id {}) from player {} rejected: not armed on the host", ev.key, id,
                     ev.actorId);
        return;
    }
    void* act = ActivatorFor(ent);
    if (!act) {
        spdlog::warn("story: OnHit request '{}' (id {}) from player {}: no host entity '{}'", ev.key, id, ev.actorId,
                     ActivateByOf(ent));
        return;
    }
    g_hit_actor = ev.actorId;
    const bool ok = guarded_hit(ent, act);
    g_hit_actor = 0;
    if (!ok) spdlog::error("story: OnHit '{}' (id {}) faulted on the host", ev.key, id);
    if (TrigInt(ent, kTrigHitRan) == 0) {
        spdlog::info("story: OnHit request '{}' (id {}) from player {}: the host's engine refused it (state {}, "
                     "wait tasks done {})", ev.key, id, ev.actorId, TrigInt(ent, kTrigState), WaitTasksDone(ent));
    }
}

// ---- spawner volumes (audit-b G8) ----
// Trigger Update (0x68800) runs the script UpdateTrigger only while the local player is inside (it sets +0x1f8 for
// that frame). On the host a living puppet inside an armed trigger counts the same, so a client alone in a stampede
// or croc zone gets the host's spawns.
bool InsideBox(void* trigger, void* ent) {
    float mn[3], mx[3], p[3];
    if (!core::safe_read(static_cast<char*>(trigger) + kTrigBoxMin, mn) ||
        !core::safe_read(static_cast<char*>(trigger) + kTrigBoxMax, mx) ||
        !core::safe_read(static_cast<char*>(ent) + kEntPosition, p))
        return false;
    for (int i = 0; i < 3; ++i) {
        if (!(mn[i] <= p[i] && p[i] <= mx[i])) return false;
    }
    return true;
}

std::unordered_set<uint32_t> g_puppet_inside;  // (trigger id << 8 | player) while that puppet is inside

void __fastcall hk_TriggerUpdate(void* self, void*, float dt) {
    if (g_puppet_update && g_sync_events && NetIsHost() && NetIsOnline() && IsActive(self) &&
        TrigPtr(self, kTrigFnUpdate)) {
        const int16_t id = IdOf(self);
        for (uint8_t player : PeerIds()) {
            void* body = puppets::EntityOf(player);
            const uint32_t key = (static_cast<uint32_t>(static_cast<uint16_t>(id)) << 8) | player;
            const bool inside = body && puppets::IsLivingPuppet(body) && InsideBox(self, body);
            if (inside) SetTrigInt(self, kTrigInside, 1);
            if (inside != (g_puppet_inside.count(key) != 0)) {
                if (inside) g_puppet_inside.insert(key);
                else g_puppet_inside.erase(key);
                spdlog::info("story: player {} {} spawner '{}' (id {}): UpdateTrigger {} for them on the host", player,
                             inside ? "inside" : "left", NameOf(self), id, inside ? "runs" : "stops");
            }
        }
    }
    g_update_hook->unsafe_thiscall<void>(self, dt);
}

// cScrEntity::SetAnim. Body-state machines call this when a scripted prop changes pose.
// MAP2A's bridge is fallbridge.SetAnim("broken") from the cutscene; the client never ran that call.
bool IsMirroredActor(void* ent) {
    if (!ent || ent == LocalPlayer().entity || puppets::Owns(ent)) return true;
    for (void* a : Animals()) {
        if (a == ent) return true;
    }
    for (void* n : Npcs()) {
        if (n == ent) return true;
    }
    int16_t parent = -1;
    const LocalPlayerSample& hunter = LocalPlayer();
    if (hunter.valid && core::safe_read(static_cast<char*>(ent) + 0xA4, parent) && parent == hunter.id) return true;
    return false;
}

void OnPropAnimCommit(const net::GameEvent& ev) {
    void* ent = EntityById(ev.subjectId);
    if (!ent || ev.key.empty()) {
        spdlog::warn("story: prop anim id={} '{}': no local entity", ev.subjectId, ev.key);
        return;
    }
    g_in_prop = true;
    const bool ok = entity::SetAnim(ent, ev.key.c_str());
    g_in_prop = false;
    spdlog::info("story: applied prop anim id={} '{}' #{} name='{}' ok={}", ev.subjectId, ev.key, ev.i0, NameOf(ent), ok);
}

// Host: the serial for this SetAnim / PlaySA call, or 0 when it is not a new commit. A different key is new; the
// same key again is new only after a pause (a body state that sets its anim every frame stays one commit).
uint32_t NextAnimSerial(LastAnim& last, const std::string& key, const char* what, int16_t id) {
    const uint32_t now = GetTickCount();
    const bool same = last.key == key;
    const bool continuous = same && last.commits != 0 && now - last.ms < kAnimRepeatGapMs;
    last.ms = now;
    if (continuous) return 0;
    if (last.commits >= kMaxAnimCommits) {
        if (last.commits++ == kMaxAnimCommits) {
            spdlog::warn("story: {} id={} changed {} times on this map; later changes are not sent", what, id,
                         kMaxAnimCommits);
        }
        return 0;
    }
    last.key = key;
    return ++last.commits;
}

// Paired scene anim. MAP2A cutscene 2 plays "cs02" on Hugh with partner "fallbridge"; that call is what
// swaps the bridge mesh. It does not go through SetAnim, and a frozen client brain never starts the task.
void NotePlaySA(void* self, const char* anim, const char* partner) {
    if (g_in_prop || !g_sync_events || !NetIsOnline() || !NetIsHost() || !anim || !anim[0]) return;
    const int16_t id = IdOf(self);
    if (id < 0 || EntityById(id) != self) return;  // same guard as hk_SetAnim
    std::string key = anim;
    if (partner && partner[0]) {
        key.push_back('|');
        key += partner;
    }
    if (key.size() > 31) {
        spdlog::warn("story: PlaySA id={} key too long '{}'", id, key);
        return;
    }
    const uint32_t serial = NextAnimSerial(g_playsa_last[id], key, "PlaySA", id);
    if (!serial) return;
    spdlog::info("story: PlaySA id={} '{}' name='{}' #{}", id, key, NameOf(self), serial);
    events::Commit(events::kPlaySA, static_cast<uint16_t>(id), serial, 0.f, key, 0);
}

void __fastcall hk_PlaySA(void* self, void*, char* anim, char* partner) {
    g_playsa_hook->unsafe_thiscall<void>(self, anim, partner);
    NotePlaySA(self, anim, partner);
}

void OnPlaySACommit(const net::GameEvent& ev) {
    void* ent = EntityById(ev.subjectId);
    if (!ent || ev.key.empty() || !g_playsa_hook) {
        spdlog::warn("story: PlaySA id={} '{}': no local entity", ev.subjectId, ev.key);
        return;
    }
    PlayStrings& kept = g_play_keep[static_cast<int16_t>(ev.subjectId)];
    kept.anim = ev.key;
    kept.partner.clear();
    const size_t bar = kept.anim.find('|');
    if (bar != std::string::npos) {
        kept.partner = kept.anim.substr(bar + 1);
        kept.anim.resize(bar);
    }
    g_in_prop = true;
    g_playsa_hook->unsafe_thiscall<void>(ent, kept.anim.data(),
                                        kept.partner.empty() ? static_cast<char*>(nullptr) : kept.partner.data());
    g_in_prop = false;
    spdlog::info("story: applied PlaySA id={} anim='{}' partner='{}' name='{}'", ev.subjectId, kept.anim, kept.partner,
                 NameOf(ent));
    animals::HoldCutscene(ev.subjectId);
    animals::HoldCutsceneNamed(kept.partner.c_str());
}

void __fastcall hk_SetAnim(void* self, void*, const char* anim) {
    g_setanim_hook->unsafe_thiscall<void>(self, anim);
    if (g_in_prop || !g_sync_events || !NetIsOnline() || !NetIsHost() || !anim || !anim[0]) return;
    if (IsMirroredActor(self)) return;
    const int16_t id = IdOf(self);
    // Cutscene camera objects (csNNcamNN) also come through here without an entity layout: +0x04 is not an id
    // (28477, 4394) and the name read faults. A client could match such a number to an unrelated entity, so only
    // forward calls on an entity of the manager (log sweep 2026-10-17).
    if (id < 0 || EntityById(id) != self) {
        static std::unordered_set<std::string> logged;
        if (logged.insert(anim).second) spdlog::info("story: not forwarding SetAnim '{}' (not a map entity, +4={})", anim, id);
        return;
    }
    const uint32_t serial = NextAnimSerial(g_prop_last[id], anim, "prop anim", id);
    if (!serial) return;
    spdlog::info("story: prop anim id={} '{}' name='{}' #{}", id, anim, NameOf(self), serial);
    events::Commit(events::kPropAnim, static_cast<uint16_t>(id), serial, 0.f, anim, 0);
}

void __fastcall hk_CompleteTask(void* self, void*, int task) {
    if (g_log) {
        spdlog::info("story: [{}] CompleteTask {} caller={}", Role(), task, core::describe_address(_ReturnAddress()));
    }
    g_task_hook->unsafe_thiscall<void>(self, task);
}

// ---- coop_tasks: this process's mission task list (re/notes/triggers.md) ----
// cMission singleton GameDll7 0x1869e8. Task list at +0x1f8: +0x04 last index (-1 when empty, count 0xa5f40),
// +0x10 the cMissionTask* array (element 0xa6080). Task: +0x64 id, +0x68 state (script TASK_NOT_STARTED 1,
// STARTED 2, FINISHED 3 = End pending, PAUSED 4, FAILED 5, HAS_END 6), +0x5c bonus, +0xd4 / +0xd8 trigger entity hit when it ends / fails (cMissionTask::End 0x96da0), +0xf0 species or object to
// find, +0xf4 count still needed (cTaskShoot 0x999a0, cTaskFind 0x99da0).
constexpr uintptr_t kRvaMissionPtr = 0x1869e8;
constexpr uintptr_t kMissionTaskList = 0x1f8;
constexpr uintptr_t kMissionLastAction = 0x1dc;  // UpdateMissionStatus (0x91e80): action, value
constexpr uintptr_t kMissionStatus = 0x254;      // 1 running, 2 success, 3 failed
constexpr uintptr_t kMissionNextTrig = 0x25c;    // char*: NextTrig / forced task-end trigger name
constexpr int kMaxTasks = 64;

void* MissionPtr() {
    void* m = nullptr;
    if (g_game_base) core::safe_read(reinterpret_cast<void*>(g_game_base + kRvaMissionPtr), m);
    return m;
}

const char* TaskType(uintptr_t vt) {
    if (!g_game_base || !vt) return "?";
    switch (vt - g_game_base) {
        case 0x111c0c: return "shoot";
        case 0x111c98: return "find";
        case 0x111d08: return "race";
        case 0x111b54: return "base";
        default: return "?";
    }
}

const char* TaskStateName(int s) {
    switch (s) {
        case 1: return "not started";
        case 2: return "started";
        case 3: return "finished";
        case 4: return "paused";
        case 5: return "failed";
        case 6: return "ended";
        default: return "?";
    }
}

std::string EntityName(void* ent) { return ent ? NameOf(ent) : std::string(); }

void __cdecl cmd_tasks() {
    char* mission = static_cast<char*>(MissionPtr());
    if (!mission) {
        spdlog::info("story: coop_tasks [{}] no cMission on this map", Role());
        return;
    }
    int status = 0, last = -1, action = 0, value = 0;
    void** arr = nullptr;
    char* next = nullptr;
    core::safe_read(mission + kMissionStatus, status);
    core::safe_read(mission + kMissionTaskList + 0x04, last);
    core::safe_read(mission + kMissionTaskList + 0x10, arr);
    core::safe_read(mission + kMissionLastAction, action);
    core::safe_read(mission + kMissionLastAction + 4, value);
    core::safe_read(mission + kMissionNextTrig, next);
    const int count = (last < -1 || last >= kMaxTasks) ? 0 : last + 1;
    spdlog::info("story: coop_tasks [{}] mission status={} tasks={} last action={}/{} next trigger='{}'", Role(), status,
                 count, action, value, next ? core::safe_cstr(next, 48) : std::string());
    for (int i = 0; i < count && arr; ++i) {
        char* task = nullptr;
        if (!core::safe_read(arr + i, task) || !task) continue;
        uintptr_t vt = 0;
        int id = -1, state = 0, bonus = 0, target = 0, left = 0;
        void* on_end = nullptr;
        void* on_fail = nullptr;
        core::safe_read(task, vt);
        core::safe_read(task + 0x64, id);
        core::safe_read(task + 0x68, state);
        core::safe_read(task + 0x5c, bonus);
        core::safe_read(task + 0xd4, on_end);
        core::safe_read(task + 0xd8, on_fail);
        const char* type = TaskType(vt);
        const bool counted = std::strcmp(type, "shoot") == 0 || std::strcmp(type, "find") == 0;
        if (counted) {
            core::safe_read(task + 0xf0, target);
            core::safe_read(task + 0xf4, left);
        }
        spdlog::info("story:   task id={} {} state={} ({}){} bonus={} onEnd='{}' onFail='{}'", id, type, state,
                     TaskStateName(state), counted ? fmt::format(" target={} left={}", target, left) : std::string(),
                     bonus, EntityName(on_end), EntityName(on_fail));
    }
}

bool IsTriggerEvent(void* ent) {
    uintptr_t vt = 0;
    return ent && g_game_base && core::safe_read(ent, vt) && vt == g_game_base + kRvaTriggerVt;
}

void* TriggerNamed(const char* name) {
    for (int id = 0; id < 1024; ++id) {
        void* ent = EntityById(id);
        if (ent && IsTriggerEvent(ent) && NameOf(ent) == name) return ent;
    }
    return nullptr;
}

// coop_triggers: every cObjTriggerEvent with its state, and its box centre in coop_tp units (metres).
void __cdecl cmd_triggers() {
    int n = 0;
    for (int id = 0; id < 1024; ++id) {
        void* ent = EntityById(id);
        if (!IsTriggerEvent(ent)) continue;
        float mn[3] = {}, mx[3] = {};
        core::safe_read(static_cast<char*>(ent) + kTrigBoxMin, mn);
        core::safe_read(static_cast<char*>(ent) + kTrigBoxMax, mx);
        ++n;
        spdlog::info("story:   trigger id={} '{}' armed={} hit={} state={} by='{}' fns={}{}{}{} wait={:#x} "
                     "centre={:.1f}:{:.1f}:{:.1f} size={:.1f}x{:.1f}x{:.1f} ran={} arm#{} hit#{} fire#{}",
                     id, NameOf(ent), TrigInt(ent, kTrigArmed), TrigInt(ent, kTrigHitRan), TrigInt(ent, kTrigState),
                     ActivateByOf(ent), TrigPtr(ent, kTrigFnOnHit) ? "H" : "-", TrigPtr(ent, kTrigFnBeginCut) ? "B" : "-",
                     TrigPtr(ent, 0x20c) ? "A" : "-", TrigPtr(ent, kTrigFnUpdate) ? "U" : "-",
                     TrigInt(ent, kTrigWaitMask), (mn[0] + mx[0]) / 2, mn[1], (mn[2] + mx[2]) / 2, mx[0] - mn[0],
                     mx[1] - mn[1], mx[2] - mn[2], g_ran_fire.count(static_cast<int16_t>(id)) != 0,
                     NetIsHost() ? SerialOf(g_arm_serial, static_cast<int16_t>(id))
                                 : SerialOf(g_applied_arm, static_cast<int16_t>(id)),
                     NetIsHost() ? SerialOf(g_hit_serial, static_cast<int16_t>(id))
                                 : SerialOf(g_applied_hit, static_cast<int16_t>(id)),
                     NetIsHost() ? SerialOf(g_fire_serial, static_cast<int16_t>(id))
                                 : SerialOf(g_applied_fire, static_cast<int16_t>(id)));
    }
    spdlog::info("story: coop_triggers [{}] {} trigger events, {} commits deferred", Role(), n, g_deferred.size());
}

// coop_hit <name>: this process's OnHit for that trigger with its own ActivateBy entity, through the hook (a client
// sends the request, the host runs and commits it). Same path as walking in, without the walk.
void __cdecl cmd_hit(const char* name) {
    void* ent = name ? TriggerNamed(name) : nullptr;
    if (!ent) {
        spdlog::warn("story: coop_hit '{}' not found", name ? name : "");
        return;
    }
    void* act = ActivatorFor(ent);
    spdlog::info("story: coop_hit '{}' (id {}) by {} armed={} hit={} state={}", name, IdOf(ent), Describe(act),
                 TrigInt(ent, kTrigArmed), TrigInt(ent, kTrigHitRan), TrigInt(ent, kTrigState));
    if (!act || !g_hit_hook) return;
    if (!guarded_hit(ent, act)) spdlog::error("story: coop_hit '{}' faulted", name);
}

// ---- hint pages never open in the middle of a cutscene ----
// cModGameMenu::OpenMenuPage (GameDll7 0xbf1b0, cdecl(page), the script native) stores the page and pushes the modal
// "gamemenu", which stops the world until RETURN. MAP1: cScrTask04's box holds cScrTask03's, and 03's cutscene 2 ends
// task 1 (REACHED_POSITION), which arms 04 1.5 s into it: 04's OnActivate opens page 3 over the cutscene. The host's
// world stood still (Hugh's PlayAnim cs02 "never ended": 100 s+ in the triggers test), and RETURN closed the page and,
// being the skip key too, skipped the cutscene for everyone. Solo plays the same way (re/notes/map1-cs2.md). Clients
// already hold the OnHit / fire that opens such a page until their camera is free; the host holds the page itself.
constexpr uintptr_t kRvaOpenMenuPage = 0xbf1b0;
constexpr uintptr_t kRvaMenuOpen = 0x186cf4;  // int, 1 while the game menu / a hint page is up
safetyhook::InlineHook* g_menu_hook = nullptr;
bool g_hold_hints = true;           // [Story] HoldHintsInCutscene
uint32_t g_hold_hints_max_ms = 90000;  // [Story] HoldHintsMaxSec
struct HeldPage {
    int page = 0;
    uint32_t since = 0;
};
std::vector<HeldPage> g_held_pages;

// A trigger's BeginCutScene / cutscene is up here (state 4/5), or the render camera is on its rail.
bool CutsceneRunning() {
    if (PlayerInCutscene()) return true;
    for (int id = 0; id < 1024; ++id) {
        void* ent = EntityById(id);
        if (!IsTriggerEvent(ent)) continue;
        const int state = TrigInt(ent, kTrigState);
        if (state == 4 || state == 5) return true;
    }
    return false;
}

bool guarded_open_page(int page) {
    __try {
        g_menu_hook->unsafe_ccall<void>(page);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void __cdecl hk_OpenMenuPage(int page) {
    if (g_hold_hints && NetIsOnline() && CutsceneRunning()) {
        for (const HeldPage& h : g_held_pages) {
            if (h.page == page) return;
        }
        g_held_pages.push_back({page, GetTickCount()});
        spdlog::info("story: [{}] hint page {} held until the cutscene is over (caller {})", Role(), page,
                     core::describe_address(_ReturnAddress()));
        return;
    }
    g_menu_hook->unsafe_ccall<void>(page);
}

// One held page at a time, once the cutscene is over and no menu is up (a second push of the modal would stack).
void PumpHeldHints() {
    if (g_held_pages.empty() || !g_menu_hook || !g_game_base) return;
    int open = 0;
    if (core::safe_read(reinterpret_cast<char*>(g_game_base) + kRvaMenuOpen, open) && open != 0) return;
    const uint32_t now = GetTickCount();
    const bool timed_out = now - g_held_pages.front().since >= g_hold_hints_max_ms;
    if (!timed_out && CutsceneRunning()) return;
    const HeldPage held = g_held_pages.front();
    g_held_pages.erase(g_held_pages.begin());
    const bool ok = guarded_open_page(held.page);
    spdlog::info("story: hint page {} opened after {:.1f} s{} ok={}", held.page, (now - held.since) / 1000.f,
                 timed_out ? " (time limit)" : "", ok);
}

bool g_cmd_tasks = false;

}  // namespace

// Test hook (console coop_trigger <name>): activate a cObjTriggerEvent by name through the normal hook path.
void DebugActivate(const char* name) {
    for (int id = 0; id < 1024; ++id) {
        void* ent = EntityById(id);
        if (!ent || NameOf(ent) != name) continue;
        void* vt = nullptr;
        core::safe_read(ent, vt);
        const bool armed = IsActive(ent);
        spdlog::info("story: coop_trigger '{}' -> id {} vt={} armed={} fired={} -> {}", name, id,
                     core::describe_address(vt), armed, HasFired(ent), armed ? "fire" : "arm");
        if (!armed) hk_TriggerEvent(ent, nullptr);
        else hk_TriggerFire(ent, nullptr, LocalPlayer().entity ? LocalPlayer().entity : ent);
        return;
    }
    spdlog::warn("story: coop_trigger '{}' not found", name);
}

// Watched skips are not counted: a trigger without a cutscene stays watched until it expires.
bool CatchupBusy() { return !g_deferred.empty() || PlayerInCutscene(); }

namespace {

void* CheckedGameRva(HMODULE game, uintptr_t rva, const char* label) {
    static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
    uint8_t got[5] = {};
    void* p = reinterpret_cast<char*>(game) + rva;
    if (!core::safe_copy(p, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
        spdlog::error("story: {} at GameDll7+{:#x} has unexpected bytes; not hooked", label, rva);
        return nullptr;
    }
    return p;
}

class StoryModule : public dh2::Module {
public:
    const char* Name() const override { return "story"; }
    int Priority() const override { return 47; }

    void OnMapLoaded(const char*) override {
        g_ran_fire.clear();
        g_deferred.clear();
        g_prop_last.clear();
        g_playsa_last.clear();
        g_play_keep.clear();
        g_skip_watch.clear();
        g_cut_watch.clear();
        g_held_pages.clear();
        g_requested.clear();
        for (auto* m : {&g_arm_serial, &g_hit_serial, &g_fire_serial, &g_applied_arm, &g_applied_hit, &g_applied_fire,
                        &g_fire_wait}) {
            m->clear();
        }
        g_fired_for_request.clear();
        g_puppet_inside.clear();
        g_task_end_logged.clear();
    }

    void OnFrame(float) override {
        if (!g_cmd_tasks && console_cmds::Ready()) {
            g_cmd_tasks = console_cmds::AddCommand0("coop_tasks", &cmd_tasks);
            console_cmds::AddCommand0("coop_triggers", &cmd_triggers);
            console_cmds::AddCommandStr("coop_hit", &cmd_hit);
        }
        PumpSkips();
        PumpStuckCutscenes();
        PumpDeferred();
        PumpGather();
        PumpHeldHints();
    }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        g_log = GetPrivateProfileIntA("Story", "LogTriggers", 1, ini.c_str()) != 0;
        g_sync_events = GetPrivateProfileIntA("Story", "SyncTriggerEvents", 1, ini.c_str()) != 0;
        g_sync_onhit = GetPrivateProfileIntA("Story", "SyncOnHit", 1, ini.c_str()) != 0;
        g_puppet_update = GetPrivateProfileIntA("Story", "PuppetUpdateTrigger", 1, ini.c_str()) != 0;
        g_force_fire_ms = 1000u * static_cast<uint32_t>(GetPrivateProfileIntA("Story", "FireRequestForceSec", 45,
                                                                               ini.c_str()));
        g_stuck_after_fire_ms =
            1000u * static_cast<uint32_t>(GetPrivateProfileIntA("Story", "StuckCutsceneSec", 20, ini.c_str()));
        g_stuck_max_ms =
            1000u * static_cast<uint32_t>(GetPrivateProfileIntA("Story", "StuckCutsceneMaxSec", 90, ini.c_str()));
        g_hold_hints = GetPrivateProfileIntA("Story", "HoldHintsInCutscene", 1, ini.c_str()) != 0;
        g_hold_hints_max_ms =
            1000u * static_cast<uint32_t>(GetPrivateProfileIntA("Story", "HoldHintsMaxSec", 90, ini.c_str()));
        spdlog::info("story: SyncTriggerEvents={} SyncOnHit={} PuppetUpdateTrigger={} FireRequestForceSec={}",
                     g_sync_events, g_sync_onhit, g_puppet_update, g_force_fire_ms / 1000);
        events::SetHandler(events::kTriggerEvent, &OnEventRequest, &OnEventCommit);
        events::SetHandler(events::kTriggerFire, &OnFireRequest, &OnFireCommit);
        events::SetHandler(events::kTriggerHit, &OnHitRequest, &OnHitCommit);
        events::SetHandler(events::kPropAnim, nullptr, &OnPropAnimCommit);
        events::SetHandler(events::kPlaySA, nullptr, &OnPlaySACommit);
        events::SetHandler(events::kCutsceneSkip, nullptr, &OnSkipCommit);
    }

    void OnEngineReady(HMODULE engine) override {
        if (engine) {
            g_get_record = reinterpret_cast<GetRecordFn>(
                core::find_export(engine, "?GetObjectA@cClassManager@@QAEPAVcObjRecord@@H@Z"));
            g_class_mgr = reinterpret_cast<void**>(core::find_export(engine, "?g_pClassManager@@3PAVcClassManager@@A"));
        }
        if (HMODULE common = core::wait_for_module("CommonDll7.dll", 10000)) {
            g_get_scr = reinterpret_cast<GetScrObjectFn>(
                core::find_export(common, "?GetScrObject@cObjScriptedEntity@@QAEPAVcScrEntity@@XZ"));
            g_volume_hook = core::install_hook(
                "cObjTrigger::OnActivate (story log)",
                core::find_export(common, "?OnActivate@cObjTrigger@@UAEXPAVcObjScriptedEntity@@@Z"),
                reinterpret_cast<void*>(&hk_VolumeActivate));
            g_setanim_hook = core::install_hook(
                "cScrEntity::SetAnim",
                core::find_export(common, "?SetAnim@cScrEntity@@UAEXPBD@Z"),
                reinterpret_cast<void*>(&hk_SetAnim));
            // cScrCharacterEx::PlaySA is a thunk to this body. The body looks the partner up by name and plays the
            // clip on both the caller and the partner (MAP2A: Hugh + fallbridge, and the same pattern on other maps).
            g_playsa_hook = core::install_hook(
                "cObjCharacterEx::PlaySA",
                core::find_export(common, "?PlaySA@cObjCharacterEx@@UAEXPAD0@Z"),
                reinterpret_cast<void*>(&hk_PlaySA));
        }
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_event_hook = core::install_hook("cObjTriggerEvent activate (0x68eb0)",
                                          CheckedGameRva(game, kRvaTriggerEvent, "trigger event"),
                                          reinterpret_cast<void*>(&hk_TriggerEvent));
        g_fire_hook = core::install_hook("cObjTriggerEvent::OnActivate (0x68130)",
                                         CheckedGameRva(game, kRvaTriggerFire, "trigger fire"),
                                         reinterpret_cast<void*>(&hk_TriggerFire));
        g_task_hook = core::install_hook("cMission::CompleteTask (0x95840)",
                                         CheckedGameRva(game, kRvaCompleteTask, "complete task"),
                                         reinterpret_cast<void*>(&hk_CompleteTask));
        g_game_base = reinterpret_cast<uintptr_t>(game);
        g_disarm = reinterpret_cast<DisarmFn>(CheckedGameRva(game, kRvaTriggerDisarm, "trigger disarm"));
        g_find_entity = reinterpret_cast<FindEntityFn>(CheckedGameRva(game, kRvaFindEntity, "find entity"));
        g_task_index = reinterpret_cast<TaskIndexFn>(CheckedGameRva(game, kRvaTaskIndex, "task index"));
        g_task_ended = reinterpret_cast<TaskEndedFn>(CheckedGameRva(game, kRvaTaskEnded, "task ended"));
        // OnHit needs the disarm and the class manager to apply a host commit; without them clients keep the old path.
        if (g_disarm && g_class_mgr) {
            g_hit_hook = core::install_hook("cObjTriggerEvent::OnHit (0x6b140)",
                                            CheckedGameRva(game, kRvaTriggerHit, "trigger OnHit"),
                                            reinterpret_cast<void*>(&hk_TriggerHit));
        }
        if (!g_hit_hook) g_sync_onhit = false;
        g_update_hook = core::install_hook("cObjTriggerEvent::Update (0x68800)",
                                           CheckedGameRva(game, kRvaTriggerUpdate, "trigger update"),
                                           reinterpret_cast<void*>(&hk_TriggerUpdate));
        g_skip_hook = core::install_hook("cObjTriggerEvent skip cutscene (0x6b350)",
                                         CheckedGameRva(game, kRvaTriggerSkip, "trigger skip"),
                                         reinterpret_cast<void*>(&hk_TriggerSkip));
        g_skip_input_hook = core::install_hook("skip_cutscene key callback (0x6b600)",
                                               CheckedGameRva(game, kRvaSkipInput, "skip key"),
                                               reinterpret_cast<void*>(&hk_SkipInput));
        if (g_hold_hints) {
            g_menu_hook = core::install_hook("cModGameMenu::OpenMenuPage (0xbf1b0)",
                                             CheckedGameRva(game, kRvaOpenMenuPage, "OpenMenuPage"),
                                             reinterpret_cast<void*>(&hk_OpenMenuPage));
        }
        spdlog::info("story: skip path scr={} record={} mgr={}", reinterpret_cast<void*>(g_get_scr),
                     reinterpret_cast<void*>(g_get_record), reinterpret_cast<void*>(g_class_mgr));
    }
};

DH2_MODULE(StoryModule)

}  // namespace
}  // namespace dh2::coop::story
