// Story mode, step S1: observe the decision points of a story map (re/notes/lead-story-coop.md).
// Logs L1 trigger-volume activations (cObjTrigger::OnActivate), L2 script trigger events (GameDll 0x68eb0)
// and mission task completion (cMission::CompleteTask 0x95840) on every process. No behavior change yet.
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

safetyhook::InlineHook* g_volume_hook = nullptr;
safetyhook::InlineHook* g_event_hook = nullptr;
safetyhook::InlineHook* g_task_hook = nullptr;
safetyhook::InlineHook* g_setanim_hook = nullptr;
safetyhook::InlineHook* g_playsa_hook = nullptr;
bool g_in_prop = false;
std::unordered_map<int16_t, std::string> g_prop_anim;
std::unordered_map<int16_t, std::string> g_playsa;
// Kept for the life of the map: PlaySA stores the anim name pointer on the partner mesh.
struct PlayStrings {
    std::string anim;
    std::string partner;
};
std::unordered_map<int16_t, PlayStrings> g_play_keep;
bool g_log = true;
// Triggers whose OnActivate (0x68130) this process actually ran. The +0x1fc flag can be set
// without that call (MAP12C cScrTask01, 2026-10-16 22:45): the host fire was then skipped and
// the client stayed in the opening cutscene camera.
std::unordered_set<int16_t> g_ran_fire;
std::vector<net::GameEvent> g_deferred_fires;

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

void NoteFireRan(void* ent) {
    const int16_t id = IdOf(ent);
    if (id >= 0) g_ran_fire.insert(id);
}

std::string NameOf(void* ent) {
    char* name = nullptr;
    core::safe_read(static_cast<char*>(ent) + kEntTargetName, name);
    return name ? core::safe_cstr(name, 48) : std::string();
}

bool IsActive(void* ent) {
    int active = -1;
    core::safe_read(static_cast<char*>(ent) + 0x1e4, active);
    return active == 1;
}

void Activate(void* ent) {
    g_in_apply = true;
    g_event_hook->unsafe_fastcall<void>(ent);
    g_in_apply = false;
}

// The same BSP entity id must carry the same name on both sides; otherwise the maps differ and nothing is applied.
void* FindEvent(const net::GameEvent& ev) {
    void* ent = EntityById(ev.subjectId);
    if (!ent) return nullptr;
    return NameOf(ent) == ev.key ? ent : nullptr;
}

void OnEventRequest(const net::GameEvent& ev) {
    void* ent = FindEvent(ev);
    if (!ent) {
        spdlog::warn("story: trigger request id={} '{}' from player {}: no such trigger here", ev.subjectId, ev.key,
                     ev.actorId);
        return;
    }
    if (!IsActive(ent)) Activate(ent);
    spdlog::info("story: trigger '{}' (id {}) requested by player {} -> active", ev.key, ev.subjectId, ev.actorId);
    events::Commit(events::kTriggerEvent, ev.subjectId, 0, 0.f, ev.key, ev.actorId);
}

void OnEventCommit(const net::GameEvent& ev) {
    void* ent = FindEvent(ev);
    if (!ent) {
        spdlog::warn("story: trigger commit id={} '{}': no such trigger here", ev.subjectId, ev.key);
        return;
    }
    if (IsActive(ent)) return;
    Activate(ent);
    spdlog::info("story: applied host trigger '{}' (id {})", ev.key, ev.subjectId);
}

// ---- trigger firing (cObjTriggerEvent::OnActivate, GameDll 0x68130): kTriggerFire ----
safetyhook::InlineHook* g_fire_hook = nullptr;

bool HasFired(void* ent) {
    int fired = 0;
    core::safe_read(static_cast<char*>(ent) + 0x1fc, fired);
    return fired != 0;
}

void Fire(void* ent) {
    void* activator = LocalPlayer().entity;  // the script callback ignores it; the engine only needs non-null
    if (!activator) activator = ent;
    g_in_apply = true;
    g_fire_hook->unsafe_thiscall<void>(ent, activator);
    g_in_apply = false;
}

void OnFireRequest(const net::GameEvent& ev) {
    void* ent = FindEvent(ev);
    if (!ent) {
        spdlog::warn("story: fire request id={} '{}' from player {}: no such trigger here", ev.subjectId, ev.key,
                     ev.actorId);
        return;
    }
    if (!HasFired(ent)) Fire(ent);
    spdlog::info("story: trigger '{}' (id {}) fired by player {}", ev.key, ev.subjectId, ev.actorId);
    events::Commit(events::kTriggerFire, ev.subjectId, 0, 0.f, ev.key, ev.actorId);
}

void ApplyFireCommit(const net::GameEvent& ev) {
    void* ent = FindEvent(ev);
    if (!ent) {
        spdlog::warn("story: fire commit id={} '{}': no such trigger here", ev.subjectId, ev.key);
        return;
    }
    if (!IsActive(ent)) Activate(ent);  // arm first if this client missed the arming
    const int16_t id = static_cast<int16_t>(ev.subjectId);
    if (g_ran_fire.count(id) != 0) {
        spdlog::info("story: host fire '{}' (id {}) already ran here", ev.key, id);
        return;
    }
    g_ran_fire.insert(id);
    if (HasFired(ent)) {
        int flag = 0;
        core::safe_read(static_cast<char*>(ent) + 0x1fc, flag);
        spdlog::info("story: host fire '{}' (id {}) flag={} but OnActivate did not run; clearing it", ev.key, id, flag);
        const int zero = 0;
        std::memcpy(static_cast<char*>(ent) + 0x1fc, &zero, sizeof zero);
    }
    Fire(ent);
    spdlog::info("story: applied host fire '{}' (id {})", ev.key, ev.subjectId);
}

