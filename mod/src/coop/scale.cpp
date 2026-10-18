// Player-count difficulty scaling. Spec: re/notes/player-scale.md. What was built and verified: re/notes/scale.md.
// extra = connected players - 1, counted on the host. Clients never decide: they get the max health by commit
// (events::kScaleMax) and see reserve animals through the normal snapshot mirror.
//
// Engine facts used (GameDll7 / CommonDll7 v1.0 build 434):
//  - Every story animal comes from AddAnimal (0x8d970) inside the map script's AddAnimalOnMap, which GameDll runs
//    from 0x8cab0. AddAnimal stores the species at entity +0x434, spawns (+0x4C0 max := +0x19C, then parks at
//    -10000 with the brain frozen) and names the entity. Calling it again right after AddAnimalOnMap, in the same
//    order on every peer, gives the reserves the same entity ids everywhere.
//  - Script MoveToPos = entity vt +0x158(3,0) (0x14610: +0x19C := +0x4C0, IdleAttack, active, brain thawed), then
//    vt +0xA4(position), then the angle. A scaled +0x4C0 therefore survives every later MoveToPos by itself.
//  - SetHealth (0x15ff0) writes only +0x19C (argument * retail difficulty factor 1.0/1.1/1.2).
//  - The trigger state machine runs BeginCutScene first and OnActivate (fire 0x68130) after the cutscene, so an
//    attack wave is the set of AIHuntTask_Attack* StartTask calls of one frame, evaluated after the frame. Whether an
//    animal is still on that attack is the brain's IsTaskActive (vt +0x40), not the script calls.
//  - A reserve copies its template's CustomProps (0x16300; MakeChaser switches lions to the chaser brain, without it
//    AttackChasers fails), StartTask and SetTaskParam calls. Script Kill() (0x16a40) on the last animal of a family
//    parks its reserves too.
//  - Present-death (0x1e8b0) reports the species to the mission through 0x91e80. animals.cpp owns both hooks and keeps a
//    reserve's death out of the mission (IsReserve below). A dead reserve that is released again is a new life there
//    (kAnimalRevive), so clients see it come back (ReuseKilled=1, verified on the MAP2B siege).
#include "coop/scale.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <utility>
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

