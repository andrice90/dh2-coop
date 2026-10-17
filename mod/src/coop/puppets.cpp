#include "coop/puppets.h"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

#include <spdlog/spdlog.h>

#include "coop/coop.h"
#include "coop/player_tracker.h"
#include "core/hook.h"
#include "core/lifecycle.h"
#include "core/log.h"
#include "core/exports.h"
#include "core/safe_mem.h"
#include "core/module.h"
#include "engine/console_cmds.h"
#include "engine/entity_api.h"
#include "net/interp.h"

namespace dh2::coop::puppets {
namespace {

// Class for remote players: generic human (cObjHumanEx, "obj_humanEx"), no brain → no AI, no player logic.
// Overridable via [Puppets] Class= for experiments (e.g. obj_cschex).
constexpr const char* kDefaultClass = "obj_humanEx";
constexpr float kWalkSpeed = 0.6f;  // m/s: below = idle
constexpr float kRunSpeed = 5.5f;   // m/s: above = run (hunter walks ≈ 5 m/s)
bool g_physics = false;         // [Puppets] Physics=0: drop the puppet's physics body (see spawn_human)
float g_phys_key_y = 0.f;       // [Puppets] PhysicsKeyY: metres added to y in the "position" key when Physics=1
bool g_physy_cmd = false;
// Test: coop_physy <metres> changes PhysicsKeyY live (finding where the body's origin sits relative to the feet).
void __cdecl cmd_physy(const char* arg) {
    g_phys_key_y = static_cast<float>(std::atof(core::safe_cstr(arg, 16).c_str()));
    spdlog::info("puppets: PhysicsKeyY = {:.2f}", g_phys_key_y);
}

struct Puppet {
    net::SnapshotBuffer buffer;
    void* entity = nullptr;
    bool spawn_failed = false;
    std::string anim;
    float rel_box[6] = {};  // world AABB at spawn time (entity at origin) = model-relative AABB
    void* weapon = nullptr; // third-person weapon entity attached to the puppet's hand
    int weapon_kind = 0;    // net::WeaponKind currently attached
    bool weapon_failed = false;
    int last_shots = -1;
    uint32_t fire_until = 0;
    std::string arms_buf;
    std::string arms_shown;
    std::string gun_shown;
    bool attack_target = false;  // host: registered in the animals' attack-target list (Phase 5)
    bool was_knocked = false;    // last sample had kFlagKnocked: play the get-up when it clears
    uint32_t getup_until = 0;
    float body_half = 0.f;       // Physics=1: physics body half height (cPhysicsObject +0x78)
    Vec3 placed;                 // Physics=1 diagnostics: where place() last put the entity
    bool have_placed = false;
    uint32_t drift_log_ms = 0;
};

constexpr uintptr_t kEntWorldBox = 0xC4;  // float[6] min xyz, max xyz — used by cBspTree::UpdateEntity
constexpr uintptr_t kEntBspLink = 0x124;  // int, -1 = not linked into BSP leaves

void capture_rel_box(Puppet& p) {
    core::safe_copy(reinterpret_cast<char*>(p.entity) + kEntWorldBox, p.rel_box, sizeof p.rel_box);
    // Physics=1: the "position" key places the body's centre; its half extents are cPhysicsObject +0x74/+0x78/+0x7c
    // (0.625 / 0.900 / 0.625 on the guide body; +0x7c is the radius ProcessAttackStuck reads). Without lifting the
    // key by the half height the model was drawn 0.9 m into the ground (user 2026-10-17; A/B screenshots against
    // Physics=0 put the match between 0.9 and 1.0).
    char* phys = nullptr;
    if (g_physics && core::safe_read(reinterpret_cast<char*>(p.entity) + 0x118, phys) && phys)
        core::safe_read(phys + 0x78, p.body_half);
    // Entity kind byte +0xA7: GameDll's human spawn writes 4 (hunter), animals 0x42; a bare obj_humanEx stays 0.
    // Experiment: tag the puppet as a human. [Puppets] Kind=-1 leaves it untouched.
    const std::string ini = (core::self_dir() / "dh2coop.ini").string();
    const int kind = GetPrivateProfileIntA("Puppets", "Kind", -1, ini.c_str());
    if (kind >= 0 && kind < 256) {
        *(reinterpret_cast<unsigned char*>(p.entity) + 0xA7) = static_cast<unsigned char>(kind);
        spdlog::info("puppets: entity {:#x} kind byte +0xA7 := {}", reinterpret_cast<uintptr_t>(p.entity), kind);
    }
}

// Moves the puppet: engine SetPos/SetAngle, then refresh the world AABB from the spawn-relative box and
// relink into the BSP so the renderer's leaf culling sees it (runtime spawns are otherwise never drawn).
void place(Puppet& p, const entity::Vec3& pos, float yawDeg) {
    if (g_physics) {
        // [Puppets] Physics=1. Even frozen, the body writes its own position back onto the entity between our
        // placements, and the model is drawn from the body: without the "position" key the puppet stayed at the
        // map origin and was invisible (user, 2026-10-17 10:47; drift log = -target every frame). The key moves
        // the body, but the model then sat below the feet (10:45): the body's origin is not the feet.
        // The key places the body's centre, so it goes up by the body's half height (capture_rel_box), plus
        // g_phys_key_y ([Puppets] PhysicsKeyY, console coop_physy) as a manual trim, default 0.
        char buf[64];
        std::snprintf(buf, sizeof buf, "%.2f %.2f %.2f", pos.x * 100.f, (pos.y + p.body_half + g_phys_key_y) * 100.f,
                      pos.z * 100.f);
        entity::HandleKey(p.entity, "position", buf);
        Vec3 now{};
        const uint32_t t = GetTickCount();
        if (p.have_placed && t - p.drift_log_ms > 2000 &&
            core::safe_read(reinterpret_cast<char*>(p.entity) + 0xE8, now)) {
            p.drift_log_ms = t;
            // cPhysicsObject +4 = body, body +0x28 = position (cPhysicsObject::SetPosition, EngineDll 0xf9c40).
            char* phys = nullptr;
            char* body = nullptr;
            Vec3 bpos{};
            const bool have_body = core::safe_read(reinterpret_cast<char*>(p.entity) + 0x118, phys) && phys &&
                                   core::safe_read(phys + 4, body) && body && core::safe_read(body + 0x28, bpos);
            spdlog::info("puppets: physics drift since last place dy={:.2f} dx={:.2f} dz={:.2f} (target y {:.2f}) "
                         "body y {:.2f} (body - entity {:.2f})",
                         now.y - p.placed.y, now.x - p.placed.x, now.z - p.placed.z, pos.y,
                         have_body ? bpos.y : 0.f, have_body ? bpos.y - now.y : 0.f);
        }
        p.placed = {pos.x, pos.y, pos.z};
        p.have_placed = true;
    }
    entity::SetPos(p.entity, pos);
    entity::SetAngle(p.entity, {0.f, yawDeg, 0.f});
    float box[6] = {p.rel_box[0] + pos.x, p.rel_box[1] + pos.y, p.rel_box[2] + pos.z,
                    p.rel_box[3] + pos.x, p.rel_box[4] + pos.y, p.rel_box[5] + pos.z};
    float* dst = reinterpret_cast<float*>(reinterpret_cast<char*>(p.entity) + kEntWorldBox);
    for (int i = 0; i < 6; ++i) dst[i] = box[i];
    entity::BspUpdate(p.entity);
}

std::map<uint8_t, Puppet> g_puppets;

// Draw diagnostics: does the engine ever call cObjEntity::Draw (EngineDll7 0xbbd30) for our puppet?
safetyhook::InlineHook* g_draw_hook = nullptr;
std::atomic<void*> g_watch_puppet{nullptr};
std::atomic<void*> g_watch_ref{nullptr};
std::atomic<uint32_t> g_draw_total{0}, g_draw_puppet{0}, g_draw_ref{0};

void __fastcall hk_EntityDraw(void* self, void* /*edx*/) {
    g_draw_total.fetch_add(1, std::memory_order_relaxed);
    if (self == g_watch_puppet.load(std::memory_order_relaxed)) g_draw_puppet.fetch_add(1, std::memory_order_relaxed);
    if (self == g_watch_ref.load(std::memory_order_relaxed)) g_draw_ref.fetch_add(1, std::memory_order_relaxed);
    g_draw_hook->unsafe_thiscall<void>(self);
}
void* g_spawn_manager = nullptr;  // manager the puppets were added to (invalid after a map change)

std::string g_class = kDefaultClass;
bool g_enabled = true;
bool g_spike = false;           // [Puppets] Spike=1: one local test puppet circling in front of the hunter
bool g_anim_enabled = true;     // [Puppets] Animate=1
float g_y_offset = 0.f;         // [Puppets] YOffset (metres) added to the networked hunter y
std::string g_crouch_clip = "coveridle";  // [Puppets] CrouchClip: NPC packs have no crouch-walk clip
bool g_weapons = true;          // [Puppets] Weapons=1: attach a third-person weapon model to the puppet's hand
bool g_attack_target = false;   // [Puppets] AttackTarget=1: host animals may pick puppets as attack targets.
// Off: a tiger attacking a puppet crashed the host in cAIHuntBrain_Animal::ProcessAttackStuck (null +0x7c,
// MAP3B 2026-10-16 21:20). Puppets have no physics body; the attack code needs one. Phase 5 work.
void* g_spike_entity = nullptr;
float g_spike_t = 0.f;
float g_spike_wait = 0.f;
entity::Vec3 g_spike_center;

uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

// The player's own model (teenmm.skin) is only first-person arms ("fpshands1", teenmm1st.skin) — there is
// no third-person player body. Puppets use a full NPC body instead ([Puppets] Model=, "hunter" = player model).
// shotgun1.ap / handgun3.ap supply the arm clips. Entity SetAnim of a "shotgun1/..." name creates
// cBodyShotgun1npc (missing on MAP12C and fatal). Those clips are applied on the torso part only.
std::string g_model = "objects/npc/mountainguide/mountainguide.skin,game.ap,shotgun1.ap,handgun3.ap";

void* spawn_human(const char* why) {
    void* mgr = EntityManager();
    const char* model = g_model == "hunter" ? HunterModelName() : g_model.c_str();
    if (!mgr || !model || !*model) return nullptr;
    void* ent = entity::Create(g_class.c_str());
    if (!ent) return nullptr;
    const bool keyed = entity::HandleKey(ent, "model_name", model);
    const int index = entity::AddEntity(mgr, ent);
    const bool pre = entity::Precache(ent);
    const bool sp = entity::Spawn(ent);
    spdlog::info("puppets: spawned puppet ({}): entity {:#x} class '{}' model_key={} index={} precache={} spawn={}",
                 why, reinterpret_cast<uintptr_t>(ent), g_class, keyed, index, pre, sp);
    if (index < 0 || !sp) {
        spdlog::warn("puppets: spawn incomplete; puppet disabled for this map");
        return nullptr;
    }
    if (!g_physics) {
        // The physics step copies the body position back onto the entity every tick (puppet stuck at its
        // spawn point, pushing the local hunter). A puppet is a pure visual: drop its body.
        const bool ok = entity::UnlinkPhysics(ent);
        uint32_t phys = 0;
        core::safe_read(reinterpret_cast<char*>(ent) + 0x118, phys);
        spdlog::info("puppets: unlinked physics -> {} (phys +0x118 now {:#x})", ok, phys);
    } else {
        // Keep the body (the animal attack code reads it) but stop it colliding with characters. A solid puppet under
        // the local hunter pushed both players up without end (each side's push raised the other side's puppet: y
        // -2.9 to 65 in 15 s, MAP3B 2026-10-17 10:26). cObjCharacterEx::PlaySA (CommonDll 0x27f10) writes
        // +0x20 = +0x24 = 0xfffffff8 and +0x28 = 7 so the two partners of a sync attack can overlap; the low three
        // bits look like the character categories, the rest (ground, world) stay. The first try had the order
        // reversed (7, 7, 0xfffffff8: an ordinary animal's values, 13513 in objmodel-decomp-common-ai.c), which
        // lifted the players again (10:47). CharacterFreeze only stops input (USER_STOP), not gravity.
        char* phys = nullptr;
        if (core::safe_read(reinterpret_cast<char*>(ent) + 0x118, phys) && phys) {
            uint32_t was[3] = {};
            core::safe_copy(phys + 0x20, was, sizeof was);
            const uint32_t clear = 0xfffffff8, seven = 7;
            std::memcpy(phys + 0x20, &clear, sizeof clear);
            std::memcpy(phys + 0x24, &clear, sizeof clear);
            std::memcpy(phys + 0x28, &seven, sizeof seven);
            spdlog::info("puppets: physics body {:#x} kept; collision words were {:#x} {:#x} {:#x}, now as PlaySA",
                         reinterpret_cast<uintptr_t>(phys), was[0], was[1], was[2]);
            // One-off: the body's size fields (finding where the drawn model sits relative to the body).
            float f[16] = {};
            core::safe_copy(phys + 0x60, f, sizeof f);
            spdlog::info("puppets: phys +0x60..+0x9c {:.3f} {:.3f} {:.3f} {:.3f} | {:.3f} {:.3f} {:.3f} {:.3f} | {:.3f} "
                         "{:.3f} {:.3f} {:.3f} | {:.3f} {:.3f} {:.3f} {:.3f}",
                         f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11], f[12], f[13], f[14],
                         f[15]);
        }
    }
    g_spawn_manager = mgr;
    return ent;
}

// Legs stay on GAME.AP names (the guide legs script). Arms and the gun use the weapon pack.
// Entity SetAnim("shotgun1/...") creates the missing cBodyShotgun1npc class, so the arm clips are
// applied with cModelPart::SetAnim on every part except the legs.
// Facing vector (sin yaw, cos yaw); right = (cos yaw, -sin yaw) — verified: key A (strafe left) → walk/left.
struct ClipSet {
    const char* legs;
    const char* arms;  // nullptr: leave the torso alone (knife / unarmed)
    const char* gun;   // clip on the weapon mesh, no pack prefix
};

ClipSet choose_clips(Puppet& p, const net::InterpSample& s, uint32_t now) {
    const char* pack = nullptr;
    if (s.weaponId == net::kWeaponShotgun || s.weaponId == net::kWeaponRifle) pack = "shotgun1/";
    else if (s.weaponId == net::kWeaponHandgun) pack = "handgun3/";
    const bool armed = pack != nullptr;
    const int shots = (s.flags & net::kShotCounterMask) >> net::kShotCounterShift;
    if (p.last_shots >= 0 && shots != p.last_shots && armed) p.fire_until = now + 400;
    p.last_shots = shots;

    // Whole-body poses from the guide pack (GUIDELEGS.SCR StayDown / Getup / FinalLiedead), on every part so the
    // arms do not keep an idle clip over a body lying on the ground. re/notes/knockdown.md section 5.
    if (s.flags & net::kFlagDead) return ClipSet{"liedead", "liedead", nullptr};
    if (s.flags & net::kFlagKnocked) {
        p.was_knocked = true;
        return ClipSet{"deadfrontup", "deadfrontup", nullptr};
    }
    if (p.was_knocked) {
        p.was_knocked = false;
        p.getup_until = now + 1800;
    }
    if (now < p.getup_until) return ClipSet{"syncguidegetup", "syncguidegetup", nullptr};

    ClipSet c{"idlenormalneutral", nullptr, "idlealertneutral"};
    if (armed && now < p.fire_until) {
        c.legs = "idlenormalneutral";
        c.arms = "fire";  // filled with pack below
        c.gun = "fire";
    } else if (armed && (s.flags & net::kFlagReloading)) {
        c.gun = s.weaponId == net::kWeaponHandgun ? "reload" : "reload1";
        c.arms = c.gun;
    } else if (s.flags & net::kFlagCrouched) {
        c.legs = g_crouch_clip.c_str();
        c.arms = "idlealertneutral";
    } else {
        const float vx = s.velocity[0], vz = s.velocity[2];
        const float speed = std::sqrt(vx * vx + vz * vz);
        if (speed >= kWalkSpeed) {
            const float y = s.yaw * 3.14159265f / 180.f;
            const float fwd = vx * std::sin(y) + vz * std::cos(y);
            const float right = vx * std::cos(y) - vz * std::sin(y);
            if (fwd >= std::fabs(right)) {
                if (speed >= kRunSpeed) {
                    c.legs = "run/forward";
                    c.arms = "run";
                    c.gun = "run";
                } else {
                    c.legs = "walk/forward";
                    c.arms = "walkforwardalert";
                    c.gun = "walkforwardalert";
                }
            } else if (-fwd >= std::fabs(right)) {
                c.legs = "walk/backward";
                c.arms = "walkbackwardalert";
                c.gun = "walkbackwardalert";
            } else if (right > 0.f) {
                c.legs = "walk/right";
                c.arms = "walkstraferightalert";
                c.gun = "walkstraferightalert";
            } else {
                c.legs = "walk/left";
                c.arms = "walkstrafeleftalert";
                c.gun = "walkstrafeleftalert";
            }
        }
    }
    if (!armed) {
        c.arms = nullptr;
        c.gun = nullptr;
        p.arms_buf.clear();
    } else if (c.arms) {
        p.arms_buf = std::string(pack) + c.arms;
        c.arms = p.arms_buf.c_str();
    }
    return c;
}
// Third-person weapon models (OBJECTS\ITEMS\GAME\WEAPONS\<NAME>\<NAME>.SKIN; the *1ST dirs are first-person).
// Hands as in the NPC weapon scripts (HGUNNPC/SGUNNPC/RIFLENPC.SCR default branch): handgun right, long guns left;
// each script also LinkParts the weapon to that hand so it follows the clip.
struct WeaponModel {
    const char* skin;
    const char* hand;
};
WeaponModel weapon_model(int kind) {
    switch (kind) {
        case net::kWeaponHandgun: return {"objects/items/game/weapons/handgun3/handgun3.skin", "BipRHand"};
        case net::kWeaponShotgun: return {"objects/items/game/weapons/shotgun1/shotgun1.skin", "BipLHand"};
        case net::kWeaponRifle: return {"objects/items/game/weapons/rifle1/rifle1.skin", "BipLHand"};
        case net::kWeaponOther: return {nullptr, nullptr};
        default: return {nullptr, nullptr};
    }
}

// Unlink first: the hand controller's cModelPartLink holds a reference on the weapon's model owner, and a link left
// in place is released a second time when the world is torn down (StopServer crash, 2026-10-16 20:42).
void drop_weapon(Puppet& p) {
    if (p.weapon && p.entity) {
        const WeaponModel m = weapon_model(p.weapon_kind);
        if (m.hand) entity::UnlinkPart(p.entity, m.hand);
    }
    if (p.weapon && g_spawn_manager) {
        entity::Detach(p.weapon);
        entity::RemoveEntity(g_spawn_manager, p.weapon);
    }
    p.weapon = nullptr;
    p.weapon_kind = 0;
}

void sync_weapon(Puppet& p, int kind) {
    if (!g_weapons || !p.entity) return;
    if (kind == p.weapon_kind && (p.weapon || p.weapon_failed)) return;
    drop_weapon(p);
    p.weapon_failed = false;
    p.weapon_kind = kind;
    const WeaponModel m = weapon_model(kind);
    if (!m.skin) return;
    void* mgr = EntityManager();
    void* w = mgr ? entity::Create("obj_scripted_entity") : nullptr;
    if (!w) {
        p.weapon_failed = true;
        return;
    }
    entity::HandleKey(w, "model_name", m.skin);
    const int index = entity::AddEntity(mgr, w);
    entity::Precache(w);
    entity::Spawn(w);
    entity::UnlinkPhysics(w);
    int16_t puppet_id = -1;
    core::safe_read(static_cast<char*>(p.entity) + 0x04, puppet_id);
    const int tag = entity::TagIndex(p.entity, m.hand);
    const int attached = tag >= 0 ? entity::Attach(w, puppet_id, tag, 1) : 0;
    const int linked = attached == 1 ? entity::LinkPart(p.entity, w, m.hand) : 0;
    int pref = -99, wref = -99;
    core::safe_read(static_cast<char*>(p.entity) + 0xb8, pref);
    core::safe_read(static_cast<char*>(w) + 0xb8, wref);
    spdlog::info("puppets: weapon kind {} '{}' entity {:#x} index={} tag {}={} attach={} link={} ref puppet={} weapon={}",
                 kind, m.skin, reinterpret_cast<uintptr_t>(w), index, m.hand, tag, attached, linked, pref, wref);
    if (index < 0 || attached != 1) {
        if (index >= 0) entity::RemoveEntity(mgr, w);
        p.weapon_failed = true;
        return;
    }
    p.weapon = w;
}

// Keep the weapon drawable: give it the puppet's world box and relink it into the BSP (like the puppet itself).
void place_weapon(Puppet& p) {
    if (!p.weapon) return;
    std::memcpy(static_cast<char*>(p.weapon) + kEntWorldBox, static_cast<char*>(p.entity) + kEntWorldBox, 24);
    entity::BspUpdate(p.weapon);
}

void pick_anim(Puppet& p, const ClipSet& clips) {
    if (!g_anim_enabled || !p.entity) return;
    const std::string legs = clips.legs ? clips.legs : "";
    if (p.anim != legs && !legs.empty()) {
        p.anim = legs;
        const bool ok = entity::SetAnim(p.entity, legs.c_str());
        spdlog::info("puppets: entity {:#x} legs '{}' -> {}", reinterpret_cast<uintptr_t>(p.entity), legs, ok);
    }
    const std::string arms = clips.arms ? clips.arms : "";
    if (p.arms_shown != arms) {
        p.arms_shown = arms;
        const int n = arms.empty() ? 0 : entity::SetAnimOnParts(p.entity, arms.c_str(), true);
        spdlog::info("puppets: entity {:#x} arms '{}' parts={}", reinterpret_cast<uintptr_t>(p.entity), arms, n);
    }
    const std::string gun = clips.gun ? clips.gun : "";
    if (p.weapon && p.gun_shown != gun) {
        p.gun_shown = gun;
        const int n = gun.empty() ? 0 : entity::SetAnimOnParts(p.weapon, gun.c_str(), false);
        spdlog::info("puppets: weapon {:#x} anim '{}' parts={}", reinterpret_cast<uintptr_t>(p.weapon), gun, n);
    }
}

// Host only: animals pick the closest entry of cAIHuntAnimalInterface's attack-target list
// (re/notes/lead-combat.md). The entry must go before the entity does, or GetClosestAttackTarget reads freed memory.
void register_target(Puppet& p) {
    if (!g_attack_target || p.attack_target || !p.entity || !NetIsHost()) return;
    p.attack_target = entity::AddAttackTarget(p.entity);
    spdlog::info("puppets: attack target add {:#x} -> {} (list {})", reinterpret_cast<uintptr_t>(p.entity),
                 p.attack_target, entity::AttackTargetCount());
}

void unregister_target(Puppet& p) {
    if (!p.attack_target) return;
    p.attack_target = false;
    if (!p.entity) return;
    const bool ok = entity::RemoveAttackTarget(p.entity);
    spdlog::info("puppets: attack target remove {:#x} -> {} (list {})", reinterpret_cast<uintptr_t>(p.entity), ok,
                 entity::AttackTargetCount());
}

void load_settings() {
    const std::string ini = (core::self_dir() / "dh2coop.ini").string();
    char buf[64] = {};
    g_enabled = GetPrivateProfileIntA("Puppets", "Enabled", 1, ini.c_str()) != 0;
    g_spike = GetPrivateProfileIntA("Puppets", "Spike", 0, ini.c_str()) != 0;
    g_anim_enabled = GetPrivateProfileIntA("Puppets", "Animate", 1, ini.c_str()) != 0;
    g_physics = GetPrivateProfileIntA("Puppets", "Physics", 0, ini.c_str()) != 0;
    char yoff[32] = {};
    GetPrivateProfileStringA("Puppets", "YOffset", "0", yoff, sizeof yoff, ini.c_str());
    g_y_offset = static_cast<float>(std::atof(yoff));
    GetPrivateProfileStringA("Puppets", "PhysicsKeyY", "0", yoff, sizeof yoff, ini.c_str());
    g_phys_key_y = static_cast<float>(std::atof(yoff));
    char crouch[64] = {};
    GetPrivateProfileStringA("Puppets", "CrouchClip", g_crouch_clip.c_str(), crouch, sizeof crouch, ini.c_str());
    g_crouch_clip = crouch;
    g_weapons = GetPrivateProfileIntA("Puppets", "Weapons", 1, ini.c_str()) != 0;
    g_attack_target = GetPrivateProfileIntA("Puppets", "AttackTarget", 0, ini.c_str()) != 0;
    GetPrivateProfileStringA("Puppets", "Class", kDefaultClass, buf, sizeof buf, ini.c_str());
    g_class = buf;
    char model[260] = {};
    GetPrivateProfileStringA("Puppets", "Model", g_model.c_str(), model, sizeof model, ini.c_str());
    g_model = model;
    spdlog::info("puppets: enabled={} spike={} animate={} physics={} yoffset={:.2f} class='{}' model='{}'", g_enabled,
                 g_spike, g_anim_enabled, g_physics, g_y_offset, g_class, g_model);
}

class PuppetsModule : public dh2::Module {
public:
    const char* Name() const override { return "puppets"; }
    int Priority() const override { return 45; }

