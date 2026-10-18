// Host: what scripts and AI call "the hunter" becomes the nearest living player (re/notes/hunter-nearest.md).
// The host runs every script and every animal/NPC brain, and each of them finds the player by the name "hunter"
// or through the local-player getter, so on the host only the host's own hunter used to count. Puppets
// (obj_humanEx bodies standing in for clients) are given a target name, "coop_p<id>", and the choke points below
// hand that name (or the puppet) to the engine instead of the host's hunter:
//   1. GetDistBetween2Ais (GameDll7 0x9200, cdecl(name, name) -> float): with "hunter" on one side it returns the
//      smallest distance over the living players. HUNTMAPBASE.SCR uses it for the spook distance, the walk-up to
//      the trophy, the predator spawn range and the predator safe distance. Story maps only use it for the stealth
//      zone (HUD and crouch flags of that process's own hunter), so by default it applies on HUNTMAP* only.
//   2. cScrCharacterEx::SetTaskParam (CommonDll7 0x1f190, the script SetTaskParam of every animal and NPC): a
//      "target"/"spooker" of "hunter" becomes the nearest living player's name. Animal attack tasks accept an
//      entity only when it is in the attack-target list ([Puppets] AttackTarget=1); the hurt/knock forwarding and
//      the "are you the player" answers in combat.cpp then apply. Cobras and other classes are left alone.
//   3. Perception: animals sense the player through cAIHuntAnimalInterface's stimulus list (the hunter adds a
//      presence stimulus +0x520 whose intensity is the stealth level, and a gunshot noise +0x68c). Each living
//      puppet gets copies, at its own position, with the stealth level of that player's posture, and a gunshot
//      stimulus when its shot counter moves.
//   4. NPC follow / guide waits: npc_follow.cpp (same nearest pick, per brain tick).
// Everything else that means "the player" stays the host's: camera, HUD, profile, inventory, mission, cutscenes,
// script ::globals.player (FindEntScrObject). Trigger volumes are tested per process with that process's hunter
// and a client's own fire becomes a request (story.cpp), so they already count every player.
#include "coop/hunter_nearest.h"

#include <windows.h>

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
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
#include "engine/console_cmds.h"