namespace dh2::coop::scale {
namespace {

// GameDll7 RVAs. Each body starts 55 8b ec 6a ff (SEH frame); checked before hooking.
constexpr uintptr_t kRvaAddAnimalOnMap = 0x8cab0;  // fastcall(animal manager): runs the script's AddAnimalOnMap
constexpr uintptr_t kRvaAddAnimal = 0x8d970;       // cdecl(int species, int, const char* name) -> list index
constexpr uintptr_t kRvaMakeBoss = 0x16d20;        // thiscall(script object, int on, const char* hud name)
constexpr uintptr_t kRvaSetHealth = 0x15ff0;       // thiscall(script object, float)
constexpr uintptr_t kRvaKill = 0x16a40;            // fastcall(script object): script Kill() = entity vt +0x234 (park)
constexpr uintptr_t kRvaCustomProps = 0x16300;     // thiscall(script object, const char* method): runs that script method
constexpr uintptr_t kRvaAnimalVt = 0xff470;        // cScrAnimal (obj_animal)

constexpr uintptr_t kScrEntity = 0x28;  // script object -> entity
constexpr uintptr_t kEntId = 0x04;
constexpr uintptr_t kEntPos = 0xE8;
constexpr uintptr_t kEntAngle = 0xF4;
constexpr uintptr_t kEntName = 0x198;
constexpr uintptr_t kEntHealth = 0x19C;
constexpr uintptr_t kEntPhys = 0x118;
constexpr uintptr_t kEntSpecies = 0x434;
constexpr uintptr_t kEntMaxHealth = 0x4C0;
constexpr uintptr_t kEntKillAction = 0x420;  // MISSION_ACTION_* the death reports (bullet OnHit writes 1)
constexpr uintptr_t kEntDead = 0x600;  // set by present-death, cleared by vt +0x158
constexpr uintptr_t kEntFleeReached = 0x590;  // 1 = reached a flee point (GameDll 0x14940); Update parks and reports
constexpr uintptr_t kVtRespawn = 0x158;  // thiscall(ent, 3, 0): what script MoveToPos calls
constexpr uintptr_t kVtSetPos = 0xA4;    // thiscall(ent, const tVect3*): what script MoveToPos calls next
constexpr uintptr_t kVtPark = 0x234;     // thiscall(ent): script Kill(), -10000 and brain frozen
// Script object vtable slots behind the script natives (CommonDll thunks 0xe000/0xe0c0).
constexpr uintptr_t kScrVtStartTask = 0x15c;
constexpr uintptr_t kScrVtSetTaskParam = 0x16c;
constexpr uintptr_t kEntBrain = 0x200;
constexpr uintptr_t kBrainVtTaskActive = 0x40;  // cAIHuntBrainEx::IsTaskActive(name) -> task or null (StartTask uses it)

constexpr float kParkedY = -1000.f;
constexpr float kHealthPerPlayer = 0.25f;
constexpr int kMaxReserves = 3;  // the 4-player maximum of extra bodies
constexpr size_t kMaxTemplateParams = 16;

// ANIMAL_* constants (SCRIPTER.EXP order, 0-based).
constexpr int kSpeciesCount = 37;
const char* const kSpeciesName[kSpeciesCount] = {
    "GWOLF",  "BBEAR",     "COUGAR",   "GBEAR",    "HYENA",     "LEOPARD",   "MCARIBOU",  "MOUNTAINLION",
    "PBEAR",  "RHINO",     "WILDBOAR", "ZEBRA",    "BANTENG",   "CAMEL",     "CAPEBUFFALO", "DINGO",
    "HIPO",   "SCRUBBULL", "WILDDOG",  "COBRA",    "ABEAR",     "BTIGER",    "PUMA",      "ELEPHANT",
    "CROCODILE", "YETI",   "BIGGRIM",  "LION",     "LIONESSE",  "HOGZILLA",  "WHITETAIL", "WATERCROCODILE",
    "PANTELOPE", "WILDBOAR3", "REDDEER", "AXIS",   "IMPALA"};
constexpr int kLion = 27;
constexpr int kLionesse = 28;

const char* SpeciesName(int s) { return s >= 0 && s < kSpeciesCount ? kSpeciesName[s] : "?"; }

// One large animal starting an attack alone keeps one body and gains health (spec section 1).
bool IsLarge(int s) {
    switch (s) {
        case 1: case 2: case 3: case 7: case 8: case 9: case 16: case 17: case 20: case 21: case 22: case 23:
        case kLion: case kLionesse:
            return true;
        default:
            return false;
    }
}
bool IsBoss(int s) { return s == 25 || s == 26 || s == 29; }  // yeti, big grim, hogzilla: MakeBoss only
bool IsCroc(int s) { return s == 24 || s == 31; }               // not mirrored like the others: never extra bodies
// MAP4A/MAP4B start a lion and a lioness together: one pack of two, not two lone animals.
int FamilyOf(int s) { return s == kLionesse ? kLion : s; }

// Species that get the reserve pool, per map, from every AIHuntTask_Attack* StartTask in the trigger, cutscene and
// world scripts (re/notes/scale.md, "Pool table"): pack species, and a large species only where one script
// function starts two or more of it. Hunt maps, MAP1, MAP12B/C and MAPTSAVO get none.
struct MapPool {
    const char* map;
    int species[4];
};
const MapPool kPools[] = {
    {"MAP2A", {0, -1}},          {"MAP2B", {0, -1}},       {"MAP2C", {0, -1}},       {"MAP3A", {4, 5, -1}},
    {"MAP3B", {5, -1}},          {"MAP3C", {kLionesse, -1}}, {"MAP4A", {kLion, kLionesse, -1}},
    {"MAP4B", {kLion, kLionesse, -1}}, {"MAP5", {4, 23, -1}}, {"MAP6", {21, -1}},   {"MAP7", {21, -1}},
    {"MAP8", {17, -1}},          {"MAP9", {15, -1}},       {"MAP10", {22, 33, -1}},  {"MAP11A", {10, -1}},
    {"MAP11B", {10, -1}},        {"MAP11C", {10, -1}},     {"MAP12A", {0, -1}},      {"MAPARENA", {0, 5, 17, kLion}},
    {"MAPISLAND", {21, -1}},
};

struct Settings {
    bool health = true;   // [Scale] Health
    bool bodies = true;   // [Scale] Bodies
    int reserves = kMaxReserves;  // [Scale] Reserves (per species; every player must use the same value)
    bool log = true;      // [Scale] Log
    bool client_respawn = true;  // [Scale] ClientRespawn: a client gives a released reserve the MoveToPos body state
    bool reuse_killed = true;    // [Scale] ReuseKilled: release a reserve again after it died (a new life, kAnimalRevive;
                                 // verified on the MAP2B siege, re/notes/life.md)
};
Settings g_cfg;

// Bodies and Reserves decide which entities exist (reserve ids, and every entity created after them). A client uses the
// host's values (events::kScaleConfig, sent to each client when it joins) for every map it loads after that.
struct HostConfig {
    bool have = false;
    bool bodies = true;
    int reserves = kMaxReserves;
};
HostConfig g_host_cfg;
std::vector<uint8_t> g_cfg_sent;  // host: clients that have our [Scale] Bodies/Reserves
bool g_built = false;             // this map's reserve pool was built with g_built_bodies / g_built_reserves
bool g_built_bodies = false;
int g_built_reserves = 0;

bool UseBodies() { return g_host_cfg.have && NetIsOnline() && !NetIsHost() ? g_host_cfg.bodies : g_cfg.bodies; }
int UseReserves() {
    return g_host_cfg.have && NetIsOnline() && !NetIsHost() ? g_host_cfg.reserves : g_cfg.reserves;
}

struct Template {
    std::string task;  // the last AIHuntTask_Attack* this animal was given
    std::vector<std::pair<std::string, std::string>> params;  // SetTaskParam calls after that StartTask
    std::vector<std::string> props;  // CustomProps calls on this animal (MakeChaser sets the brain an attack needs)
};

struct ScriptAnimal {
    void* ent = nullptr;
    int16_t id = -1;
    int species = -1;
    std::string name;
    bool params_open = false; // SetTaskParam calls still belong to the attack task (no other StartTask since)
    bool attacking_prev = false;  // host: up and attacking at the previous evaluation
    bool fresh = false;       // host: started an attack this frame without already being in one (not a retarget)
    bool lone_large = false;  // on the health path: does not count toward its family's pack
    float script_hp = -1.f;   // health the script set with SetHealth since its last MoveToPos (-1: none, species max)
    Template tmpl;
};

struct Reserve {
    void* ent = nullptr;
    int16_t id = -1;
    int species = -1;
    const char* name = "";
    bool released = false;  // host: unparked at least once
    bool killed = false;    // died at least once (a client keeps it dead: gap G4)
    bool seen_up = false;   // client: the mirror moved it off the park position
    Vec3 last_pos{};        // client: position at the previous frame (the mirror lerps out of the park spot)
};

// Health path. base = the unscaled max (+0x4C0 at the first mark); factor = what +0x4C0 holds now relative to it.
struct Mark {
    float base = 0.f;
    float factor = 1.f;
};

struct Family {
    bool epoch = false;  // host: a scripted attack of this family is running (reserves are topped up as it begins and
                         // when a later trigger sends in fresh attackers)
    size_t rr = 0;       // round-robin over the attackers used as templates
};

HMODULE g_game = nullptr;
uintptr_t g_vt_animal = 0;
safetyhook::InlineHook* g_onmap_hook = nullptr;
safetyhook::InlineHook* g_add_hook = nullptr;
safetyhook::InlineHook* g_boss_hook = nullptr;
safetyhook::InlineHook* g_sethealth_hook = nullptr;
safetyhook::InlineHook* g_start_hook = nullptr;
safetyhook::InlineHook* g_param_hook = nullptr;
safetyhook::InlineHook* g_props_hook = nullptr;
safetyhook::InlineHook* g_kill_hook = nullptr;
void* g_fn_start = nullptr;  // cScrCharacterEx::StartTask (export; called for reserves so other hooks see it too)
void* g_fn_param = nullptr;  // cScrCharacterEx::SetTaskParam
void* g_fn_props = nullptr;  // GameDll 0x16300 CustomProps
using GetScrFn = void*(__thiscall*)(void* ent);
GetScrFn g_get_scr = nullptr;  // cObjScriptedEntity::GetScrObject

std::string g_map;
bool g_map_ok = false;
bool g_creating = false;
bool g_replaying = false;
uint32_t g_mark_serial = 0;
bool g_vt_checked = false;
std::vector<ScriptAnimal> g_script;
std::vector<Reserve> g_reserves;
std::map<void*, Mark> g_marks;
std::vector<void*> g_batch;  // script animals that started an attack since the last evaluation
std::map<int, Family> g_families;
std::vector<int> g_script_kills;  // families whose script animals the script parked (Kill()) since the last evaluation
std::deque<std::string> g_arena;  // strings handed to the engine for reserves; kept until the map unloads
char g_names[kSpeciesCount][kMaxReserves][24] = {};  // reserve target names (static: the entity may keep the pointer)

std::string ini_path() { return (core::self_dir() / "dh2coop.ini").string(); }

bool Authoritative() { return !NetIsOnline() || NetIsHost(); }

int Extra() {
    if (!NetIsHost()) return 0;
    return std::max(0, ConnectedPlayers() - 1);
}

float FactorFor(int extra) { return 1.f + kHealthPerPlayer * static_cast<float>(extra); }

void* Rva(uintptr_t rva) { return g_game ? reinterpret_cast<char*>(g_game) + rva : nullptr; }

void* CheckedRva(uintptr_t rva, const char* label) {
    static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
    void* p = Rva(rva);
    uint8_t got[5] = {};
    if (!p || !core::safe_copy(p, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
        spdlog::error("scale: {} at GameDll7+{:#x} has unexpected bytes; not hooked (wrong game build?)", label, rva);
        return nullptr;
    }
    return p;
}

template <class T>
T Read(void* base, uintptr_t off, T fallback = T{}) {
    T v = fallback;
    if (!base || !core::safe_read(static_cast<char*>(base) + off, v)) return fallback;
    return v;
}

template <class T>
void Write(void* base, uintptr_t off, T v) {
    if (base) std::memcpy(static_cast<char*>(base) + off, &v, sizeof v);
}

void* EntityOfScr(void* scr) { return Read<void*>(scr, kScrEntity, nullptr); }
float Health(void* ent) { return Read<float>(ent, kEntHealth, 0.f); }
Vec3 Pos(void* ent) { return Read<Vec3>(ent, kEntPos, Vec3{0.f, -10000.f, 0.f}); }
bool Alive(void* ent) { return Health(ent) > 0.f && Read<int>(ent, kEntDead, 1) == 0; }
bool Parked(void* ent) { return Pos(ent).y < kParkedY; }
bool IsAnimal(void* ent) { return ent && g_vt_animal && Read<uintptr_t>(ent, 0, 0) == g_vt_animal; }

std::string NameOf(void* ent) {
    char* n = Read<char*>(ent, kEntName, nullptr);
    return n ? core::safe_cstr(n, 48) : std::string();
}

std::string PhysWords(void* ent) {
    char* phys = Read<char*>(ent, kEntPhys, nullptr);
    if (!phys) return "phys=none";
    return fmt::format("coll={:#x}/{:#x}/{:#x}", Read<uint32_t>(phys, 0x20, 0), Read<uint32_t>(phys, 0x24, 0),
                       Read<uint32_t>(phys, 0x28, 0));
}

void* VtSlot(void* obj, uintptr_t off) {
    void** vt = Read<void**>(obj, 0, nullptr);
    return vt ? Read<void*>(vt, off, nullptr) : nullptr;
}

// GameDll reaches CommonDll methods through import thunks (FF 25 = jmp [IAT]); the address behind one.
void* ThroughThunk(void* fn) {
    uint8_t op[2] = {};
    void** slot = nullptr;
    void* target = nullptr;
    if (!fn || !core::safe_copy(fn, op, 2) || op[0] != 0xFF || op[1] != 0x25) return fn;
    if (!core::safe_read(static_cast<char*>(fn) + 2, slot) || !core::safe_read(slot, target)) return fn;
    return target;
}

// ---- guarded engine calls (no C++ objects inside __try) ----

bool CallRespawn(void* ent) {
    void* fn = VtSlot(ent, kVtRespawn);
    if (!fn) return false;
    __try {
        reinterpret_cast<void(__thiscall*)(void*, int, int)>(fn)(ent, 3, 0);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CallSetPos(void* ent, const float* pos) {
    void* fn = VtSlot(ent, kVtSetPos);
    if (!fn) return false;
    __try {
        reinterpret_cast<void(__thiscall*)(void*, const float*)>(fn)(ent, pos);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CallPark(void* ent) {
    void* fn = VtSlot(ent, kVtPark);
    if (!fn) return false;
    __try {
        reinterpret_cast<void(__thiscall*)(void*)>(fn)(ent);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void* CallGetScr(void* ent) {
    if (!g_get_scr || !ent) return nullptr;
    __try {
        return g_get_scr(ent);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool CallStartTask(void* scr, const char* task) {
    if (!g_fn_start || !scr) return false;
    __try {
        reinterpret_cast<void(__thiscall*)(void*, const char*)>(g_fn_start)(scr, task);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CallSetTaskParam(void* scr, const char* key, const char* value) {
    if (!g_fn_param || !scr) return false;
    __try {
        reinterpret_cast<void(__thiscall*)(void*, const char*, const char*)>(g_fn_param)(scr, key, value);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CallTaskActive(void* brain, const char* task) {
    void* fn = VtSlot(brain, kBrainVtTaskActive);
    if (!fn) return false;
    __try {
        return reinterpret_cast<void*(__thiscall*)(void*, const char*)>(fn)(brain, task) != nullptr;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CallCustomProps(void* scr, const char* method) {
    if (!g_fn_props || !scr) return false;
    __try {
        reinterpret_cast<void(__thiscall*)(void*, const char*)>(g_fn_props)(scr, method);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int CallAddAnimal(int species, const char* name) {
    if (!g_add_hook) return -2;
    __try {
        return g_add_hook->unsafe_ccall<int>(species, 1, name);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -3;
    }
}

// The newest cScrAnimal with this target name in the live manager (a BSP marker may share the name).
void* FindAnimalNamed(const char* name) {
    void* mgr = EntityManager();
    int count = 0;
    void** arr = nullptr;
    if (!mgr || !core::safe_read(static_cast<char*>(mgr) + 0x08, count) ||
        !core::safe_read(static_cast<char*>(mgr) + 0x04, arr) || !arr || count <= 0 || count > 0x400)
        return nullptr;
    void* found = nullptr;
    for (int i = 0; i < count; ++i) {
        void* ent = nullptr;
        if (!core::safe_read(arr + i, ent) || !IsAnimal(ent)) continue;
        if (NameOf(ent) == name) found = ent;
    }
    return found;
}

ScriptAnimal* FindScript(void* ent) {
    for (ScriptAnimal& s : g_script) {
        if (s.ent == ent) return &s;
    }
    return nullptr;
}

Reserve* FindReserve(void* ent) {
    for (Reserve& r : g_reserves) {
        if (r.ent == ent) return &r;
    }
    return nullptr;
}

// On its scripted attack right now: the brain still runs the AIHuntTask_Attack* the script gave it. The attack ends when
// the script stops it, starts another task, or the task ends by itself ("time" param, flee).
bool Attacking(const ScriptAnimal& s) {
    if (s.tmpl.task.empty()) return false;
    void* brain = Read<void*>(s.ent, kEntBrain, nullptr);
    return brain && CallTaskActive(brain, s.tmpl.task.c_str());
}

bool FamilyHasReserves(int family) {
    for (const Reserve& r : g_reserves) {
        if (FamilyOf(r.species) == family) return true;
    }
    return false;
}

const MapPool* PoolFor(const std::string& map) {
    for (const MapPool& p : kPools) {
        if (map == p.map) return &p;
    }
    return nullptr;
}

void ClearMap() {
    g_script.clear();
    g_reserves.clear();
    g_marks.clear();
    g_batch.clear();
    g_families.clear();
    g_script_kills.clear();
    g_arena.clear();
    g_built = false;
}

// ---- health ----

void CommitMax(void* ent, const Mark& m) {
    if (!NetIsHost()) return;
    const int16_t id = Read<int16_t>(ent, kEntId, -1);
    if (id < 0) return;
    ++g_mark_serial;
    // The key is unique per commit: a later max for the same animal (a join or a leave) must not be deduplicated.
    const std::string key = fmt::format("{:.4f} #{}", m.factor, g_mark_serial);
    events::Commit(events::kScaleMax, static_cast<uint16_t>(id), g_mark_serial, m.base * m.factor, key, 0);
}

// Host (or solo): put this animal's max at base * factor. Current health keeps its ratio to the max, so a join during
// the fight does not refill the bar and a leave does not heal it. A dead animal stays at 0.
void ApplyFactor(void* ent, Mark& m, float factor, const char* why) {
    const float old_max = Read<float>(ent, kEntMaxHealth, 0.f);
    const float old_hp = Health(ent);
    const float ratio = factor / m.factor;
    const float max = m.base * factor;
    Write<float>(ent, kEntMaxHealth, max);
    float hp = old_hp;
    if (old_hp > 0.f) {
        hp = old_hp * ratio;
        Write<float>(ent, kEntHealth, hp);
    }
    m.factor = factor;
    spdlog::info("scale: {} '{}' id={} max {:.1f} -> {:.1f} hp {:.1f} -> {:.1f} (factor {:.2f}, players {})", why,
                 NameOf(ent), Read<int16_t>(ent, kEntId, -1), old_max, max, old_hp, hp, factor, ConnectedPlayers());
    CommitMax(ent, m);
}

void MarkHealth(void* ent, const char* why) {
    if (!g_cfg.health || !g_map_ok || !ent) return;
    if (g_marks.count(ent) != 0) return;  // already scaled for this map; a join/leave recomputes by ratio
    Mark m;
    m.base = Read<float>(ent, kEntMaxHealth, 0.f);  // only spawn writes +0x4C0: this is the spawn max
    if (!(m.base > 0.f)) {
        spdlog::warn("scale: {} '{}' has max health {:.1f}; not scaled", why, NameOf(ent), m.base);
        return;
    }
    Mark& stored = g_marks[ent] = m;
    ApplyFactor(ent, stored, FactorFor(Extra()), why);
}

// Host or solo, every frame: the player count changed since the factor was applied.
void RecomputeMarks() {
    const float want = FactorFor(Extra());
    for (auto& [ent, m] : g_marks) {
        if (std::fabs(m.factor - want) < 1e-4f) continue;
        ApplyFactor(ent, m, want, "player count");
    }
}

void OnScaleMaxCommit(const net::GameEvent& ev) {
    void* ent = EntityById(ev.subjectId);
    if (!IsAnimal(ent)) {
        spdlog::warn("scale: max commit id={} '{}': no animal here", ev.subjectId, ev.key);
        return;
    }
    const float factor = static_cast<float>(std::atof(ev.key.c_str()));
    if (!(factor > 0.f) || !(ev.f0 > 0.f)) return;
    const float old_max = Read<float>(ent, kEntMaxHealth, 0.f);
    Write<float>(ent, kEntMaxHealth, ev.f0);
    Mark& m = g_marks[ent];
    m.base = ev.f0 / factor;
    m.factor = factor;
    spdlog::info("scale: host max '{}' id={} {:.1f} -> {:.1f} (factor {:.2f}); hp here {:.1f}", NameOf(ent),
                 ev.subjectId, old_max, ev.f0, factor, Health(ent));
}

// ---- waves and reserves ----

bool AttackTask(const std::string& task) { return task.rfind("AIHuntTask_Attack", 0) == 0; }

void ClassifyBatch() {
    if (g_batch.empty()) return;
    std::map<int, std::vector<ScriptAnimal*>> groups;
    for (void* ent : g_batch) {
        ScriptAnimal* s = FindScript(ent);
        if (!s || IsBoss(s->species) || IsCroc(s->species)) continue;
        auto& members = groups[FamilyOf(s->species)];
        if (std::find(members.begin(), members.end(), s) == members.end()) members.push_back(s);
    }
    g_batch.clear();
    for (auto& [family, members] : groups) {
        for (ScriptAnimal* s : members) {
            if (!s->attacking_prev) s->fresh = true;
        }
        std::string names;
        for (ScriptAnimal* s : members) names += (names.empty() ? "" : ",") + s->name;
        int others = 0;
        for (ScriptAnimal& o : g_script) {
            if (FamilyOf(o.species) != family || o.lone_large || !Attacking(o)) continue;
            if (std::find(members.begin(), members.end(), &o) != members.end()) continue;
            if (Alive(o.ent) && !Parked(o.ent)) ++others;
        }
        const bool lone = members.size() == 1 && IsLarge(members[0]->species) && others == 0;
        if (g_cfg.log) {
            spdlog::info("scale: wave {} x{} [{}] task={} others={} -> {}", SpeciesName(family), members.size(), names,
                         members[0]->tmpl.task, others, lone ? "one body, health" : "pack");
        }
        if (lone) {
            members[0]->lone_large = true;
            MarkHealth(members[0]->ent, "lone large");
        } else {
            for (ScriptAnimal* s : members) s->lone_large = false;
        }
    }
}

Vec3 SlotOffset(int slot) {
    static const float kDir[4][2] = {{1.f, 0.f}, {-1.f, 0.f}, {0.f, 1.f}, {0.f, -1.f}};
    const float d = 1.5f * static_cast<float>(1 + slot / 4);
    return Vec3{kDir[slot % 4][0] * d, 0.f, kDir[slot % 4][1] * d};
}

Reserve* PickReserve(int family, int species) {
    Reserve* best = nullptr;
    int best_rank = 99;
    for (Reserve& r : g_reserves) {
        if (FamilyOf(r.species) != family || !Parked(r.ent)) continue;
        if (r.killed && !g_cfg.reuse_killed) continue;
        // Never-killed first (a fresh body needs no revive on the clients). Then the template's own species.
        const int rank = (r.killed ? 2 : 0) + (r.species == species ? 0 : 1);
        if (rank < best_rank) {
            best = &r;
            best_rank = rank;
        }
    }
    return best;
}

bool Release(Reserve& r, const ScriptAnimal& t, int slot) {
    const Vec3 at = Pos(t.ent);
    if (at.y < kParkedY) return false;
    const Vec3 off = SlotOffset(slot);
    const float pos[3] = {at.x + off.x, at.y, at.z + off.z};
    const bool respawned = CallRespawn(r.ent);
    // Reserve health (re/notes/life.md): the species max (what the respawn just wrote), or the template's scripted
    // health when the script lowered it (MAP4A lions SetHealth(100) for the truck chase; species 170). A script that
    // raises one animal singles it out (MAP2A wolf3 600, MAP3C lions 500/200); the extra bodies stay ordinary members.
    const float species_max = Read<float>(r.ent, kEntMaxHealth, 0.f);
    if (respawned && t.script_hp > 0.f && t.script_hp < species_max) Write<float>(r.ent, kEntHealth, t.script_hp);
    const bool placed = CallSetPos(r.ent, pos);
    const Vec3 ang = Read<Vec3>(t.ent, kEntAngle, Vec3{});
    Write<Vec3>(r.ent, kEntAngle, ang);
    void* scr = CallGetScr(r.ent);
    g_replaying = true;
    std::string props;
    for (const std::string& prop : t.tmpl.props) {
        g_arena.push_back(prop);
        const bool ok = CallCustomProps(scr, g_arena.back().c_str());
        props += fmt::format(" {}{}", prop, ok ? "" : "(fault)");
    }
    g_arena.push_back(t.tmpl.task);
    const bool started = CallStartTask(scr, g_arena.back().c_str());
    std::string params;
    for (const auto& [k, v] : t.tmpl.params) {
        g_arena.push_back(k);
        const char* key = g_arena.back().c_str();
        g_arena.push_back(v);
        CallSetTaskParam(scr, key, g_arena.back().c_str());
        params += fmt::format(" {}={}", k, v);
    }
    g_replaying = false;
    r.released = true;
    const Vec3 now = Pos(r.ent);
    void* brain = Read<void*>(r.ent, kEntBrain, nullptr);
    const bool running = brain && CallTaskActive(brain, t.tmpl.task.c_str());
    spdlog::info("scale: release {} id={} like '{}' (id {}) respawn={} placed={} scr={} props:{} task={} ok={} running={}{} "
                 "pos=({:.1f},{:.1f},{:.1f}) hp={:.1f}/{:.1f}{} (players {})",
                 r.name, r.id, t.name, t.id, respawned, placed, scr != nullptr, props.empty() ? " none" : props,
                 t.tmpl.task, started, running, params, now.x,
                 now.y, now.z, Health(r.ent), Read<float>(r.ent, kEntMaxHealth, 0.f), r.killed ? " (reused after a death)" : "",
                 ConnectedPlayers());
    return respawned && placed;
}

// Host (and solo, where extra is 0): keep `extra` live reserves in a family while its scripted attack runs; follow a
// script park.
void UpdateBodies() {
    const int extra = Extra();
    for (Reserve& r : g_reserves) {
        if (r.released && !r.killed && Health(r.ent) <= 0.f) {
            r.killed = true;
            spdlog::info("scale: {} id={} died", r.name, r.id);
        }
    }
    std::map<int, std::vector<ScriptAnimal*>> active;
    std::map<int, int> up_count;
    for (ScriptAnimal& s : g_script) {
        const int family = FamilyOf(s.species);
        if (!FamilyHasReserves(family)) continue;
        const bool up = Alive(s.ent) && !Parked(s.ent);
        if (up) ++up_count[family];
        if (up && !s.lone_large && Attacking(s)) active[family].push_back(&s);
    }
    // The script took its own animals of this family off the map (Kill(), e.g. MAP2B before the grizzly): the extra
    // bodies leave with them. A Kill() followed by MoveToPos in the same frame is a teleport and leaves one up.
    for (int family : g_script_kills) {
        if (up_count[family] != 0) continue;
        for (Reserve& r : g_reserves) {
            if (FamilyOf(r.species) != family || Parked(r.ent) || !Alive(r.ent)) continue;
            const bool ok = CallPark(r.ent);
            spdlog::info("scale: script parked every {}; parking {} id={} ok={}", SpeciesName(family), r.name, r.id, ok);
        }
    }
    g_script_kills.clear();
    std::vector<int> families;
    for (const Reserve& r : g_reserves) {
        const int f = FamilyOf(r.species);
        if (std::find(families.begin(), families.end(), f) == families.end()) families.push_back(f);
    }
    for (int family : families) {
        Family& fam = g_families[family];
        auto it = active.find(family);
        const size_t n = it == active.end() ? 0 : it->second.size();
        if (n == 0) {
            if (fam.epoch && g_cfg.log) spdlog::info("scale: {} attack over (no script attacker up)", SpeciesName(family));
            fam.epoch = false;
            continue;
        }
        // Top up when the family's attack begins, and when a later trigger sends in an animal that was not attacking
        // (a new wave while an older one still runs). A StartTask on an animal already attacking is a retarget, and a
        // player joining mid-wave adds nothing until the script starts a new attack.
        std::vector<ScriptAnimal*>& attackers = it->second;
        const bool begins = !fam.epoch;
        bool fresh = false;
        for (ScriptAnimal* s : attackers) fresh = fresh || s->fresh;
        fam.epoch = true;
        if (!begins && !fresh) continue;
        std::sort(attackers.begin(), attackers.end(), [](ScriptAnimal* a, ScriptAnimal* b) { return a->id < b->id; });
        int live = 0;
        for (const Reserve& r : g_reserves) {
            if (FamilyOf(r.species) == family && !Parked(r.ent) && Alive(r.ent)) ++live;
        }
        const int need = std::min(extra, g_cfg.reserves) - live;
        spdlog::info("scale: {} {}: {} script attacker(s), {} reserve(s) up, extra {} -> release {}", SpeciesName(family),
                     begins ? "attack begins" : "new attackers join the attack", n, live, extra, std::max(0, need));
        for (int k = 0; k < need; ++k) {
            const ScriptAnimal* t = attackers[fam.rr++ % attackers.size()];
            Reserve* r = PickReserve(family, t->species);
            if (!r) {
                spdlog::warn("scale: no {} reserve left to release (killed ones are kept parked unless ReuseKilled=1)",
                             SpeciesName(family));
                break;
            }
            Release(*r, *t, live + k);
        }
    }
    for (ScriptAnimal& s : g_script) {
        s.fresh = false;
        s.attacking_prev = Alive(s.ent) && !Parked(s.ent) && Attacking(s);
    }
}

// Client: the snapshot moved a reserve off the park position (the host released it).
void WatchClientReserves() {
    for (Reserve& r : g_reserves) {
        const Vec3 p = Pos(r.ent);
        const float dx = p.x - r.last_pos.x, dy = p.y - r.last_pos.y, dz = p.z - r.last_pos.z;
        r.last_pos = p;
        // Up = off the park spot and settled: the mirror interpolates from (-10000,...) for a few frames first.
        const bool up = p.y >= kParkedY && dx * dx + dy * dy + dz * dz < 1.f;
        if (p.y < kParkedY) r.seen_up = false;
        if (up && !r.seen_up) {
            const std::string before = PhysWords(r.ent);
            // The parked body keeps the park collision words (no collision). Script MoveToPos is what restores them on
            // every other animal, so run the same respawn here, put the body back where the mirror placed it, and freeze
            // the brain again before it can tick (the respawn thaws it: gap G5).
            bool respawned = false, frozen = false;
            if (g_cfg.client_respawn && Alive(r.ent)) {
                const float at[3] = {p.x, p.y, p.z};
                respawned = CallRespawn(r.ent) && CallSetPos(r.ent, at);
                frozen = entity::FreezeBrain(r.ent, true);
            }
            spdlog::info("scale: client sees {} id={} up at ({:.1f},{:.1f},{:.1f}) hp={:.1f} state='{}' {} -> respawn={} "
                         "frozen={} {} brain79={}",
                         r.name, r.id, p.x, p.y, p.z, Health(r.ent), entity::BodyStateName(r.ent), before, respawned, frozen,
                         PhysWords(r.ent), Read<uint8_t>(Read<char*>(r.ent, 0x200, nullptr), 0x79, 0xFF));
            r.seen_up = true;
        }
    }
}

void CheckScriptSlots(void* ent) {
    if (g_vt_checked) return;
    void* scr = CallGetScr(ent);
    if (!scr) return;
    g_vt_checked = true;
    const bool start = ThroughThunk(VtSlot(scr, kScrVtStartTask)) == g_fn_start;
    const bool param = ThroughThunk(VtSlot(scr, kScrVtSetTaskParam)) == g_fn_param;
    const auto level = start && param ? spdlog::level::info : spdlog::level::err;
    spdlog::log(level, "scale: animal script object {}: StartTask {} SetTaskParam {} (the hooked bodies: {}/{})",
                core::describe_address(Read<void*>(scr, 0, nullptr)), core::describe_address(VtSlot(scr, kScrVtStartTask)),
                core::describe_address(VtSlot(scr, kScrVtSetTaskParam)), start, param);
}

void LogState(const char* why) {
    spdlog::info("scale: state ({}) map={} ok={} players={} extra={} health={} bodies={} script={} reserves={} marks={}",
                 why, g_map, g_map_ok, ConnectedPlayers(), Extra(), g_cfg.health, g_cfg.bodies, g_script.size(),
                 g_reserves.size(), g_marks.size());
    for (const ScriptAnimal& s : g_script) {
        const Vec3 p = Pos(s.ent);
        spdlog::info("scale:   script {} id={} {} attacking={} lone={} task='{}' params={} props={} hp={:.1f}/{:.1f} "
                     "pos=({:.1f},{:.1f},{:.1f}) state='{}' brain79={} {}",
                     s.name, s.id, SpeciesName(s.species), Attacking(s), s.lone_large, s.tmpl.task, s.tmpl.params.size(),
                     s.tmpl.props.size(),
                     Health(s.ent), Read<float>(s.ent, kEntMaxHealth, 0.f), p.x, p.y, p.z, entity::BodyStateName(s.ent),
                     Read<uint8_t>(Read<char*>(s.ent, 0x200, nullptr), 0x79, 0xFF), PhysWords(s.ent));
    }
    for (const Reserve& r : g_reserves) {
        const Vec3 p = Pos(r.ent);
        spdlog::info("scale:   reserve {} id={} released={} killed={} hp={:.1f}/{:.1f} pos=({:.1f},{:.1f},{:.1f}) state='{}' "
                     "brain79={} {}",
                     r.name, r.id, r.released, r.killed, Health(r.ent), Read<float>(r.ent, kEntMaxHealth, 0.f), p.x, p.y,
                     p.z, entity::BodyStateName(r.ent), Read<uint8_t>(Read<char*>(r.ent, 0x200, nullptr), 0x79, 0xFF),
                     PhysWords(r.ent));
    }
    for (const auto& [ent, m] : g_marks) {
        spdlog::info("scale:   mark '{}' id={} base={:.1f} factor={:.2f} max={:.1f} hp={:.1f}", NameOf(ent),
                     Read<int16_t>(ent, kEntId, -1), m.base, m.factor, Read<float>(ent, kEntMaxHealth, 0.f), Health(ent));
    }
}

void __cdecl cmd_scale() { LogState("coop_scale"); }

// Test: coop_killshot <id> kills an animal the way a player's bullet reports it. The animal's bullet OnHit sets
// +0x420 = 1 (MISSION_ACTION_KILL) before the death; coop_kill alone reports action 0, which no kill task counts.
void __cdecl cmd_killshot(const char* arg) {
    const int id = std::atoi(core::safe_cstr(arg, 16).c_str());
    void* ent = id > 0 ? EntityById(id) : nullptr;
    if (!IsAnimal(ent)) {
        spdlog::warn("scale: coop_killshot <animal id>: no animal {}", id);
        return;
    }
    Write<int>(ent, kEntKillAction, 1);
    animals::DebugDamage(static_cast<uint16_t>(id), 100000.f);
}

// ---- the host's [Scale] Bodies/Reserves (they decide entity ids, so the host's values win) ----

uint32_t PackConfig(bool bodies, int reserves) {
    return (bodies ? 1u : 0u) | (static_cast<uint32_t>(std::clamp(reserves, 0, kMaxReserves)) << 8);
}

// Not journaled: a client keeps the values for the whole session, and a client that is still at the main menu (not
// synced, so Commit would skip it) must have them before it loads its first story map.
void SendConfigTo(uint8_t peer) {
    net::GameEvent ev;
    ev.flags = net::game_event_flag::kCommit;
    ev.actorId = 0;
    ev.kind = events::kScaleConfig;
    ev.i0 = PackConfig(g_cfg.bodies, g_cfg.reserves);
    ev.key = fmt::format("bodies={} reserves={}", g_cfg.bodies ? 1 : 0, g_cfg.reserves);
    const bool sent = SendGameEventTo(peer, ev);
    spdlog::info("scale: sent [Scale] {} to player {} sent={}", ev.key, peer, sent);
}

// Every frame: the host sends its values to each client once per connection; a process that is offline forgets the
// host's values (its own ini applies again).
void PumpConfig() {
    if (!NetIsOnline()) {
        g_host_cfg.have = false;
        g_cfg_sent.clear();
        return;
    }
    if (!NetIsHost()) return;
    const std::vector<uint8_t> peers = PeerIds();
    g_cfg_sent.erase(std::remove_if(g_cfg_sent.begin(), g_cfg_sent.end(),
                                    [&](uint8_t id) { return std::find(peers.begin(), peers.end(), id) == peers.end(); }),
                     g_cfg_sent.end());
    for (uint8_t id : peers) {
        if (std::find(g_cfg_sent.begin(), g_cfg_sent.end(), id) != g_cfg_sent.end()) continue;
        g_cfg_sent.push_back(id);
        SendConfigTo(id);
    }
}

void OnScaleConfigCommit(const net::GameEvent& ev) {
    const bool bodies = (ev.i0 & 1u) != 0;
    const int reserves = std::clamp(static_cast<int>((ev.i0 >> 8) & 0xffu), 0, kMaxReserves);
    g_host_cfg.have = true;
    g_host_cfg.bodies = bodies;
    g_host_cfg.reserves = reserves;
    const bool differs = bodies != g_cfg.bodies || reserves != g_cfg.reserves;
    spdlog::info("scale: host [Scale] bodies={} reserves={} (this ini bodies={} reserves={}){}", bodies, reserves,
                 g_cfg.bodies, g_cfg.reserves, differs ? ": the host's values apply to every map loaded from now on" : "");
    if (!g_built) return;
    const int built = g_built_bodies ? g_built_reserves : 0;
    const int want = bodies ? reserves : 0;
    if (built != want) {
        spdlog::error("scale: map '{}' was loaded with {} reserve(s) per pool species, the host uses {}: entity ids after "
                      "the reserve pool differ from the host's until the next map (host: F5 twice restarts the level)",
                      g_map, built, want);
    }
}

// A reserve that reached a flee point: the engine's next cScrAnimal::Update would park it and report the map's flee
// action (MISSION_ACTION_KILL on MAP2A/2B/2C/4B/7/12A/12B) to the mission, so a fleeing extra body would count as a
// kill. Park it here first, without the report. +0x590 is set by GameDll 0x14940 ("reached flee_point") during the
// brain update and read at the start of the next Update; this runs in between.
void ParkFleeingReserves() {
    for (Reserve& r : g_reserves) {
        if (Read<int>(r.ent, kEntFleeReached, 0) != 1) continue;
        Write<int>(r.ent, kEntFleeReached, 0);
        const bool ok = CallPark(r.ent);
        spdlog::info("scale: {} id={} reached a flee point: parked without a mission report ok={}", r.name, r.id, ok);
    }
}

// ---- hooks ----

int __cdecl hk_AddAnimal(int species, int flag, const char* name) {
    const int r = g_add_hook->unsafe_ccall<int>(species, flag, name);
    if (!g_creating) AfterAddAnimal(species, name);
    return r;
}

void __fastcall hk_AddAnimalOnMap(void* mgr) {
    BeforeAddAnimalOnMap();
    g_onmap_hook->unsafe_fastcall<void>(mgr);
    AfterAddAnimalOnMap();
}

void __fastcall hk_MakeBoss(void* scr, void*, int on, const char* name) {
    g_boss_hook->unsafe_thiscall<void>(scr, on, name);
    AfterMakeBoss(scr, on);
}

void __fastcall hk_SetHealth(void* scr, void*, float hp) {
    g_sethealth_hook->unsafe_thiscall<void>(scr, hp);
    AfterSetHealth(scr);
}

void __fastcall hk_StartTask(void* scr, void*, char* task) {
    g_start_hook->unsafe_thiscall<void>(scr, task);
    AfterStartTask(scr, task);
}

void __fastcall hk_Kill(void* scr) {
    g_kill_hook->unsafe_fastcall<void>(scr);
    AfterKill(scr);
}

void __fastcall hk_CustomProps(void* scr, void*, char* method) {
    g_props_hook->unsafe_thiscall<void>(scr, method);
    AfterCustomProps(scr, method);
}

void __fastcall hk_SetTaskParam(void* scr, void*, char* key, char* value) {
    g_param_hook->unsafe_thiscall<void>(scr, key, value);
    AfterSetTaskParam(scr, key, value);
}

class ScaleModule : public dh2::Module {
public:
    const char* Name() const override { return "scale"; }
    int Priority() const override { return 49; }

    void OnInit() override {
        const std::string ini = ini_path();
        g_cfg.health = GetPrivateProfileIntA("Scale", "Health", 1, ini.c_str()) != 0;
        g_cfg.bodies = GetPrivateProfileIntA("Scale", "Bodies", 1, ini.c_str()) != 0;
        g_cfg.reserves = std::clamp(static_cast<int>(GetPrivateProfileIntA("Scale", "Reserves", kMaxReserves, ini.c_str())),
                                    0, kMaxReserves);
        g_cfg.log = GetPrivateProfileIntA("Scale", "Log", 1, ini.c_str()) != 0;
        g_cfg.client_respawn = GetPrivateProfileIntA("Scale", "ClientRespawn", 1, ini.c_str()) != 0;
        g_cfg.reuse_killed = GetPrivateProfileIntA("Scale", "ReuseKilled", 1, ini.c_str()) != 0;
        events::SetHandler(events::kScaleMax, nullptr, &OnScaleMaxCommit);
        events::SetHandler(events::kScaleConfig, nullptr, &OnScaleConfigCommit);
        spdlog::info("scale: health={} bodies={} reserves={} clientRespawn={} reuseKilled={} log={}", g_cfg.health,
                     g_cfg.bodies, g_cfg.reserves, g_cfg.client_respawn, g_cfg.reuse_killed, g_cfg.log);
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_game = game;
        g_vt_animal = reinterpret_cast<uintptr_t>(game) + kRvaAnimalVt;
        g_add_hook = core::install_hook("AddAnimal (scale, 0x8d970)", CheckedRva(kRvaAddAnimal, "AddAnimal"),
                                        reinterpret_cast<void*>(&hk_AddAnimal));
        g_onmap_hook = core::install_hook("AddAnimalOnMap runner (scale, 0x8cab0)",
                                          CheckedRva(kRvaAddAnimalOnMap, "AddAnimalOnMap runner"),
                                          reinterpret_cast<void*>(&hk_AddAnimalOnMap));
        g_boss_hook = core::install_hook("MakeBoss (scale, 0x16d20)", CheckedRva(kRvaMakeBoss, "MakeBoss"),
                                         reinterpret_cast<void*>(&hk_MakeBoss));
        g_sethealth_hook = core::install_hook("SetHealth (scale, 0x15ff0)", CheckedRva(kRvaSetHealth, "SetHealth"),
                                              reinterpret_cast<void*>(&hk_SetHealth));
        g_fn_props = CheckedRva(kRvaCustomProps, "CustomProps");
        g_props_hook = core::install_hook("CustomProps (scale, 0x16300)", g_fn_props, reinterpret_cast<void*>(&hk_CustomProps));
        g_kill_hook = core::install_hook("script Kill (scale, 0x16a40)", CheckedRva(kRvaKill, "Kill"),
                                         reinterpret_cast<void*>(&hk_Kill));
        HMODULE common = GetModuleHandleA("CommonDll7.dll");
        if (!common) {
            spdlog::error("scale: CommonDll7 not loaded; task hooks skipped");
            return;
        }
        g_get_scr = core::find_export_as<GetScrFn>(common, "?GetScrObject@cObjScriptedEntity@@QAEPAVcScrEntity@@XZ");
        g_fn_start = core::find_export(common, "?StartTask@cScrCharacterEx@@UAEXPAD@Z");
        g_fn_param = core::find_export(common, "?SetTaskParam@cScrCharacterEx@@UAEXPAD0@Z");
        g_start_hook = core::install_hook("cScrCharacterEx::StartTask (scale)", g_fn_start,
                                          reinterpret_cast<void*>(&hk_StartTask));
        g_param_hook = core::install_hook("cScrCharacterEx::SetTaskParam (scale)", g_fn_param,
                                          reinterpret_cast<void*>(&hk_SetTaskParam));
    }

    void OnMapUnloading() override { ClearMap(); }

    void OnFrame(float) override {
        if (!commands_ && console_cmds::Ready()) {
            commands_ = console_cmds::AddCommand0("coop_scale", &cmd_scale);
            console_cmds::AddCommandStr("coop_killshot", &cmd_killshot);
        }
        PumpConfig();
        if (g_map_ok) ParkFleeingReserves();
        if (!g_map_ok) {
            g_batch.clear();
            g_script_kills.clear();
            return;
        }
        if (!Authoritative()) {
            g_batch.clear();
            g_script_kills.clear();
            WatchClientReserves();
            return;
        }
        ClassifyBatch();
        RecomputeMarks();
        // Solo too (extra 0 releases nothing): the attack bookkeeping must already be running when a client joins.
        if (g_cfg.bodies) UpdateBodies();
        g_script_kills.clear();
        const int players = ConnectedPlayers();
        if (players != last_players_) {
            last_players_ = players;
            if (g_cfg.log) LogState("player count");
        }
    }

private:
    bool commands_ = false;
    int last_players_ = 0;
};

DH2_MODULE(ScaleModule)

}  // namespace

void AfterAddAnimal(int species, const char* name) {
    if (!name) return;
    void* ent = FindAnimalNamed(name);
    if (!ent) {
        spdlog::warn("scale: AddAnimal({}, '{}') entity not found", SpeciesName(species), core::safe_cstr(name, 48));
        return;
    }
    if (FindScript(ent)) return;
    ScriptAnimal s;
    s.ent = ent;
    s.id = Read<int16_t>(ent, kEntId, -1);
    s.species = Read<int>(ent, kEntSpecies, species);
    s.name = core::safe_cstr(name, 48);
    g_script.push_back(std::move(s));
}

void BeforeAddAnimalOnMap() {
    ClearMap();
    g_map = events::CurrentMap();
    g_map_ok = !g_map.empty() && g_map.rfind("HUNTMAP", 0) != 0 && g_map != "MENUMAP" && g_map != "MAPMENU";
    g_vt_checked = false;
}

void AfterAddAnimalOnMap() {
    std::string list;
    for (const ScriptAnimal& s : g_script) list += fmt::format(" {}={}({})", s.name, s.id, SpeciesName(s.species));
    spdlog::info("scale: map '{}' story animals{} (scaling {})", g_map, list, g_map_ok ? "on" : "off on this map");
    if (!g_script.empty()) CheckScriptSlots(g_script.front().ent);
    const bool bodies = UseBodies();
    const int reserves = std::clamp(UseReserves(), 0, kMaxReserves);
    g_built = false;
    if (!g_map_ok) return;
    const MapPool* pool = PoolFor(g_map);
    if (!pool) return;
    // From here the entity list depends on Bodies/Reserves: remember what this map was built with.
    g_built = true;
    g_built_bodies = bodies;
    g_built_reserves = reserves;
    spdlog::info("scale: pool on '{}' built with bodies={} reserves={} ({})", g_map, bodies, reserves,
                 g_host_cfg.have && NetIsOnline() && !NetIsHost() ? "the host's values" : "this ini");
    if (!bodies || reserves <= 0) return;
    std::string made;
    for (int species : pool->species) {
        if (species < 0 || species >= kSpeciesCount) break;
        const bool present = std::any_of(g_script.begin(), g_script.end(), [&](const ScriptAnimal& s) { return s.species == species; });
        if (!present) {
            spdlog::warn("scale: pool species {} is not in this map's AddAnimal list; skipped", SpeciesName(species));
            continue;
        }
        for (int n = 0; n < reserves; ++n) {
            char* name = g_names[species][n];
            std::snprintf(name, sizeof g_names[species][n], "coop_%s_%d", SpeciesName(species), n);
            g_creating = true;
            const int r = CallAddAnimal(species, name);
            g_creating = false;
            void* ent = FindAnimalNamed(name);
            if (!ent) {
                spdlog::error("scale: AddAnimal({}, '{}') -> {}: no entity", SpeciesName(species), name, r);
                continue;
            }
            Reserve res;
            res.ent = ent;
            res.id = Read<int16_t>(ent, kEntId, -1);
            res.species = species;
            res.name = name;
            g_reserves.push_back(res);
            made += fmt::format(" {}={}", name, res.id);
        }
    }
    spdlog::info("scale: reserves on '{}':{}", g_map, made.empty() ? " none" : made);
}

void AfterStartTask(void* scr, const char* task) {
    if (g_replaying || !task) return;
    ScriptAnimal* s = FindScript(EntityOfScr(scr));
    if (!s) return;
    const std::string name = core::safe_cstr(task, 64);
    s->params_open = AttackTask(name);
    if (!s->params_open) return;  // another task: the attack template stays; IsTaskActive says whether it still runs
    s->tmpl.task = name;
    s->tmpl.params.clear();
    g_batch.push_back(s->ent);
}

void AfterSetTaskParam(void* scr, const char* key, const char* value) {
    if (g_replaying || !key || !value) return;
    ScriptAnimal* s = FindScript(EntityOfScr(scr));
    if (!s || !s->params_open || s->tmpl.params.size() >= kMaxTemplateParams) return;
    s->tmpl.params.emplace_back(core::safe_cstr(key, 32), core::safe_cstr(value, 64));
}

void AfterKill(void* scr) {
    if (g_replaying) return;
    const ScriptAnimal* s = FindScript(EntityOfScr(scr));
    if (!s) return;
    const int family = FamilyOf(s->species);
    if (std::find(g_script_kills.begin(), g_script_kills.end(), family) == g_script_kills.end()) g_script_kills.push_back(family);
}

void AfterCustomProps(void* scr, const char* method) {
    if (g_replaying || !method) return;
    ScriptAnimal* s = FindScript(EntityOfScr(scr));
    if (!s) return;
    const std::string name = core::safe_cstr(method, 48);
    // Make_Scared is the grizzly's 10000-health scare: never copied onto a reserve.
    if (name.empty() || name == "Make_Scared" || s->tmpl.props.size() >= 8) return;
    if (std::find(s->tmpl.props.begin(), s->tmpl.props.end(), name) == s->tmpl.props.end()) s->tmpl.props.push_back(name);
}

void AfterMakeBoss(void* scr, int on) {
    if (!on || !Authoritative()) return;
    void* ent = EntityOfScr(scr);
    if (!IsAnimal(ent)) return;
    MarkHealth(ent, "boss");
}

// Retail SetHealth changes the current health only. On a scaled animal the new value gets the same factor, on every
// peer (a lower local value on a client would read as a hit and be sent to the host).
void AfterSetHealth(void* scr) {
    void* ent = EntityOfScr(scr);
    if (ScriptAnimal* s = FindScript(ent)) s->script_hp = Health(ent);  // before any factor: what reserves may copy
    const auto it = g_marks.find(ent);
    if (it == g_marks.end() || std::fabs(it->second.factor - 1.f) < 1e-4f) return;
    const float hp = Health(ent);
    if (hp <= 0.f) return;
    Write<float>(ent, kEntHealth, hp * it->second.factor);
    spdlog::info("scale: SetHealth on '{}' id={} {:.1f} -> {:.1f} (factor {:.2f})", NameOf(ent),
                 Read<int16_t>(ent, kEntId, -1), hp, hp * it->second.factor, it->second.factor);
}

bool IsReserve(void* ent) { return ent && FindReserve(ent) != nullptr; }

void AfterRespawn(void* ent) {
    // Script MoveToPos put the health back to +0x4C0: a SetHealth from an earlier life no longer applies.
    if (ScriptAnimal* s = FindScript(ent)) s->script_hp = -1.f;
}

}  // namespace dh2::coop::scale