    void OnInit() override { load_settings(); }
    void OnEngineReady(HMODULE engine) override {
        entity::Bind(engine);
        if (g_spike) {
            void* draw = core::find_export(engine, "?Draw@cObjEntity@@UAEXXZ");
            g_draw_hook = core::install_hook("cObjEntity::Draw (diag)", draw, reinterpret_cast<void*>(&hk_EntityDraw));
        }
    }

    void OnMapLoaded(const char*) override {
        // The map change destroyed every entity, including ours: forget pointers, keep the snapshot buffers.
        for (auto& [id, p] : g_puppets) {
            p.entity = nullptr;
            p.attack_target = false;
            p.spawn_failed = false;
            p.anim.clear();
    p.weapon = nullptr;
    p.weapon_kind = 0;
    p.weapon_failed = false;
    p.gun_shown.clear();
}
        g_spawn_manager = nullptr;
        g_spike_entity = nullptr;
        g_spike_wait = 0.f;
    }

    void OnMapUnloading() override {
        // Entities are still alive here: undo every engine-side link to our puppets before DeleteAllObjects.
        for (auto& [id, p] : g_puppets) {
            int pref = -99, wref = -99, wpref = -99;
            if (p.entity) core::safe_read(static_cast<char*>(p.entity) + 0xb8, pref);
            if (p.weapon) core::safe_read(static_cast<char*>(p.weapon) + 0xb8, wref);
            void* wkeep = p.weapon;
            unregister_target(p);
            // Break the hand/weapon sync ring (mutual references) and detach, but leave the deletion to
            // DeleteAllObjects: a RemoveEntity right before it destroyed the weapon twice (20:59:58 crash, refs 2->0).
            if (p.weapon && p.entity) {
                const WeaponModel m = weapon_model(p.weapon_kind);
                if (m.hand) entity::UnlinkPart(p.entity, m.hand);
                entity::Detach(p.weapon);
            }
            p.weapon = nullptr;
            p.weapon_kind = 0;
            if (p.entity) core::safe_read(static_cast<char*>(p.entity) + 0xb8, wpref);
            int wafter = -99;
            if (wkeep) core::safe_read(static_cast<char*>(wkeep) + 0xb8, wafter);
            spdlog::info("puppets: unloading remote {} puppet {:#x} ref {}->{} weapon {:#x} ref {}->{}", id,
                         reinterpret_cast<uintptr_t>(p.entity), pref, wpref, reinterpret_cast<uintptr_t>(wkeep), wref,
                         wafter);
        }
    }