namespace dh2::coop::hunter_nearest {
namespace {

constexpr uintptr_t kRvaGetDist = 0x9200;      // GameDll7 cMODApp::GetDistBetween2Ais
constexpr uintptr_t kRvaNotFound = 0xfc64c;    // GameDll7 float GetDistBetween2Ais returns for an unknown name
constexpr uintptr_t kRvaAnimalVt = 0xff470;    // GameDll7 cScrAnimal
constexpr uintptr_t kScrEntity = 0x28;         // script object -> C++ entity
constexpr uintptr_t kEntPos = 0xE8;
constexpr uintptr_t kEntName = 0x198;          // cObjBaseEntity target name
constexpr uintptr_t kHunterDead = 0x6CC;       // cObjHunter::OnDie
constexpr uintptr_t kHunterPresence = 0x520;   // tAIHuntStimulus*: flags 0x644, intensity = GetStealthLevel()
constexpr uintptr_t kHunterShot = 0x68c;       // tAIHuntStimulus*: flags 0x612, armed per shot (GameDll 0x38850)
constexpr uintptr_t kHunterStealth = 0x4e8;    // float[4] SetStealthLevels(crouch, stand, walk, run)
constexpr size_t kStimSize = 0x30;             // the hunter mallocs 0x30 per stimulus (cObjHunter::Spawn 0x4ea30)
constexpr uintptr_t kStimPos = 0x18, kStimIntensity = 0x24, kStimTtl = 0x28, kStimSource = 0x2c;

bool g_enabled = false;  // [Story] HunterNearest
bool g_dist = true;      // [Story] HunterNearestDistance
bool g_tasks = true;     // [Story] HunterNearestTasks
bool g_senses = true;    // [Story] HunterNearestSenses
std::vector<std::string> g_dist_maps = {"HUNTMAP"};  // [Story] HunterNearestDistanceMaps (prefixes, * = all)

uintptr_t g_game = 0;
float g_not_found = 0.f;
safetyhook::InlineHook* g_dist_hook = nullptr;
safetyhook::InlineHook* g_task_hook = nullptr;

using SetNameFn = void(__thiscall*)(void* ent, const char* name);
using StimFn = void(__thiscall*)(void* iface, void* stim);
using CountFn = int(__thiscall*)(void* iface);
using AtFn = void*(__thiscall*)(void* iface, int i);
using InListFn = int(__thiscall*)(void* iface, void* ent);
SetNameFn g_set_name = nullptr;
StimFn g_add_stim = nullptr;
StimFn g_remove_stim = nullptr;
CountFn g_stim_count = nullptr;
AtFn g_stim_at = nullptr;
InListFn g_in_list = nullptr;
void** g_iface = nullptr;  // CommonDll7 0x1aea7c, the cAIHuntAnimalInterface singleton

char g_names[8][16] = {};
uint32_t g_dist_logs = 0;
uint32_t g_task_logs = 0;
bool g_cmd_registered = false;

bool Active() { return g_enabled && NetIsHost() && NetIsOnline(); }

bool HostDead(void* hunter) {
    int dead = 0;
    return !hunter || (core::safe_read(static_cast<char*>(hunter) + kHunterDead, dead) && dead != 0);
}

bool IsHunterName(const char* s) { return s && _stricmp(s, "hunter") == 0; }

bool MapAllowed() {
    const std::string& map = events::CurrentMap();
    for (const std::string& p : g_dist_maps) {
        if (p == "*") return true;
        if (!p.empty() && map.size() >= p.size() && _strnicmp(map.c_str(), p.c_str(), p.size()) == 0) return true;
    }
    return false;
}

bool HasName(void* ent, const char* name) {
    char* n = nullptr;
    return ent && core::safe_read(static_cast<char*>(ent) + kEntName, n) && n &&
           core::safe_cstr(n, 32) == name;
}

// ---- guarded engine calls ----
template <class Fn, class... A>
bool guarded_void(Fn fn, A... a) {
    __try {
        fn(a...);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int guarded_count(void* iface) {
    __try {
        return g_stim_count(iface);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

void* guarded_at(void* iface, int i) {
    __try {
        return g_stim_at(iface, i);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

int guarded_in_list(void* iface, void* ent) {
    __try {
        return g_in_list(iface, ent);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

void* Iface() { return g_iface ? *g_iface : nullptr; }

bool InAttackList(void* ent) {
    void* iface = Iface();
    return iface && g_in_list && guarded_in_list(iface, ent) == 1;
}

// ---- 1. GetDistBetween2Ais ----
float __cdecl hk_GetDist(char* a, char* b) {
    const float d = g_dist_hook->unsafe_ccall<float>(a, b);
    if (!g_dist || !Active() || !a || !b) return d;
    const bool ha = IsHunterName(a), hb = IsHunterName(b);
    if (ha == hb || !MapAllowed()) return d;
    char* other = ha ? b : a;
    float best = (d == g_not_found || HostDead(LocalPlayer().entity)) ? FLT_MAX : d;
    int who = -1;
    puppets::SenseInfo list[4];
    const int n = puppets::List(list, 4);
    for (int i = 0; i < n; ++i) {
        if (list[i].dead) continue;
        char* nm = const_cast<char*>(PuppetName(list[i].id));
        const float pd = ha ? g_dist_hook->unsafe_ccall<float>(nm, other) : g_dist_hook->unsafe_ccall<float>(other, nm);
        if (pd != g_not_found && pd < best) {
            best = pd;
            who = list[i].id;
        }
    }
    if (best == FLT_MAX) return d;
    if (who >= 0 && (g_dist_logs++ < 20 || g_dist_logs % 500 == 0))
        spdlog::info("hunter_nearest: distance hunter<->'{}' = {:.1f} m from player {} (host {:.1f} m)",
                     core::safe_cstr(other, 32), best, who, d);
    return best;
}

// ---- 2. SetTaskParam("target"/"spooker", "hunter") ----
bool IsNpc(void* ent) {
    for (void* n : Npcs()) {
        if (n == ent) return !puppets::Owns(ent);
    }
    return false;
}

const char* Substitute(void* scr, const char* key) {
    void* ent = nullptr;
    if (!core::safe_read(static_cast<char*>(scr) + kScrEntity, ent) || !ent) return nullptr;
    uintptr_t vt = 0;
    core::safe_read(ent, vt);
    const bool animal = vt == g_game + kRvaAnimalVt;
    const bool target = std::strcmp(key, "target") == 0;
    if (!animal && !(target && IsNpc(ent))) return nullptr;  // cobras, props, the hunter's own tasks
    float pos[3] = {};
    if (!core::safe_copy(static_cast<char*>(ent) + kEntPos, pos, sizeof pos)) return nullptr;
    float d2 = 0.f;
    void* pick = NearestLivingPlayer(pos, &d2);
    const uint8_t who = puppets::PlayerOf(pick);
    if (who == net::kInvalidPlayerId) return nullptr;  // the host is nearest: unchanged
    // StartAttackTask only accepts an entity that is in the attack-target list. A script that made the player
    // unattackable (SetCanBeAttacked(0), GameDll 0x49ec0, takes only the host hunter off the list) meant "no
    // attack": keep that, rather than sending the animal at a client.
    if (animal && target && (!InAttackList(pick) || !InAttackList(LocalPlayer().entity))) return nullptr;
    if (!animal) npc_follow::NoteTarget(ent, pick);
    int16_t id = -1;
    core::safe_read(static_cast<char*>(ent) + 0x04, id);
    if (g_task_logs++ < 40 || g_task_logs % 200 == 0)
        spdlog::info("hunter_nearest: {} id {} task '{}' = player {} ({:.1f} m) instead of the host",
                     animal ? "animal" : "NPC", id, key, who, std::sqrt(d2));
    return PuppetName(who);
}

void __fastcall hk_SetTaskParam(void* scr, void*, char* key, char* value) {
    if (g_tasks && Active() && scr && key && IsHunterName(value) &&
        (std::strcmp(key, "target") == 0 || std::strcmp(key, "spooker") == 0)) {
        if (const char* sub = Substitute(scr, key)) {
            g_task_hook->unsafe_thiscall<void>(scr, key, const_cast<char*>(sub));
            return;
        }
    }
    g_task_hook->unsafe_thiscall<void>(scr, key, value);
}

// ---- 3. stimuli ----
struct Stim {
    alignas(16) uint8_t presence[kStimSize] = {};
    alignas(16) uint8_t shot[kStimSize] = {};
    bool presence_in = false;
    bool shot_armed = false;
    int last_shots = -1;
    uint32_t check_ms = 0;
};
std::unordered_map<void*, std::unique_ptr<Stim>> g_stims;  // puppet entity -> its stimuli

bool StimuliBound() { return g_add_stim && g_remove_stim && g_stim_count && g_stim_at && Iface(); }

bool StimInList(void* stim) {
    void* iface = Iface();
    const int n = iface ? guarded_count(iface) : -1;
    for (int i = 0; i < n; ++i) {
        if (guarded_at(iface, i) == stim) return true;
    }
    return false;
}

void AddStim(void* stim) {
    if (void* iface = Iface()) guarded_void(g_add_stim, iface, stim);
}

void RemoveStim(void* stim) {
    if (void* iface = Iface()) guarded_void(g_remove_stim, iface, stim);
}

void DropStim(void* ent) {
    const auto it = g_stims.find(ent);
    if (it == g_stims.end()) return;
    if (StimuliBound()) {
        RemoveStim(it->second->presence);
        RemoveStim(it->second->shot);
    }
    g_stims.erase(it);
}

void DropAllStims() {
    while (!g_stims.empty()) DropStim(g_stims.begin()->first);
}

void PumpSenses() {
    if (!g_senses || !Active() || !StimuliBound()) {
        if (!g_stims.empty()) DropAllStims();
        return;
    }
    char* hunter = static_cast<char*>(LocalPlayer().entity);
    char* hp = nullptr;
    char* hs = nullptr;
    if (!hunter || !core::safe_read(hunter + kHunterPresence, hp) || !hp) return;
    core::safe_read(hunter + kHunterShot, hs);
    uint8_t host_presence[kStimSize];
    float table[4] = {};
    if (!core::safe_copy(hp, host_presence, sizeof host_presence) ||
        !core::safe_copy(hunter + kHunterStealth, table, sizeof table))
        return;
    float host_level = 0.f;
    std::memcpy(&host_level, host_presence + kStimIntensity, sizeof host_level);
    // cHUDStealth::Update writes one of the four stealth levels into the presence intensity. Without the stealth
    // HUD (free hunt) the intensity keeps its spawn value: then every player gets the host's value.
    bool stealth_live = false;
    for (float t : table) stealth_live |= t == host_level;
    puppets::SenseInfo list[4];
    const int n = puppets::List(list, 4);
    const uint32_t now = GetTickCount();
    for (auto it = g_stims.begin(); it != g_stims.end();) {
        bool listed = false;
        for (int i = 0; i < n; ++i) listed |= list[i].entity == it->first;
        if (listed) {
            ++it;
            continue;
        }
        RemoveStim(it->second->presence);
        RemoveStim(it->second->shot);
        it = g_stims.erase(it);
    }
    for (int i = 0; i < n; ++i) {
        const puppets::SenseInfo& p = list[i];
        std::unique_ptr<Stim>& slot = g_stims[p.entity];
        if (!slot) slot = std::make_unique<Stim>();
        Stim& s = *slot;
        if (p.dead) {
            if (s.presence_in) RemoveStim(s.presence);
            RemoveStim(s.shot);
            s.presence_in = false;
            s.shot_armed = false;
            s.last_shots = p.shots;
            continue;
        }
        float pos[3] = {};
        if (!core::safe_copy(static_cast<char*>(p.entity) + kEntPos, pos, sizeof pos)) continue;
        // Presence: the host's stimulus, moved to this player and given this player's stealth level.
        std::memcpy(s.presence, host_presence, kStimSize);
        std::memcpy(s.presence + kStimPos, pos, sizeof pos);
        const float level = stealth_live ? table[p.posture < 0 ? 0 : p.posture > 3 ? 3 : p.posture] : host_level;
        std::memcpy(s.presence + kStimIntensity, &level, sizeof level);
        void* src = p.entity;
        std::memcpy(s.presence + kStimSource, &src, sizeof src);
        if (!s.presence_in || now - s.check_ms > 1000) {
            s.check_ms = now;
            if (!StimInList(s.presence)) {
                AddStim(s.presence);
                if (!s.presence_in)
                    spdlog::info("hunter_nearest: animals can sense player {} (presence stimulus, level {:.2f}{})", p.id,
                                 level, stealth_live ? "" : ", host value");
            }
            s.presence_in = true;
        }
        // Gunshot noise: the hunter's shot re-arms its stimulus for 1.4 s at the muzzle (GameDll 0x38850).
        if (s.last_shots >= 0 && p.shots != s.last_shots && hs) {
            if (core::safe_copy(hs, s.shot, kStimSize)) {
                const float muzzle[3] = {pos[0], pos[1] + 1.5f, pos[2]};
                const float one = 1.f, ttl = 1.4f;
                std::memcpy(s.shot + kStimPos, muzzle, sizeof muzzle);
                std::memcpy(s.shot + kStimIntensity, &one, sizeof one);
                std::memcpy(s.shot + kStimTtl, &ttl, sizeof ttl);
                std::memcpy(s.shot + kStimSource, &src, sizeof src);
                if (!StimInList(s.shot)) AddStim(s.shot);
                if (!s.shot_armed) spdlog::info("hunter_nearest: animals can hear player {}'s shots", p.id);
                s.shot_armed = true;
            }
        }
        s.last_shots = p.shots;
    }
}

// Test: coop_hdist <name> logs GetDistBetween2Ais("hunter", name) through the hook (nearest player) and raw.
void __cdecl cmd_hdist(const char* arg) {
    std::string name = core::safe_cstr(arg, 64);
    if (name.empty() || !g_dist_hook) {
        spdlog::warn("coop_hdist: usage coop_hdist <entity name>");
        return;
    }
    char hunter[] = "hunter";
    const float raw = g_dist_hook->unsafe_ccall<float>(hunter, name.data());
    const float nearest = hk_GetDist(hunter, name.data());
    spdlog::info("coop_hdist: '{}' host {:.2f} m, nearest player {:.2f} m (map allowed={}, active={})", name, raw,
                 nearest, MapAllowed(), Active());
}

void* CheckedGameRva(HMODULE game, uintptr_t rva, const char* label) {
    static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
    uint8_t got[5] = {};
    void* p = reinterpret_cast<char*>(game) + rva;
    if (!core::safe_copy(p, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
        spdlog::error("hunter_nearest: {} at GameDll7+{:#x} has unexpected bytes; not hooked", label, rva);
        return nullptr;
    }
    return p;
}

class HunterNearestModule : public dh2::Module {
public:
    const char* Name() const override { return "hunter_nearest"; }
    int Priority() const override { return 49; }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        g_enabled = GetPrivateProfileIntA("Story", "HunterNearest", 1, ini.c_str()) != 0;
        g_dist = GetPrivateProfileIntA("Story", "HunterNearestDistance", 1, ini.c_str()) != 0;
        g_tasks = GetPrivateProfileIntA("Story", "HunterNearestTasks", 1, ini.c_str()) != 0;
        g_senses = GetPrivateProfileIntA("Story", "HunterNearestSenses", 1, ini.c_str()) != 0;
        char maps[256] = {};
        GetPrivateProfileStringA("Story", "HunterNearestDistanceMaps", "HUNTMAP", maps, sizeof maps, ini.c_str());
        g_dist_maps.clear();
        std::string cur;
        for (const char* c = maps;; ++c) {
            if (*c == ',' || *c == ' ' || *c == '\0') {
                if (!cur.empty()) g_dist_maps.push_back(cur);
                cur.clear();
                if (*c == '\0') break;
            } else {
                cur.push_back(*c);
            }
        }
        for (int i = 0; i < 8; ++i) std::snprintf(g_names[i], sizeof g_names[i], "coop_p%d", i);
        spdlog::info("hunter_nearest: enabled={} distance={} (maps '{}') tasks={} senses={}", g_enabled, g_dist, maps,
                     g_tasks, g_senses);
    }

    void OnEngineReady(HMODULE engine) override {
        (void)engine;
        if (!g_enabled) return;
        HMODULE common = core::wait_for_module("CommonDll7.dll", 10000);
        if (!common) return;
        g_set_name = reinterpret_cast<SetNameFn>(core::find_export(common, "?SetTargetName@cObjBaseEntity@@UAEXPBD@Z"));
        g_add_stim = reinterpret_cast<StimFn>(
            core::find_export(common, "?AddStimulus@cAIHuntAnimalInterface@@QAEXPAUtAIHuntStimulus@@@Z"));
        g_remove_stim = reinterpret_cast<StimFn>(
            core::find_export(common, "?RemoveStimulus@cAIHuntAnimalInterface@@QAEXPAUtAIHuntStimulus@@@Z"));
        g_stim_count =
            reinterpret_cast<CountFn>(core::find_export(common, "?GetStimuliCount@cAIHuntAnimalInterface@@QAEHXZ"));
        g_stim_at = reinterpret_cast<AtFn>(
            core::find_export(common, "?GetStimulusAt@cAIHuntAnimalInterface@@QAEPAUtAIHuntStimulus@@H@Z"));
        g_in_list = reinterpret_cast<InListFn>(
            core::find_export(common, "?IsAttackTargetInList@cAIHuntAnimalInterface@@QAEHPAVcObjEntity@@@Z"));
        g_iface = reinterpret_cast<void**>(reinterpret_cast<char*>(common) + 0x1aea7c);
        g_task_hook = core::install_hook("cScrCharacterEx::SetTaskParam (hunter_nearest)",
                                         core::find_export(common, "?SetTaskParam@cScrCharacterEx@@UAEXPAD0@Z"),
                                         reinterpret_cast<void*>(&hk_SetTaskParam));
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_game = reinterpret_cast<uintptr_t>(game);
        if (!g_enabled) return;
        core::safe_read(reinterpret_cast<void*>(g_game + kRvaNotFound), g_not_found);
        g_dist_hook = core::install_hook("GetDistBetween2Ais (hunter_nearest, 0x9200)",
                                         CheckedGameRva(game, kRvaGetDist, "GetDistBetween2Ais"),
                                         reinterpret_cast<void*>(&hk_GetDist));
        spdlog::info("hunter_nearest: unknown-name distance is {:.1f}", g_not_found);
    }

    void OnMapUnloading() override { DropAllStims(); }

    void OnFrame(float) override {
        if (!g_enabled) return;
        if (!g_cmd_registered && console_cmds::Ready()) g_cmd_registered = console_cmds::AddCommandStr("coop_hdist", &cmd_hdist);
        PumpSenses();
    }
};

DH2_MODULE(HunterNearestModule)

}  // namespace

bool Enabled() { return g_enabled; }

const char* PuppetName(uint8_t playerId) { return g_names[playerId & 7]; }

void NamePuppet(void* ent, uint8_t playerId) {
    if (!g_enabled || !ent || !g_set_name) return;
    const bool ok = guarded_void(g_set_name, ent, static_cast<const char*>(PuppetName(playerId)));
    spdlog::info("hunter_nearest: puppet {:#x} of player {} named '{}' ok={} check={}", reinterpret_cast<uintptr_t>(ent),
                 playerId, PuppetName(playerId), ok, HasName(ent, PuppetName(playerId)));
}

void* NearestLivingPlayer(const float pos[3], float* dist_sq) {
    void* hunter = LocalPlayer().entity;
    float best = FLT_MAX;
    void* pick = nullptr;
    float hp[3] = {};
    if (hunter && !HostDead(hunter) && core::safe_copy(static_cast<char*>(hunter) + kEntPos, hp, sizeof hp)) {
        const float dx = hp[0] - pos[0], dy = hp[1] - pos[1], dz = hp[2] - pos[2];
        best = dx * dx + dy * dy + dz * dz;
        pick = hunter;
    }
    if (void* p = puppets::NearestLivingPuppet(pos, best)) pick = p;
    if (dist_sq) *dist_sq = best;
    return pick;
}

void OnPuppetRemoving(void* ent) {
    DropStim(ent);
    npc_follow::Forget(ent);
}

}  // namespace dh2::coop::hunter_nearest
