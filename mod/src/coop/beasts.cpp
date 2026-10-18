// Species and weapons a client uses count the same as the host's. Design and evidence: re/notes/beasts.md.
//
// Tranquilizer darts (MAP6/MAP7 give the player nothing else). cScrAnimal::OnHit (GameDll7 0x1a680) on a
// mod_obj_tranq_bullet that hit a bone sets the tranq state +0x4f0 = 2, credits the kill (+0x420 = 1) and hurts by
// +0x4f8, which is 0 on every story species. Update (0x1f3c0) then counts +0x4f4 down and runs the death slot +0x154:
// the animal falls asleep, UpdateMissionStatus counts it. A dart into a protected animal (+0x604) that is already
// tranquilized is an overdose: mission failed (0x965e0). A client's dart never changed health, so the mirror's
// health-drop AnimalHit never fired and the host never knew. Client: run OnHit for the local hit effects with the
// tranq damage and state neutralised, then request kTranqHit. Host: the same branch on its animal.
//
// Mission cobras. cObjCobra::OnHit (0x80440) never subtracts health either: a bone hit sets body state Death and the
// +0x738 timer (re/notes/snake.md). A client's bullet or dart on a mirrored cobra becomes an AnimalHit, which the host
// already turns into the cobra's own death timer (animals.cpp StartCobraDeath).
//
// Clip coverage. cModelPart::SetAnim(int,float,float) (EngineDll7 0x1e73b0) forwards every clip it sets to the parts
// linked with it (a sync attack ring) and raises "Could not forward animation" when a partner has neither
// "<prefix>/<clip>" nor "<clip>". That is what killed the MAP4A client's script VM: the lioness model has idleattack1;
// its copy was still linked to a hunter body after a tackle. A clip the part itself lacks is not forwarded (the set
// fails quietly and stops the part's animation: the bind pose on girl/fiancée NPCs).
#include "coop/beasts.h"

#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "coop/animals.h"
#include "coop/coop.h"
#include "coop/events.h"
#include "coop/player_tracker.h"
#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/console_cmds.h"
#include "engine/entity_api.h"
#include "net/protocol.h"

namespace dh2::coop::combat {
void OnClientHitAnimal(void* animal, uint8_t shooter);  // combat.cpp: the brain hears the shooter's puppet
}