    void OnFrame(float dt) override {
        if (!g_enabled) return;
        if (!g_physy_cmd && console_cmds::Ready()) g_physy_cmd = console_cmds::AddCommandStr("coop_physy", &cmd_physy);
        const auto& local = LocalPlayer();
        if (!local.valid) return;  // map not ready / hunter unknown
        if (!hunter_box_logged_ && local.entity) {
            // Feet vs +0xE8: is the hunter position the eye or the feet? (puppet height calibration)
            hunter_box_logged_ = true;
            float box[6] = {};
            core::safe_copy(reinterpret_cast<char*>(local.entity) + kEntWorldBox, box, sizeof box);
            spdlog::info("puppets: hunter pos.y={:.3f} world box y {:.3f}..{:.3f}", local.pos.y, box[1], box[4]);
        }
        if (g_spike) Spike(dt, local);

        const uint32_t now = now_ms();
        for (auto& [id, p] : g_puppets) {
            net::InterpSample s;
            if (!p.buffer.Sample(now, s)) continue;
            if (!p.entity && !p.spawn_failed) {
                char why[32];
                std::snprintf(why, sizeof why, "remote %u", static_cast<unsigned>(id));
                p.entity = spawn_human(why);
                p.spawn_failed = p.entity == nullptr;
                if (p.entity) capture_rel_box(p);
            }
            if (!p.entity) continue;
            // A dead player is not prey: animals drop list entries only through RemoveAttackTarget.
            if (s.flags & net::kFlagDead) unregister_target(p);
            else register_target(p);
            place(p, {s.pos[0], s.pos[1] + g_y_offset, s.pos[2]}, s.yaw);
            sync_weapon(p, s.weaponId);
            place_weapon(p);
            pick_anim(p, choose_clips(p, s, now));
        }
    }

private:
    // Local test puppet: walks a 3 m circle 4 m in front of the hunter, facing its direction of travel.
    void Spike(float dt, const LocalPlayerSample& local) {
        if (!g_spike_entity) {
            g_spike_wait += dt;
            if (g_spike_wait < 3.f) return;  // let the hunter settle
            const float yaw = local.angle.y * 3.14159265f / 180.f;
            g_spike_center = {local.pos.x + 4.f * std::sin(yaw), local.pos.y, local.pos.z + 4.f * std::cos(yaw)};
            g_spike_entity = spawn_human("spike");
            if (!g_spike_entity) {
                g_spike = false;
                return;
            }
            spike_puppet_.entity = g_spike_entity;
            capture_rel_box(spike_puppet_);
            spdlog::info("puppets: spike rel box ({:.2f},{:.2f},{:.2f})-({:.2f},{:.2f},{:.2f})", spike_puppet_.rel_box[0],
                         spike_puppet_.rel_box[1], spike_puppet_.rel_box[2], spike_puppet_.rel_box[3],
                         spike_puppet_.rel_box[4], spike_puppet_.rel_box[5]);
        }
        g_spike_t += dt;
        diag_accum_ += dt;
        g_watch_puppet.store(g_spike_entity);
        draw_accum_ += dt;
        if (draw_accum_ >= 2.f) {
            draw_accum_ = 0.f;
            spdlog::info("puppets: draws/2s total={} puppet={} deer120={} (puppet {:#x} +0x1c8={:#x} deer +0x1c8={:#x})",
                         g_draw_total.exchange(0), g_draw_puppet.exchange(0), g_draw_ref.exchange(0),
                         reinterpret_cast<uintptr_t>(g_spike_entity), Rd(g_spike_entity, 0x1c8),
                         Rd(g_watch_ref.load(), 0x1c8));
        }
        if (diag_logged_ < 3 && diag_accum_ >= 1.5f) {
            diag_accum_ = 0.f;
            ++diag_logged_;
            DumpEntity("puppet", g_spike_entity);
            DumpEntity("hunter", local.entity);
            // Reference: a rendered character (first animal; HUNTMAP4 animals are ids 120-124).
            if (void* mgr = EntityManager()) {
                void** arr = nullptr;
                core::safe_read(reinterpret_cast<char*>(mgr) + 0x04, arr);
                void* animal = nullptr;
                if (arr) core::safe_read(arr + 120, animal);
                g_watch_ref.store(animal);
                DumpEntity("animal120", animal);
            }
        }
        // Visibility test: stand 3 m straight ahead of the hunter (current pos/yaw), facing the hunter,
        // swaying ±1 m sideways so movement is visible.
        // User report 2026-10-16: with +forward the puppet was BEHIND the camera → the view looks along
        // -(sin yaw, cos yaw). [Puppets] SpikeSign=-1 (default) places it in front; +1 restores the old side.
        // The view camera's forward when known (+0xF4 only follows the view while walking; MAP3B 2026-10-17 the
        // +0xF4 side put the spike out of the shot): forward = (sin, cos) of the view yaw.
        const bool by_view = local.viewValid;
        const float hy = (by_view ? local.viewYaw : local.angle.y) * 3.14159265f / 180.f;
        const float sgn = by_view ? 1.f : static_cast<float>(spike_sign_);
        const float fx = sgn * std::sin(hy), fz = sgn * std::cos(hy);
        const float side = spike_sway_ ? std::sin(g_spike_t * 0.8f) : 0.f;
        const entity::Vec3 pos{local.pos.x + 3.f * fx + side * fz, local.pos.y, local.pos.z + 3.f * fz - side * fx};
        const float yawDeg = by_view ? local.viewYaw + 180.f
                                     : (spike_sign_ < 0 ? local.angle.y : local.angle.y + 180.f);  // face the camera
        place(spike_puppet_, pos, yawDeg);
        pick_anim(spike_puppet_, ClipSet{"walk/forward", nullptr, nullptr});
        log_accum_ += dt;
        if (log_accum_ >= 2.f) {
            log_accum_ = 0.f;
            spdlog::info("puppets: spike pos=({:.2f},{:.2f},{:.2f}) yaw={:.1f}", pos.x, pos.y, pos.z, yawDeg);
        }
    }

