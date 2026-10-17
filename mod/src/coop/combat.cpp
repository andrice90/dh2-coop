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

namespace dh2::coop::combat {
namespace {

constexpr uintptr_t kBrainTargetId = 0x254;  // cAIHuntBrain_Animal::SetTarget stores the target entity id here
constexpr uintptr_t kEntPhys = 0x118;        // physics body; ProcessAttackStuck reads its radius at +0x7c
constexpr uintptr_t kEntPos = 0xE8;
constexpr uintptr_t kInSA = 0x368;           // cObjCharacterEx: inside a sync attack (PlaySA .. OnEndSA)
constexpr uintptr_t kHunterDead = 0x6CC;     // cObjHunter::OnDie sets it; Hurt returns at once while it is set
constexpr uintptr_t kRvaHunterVt = 0x106ff0; // GameDll7 cScr_ModHunter
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
                                 const void* dir) {
    if (!g_in_apply && NetIsHost()) {
        const uint8_t who = puppets::PlayerOf(self);
        if (who != net::kInvalidPlayerId) {
            const int16_t aid = IdOf(attacker);
            const uint32_t packed = static_cast<uint16_t>(aid) | (static_cast<uint32_t>(type & 0xFF) << 16);
            spdlog::info("combat: animal id {} hurt player {} for {:.1f} (type {})", aid, who, dmg, type);
            events::Send(events::kPlayerHurt, who, packed, dmg, "", 0);
            return;
        }
    }
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
    if (!h || !g_game_base || !core::safe_read(h, vt) || vt != g_game_base + kRvaHunterVt) return nullptr;
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
    const float yaw = lp.viewYaw * 3.14159265f / 180.f, pitch = lp.viewPitch * 3.14159265f / 180.f;
    const float d[3] = {std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
    const SessionView v = Snapshot();
    uint8_t best = net::kInvalidPlayerId;
    float best_t = melee ? 2.2f : 80.f;
    for (int i = 0; i < v.remoteCount; ++i) {
        const RemoteView& r = v.remotes[i];
        if (r.packets == 0) continue;
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

void TryKnock(bool melee) {
    const uint8_t victim = PickVictim(melee);
    if (victim == net::kInvalidPlayerId) return;
    const uint32_t how = melee ? 2u : 1u;
    if (NetIsHost()) RouteKnock(NetLocalId(), victim, how);
    else events::Request(events::kKnockRequest, victim, how, 0.f, "");
}

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
        g_stuck_hook = core::install_hook("cAIHuntBrain_Animal::ProcessAttackStuck (combat)",
                                          core::find_export(common, "?ProcessAttackStuck@cAIHuntBrain_Animal@@MAEXXZ"),
                                          reinterpret_cast<void*>(&hk_ProcessAttackStuck));
    }

    void OnGameDllLoaded(HMODULE game) override { g_game_base = reinterpret_cast<uintptr_t>(game); }

    void OnMapUnloading() override {
        g_knocked = false;
    }

    void OnFrame(float) override {
        const uint32_t now = GetTickCount();
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
    bool have_counts_ = false;
    uint8_t shots_ = 0;
    uint32_t melees_ = 0;
};

DH2_MODULE(CombatModule)

}  // namespace
}  // namespace dh2::coop::combat