namespace dh2::coop::beasts {
namespace {

// GameDll7 v1.0 (build 434). Every hooked or called body starts 55 8b ec 6a ff (checked before use).
constexpr uintptr_t kRvaAnimalOnHit = 0x1a680;  // cScrAnimal vt +0x8c: thiscall(animal, bullet, tVect3* pos, tVect3* dir)
constexpr uintptr_t kRvaCobraOnHit = 0x80440;   // cObjCobra vt +0x8c, same arguments
constexpr uintptr_t kRvaMissionFail = 0x965e0;  // thiscall(cMission*, const char*): status 3, then 0x91c20(6, text)
constexpr uintptr_t kRvaMission = 0x1869e8;     // cMission*
constexpr uintptr_t kVtAnimalRva = 0xff470;     // cScrAnimal (obj_animal)
constexpr uintptr_t kVtCrocodileRva = 0xffa50;  // cObjCrocodile (obj_crocodile)
constexpr uintptr_t kVtCobraRva = 0x10e920;     // cObjCobra (obj_cobra)
constexpr uintptr_t kVtOnHit = 0x8c;

constexpr uintptr_t kEntId = 0x04;
constexpr uintptr_t kEntParent = 0xA4;
constexpr uintptr_t kEntModel = 0xE4;
constexpr uintptr_t kEntModelName = 0xE0;
constexpr uintptr_t kEntPos = 0xE8;
constexpr uintptr_t kEntHealth = 0x19C;
// cScrAnimal (OnHit 0x1a680, hurt vt +0x228 = 0x13d80, Update 0x1f3c0, death vt +0x154 = 0x1e8b0)
constexpr uintptr_t kHitList = 0x3c4;     // hit-bone list (AddHitBone); entries of 0xc bytes at *(list +0x10)
constexpr uintptr_t kHitBone = 0x3d8;     // entry the bullet ray hit; OnHit needs >= 0 and resets it to -1
constexpr uintptr_t kKillAction = 0x420;  // MISSION_ACTION the death reports (OnHit writes 1 for a bone hit)
constexpr uintptr_t kTranqState = 0x4f0;  // 0 none, 2 tranquilized, 1 asleep (death ran), 3 overdosed
constexpr uintptr_t kTranqTime = 0x4f4;   // seconds before the sleep (SetTranqTime; MoveToPos default 5.0)
constexpr uintptr_t kTranqHurt = 0x4f8;   // damage per dart (SetTranqHurt; 0 unless set)
constexpr uintptr_t kDeathDone = 0x600;   // death slot ran
constexpr uintptr_t kProtected = 0x604;   // IsProtected()
// Bullet (cMOD_ObjBullet ctor 0x2b590, Start 0x2b0b0): +0x1b8 the firing weapon item, +0x1bc its parent (the hunter),
// +0x1b4 type (2 = damage without a hit bone, no tranq branch).
constexpr uintptr_t kBulletWeapon = 0x1b8;
constexpr uintptr_t kBulletHunter = 0x1bc;
constexpr uintptr_t kBulletType = 0x1b4;
constexpr uintptr_t kCobraTimer = 0x738;  // OnHit writes 50.0 on a bone hit
constexpr float kCobraHitTimer = 50.f;
constexpr const char* kOverdoseText = "MISSION FAILED\nYou've injected an overdose \nand the animal died!";
constexpr uint32_t kDartRepeatMs = 300;   // one dart is one OnHit; a second request this soon is the same dart
constexpr uint32_t kCobraRepeatMs = 1000;

using OnHitFn = void(__thiscall*)(void* self, void* bullet, const float* pos, const float* dir);
using KindOfFn = int(__thiscall*)(void* self, const char* name);
using MissionFailFn = void(__thiscall*)(void* mission, const char* text);
using AnimIndexFn = int(__thiscall*)(void* part, const char* name);
using LinkValidFn = int(__thiscall*)(void* link);
using LinkPartFn = void*(__thiscall*)(void* link);
using BrainParamFn = char*(__thiscall*)(void* ent, char* name);

HMODULE g_game = nullptr;
safetyhook::InlineHook* g_onhit_hook = nullptr;
safetyhook::InlineHook* g_cobra_hook = nullptr;
KindOfFn g_kind_of = nullptr;
MissionFailFn g_mission_fail = nullptr;
AnimIndexFn g_anim_index = nullptr;
LinkValidFn g_link_valid = nullptr;
LinkPartFn g_link_part = nullptr;
BrainParamFn g_brain_param = nullptr;

bool g_tranq_clients = true;    // [Animals] TranqFromClients
bool g_overdose_clients = false; // [Animals] TranqOverdoseFromClients: another player's second dart can fail the mission
bool g_cobra_clients = true;    // [Animals] CobraHitsFromClients
bool g_clip_check = true;       // [Animals] ClipCheck
bool g_cmd = false;
void* g_test_bullet = nullptr;  // coop_tranq: one unspawned mod_obj_tranq_bullet per map (never added to the manager)
std::map<uint16_t, uint32_t> g_last_dart_ms;
std::map<uint16_t, uint8_t> g_tranq_by;  // host: animal id -> player whose dart tranquilized it

// Host: a dart from `shooter` on an animal that is already tranquilized and protected. The game's overdose rule
// (mission failed) applies to the player who darted it; another player's dart is ignored unless
// TranqOverdoseFromClients=1 (user decision 2026-10-18).
bool OverdoseIgnored(int id, uint8_t shooter) {
    if (g_overdose_clients || id < 0) return false;
    auto it = g_tranq_by.find(static_cast<uint16_t>(id));
    return it != g_tranq_by.end() && it->second != shooter;
}
std::map<uint16_t, uint32_t> g_last_cobra_ms;
std::set<std::string> g_logged;  // one line per model/state/clip finding

std::string ini_path() { return (core::self_dir() / "dh2coop.ini").string(); }

void* Rva(uintptr_t rva) { return g_game ? reinterpret_cast<char*>(g_game) + rva : nullptr; }

// `chain`: another module of this mod may have hooked the function already (animals.cpp hooks cScrAnimal::OnHit, 0x1a680,
// first: its prologue is a jmp rel32 by now). safetyhook relocates that jmp into the new trampoline, so the hooks chain.
void* CheckedRva(uintptr_t rva, const char* label, bool chain = false) {
    static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
    void* p = Rva(rva);
    uint8_t got[5] = {};
    if (chain && p && core::safe_copy(p, got, sizeof got) && got[0] == 0xE9) {
        spdlog::info("beasts: {} at GameDll7+{:#x} is already hooked by another module; chaining", label, rva);
        return p;
    }
    if (!p || !core::safe_copy(p, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
        spdlog::error("beasts: {} at GameDll7+{:#x} has unexpected bytes; not used", label, rva);
        return nullptr;
    }
    return p;
}

template <class T>
T Read(void* ent, uintptr_t off, T fallback) {
    T v = fallback;
    if (!ent || !core::safe_read(static_cast<char*>(ent) + off, v)) return fallback;
    return v;
}

template <class T>
bool Write(void* ent, uintptr_t off, T v) {
    if (!ent) return false;
    __try {
        *reinterpret_cast<T*>(static_cast<char*>(ent) + off) = v;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int IdOf(void* ent) { return Read<int16_t>(ent, kEntId, -1); }

uint32_t NowMs() { return GetTickCount(); }

bool HasVt(void* ent, uintptr_t rva) {
    return ent && g_game && Read<uintptr_t>(ent, 0, 0) == reinterpret_cast<uintptr_t>(g_game) + rva;
}

// The two classes that run cScrAnimal::OnHit and its tranq branch.
bool IsDartTarget(void* ent) { return HasVt(ent, kVtAnimalRva) || HasVt(ent, kVtCrocodileRva); }
bool IsCobra(void* ent) { return HasVt(ent, kVtCobraRva); }

bool KindOf(void* ent, const char* name) {
    if (!ent || !g_kind_of) return false;
    __try {
        return g_kind_of(ent, name) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool InAnimals(void* ent) {
    for (void* a : Animals()) {
        if (a == ent) return true;
    }
    return false;
}

// Client of a session on the host's map: every animal of the tracker list is a mirror (brain frozen, pose from the host).
bool MirrorClient() { return NetIsOnline() && !NetIsHost() && events::SameMapAsHost(); }

bool CallOnHit(safetyhook::InlineHook* hook, void* self, void* bullet, const float* pos, const float* dir) {
    __try {
        hook->unsafe_thiscall<void>(self, bullet, pos, dir);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CallVirtualOnHit(void* self, void* bullet, const float* pos, const float* dir) {
    __try {
        void** vt = *reinterpret_cast<void***>(self);
        reinterpret_cast<OnHitFn>(vt[kVtOnHit / sizeof(void*)])(self, bullet, pos, dir);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CallMissionFail(void* mission, const char* text) {
    __try {
        g_mission_fail(mission, text);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void* Mission() { return Read<void*>(g_game, kRvaMission, nullptr); }

// ---- tranquilizer darts ----

// Client: a dart on a mirror. OnHit runs for its local effects (sound, flinch, impulse) with +0x4f8 = 0, so the copy
// takes no damage, and +0x4f0 is put back afterwards, so this copy's Update never runs its own sleep (that is the
// host's, through kAnimalDied) and a later dart never reads as an overdose here.
void ClientDart(void* self, void* bullet, const float* pos, const float* dir) {
    const int id = IdOf(self);
    const int bone = Read<int>(self, kHitBone, -1);
    const int type = Read<int>(bullet, kBulletType, 0);
    const int state = Read<int>(self, kTranqState, 0);
    const float hurt = Read<float>(self, kTranqHurt, 0.f);
    const float hp = Read<float>(self, kEntHealth, 0.f);
    Write<float>(self, kTranqHurt, 0.f);
    const bool ok = CallOnHit(g_onhit_hook, self, bullet, pos, dir);
    Write<float>(self, kTranqHurt, hurt);
    Write<int>(self, kTranqState, state);
    // Type 2 bullets take OnHit's damage path, not the tranq branch: their health drop is the usual AnimalHit.
    if (bone < 0 || type == 2 || id < 0) {
        spdlog::info("beasts: client dart id={} missed the hit bones (bone={} type={}) ok={}", id, bone, type, ok);
        return;
    }
    const uint32_t now = NowMs();
    uint32_t& last = g_last_dart_ms[static_cast<uint16_t>(id)];
    if (last != 0 && now - last < kDartRepeatMs) return;
    last = now;
    events::Request(events::kTranqHit, static_cast<uint16_t>(id), static_cast<uint32_t>(bone), 0.f, "");
    spdlog::info("beasts: client dart id={} bone={} hp={:.1f} (local effects ok={}) -> host", id, bone, hp, ok);
}

// Host: OnHit's tranq branch for a client's dart (0x1a680 after the hit-bone lookup, then the hurt 0x13d80 with the
// tranq damage, then the brain's OnHurt). The hit sound and flinch belong to the shooter's screen and stay there.
void HostDart(void* ent, uint8_t shooter) {
    const int id = IdOf(ent);
    float hp = Read<float>(ent, kEntHealth, 0.f);
    const float y = Read<float>(ent, kEntPos + 4, 0.f);
    const int done = Read<int>(ent, kDeathDone, 0);
    if (hp <= 0.f || done != 0 || y < -1000.f) {
        spdlog::info("beasts: dart from player {} on id={} ignored (hp {:.1f}, death ran {}, y {:.0f})", shooter, id, hp,
                     done, y);
        return;
    }
    const int state = Read<int>(ent, kTranqState, 0);
    const int prot = Read<int>(ent, kProtected, 0);
    const float hurt = Read<float>(ent, kTranqHurt, 0.f);
    const float time = Read<float>(ent, kTranqTime, 0.f);
    if (state == 2 && prot == 1) {
        if (OverdoseIgnored(id, shooter)) {
            spdlog::info("beasts: dart from player {} on protected id={} tranquilized by player {}: ignored "
                         "([Animals] TranqOverdoseFromClients=0)", shooter, id, g_tranq_by[static_cast<uint16_t>(id)]);
            return;
        }
        Write<int>(ent, kKillAction, 1);
        Write<int>(ent, kTranqState, 3);
        void* mission = Mission();
        const bool failed = mission && g_mission_fail && CallMissionFail(mission, kOverdoseText);
        spdlog::warn("beasts: dart from player {} on tranquilized protected id={}: overdose, mission failed ok={}",
                     shooter, id, failed);
        return;
    }
    Write<int>(ent, kKillAction, 1);
    Write<int>(ent, kTranqState, 2);
    if (state != 2) g_tranq_by[static_cast<uint16_t>(id)] = shooter;
    bool slept = false;
    if (hurt > 0.f) {
        hp -= hurt;
        if (hp < 0.f) {
            // The hurt function: a tranquilized animal whose health runs out falls asleep at once.
            Write<float>(ent, kEntHealth, 0.f);
            Write<int>(ent, kTranqState, 1);
            slept = entity::PresentDeath(ent);
        } else {
            Write<float>(ent, kEntHealth, hp);
        }
    }
    if (!slept) combat::OnClientHitAnimal(ent, shooter);
    spdlog::info("beasts: host ran player {}'s dart on id={}: tranq {}->{} sleeps in {:.1f} s protected={} hurt={:.1f} "
                 "hp={:.1f}{}",
                 shooter, id, state, Read<int>(ent, kTranqState, 0), time, prot, hurt, Read<float>(ent, kEntHealth, 0.f),
                 slept ? " (asleep now)" : "");
}

void OnTranqRequest(const net::GameEvent& ev) {
    if (!g_tranq_clients) return;
    void* ent = animals::Find(ev.subjectId);
    if (!IsDartTarget(ent)) {
        spdlog::warn("beasts: dart request id={} from player {}: not a cScrAnimal here", ev.subjectId, ev.actorId);
        return;
    }
    HostDart(ent, ev.actorId);
}

// Host's own dart (and coop_tranq on the host): the real branch, logged so the client emulation can be compared.
void HostOwnDart(void* self, void* bullet, const float* pos, const float* dir) {
    const int state = Read<int>(self, kTranqState, 0);
    const float hp = Read<float>(self, kEntHealth, 0.f);
    const int bone = Read<int>(self, kHitBone, -1);
    const int id = IdOf(self);
    if (state == 2 && Read<int>(self, kProtected, 0) == 1 && OverdoseIgnored(id, NetLocalId())) {
        spdlog::info("beasts: host dart on protected id={} tranquilized by player {}: ignored", id,
                     g_tranq_by[static_cast<uint16_t>(id)]);
        return;
    }
    const bool ok = CallOnHit(g_onhit_hook, self, bullet, pos, dir);
    if (state != 2 && Read<int>(self, kTranqState, 0) == 2 && id >= 0) g_tranq_by[static_cast<uint16_t>(id)] = NetLocalId();
    spdlog::info("beasts: host dart id={} bone={} tranq {}->{} sleeps in {:.1f} s action={} protected={} hp {:.1f}->{:.1f} "
                 "ok={}",
                 IdOf(self), bone, state, Read<int>(self, kTranqState, 0), Read<float>(self, kTranqTime, 0.f),
                 Read<int>(self, kKillAction, 0), Read<int>(self, kProtected, 0), hp, Read<float>(self, kEntHealth, 0.f),
                 ok);
}

void __fastcall hk_AnimalOnHit(void* self, void*, void* bullet, const float* pos, const float* dir) {
    if (g_tranq_clients && NetIsOnline() && KindOf(bullet, "mod_obj_tranq_bullet")) {
        if (MirrorClient() && InAnimals(self)) {
            ClientDart(self, bullet, pos, dir);
            return;
        }
        if (NetIsHost()) {
            HostOwnDart(self, bullet, pos, dir);
            return;
        }
    }
    g_onhit_hook->unsafe_thiscall<void>(self, bullet, pos, dir);
}

// ---- mission cobras ----

void __fastcall hk_CobraOnHit(void* self, void*, void* bullet, const float* pos, const float* dir) {
    if (!g_cobra_clients || !MirrorClient() || !InAnimals(self) || !KindOf(bullet, "mod_obj_bullet")) {
        g_cobra_hook->unsafe_thiscall<void>(self, bullet, pos, dir);
        return;
    }
    const float before = Read<float>(self, kCobraTimer, 0.f);
    const bool ok = CallOnHit(g_cobra_hook, self, bullet, pos, dir);
    const float after = Read<float>(self, kCobraTimer, 0.f);
    if (after != kCobraHitTimer || before == kCobraHitTimer) return;  // the bone ray missed
    Write<float>(self, kCobraTimer, before);  // this copy never runs the timer (its bite pointer is cleared)
    const int id = IdOf(self);
    if (id < 0) return;
    const uint32_t now = NowMs();
    uint32_t& last = g_last_cobra_ms[static_cast<uint16_t>(id)];
    if (last != 0 && now - last < kCobraRepeatMs) return;
    last = now;
    net::AnimalHit hit;
    hit.animalId = static_cast<uint16_t>(id);
    hit.damage = 1.f;  // the host's AnimalHit on a cobra starts its Death timer whatever the amount
    if (pos) std::memcpy(hit.hitPos, pos, sizeof hit.hitPos);
    const bool sent = ReportAnimalHit(hit);
    spdlog::info("beasts: client {} hit cobra id={} (local ok={}) -> host AnimalHit sent={}",
                 KindOf(bullet, "mod_obj_tranq_bullet") ? "dart" : "bullet", id, ok, sent);
}

// ---- clip coverage ----

void* GuardedPart0(void* ent) {
    __try {
        void* model = *reinterpret_cast<void**>(static_cast<char*>(ent) + kEntModel);
        if (!model) return nullptr;
        void** vt = *reinterpret_cast<void***>(model);
        const short parts = reinterpret_cast<short(__thiscall*)(void*)>(vt[0xf8 / 4])(model);
        if (parts <= 0) return nullptr;
        return reinterpret_cast<void*(__thiscall*)(void*, int)>(vt[0xf0 / 4])(model, 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

int GuardedAnimIndex(void* part, const char* clip) {
    __try {
        return g_anim_index(part, clip);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -2;
    }
}

int AnimIndex(void* part, const char* clip) { return part && clip && g_anim_index ? GuardedAnimIndex(part, clip) : -2; }

// Parts the engine forwards a clip to: the ring that starts at part +0xf4 and continues at each partner's +0x10c
// (cModelPart::SetAnim(int,float,float) loop). At most `max`.
int GuardedPartners(void* part, void** out, int max) {
    __try {
        int n = 0;
        void* link = static_cast<char*>(part) + 0xf4;
        while (n < max && g_link_valid(link)) {
            void* p = g_link_part(link);
            if (!p || p == part) break;
            out[n++] = p;
            link = static_cast<char*>(p) + 0x10c;
        }
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

int Partners(void* part, void** out, int max) {
    return part && g_link_valid && g_link_part ? GuardedPartners(part, out, max) : 0;
}

// Same names the forward tries: "<this part's prefix>/<clip>" (unless both parts share a model or the partner takes
// plain names, +0x8c bit 0), then "<clip>".
bool PartnerTakes(void* part, void* partner, const char* clip) {
    const void* model = Read<void*>(part, 0x68, nullptr);
    const void* pmodel = Read<void*>(partner, 0x68, nullptr);
    const uint8_t plain = Read<uint8_t>(partner, 0x8c, 0);
    if (pmodel != model && (plain & 1) == 0) {
        const char* prefix = Read<const char*>(part, 0x5c, nullptr);
        if (prefix) {
            char name[264];
            std::snprintf(name, sizeof name, "%s/%s", core::safe_cstr(prefix, 120).c_str(), clip);
            if (AnimIndex(partner, name) >= 0) return true;
        }
    }
    return AnimIndex(partner, clip) >= 0;
}

// The clip can play on this part without a raise: the part has it and every linked part takes it.
bool Playable(void* part, const char* clip, std::string* why) {
    if (AnimIndex(part, clip) < 0) {
        if (why) *why = "model has no clip";
        return false;
    }
    void* partners[8] = {};
    const int n = Partners(part, partners, 8);
    for (int i = 0; i < n; ++i) {
        if (PartnerTakes(part, partners[i], clip)) continue;
        if (why) {
            void* pm = Read<void*>(partners[i], 0x68, nullptr);
            const char* pname = pm ? Read<const char*>(pm, 0x10, nullptr) : nullptr;
            *why = "linked part of '" + (pname ? core::safe_cstr(pname, 120) : std::string("?")) + "' cannot take it";
        }
        return false;
    }
    return true;
}

std::string ModelName(void* ent) {
    const char* m = Read<const char*>(ent, kEntModelName, nullptr);
    return m ? core::safe_cstr(m, 160) : std::string();
}

bool LogOnce(const std::string& key) { return g_logged.insert(key).second; }

// cAnimalLegs (SCRIPTS/MODELS/ANIMALS/ANIMALLEGS.SCR; every animal's legs machine, or cChargerLegs, which only
// changes Attack): the literal clips each state's Begin/Update/End sets, and the brain parameters it asks for a clip
// name (cAIHuntBrain_Animal::GetParam: idleanim/stalkanim/attackanim come from the current task).
struct StateClips {
    const char* state;
    const char* clips[8];
    const char* brain_param;
};
constexpr StateClips kAnimalStates[] = {
    {"Idle", {"runtoidle", "walktoidle"}, "idleanim"},
    {"IdleWait", {"idleattack1"}, nullptr},
    {"IdleNonAttack", {"idle1", "idle2", "idle3", "runtoidle", "walktoidle", "beginchain", "loop", "endchain"}, nullptr},
    {"Walk", {"walk", "idletowalk", "runtowalk"}, nullptr},
    {"WalkGraze", {"walk", "idletowalk", "runtowalk"}, nullptr},
    {"Run", {"run", "circlerun", "idletorun", "walktorun"}, nullptr},
    {"IdleCall", {"call", "runtoidle", "walktoidle"}, nullptr},
    {"Climb", {"climb"}, nullptr},
    {"ShortJump", {"shortjump"}, nullptr},
    {"Fishing", {"fishing"}, nullptr},
    {"Eating", {"eating"}, nullptr},
    {"IdleAttack", {}, "idleanim"},
    {"Stalk", {"runtostalk"}, "stalkanim"},
    {"Turnleft22", {"turnleft22f", "turnleft22"}, nullptr},
    {"Limp", {"circlerun", "rundisabled"}, nullptr},
    {"Jump", {"leap"}, nullptr},
    {"ScareC", {"scare"}, nullptr},
    {"Alert", {"alert"}, nullptr},
    {"RisingAlert", {"risealert"}, nullptr},
    {"Panic", {"idle1"}, nullptr},
    {"Lying", {"lying"}, nullptr},
    {"Siting", {"sitting"}, nullptr},
    {"Rising", {"rise"}, nullptr},
    {"SitGraze", {"sitting"}, nullptr},
    {"Graze", {"idle1", "idle2", "idle3", "runtoidle", "walktoidle"}, nullptr},
    {"Attack", {}, "attackanim"},
    {"CutScene", {"idleattack1"}, nullptr},
    {"PostPlayAnim", {"idleattack1"}, nullptr},
    {"Hurt", {"hurt"}, nullptr},
    {"SAHurt", {"synchurt"}, nullptr},
    {"GetupLeft", {"getupleft"}, nullptr},
    {"GetupRight", {"getupright"}, nullptr},
};

const StateClips* FindState(const std::string& state) {
    for (const StateClips& s : kAnimalStates) {
        if (state == s.state) return &s;
    }
    return nullptr;
}

const char* GuardedBrainParamRaw(void* ent, char* name) {
    __try {
        return g_brain_param(ent, name);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

std::string BrainParam(void* ent, const char* name) {
    char buf[32];
    strncpy_s(buf, name, _TRUNCATE);
    const char* r = GuardedBrainParamRaw(ent, buf);
    return r ? core::safe_cstr(r, 64) : std::string();
}

// Once per model: which cAnimalLegs clips the model lacks (they fail quietly on its own part, host and client alike).
void NoteCoverage(void* ent, void* part) {
    const std::string model = ModelName(ent);
    if (model.empty() || !LogOnce("cov|" + model)) return;
    std::set<std::string> all, missing;
    for (const StateClips& s : kAnimalStates) {
        for (const char* c : s.clips) {
            if (!c) continue;
            all.insert(c);
            if (AnimIndex(part, c) < 0) missing.insert(c);
        }
    }
    std::string list;
    for (const std::string& c : missing) list += (list.empty() ? "" : " ") + c;
    spdlog::info("beasts: clips model='{}' has {}/{} cAnimalLegs clips; missing: {}", model, all.size() - missing.size(),
                 all.size(), list.empty() ? "none" : list);
}

// Fallbacks for animals.cpp NpcLegClip names, from the girl/nephew (idle, walk/forward, run/forward, turns), fiancée
// (defaultpose, cover, coveridle, hit, scare, hurtdown) and ranger (liedeadidle) packs.
std::vector<const char*> LegFallbacks(const std::string& clip) {
    if (clip == "idlenormalneutral") return {"idle", "idlenormalgestures", "coveridle", "defaultpose"};
    if (clip == "walk/backward" || clip == "walk/left" || clip == "walk/right" || clip == "walk/forwardlimp")
        return {"walk/forward"};
    if (clip == "run/forward") return {"walk/forward"};
    if (clip == "fall") return {"hurtdown"};
    if (clip == "liedead") return {"liedeadidle"};
    if (clip == "hurt") return {"hit"};
    if (clip == "coveridle") return {"cover"};
    return {};
}

// ---- coop_tranq <id> ----

// The local hunter's weapon in hand: a child item (parent id +0xA4) whose activation state +0x1f0 is 1 or 2.
void* ActiveWeapon(int16_t hunterId) {
    void* mgr = EntityManager();
    int count = 0;
    void** arr = nullptr;
    if (!mgr || !core::safe_read(static_cast<char*>(mgr) + 0x08, count) ||
        !core::safe_read(static_cast<char*>(mgr) + 0x04, arr) || !arr || count <= 0 || count > 0x400)
        return nullptr;
    for (int i = 0; i < count; ++i) {
        void* ent = nullptr;
        if (!core::safe_read(arr + i, ent) || !ent) continue;
        if (Read<int16_t>(ent, kEntParent, -1) != hunterId) continue;
        const int active = Read<int>(ent, 0x1f0, -1);
        if (active == 1 || active == 2) return ent;
    }
    return nullptr;
}

// A dart from this process's hunter into animal <id>, through the animal's OnHit (so the hooks above see it): an
// unspawned mod_obj_tranq_bullet whose shooter is the weapon in hand, hit-bone entry 0. Mouse input does not reach a
// background window; this is how two-window tests fire.
void __cdecl cmd_tranq(const char* arg) {
    const int id = std::atoi(core::safe_cstr(arg, 16).c_str());
    void* ent = id >= 0 ? animals::Find(static_cast<uint16_t>(id)) : nullptr;
    const bool cobra = IsCobra(ent);
    if (!IsDartTarget(ent) && !cobra) {
        spdlog::warn("beasts: coop_tranq <animal id>: no animal {}", id);
        return;
    }
    const LocalPlayerSample& me = LocalPlayer();
    if (!me.valid || !me.entity) {
        spdlog::warn("beasts: coop_tranq: no local hunter");
        return;
    }
    void* weapon = ActiveWeapon(me.id);
    if (!weapon) {
        spdlog::warn("beasts: coop_tranq: no weapon in hand");
        return;
    }
    if (!g_test_bullet) g_test_bullet = entity::Create("mod_obj_tranq_bullet");
    if (!g_test_bullet || !KindOf(g_test_bullet, "mod_obj_tranq_bullet")) {
        spdlog::warn("beasts: coop_tranq: could not make a mod_obj_tranq_bullet");
        return;
    }
    Write<void*>(g_test_bullet, kBulletWeapon, weapon);
    Write<void*>(g_test_bullet, kBulletHunter, me.entity);
    if (!cobra) {
        void* entries = Read<void*>(ent, kHitList + 0x10, nullptr);
        if (!entries) {
            spdlog::warn("beasts: coop_tranq id={}: the animal has no hit bones", id);
            return;
        }
        Write<int>(ent, kHitBone, 0);
    }
    float pos[3] = {};
    core::safe_copy(static_cast<char*>(ent) + kEntPos, pos, sizeof pos);
    pos[1] += 0.5f;
    float dir[3] = {pos[0] - me.pos.x, pos[1] - (me.pos.y + 1.7f), pos[2] - me.pos.z};
    const float len = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
    if (len > 0.001f) {
        for (float& d : dir) d /= len;
    }
    char* weapon_model = Read<char*>(weapon, kEntModelName, nullptr);
    spdlog::info("beasts: coop_tranq id={} weapon='{}' {:.1f} m away", id,
                 weapon_model ? core::safe_cstr(weapon_model, 120) : "", len);
    const bool ok = CallVirtualOnHit(ent, g_test_bullet, pos, dir);
    spdlog::info("beasts: coop_tranq id={} OnHit ok={} tranq={} hp={:.1f} state='{}'", id, ok,
                 Read<int>(ent, kTranqState, 0), Read<float>(ent, kEntHealth, 0.f), entity::BodyStateName(ent));
}

class BeastsModule : public dh2::Module {
public:
    const char* Name() const override { return "beasts"; }
    int Priority() const override { return 47; }

    void OnInit() override {
        const std::string ini = ini_path();
        g_tranq_clients = GetPrivateProfileIntA("Animals", "TranqFromClients", 1, ini.c_str()) != 0;
        g_overdose_clients = GetPrivateProfileIntA("Animals", "TranqOverdoseFromClients", 0, ini.c_str()) != 0;
        g_cobra_clients = GetPrivateProfileIntA("Animals", "CobraHitsFromClients", 1, ini.c_str()) != 0;
        g_clip_check = GetPrivateProfileIntA("Animals", "ClipCheck", 1, ini.c_str()) != 0;
        events::SetHandler(events::kTranqHit, &OnTranqRequest, nullptr);
        spdlog::info("beasts: tranq from clients={} overdose={} cobra hits={} clip check={}", g_tranq_clients,
                     g_overdose_clients, g_cobra_clients, g_clip_check);
    }

    void OnEngineReady(HMODULE engine) override {
        g_kind_of = core::find_export_as<KindOfFn>(engine, "?IsEntityKindOf@cObjEntity@@QBEHPBD@Z");
        g_anim_index = core::find_export_as<AnimIndexFn>(engine, "?GetAnimIndex@cModelPart@@QBEHPBD@Z");
        g_link_valid = core::find_export_as<LinkValidFn>(engine, "?IsValid@cModelPartLink@@QBEHXZ");
        g_link_part = core::find_export_as<LinkPartFn>(engine, "??BcModelPartLink@@QBEPAVcModelPart@@XZ");
        if (HMODULE common = core::wait_for_module("CommonDll7.dll", 10000))
            g_brain_param = core::find_export_as<BrainParamFn>(common, "?GetBrainParam@cObjCharacterEx@@UAEPADPAD@Z");
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_game = game;
        g_mission_fail = reinterpret_cast<MissionFailFn>(CheckedRva(kRvaMissionFail, "mission fail"));
        g_onhit_hook = core::install_hook("cScrAnimal::OnHit (0x1a680, darts)", CheckedRva(kRvaAnimalOnHit, "animal OnHit", true),
                                          reinterpret_cast<void*>(&hk_AnimalOnHit));
        g_cobra_hook = core::install_hook("cObjCobra::OnHit (0x80440)", CheckedRva(kRvaCobraOnHit, "cobra OnHit"),
                                          reinterpret_cast<void*>(&hk_CobraOnHit));
    }

    void OnMapLoaded(const char*) override {
        g_last_dart_ms.clear();
        g_tranq_by.clear();
        g_last_cobra_ms.clear();
    }

    void OnMapUnloading() override { g_test_bullet = nullptr; }  // freed with nothing; one small object per map

    void OnFrame(float) override {
        if (!g_cmd && console_cmds::Ready()) {
            g_cmd = console_cmds::AddCommandStr("coop_tranq", &cmd_tranq);
        }
    }
};

DH2_MODULE(BeastsModule)

}  // namespace

bool MirrorStateOk(void* ent, const std::string& state) {
    if (!g_clip_check || !ent || state.empty() || IsCobra(ent)) return true;
    void* part = GuardedPart0(ent);
    if (!part) return true;
    NoteCoverage(ent, part);
    const StateClips* sc = FindState(state);
    if (!sc) return true;
    void* partners[8] = {};
    if (Partners(part, partners, 8) <= 0) return true;  // nothing to forward to: a missing clip fails quietly
    std::vector<std::string> clips;
    for (const char* c : sc->clips) {
        if (c) clips.emplace_back(c);
    }
    if (sc->brain_param && g_brain_param) {
        const std::string p = BrainParam(ent, sc->brain_param);
        if (p.size() > 1) clips.push_back(p);
    }
    for (const std::string& c : clips) {
        if (AnimIndex(part, c.c_str()) < 0) continue;  // never set on this part, so never forwarded
        std::string why;
        if (Playable(part, c.c_str(), &why)) continue;
        if (LogOnce("state|" + std::to_string(IdOf(ent)) + "|" + state))
            spdlog::warn("beasts: mirror id={} not put in '{}': clip '{}' {} (a linked part raises \"Could not forward "
                         "animation\")",
                         IdOf(ent), state, c, why);
        return false;
    }
    return true;
}

bool NoCutSceneThaw(void* ent) { return HasVt(ent, kVtCrocodileRva); }

const char* NpcLegClipFor(void* ent, const char* preferred) {
    if (!preferred || !g_clip_check || !ent) return preferred;
    void* part = GuardedPart0(ent);
    if (!part || !g_anim_index) return preferred;
    std::string why;
    if (Playable(part, preferred, &why)) return preferred;
    const std::string first_why = why;
    for (const char* alt : LegFallbacks(preferred)) {
        if (!Playable(part, alt, nullptr)) continue;
        if (LogOnce("leg|" + ModelName(ent) + "|" + preferred))
            spdlog::info("beasts: npc model '{}': '{}' ({}) -> '{}'", ModelName(ent), preferred, first_why, alt);
        return alt;
    }
    if (LogOnce("leg|" + ModelName(ent) + "|" + preferred))
        spdlog::info("beasts: npc model '{}': no '{}' ({}) and no fallback; leg clip left to the body state",
                     ModelName(ent), preferred, first_why);
    return nullptr;
}

}  // namespace dh2::coop::beasts
