// Boss fights and scripted beats with a special animation (re/notes/yeti-bosses.md). Overview in coop/bosses.h.
// Engine facts (CommonDll7 / GameDll7 v1.0 RVAs, decompiled in slot W):
//   - AITaskNPC_PlayAnim (GameDll7): Update 0xb5b10, thiscall(task, dt). Task +0xa0 = clip pending, +0xb8 = the NPC,
//     +0xc0 = clip name, +0x100 / +0x104 = "play_sa" flag / partner. When it starts the clip it clears +0xa0, holsters
//     (0x55430(npc, 0), 0x54f80(npc)), sets the NPC state "NoChange" (0x5de00), then either PlaySA(clip, partner)
//     (story.cpp kPlaySA covers that) or SetAnim(clip) (vt +0x174) on the NPC's active item (vt +0x1b8, 0x552f0) or,
//     without one, on the NPC itself.
//   - cObjAIHuntMoveObject (CommonDll7 vtable 0x1342c0, BSP obj_move_object, MAP12B msn12faltree): a character. Big
//     Grimm's AttackObjectMovers TryMoveObject (0xc9ea0) builds a basis from the tree's angles (0x15230) and position
//     and calls PlaySA(clip, basis) on the tree (its override 0xb5ff0) and on the bear. The tree's OnEndSA override
//     (0xb6130) moves it to its fallen pose.
//   - AttackThrowers ProcessThrow (CommonDll7 0xc9740, fastcall(task)): creates obj_throw_object (rock1.skin) in the
//     yeti's left hand, detaches it (task +0x17c = 1), Throw (0xb4c10: +0x1f0 target, +0x1e4 damage, +0x1e8 impulse)
//     and an impulse. The rock's physics callback (0xb43b0, cdecl(rock, other)) hurts the target once when it touches
//     it (+0x1dc armed). task +8 -> +0x68 = the yeti; its brain +0x254 = target id.
#include "coop/bosses.h"

#include "coop/animals.h"
#include "engine/console_cmds.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "coop/coop.h"
#include "coop/events.h"
#include "coop/player_tracker.h"
#include "coop/puppets.h"
#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/entity_api.h"

