// Phase 4 shared animals.
// Host (session active, ≥1 client): every 50 ms, AnimalStates from the tracker list.
// Client: FreezeBrain, keep physics, and in the pre-render callback HandleKey position/angle
// (map-load path: entity + physics body). OnFrame placement flickers. UnlinkPhysics hides animals.
// [Animals] Spike=1 remains a solo experiment and is skipped while a session is active.
#include "coop/animals.h"

#include "coop/events.h"

#include <windows.h>
#include <intrin.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "coop/coop.h"
#include "coop/player_tracker.h"
#include "coop/puppets.h"
#include "coop/scale.h"
#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/console_cmds.h"
#include "engine/entity_api.h"

namespace dh2::coop::combat {
void OnClientHitAnimal(void* animal, uint8_t shooter);  // combat.cpp
bool ForwardPuppetHurt(void* self, void* attacker, float dmg, int type);  // combat.cpp: true = it was a puppet
}

namespace dh2::coop::animals {
namespace {

constexpr uintptr_t kEntId = 0x04;
constexpr uintptr_t kEntPos = 0xE8;
constexpr uintptr_t kEntAngle = 0xF4;
constexpr uintptr_t kEntParent = 0xA4;  // int16 parent entity id, -1 = free in the world
constexpr uintptr_t kEntWorldBox = 0xC4;
constexpr uintptr_t kEntHealth = 0x19C;
// cObjCobra (re/notes/snake.md). +0x728 null makes Update return before the bite and the death timer.
// +0x738 = 50.0f (bytes 0x42480000 in FUN_10080440) is the bullet OnHit timer; Update calls vt +0x154 once it crosses 0.
constexpr uintptr_t kVtCobraRva = 0x10e920;
constexpr uintptr_t kCobraBite = 0x728;
constexpr uintptr_t kCobraTimer = 0x738;
constexpr float kCobraDeathTimer = 50.f;
constexpr float kYawMoveM = 0.05f;
constexpr uint32_t kInterpDelayMs = 100;
constexpr uint32_t kInterpHistoryMs = 1000;
constexpr int kTeleportRepeats = 3;

class AnimalsModule;
extern AnimalsModule* g_self;
void prerender();

safetyhook::InlineHook* g_draw_hook = nullptr;
std::atomic<void*> g_watch{nullptr};
std::atomic<uint32_t> g_draws{0};
void __fastcall hk_Draw(void* self, void* /*edx*/) {
    if (self == g_watch.load(std::memory_order_relaxed)) g_draws.fetch_add(1, std::memory_order_relaxed);
    g_draw_hook->unsafe_thiscall<void>(self);
}

// Hit-path diagnostics ([Animals] HitDiag=1): which engine call a real bullet takes into an animal. The client's own
// shots at a mirrored deer produced no health drop in the 20:32 user test, so log both candidate entries.
safetyhook::InlineHook* g_inflict_hook = nullptr;
safetyhook::InlineHook* g_onhurt_hook = nullptr;
std::atomic<int> g_hit_diag_left{40};

void __cdecl hk_FuncInflictDamage(void* animal, float dmg) {
    float before = 0.f;
    core::safe_read(static_cast<char*>(animal) + 0x19C, before);
    g_inflict_hook->unsafe_ccall<void>(animal, dmg);
    if (g_hit_diag_left.fetch_sub(1) > 0) {
        int16_t id = -1;
        core::safe_read(static_cast<char*>(animal) + 0x04, id);
        spdlog::info("animals: diag FuncInflictDamage id={} dmg={:.2f} hp {:.1f}-> caller={}", id, dmg, before,
                     core::describe_address(_ReturnAddress()));
    }
}

bool g_hit_diag = true;  // [Animals] HitDiag: log the first 40 hits

void __fastcall hk_OnHurt(void* self, void*, void* attacker, float dmg, int type, const void* pos, const void* dir) {
    // An animal's attack on a puppet lands here (target vt +0x14C on obj_humanEx is this base, not
    // cObjCharacter::OnHurt; 2026-10-17 11:11 the puppet's own hp went 100 -> 84). It belongs to that player.
    if (combat::ForwardPuppetHurt(self, attacker, dmg, type)) return;
    if (g_hit_diag && g_hit_diag_left.fetch_sub(1) > 0) {
        int16_t id = -1;
        uint8_t kind = 0;
        float hp = 0.f;
        core::safe_read(static_cast<char*>(self) + 0x04, id);
        core::safe_read(static_cast<char*>(self) + 0xA7, kind);
        core::safe_read(static_cast<char*>(self) + 0x19C, hp);
        spdlog::info("animals: diag OnHurt id={} kind={:#x} dmg={:.2f} type={} hp={:.1f} attacker={:#x} caller={}", id,
                     kind, dmg, type, hp, reinterpret_cast<uintptr_t>(attacker), core::describe_address(_ReturnAddress()));
    }
    g_onhurt_hook->unsafe_thiscall<void>(self, attacker, dmg, type, pos, dir);
}

// Story NPC health (re/notes/npc-health.md): cObjNPC / cObjGuide keep a cProfile at +0x1F8; current health is the
// float at profile +0x54. cObjNPC::Update (GameDll 0x630a0, DecideCurentCondition 0x62ac0) copies it onto +0x19C
// every frame; the protect-objective bar (cHUDNpcStatus) and the death check read the profile value.
// Only cObjNPC and its subclasses keep that profile (review L1: any other /npc/ model uses +0x19C). They are the
// classes whose vt +0x154 is cObjNPC::OnDie (GameDll 0x5fde0): cObjNPC 0x108270, cObjGuide 0x108a50, cObjGirl
// 0x108d38 and two more (0x109018, 0x109308; audit-maps-c "Other facts").
constexpr uintptr_t kRvaNpcOnDie = 0x5fde0;

bool HasNpcProfile(void* npc) {
    HMODULE game = GetModuleHandleA("GameDll7.dll");
    char* vt = nullptr;
    void* on_die = nullptr;
    if (!npc || !game || !core::safe_read(npc, vt) || !vt || !core::safe_read(vt + 0x154, on_die)) return false;
    if (on_die == reinterpret_cast<char*>(game) + kRvaNpcOnDie) return true;
    static std::set<uintptr_t> logged;
    if (logged.insert(reinterpret_cast<uintptr_t>(vt)).second)
        spdlog::info("animals: npc vt {} is not a cObjNPC; its health stays on +0x19C", core::describe_address(vt));
    return false;
}

bool guarded_store_float(float* dst, float v) {
    __try {
        *dst = v;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool NpcProfileHealth(void* npc, float& hp) {
    if (!HasNpcProfile(npc)) return core::safe_read(static_cast<char*>(npc) + 0x19C, hp);
    char* profile = nullptr;
    return core::safe_read(static_cast<char*>(npc) + 0x1F8, profile) && profile &&
           core::safe_read(profile + 0x54, hp);
}

void SetNpcProfileHealth(void* npc, float hp) {
    char* profile = nullptr;
    if (HasNpcProfile(npc) && core::safe_read(static_cast<char*>(npc) + 0x1F8, profile) && profile)
        guarded_store_float(reinterpret_cast<float*>(profile + 0x54), hp);
    else
        guarded_store_float(reinterpret_cast<float*>(static_cast<char*>(npc) + 0x19C), hp);
}

// Client: a mirrored NPC takes no local damage (cObjNPC::OnHurt GameDll 0x5eeb0, vt +0x14C): the host's health
// arrives every frame. Two-PC test: Hugh's bar dropped at different rates on host and client.
safetyhook::InlineHook* g_npc_hurt_hook = nullptr;
safetyhook::InlineHook* g_movetopos_hook = nullptr;
int __fastcall hk_MoveToPos(void* self, void*, int mode, int arg);  // below (needs the module's mirror table)
bool IsMirroredNpc(void* ent);  // below (needs the module's mirror table)

// ---- kill credit, lives and mission reports (re/notes/life.md) ----
// GameDll7 v1.0 RVAs; every body starts 55 8b ec 6a ff (checked before hooking).
constexpr uintptr_t kRvaAnimalOnHit = 0x1a680;    // cScrAnimal::OnHit (vt +0x8c), thiscall(animal, bullet, pos*, dir*)
constexpr uintptr_t kRvaPresentDeath = 0x1e8b0;   // cScrAnimal vt +0x154, fastcall(animal)
constexpr uintptr_t kRvaMissionReport = 0x91e80;  // thiscall(cMission*, int action, int species): records one report
constexpr uintptr_t kRvaMissionUpdate = 0x94ba0;  // cMission vt +0x44 Update(float dt): hands the report to the tasks
constexpr uintptr_t kRvaMissionFail = 0x965e0;    // thiscall(cMission*, const char*): status 3, then 0x91c20(6, text)
constexpr uintptr_t kRvaMission = 0x1869e8;       // cMission* (null outside a mission)
// cScrAnimal fields (decompiles of 0x1a680, 0x1e8b0, 0x14610, 0x1f3c0).
constexpr uintptr_t kEntKillAction = 0x420;   // MISSION_ACTION_* the death reports; the bullet OnHit writes 1, death clears
constexpr uintptr_t kEntSpecies = 0x434;
constexpr uintptr_t kEntHitBone = 0x3d8;      // hit bone of the bullet being handled (-1: no bone, OnHit does nothing)
constexpr uintptr_t kEntHitBits = 0x4d8;      // OnHit ORs 8 for a shot by a non-player, else 1/2/4 by bullet kind
constexpr uintptr_t kEntTranq = 0x4f0;        // 2 = tranquilized (Update counts +0x4f4 down, then present-death)
constexpr uintptr_t kEntTranqHurt = 0x4f8;    // damage of one dart
constexpr uintptr_t kEntEventMode = 0x500;    // 2 = event animal (MoveToPos mode 2): its death is not reported
constexpr uintptr_t kEntDeadGuard = 0x600;    // 1 after present-death or a flee; MoveToPos clears it
constexpr uintptr_t kEntProtected = 0x604;    // IsProtected(): a bullet fails the mission, a second dart overdoses
constexpr uintptr_t kBulletKind = 0x1b4;      // mod_obj_bullet: 2 = melee (knife), no protected-animal check
constexpr uintptr_t kMissionReportAction = 0x1dc;  // the report waiting for the next Update (0 = none)
constexpr uintptr_t kMissionFleeAction = 0x1ec;    // SetMissionOnFleePointReachedAction
constexpr int kActionKill = 1;               // MISSION_ACTION_KILL

safetyhook::InlineHook* g_onhit_hook = nullptr;
safetyhook::InlineHook* g_death_hook = nullptr;
safetyhook::InlineHook* g_report_hook = nullptr;
safetyhook::InlineHook* g_mission_update_hook = nullptr;
void* g_fn_mission_fail = nullptr;
int g_suppress_report = 0;         // > 0: present-death runs without its mission report (reserve, or a client copy)
const char* g_suppress_why = "";
int g_mission_update_depth = 0;    // > 0: inside cMission Update (a report now would be wiped by its reset)
bool g_presenting = false;         // client: PresentDeathOnce is presenting the host's death
std::deque<std::pair<int, int>> g_report_queue;  // reports that arrived while one was still waiting

void* MissionPtr() {
    static char* game = nullptr;
    if (!game) game = reinterpret_cast<char*>(GetModuleHandleA("GameDll7.dll"));
    void* m = nullptr;
    if (game) core::safe_read(game + kRvaMission, m);
    return m;
}

bool IsKindOf(void* ent, const char* kind) {
    using Fn = int(__thiscall*)(void*, const char*);
    static Fn fn = nullptr;
    if (!fn) {
        if (HMODULE engine = GetModuleHandleA("EngineDll7.dll"))
            fn = core::find_export_as<Fn>(engine, "?IsEntityKindOf@cObjEntity@@QBEHPBD@Z");
    }
    if (!fn || !ent) return false;
    __try {
        return fn(ent, kind) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

template <class T>
T ReadAt(void* base, uintptr_t off, T fallback = T{}) {
    T v = fallback;
    if (!base || !core::safe_read(static_cast<char*>(base) + off, v)) return fallback;
    return v;
}

template <class T>
void WriteAt(void* base, uintptr_t off, T v) {
    if (base) std::memcpy(static_cast<char*>(base) + off, &v, sizeof v);
}

bool CallVt(void* ent, uintptr_t slot) {
    void** vt = ReadAt<void**>(ent, 0, nullptr);
    void* fn = vt ? ReadAt<void*>(vt, slot, nullptr) : nullptr;
    if (!fn) return false;
    __try {
        reinterpret_cast<void(__thiscall*)(void*)>(fn)(ent);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Entity vt +0x158(3, 0): what script MoveToPos calls on an animal (0x14610: health = max, dead bits and +0x600 cleared,
// IdleAttack, collision words back, brain thawed). Goes through hk_MoveToPos.
bool CallRespawn(void* ent) {
    void** vt = ReadAt<void**>(ent, 0, nullptr);
    void* fn = vt ? ReadAt<void*>(vt, 0x158, nullptr) : nullptr;
    if (!fn) return false;
    __try {
        reinterpret_cast<void(__thiscall*)(void*, int, int)>(fn)(ent, 3, 0);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Entity vt +0x234: script Kill(), the park at -10000 that also follows a corpse's fade (0x1be20).
bool CallPark(void* ent) { return CallVt(ent, 0x234); }

bool CallMissionFail(const char* text) {
    void* m = MissionPtr();
    if (!m || !g_fn_mission_fail) return false;
    __try {
        reinterpret_cast<void(__thiscall*)(void*, const char*)>(g_fn_mission_fail)(m, text);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CallMissionReportRaw(void* m, int action, int species) {
    if (!g_report_hook || !m) return false;
    __try {
        g_report_hook->unsafe_thiscall<void>(m, action, species);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CallMissionUpdate0(void* m) {
    if (!g_mission_update_hook || !m) return false;
    ++g_mission_update_depth;
    bool ok = true;
    __try {
        g_mission_update_hook->unsafe_thiscall<void>(m, 0.f);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ok = false;
    }
    --g_mission_update_depth;
    return ok;
}

bool MissionSlotBusy(void* m) { return ReadAt<int>(m, kMissionReportAction, 0) != 0; }

// cMission keeps one report (+0x1dc/+0x1e0) until its next Update hands it to the tasks; a second report in between
// overwrites the first, so one kill is lost. Two kills in a frame are rare alone, but co-op adds them: a client's kill
// arrives with the network, more players shoot. The engine's own flee path reports and then calls Update(0) at once;
// this does the same, so each report reaches the tasks: hand the waiting one over first, then record this one.
void ReportNow(void* m, int action, int species, const char* why) {
    if (MissionSlotBusy(m)) CallMissionUpdate0(m);
    const bool ok = CallMissionReportRaw(m, action, species);
    CallMissionUpdate0(m);
    spdlog::info("animals: mission report action={} species={} ({}) ok={}", action, species, why, ok);
}

void DrainReports(void* m) {
    while (!g_report_queue.empty() && g_mission_update_depth == 0) {
        const auto [action, species] = g_report_queue.front();
        g_report_queue.pop_front();
        ReportNow(m, action, species, "queued");
    }
}

// Engine reports (present-death, flee, script UpdateMissionStatus, cobra). One waiting already, or inside the mission's
// Update: queue it, and hand it over right after the next Update (hk_MissionUpdate). Never inside a present-death, so
// a task that completes cannot run its end trigger halfway through the animal's death.
void __fastcall hk_MissionReport(void* m, void*, int action, int species) {
    if (g_suppress_report > 0) {
        spdlog::info("animals: {} death kept out of the mission (action {}, species {})", g_suppress_why, action, species);
        return;
    }
    if (g_mission_update_depth > 0 || MissionSlotBusy(m) || !g_report_queue.empty()) {
        if (g_report_queue.size() < 64) g_report_queue.emplace_back(action, species);
        spdlog::info("animals: mission report action={} species={} queued (one is waiting; {} queued)", action, species,
                     g_report_queue.size());
        return;
    }
    spdlog::info("animals: mission report action={} species={}", action, species);
    g_report_hook->unsafe_thiscall<void>(m, action, species);
}

void __fastcall hk_MissionUpdate(void* m, void*, float dt) {
    ++g_mission_update_depth;
    g_mission_update_hook->unsafe_thiscall<void>(m, dt);
    --g_mission_update_depth;
    if (!g_report_queue.empty() && g_mission_update_depth == 0) DrainReports(m);
}

const char* HitKindName(float kind) {
    if (kind == kHitBullet) return "bullet";
    if (kind == kHitTranq) return "dart";
    if (kind == kHitMelee) return "melee";
    return "damage";
}

void OnReviveCommitThunk(const net::GameEvent& ev);  // below (module)
void __cdecl CmdLivesThunk();
void __fastcall hk_PresentDeath(void* ent);                                         // below (mirror table)
void __fastcall hk_AnimalOnHit(void* self, void*, void* bullet, void* pos, void* dir);  // below

void __fastcall hk_NpcHurt(void* self, void*, void* attacker, float dmg, int type, const void* pos, const void* dir) {
    if (NetIsOnline() && !NetIsHost() && IsMirroredNpc(self)) return;
    g_npc_hurt_hook->unsafe_thiscall<void>(self, attacker, dmg, type, pos, dir);
}

// cObjCharacterEx sync attack (re/notes/knockdown.md): +0x368 is set by PlaySA and cleared by OnEndSA (vt +0x214),
// which also restores the part's animation pack and sets every part to Idle.
bool InSyncAttack(void* ent) {
    int in_sa = 0;
    return core::safe_read(static_cast<char*>(ent) + 0x368, in_sa) && in_sa != 0;
}

// The base class body, not the virtual: the yeti's override faults without its (frozen) attack task
// (2026-10-17 10:20:27 ok=false); the base clears +0x368, restores physics and sets the parts to Idle.
bool EndSyncAttack(void* ent) {
    static void* fn = nullptr;
    if (!fn) {
        if (HMODULE common = GetModuleHandleA("CommonDll7.dll"))
            fn = core::find_export(common, "?OnEndSA@cObjCharacterEx@@UAEXXZ");
    }
    if (!fn || !ent) return false;
    __try {
        reinterpret_cast<void(__thiscall*)(void*)>(fn)(ent);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool g_log_states = true;
bool g_sync_log = true;
bool g_spike = false;
bool g_spike_unlink = true;
bool g_unfreeze_dead = true;  // [Animals] UnfreezeDead=1: release a client animal's brain after PresentDeath
bool g_cobra_bites = false;   // [Animals] CobraBitesNearest=1: host points +0x728 at the nearest living player
float g_spike_y = 0.7f;

std::string ini_path() { return (core::self_dir() / "dh2coop.ini").string(); }

uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

float NormYaw(float yaw) {
    yaw = std::fmod(yaw, 360.f);
    if (yaw < 0.f) yaw += 360.f;
    return yaw;
}

float Wrap180(float deg) {
    float x = std::fmod(deg + 180.f, 360.f);
    if (x < 0.f) x += 360.f;
    return x - 180.f;
}

float LerpYaw(float a, float b, float t) { return NormYaw(a + Wrap180(b - a) * t); }

struct AnimalSample {
    float pos[3] = {};
    float yaw = 0.f;
    float health = 0.f;
    uint8_t flags = 0;
    std::string state;
    std::string arms;
};

class AnimalBuffer {
public:
    void Clear() { snaps_.clear(); }

    void Push(uint32_t localMs, const net::AnimalState& state) {
        if (!snaps_.empty() && localMs < snaps_.back().timeMs) return;
        if (!snaps_.empty() && localMs == snaps_.back().timeMs) {
            snaps_.back().state = state;
            return;
        }
        snaps_.push_back(Snap{localMs, state});
        while (snaps_.size() > 1 && snaps_.back().timeMs - snaps_.front().timeMs > kInterpHistoryMs) snaps_.pop_front();
    }

    // 100 ms delay. Position and yaw lerp. State, health, and flags come from the newer snapshot.
    bool Sample(uint32_t renderMs, AnimalSample& out) const {
        if (snaps_.empty()) return false;
        const uint32_t sampleMs = renderMs > kInterpDelayMs ? renderMs - kInterpDelayMs : 0;
        const Snap& newest = snaps_.back();
        if (snaps_.size() == 1 || sampleMs >= newest.timeMs) return Copy(newest.state, out);
        if (sampleMs <= snaps_.front().timeMs) return Copy(snaps_.front().state, out);

        size_t hi = 1;
        while (hi + 1 < snaps_.size() && snaps_[hi].timeMs < sampleMs) ++hi;
        const Snap& a = snaps_[hi - 1];
        const Snap& b = snaps_[hi];
        const uint32_t span = b.timeMs - a.timeMs;
        const float t = span == 0 ? 1.f : static_cast<float>(sampleMs - a.timeMs) / static_cast<float>(span);
        for (int i = 0; i < 3; ++i) out.pos[i] = a.state.pos[i] + (b.state.pos[i] - a.state.pos[i]) * t;
        out.yaw = LerpYaw(a.state.yaw, b.state.yaw, t);
        out.health = b.state.health;
        out.flags = b.state.flags;
        out.state = b.state.state;
        out.arms = b.state.arms;
        return true;
    }

private:
    struct Snap {
        uint32_t timeMs = 0;
        net::AnimalState state{};
    };

    static bool Copy(const net::AnimalState& s, AnimalSample& out) {
        out.pos[0] = s.pos[0];
        out.pos[1] = s.pos[1];
        out.pos[2] = s.pos[2];
        out.yaw = s.yaw;
        out.health = s.health;
        out.flags = s.flags;
        out.state = s.state;
        out.arms = s.arms;
        return true;
    }

    std::deque<Snap> snaps_;
};

struct YawTrack {
    Vec3 prev{};
    float yaw = 0.f;
    bool have = false;
};

struct Mirror {
    AnimalBuffer buffer;
    bool frozen = false;
    bool freeze_logged = false;
    int refreeze_logged = 0;
    std::map<std::string, int> state_fails;  // body-state name -> failed SetBodyState calls on this copy
    uint32_t last_hit_ms = 0;  // client: last AnimalHit sent for this animal
    bool missing_logged = false;
    float rel[6] = {};
    bool have_rel = false;
    std::string applied_state;
    std::string warned_state;
    float last_host_hp = 0.f;
    bool have_hp = false;
    int place_burst = 0;
    bool have_first = false;
    entity::Vec3 first_pos{};
    float first_yaw = 0.f;
    entity::Vec3 last_target{};
    float last_yaw = 0.f;
    bool have_target = false;
    bool death_presented = false;
    bool cobra_cleared = false;
    void* cobra_bite_saved = nullptr;  // the cobra's own +0x728 before the mirror cleared it (Reset gives it back)
    bool is_npc = false;
    bool hold_pose = false;
    std::string pose_at_play;
    // Lives (client): the host's life number of this animal. A death or revive commit of another life is not this one's.
    uint16_t life = 1;
    uint32_t revive_ms = 0;     // when the last revive was applied: older snapshots may still say dead
    bool corpse_parked_logged = false;
    int relive_undone = 0;      // local script MoveToPos revivals of a corpse the host had not revived (logged 3x)
    uint32_t fade_park_ms = 0;  // when this corpse was first seen parked by its own fade while the host's is not
    int corpse_restored = 0;    // corpses put back at the host's after a local fade (logged 3x)
    // Hits (client): what the animal's own OnHit saw since the last AnimalHit.
    float hit_kind = kHitUnknown;
    int tranq_pending = 0;      // darts not reported yet (a dart does no damage, so no health drop shows it)
    bool npc_hit = false;       // the damage came from an NPC's bullet (a replayed cutscene), not a player's
    float unsent_dmg = 0.f;     // damage seen while the 150 ms report limit held it back
    bool local_death_logged = false;
};

// Guide legs (GUIDELEGS.SCR Begin) and the NPC weapon script (RIFLENPC/HGUNNPC) are separate
// state machines. Freezing the character brain stops the weapon machine, so the gun and arms
// stay in the spawn pose while the legs follow the host state.
const char* NpcLegClip(const std::string& state) {
    if (state == "Idle" || state == "NoChange") return "idlenormalneutral";
    if (state == "WalkForward") return "walk/forward";
    if (state == "WalkBackward" || state == "RunBackward") return "walk/backward";
    if (state == "WalkLeft" || state == "RunLeft") return "walk/left";
    if (state == "WalkRight" || state == "RunRight") return "walk/right";
    if (state == "RunForward") return "run/forward";
    if (state == "WalkForwardLimp") return "walk/forwardlimp";
    if (state == "IdleToWalk") return "idletowalk";
    if (state == "WalkToIdle") return "walktoidle";
    if (state == "IdleToRun") return "idletorun";
    if (state == "RunToIdle") return "runtoidle";
    if (state == "Hurt") return "hurt";
    if (state == "Fall") return "fall";
    if (state == "Liedead" || state == "FinalLiedead") return "liedead";
    if (state == "Followme" || state == "Hello") return "followme";
    if (state == "Followmeleft") return "followmeleft";
    if (state == "Followmeright") return "followmeright";
    if (state == "Conversation") return "conversation";
    if (state == "Pickup") return "pickup";
    if (state == "Find") return "find";
    if (state == "Point") return "point";
    if (state == "Cover") return "cover";
    if (state == "Uncover") return "uncover";
    if (state == "Coveridle" || state == "FinalCover") return "coveridle";
    if (state == "IdleScare" || state == "CoverScare" || state == "Scare") return "scare";
    if (state.rfind("Turnnormal", 0) == 0 && state.size() > 10) {
        static char buf[32];
        const std::string rest = state.substr(10);
        if (rest.size() + 4 >= sizeof buf) return nullptr;
        buf[0] = 't';
        buf[1] = 'u';
        buf[2] = 'r';
        buf[3] = 'n';
        for (size_t i = 0; i < rest.size(); ++i) buf[4 + i] = static_cast<char>(std::tolower(static_cast<unsigned char>(rest[i])));
        buf[4 + rest.size()] = 0;
        return buf;
    }
    return nullptr;
}

bool WeaponModel(const std::string& model) {
    std::string m = model;
    for (char& c : m) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return m.find("handgun") != std::string::npos || m.find("rifle") != std::string::npos ||
           m.find("shotgun") != std::string::npos || m.find("/weapons/") != std::string::npos;
}

void NpcWeapons(int16_t npcId, std::vector<void*>& out) {
    out.clear();
    void* mgr = EntityManager();
    int count = 0;
    void** arr = nullptr;
    if (!mgr || !core::safe_read(reinterpret_cast<char*>(mgr) + 0x08, count) ||
        !core::safe_read(reinterpret_cast<char*>(mgr) + 0x04, arr) || !arr || count <= 0 || count > 0x400)
        return;
    for (int i = 0; i < count; ++i) {
        void* ent = nullptr;
        int16_t parent = -1;
        char* model = nullptr;
        if (!core::safe_read(arr + i, ent) || !ent) continue;
        if (!core::safe_read(static_cast<char*>(ent) + 0xA4, parent) || parent != npcId) continue;
        if (!core::safe_read(static_cast<char*>(ent) + 0xE0, model) || !model) continue;
        if (!WeaponModel(core::safe_cstr(model, 160))) continue;
        out.push_back(ent);
    }
}

// Prefer a weapon that has left its spawn state, so a holstered gun does not hide the drawn one.
std::string NpcWeaponState(int16_t npcId) {
    std::vector<void*> weapons;
    NpcWeapons(npcId, weapons);
    std::string best;
    for (void* w : weapons) {
        const std::string s = entity::BodyStateName(w);
        if (s.empty()) continue;
        if (best.empty() || best == "Idle" || best == "Activate" || best == "Changein" || best == "Changeout") best = s;
    }
    return best;
}

bool IsCobra(void* ent) {
    uintptr_t vt = 0;
    HMODULE game = GetModuleHandleA("GameDll7.dll");
    return ent && game && core::safe_read(ent, vt) && vt == reinterpret_cast<uintptr_t>(game) + kVtCobraRva;
}

bool CobraMirrorState(const std::string& name) {
    return name == "Rise" || name == "Idle" || name == "Attack" || name == "TurnLeft" || name == "TurnRight" ||
           name == "Death";
}

// FUN_10012cc0 (GameDll7 0x12cc0): the call OnHit uses for "Death". It resets every part's state machine
// and then sets the name. SetBodyState alone lost to the clip that was already playing (MAP3B id 49
// returned to Idle before the timer fired).
bool CobraBodyHelper(void* ent, const char* name) {
    using Fn = void(__thiscall*)(void*, const char*);
    static Fn fn = nullptr;
    static bool ready = false;
    if (!ready) {
        ready = true;
        if (HMODULE game = GetModuleHandleA("GameDll7.dll")) {
            auto* p = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(game) + 0x12cc0);
            uint8_t got[5] = {};
            static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
            if (core::safe_copy(p, got, sizeof got) && std::memcmp(got, kPrologue, sizeof got) == 0) fn = reinterpret_cast<Fn>(p);
            else spdlog::error("animals: GameDll7+0x12cc0 prologue mismatch; cobra body helper skipped");
        }
    }
    if (!fn || !ent || !name) return false;
    __try {
        fn(ent, name);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Bullet OnHit (FUN_10080440): body state Death and +0x738 = 50. Does not subtract health and does not call +0x154.
// The host Update calls that slot once when the timer crosses 0. A second call can fail the mission.
void StartCobraDeath(void* ent, const char* why) {
    float hp = 0.f;
    core::safe_read(static_cast<char*>(ent) + kEntHealth, hp);
    int16_t id = -1;
    core::safe_read(static_cast<char*>(ent) + kEntId, id);
    if (hp <= 0.f) {
        spdlog::info("animals: cobra id={} {} skipped (hp {:.1f}, kill already counted)", id, why, hp);
        return;
    }
    const bool helper = CobraBodyHelper(ent, "Death");
    if (!helper) entity::SetBodyState(ent, "Death", true);
    std::memcpy(static_cast<char*>(ent) + kCobraTimer, &kCobraDeathTimer, sizeof kCobraDeathTimer);
    spdlog::info("animals: cobra id={} {} -> Death helper={} state='{}' timer=50", id, why, helper,
                 entity::BodyStateName(ent));
}

void* FindAnimal(uint16_t id) {
    auto match = [&](void* a) {
        int16_t eid = -1;
        return core::safe_read(static_cast<char*>(a) + kEntId, eid) && eid >= 0 && static_cast<uint16_t>(eid) == id;
    };
    for (void* a : Animals()) {
        if (match(a)) return a;
    }
    for (void* a : Npcs()) {
        if (puppets::Owns(a)) continue;
        if (match(a)) return a;
    }
    return nullptr;
}

bool CaptureRel(void* ent, float rel[6]) {
    float box[6] = {};
    Vec3 p{};
    if (!core::safe_copy(static_cast<char*>(ent) + kEntWorldBox, box, sizeof box)) return false;
    if (!core::safe_read(static_cast<char*>(ent) + kEntPos, p)) return false;
    const float* c = &p.x;
    for (int i = 0; i < 3; ++i) {
        rel[i] = box[i] - c[i];
        rel[i + 3] = box[i + 3] - c[i];
    }
    return true;
}

void PlaceEntity(void* ent, const entity::Vec3& pos, float yawDeg, const float rel[6]) {
    char buf[96];
    std::snprintf(buf, sizeof buf, "%.2f %.2f %.2f", pos.x * 100.f, pos.y * 100.f, pos.z * 100.f);
    entity::HandleKey(ent, "position", buf);
    std::snprintf(buf, sizeof buf, "0 %.2f 0", yawDeg);
    entity::HandleKey(ent, "angle", buf);
    float* box = reinterpret_cast<float*>(static_cast<char*>(ent) + kEntWorldBox);
    box[0] = pos.x + rel[0];
    box[1] = pos.y + rel[1];
    box[2] = pos.z + rel[2];
    box[3] = pos.x + rel[3];
    box[4] = pos.y + rel[4];
    box[5] = pos.z + rel[5];
    entity::BspUpdate(ent);
}

// Host only, and only when [Animals] CobraBitesNearest=1. Keeps +0x728 non-null so Update can still
// run the death timer. Default off: the spawn pointer is this process's "hunter".
void AimCobras() {
    const LocalPlayerSample& hunter = LocalPlayer();
    for (void* ent : Animals()) {
        if (!IsCobra(ent)) continue;
        float hp = 0.f;
        if (!core::safe_read(static_cast<char*>(ent) + kEntHealth, hp) || hp <= 0.f) continue;
        float pos[3] = {};
        if (!core::safe_copy(static_cast<char*>(ent) + kEntPos, pos, sizeof pos)) continue;
        float best = 1.0e12f;
        void* target = puppets::NearestLivingPuppet(pos, best);
        if (hunter.valid && hunter.entity) {
            const float dx = pos[0] - hunter.pos.x;
            const float dy = pos[1] - hunter.pos.y;
            const float dz = pos[2] - hunter.pos.z;
            const float d = dx * dx + dy * dy + dz * dz;
            if (!target || d < best) {
                target = hunter.entity;
                best = d;
            }
        }
        if (!target) continue;
        void* cur = nullptr;
        core::safe_read(static_cast<char*>(ent) + kCobraBite, cur);
        if (cur == target) continue;
        std::memcpy(static_cast<char*>(ent) + kCobraBite, &target, sizeof target);
        int16_t id = -1;
        core::safe_read(static_cast<char*>(ent) + kEntId, id);
        const bool host_hunter = target == hunter.entity;
        spdlog::info("animals: cobra id={} bite target -> {} (player {}, {:.1f} m)", id, host_hunter ? "hunter" : "puppet",
                     host_hunter ? 0 : puppets::PlayerOf(target), std::sqrt(best));
    }
}

class AnimalsModule : public dh2::Module {
public:
    const char* Name() const override { return "animals"; }
    int Priority() const override { return 46; }

    void OnInit() override {
        g_log_states = GetPrivateProfileIntA("Animals", "LogStates", 1, ini_path().c_str()) != 0;
        g_sync_log = GetPrivateProfileIntA("Animals", "SyncLog", 1, ini_path().c_str()) != 0;
        g_spike = GetPrivateProfileIntA("Animals", "Spike", 0, ini_path().c_str()) != 0;
        g_spike_unlink = GetPrivateProfileIntA("Animals", "SpikeUnlink", 1, ini_path().c_str()) != 0;
        g_unfreeze_dead = GetPrivateProfileIntA("Animals", "UnfreezeDead", 1, ini_path().c_str()) != 0;
        g_cobra_bites = GetPrivateProfileIntA("Animals", "CobraBitesNearest", 0, ini_path().c_str()) != 0;
        spdlog::info("animals: cobra bites nearest={}", g_cobra_bites);
        char y[32] = {};
        GetPrivateProfileStringA("Animals", "SpikeYOffset", "0.7", y, sizeof y, ini_path().c_str());
        g_spike_y = static_cast<float>(std::atof(y));
        spdlog::info("animals: logStates={} syncLog={} spike={}", g_log_states, g_sync_log, g_spike);
        g_self = this;
        SetPreRenderCallback(&prerender);
        events::SetHandler(events::kAnimalRevive, nullptr, &OnReviveCommitThunk);
    }

    void OnGameDllLoaded(HMODULE game) override {
        static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
        void* mtp = reinterpret_cast<char*>(game) + 0x14610;  // cScrAnimal vt +0x158 (script MoveToPos reset)
        uint8_t mtp_bytes[5] = {};
        if (core::safe_copy(mtp, mtp_bytes, sizeof mtp_bytes) && std::memcmp(mtp_bytes, kPrologue, sizeof mtp_bytes) == 0)
            g_movetopos_hook = core::install_hook("cScrAnimal MoveToPos reset (0x14610)", mtp,
                                                  reinterpret_cast<void*>(&hk_MoveToPos));
        else
            spdlog::error("animals: MoveToPos reset at GameDll7+0x14610 has unexpected bytes; not hooked");
        auto checked = [&](uintptr_t rva, const char* label) -> void* {
            void* p = reinterpret_cast<char*>(game) + rva;
            uint8_t b[5] = {};
            if (core::safe_copy(p, b, sizeof b) && std::memcmp(b, kPrologue, sizeof b) == 0) return p;
            spdlog::error("animals: {} at GameDll7+{:#x} has unexpected bytes; not hooked", label, rva);
            return nullptr;
        };
        if (void* p = checked(kRvaMissionReport, "cMission report"))
            g_report_hook = core::install_hook("cMission species report (animals, 0x91e80)", p,
                                               reinterpret_cast<void*>(&hk_MissionReport));
        if (void* p = checked(kRvaMissionUpdate, "cMission Update"))
            g_mission_update_hook = core::install_hook("cMission Update (animals, 0x94ba0)", p,
                                                       reinterpret_cast<void*>(&hk_MissionUpdate));
        if (void* p = checked(kRvaPresentDeath, "animal present-death"))
            g_death_hook = core::install_hook("animal present-death (animals, 0x1e8b0)", p,
                                              reinterpret_cast<void*>(&hk_PresentDeath));
        if (void* p = checked(kRvaAnimalOnHit, "cScrAnimal::OnHit"))
            g_onhit_hook = core::install_hook("cScrAnimal::OnHit (client hit kinds, 0x1a680)", p,
                                              reinterpret_cast<void*>(&hk_AnimalOnHit));
        g_fn_mission_fail = checked(kRvaMissionFail, "cMission fail");
        void* fn = reinterpret_cast<char*>(game) + 0x5eeb0;  // cObjNPC::OnHurt
        uint8_t got[5] = {};
        if (!core::safe_copy(fn, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
            spdlog::error("animals: cObjNPC::OnHurt at GameDll7+0x5eeb0 has unexpected bytes; not hooked");
            return;
        }
        g_npc_hurt_hook = core::install_hook("cObjNPC::OnHurt (client NPC mirrors)", fn, reinterpret_cast<void*>(&hk_NpcHurt));
    }

    void OnEngineReady(HMODULE engine) override {
        g_hit_diag = GetPrivateProfileIntA("Animals", "HitDiag", 1, ini_path().c_str()) != 0;
        if (HMODULE common = core::wait_for_module("CommonDll7.dll", 10000)) {
            if (g_hit_diag) {
                g_inflict_hook = core::install_hook(
                    "FuncInflictDamage (hit diag)",
                    core::find_export(common, "?FuncInflictDamage@cAIHuntAnimalInterface@@KAXPAXM@Z"),
                    reinterpret_cast<void*>(&hk_FuncInflictDamage));
            }
            // Always: animal hits on puppets are forwarded from here (combat.cpp). Logging is HitDiag only.
            g_onhurt_hook = core::install_hook(
                "cObjScriptedEntity::OnHurt (puppet hits, hit diag)",
                core::find_export(common, "?OnHurt@cObjScriptedEntity@@UAEXPAV1@MHABUtVect3@@1@Z"),
                reinterpret_cast<void*>(&hk_OnHurt));
        }
        if (g_spike) {
            void* draw = core::find_export(engine, "?Draw@cObjEntity@@UAEXXZ");
            g_draw_hook = core::install_hook("cObjEntity::Draw (animal diag)", draw, reinterpret_cast<void*>(&hk_Draw));
        }
    }

    // Every animal is about to be freed: forget pointers so nothing touches them until the next map.
    void OnMapUnloading() override {
        states_.clear();
        spike_ = nullptr;
        g_watch.store(nullptr);
        mirrors_.clear();
        dead_.clear();
        lives_.clear();
        reported_.clear();
        g_report_queue.clear();
    }

    void OnMapLoaded(const char*) override {
        states_.clear();
        seen_.clear();
        spike_ = nullptr;
        wait_ = 0.f;
        t_ = 0.f;
        state_t_ = 0.f;
        mirrors_.clear();
        yaws_.clear();
        dead_.clear();
        lives_.clear();
        reported_.clear();
        g_report_queue.clear();
        send_started_ = false;
        logged_tx_ = false;
        logged_rx_ = false;
        logged_cap_ = false;
        log_sync_ = 2.f;
    }

    void OnFrame(float dt) override {
        if (!commands_ && console_cmds::Ready()) {
            commands_ = console_cmds::AddCommand0("coop_lives", &CmdLivesThunk);
        }
        const auto& local = LocalPlayer();
        if (!local.valid) return;
        const auto& animals = Animals();
        if (g_log_states) LogStates(animals);
        if (!NetIsOnline() || NetIsHost()) ScanLives(animals);
        if (session_active_) {
            if (g_sync_log) {
                log_sync_ += dt;
                if (log_sync_ >= 2.f) {
                    log_sync_ = 0.f;
                    std::vector<void*> tracked = animals;
                    for (void* npc : Npcs()) {
                        if (!puppets::Owns(npc)) tracked.push_back(npc);
                    }
                    LogSync(tracked);
                }
            }
            return;
        }
        if (g_spike) Spike(dt, local, animals);
    }

    void SetSessionActive(bool active, bool hosting) {
        if (session_active_ && !active) Reset();
        if (active && !session_active_ && spike_)
            spdlog::warn("animals: session started while the solo spike held a deer; that deer stays as the spike left it");
        session_active_ = active;
        hosting_ = hosting;
    }

    void HostTick(uint32_t nowMs) {
        if (!session_active_ || !hosting_) return;
        if (g_cobra_bites) AimCobras();
        if (send_started_ && nowMs - last_send_ms_ < net::kStateIntervalMs) return;
        send_started_ = true;
        last_send_ms_ = nowMs;

        net::AnimalStates msg;
        msg.seq = ++seq_;
        msg.tickMs = nowMs;
        for (void* a : Animals()) {
            if (msg.animals.size() >= net::kMaxAnimals) break;
            int16_t id = -1;
            Vec3 p{};
            Vec3 ang{};
            float hp = 0.f;
            if (!core::safe_read(static_cast<char*>(a) + kEntId, id) || id < 0) continue;
            if (!core::safe_read(static_cast<char*>(a) + kEntPos, p)) continue;
            core::safe_read(static_cast<char*>(a) + kEntAngle, ang);
            core::safe_read(static_cast<char*>(a) + kEntHealth, hp);

            const auto uid = static_cast<uint16_t>(id);
            YawTrack& tr = yaws_[uid];
            if (!tr.have) {
                tr.have = true;
                tr.prev = p;
                tr.yaw = NormYaw(ang.y);
            } else {
                const float dx = p.x - tr.prev.x;
                const float dz = p.z - tr.prev.z;
                if (dx * dx + dz * dz > kYawMoveM * kYawMoveM) tr.yaw = NormYaw(std::atan2(dx, dz) * 57.2957795f);
                tr.prev = p;
            }

            net::AnimalState st;
            st.id = uid;
            st.pos[0] = p.x;
            st.pos[1] = p.y;
            st.pos[2] = p.z;
            st.yaw = tr.yaw;
            st.health = hp;
            st.state = entity::BodyStateName(a);
            if (hp <= 0.f) st.flags |= net::animal_flag::kDead;
            if (p.y < -1000.f) st.flags |= net::animal_flag::kParked;
            // Deaths, flees and revives are committed by ScanLives (every animal, not only the first kMaxAnimals).
            msg.animals.push_back(std::move(st));
        }
        int npc_count = 0;
        for (void* npc : Npcs()) {
            if (msg.animals.size() >= net::kMaxAnimals) break;
            if (puppets::Owns(npc)) continue;
            int16_t id = -1;
            Vec3 p{};
            Vec3 ang{};
            float hp = 0.f;
            if (!core::safe_read(static_cast<char*>(npc) + kEntId, id) || id < 0) continue;
            if (!core::safe_read(static_cast<char*>(npc) + kEntPos, p)) continue;
            core::safe_read(static_cast<char*>(npc) + kEntAngle, ang);
            // An NPC's real health is its profile's (re/notes/npc-health.md): +0x19C is only a per-frame copy.
            if (!NpcProfileHealth(npc, hp)) core::safe_read(static_cast<char*>(npc) + kEntHealth, hp);
            const auto uid = static_cast<uint16_t>(id);
            YawTrack& tr = yaws_[uid];
            if (!tr.have) {
                tr.have = true;
                tr.prev = p;
                tr.yaw = NormYaw(ang.y);
                char* model = nullptr;
                core::safe_read(static_cast<char*>(npc) + 0xE0, model);
                std::vector<void*> weapons;
                NpcWeapons(id, weapons);
                std::string wmodel;
                if (!weapons.empty()) {
                    char* wm = nullptr;
                    core::safe_read(static_cast<char*>(weapons[0]) + 0xE0, wm);
                    if (wm) wmodel = core::safe_cstr(wm, 120);
                }
                const int parts = entity::ModelPartCount(npc);
                std::string states;
                for (int i = 0; i < parts && i < 8; ++i) {
                    if (i) states += ',';
                    states += entity::PartStateName(npc, i);
                }
                spdlog::info("animals: npc id={} model='{}' weapons={} weapon='{}' parts={} states='{}'", uid,
                             model ? core::safe_cstr(model, 120) : "", weapons.size(), wmodel, parts, states);
            } else {
                const float dx = p.x - tr.prev.x;
                const float dz = p.z - tr.prev.z;
                if (dx * dx + dz * dz > kYawMoveM * kYawMoveM) tr.yaw = NormYaw(std::atan2(dx, dz) * 57.2957795f);
                tr.prev = p;
            }
            net::AnimalState st;
            st.id = uid;
            st.pos[0] = p.x;
            st.pos[1] = p.y;
            st.pos[2] = p.z;
            st.yaw = tr.yaw;
            st.health = hp;
            st.state = entity::BodyStateName(npc);
            st.arms = NpcWeaponState(id);
            if (st.arms.empty()) st.arms = entity::UpperBodyState(npc);
            st.flags = net::animal_flag::kNpc;
            if (p.y < -1000.f) st.flags |= net::animal_flag::kParked;
            msg.animals.push_back(std::move(st));
            ++npc_count;
        }
        if (!logged_tx_ && !msg.animals.empty()) {
            logged_tx_ = true;
            spdlog::info("animals: host snapshot {} animals seq={}", msg.animals.size(), msg.seq);
        }
        // Reserve animals (scale.cpp) share these slots. A full snapshot drops the rest of the list.
        const size_t total = Animals().size() + static_cast<size_t>(std::count_if(
                                 Npcs().begin(), Npcs().end(), [](void* n) { return !puppets::Owns(n); }));
        if (total > net::kMaxAnimals && !logged_cap_) {
            logged_cap_ = true;
            spdlog::warn("animals: snapshot full: {} animals and NPCs, {} sent (kMaxAnimals)", total, msg.animals.size());
        }
        if (!BroadcastAnimalStates(msg) && !logged_send_fail_) {
            logged_send_fail_ = true;
            spdlog::warn("animals: SendAnimalStates failed ({} animals)", msg.animals.size());
        }
    }

    void OnAnimalStates(const net::AnimalStates& states, uint32_t localMs) {
        if (hosting_ || !session_active_) return;
        // Entity ids only mean the same animal inside the same map (a follow load may still be in progress).
        if (!events::SameMapAsHost()) return;
        for (const net::AnimalState& a : states.animals) {
            Mirror& m = mirrors_[a.id];
            if ((a.flags & net::animal_flag::kNpc) != 0) m.is_npc = true;
            m.buffer.Push(localMs, a);
        }
        if (!logged_rx_ && !states.animals.empty()) {
            logged_rx_ = true;
            spdlog::info("animals: client received {} animals seq={}", states.animals.size(), states.seq);
        }
    }

    void OnAnimalHit(const net::AnimalHit& hit) {
        if (!hosting_) {
            spdlog::warn("animals: AnimalHit ignored (not host) id={} dmg={:.2f}", hit.animalId, hit.damage);
            return;
        }
        const float kind = hit.hitPos[0];
        const bool tranq = kind == kHitTranq;
        if (!(hit.damage > 0.f) && !tranq) return;
        void* ent = FindAnimal(hit.animalId);
        if (!ent) {
            spdlog::warn("animals: AnimalHit id={} shooter={} dmg={:.2f} no entity", hit.animalId, hit.shooterId,
                         hit.damage);
            return;
        }
        for (void* npc : Npcs()) {
            if (npc != ent) continue;
            // A story NPC is never a target from the network (MAP2B Hugh, 12:06: ~100 bogus hits a second).
            static uint32_t warned = 0;
            if (warned++ < 3) spdlog::warn("animals: AnimalHit on NPC id={} from player {} ignored", hit.animalId, hit.shooterId);
            return;
        }
        float before = 0.f;
        core::safe_read(static_cast<char*>(ent) + kEntHealth, before);
        if (before <= 0.f) {
            spdlog::info("animals: AnimalHit id={} shooter={} dmg={:.2f} ignored (already dead)", hit.animalId,
                         hit.shooterId, hit.damage);
            return;
        }
        // A cobra bullet never subtracts +0x19C. OnHit sets Death and the 50 timer; Update calls +0x154 once.
        // InflictDamage plus PresentDeath would count immediately, and the timer would count again.
        if (IsCobra(ent)) {
            StartCobraDeath(ent, "AnimalHit");
            spdlog::info("animals: AnimalHit cobra id={} shooter={} dmg={:.2f} (no health subtract)", hit.animalId,
                         hit.shooterId, hit.damage);
            return;
        }
        // What the animal's own bullet OnHit (GameDll 0x1a680) does before the damage. Without it a client's kill reached
        // the host mission as MISSION_ACTION_NONE (+0x420 = 0): kill tasks never counted it, and a client making the
        // last kill of a task left the story stuck (re/notes/life.md).
        WriteAt<int>(ent, kEntKillAction, kActionKill);
        const int protected_animal = ReadAt<int>(ent, kEntProtected, 0);
        float dmg = hit.damage;
        const char* fail = nullptr;
        if (tranq) {
            // The dart: the host runs the tranq timer (Update presents the death when +0x4f4 runs out, and that death
            // reports +0x420 = KILL: MAP6/MAP7 tranq tasks). A second dart into a tranquilized protected animal fails.
            if (ReadAt<int>(ent, kEntTranq, 0) == 2 && protected_animal == 1) {
                WriteAt<int>(ent, kEntTranq, 3);
                fail = "MISSION FAILED\nYou've injected an overdose \nand the animal died!";
            } else {
                WriteAt<int>(ent, kEntTranq, 2);
            }
            dmg = ReadAt<float>(ent, kEntTranqHurt, 0.f);
        } else if (kind == kHitBullet && protected_animal != 0) {
            fail = "MISSION FAILED\nYou've killed a protected animal!";
        }
        if (fail) {
            const bool failed = CallMissionFail(fail);
            spdlog::info("animals: AnimalHit id={} from player {} on a protected animal ({}) -> mission fail ok={}",
                         hit.animalId, hit.shooterId, tranq ? "second dart" : "bullet", failed);
        }
        const bool ok = dmg > 0.f ? entity::InflictDamage(ent, dmg) : true;
        float after = before;
        core::safe_read(static_cast<char*>(ent) + kEntHealth, after);
        if (after < 0.f) {
            // cObjScriptedEntity::OnHurt clamps at 0; FuncInflictDamage does not. Scripts and the claim prompt
            // treat a dead animal as health 0.
            *reinterpret_cast<float*>(static_cast<char*>(ent) + kEntHealth) = 0.f;
            after = 0.f;
        }
        // A bullet takes cObjScriptedEntity::OnHurt, which runs the death vfunc at 0. FuncInflictDamage only subtracts,
        // so a lethal remote hit left the host deer standing at hp 0 (coop_kill test 20:38). Present it here.
        if (after <= 0.f) {
            const bool dead = entity::PresentDeath(ent);
            spdlog::info("animals: AnimalHit id={} lethal -> PresentDeath {}", hit.animalId, dead);
        } else {
            combat::OnClientHitAnimal(ent, hit.shooterId);  // the animal turns on the shooter (with -Combat)
        }
        spdlog::info("animals: AnimalHit id={} shooter={} kind={} dmg={:.2f} hp {:.1f}->{:.1f} tranq={} ok={}",
                     hit.animalId, hit.shooterId, HitKindName(kind), dmg, before, after, ReadAt<int>(ent, kEntTranq, 0), ok);
    }

    void Reset() {
        // Review M2: thaw each mirror with the call that froze it. NPCs: CharacterBrainFreeze (their +0x200 is a
        // cAIBrain_NPC, not the animal brain). Cobras were never brain-frozen; they get their bite target back.
        for (auto& [id, m] : mirrors_) {
            void* ent = FindAnimal(id);
            if (!ent) continue;
            if (IsCobra(ent)) {
                if (m.cobra_cleared) {
                    const LocalPlayerSample& hunter = LocalPlayer();
                    void* bite = hunter.valid && hunter.entity ? hunter.entity : m.cobra_bite_saved;
                    std::memcpy(static_cast<char*>(ent) + kCobraBite, &bite, sizeof bite);
                    spdlog::info("animals: reset cobra id={} bite target -> {:#x}", id, reinterpret_cast<uintptr_t>(bite));
                }
                continue;
            }
            if (!m.frozen) continue;
            const bool ok = m.is_npc ? entity::CharacterBrainFreeze(ent, false) : entity::FreezeBrain(ent, false);
            spdlog::info("animals: reset thaw id={} npc={} ok={}", id, m.is_npc, ok);
        }
        mirrors_.clear();
        yaws_.clear();
        dead_.clear();  // lives_ (host) and reported_ (client) stay: a rejoin on this map replays the journal
        send_started_ = false;
        logged_tx_ = false;
        logged_rx_ = false;
    }

    bool IsClientMirror() const { return session_active_ && !hosting_; }

    // ---- lives ----
    // Story scripts recycle animals: script MoveToPos (0x14610) brings a dead or fled animal back (MAP2B's siege makes
    // 10 kills from 3 wolves, MAP3A 9 from 4 hyenas, MAP9 11 from 3 dingoes). Each life ends once (death or flee) and
    // is committed once: kAnimalDied with key "life <n>", kAnimalRevive when the next life starts.

    // Client: a death commit of the host's animal.
    void NoteDeath(const net::GameEvent& ev) {
        if (hosting_ || !session_active_) return;
        const uint16_t id = ev.subjectId;
        const uint16_t life = static_cast<uint16_t>(ev.i0 & 0xffffu);
        const int report = static_cast<int>((ev.i0 >> 16) & 0xffu);
        const bool fled = ((ev.i0 >> 24) & 1u) != 0;
        Mirror& m = mirrors_[id];
        if (life != 0 && life < m.life) {
            spdlog::info("animals: death commit id={} life={} ignored (this mirror is on life {})", id, life, m.life);
            return;
        }
        if (life > m.life) NewLife(id, m, life, "a later life's death");  // its revive commit was not seen here
        // The host's mission got this report from this end of life; this process's mission gets the same one, once.
        // Local deaths and flees of a client copy never report (hk_PresentDeath), so both missions count alike.
        if (report > 0 && reported_.insert((static_cast<uint32_t>(id) << 16) | life).second) {
            void* ent = FindAnimal(id);
            const int species = ent ? ReadAt<int>(ent, kEntSpecies, static_cast<int>(ev.f0)) : static_cast<int>(ev.f0);
            if (void* mission = MissionPtr()) ReportNow(mission, report - 1, species, "host death commit");
        }
        if (fled) {
            spdlog::info("animals: flee commit id={} life={} report={} (the snapshot parks it)", id, life, report);
            return;
        }
        if (dead_.insert(id).second) spdlog::info("animals: death event id={} life={} report={}", id, life, report);
    }

    // Client: the host's animal started a new life.
    void OnReviveCommit(const net::GameEvent& ev) {
        if (hosting_ || !session_active_) return;
        const uint16_t id = ev.subjectId;
        const uint16_t life = static_cast<uint16_t>(ev.i0 & 0xffffu);
        Mirror& m = mirrors_[id];
        if (life <= m.life) return;  // already on it (a replayed commit)
        NewLife(id, m, life, "host revive");
    }

    // Client: forget the old life's state, and bring the local copy back the way the host's came back (the copy may be a
    // corpse, or parked after its fade): the same respawn, then the brain frozen again before it can tick.
    void NewLife(uint16_t id, Mirror& m, uint16_t life, const char* why) {
        const uint16_t old = m.life;
        const bool was_dead = m.death_presented || dead_.count(id) != 0;
        m.life = life;
        dead_.erase(id);
        m.death_presented = false;
        m.have_first = false;
        m.have_hp = false;
        m.last_host_hp = 0.f;
        m.applied_state.clear();
        m.warned_state.clear();
        m.hold_pose = false;
        m.pose_at_play.clear();
        m.buffer.Clear();
        m.hit_kind = kHitUnknown;
        m.tranq_pending = 0;
        m.npc_hit = false;
        m.unsent_dmg = 0.f;
        m.refreeze_logged = 0;
        m.corpse_parked_logged = false;
        m.fade_park_ms = 0;
        m.local_death_logged = false;
        m.revive_ms = now_ms();
        void* ent = FindAnimal(id);
        bool respawned = false, frozen = false;
        if (ent && !m.is_npc && !IsCobra(ent)) {
            const bool local_dead = ReadAt<int>(ent, kEntDeadGuard, 0) != 0 || ReadAt<float>(ent, kEntHealth, 0.f) <= 0.f ||
                                    entity::BodyStateName(ent) == "Dead";
            if (local_dead) respawned = CallRespawn(ent);
            frozen = entity::FreezeBrain(ent, true);
            m.frozen = frozen;
            if (frozen && !m.have_rel) m.have_rel = CaptureRel(ent, m.rel);
        }
        spdlog::info("animals: revive id={} life {} -> {} ({}) was_dead={} respawn={} frozen={} state='{}'", id, old,
                     life, why, was_dead, respawned, frozen, ent ? entity::BodyStateName(ent) : std::string("?"));
    }

    // Host (and solo): one life per animal; every end is committed once, every new life too.
    struct Life {
        uint16_t life = 1;
        bool ended = false;
        int report = 0;           // 1 + the action the last present-death gave the mission (0: none)
        bool report_seen = false;
    };

    int FleeReportCode() const {
        void* m = MissionPtr();
        return m ? 1 + ReadAt<int>(m, kMissionFleeAction, 0) : 0;
    }

    void EndLife(void* ent, uint16_t id, Life& L, bool fled, const char* why) {
        const int report = fled ? FleeReportCode() : (L.report_seen ? L.report : 0);
        L.ended = true;
        const int species = ReadAt<int>(ent, kEntSpecies, -1);
        const uint32_t i0 = L.life | (static_cast<uint32_t>(report & 0xff) << 16) | (fled ? 1u << 24 : 0u);
        spdlog::info("animals: {} commit id={} life={} report={} ({}) hp={:.1f} state='{}'", fled ? "flee" : "death", id,
                     L.life, report, why, ReadAt<float>(ent, kEntHealth, 0.f), entity::BodyStateName(ent));
        events::Commit(events::kAnimalDied, id, i0, static_cast<float>(species), fmt::format("life {}", L.life), 0);
        L.report = 0;
        L.report_seen = false;
    }

    void NextLife(void* ent, uint16_t id, Life& L, const char* why) {
        ++L.life;
        L.ended = false;
        L.report = 0;
        L.report_seen = false;
        const float hp = ReadAt<float>(ent, kEntHealth, 0.f);
        spdlog::info("animals: revive commit id={} life={} ({}) hp={:.1f}", id, L.life, why, hp);
        events::Commit(events::kAnimalRevive, id, L.life, hp, fmt::format("life {}", L.life), 0);
    }

    static bool IsParkedPos(void* ent) { return ReadAt<Vec3>(ent, kEntPos, Vec3{0.f, -10000.f, 0.f}).y < -1000.f; }

    // Every frame on the host: an animal at 0 health that is not parked died; one that is parked with the death guard
    // set but health left reached a flee point (cScrAnimal::Update parks it and reports the map's flee action).
    void ScanLives(const std::vector<void*>& animals) {
        for (void* a : animals) {
            const int16_t sid = ReadAt<int16_t>(a, kEntId, -1);
            if (sid < 0) continue;
            const auto id = static_cast<uint16_t>(sid);
            Life& L = lives_[id];
            const float hp = ReadAt<float>(a, kEntHealth, 0.f);
            const bool parked = IsParkedPos(a);
            const bool guard = ReadAt<int>(a, kEntDeadGuard, 0) != 0;
            if (!L.ended) {
                if (hp <= 0.f && !parked) EndLife(a, id, L, false, "health 0");
                else if (parked && guard && hp > 0.f && !IsCobra(a)) EndLife(a, id, L, true, "reached a flee point");
            } else if (hp > 0.f && !guard && !parked) {
                NextLife(a, id, L, "alive again");  // a respawn that did not go through MoveToPos
            }
        }
    }

    // Host: script MoveToPos is about to reset this animal. A life that ended this frame is committed first.
    void HostBeforeRespawn(void* ent) {
        const int16_t sid = ReadAt<int16_t>(ent, kEntId, -1);
        if (sid < 0) return;
        Life& L = lives_[static_cast<uint16_t>(sid)];
        if (L.ended) return;
        if (ReadAt<float>(ent, kEntHealth, 0.f) <= 0.f) EndLife(ent, static_cast<uint16_t>(sid), L, false, "before a respawn");
        else if (ReadAt<int>(ent, kEntDeadGuard, 0) != 0) EndLife(ent, static_cast<uint16_t>(sid), L, true, "before a respawn");
    }

    void HostAfterRespawn(void* ent) {
        const int16_t sid = ReadAt<int16_t>(ent, kEntId, -1);
        if (sid < 0) return;
        Life& L = lives_[static_cast<uint16_t>(sid)];
        if (L.ended && ReadAt<int>(ent, kEntDeadGuard, 0) == 0) NextLife(ent, static_cast<uint16_t>(sid), L, "MoveToPos");
    }

    // Host: what present-death just told the mission (hk_PresentDeath), for the death commit.
    void NoteHostDeathReport(void* ent, int report) {
        const int16_t sid = ReadAt<int16_t>(ent, kEntId, -1);
        if (sid < 0) return;
        Life& L = lives_[static_cast<uint16_t>(sid)];
        if (L.ended) return;
        if (!L.report_seen || report > 0) {
            L.report = report;
            L.report_seen = true;
        }
    }

    // Client: a mirrored animal (frozen, or dead the host's way), never an NPC.
    Mirror* MirrorOfAnimal(void* ent) {
        if (hosting_ || !session_active_ || !ent) return nullptr;
        const int16_t sid = ReadAt<int16_t>(ent, kEntId, -1);
        if (sid < 0) return nullptr;
        const auto it = mirrors_.find(static_cast<uint16_t>(sid));
        // Any animal the host sends is the host's, frozen or thawed for a cutscene (ApplyMirror).
        if (it == mirrors_.end() || it->second.is_npc) return nullptr;
        return &it->second;
    }

    // Client: this copy's death is the host's to decide. A lethal local shot, the tranq timer or a replayed script must
    // not present it here (it would also report to this mission); PresentDeathOnce presents the host's death.
    bool SwallowClientDeath(void* ent) {
        if (g_presenting) return false;
        Mirror* m = MirrorOfAnimal(ent);
        if (!m) return false;
        if (!m->local_death_logged) {
            m->local_death_logged = true;
            spdlog::info("animals: local death of mirror id={} not presented (hp {:.1f}); the host decides",
                         ReadAt<int16_t>(ent, kEntId, -1), ReadAt<float>(ent, kEntHealth, 0.f));
        }
        return true;
    }

    // Client: the copy's OnHit saw a player's hit of this kind.
    void NoteClientHit(void* ent, float kind, bool npc_shot) {
        Mirror* m = MirrorOfAnimal(ent);
        if (!m) return;
        if (npc_shot) {
            m->npc_hit = true;
            return;
        }
        if (kind == kHitTranq) {
            ++m->tranq_pending;
        } else if (kind == kHitBullet || m->hit_kind == kHitUnknown) {
            m->hit_kind = kind;  // a bullet outranks a melee hit (only a bullet fails a protected animal)
        }
    }

    void DumpLives() {
        if (hosting_ || !session_active_) {
            for (const auto& [id, L] : lives_) {
                void* a = FindAnimal(id);
                spdlog::info("animals: life id={} '{}' life={} ended={} report={} hp={:.1f} guard={} state='{}'", id,
                             a ? core::safe_cstr(ReadAt<char*>(a, 0x198, nullptr), 32) : std::string("?"), L.life, L.ended,
                             L.report, a ? ReadAt<float>(a, kEntHealth, 0.f) : -1.f, a ? ReadAt<int>(a, kEntDeadGuard, 0) : -1,
                             a ? entity::BodyStateName(a) : std::string("?"));
            }
        }
        if (!hosting_ && session_active_) {
            for (const auto& [id, m] : mirrors_) {
                if (m.is_npc) continue;
                void* a = FindAnimal(id);
                spdlog::info("animals: mirror id={} life={} dead={} presented={} frozen={} hp={:.1f} guard={} state='{}' "
                             "brain79={}",
                             id, m.life, dead_.count(id) != 0, m.death_presented, m.frozen,
                             a ? ReadAt<float>(a, kEntHealth, 0.f) : -1.f, a ? ReadAt<int>(a, kEntDeadGuard, 0) : -1,
                             a ? entity::BodyStateName(a) : std::string("?"),
                             a ? ReadAt<uint8_t>(ReadAt<char*>(a, 0x200, nullptr), 0x79, 0xFF) : 0xFF);
            }
        }
    }

    // Client: an animal this module holds frozen (brain off, the host's pose applied every frame).
    bool IsFrozenMirrorAnimal(void* ent) {
        if (hosting_ || !session_active_ || !ent) return false;
        int16_t id = -1;
        if (!core::safe_read(static_cast<char*>(ent) + kEntId, id) || id < 0) return false;
        const auto it = mirrors_.find(static_cast<uint16_t>(id));
        return it != mirrors_.end() && it->second.frozen && !it->second.is_npc;
    }

    bool IsMirroredNpcEntity(void* ent) {
        if (hosting_ || !ent) return false;
        int16_t id = -1;
        if (!core::safe_read(static_cast<char*>(ent) + kEntId, id) || id < 0) return false;
        const auto it = mirrors_.find(static_cast<uint16_t>(id));
        return it != mirrors_.end() && it->second.is_npc;
    }

    void HoldCutscene(uint16_t id) {
        if (hosting_ || !session_active_ || !FindAnimal(id)) return;
        Mirror& m = mirrors_[id];
        m.hold_pose = true;
        m.pose_at_play.clear();
        spdlog::info("animals: cutscene hold id={}", id);
    }

    void HoldCutsceneNamed(const char* name) {
        if (!name || !name[0]) return;
        auto match = [&](void* ent) {
            char* stored = nullptr;
            if (!core::safe_read(static_cast<char*>(ent) + 0x198, stored) || !stored) return;
            if (core::safe_cstr(stored, 48) != name) return;
            int16_t id = -1;
            if (core::safe_read(static_cast<char*>(ent) + kEntId, id) && id >= 0) HoldCutscene(static_cast<uint16_t>(id));
        };
        for (void* ent : Animals()) match(ent);
        for (void* ent : Npcs()) {
            if (!puppets::Owns(ent)) match(ent);
        }
    }

    void Place() {
        if (session_active_) {
            if (!hosting_ && events::SameMapAsHost()) ApplyMirror();
            return;
        }
        PlaceSpike();
    }

private:
    void LogStates(const std::vector<void*>& animals) {
        for (void* a : animals) {
            if (a == spike_) continue;
            const std::string st = entity::BodyStateName(a);
            auto& prev = states_[a];
            if (st == prev) continue;
            prev = st;
            if (!st.empty()) seen_.insert(st);
            int16_t id = -1;
            Vec3 p{};
            float hp = 0.f;
            core::safe_read(static_cast<char*>(a) + kEntId, id);
            core::safe_read(static_cast<char*>(a) + kEntPos, p);
            core::safe_read(static_cast<char*>(a) + kEntHealth, hp);
            spdlog::info("animals: id={} state='{}' pos=({:.1f},{:.1f},{:.1f}) hp={:.0f}", id, st, p.x, p.y, p.z, hp);
        }
    }

    void LogSync(const std::vector<void*>& animals) {
        const char* role = hosting_ ? "host" : "client";
        for (void* a : animals) {
            int16_t id = -1;
            Vec3 p{};
            float hp = 0.f;
            if (!core::safe_read(static_cast<char*>(a) + kEntId, id)) continue;
            if (!core::safe_read(static_cast<char*>(a) + kEntPos, p)) continue;
            core::safe_read(static_cast<char*>(a) + kEntHealth, hp);
            const std::string st = entity::BodyStateName(a);
            float yaw = 0.f;
            if (hosting_) {
                auto it = yaws_.find(static_cast<uint16_t>(id));
                if (it != yaws_.end()) yaw = it->second.yaw;
                spdlog::info("animals: sync role={} id={} pos=({:.2f},{:.2f},{:.2f}) state='{}' hp={:.1f} yaw={:.1f}",
                             role, id, p.x, p.y, p.z, st, hp, yaw);
            } else {
                auto it = mirrors_.find(static_cast<uint16_t>(id));
                if (it != mirrors_.end() && it->second.have_target) {
                    const Mirror& m = it->second;
                    yaw = m.last_yaw;
                    const float drift = std::fabs(p.x - m.last_target.x) + std::fabs(p.z - m.last_target.z);
                    spdlog::info(
                        "animals: sync role={} id={} pos=({:.2f},{:.2f},{:.2f}) state='{}' hp={:.1f} yaw={:.1f} "
                        "target=({:.2f},{:.2f},{:.2f}) drift={:.3f}",
                        role, id, p.x, p.y, p.z, st, hp, yaw, m.last_target.x, m.last_target.y, m.last_target.z, drift);
                } else {
                    spdlog::info("animals: sync role={} id={} pos=({:.2f},{:.2f},{:.2f}) state='{}' hp={:.1f} yaw={:.1f}",
                                 role, id, p.x, p.y, p.z, st, hp, yaw);
                }
            }
        }
    }

    void ApplyMirror() {
        const uint32_t t = now_ms();
        for (uint16_t id : dead_) mirrors_[id];
        for (auto& [id, m] : mirrors_) {
            void* ent = FindAnimal(id);
            if (!ent) {
                if (!m.missing_logged) {
                    m.missing_logged = true;
                    spdlog::info("animals: mirror id={} waiting for a local entity", id);
                }
                continue;
            }
            if (m.death_presented) {
                // The released brain plays the death; only the health is pinned (claim needs exactly 0).
                *reinterpret_cast<float*>(static_cast<char*>(ent) + kEntHealth) = 0.f;
                KeepCorpse(id, m, ent, t);
                continue;
            }
            // MAP12C's opening cutscene WaitForTask's the yeti. Freezing that brain, or pinning
            // the yeti to the host pose, never lets the task finish, so the client stays in the
            // cutscene camera (player 2 could see their character and could not move, 22:48).
            // Only while the host is in that same pose. MAP2A wolf 154 stayed CutScene on the
            // client after the host had left it, and the later death commit never presented (23:45).
            AnimalSample early;
            const bool have_early = m.buffer.Sample(t, early);
            const bool host_cutscene = !have_early || early.state == "CutScene";
            // Right after a revive, a snapshot sent before it can still arrive and say dead: only commits count then.
            const bool fresh_life = m.revive_ms != 0 && t - m.revive_ms < 500;
            const bool host_dead = dead_.count(id) != 0 ||
                                   (!fresh_life && have_early &&
                                    ((early.flags & net::animal_flag::kDead) != 0 || early.health <= 0.f));
            if (!m.is_npc && !host_dead && host_cutscene && entity::BodyStateName(ent) == "CutScene") {
                if (m.frozen) {
                    entity::FreezeBrain(ent, false);
                    m.frozen = false;
                    spdlog::info("animals: mirror thaw id={} while state is CutScene", id);
                }
                continue;
            }
            // The local copy is still inside a sync attack (PlaySA, +0x368) that the host's actor has finished. Its task
            // is frozen here, so nothing calls OnEndSA: the part keeps the partner's animation pack and the next state
            // the body script enters raises the engine's fatal "Could not forward animation" (MAP12C yeti, its opening
            // PlaySA ran before the join; 2026-10-17 10:17:08 the client exited when the host's yeti attacked).
            // NPCs only: on the MAP12C yeti both the virtual and the base OnEndSA fault halfway (10:20:27, 10:22:39),
            // and a half-run end is worse than none. The yeti fight still crashes clients (EngineDll7+0x28b337): open.
            if (m.is_npc && have_early && !host_cutscene && InSyncAttack(ent)) {
                const bool ended = EndSyncAttack(ent);
                spdlog::info("animals: mirror id={} ended its local sync attack (host state '{}') ok={}", id, early.state,
                             ended);
                m.frozen = false;  // OnEndSA unfreezes the character; freeze again below
                m.applied_state.clear();
            }
            if (!m.frozen) {
                const bool ok = m.is_npc ? entity::CharacterBrainFreeze(ent, true) : entity::FreezeBrain(ent, true);
                if (!m.freeze_logged) {
                    m.freeze_logged = true;
                    spdlog::info("animals: mirror freeze id={} ok={} npc={} state='{}'", id, ok, m.is_npc,
                                 entity::BodyStateName(ent));
                }
                // cObjCobra's +0x200 is not a cAIHuntBrain_Animal: FreezeBrain faults (MAP3B ids 49–51,
                // ok=false). Clearing +0x728 is what stops Update. Do not retry the faulting call.
                if (!ok && !m.is_npc && !IsCobra(ent)) continue;
                if (ok) {
                    m.frozen = true;
                    m.have_rel = CaptureRel(ent, m.rel);
                    if (!m.have_rel) spdlog::warn("animals: mirror id={} world box unreadable", id);
                } else if (IsCobra(ent)) {
                    m.frozen = true;
                    m.have_rel = CaptureRel(ent, m.rel);
                    if (!m.have_rel) spdlog::warn("animals: mirror id={} world box unreadable", id);
                }
            } else if (!m.is_npc && !IsCobra(ent) && !host_cutscene) {
                // A replayed script MoveToPos (0x14610) clears brain +0x79, the FreezeBrain byte: this copy then ran its
                // own AI, attacked the local hunter and fought the mirror's body states until the script VM faulted
                // (MAP4A client, cLionesse "Attack chain", 2026-10-18; audit-a G5 / audit-b G2). Freeze it again.
                char* brain = nullptr;
                uint8_t frozen_byte = 1;
                if (core::safe_read(static_cast<char*>(ent) + 0x200, brain) && brain &&
                    core::safe_read(brain + 0x79, frozen_byte) && frozen_byte == 0) {
                    const bool ok = entity::FreezeBrain(ent, true);
                    if (m.refreeze_logged < 3) {
                        ++m.refreeze_logged;
                        spdlog::info("animals: mirror id={} brain woke up (script MoveToPos); frozen again ok={}", id, ok);
                    }
                }
            }
            if (IsCobra(ent)) {
                void* bite = nullptr;
                core::safe_read(static_cast<char*>(ent) + kCobraBite, bite);
                if (bite) {
                    void* zero = nullptr;
                    std::memcpy(static_cast<char*>(ent) + kCobraBite, &zero, sizeof zero);
                }
                if (!m.cobra_cleared) {
                    m.cobra_cleared = true;
                    m.cobra_bite_saved = bite;
                    spdlog::info("animals: cobra id={} bite target cleared (was {:#x})", id,
                                 reinterpret_cast<uintptr_t>(bite));
                }
            }
            AnimalSample sample;
            if (!m.buffer.Sample(t, sample)) continue;

            // Review M1: a dead NPC is not presented the animal way (vt +0x154 is cObjNPC::OnDie, and the NPC's own update
            // already runs it once its profile health is 0). It stays placed and posed: the host's fall/liedead clips.
            const bool npc_dead = m.is_npc && dead_.count(id) != 0;
            if (dead_.count(id) != 0 && !m.is_npc) {
                // Dead (the reliable commit arrives before the delayed snapshot says so). Health stays 0: a stale
                // sample (hp 45, 100 ms old) revived the client deer, and PresentDeath's own 0 then looked like a
                // client hit (2026-10-16 20:32 user test). The claim prompt needs an animal at exactly 0.
                *reinterpret_cast<float*>(static_cast<char*>(ent) + kEntHealth) = 0.f;
                m.last_host_hp = 0.f;
                m.have_hp = true;
                if (!m.death_presented) PresentDeathOnce(id, m, ent, sample);
                continue;
            }

            // NPCs are not targets: the client's Uncle Hugh read lower than the host's every frame and ~100 "hits" a
            // second reached the host's Hugh (two-PC test, MAP2B 12:06). One report per animal per 150 ms at most.
            if (!m.is_npc) ReportLocalHits(id, m, ent, t);
            // A dead NPC stays at 0: a stale sample (100 ms old) must not revive it.
            const float hp_now = npc_dead ? 0.f : sample.health;
            *reinterpret_cast<float*>(static_cast<char*>(ent) + kEntHealth) = hp_now;
            // NPCs: the host's profile health into ours (runs after the client's own Update, so it wins until the
            // next frame). The bar and the NPC's death check read it; at 0 the NPC's own update runs its OnDie.
            if (m.is_npc) SetNpcProfileHealth(ent, hp_now > 0.f ? hp_now : 0.f);
            m.last_host_hp = hp_now;
            m.have_hp = true;

            const bool parked = (sample.flags & net::animal_flag::kParked) != 0 || sample.pos[1] < -1000.f;
            const bool dead = !parked && !fresh_life &&
                              ((sample.flags & net::animal_flag::kDead) != 0 || sample.health <= 0.f);
            if (dead && m.is_npc) {
                // Not the animal death presentation: the profile at 0 lets the NPC's own update die the NPC way. The NPC
                // stays placed below, so the host's fall and liedead clips play on it.
                if (dead_.insert(id).second) spdlog::info("animals: npc id={} died on the host (profile 0 here)", id);
            } else if (dead) {
                dead_.insert(id);
                *reinterpret_cast<float*>(static_cast<char*>(ent) + kEntHealth) = 0.f;
                PresentDeathOnce(id, m, ent, sample);
                continue;
            }

            entity::Vec3 pos{sample.pos[0], sample.pos[1], sample.pos[2]};
            float yaw = sample.yaw;
            if (!m.have_first) {
                Vec3 spawn{};
                core::safe_read(static_cast<char*>(ent) + kEntPos, spawn);
                m.have_first = true;
                m.first_pos = pos;
                m.first_yaw = yaw;
                m.place_burst = kTeleportRepeats;
                spdlog::info(
                    "animals: mirror first place id={} spawn=({:.1f},{:.1f},{:.1f}) target=({:.1f},{:.1f},{:.1f}) "
                    "yaw={:.1f} (repeat {} frames)",
                    id, spawn.x, spawn.y, spawn.z, pos.x, pos.y, pos.z, yaw, kTeleportRepeats);
            }
            if (m.place_burst > 0) {
                pos = m.first_pos;
                yaw = m.first_yaw;
                --m.place_burst;
            }
            // An actor attached to something here (the MAP4A driver in this client's own truck) rides with it. Moving
            // it to the host's seat position pulled it out of the truck whenever the two trucks were apart (audit-a G10).
            int16_t parent = -1;
            const bool attached = core::safe_read(static_cast<char*>(ent) + kEntParent, parent) && parent >= 0;
            if (m.have_rel && !attached) PlaceEntity(ent, pos, yaw, m.rel);
            m.last_target = pos;
            m.last_yaw = yaw;
            m.have_target = true;

            // Re-apply when the live name drifts. FreezeBrain stops pathing, but the state machine can
            // still leave the host's state (deer 121 showed IdleAttack for ~4 s while the host stayed CutScene).
            if (m.hold_pose) {
                if (m.pose_at_play.empty()) m.pose_at_play = sample.state;
                else if (!sample.state.empty() && sample.state != m.pose_at_play) {
                    m.hold_pose = false;
                    m.applied_state.clear();
                    spdlog::info("animals: cutscene done id={} state='{}'", id, sample.state);
                }
            }
            if (m.hold_pose) {
                // PlaySA is posing this actor. A body-state write replaces the paired clip.
            } else if (m.is_npc) {
                if (!sample.state.empty() && entity::BodyStateName(ent) != sample.state)
                    entity::SetBodyState(ent, sample.state.c_str(), true);
                if (m.applied_state != sample.state) {
                    if (const char* clip = NpcLegClip(sample.state)) entity::SetAnim(ent, clip);
                    m.applied_state = sample.state;
                }
                // The handgun's own body script SetAnims clips like handgun3/normaltoalert onto its legs
                // part and the engine raises a fatal dialog (client 23:32). Leave the gun alone.
            } else if (!sample.state.empty() && entity::BodyStateName(ent) != sample.state) {
                if (IsCobra(ent) && !CobraMirrorState(sample.state)) {
                    if (sample.state != m.warned_state) {
                        m.warned_state = sample.state;
                        spdlog::info("animals: cobra id={} skip body state '{}'", id, sample.state);
                    }
                    continue;
                }
                // Rise does not yield to SetBodyState. The game's own helper resets the state machine first.
                if (IsCobra(ent) && CobraBodyHelper(ent, sample.state.c_str())) {
                    m.applied_state = sample.state;
                    continue;
                }
                // A state this copy cannot enter is not retried every frame: each failed GotoState runs the body
                // script into a missing clip (lioness "idleattack1"), and hundreds of those a second ended in a script
                // VM fault (MAP4A client, EngineDll7+0x29a8bf, 2026-10-18). Three tries per state name, then leave it.
                int& fails = m.state_fails[sample.state];
                if (fails >= 3) continue;
                int r = entity::SetBodyState(ent, sample.state.c_str(), false);
                // From some states the machine refuses a direct jump (the lioness copy after a tackle stood frozen in
                // one pose, sliding along with the host's lioness). A reset first is how the NPC mirror enters states.
                if (r != 1) r = entity::SetBodyState(ent, sample.state.c_str(), true);
                if (r == 1) {
                    m.applied_state = sample.state;
                    fails = 0;
                } else {
                    if (++fails >= 3)
                        spdlog::warn("animals: SetBodyState id={} '{}' -> {} (copy is in '{}'); not tried again on this "
                                     "animal",
                                     id, sample.state, r, entity::BodyStateName(ent));
                    else if (sample.state != m.warned_state)
                        spdlog::warn("animals: SetBodyState id={} '{}' -> {}", id, sample.state, r);
                    m.warned_state = sample.state;
                }
            }
        }
    }

    // Client, every frame for a mirror whose death was presented. The corpse follows the host's: when the host parks its
    // corpse (fade) this one is parked too. And a replayed script MoveToPos the host has not made (each process runs a
    // siege's spawn timer and CanBeRespawned on its own copies: MAP2B SpawnWolves) brings the local copy back with
    // health and AI while the host's animal is still dead: it is put back (parked, or dead again at the host's corpse)
    // until the host's revive commit arrives.
    void KeepCorpse(uint16_t id, Mirror& m, void* ent, uint32_t t) {
        if (IsCobra(ent)) return;
        AnimalSample s;
        if (!m.buffer.Sample(t, s)) return;
        const bool host_parked = (s.flags & net::animal_flag::kParked) != 0 || s.pos[1] < -1000.f;
        const bool here_parked = IsParkedPos(ent);
        const bool relived = ReadAt<int>(ent, kEntDeadGuard, 1) == 0;  // MoveToPos cleared the death guard
        if (relived) {
            bool ok = false;
            if (host_parked) {
                ok = CallPark(ent);
            } else {
                entity::Vec3 pos{s.pos[0], s.pos[1], s.pos[2]};
                if (m.have_rel) PlaceEntity(ent, pos, s.yaw, m.rel);
                g_presenting = true;
                ok = entity::PresentDeath(ent);
                g_presenting = false;
                if (g_unfreeze_dead) entity::FreezeBrain(ent, false);
            }
            if (m.relive_undone++ < 3)
                spdlog::info("animals: mirror id={} came back here but not on the host (life {}): {} ok={}", id, m.life,
                             host_parked ? "parked again" : "dead again at the host's corpse", ok);
            return;
        }
        if (here_parked && !host_parked) {
            // This copy's own corpse fade parked it (cScrAnimal::Update 0x1f3c0, health 0: countdowns +0x6f4, +0x450, then
            // vt +0x234), but the host's corpse is still there (MAP7: a tranquilized BTIGER stays; the client's sat at
            // -10000 for good). Wolves fade on both within a second of each other, so give the host a moment to follow;
            // then put the corpse back at the host's. The parked copy no longer updates (+0x4b0 = 0): it cannot park again.
            if (m.fade_park_ms == 0) {
                m.fade_park_ms = t;
            } else if (t - m.fade_park_ms >= 3000 && m.have_rel) {
                entity::Vec3 pos{s.pos[0], s.pos[1], s.pos[2]};
                PlaceEntity(ent, pos, s.yaw, m.rel);
                m.fade_park_ms = 0;
                if (m.corpse_restored++ < 3)
                    spdlog::info("animals: mirror id={} corpse faded away here but not on the host: put back at "
                                 "({:.1f},{:.1f},{:.1f})", id, pos.x, pos.y, pos.z);
            }
            return;
        }
        m.fade_park_ms = 0;
        if (host_parked && !here_parked) {
            const bool ok = CallPark(ent);
            if (!m.corpse_parked_logged) {
                m.corpse_parked_logged = true;
                spdlog::info("animals: mirror id={} corpse parked like the host's ok={}", id, ok);
            }
        }
    }

    // Client: turn what this copy took locally into AnimalHit. The copy's health is the host's again every frame, so a
    // drop below it is damage from this frame. Its OnHit (hk_AnimalOnHit) says whether a player's bullet, dart or knife did
    // it; damage nobody's shot explains (a replayed cutscene's script, an NPC's bullet) is not the host's business
    // (review M8), except coop_kill's.
    void ReportLocalHits(uint16_t id, Mirror& m, void* ent, uint32_t t) {
        float localHp = 0.f;
        if (!core::safe_read(static_cast<char*>(ent) + kEntHealth, localHp) || !m.have_hp || !(m.last_host_hp > 0.f)) {
            m.tranq_pending = 0;
            m.hit_kind = kHitUnknown;
            m.npc_hit = false;
            m.unsent_dmg = 0.f;
            return;
        }
        if (localHp < m.last_host_hp - 0.01f) m.unsent_dmg += m.last_host_hp - localHp;
        const bool tranq = m.tranq_pending > 0;
        if (!tranq && !(m.unsent_dmg > 0.f)) return;
        if (!tranq && m.hit_kind == kHitUnknown && (m.npc_hit || CameraOnRail())) {
            spdlog::info("animals: local damage id={} {:.2f} not reported ({})", id, m.unsent_dmg,
                         m.npc_hit ? "an NPC's bullet" : "cutscene camera");
            m.unsent_dmg = 0.f;
            m.npc_hit = false;
            return;
        }
        if (!tranq && t - m.last_hit_ms < 150) return;  // held back: the next report carries it
        m.last_hit_ms = t;
        net::AnimalHit hit;
        hit.animalId = id;
        hit.damage = m.unsent_dmg;
        hit.hitPos[0] = tranq ? kHitTranq : m.hit_kind;
        const bool sent = ReportAnimalHit(hit);
        spdlog::info("animals: local hit id={} kind={} dmg={:.2f} localHp={:.2f} hostHp={:.2f} sent={}", id,
                     HitKindName(hit.hitPos[0]), hit.damage, localHp, m.last_host_hp, sent);
        if (tranq) --m.tranq_pending;
        m.unsent_dmg = 0.f;
        m.hit_kind = kHitUnknown;
        m.npc_hit = false;
    }

    // story.cpp's test: a cutscene puts the render camera on a rail, metres from the hunter.
    static bool CameraOnRail() {
        const LocalPlayerSample& p = LocalPlayer();
        if (!p.valid || !p.viewValid) return false;
        const float dx = p.viewPos.x - p.pos.x, dy = p.viewPos.y - p.pos.y, dz = p.viewPos.z - p.pos.z;
        return dx * dx + dy * dy + dz * dz > 16.f;
    }

    // One place at the corpse, then the death vfunc. Later frames must not HandleKey or SetBodyState: that pins
    // the standing pose. A frozen brain never runs the death itself (user 20:32: PresentDeath ok, deer stayed
    // standing, state went IdleAttack), so the brain is released afterwards to play the fall like the host's.
    void PresentDeathOnce(uint16_t id, Mirror& m, void* ent, const AnimalSample& sample) {
        if (m.death_presented || m.is_npc) return;  // NPCs die through their own OnDie (review M1)
        entity::Vec3 pos{sample.pos[0], sample.pos[1], sample.pos[2]};
        if (m.have_rel) PlaceEntity(ent, pos, sample.yaw, m.rel);
        m.last_target = pos;
        m.last_yaw = sample.yaw;
        m.have_target = true;
        // Cobra slot +0x154 (FUN_10080320) counts the kill and does not set a body state. The fall is "Death",
        // never the cScrAnimal name "Dead". Leave the brain frozen: Update only runs the timer while +0x728 is set,
        // and that pointer is cleared on the client, so an unfreeze does not play the fall.
        const bool cobra = IsCobra(ent);
        if (cobra && entity::BodyStateName(ent) != "Death" && !CobraBodyHelper(ent, "Death"))
            entity::SetBodyState(ent, "Death", true);
        // hk_PresentDeath lets this one through (and keeps it out of this mission: the death commit reports instead).
        g_presenting = true;
        m.death_presented = entity::PresentDeath(ent);
        g_presenting = false;
        bool unfrozen = false;
        if (g_unfreeze_dead && m.frozen && !cobra) {
            unfrozen = entity::FreezeBrain(ent, false);
            if (unfrozen) m.frozen = false;
        }
        spdlog::info("animals: present death id={} ok={} unfrozen={} pos=({:.1f},{:.1f},{:.1f}) state='{}'", id,
                     m.death_presented, unfrozen, pos.x, pos.y, pos.z, entity::BodyStateName(ent));
    }

    void Spike(float dt, const LocalPlayerSample& local, const std::vector<void*>& animals) {
        (void)local;
        if (!spike_) {
            wait_ += dt;
            if (wait_ < 8.f) return;
            for (void* a : animals) {
                Vec3 p{};
                float hp = 0.f;
                core::safe_read(static_cast<char*>(a) + kEntPos, p);
                core::safe_read(static_cast<char*>(a) + kEntHealth, hp);
                if (p.y > -1000.f && hp > 0.f && std::fabs(hp - 45.f) < 1.f) {
                    spike_ = a;
                    break;
                }
            }
            if (!spike_) return;
            const bool frozen = entity::FreezeBrain(spike_, true);
            const bool unlinked = g_spike_unlink && entity::UnlinkPhysics(spike_);
            float box[6] = {};
            Vec3 p{};
            core::safe_copy(static_cast<char*>(spike_) + kEntWorldBox, box, sizeof box);
            core::safe_read(static_cast<char*>(spike_) + kEntPos, p);
            for (int i = 0; i < 3; ++i) {
                rel_[i] = box[i] - (&p.x)[i];
                rel_[i + 3] = box[i + 3] - (&p.x)[i];
            }
            int16_t id = -1;
            core::safe_read(static_cast<char*>(spike_) + kEntId, id);
            spdlog::info(
                "animals: spike took id={} frozen={} unlinked={} state='{}' box rel ({:.2f},{:.2f},{:.2f})-({:.2f},{:.2f},{:.2f})",
                id, frozen, unlinked, entity::BodyStateName(spike_), rel_[0], rel_[1], rel_[2], rel_[3], rel_[4],
                rel_[5]);
        }
        t_ += dt;
        const float drift = drift_;
        const entity::Vec3 pos = pos_;
        state_t_ += dt;
        if (state_t_ >= 4.f && !seen_.empty()) {
            state_t_ = 0.f;
            std::vector<std::string> names(seen_.begin(), seen_.end());
            const std::string& want = names[state_idx_++ % names.size()];
            const int r = entity::SetBodyState(spike_, want.c_str(), false);
            spdlog::info("animals: spike set state '{}' -> {} now '{}' drift={:.3f} pos=({:.1f},{:.1f},{:.1f})", want, r,
                         entity::BodyStateName(spike_), drift, pos.x, pos.y, pos.z);
        }
    }

    void PlaceSpike() {
        if (!spike_) return;
        const auto& local = LocalPlayer();
        if (!local.valid) return;
        Vec3 now{};
        core::safe_read(static_cast<char*>(spike_) + kEntPos, now);
        const float drift = std::fabs(now.x - last_.x) + std::fabs(now.z - last_.z);
        const float vy = (local.viewValid ? local.viewYaw : local.angle.y) * 3.14159265f / 180.f;
        const float cx = local.pos.x + 8.f * std::sin(vy), cz = local.pos.z + 8.f * std::cos(vy);
        const float a = t_ * 0.5f;
        const entity::Vec3 pos{cx + 3.f * std::sin(a), local.pos.y + g_spike_y, cz + 3.f * std::cos(a)};
        const float heading = (a + 1.5707963f) * 57.2957795f;
        if (g_spike_unlink) {
            entity::SetPos(spike_, pos);
            entity::SetAngle(spike_, {0.f, heading, 0.f});
        } else {
            PlaceEntity(spike_, pos, heading, rel_);
        }
        if (g_spike_unlink) {
            float* box = reinterpret_cast<float*>(static_cast<char*>(spike_) + kEntWorldBox);
            box[0] = pos.x + rel_[0];
            box[1] = pos.y + rel_[1];
            box[2] = pos.z + rel_[2];
            box[3] = pos.x + rel_[3];
            box[4] = pos.y + rel_[4];
            box[5] = pos.z + rel_[5];
            entity::BspUpdate(spike_);
        }
        last_ = {pos.x, pos.y, pos.z};
        g_watch.store(spike_);
        if (++diag_frames_ >= 50) {
            int link = -2;
            core::safe_read(static_cast<char*>(spike_) + 0x124, link);
            float clock = std::fmod(a * 57.2957795f, 360.f);
            spdlog::info("animals: diag circle={:.0f}deg draws/50f={} bsplink={} pos=({:.1f},{:.1f},{:.1f})", clock,
                         g_draws.exchange(0), link, pos.x, pos.y, pos.z);
            diag_frames_ = 0;
        }
        drift_ = drift;
        pos_ = pos;
    }

    std::map<void*, std::string> states_;
    std::set<std::string> seen_;
    void* spike_ = nullptr;
    float wait_ = 0.f, t_ = 0.f, state_t_ = 0.f;
    size_t state_idx_ = 0;
    float rel_[6] = {};
    Vec3 last_{};
    float drift_ = 0.f;
    int diag_frames_ = 0;
    entity::Vec3 pos_{};

    bool session_active_ = false;
    bool hosting_ = false;
    bool send_started_ = false;
    bool logged_tx_ = false;
    bool logged_rx_ = false;
    bool logged_send_fail_ = false;
    bool logged_cap_ = false;
    uint32_t last_send_ms_ = 0;
    uint16_t seq_ = 0;
    float log_sync_ = 2.f;
    std::map<uint16_t, YawTrack> yaws_;
    std::map<uint16_t, Mirror> mirrors_;
    std::set<uint16_t> dead_;
    std::map<uint16_t, Life> lives_;  // host / solo
    std::set<uint32_t> reported_;     // client: (id << 16 | life) whose host report this mission already got
    bool commands_ = false;
};

AnimalsModule* g_self = nullptr;
void prerender() {
    if (g_self) g_self->Place();
}

bool IsMirroredNpc(void* ent) { return g_self && g_self->IsMirroredNpcEntity(ent); }

// Client: a replayed script MoveToPos resets the animal (health, dead bits, IdleAttack) and clears brain +0x79. The
// per-frame re-freeze in ApplyMirror came one AI tick too late on MAP4A: that tick started a lion's attack chain
// on the local hunter, and the sync attack later faulted (EngineDll7+0x28b337). Freeze before the AI ever runs.
// Host / solo: the same call starts a new life when the animal had died or fled (kAnimalRevive).
int __fastcall hk_MoveToPos(void* self, void*, int mode, int arg) {
    if (!NetIsOnline() || NetIsHost()) {
        if (g_self) g_self->HostBeforeRespawn(self);
        const int r = g_movetopos_hook->unsafe_thiscall<int>(self, mode, arg);
        if (g_self) g_self->HostAfterRespawn(self);
        scale::AfterRespawn(self);
        return r;
    }
    const int r = g_movetopos_hook->unsafe_thiscall<int>(self, mode, arg);
    scale::AfterRespawn(self);
    if (g_self && (g_self->IsFrozenMirrorAnimal(self) || g_self->MirrorOfAnimal(self))) {
        const bool ok = entity::FreezeBrain(self, true);
        static int logged = 0;
        if (logged < 8) {
            ++logged;
            int16_t id = -1;
            core::safe_read(static_cast<char*>(self) + kEntId, id);
            spdlog::info("animals: MoveToPos on mirror id={}: brain frozen again at once ok={}", id, ok);
        }
    }
    return r;
}

// Present-death (0x1e8b0). Host: note what it reports for the death commit; a reserve's death stays out of the mission.
// Client: a mirror only dies when the host's commit says so, and never reports to this mission here.
void __fastcall hk_PresentDeath(void* ent) {
    if (g_self && g_self->SwallowClientDeath(ent)) return;
    const bool client = NetIsOnline() && !NetIsHost();
    const bool reserve = scale::IsReserve(ent);
    int report = 0;
    if (!client && !reserve && ReadAt<int>(ent, kEntDeadGuard, 1) == 0 && (ReadAt<uint8_t>(ent, 0xC1, 0) & 4) == 0 &&
        MissionPtr() && ReadAt<int>(ent, kEntEventMode, 0) != 2)
        report = 1 + std::clamp(ReadAt<int>(ent, kEntKillAction, 0), 0, 250);
    if (client || reserve) {
        ++g_suppress_report;
        g_suppress_why = reserve ? "reserve" : "client copy's";
        __try {
            g_death_hook->unsafe_fastcall<void>(ent);
        } __finally {
            --g_suppress_report;
        }
    } else {
        g_death_hook->unsafe_fastcall<void>(ent);
    }
    if (!client && g_self) g_self->NoteHostDeathReport(ent, report);
}

// Client: what kind of hit the copy took (a dart does no damage; a protected animal fails only for a bullet). The local
// tranq state is put back: the host runs the tranq timer and decides the death.
void __fastcall hk_AnimalOnHit(void* self, void*, void* bullet, void* pos, void* dir) {
    if (!g_self || !g_self->MirrorOfAnimal(self) || !bullet) {
        g_onhit_hook->unsafe_thiscall<void>(self, bullet, pos, dir);
        return;
    }
    const bool is_bullet = IsKindOf(bullet, "mod_obj_bullet");
    const bool tranq = is_bullet && IsKindOf(bullet, "mod_obj_tranq_bullet");
    const int kind = ReadAt<int>(bullet, kBulletKind, 0);
    const int bone = ReadAt<int>(self, kEntHitBone, -1);
    const uint32_t bits_before = ReadAt<uint32_t>(self, kEntHitBits, 0);
    const int tranq_before = ReadAt<int>(self, kEntTranq, 0);
    WriteAt<uint32_t>(self, kEntHitBits, 0);
    g_onhit_hook->unsafe_thiscall<void>(self, bullet, pos, dir);
    const uint32_t bits = ReadAt<uint32_t>(self, kEntHitBits, 0);
    WriteAt<uint32_t>(self, kEntHitBits, bits_before | bits);
    WriteAt<int>(self, kEntTranq, tranq_before);
    if (!is_bullet) return;
    // Darts are beasts.cpp's: its hook sends kTranqHit and the host runs the tranq branch. Reporting one here too would
    // run it twice on the host (a second dart into a protected animal is an overdose: mission failed).
    if (tranq) return;
    if (kind != 2 && bone < 0) return;  // no bone hit: OnHit did nothing
    const bool npc_shot = (bits & 8u) != 0;
    const float hit = kind == 2 ? kHitMelee : kHitBullet;
    g_self->NoteClientHit(self, hit, npc_shot);
}

void OnReviveCommitThunk(const net::GameEvent& ev) {
    if (g_self) g_self->OnReviveCommit(ev);
}

void __cdecl CmdLivesThunk() {
    if (g_self) g_self->DumpLives();
}

DH2_MODULE(AnimalsModule)

}  // namespace

void HostTick(uint32_t nowMs) {
    if (g_self) g_self->HostTick(nowMs);
}
void OnAnimalStates(const net::AnimalStates& states, uint32_t localMs) {
    if (g_self) g_self->OnAnimalStates(states, localMs);
}
void OnAnimalHit(const net::AnimalHit& hit) {
    if (g_self) g_self->OnAnimalHit(hit);
}
void OnAnimalDied(const net::GameEvent& ev) {
    if (g_self) g_self->NoteDeath(ev);
}

void HoldCutscene(uint16_t id) {
    if (!g_self) return;
    g_self->HoldCutscene(id);
}

void HoldCutsceneNamed(const char* name) {
    if (g_self) g_self->HoldCutsceneNamed(name);
}
void Reset() {
    if (g_self) g_self->Reset();
}
void SetSessionActive(bool active, bool hosting) {
    if (g_self) g_self->SetSessionActive(active, hosting);
}
void* Find(uint16_t id) { return FindAnimal(id); }
int IdOf(void* animal) {
    int16_t id = -1;
    if (!animal || !core::safe_read(static_cast<char*>(animal) + kEntId, id)) return -1;
    return id;
}
bool DebugDamage(uint16_t id, float damage) {
    void* ent = FindAnimal(id);
    if (!ent) {
        spdlog::warn("animals: coop_kill id={} no entity", id);
        return false;
    }
    if (IsCobra(ent)) {
        if (g_self && g_self->IsClientMirror()) {
            // A health drop is what the mirror turns into AnimalHit. The host starts the death timer.
            float before = 0.f, after = 0.f;
            core::safe_read(static_cast<char*>(ent) + kEntHealth, before);
            const bool ok = entity::InflictDamage(ent, damage);
            core::safe_read(static_cast<char*>(ent) + kEntHealth, after);
            spdlog::info("animals: coop_kill cobra id={} dmg={:.1f} hp {:.1f}->{:.1f} ok={} (client reports the hit)",
                         id, damage, before, after, ok);
            return ok;
        }
        StartCobraDeath(ent, "coop_kill");
        return true;
    }
    float before = 0.f, after = 0.f;
    core::safe_read(static_cast<char*>(ent) + kEntHealth, before);
    const bool ok = entity::InflictDamage(ent, damage);
    core::safe_read(static_cast<char*>(ent) + kEntHealth, after);
    if (after < 0.f) {
        *reinterpret_cast<float*>(static_cast<char*>(ent) + kEntHealth) = 0.f;  // as OnHurt: dead = exactly 0
        after = 0.f;
    }
    // On a client this is a simulated bullet on the mirror: the host decides the death. Host/solo: present it.
    if (after <= 0.f && before > 0.f && (!g_self || !g_self->IsClientMirror())) entity::PresentDeath(ent);
    spdlog::info("animals: coop_kill id={} dmg={:.1f} hp {:.1f}->{:.1f} ok={}", id, damage, before, after, ok);
    return ok;
}

}  // namespace dh2::coop::animals
