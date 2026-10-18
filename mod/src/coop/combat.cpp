// Phase 5: host animals fight every player. Plan: re/notes/lead-combat.md, engine path: re/notes/knockdown.md.
// On the host, puppets are in the animals' attack-target list ([Puppets] AttackTarget=1, needs Physics=1: the attack
// code reads the target's physics body). What an animal does to a puppet is forwarded to that player's own hunter:
//   - hurt (character vt +0x14C, cObjCharacter::OnHurt on a puppet) -> kPlayerHurt -> hunter vt +0x14C
//   - sync attack start (vt +0x210, cObjCharacterEx::PlaySA(char*, m4x4&)) -> kPlayerKnock -> hunter vt +0x210
//   - sync attack end (vt +0x214, cObjCharacterEx::OnEndSA) -> kPlayerKnockEnd -> hunter vt +0x214
// The puppet itself takes nothing: it is a picture of the other player, who takes the hit on their own machine.
#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

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
#include "engine/console_cmds.h"
#include "engine/entity_api.h"

namespace dh2::coop::combat {
namespace {

constexpr uintptr_t kBrainTargetId = 0x254;  // cAIHuntBrain_Animal::SetTarget stores the target entity id here
constexpr uintptr_t kEntPhys = 0x118;        // physics body; ProcessAttackStuck reads its radius at +0x7c
constexpr uintptr_t kEntPos = 0xE8;
constexpr uintptr_t kInSA = 0x368;           // cObjCharacterEx: inside a sync attack (PlaySA .. OnEndSA)
constexpr uintptr_t kHunterDead = 0x6CC;     // cObjHunter::OnDie sets it; Hurt returns at once while it is set
constexpr uintptr_t kRvaHunterVt = 0x106ff0; // GameDll7 cScr_ModHunter
constexpr uintptr_t kRvaPilotVt = 0x109d48;  // GameDll7 cObjPilot (MAP11A / MAP11C helicopter maps)
constexpr int kSlotHurt = 0x14C / 4;
constexpr int kSlotPlaySA = 0x210 / 4;
constexpr int kSlotEndSA = 0x214 / 4;
constexpr uint32_t kKnockTimeoutMs = 8000;   // stand up even if the host's end never arrives (animal died mid-attack)

safetyhook::InlineHook* g_hurt_hook = nullptr;
safetyhook::InlineHook* g_playsa_hook = nullptr;
safetyhook::InlineHook* g_endsa_hook = nullptr;
safetyhook::InlineHook* g_stuck_hook = nullptr;
uintptr_t g_game_base = 0;
bool g_in_apply = false;
int g_stuck_skips = 0;
// Client: our hunter is inside a knock the host started.
bool g_knocked = false;
uint32_t g_knock_ms = 0;
uint32_t g_knock_end_ms = 0;  // a player's knock stands up on its own timer (0 = wait for the host's end)
std::string g_knock_clip;  // PlaySA keeps the pointer; keep the string alive until OnEndSA
float g_knock_basis[16] = {};

int16_t IdOf(void* ent) {
    int16_t id = -1;
    if (ent) core::safe_read(static_cast<char*>(ent) + 0x04, id);
    return id;
}

// ---- host side: catch what animals do to puppets ----

void __fastcall hk_ProcessAttackStuck(void* brain, void*) {
    int id = -1;
    core::safe_read(static_cast<char*>(brain) + kBrainTargetId, id);
    void* target = id >= 0 ? EntityById(id) : nullptr;
    uintptr_t phys = 0;
    if (target && (!core::safe_read(static_cast<char*>(target) + kEntPhys, phys) || phys == 0)) {
        // A physics-less target (a puppet with Physics=0) used to crash here (null +0x7c, 2026-10-16 21:20).
        if (g_stuck_skips++ < 5) spdlog::warn("combat: ProcessAttackStuck skipped, target id {} has no physics body", id);
        return;
    }
    g_stuck_hook->unsafe_thiscall<void>(brain);
}

void __fastcall hk_CharacterHurt(void* self, void*, void* attacker, float dmg, int type, const void* pos,
                                 const void* dir);

}  // namespace

// Host: a hit on a puppet goes to that player's own hunter (kPlayerHurt) and the puppet takes nothing. Called from
// both hurt entries: cObjScriptedEntity::OnHurt (animals.cpp hook; the one a leopard's attack reaches, 11:11) and
// cObjCharacter::OnHurt (below). True when `self` was a puppet.
bool ForwardPuppetHurt(void* self, void* attacker, float dmg, int type) {
    if (g_in_apply || !NetIsHost()) return false;
    const uint8_t who = puppets::PlayerOf(self);
    if (who == net::kInvalidPlayerId) return false;
    const int16_t aid = IdOf(attacker);
    const uint32_t packed = static_cast<uint16_t>(aid) | (static_cast<uint32_t>(type & 0xFF) << 16);
    spdlog::info("combat: animal id {} hurt player {} for {:.1f} (type {})", aid, who, dmg, type);
    events::Send(events::kPlayerHurt, who, packed, dmg, "", 0);
    return true;
}

namespace {

void __fastcall hk_CharacterHurt(void* self, void*, void* attacker, float dmg, int type, const void* pos,
                                 const void* dir) {
    if (ForwardPuppetHurt(self, attacker, dmg, type)) return;
    g_hurt_hook->unsafe_thiscall<void>(self, attacker, dmg, type, pos, dir);
}

void __fastcall hk_PlaySA(void* self, void*, char* clip, float* basis) {
    if (!g_in_apply && NetIsHost()) {
        const uint8_t who = puppets::PlayerOf(self);
        if (who != net::kInvalidPlayerId) {
            // Row 2 of the basis is the victim's forward (row 3 is the victim position the task wrote).
            float fwd[3] = {};
            core::safe_copy(basis + 8, fwd, sizeof fwd);
            const float yaw = std::atan2(fwd[0], fwd[2]) * 57.2957795f;
            const std::string c = core::safe_cstr(clip, 64);
            spdlog::info("combat: sync attack '{}' on player {} (yaw {:.1f})", c, who, yaw);
            // The puppet's pack has no pl_* clips. Mark it busy so a second animal does not start another one.
            const int one = 1;
            std::memcpy(static_cast<char*>(self) + kInSA, &one, sizeof one);
            events::Send(events::kPlayerKnock, who, 0, yaw, c, 0);
            return;
        }
    }
    g_playsa_hook->unsafe_thiscall<void>(self, clip, basis);
}

void __fastcall hk_OnEndSA(void* self, void*) {
    if (!g_in_apply && NetIsHost()) {
        const uint8_t who = puppets::PlayerOf(self);
        if (who != net::kInvalidPlayerId) {
            const int zero = 0;
            std::memcpy(static_cast<char*>(self) + kInSA, &zero, sizeof zero);
            spdlog::info("combat: sync attack on player {} ended", who);
            events::Send(events::kPlayerKnockEnd, who, 0, 0.f, "", 0);
            return;
        }
    }
    g_endsa_hook->unsafe_thiscall<void>(self);
}

// ---- client side: the hit lands on our own hunter ----

void* LocalHunter() {
    void* h = LocalPlayer().entity;
    uintptr_t vt = 0;
    if (!h || !g_game_base || !core::safe_read(h, vt) ||
        (vt != g_game_base + kRvaHunterVt && vt != g_game_base + kRvaPilotVt))
        return nullptr;
    return h;
}

bool Dead(void* hunter) {
    int dead = 0;
    core::safe_read(static_cast<char*>(hunter) + kHunterDead, dead);
    return dead != 0;
}

void* Slot(void* obj, int slot) {
    void** vt = nullptr;
    void* fn = nullptr;
    if (!core::safe_read(obj, vt) || !vt || !core::safe_read(vt + slot, fn)) return nullptr;
    return fn;
}

bool guarded_hurt(void* fn, void* hunter, void* attacker, float dmg, int type, const float* pos, const float* dir) {
    __try {
        reinterpret_cast<void(__thiscall*)(void*, void*, float, int, const float*, const float*)>(fn)(hunter, attacker,
                                                                                                    dmg, type, pos, dir);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_playsa(void* fn, void* hunter, char* clip, float* basis) {
    __try {
        reinterpret_cast<void(__thiscall*)(void*, char*, float*)>(fn)(hunter, clip, basis);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_endsa(void* fn, void* hunter) {
    __try {
        reinterpret_cast<void(__thiscall*)(void*)>(fn)(hunter);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void OnHurtCommit(const net::GameEvent& ev) {
    if (ev.subjectId != NetLocalId()) return;
    void* hunter = LocalHunter();
    if (!hunter || Dead(hunter)) return;
    const int16_t aid = static_cast<int16_t>(ev.i0 & 0xFFFF);
    const int type = static_cast<int>((ev.i0 >> 16) & 0xFF);
    void* attacker = EntityById(aid);  // this process's mirror of the animal
    if (!attacker) {
        spdlog::warn("combat: hurt from animal id {}: no mirror here", aid);
        return;
    }
    float pos[3] = {}, from[3] = {};
    core::safe_copy(static_cast<char*>(hunter) + kEntPos, pos, sizeof pos);
    core::safe_copy(static_cast<char*>(attacker) + kEntPos, from, sizeof from);
    float dir[3] = {pos[0] - from[0], pos[1] - from[1], pos[2] - from[2]};
    const float len = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
    if (len > 0.001f) {
        for (float& d : dir) d /= len;
    }
    void* fn = Slot(hunter, kSlotHurt);
    g_in_apply = true;
    const bool ok = fn && guarded_hurt(fn, hunter, attacker, ev.f0, type, pos, dir);
    g_in_apply = false;
    spdlog::info("combat: took {:.1f} from animal id {} (type {}) ok={} dead={}", ev.f0, aid, type, ok, Dead(hunter));
}

void EndKnock(const char* why) {
    if (!g_knocked) return;
    g_knocked = false;
    void* hunter = LocalHunter();
    void* fn = hunter ? Slot(hunter, kSlotEndSA) : nullptr;
    g_in_apply = true;
    const bool ok = fn && guarded_endsa(fn, hunter);
    g_in_apply = false;
    spdlog::info("combat: knock over ({}) ok={}", why, ok);
}

// The hunter's own sync-attack entry (vt +0x210) with a basis facing `yawDeg`, at the hunter's position. The legs
// script holds PlaySA until OnEndSA, then plays synchuntergetup. endAfterMs = 0 waits for the host's end.
bool StartKnock(const std::string& clip, float yawDeg, uint32_t endAfterMs, const char* why) {
    void* hunter = LocalHunter();
    if (!hunter || Dead(hunter) || clip.empty()) return false;
    // A pilot sits in the helicopter (body state "Heli"): a knock-down clip there would pull it off its seat.
    if (LocalPlayer().pilot) {
        spdlog::info("combat: knock ({}) ignored: the local player is a pilot", why);
        return false;
    }
    if (g_knocked) EndKnock("new knock");
    // The task's basis: rows 0-2 rotation (right, up, forward), row 3 the victim position.
    const float yaw = yawDeg * 3.14159265f / 180.f;
    const float s = std::sin(yaw), c = std::cos(yaw);
    float m[16] = {c, 0.f, -s, 0.f, 0.f, 1.f, 0.f, 0.f, s, 0.f, c, 0.f, 0.f, 0.f, 0.f, 1.f};
    core::safe_copy(static_cast<char*>(hunter) + kEntPos, m + 12, sizeof(float) * 3);
    std::memcpy(g_knock_basis, m, sizeof m);
    g_knock_clip = clip;
    void* fn = Slot(hunter, kSlotPlaySA);
    g_in_apply = true;
    const bool ok = fn && guarded_playsa(fn, hunter, g_knock_clip.data(), g_knock_basis);
    g_in_apply = false;
    g_knocked = ok;
    g_knock_ms = GetTickCount();
    g_knock_end_ms = endAfterMs ? g_knock_ms + endAfterMs : 0;
    spdlog::info("combat: knocked down ({}), clip '{}' yaw {:.1f} ok={}", why, g_knock_clip, yawDeg, ok);
    return ok;
}

// ---- players knocking each other over (stretch goal) ----
// Each map loads one teen-body pak with the player half of its animals' sync attacks (TEENM\BODY\<MAP>.AP, listed
// with re\scripts\ap_list.py). A player's hit borrows that clip: front when the hit comes from ahead, back otherwise.
struct MapClips {
    const char* map;
    const char* front;
    const char* back;
};
constexpr MapClips kMapClips[] = {
    {"HUNTMAP1", "pl_l_syncattack1", "pl_back_l_syncattack1"},
    {"HUNTMAP2", "pl_do_syncattack1", nullptr},
    {"HUNTMAP3", "pl_gw_syncattack1", nullptr},
    {"HUNTMAP4", "pl_p_syncattack1", "pl_back_p_syncattack1"},
    {"MAP1", "pl_ml_syncattack1", nullptr},
    {"MAP2A", "pl_gw_syncattack1", nullptr},
    {"MAP2B", "pl_gw_syncattack1", nullptr},
    {"MAP2C", "pl_gw_syncattack1", nullptr},
    {"MAP3A", "pl_l_syncattack1", "pl_back_l_syncattack1"},
    {"MAP3B", "pl_l_syncattack1", "pl_back_l_syncattack1"},
    {"MAP3C", "pl_ln_syncattack1", nullptr},
    {"MAP4A", "pl_ln_syncattack1", nullptr},
    {"MAP4B", "pl_h_syncattack1", "pl_back_h_syncattack1"},
    {"MAP5", "pl_hy_syncattack1", nullptr},
    {"MAP6", "pl_t_syncattack1", "pl_back_t_syncattack1"},
    {"MAP7", "pl_t_syncattack1", "pl_back_t_syncattack1"},
    {"MAP8", "pl_sb_syncattack1", "pl_back_sb_syncattack1"},
    {"MAP9", "pl_do_syncattack1", nullptr},
    {"MAP10", "pl_p_syncattack1", "pl_back_p_syncattack1"},
    {"MAP11A", "pl_wb_syncattack1", nullptr},
    {"MAP11B", "pl_wb_syncattack1", nullptr},
    {"MAP11C", "pl_wb_syncattack1", nullptr},
    {"MAP12A", "pl_pb_syncattack1", "pl_back_pb_syncattack1"},
    {"MAP12B", "pl_pb_syncattack1", nullptr},
    {"MAP12C", "pl_pb_syncattack1", nullptr},
    {"MAPARENA", "pl_l_syncattack1", "pl_back_l_syncattack1"},
    {"MAPISLAND", "pl_t_syncattack1", "pl_back_t_syncattack1"},
    {"MAPTSAVO", "pl_ln_syncattack1", nullptr},
};

bool g_knock_on_shot = true;   // [Combat] KnockOnShot
bool g_knock_on_melee = true;  // [Combat] KnockOnMelee
uint32_t g_pvp_knock_ms = 2600;  // [Combat] KnockMs: a player's knock has no animal task to end it

bool RemotePos(uint8_t id, float out[3]) {
    const SessionView v = Snapshot();
    for (int i = 0; i < v.remoteCount; ++i) {
        if (v.remotes[i].id != id || v.remotes[i].packets == 0) continue;
        std::memcpy(out, v.remotes[i].pos, sizeof(float) * 3);
        return true;
    }
    return false;
}

void KnockedByPlayer(uint8_t attacker) {
    const LocalPlayerSample& lp = LocalPlayer();
    const std::string& map = events::CurrentMap();
    const MapClips* row = nullptr;
    for (const MapClips& r : kMapClips) {
        if (map == r.map) row = &r;
    }
    if (!row || !lp.valid) {
        spdlog::info("combat: knock from player {} ignored, no clip for map '{}'", attacker, map);
        return;
    }
    // Face the attacker for the front clip. From behind (more than 90 degrees off the view) take the back clip
    // when the map has one, with the basis facing away from the attacker.
    float from[3];
    float yaw = lp.viewValid ? lp.viewYaw : lp.angle.y;
    bool back = false;
    if (RemotePos(attacker, from)) {
        const float to_att = std::atan2(from[0] - lp.pos.x, from[2] - lp.pos.z) * 57.2957795f;
        float off = std::fmod(to_att - yaw + 540.f, 360.f) - 180.f;
        back = row->back && std::fabs(off) > 90.f;
        yaw = back ? to_att + 180.f : to_att;
    }
    char why[48];
    std::snprintf(why, sizeof why, "player %u%s", static_cast<unsigned>(attacker), back ? ", from behind" : "");
    StartKnock(back ? row->back : row->front, yaw, g_pvp_knock_ms, why);
}

void OnKnockCommit(const net::GameEvent& ev) {
    if (ev.subjectId != NetLocalId()) return;
    if (ev.key.empty()) {
        KnockedByPlayer(static_cast<uint8_t>(ev.i0 & 0xFF));
        return;
    }
    StartKnock(ev.key, ev.f0, 0, "host animal");
}

void OnKnockEndCommit(const net::GameEvent& ev) {
    if (ev.subjectId != NetLocalId()) return;
    EndKnock("host animal finished");
}

// Host: a player hit another. Knock the victim on its own machine (the host's own hunter is applied here).
std::map<uint8_t, uint32_t> g_last_knocked;  // victim -> GetTickCount, so a burst of pellets is one knock

void RouteKnock(uint8_t attacker, uint8_t victim, uint32_t how) {
    const uint32_t now = GetTickCount();
    auto it = g_last_knocked.find(victim);
    if (it != g_last_knocked.end() && now - it->second < 4000) return;
    g_last_knocked[victim] = now;
    spdlog::info("combat: player {} knocks player {} over ({})", attacker, victim, how == 2 ? "melee" : "shot");
    if (victim == NetLocalId()) {
        KnockedByPlayer(attacker);
        return;
    }
    events::Send(events::kPlayerKnock, victim, attacker, 0.f, "", attacker);
}

void OnKnockRequest(const net::GameEvent& ev) { RouteKnock(ev.actorId, static_cast<uint8_t>(ev.subjectId), ev.i0); }

// Attacker side: which remote player the local shot or swing hit. A capsule from the feet up 1.8 m, radius
// 0.45 m, against the view ray; a swing reaches 2.2 m in a 50 degree cone. No wall test (it is a game of tag).
uint8_t PickVictim(bool melee) {
    const LocalPlayerSample& lp = LocalPlayer();
    if (!lp.valid || !lp.viewValid) return net::kInvalidPlayerId;
    // Nobody is knocked over on a ride (MAP4A / MAP5 truck): every rider reports the same seat of their own copy of
    // the truck, which is where our own eye is, so every shot "hit" the other players (user, 2026-10-18).
    int16_t parent = -1;
    if (lp.entity && core::safe_read(static_cast<char*>(lp.entity) + 0xA4, parent) && parent >= 0)
        return net::kInvalidPlayerId;
    const float yaw = lp.viewYaw * 3.14159265f / 180.f, pitch = lp.viewPitch * 3.14159265f / 180.f;
    const float d[3] = {std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
    const SessionView v = Snapshot();
    uint8_t best = net::kInvalidPlayerId;
    float best_t = melee ? 2.2f : 80.f;
    for (int i = 0; i < v.remoteCount; ++i) {
        const RemoteView& r = v.remotes[i];
        if (r.packets == 0 || (r.flags & net::kFlagRiding)) continue;
        if (melee) {
            const float dx = r.pos[0] - lp.pos.x, dz = r.pos[2] - lp.pos.z;
            const float dist = std::sqrt(dx * dx + dz * dz);
            const float cosang = dist > 0.01f ? (dx * d[0] + dz * d[2]) / (dist * std::sqrt(d[0] * d[0] + d[2] * d[2])) : 1.f;
            if (dist < best_t && cosang > 0.64f) {
                best_t = dist;
                best = r.id;
            }
            continue;
        }
        // Closest approach of the view ray to the capsule axis, sampled along the 1.8 m body.
        for (float h = 0.2f; h <= 1.8f; h += 0.4f) {
            const float c[3] = {r.pos[0] - lp.viewPos.x, r.pos[1] + h - lp.viewPos.y, r.pos[2] - lp.viewPos.z};
            const float t = c[0] * d[0] + c[1] * d[1] + c[2] * d[2];
            if (t <= 0.f || t >= best_t) continue;
            const float px = c[0] - d[0] * t, py = c[1] - d[1] * t, pz = c[2] - d[2] * t;
            if (px * px + py * py + pz * pz < 0.45f * 0.45f) {
                best_t = t;
                best = r.id;
            }
        }
    }
    return best;
}

// Test: coop_knock <player id> knocks that player over as if this process had hit them (no aim needed).
void __cdecl cmd_knock(const char* arg) {
    const int victim = std::atoi(core::safe_cstr(arg, 16).c_str());
    if (victim < 0 || victim >= net::kMaxPlayers) {
        spdlog::warn("coop_knock: usage coop_knock <player id 0-3>");
        return;
    }
    spdlog::info("coop_knock: player {}", victim);
    if (NetIsHost()) RouteKnock(NetLocalId(), static_cast<uint8_t>(victim), 2);
    else events::Request(events::kKnockRequest, static_cast<uint16_t>(victim), 2, 0.f, "");
}

void TryKnock(bool melee) {
    const uint8_t victim = PickVictim(melee);
    if (victim == net::kInvalidPlayerId) return;
    const uint32_t how = melee ? 2u : 1u;
    if (NetIsHost()) RouteKnock(NetLocalId(), victim, how);
    else events::Request(events::kKnockRequest, victim, how, 0.f, "");
}

// ---- host: animals go for the nearest player, and turn on a client who shoots them ----
// Story scripts aim attack tasks at "hunter" (MAP3B leopards: AIHuntTask_AttackCats, target "hunter"), which on the
// host is only the host. The engine re-targets to cAIHuntBrain_Animal::GetClosestAttackTarget only when an animal is
// stuck (ProcessAttackStuck 0x8b800: task vt +0xf8(old, new) when task[1] == 1, then SetTarget(new, 0.1)). We run
// the same switch every half second, for animals already chasing a player, when another player is clearly closer.
// Only with puppets in the attack list ([Puppets] AttackTarget=1 / run-coop-local -Combat).
using GetEntFn = void*(__thiscall*)(void* brain);
using SetTargetFn = void(__thiscall*)(void* brain, void* ent, float f);
using BrainHurtFn = void(__thiscall*)(void* brain, void* attacker);
GetEntFn g_get_target = nullptr;
GetEntFn g_closest = nullptr;
SetTargetFn g_set_target = nullptr;
BrainHurtFn g_brain_hurt = nullptr;
constexpr float kRetargetMargin = 3.f;  // metres closer before an animal switches player
constexpr uintptr_t kEntBrain = 0x200;  // cObjCharacterEx brain (entity_api FreezeBrain)
constexpr uintptr_t kBrainTask = 0x158; // current task

void* guarded_get(GetEntFn fn, void* brain) {
    __try {
        return fn(brain);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool guarded_switch(void* brain, void* task, void* from, void* to) {
    __try {
        if (task) {
            using TaskSwitchFn = void(__thiscall*)(void* task, void* from, void* to);
            reinterpret_cast<TaskSwitchFn>((*reinterpret_cast<void***>(task))[0xf8 / 4])(task, from, to);
        }
        g_set_target(brain, to, 0.1f);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_brain_hurt(void* brain, void* attacker) {
    __try {
        g_brain_hurt(brain, attacker);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool BindBrain() {
    if (g_get_target && g_closest && g_set_target && g_brain_hurt) return true;
    HMODULE common = GetModuleHandleA("CommonDll7.dll");
    if (!common) return false;
    g_get_target = reinterpret_cast<GetEntFn>(
        core::find_export(common, "?GetTarget@cAIHuntBrain_Animal@@QBEPAVcObjEntity@@XZ"));
    g_closest = reinterpret_cast<GetEntFn>(
        core::find_export(common, "?GetClosestAttackTarget@cAIHuntBrain_Animal@@IAEPAVcObjEntity@@XZ"));
    g_set_target = reinterpret_cast<SetTargetFn>(
        core::find_export(common, "?SetTarget@cAIHuntBrain_Animal@@QAEXPAVcObjEntity@@M@Z"));
    g_brain_hurt = reinterpret_cast<BrainHurtFn>(
        core::find_export(common, "?OnHurt@cAIHuntBrain_Animal@@UAEXPAVcObjEntity@@@Z"));
    return g_get_target && g_closest && g_set_target && g_brain_hurt;
}

bool PuppetsAreTargets() { return NetIsHost() && entity::AttackTargetCount() > 1; }

bool IsPlayerEntity(void* ent) {
    return ent && (ent == LocalPlayer().entity || puppets::PlayerOf(ent) != net::kInvalidPlayerId);
}

float DistSq(void* a, void* b) {
    float pa[3] = {}, pb[3] = {};
    core::safe_copy(static_cast<char*>(a) + kEntPos, pa, sizeof pa);
    core::safe_copy(static_cast<char*>(b) + kEntPos, pb, sizeof pb);
    const float dx = pa[0] - pb[0], dy = pa[1] - pb[1], dz = pa[2] - pb[2];
    return dx * dx + dy * dy + dz * dz;
}

void RetargetToNearestPlayer() {
    if (!PuppetsAreTargets() || !BindBrain()) return;
    for (void* animal : Animals()) {
        void* brain = nullptr;
        if (!core::safe_read(static_cast<char*>(animal) + kEntBrain, brain) || !brain) continue;
        void* cur = guarded_get(g_get_target, brain);
        if (!IsPlayerEntity(cur)) continue;  // not chasing a player (or a scripted target): leave it alone
        void* best = guarded_get(g_closest, brain);
        if (!best || best == cur || !IsPlayerEntity(best)) continue;
        const float d_cur = std::sqrt(DistSq(animal, cur)), d_best = std::sqrt(DistSq(animal, best));
        if (d_cur - d_best < kRetargetMargin) continue;
        void* task = nullptr;
        core::safe_read(static_cast<char*>(brain) + kBrainTask, task);
        int task_state = 0;
        if (task) core::safe_read(static_cast<char*>(task) + 4, task_state);
        if (task && task_state != 1) continue;  // ProcessAttackStuck only switches a task in state 1
        const bool ok = guarded_switch(brain, task, cur, best);
        spdlog::info("combat: animal id {} switches to the nearer player {} ({:.1f} m, was {:.1f} m) ok={}", IdOf(animal),
                     puppets::PlayerOf(best) == net::kInvalidPlayerId ? 0 : puppets::PlayerOf(best), d_best, d_cur, ok);
    }
}

// ---- host: an attacking animal treats a living puppet as a player ----
// re/notes/animal-park.md (Cursor worker, 2026-10-17). The attack code asks its target "are you the player" (entity
// vt +0xFC: 1 on cScr_ModHunter, 0 on a puppet). For a puppet, cAIHuntBrain_Animal::ProcessAttackState fell through
// to DoDesperateAttack / DoDesorient, which ends in the flee-point callback that parks the animal at -10000 (the
// MAP2A lion vanished on the host mid-attack on the client; the client's mirror followed it there mid-pounce and
// the client crashed). After each of the three calls, a mid hook turns a 0 into 1 when the target is a living
// puppet, before the existing `cmp eax, 1`. CommonDll7 v1.0, bytes checked before hooking.
struct PlayerTestSite {
    uintptr_t verify_rva;  // first byte of the checked run (mov ...; mov ecx, target; call [vt+0xFC]; cmp eax,1; jcc)
    uint8_t bytes[15];
    uintptr_t cmp_rva;     // the `cmp eax, 1` after the call: the hook site
    bool target_in_esi;    // else EBX
    const char* label;
};
constexpr PlayerTestSite kPlayerTests[] = {
    {0x9278e, {0x8b, 0x06, 0x8b, 0xce, 0xff, 0x90, 0xfc, 0x00, 0x00, 0x00, 0x83, 0xf8, 0x01, 0x74, 0x1a}, 0x92798, true,
     "ProcessAttackState player test"},
    {0xcaec3, {0x8b, 0x13, 0x8b, 0xcb, 0xff, 0x92, 0xfc, 0x00, 0x00, 0x00, 0x83, 0xf8, 0x01, 0x0f, 0x84}, 0xcaecd, false,
     "sync-attack player wait"},
    {0xcb14f, {0x8b, 0x03, 0x8b, 0xcb, 0xff, 0x90, 0xfc, 0x00, 0x00, 0x00, 0x83, 0xf8, 0x01, 0x75, 0x06}, 0xcb159, false,
     "sync-attack timer reset"},
};
safetyhook::MidHook g_player_test_hooks[3];
uint32_t g_player_test_logs = 0;

void puppet_counts_as_player(uintptr_t target, uintptr_t& eax) {
    if (eax == 1 || !PuppetsAreTargets()) return;
    if (!puppets::IsLivingPuppet(reinterpret_cast<void*>(target))) return;
    eax = 1;
    if (g_player_test_logs++ < 5)
        spdlog::info("combat: animal attack treats player {} as a player", puppets::PlayerOf(reinterpret_cast<void*>(target)));
}

void on_player_test_esi(safetyhook::Context& ctx) { puppet_counts_as_player(ctx.esi, ctx.eax); }
void on_player_test_ebx(safetyhook::Context& ctx) { puppet_counts_as_player(ctx.ebx, ctx.eax); }

void InstallPlayerTests(HMODULE common) {
    const uintptr_t base = reinterpret_cast<uintptr_t>(common);
    for (size_t i = 0; i < std::size(kPlayerTests); ++i) {
        const PlayerTestSite& s = kPlayerTests[i];
        uint8_t got[15] = {};
        if (!core::safe_copy(reinterpret_cast<void*>(base + s.verify_rva), got, sizeof got) ||
            std::memcmp(got, s.bytes, sizeof got) != 0) {
            spdlog::error("combat: {} at CommonDll7+{:#x} has unexpected bytes; not hooked", s.label, s.verify_rva);
            continue;
        }
        auto created = safetyhook::MidHook::create(reinterpret_cast<void*>(base + s.cmp_rva),
                                                   s.target_in_esi ? &on_player_test_esi : &on_player_test_ebx);
        if (!created) {
            spdlog::error("combat: {} mid hook failed (safetyhook error {})", s.label,
                          static_cast<int>(created.error().type));
            continue;
        }
        g_player_test_hooks[i] = std::move(*created);
        spdlog::info("combat: hooked {} (CommonDll7+{:#x})", s.label, s.cmp_rva);
    }
}

}  // namespace

// Host, from animals.cpp after a client's AnimalHit is applied: the animal's brain hears that the client's puppet
// hurt it (cAIHuntBrain_Animal::OnHurt 0x83ec0 switches to an attacker that is in the attack list).
void OnClientHitAnimal(void* animal, uint8_t shooter) {
    if (!animal || !PuppetsAreTargets() || !BindBrain()) return;
    void* puppet = puppets::EntityOf(shooter);
    void* brain = nullptr;
    if (!puppet || !core::safe_read(static_cast<char*>(animal) + kEntBrain, brain) || !brain) return;
    const bool ok = guarded_brain_hurt(brain, puppet);
    spdlog::info("combat: animal id {} hurt by player {}: brain told ok={}", IdOf(animal), shooter, ok);
}

namespace {

class CombatModule : public dh2::Module {
public:
    const char* Name() const override { return "combat"; }
    int Priority() const override { return 46; }

    void OnInit() override {
        events::SetHandler(events::kPlayerHurt, nullptr, &OnHurtCommit);
        events::SetHandler(events::kPlayerKnock, nullptr, &OnKnockCommit);
        events::SetHandler(events::kPlayerKnockEnd, nullptr, &OnKnockEndCommit);
        events::SetHandler(events::kKnockRequest, &OnKnockRequest, nullptr);
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        g_knock_on_shot = GetPrivateProfileIntA("Combat", "KnockOnShot", 1, ini.c_str()) != 0;
        g_knock_on_melee = GetPrivateProfileIntA("Combat", "KnockOnMelee", 1, ini.c_str()) != 0;
        g_pvp_knock_ms = static_cast<uint32_t>(GetPrivateProfileIntA("Combat", "KnockMs", 2600, ini.c_str()));
        spdlog::info("combat: knock on shot={} melee={} ms={}", g_knock_on_shot, g_knock_on_melee, g_pvp_knock_ms);
    }

    void OnEngineReady(HMODULE) override {
        HMODULE common = core::wait_for_module("CommonDll7.dll", 10000);
        if (!common) return;
        g_hurt_hook = core::install_hook(
            "cObjCharacter::OnHurt (combat)",
            core::find_export(common, "?OnHurt@cObjCharacter@@UAEXPAVcObjScriptedEntity@@MHABUtVect3@@1@Z"),
            reinterpret_cast<void*>(&hk_CharacterHurt));
        g_playsa_hook = core::install_hook("cObjCharacterEx::PlaySA(m4x4) (combat)",
                                           core::find_export(common, "?PlaySA@cObjCharacterEx@@UAEXPADAAUm4x4@@@Z"),
                                           reinterpret_cast<void*>(&hk_PlaySA));
        g_endsa_hook = core::install_hook("cObjCharacterEx::OnEndSA (combat)",
                                          core::find_export(common, "?OnEndSA@cObjCharacterEx@@UAEXXZ"),
                                          reinterpret_cast<void*>(&hk_OnEndSA));
        InstallPlayerTests(common);
        g_stuck_hook = core::install_hook("cAIHuntBrain_Animal::ProcessAttackStuck (combat)",
                                          core::find_export(common, "?ProcessAttackStuck@cAIHuntBrain_Animal@@MAEXXZ"),
                                          reinterpret_cast<void*>(&hk_ProcessAttackStuck));
    }

    void OnGameDllLoaded(HMODULE game) override { g_game_base = reinterpret_cast<uintptr_t>(game); }

    void OnMapUnloading() override {
        g_knocked = false;
    }

    void OnFrame(float) override {
        if (!cmd_registered_ && console_cmds::Ready()) cmd_registered_ = console_cmds::AddCommandStr("coop_knock", &cmd_knock);
        const uint32_t now = GetTickCount();
        if (now - retarget_ms_ >= 500) {
            retarget_ms_ = now;
            RetargetToNearestPlayer();
        }
        if (g_knocked && g_knock_end_ms && static_cast<int32_t>(now - g_knock_end_ms) >= 0) EndKnock("player knock done");
        if (g_knocked && now - g_knock_ms > kKnockTimeoutMs) EndKnock("timeout");
        const LocalPlayerSample& lp = LocalPlayer();
        if (!NetIsOnline() || !lp.valid) {
            have_counts_ = false;
            return;
        }
        if (have_counts_) {
            if (g_knock_on_shot && lp.shots != shots_) TryKnock(false);
            if (g_knock_on_melee && lp.melees != melees_) TryKnock(true);
        }
        shots_ = lp.shots;
        melees_ = lp.melees;
        have_counts_ = true;
    }

private:
    uint32_t retarget_ms_ = 0;
    bool cmd_registered_ = false;
    bool have_counts_ = false;
    uint8_t shots_ = 0;
    uint32_t melees_ = 0;
};

DH2_MODULE(CombatModule)

}  // namespace
}  // namespace dh2::coop::combat