namespace dh2::coop::bosses {
namespace {

constexpr uintptr_t kEntId = 0x04;
constexpr uintptr_t kEntPos = 0xE8;
constexpr uintptr_t kEntAngle = 0xF4;
constexpr uintptr_t kEntPhys = 0x118;
constexpr uintptr_t kEntBrain = 0x200;
constexpr uintptr_t kEntName = 0x198;

// GameDll7 AITaskNPC_PlayAnim
constexpr uintptr_t kRvaNpcPlayAnimUpdate = 0xb5b10;
constexpr uintptr_t kRvaNpcHolster = 0x55430;     // thiscall(npc, int)
constexpr uintptr_t kRvaNpcResetAim = 0x54f80;    // fastcall(npc)
constexpr uintptr_t kRvaNpcSetState = 0x5de00;    // thiscall(npc, const char*)
constexpr uintptr_t kRvaNpcActiveItem = 0x552f0;  // fastcall(npc) -> item or null
constexpr uintptr_t kTaskPending = 0xa0;
constexpr uintptr_t kTaskNpc = 0xb8;
constexpr uintptr_t kTaskClip = 0xc0;
constexpr uintptr_t kTaskPlaySA = 0x100;
constexpr uintptr_t kTaskPartner = 0x104;
// CommonDll7 cObjAIHuntMoveObject
constexpr uintptr_t kRvaMoverVt = 0x1342c0;
constexpr uintptr_t kRvaMoverPlaySA = 0xb5ff0;
constexpr uintptr_t kRvaMoverEndSA = 0xb6130;
constexpr uintptr_t kRvaBasisFromAngles = 0x15230;  // thiscall(m4x4*, a, b, c), no SEH frame
// CommonDll7 thrower
constexpr uintptr_t kRvaProcessThrow = 0xc9740;
constexpr uintptr_t kRvaRockThrow = 0xb4c10;
constexpr uintptr_t kRvaRockCollision = 0xb43b0;
constexpr uintptr_t kTaskAnimalRef = 0x08;  // -> +0x68 animal
constexpr uintptr_t kTaskThrown = 0x17c;
constexpr uintptr_t kTaskRock = 0x180;
constexpr uintptr_t kRockArmed = 0x1dc;
constexpr uintptr_t kRockTarget = 0x1f0;
constexpr uintptr_t kBodyVel = 0x34;  // rigid body (phys +4) linear velocity
constexpr uintptr_t kBrainTarget = 0x254;

const uint8_t kSehPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
const uint8_t kBasisPrologue[7] = {0x83, 0xec, 0x08, 0xd9, 0x44, 0x24, 0x10};

bool g_npc_anims = true;   // [Bosses] NpcAnims
bool g_props = true;       // [Bosses] PropSA
bool g_throw_copies = true;  // [Bosses] ThrowCopies
bool g_throw_hits = true;  // [Bosses] ThrowHitsPuppets
uintptr_t g_game = 0;
uintptr_t g_common = 0;
bool g_in_apply = false;

safetyhook::InlineHook* g_npc_update = nullptr;
safetyhook::InlineHook* g_mover_playsa = nullptr;
safetyhook::InlineHook* g_mover_endsa = nullptr;
safetyhook::InlineHook* g_process_throw = nullptr;
safetyhook::InlineHook* g_rock_collision = nullptr;

uint32_t now_ms() { return GetTickCount(); }

int16_t IdOf(void* ent) {
    int16_t id = -1;
    if (ent) core::safe_read(static_cast<char*>(ent) + kEntId, id);
    return id;
}

std::string NameOf(void* ent) {
    char* name = nullptr;
    if (!ent || !core::safe_read(static_cast<char*>(ent) + kEntName, name) || !name) return {};
    return core::safe_cstr(name, 48);
}

const char* Role() { return NetIsHost() ? "host" : (NetIsOnline() ? "client" : "solo"); }

bool IsWorldNpc(void* ent) {
    if (!ent || puppets::Owns(ent)) return false;
    for (void* n : Npcs()) {
        if (n == ent) return true;
    }
    return false;
}

void* CheckedRva(uintptr_t base, uintptr_t rva, const uint8_t* bytes, size_t n, const char* label) {
    if (!base) return nullptr;
    void* p = reinterpret_cast<void*>(base + rva);
    uint8_t got[8] = {};
    if (!core::safe_copy(p, got, n) || std::memcmp(got, bytes, n) != 0) {
        spdlog::error("bosses: {} at +{:#x} has unexpected bytes; off", label, rva);
        return nullptr;
    }
    return p;
}

// ---- SEH-guarded engine calls (no C++ objects in these) ----

using NpcIntFn = void(__thiscall*)(void* npc, int v);
using NpcFn = void(__fastcall*)(void* npc);
using NpcStrFn = void(__thiscall*)(void* npc, const char* s);
using NpcItemFn = void*(__fastcall*)(void* npc);
using StrVtFn = void(__thiscall*)(void* obj, const char* s);
using MoverPlayFn = void(__thiscall*)(void* self, char* clip, float* basis);
using MoverEndFn = void(__thiscall*)(void* self);
using BasisFn = void(__thiscall*)(float* m, float a, float b, float c);
using ThrowFn = void(__thiscall*)(void* rock, void* target, float dmg, float imp);
using CollisionFn = void(__cdecl*)(void* rock, void* other);
using VecVtFn = void(__thiscall*)(void* obj, const float* v);

void* vt_slot(void* obj, uintptr_t off) { return (*reinterpret_cast<void***>(obj))[off / sizeof(void*)]; }

// The task's own start of a clip (GameDll7 0xb5b10 without its bookkeeping). 1 ok, 0 missing function, -1 fault.
int guarded_npc_clip(void* npc, const char* clip, void** item_out) {
    if (!g_game) return 0;
    __try {
        reinterpret_cast<NpcIntFn>(g_game + kRvaNpcHolster)(npc, 0);
        reinterpret_cast<NpcFn>(g_game + kRvaNpcResetAim)(npc);
        reinterpret_cast<NpcStrFn>(g_game + kRvaNpcSetState)(npc, "NoChange");
        void* item = reinterpret_cast<NpcItemFn>(g_game + kRvaNpcActiveItem)(npc);
        *item_out = item;
        void* target = item ? item : npc;
        reinterpret_cast<StrVtFn>(vt_slot(target, 0x174))(target, clip);
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

void* guarded_npc_item(void* npc) {
    if (!g_game) return nullptr;
    __try {
        return reinterpret_cast<NpcItemFn>(g_game + kRvaNpcActiveItem)(npc);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool guarded_basis(float* m, float a, float b, float c) {
    __try {
        reinterpret_cast<BasisFn>(g_common + kRvaBasisFromAngles)(m, a, b, c);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_mover_play(void* self, char* clip, float* basis) {
    __try {
        g_mover_playsa->unsafe_thiscall<void>(self, clip, basis);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_mover_end(void* self) {
    __try {
        g_mover_endsa->unsafe_thiscall<void>(self);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_vt_str(void* obj, uintptr_t off, const char* s) {
    __try {
        reinterpret_cast<StrVtFn>(vt_slot(obj, off))(obj, s);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_vt_vec(void* obj, uintptr_t off, const float* v) {
    __try {
        reinterpret_cast<VecVtFn>(vt_slot(obj, off))(obj, v);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_throw(void* rock, void* target) {
    __try {
        reinterpret_cast<ThrowFn>(g_common + kRvaRockThrow)(rock, target, 0.f, 0.f);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_collision(void* rock, void* other) {
    __try {
        g_rock_collision->unsafe_ccall<void>(rock, other);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Rigid body of an entity's physics object (phys +4), or null.
char* RigidBody(void* ent) {
    char* phys = nullptr;
    char* body = nullptr;
    if (!ent || !core::safe_read(static_cast<char*>(ent) + kEntPhys, phys) || !phys) return nullptr;
    if (!core::safe_read(phys + 4, body)) return nullptr;
    return body;
}

// ======================= NPC PlayAnim beats =======================

struct NpcHold {
    std::string clip;
    uint32_t since = 0;
    bool seen_nochange = false;
    std::string last_state;
};
std::map<uint16_t, NpcHold> g_holds;      // client: NPC id -> the host clip it holds
std::map<uint16_t, int> g_npc_serial;     // host: clips committed per NPC (journal key suffix)
std::map<uint16_t, std::string> g_npc_clip_keep;  // the engine may keep the clip pointer
int g_npc_logs = 0;

void __fastcall hk_NpcPlayAnimUpdate(void* task, void*, float dt) {
    int pending = 0;
    core::safe_read(static_cast<char*>(task) + kTaskPending, pending);
    g_npc_update->unsafe_thiscall<void>(task, dt);
    if (!pending) return;
    int still = 1;
    if (!core::safe_read(static_cast<char*>(task) + kTaskPending, still) || still != 0) return;
    // The task started its clip this tick.
    void* npc = nullptr;
    int play_sa = 0;
    void* partner = nullptr;
    core::safe_read(static_cast<char*>(task) + kTaskNpc, npc);
    core::safe_read(static_cast<char*>(task) + kTaskPlaySA, play_sa);
    core::safe_read(static_cast<char*>(task) + kTaskPartner, partner);
    const std::string clip = core::safe_cstr(static_cast<char*>(task) + kTaskClip, 48);
    const int16_t id = IdOf(npc);
    if (!npc || id < 0 || clip.empty() || clip[0] == '<') return;
    const bool paired = play_sa != 0 && partner != nullptr;
    if (g_npc_logs++ < 60)
        spdlog::info("bosses: [{}] npc PlayAnim id={} '{}' name='{}' paired={} item={} state='{}' mirror={}", Role(), id,
                     clip, NameOf(npc), paired, guarded_npc_item(npc) != nullptr, entity::BodyStateName(npc),
                     NetIsOnline() && !NetIsHost() && IsWorldNpc(npc));
    // A paired clip goes through cObjCharacterEx::PlaySA(char*, char*), which story.cpp already commits (kPlaySA).
    if (paired || !g_npc_anims || !NetIsHost() || !NetIsOnline() || !IsWorldNpc(npc)) return;
    const int n = ++g_npc_serial[static_cast<uint16_t>(id)];
    const std::string key = fmt::format("{}#{}", clip.substr(0, 26), n);
    events::Commit(events::kNpcAnim, static_cast<uint16_t>(id), 0, 0.f, key, 0);
}

void OnNpcAnimCommit(const net::GameEvent& ev) {
    if (!g_npc_anims || !events::SameMapAsHost()) return;
    const size_t hash = ev.key.rfind('#');
    std::string& clip = g_npc_clip_keep[ev.subjectId];
    clip = ev.key.substr(0, hash);
    void* npc = EntityById(ev.subjectId);
    if (!npc || clip.empty() || !IsWorldNpc(npc)) {
        spdlog::warn("bosses: npc anim id={} '{}': no local NPC", ev.subjectId, ev.key);
        return;
    }
    const bool has = entity::HasClip(npc, clip.c_str());
    void* item = nullptr;
    int r = 0;
    if (has) {
        g_in_apply = true;
        r = guarded_npc_clip(npc, clip.c_str(), &item);
        g_in_apply = false;
        if (r != 1) {
            // Plain fallback: the clip on every part of the body that has it.
            entity::SetBodyState(npc, "NoChange", true);
            r = entity::PlayClip(npc, clip.c_str()) > 0 ? 2 : r;
        }
    }
    NpcHold& h = g_holds[ev.subjectId];
    h.clip = clip;
    h.since = now_ms();
    h.seen_nochange = false;
    h.last_state.clear();
    spdlog::info("bosses: applied npc anim id={} '{}' name='{}' has={} item={} r={} state='{}' clip0='{}'", ev.subjectId,
                 clip, NameOf(npc), has, item != nullptr, r, entity::BodyStateName(npc), entity::PartClipName(npc, 0));
}

// ======================= boss props (MAP12B falling trees) =======================

struct MoverPlay {
    std::string clip;      // kept alive: the engine may keep the pointer
    uint32_t started = 0;  // local clock of the applied start (client) or of the start (host)
    uint32_t end_at = 0;   // client: run OnEndSA at this time (0 = not asked yet)
    bool ended = false;
};
std::map<uint16_t, MoverPlay> g_movers;

bool IsMover(void* ent) {
    uintptr_t vt = 0;
    return ent && g_common && core::safe_read(ent, vt) && vt == g_common + kRvaMoverVt;
}

void __fastcall hk_MoverPlaySA(void* self, void*, char* clip, float* basis) {
    g_mover_playsa->unsafe_thiscall<void>(self, clip, basis);
    if (g_in_apply) return;
    const int16_t id = IdOf(self);
    const std::string c = core::safe_cstr(clip, 40);
    spdlog::info("bosses: [{}] mover id={} PlaySA '{}' pos=({:.1f},{:.1f},{:.1f})", Role(), id, c,
                 *reinterpret_cast<float*>(static_cast<char*>(self) + kEntPos),
                 *reinterpret_cast<float*>(static_cast<char*>(self) + kEntPos + 4),
                 *reinterpret_cast<float*>(static_cast<char*>(self) + kEntPos + 8));
    if (!g_props || id < 0 || !NetIsHost() || !NetIsOnline()) return;
    MoverPlay& m = g_movers[static_cast<uint16_t>(id)];
    m.clip = c;
    m.started = now_ms();
    events::Commit(events::kPropSA, static_cast<uint16_t>(id), 0, 0.f, c, 0);
}

void __fastcall hk_MoverEndSA(void* self, void*) {
    g_mover_endsa->unsafe_thiscall<void>(self);
    if (g_in_apply) return;
    const int16_t id = IdOf(self);
    spdlog::info("bosses: [{}] mover id={} OnEndSA pos=({:.1f},{:.1f},{:.1f})", Role(), id,
                 *reinterpret_cast<float*>(static_cast<char*>(self) + kEntPos),
                 *reinterpret_cast<float*>(static_cast<char*>(self) + kEntPos + 4),
                 *reinterpret_cast<float*>(static_cast<char*>(self) + kEntPos + 8));
    if (!g_props || id < 0 || !NetIsHost() || !NetIsOnline()) return;
    const auto it = g_movers.find(static_cast<uint16_t>(id));
    const float secs = it != g_movers.end() ? static_cast<float>(now_ms() - it->second.started) / 1000.f : 0.f;
    events::Commit(events::kPropSAEnd, static_cast<uint16_t>(id), 0, secs,
                   it != g_movers.end() ? it->second.clip : std::string("end"), 0);
}

void OnPropSACommit(const net::GameEvent& ev) {
    if (!g_props || !g_mover_playsa || !events::SameMapAsHost()) return;
    void* ent = EntityById(ev.subjectId);
    if (!IsMover(ent) || ev.key.empty()) {
        spdlog::warn("bosses: prop SA id={} '{}': no local tree", ev.subjectId, ev.key);
        return;
    }
    MoverPlay& m = g_movers[ev.subjectId];
    if (m.started && !m.ended) return;  // already falling here
    m = MoverPlay{};
    m.clip = ev.key;
    // TryMoveObject's basis: rotation from the tree's own angles (+0xfc, +0xf4, +0xf8), row 3 = its position.
    float basis[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};
    float ang[3] = {}, pos[3] = {};
    core::safe_copy(static_cast<char*>(ent) + kEntAngle, ang, sizeof ang);
    core::safe_copy(static_cast<char*>(ent) + kEntPos, pos, sizeof pos);
    const bool built = guarded_basis(basis, ang[2], ang[0], ang[1]);
    basis[12] = pos[0];
    basis[13] = pos[1];
    basis[14] = pos[2];
    g_in_apply = true;
    const bool ok = guarded_mover_play(ent, m.clip.data(), basis);
    g_in_apply = false;
    m.started = now_ms();
    spdlog::info("bosses: applied prop SA id={} '{}' basis={} ok={} clip0='{}'", ev.subjectId, m.clip, built, ok,
                 entity::PartClipName(ent, 0));
}

void OnPropSAEndCommit(const net::GameEvent& ev) {
    if (!g_props || !events::SameMapAsHost()) return;
    const auto it = g_movers.find(ev.subjectId);
    if (it == g_movers.end() || !it->second.started) {
        spdlog::warn("bosses: prop SA end id={}: never started here", ev.subjectId);
        return;
    }
    // At the host's moment in the clip: OnEndSA turns the last frame into the fallen pose. A late joiner gets both
    // commits at once and must let its clip play first.
    const uint32_t len = static_cast<uint32_t>(std::max(0.f, ev.f0) * 1000.f);
    it->second.end_at = it->second.started + len;
    if (static_cast<int32_t>(it->second.end_at - now_ms()) < 0) it->second.end_at = now_ms();
    spdlog::info("bosses: prop SA end id={} in {} ms", ev.subjectId, static_cast<int32_t>(it->second.end_at - now_ms()));
}

void PumpMovers() {
    const uint32_t now = now_ms();
    for (auto& [id, m] : g_movers) {
        if (m.ended || !m.end_at || static_cast<int32_t>(now - m.end_at) < 0) continue;
        m.ended = true;
        void* ent = EntityById(id);
        if (!IsMover(ent)) continue;
        g_in_apply = true;
        const bool ok = guarded_mover_end(ent);
        g_in_apply = false;
        spdlog::info("bosses: applied prop SA end id={} ok={} pos=({:.1f},{:.1f},{:.1f})", id, ok,
                     *reinterpret_cast<float*>(static_cast<char*>(ent) + kEntPos),
                     *reinterpret_cast<float*>(static_cast<char*>(ent) + kEntPos + 4),
                     *reinterpret_cast<float*>(static_cast<char*>(ent) + kEntPos + 8));
    }
}

// ======================= yeti throws =======================

struct Rock {
    void* ent = nullptr;
    int16_t id = -1;
    void* target = nullptr;  // host: the puppet it was thrown at
    uint8_t player = 0xFF;
    uint32_t t0 = 0;
    bool client_copy = false;
    float best_d = 1e9f;     // host: closest horizontal distance to the target so far (for the drop log)
    float best_dy = 0.f;
    float prev[3] = {};      // host: rock position at the previous pump
    bool have_prev = false;
    uint32_t last_ms = 0;
};
int g_drop_logs = 0;
std::vector<Rock> g_rocks;
int g_throw_logs = 0;
int g_collision_logs = 0;

void* AnimalOfTask(void* task) {
    char* ref = nullptr;
    void* animal = nullptr;
    if (!core::safe_read(static_cast<char*>(task) + kTaskAnimalRef, ref) || !ref) return nullptr;
    core::safe_read(ref + 0x68, animal);
    return animal;
}

void* TargetOf(void* animal) {
    char* brain = nullptr;
    int target = -1;
    if (!animal || !core::safe_read(static_cast<char*>(animal) + kEntBrain, brain) || !brain) return nullptr;
    if (!core::safe_read(brain + kBrainTarget, target) || target < 0) return nullptr;
    return EntityById(target);
}

void __fastcall hk_ProcessThrow(void* task) {
    int thrown_before = 1;
    core::safe_read(static_cast<char*>(task) + kTaskThrown, thrown_before);
    g_process_throw->unsafe_fastcall<void>(task);
    int thrown = 0;
    if (thrown_before != 0 || !core::safe_read(static_cast<char*>(task) + kTaskThrown, thrown) || thrown == 0) return;
    void* rock = nullptr;
    core::safe_read(static_cast<char*>(task) + kTaskRock, rock);
    void* animal = AnimalOfTask(task);
    void* target = TargetOf(animal);
    float pos[3] = {}, vel[3] = {};
    core::safe_copy(static_cast<char*>(rock) + kEntPos, pos, sizeof pos);
    if (char* body = RigidBody(rock)) core::safe_copy(body + kBodyVel, vel, sizeof vel);
    const uint8_t who = puppets::PlayerOf(target);
    if (g_throw_logs++ < 40)
        spdlog::info("bosses: [{}] throw by id={} rock id={} at {} (player {}) pos=({:.2f},{:.2f},{:.2f}) "
                     "vel=({:.2f},{:.2f},{:.2f})",
                     Role(), IdOf(animal), IdOf(rock), target == LocalPlayer().entity ? "hunter" : NameOf(target),
                     who == net::kInvalidPlayerId ? -1 : who, pos[0], pos[1], pos[2], vel[0], vel[1], vel[2]);
    if (!NetIsHost() || !NetIsOnline() || !rock) return;
    if (g_throw_hits && who != net::kInvalidPlayerId) {
        Rock r;
        r.ent = rock;
        r.id = IdOf(rock);
        r.target = target;
        r.player = who;
        r.t0 = now_ms();
        g_rocks.push_back(r);
    }
    if (g_throw_copies) {
        const auto pack = [](float v) {
            const float c = std::fmax(-327.f, std::fmin(327.f, v)) * 100.f;
            return static_cast<uint32_t>(static_cast<uint16_t>(static_cast<int16_t>(std::lround(c))));
        };
        const uint32_t vxz = pack(vel[0]) | (pack(vel[2]) << 16);
        events::Send(events::kBossThrow, static_cast<uint16_t>(IdOf(animal)), vxz, vel[1],
                     fmt::format("{:.2f} {:.2f} {:.2f}", pos[0], pos[1], pos[2]), 0);
    }
}

// Diagnostics and the puppet case: the rock's physics callback.
void __cdecl hk_RockCollision(void* rock, void* other) {
    if (g_collision_logs < 20) {
        void* target = nullptr;
        int armed = 0;
        core::safe_read(static_cast<char*>(rock) + kRockTarget, target);
        core::safe_read(static_cast<char*>(rock) + kRockArmed, armed);
        if (other == target || puppets::PlayerOf(other) != net::kInvalidPlayerId) {
            ++g_collision_logs;
            spdlog::info("bosses: [{}] rock id={} touched id={} (target={} player {}) armed={}", Role(), IdOf(rock),
                         IdOf(other), other == target, puppets::PlayerOf(other), armed);
        }
    }
    g_rock_collision->unsafe_ccall<void>(rock, other);
}

// Host: a rock thrown at a puppet. The puppet's body does not collide with characters' bodies (PlaySA collision
// words), so a hit is decided here: an armed rock inside the puppet's 1.9 m x 0.8 m capsule runs the engine's own
// hit (the callback above), which reaches the puppet's OnHurt and goes to that player (combat.cpp kPlayerHurt).
// The pump runs once per frame and a rock moves ~20 m/s: at 10-20 fps (two windows) it steps 1-2 m, wider than the
// capsule, and the rock was destroyed by the ground before a sample fell inside (2026-10-18: 3 of 4 rocks missed at
// 0.9-1.0 m). So the path from the previous sample to where the rock will be at the next one (velocity x the last
// frame time) is tested in 0.25 m steps, while the rock still exists.
constexpr float kCapsuleRadius = 0.8f;
constexpr float kCapsuleBelow = 0.3f;
constexpr float kCapsuleHeight = 2.0f;

bool PathHitsCapsule(const float a[3], const float b[3], const float pp[3]) {
    const float len = std::sqrt((b[0] - a[0]) * (b[0] - a[0]) + (b[1] - a[1]) * (b[1] - a[1]) +
                                (b[2] - a[2]) * (b[2] - a[2]));
    const int steps = std::max(1, std::min(64, static_cast<int>(std::ceil(len / 0.25f))));
    for (int s = 0; s <= steps; ++s) {
        const float f = static_cast<float>(s) / static_cast<float>(steps);
        const float x = a[0] + (b[0] - a[0]) * f - pp[0], y = a[1] + (b[1] - a[1]) * f - pp[1],
                    z = a[2] + (b[2] - a[2]) * f - pp[2];
        if (x * x + z * z < kCapsuleRadius * kCapsuleRadius && y > -kCapsuleBelow && y < kCapsuleHeight) return true;
    }
    return false;
}

void PumpRockHits() {
    if (g_rocks.empty()) return;
    const uint32_t now = now_ms();
    for (size_t i = 0; i < g_rocks.size();) {
        Rock& r = g_rocks[i];
        int armed = 0;
        const bool alive = r.id >= 0 && EntityById(r.id) == r.ent &&
                           core::safe_read(static_cast<char*>(r.ent) + kRockArmed, armed);
        if (!alive || armed != 1 || now - r.t0 > 6000 || !puppets::IsLivingPuppet(r.target)) {
            if (g_drop_logs++ < 12)
                spdlog::info("bosses: rock id={} for player {} dropped after {} ms: alive={} armed={} puppet alive={} "
                             "closest {:.2f} m (dy {:.2f})",
                             r.id, r.player, now - r.t0, alive, armed, puppets::IsLivingPuppet(r.target),
                             r.best_d < 1e8f ? r.best_d : -1.f, r.best_dy);
            g_rocks.erase(g_rocks.begin() + static_cast<std::ptrdiff_t>(i));
            continue;
        }
        float rp[3] = {}, pp[3] = {}, vel[3] = {};
        core::safe_copy(static_cast<char*>(r.ent) + kEntPos, rp, sizeof rp);
        core::safe_copy(static_cast<char*>(r.target) + kEntPos, pp, sizeof pp);
        if (char* body = RigidBody(r.ent)) core::safe_copy(body + kBodyVel, vel, sizeof vel);
        const float dx = rp[0] - pp[0], dz = rp[2] - pp[2], dy = rp[1] - pp[1];
        const float hd = std::sqrt(dx * dx + dz * dz);
        if (hd < r.best_d) {
            r.best_d = hd;
            r.best_dy = dy;
        }
        // From the previous sample (this frame's position on the first one) to the next frame's.
        const float step = std::max(0.05f, std::min(0.3f, static_cast<float>(now - (r.last_ms ? r.last_ms : now)) / 1000.f));
        const float from[3] = {r.have_prev ? r.prev[0] : rp[0], r.have_prev ? r.prev[1] : rp[1],
                               r.have_prev ? r.prev[2] : rp[2]};
        const float to[3] = {rp[0] + vel[0] * step, rp[1] + vel[1] * step, rp[2] + vel[2] * step};
        const bool hit = PathHitsCapsule(from, rp, pp) || PathHitsCapsule(rp, to, pp);
        r.prev[0] = rp[0];
        r.prev[1] = rp[1];
        r.prev[2] = rp[2];
        r.have_prev = true;
        r.last_ms = now;
        if (hit) {
            const bool ok = guarded_collision(r.ent, r.target);
            spdlog::info("bosses: rock id={} reached player {} after {} ms: hit ok={}", r.id, r.player, now - r.t0, ok);
            g_rocks.erase(g_rocks.begin() + static_cast<std::ptrdiff_t>(i));
            continue;
        }
        ++i;
    }
}

// Client: a harmless copy of the host's rock, from the same spot with the same velocity.
void OnBossThrowCommit(const net::GameEvent& ev) {
    if (!g_throw_copies || !g_common || !events::SameMapAsHost()) return;
    float pos[3] = {};
    if (std::sscanf(ev.key.c_str(), "%f %f %f", &pos[0], &pos[1], &pos[2]) != 3) return;
    const auto unpack = [](uint32_t bits) { return static_cast<float>(static_cast<int16_t>(bits & 0xFFFF)) / 100.f; };
    const float vel[3] = {unpack(ev.i0), ev.f0, unpack(ev.i0 >> 16)};
    void* mgr = EntityManager();
    void* rock = mgr ? entity::Create("obj_throw_object") : nullptr;
    if (!rock) {
        spdlog::warn("bosses: throw copy: no obj_throw_object");
        return;
    }
    const int idx = entity::AddEntity(mgr, rock);
    const bool model = guarded_vt_str(rock, 0x68, "objects\\world\\rock1\\rock1.skin");
    const bool spawned = entity::Spawn(rock);
    const bool moved = guarded_vt_vec(rock, 0xa4, pos);
    // Throw arms the rock (+0x1d8, timers); its hit stays off (+0x1dc = 0, target = itself): only the host's rock hurts.
    const bool thrown = guarded_throw(rock, rock);
    const int zero = 0;
    std::memcpy(static_cast<char*>(rock) + kRockArmed, &zero, sizeof zero);
    bool vel_ok = false;
    if (char* body = RigidBody(rock)) vel_ok = core::safe_copy(vel, body + kBodyVel, sizeof vel);
    Rock r;
    r.ent = rock;
    r.id = IdOf(rock);
    r.t0 = now_ms();
    r.client_copy = true;
    g_rocks.push_back(r);
    spdlog::info("bosses: throw copy by id={} rock id={} idx={} model={} spawn={} move={} throw={} vel={} "
                 "pos=({:.2f},{:.2f},{:.2f}) vel=({:.2f},{:.2f},{:.2f})",
                 ev.subjectId, r.id, idx, model, spawned, moved, thrown, vel_ok, pos[0], pos[1], pos[2], vel[0], vel[1],
                 vel[2]);
}

// Client: where the copies went (first second of each, for the log).
void PumpRockCopies() {
    const uint32_t now = now_ms();
    for (size_t i = 0; i < g_rocks.size();) {
        Rock& r = g_rocks[i];
        if (!r.client_copy) {
            ++i;
            continue;
        }
        if (now - r.t0 >= 700) {
            float p[3] = {};
            const bool alive = r.id >= 0 && EntityById(r.id) == r.ent;
            if (alive) core::safe_copy(static_cast<char*>(r.ent) + kEntPos, p, sizeof p);
            spdlog::info("bosses: throw copy rock id={} after {} ms alive={} pos=({:.2f},{:.2f},{:.2f})", r.id, now - r.t0,
                         alive, p[0], p[1], p[2]);
            g_rocks.erase(g_rocks.begin() + static_cast<std::ptrdiff_t>(i));
            continue;
        }
        ++i;
    }
}

// Test: coop_hurtname <target name>:<damage> hurts an animal by script name (hogzilla1, pbear, yeti) the way
// coop_kill does by id. On a client it is a hit on the mirror, which goes to the host as AnimalHit (the animal then
// turns on this player).
void __cdecl cmd_hurtname(const char* arg) {
    const std::string a = core::safe_cstr(arg, 64);
    const size_t colon = a.find(':');
    const std::string name = a.substr(0, colon);
    const float dmg = colon == std::string::npos ? 10.f : static_cast<float>(std::atof(a.c_str() + colon + 1));
    for (void* ent : Animals()) {
        if (NameOf(ent) != name) continue;
        spdlog::info("bosses: coop_hurtname '{}' id={} dmg={:.1f}", name, IdOf(ent), dmg);
        // As a bullet's OnHit: +0x420 = MISSION_ACTION_KILL, so a kill counts for the mission (re/notes/scale.md).
        if (!NetIsOnline() || NetIsHost()) {
            const int kill = 1;
            std::memcpy(static_cast<char*>(ent) + 0x420, &kill, sizeof kill);
        }
        animals::DebugDamage(static_cast<uint16_t>(IdOf(ent)), dmg);
        return;
    }
    spdlog::warn("bosses: coop_hurtname '{}': no such animal", name);
}

// ======================= module =======================

class BossesModule : public dh2::Module {
public:
    const char* Name() const override { return "bosses"; }
    int Priority() const override { return 48; }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        g_npc_anims = GetPrivateProfileIntA("Bosses", "NpcAnims", 1, ini.c_str()) != 0;
        g_props = GetPrivateProfileIntA("Bosses", "PropSA", 1, ini.c_str()) != 0;
        g_throw_copies = GetPrivateProfileIntA("Bosses", "ThrowCopies", 1, ini.c_str()) != 0;
        g_throw_hits = GetPrivateProfileIntA("Bosses", "ThrowHitsPuppets", 1, ini.c_str()) != 0;
        events::SetHandler(events::kNpcAnim, nullptr, &OnNpcAnimCommit);
        events::SetHandler(events::kPropSA, nullptr, &OnPropSACommit);
        events::SetHandler(events::kPropSAEnd, nullptr, &OnPropSAEndCommit);
        events::SetHandler(events::kBossThrow, nullptr, &OnBossThrowCommit);
        spdlog::info("bosses: npc anims={} prop SA={} throw copies={} throw hits={}", g_npc_anims, g_props,
                     g_throw_copies, g_throw_hits);
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_game = reinterpret_cast<uintptr_t>(game);
        if (void* p = CheckedRva(g_game, kRvaNpcPlayAnimUpdate, kSehPrologue, 5, "AITaskNPC_PlayAnim::Update"))
            g_npc_update = core::install_hook("AITaskNPC_PlayAnim::Update (bosses)", p,
                                              reinterpret_cast<void*>(&hk_NpcPlayAnimUpdate));
        // The client replays the task's own calls: check them too.
        if (!CheckedRva(g_game, kRvaNpcHolster, kSehPrologue, 5, "npc holster") ||
            !CheckedRva(g_game, kRvaNpcResetAim, kSehPrologue, 5, "npc reset aim") ||
            !CheckedRva(g_game, kRvaNpcSetState, kSehPrologue, 5, "npc set state") ||
            !CheckedRva(g_game, kRvaNpcActiveItem, kSehPrologue, 5, "npc active item")) {
            g_game = 0;  // guarded_npc_clip falls back to SetBodyState + PlayClip
        }
        HMODULE common = GetModuleHandleA("CommonDll7.dll");
        g_common = reinterpret_cast<uintptr_t>(common);
        if (void* p = CheckedRva(g_common, kRvaMoverPlaySA, kSehPrologue, 5, "cObjAIHuntMoveObject::PlaySA"))
            g_mover_playsa = core::install_hook("cObjAIHuntMoveObject::PlaySA (bosses)", p,
                                                reinterpret_cast<void*>(&hk_MoverPlaySA));
        if (void* p = CheckedRva(g_common, kRvaMoverEndSA, kSehPrologue, 5, "cObjAIHuntMoveObject::OnEndSA"))
            g_mover_endsa = core::install_hook("cObjAIHuntMoveObject::OnEndSA (bosses)", p,
                                               reinterpret_cast<void*>(&hk_MoverEndSA));
        if (void* p = CheckedRva(g_common, kRvaProcessThrow, kSehPrologue, 5, "AttackThrowers::ProcessThrow"))
            g_process_throw = core::install_hook("AttackThrowers::ProcessThrow (bosses)", p,
                                                 reinterpret_cast<void*>(&hk_ProcessThrow));
        if (void* p = CheckedRva(g_common, kRvaRockCollision, kSehPrologue, 5, "cObjAIHuntThrowObject::OnCollision"))
            g_rock_collision = core::install_hook("cObjAIHuntThrowObject::OnCollision (bosses)", p,
                                                  reinterpret_cast<void*>(&hk_RockCollision));
        if (!CheckedRva(g_common, kRvaBasisFromAngles, kBasisPrologue, sizeof kBasisPrologue, "basis from angles") ||
            !CheckedRva(g_common, kRvaRockThrow, kSehPrologue, 5, "cObjAIHuntThrowObject::Throw")) {
            g_props = false;
            g_throw_copies = false;
        }
    }

    void OnMapUnloading() override { Clear(); }
    void OnMapLoaded(const char*) override { Clear(); }

    void OnFrame(float) override {
        if (!cmd_registered_ && console_cmds::Ready())
            cmd_registered_ = console_cmds::AddCommandStr("coop_hurtname", &cmd_hurtname);
        if (!NetIsOnline()) return;
        if (NetIsHost()) {
            if (g_throw_hits) PumpRockHits();
            return;
        }
        PumpMovers();
        PumpRockCopies();
    }

private:
    bool cmd_registered_ = false;

    static void Clear() {
        g_holds.clear();
        g_npc_serial.clear();
        g_npc_clip_keep.clear();
        g_movers.clear();
        g_rocks.clear();
        g_npc_logs = 0;
        g_throw_logs = 0;
        g_collision_logs = 0;
        g_drop_logs = 0;
    }
};

DH2_MODULE(BossesModule)

}  // namespace

bool NpcAnimHold(uint16_t id, const std::string& hostState) {
    const auto it = g_holds.find(id);
    if (it == g_holds.end()) return false;
    NpcHold& h = it->second;
    if (hostState != h.last_state) {
        spdlog::info("bosses: npc id={} holds '{}': host state '{}'", id, h.clip, hostState);
        h.last_state = hostState;
    }
    // The task puts the NPC in NoChange for as long as the clip (and its last frame) should show.
    if (hostState == "NoChange") {
        h.seen_nochange = true;
        return true;
    }
    // The first samples after the commit can still be older than it (100 ms interpolation delay).
    if (!h.seen_nochange && now_ms() - h.since < 3000) return true;
    spdlog::info("bosses: npc id={} released from '{}' (host state '{}')", id, h.clip, hostState);
    g_holds.erase(it);
    return false;
}

}  // namespace dh2::coop::bosses