    static uint32_t Rd(void* e, uintptr_t off) {
        uint32_t v = 0;
        if (e) core::safe_read(reinterpret_cast<char*>(e) + off, v);
        return v;
    }

    // Field dump for comparing a puppet with live game characters (render/visibility diagnosis).
    static void DumpEntity(const char* label, void* e) {
        if (!e) return;
        auto rd = [e](uintptr_t off) {
            std::uint32_t v = 0;
            core::safe_read(reinterpret_cast<char*>(e) + off, v);
            return v;
        };
        const char* name = reinterpret_cast<const char*>(static_cast<uintptr_t>(rd(0xE0)));
        float box[6] = {};
        core::safe_copy(reinterpret_cast<char*>(e) + 0xC4, box, sizeof box);
        spdlog::info("puppets: diag {} {:#x} vt={} id={} parent={} A8={:#x} C0={:#x} model={:#x} phys={:#x} "
                     "bsplink124={} box=({:.2f},{:.2f},{:.2f})-({:.2f},{:.2f},{:.2f}) name='{}'",
                     label, reinterpret_cast<uintptr_t>(e), core::describe_address(reinterpret_cast<void*>(rd(0))),
                     static_cast<int16_t>(rd(0x04) & 0xFFFF), static_cast<int16_t>(rd(0xA4) & 0xFFFF), rd(0xA8),
                     rd(0xC0), rd(0xE4), rd(0x118), static_cast<int32_t>(rd(0x124)), box[0], box[1], box[2], box[3],
                     box[4], box[5], core::safe_cstr(name, 120));
        spdlog::info("puppets: diag {} useServer={} +8={:#x} +0x80..+0xE0 {}", label, entity::UseServerValue(), rd(0x08), core::hex_dump(reinterpret_cast<char*>(e) + 0x80, 0x60));
    }