void OnFireCommit(const net::GameEvent& ev) {
    if (PlayerInCutscene()) {
        for (const net::GameEvent& pending : g_deferred_fires) {
            if (pending.subjectId == ev.subjectId && pending.key == ev.key) return;
        }
        g_deferred_fires.push_back(ev);
        spdlog::info("story: defer host fire '{}' (id {}) until the local cutscene camera ends", ev.key,
                     ev.subjectId);
        return;
    }
    ApplyFireCommit(ev);
}

void PumpDeferredFires() {
    if (g_deferred_fires.empty() || PlayerInCutscene()) return;
    const std::vector<net::GameEvent> pending = std::move(g_deferred_fires);
    g_deferred_fires.clear();
    for (const net::GameEvent& ev : pending) ApplyFireCommit(ev);
}

void __fastcall hk_TriggerFire(void* self, void*, void* activator) {
    if (g_log) {
        spdlog::info("story: [{}] trigger FIRE {} activator={}{}", Role(), Describe(self), Describe(activator),
                     g_in_apply ? " (host commit)" : "");
    }
    if (g_in_apply || !g_sync_events || !NetIsOnline() || !activator) {
        g_fire_hook->unsafe_thiscall<void>(self, activator);
        NoteFireRan(self);
        return;
    }
    const int16_t id = IdOf(self);
    const std::string name = NameOf(self);
    if (NetIsHost()) {
        g_fire_hook->unsafe_thiscall<void>(self, activator);
        NoteFireRan(self);
        if (id >= 0) events::Commit(events::kTriggerFire, static_cast<uint16_t>(id), 0, 0.f, name, 0);
        return;
    }
    if (id >= 0) events::Request(events::kTriggerFire, static_cast<uint16_t>(id), 0, 0.f, name);
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
        return;
    }
    const int16_t id = IdOf(self);
    const std::string name = NameOf(self);
    if (NetIsHost()) {
        g_event_hook->unsafe_fastcall<void>(self);
        if (id >= 0) events::Commit(events::kTriggerEvent, static_cast<uint16_t>(id), 0, 0.f, name, 0);
        return;
    }
    // Client: its own player or script reached this trigger first. The host decides; the commit activates it here.
    if (id >= 0) events::Request(events::kTriggerEvent, static_cast<uint16_t>(id), 0, 0.f, name);
    spdlog::info("story: client trigger '{}' (id {}) sent to host", name, id);
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
    g_prop_anim[static_cast<int16_t>(ev.subjectId)] = ev.key;
    spdlog::info("story: applied prop anim id={} '{}' name='{}' ok={}", ev.subjectId, ev.key, NameOf(ent), ok);
}

// Paired scene anim. MAP2A cutscene 2 plays "cs02" on Hugh with partner "fallbridge"; that call is what
// swaps the bridge mesh. It does not go through SetAnim, and a frozen client brain never starts the task.
void NotePlaySA(void* self, const char* anim, const char* partner) {
    if (g_in_prop || !g_sync_events || !NetIsOnline() || !NetIsHost() || !anim || !anim[0]) return;
    const int16_t id = IdOf(self);
    if (id < 0) return;
    std::string key = anim;
    if (partner && partner[0]) {
        key.push_back('|');
        key += partner;
    }
    if (key.size() > 31) {
        spdlog::warn("story: PlaySA id={} key too long '{}'", id, key);
        return;
    }
    const auto it = g_playsa.find(id);
    if (it != g_playsa.end() && it->second == key) return;
    g_playsa[id] = key;
    spdlog::info("story: PlaySA id={} '{}' name='{}'", id, key, NameOf(self));
    events::Commit(events::kPlaySA, static_cast<uint16_t>(id), 0, 0.f, key, 0);
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
    if (id < 0) return;
    const auto it = g_prop_anim.find(id);
    if (it != g_prop_anim.end() && it->second == anim) return;
    g_prop_anim[id] = anim;
    spdlog::info("story: prop anim id={} '{}' name='{}'", id, anim, NameOf(self));
    events::Commit(events::kPropAnim, static_cast<uint16_t>(id), 0, 0.f, anim, 0);
}

void __fastcall hk_CompleteTask(void* self, void*, int task) {
    if (g_log) {
        spdlog::info("story: [{}] CompleteTask {} caller={}", Role(), task, core::describe_address(_ReturnAddress()));
    }
    g_task_hook->unsafe_thiscall<void>(self, task);
}

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
        g_deferred_fires.clear();
        g_prop_anim.clear();
        g_playsa.clear();
        g_play_keep.clear();
    }

    void OnFrame(float) override { PumpDeferredFires(); }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        g_log = GetPrivateProfileIntA("Story", "LogTriggers", 1, ini.c_str()) != 0;
        g_sync_events = GetPrivateProfileIntA("Story", "SyncTriggerEvents", 1, ini.c_str()) != 0;
        events::SetHandler(events::kTriggerEvent, &OnEventRequest, &OnEventCommit);
        events::SetHandler(events::kTriggerFire, &OnFireRequest, &OnFireCommit);
        events::SetHandler(events::kPropAnim, nullptr, &OnPropAnimCommit);
        events::SetHandler(events::kPlaySA, nullptr, &OnPlaySACommit);
    }

    void OnEngineReady(HMODULE) override {
        if (HMODULE common = core::wait_for_module("CommonDll7.dll", 10000)) {
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
    }
};

DH2_MODULE(StoryModule)

}  // namespace
}  // namespace dh2::coop::story