    Puppet spike_puppet_;
    bool hunter_box_logged_ = false;
    int spike_sign_ = static_cast<int>(GetPrivateProfileIntA("Puppets", "SpikeSign", -1,
                                                             (core::self_dir() / "dh2coop.ini").string().c_str()));
    // [Puppets] SpikeSway=0 holds the test puppet still (same spot in every screenshot, for A/B pictures).
    bool spike_sway_ = GetPrivateProfileIntA("Puppets", "SpikeSway", 1,
                                             (core::self_dir() / "dh2coop.ini").string().c_str()) != 0;
    float log_accum_ = 0.f;
    float diag_accum_ = 0.f;
    float draw_accum_ = 0.f;
    int diag_logged_ = 0;
};

DH2_MODULE(PuppetsModule)

}  // namespace

void OnRemoteState(uint8_t playerId, const net::PlayerState& state, uint32_t localTimeMs) {
    g_puppets[playerId].buffer.Push(localTimeMs, state);
}

void OnRemoteLeft(uint8_t playerId) {
    auto it = g_puppets.find(playerId);
    if (it == g_puppets.end()) return;
    unregister_target(it->second);
    drop_weapon(it->second);
    if (it->second.entity && g_spawn_manager) {
        const bool ok = entity::RemoveEntity(g_spawn_manager, it->second.entity);
        spdlog::info("puppets: removed puppet for player {} -> {}", playerId, ok);
    }
    g_puppets.erase(it);
}

void RemoveAll() {
    for (auto& [id, p] : g_puppets) {
        unregister_target(p);
        drop_weapon(p);
        if (p.entity && g_spawn_manager) entity::RemoveEntity(g_spawn_manager, p.entity);
    }
    g_puppets.clear();
}

int Count() {
    int n = 0;
    for (auto& [id, p] : g_puppets) n += p.entity ? 1 : 0;
    return n;
}

void* EntityOf(uint8_t playerId) {
    const auto it = g_puppets.find(playerId);
    return it != g_puppets.end() ? it->second.entity : nullptr;
}

uint8_t PlayerOf(void* ent) {
    if (!ent) return net::kInvalidPlayerId;
    for (const auto& [id, p] : g_puppets) {
        if (p.entity == ent) return id;
    }
    return net::kInvalidPlayerId;
}

bool Owns(void* ent) {
    if (!ent) return false;
    if (ent == g_spike_entity) return true;
    for (const auto& [id, p] : g_puppets) {
        (void)id;
        if (p.entity == ent || p.weapon == ent) return true;
    }
    return false;
}

}  // namespace dh2::coop::puppets
