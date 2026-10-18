#include "coop/player_tracker.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "net/protocol.h"

namespace dh2::coop {
namespace {

// cObjManager (EngineDll7): entity array +0x04, count +0x08, capacity 0x400 (Track B, lead decompile).
constexpr uintptr_t kMgrArray = 0x04;
constexpr uintptr_t kMgrCount = 0x08;
constexpr int kMgrMaxEntities = 0x400;
// cObjEntity
constexpr uintptr_t kEntId = 0x04;       // int16 id/index
constexpr uintptr_t kEntParent = 0xA4;   // int16, -1 = world-space
constexpr uintptr_t kEntPos = 0xE8;      // tVect3 world position
constexpr uintptr_t kEntAngle = 0xF4;    // tVect3 world angle
constexpr uintptr_t kEntHealth = 0x19C;  // float, cObjScriptedEntity (animals)
constexpr uintptr_t kEntModelName = 0xE0;  // char*, set by HandleKey("model_name") (lead decompile 0xbb660)

// cObjManager::DumpClassesKeys(const char* file) — engine dumps every entity class and the keys it accepts.
using DumpClassesKeysFn = void(__thiscall*)(void* mgr, const char* file);
DumpClassesKeysFn g_dump_classes = nullptr;
bool g_classes_dumped = false;

void DumpClassesOnce(void* mgr) {
    if (g_classes_dumped || !g_dump_classes || !mgr) return;
    g_classes_dumped = true;
    const std::string path = (core::self_dir() / "dh2coop-classes.txt").string();
    g_dump_classes(mgr, path.c_str());
    spdlog::info("tracker: DumpClassesKeys -> {}", path);
}
// GameDll7 vtable RVAs (Track B)
constexpr uintptr_t kVtHunterRva = 0x106ff0;  // cScr_ModHunter
// cObjPilot (script cScr_ModPilot): the player on the helicopter maps MAP11A / MAP11C (WORLD.SCR mod_player_cls).
// Its ctor 0x64660 runs the hunter ctor and then writes this vtable; cObjPilot::Spawn 0x65370 attaches it to the
// entity "helicopter" at tag "masscenter" and takes it out of the animals' attack list (re/notes/audit-maps-c.md 0.1).
constexpr uintptr_t kVtPilotRva = 0x109d48;
constexpr uintptr_t kVtAnimalRva = 0xff470;   // cScrAnimal
constexpr uintptr_t kVtCobraRva = 0x10e920;   // cObjCobra (mission cobras). Not cObjSnake +0x10e470.
// cObjCrocodile (obj_crocodile, ANIMAL_WATERCROCODILE): the cScrAnimal ctor plus this vtable and a water-trail FX
// (ctor GameDll 0x1d720). OnHit, death (+0x154) and the brain slot are cScrAnimal's; its MoveToPos (+0x158 0x17270)
// calls cScrAnimal's 0x14610 first (re/notes/beasts.md). ANIMAL_CROCODILE is a plain obj_animal.
constexpr uintptr_t kVtCrocodileRva = 0xffa50;

// SetLoadSaveObjManager(cObjManager*) (EngineDll7 export, RVA 0xbf6a0) stores Main7's live manager in an
// unexported global (RVA 0x5826ec). Its body has `A3 <imm32>` (mov [global], eax) at +0x1A; we verify that
// instruction and read the global instead of hooking (cObjManager::Get is never called via the export).
constexpr uintptr_t kSetLoadSaveRva = 0xbf6a0;
constexpr uintptr_t kSetLoadSaveStoreOff = 0x1A;
constexpr uintptr_t kLoadSaveMgrRva = 0x5826ec;

std::atomic<void**> g_manager_slot{nullptr};
// Render camera (EngineDll7 cRenderCamera): X axis +0x84, Z axis (view forward) +0xA4, position +0xB4,
// projection type +0xC4, fov +0xD0 (lead decompile of the cRenderCamera getters). cRender::SetCamera
// (EngineDll7 0x242b0, virtual) receives every camera the renderer uses; the first one each frame is the world
// view (verified by the [Tracker] DiffHunter camera log). The hunter's +0xF4 only follows the view while moving.
struct RenderCam {
    Vec3 x, z, pos;
    int proj = -1;
    float fov = 0.f;
};
safetyhook::InlineHook* g_setcam_hook = nullptr;
RenderCam g_frame_cam;            // first camera set in the current frame (main thread)
bool g_frame_cam_taken = false;
RenderCam g_view_cam;             // world camera of the last completed frame
bool g_view_cam_valid = false;
int g_setcam_log = 0;             // diagnostic: log the first N SetCamera calls
void (*g_prerender)() = nullptr;  // SetPreRenderCallback
std::atomic<DWORD> g_main_tid{0}; // game thread (set from OnFrame)
bool g_map_ready = false;         // local hunter known (set from Scan)

void __fastcall hk_SetCamera(void* self, void* /*edx*/, const char* cam) {
    if (cam && !g_frame_cam_taken && g_prerender && g_map_ready && GetCurrentThreadId() == g_main_tid.load())
        g_prerender();
    if (cam && !g_frame_cam_taken) {
        RenderCam c;
        if (core::safe_read(cam + 0x84, c.x) && core::safe_read(cam + 0xA4, c.z) && core::safe_read(cam + 0xB4, c.pos) &&
            core::safe_read(cam + 0xC4, c.proj) && core::safe_read(cam + 0xD0, c.fov)) {
            g_frame_cam = c;
            g_frame_cam_taken = true;
            // One line: stored Y (+0x94) vs y derived as z × x, so a mirrored basis shows up in the log.
            static bool y_checked = false;
            if (!y_checked) {
                y_checked = true;
                Vec3 stored{};
                core::safe_read(cam + 0x94, stored);
                const Vec3 derived{c.z.y * c.x.z - c.z.z * c.x.y, c.z.z * c.x.x - c.z.x * c.x.z,
                                   c.z.x * c.x.y - c.z.y * c.x.x};
                spdlog::info(
                    "tracker: view y stored=({:.3f},{:.3f},{:.3f}) cross(z,x)=({:.3f},{:.3f},{:.3f}) fov={:.3f} proj={}",
                    stored.x, stored.y, stored.z, derived.x, derived.y, derived.z, c.fov, c.proj);
            }
        }
    }
    if (g_setcam_log > 0 && cam) {
        --g_setcam_log;
        RenderCam c;
        core::safe_read(cam + 0xA4, c.z);
        core::safe_read(cam + 0xB4, c.pos);
        core::safe_read(cam + 0xC4, c.proj);
        core::safe_read(cam + 0xD0, c.fov);
        spdlog::info("tracker: SetCamera #{} first={} proj={} fov={:.3f} pos=({:.2f},{:.2f},{:.2f}) z=({:.3f},{:.3f},{:.3f})",
                     g_setcam_log, !g_frame_cam_taken, c.proj, c.fov, c.pos.x, c.pos.y, c.pos.z, c.z.x, c.z.y, c.z.z);
    }
    g_setcam_hook->unsafe_thiscall<void>(self, cam);
}

// Hunter action state (CommonDll7 lead decompile): crouched = cObjCharacterEx +0x204 bit 1 (IsCrouched 0x14950);
// current item index = cObjHumanEx +0x3c0 (SetCurrentItem 0x15150); items list +0x1d8. Weapon entities are
// children of the hunter (parent id +0xA4 == hunter id); cObjItemEx::GetCurrentState (0x32c00) returns the
// weapon model's body-state name, item activation state +0x1f0 (1/2 = active, IsActive 0x12bd0).
using GetStateFn = const char*(__thiscall*)(void* item);
GetStateFn g_item_state = nullptr;

const char* guarded_item_state(void* item) {
    __try {
        return g_item_state(item);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

struct ActionState {
    bool crouched = false;
    uint8_t weapon = 0;
    bool firing = false;
    bool reloading = false;
    uint8_t shots = 0;
    bool meleeing = false;
    uint32_t melees = 0;
};
ActionState g_act;

uint8_t weapon_kind(const char* model) {
    if (!model || !*model) return net::kWeaponNone;
    std::string m = model;
    for (auto& ch : m) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    // More specific folders first: "handgun51st" contains "handgun", "rifle21st" contains "rifle".
    if (m.find("knife") != std::string::npos) return net::kWeaponKnife;
    if (m.find("macheta") != std::string::npos) return net::kWeaponMachete;
    if (m.find("mbranch") != std::string::npos) return net::kWeaponBranch;
    if (m.find("handgun5") != std::string::npos) return net::kWeaponHandgun5;
    if (m.find("handgun") != std::string::npos || m.find("pistol") != std::string::npos) return net::kWeaponHandgun;
    if (m.find("shotgun") != std::string::npos || m.find("12ga") != std::string::npos) return net::kWeaponShotgun;
    if (m.find("rifle2") != std::string::npos) return net::kWeaponRifle2;
    if (m.find("rifle") != std::string::npos) return net::kWeaponRifle;
    return net::kWeaponOther;
}

void read_camera(LocalPlayerSample& s) {
    s.crouched = g_act.crouched;
    s.weapon = g_act.weapon;
    s.firing = g_act.firing;
    s.reloading = g_act.reloading;
    s.shots = g_act.shots;
    s.melees = g_act.melees;
    if (g_view_cam_valid) {
        const Vec3& z = g_view_cam.z;
        s.viewValid = true;
        s.viewYaw = std::atan2(z.x, z.z) * 57.2957795f;  // same convention as the hunter heading: 0 = +z, 90 = +x
        s.viewPitch = std::asin(std::clamp(z.y, -1.f, 1.f)) * 57.2957795f;
        s.viewPos = g_view_cam.pos;
    }
}
std::atomic<uintptr_t> g_game_base{0};

LocalPlayerSample g_local;
int g_animals = 0;
std::vector<void*> g_animal_list;
std::vector<void*> g_npc_list;
std::string g_hunter_model;

void** resolve_manager_slot(HMODULE engine) {
    const uintptr_t base = reinterpret_cast<uintptr_t>(engine);
    void* fn = core::find_export(engine, "?SetLoadSaveObjManager@@YAXPAVcObjManager@@@Z");
    if (!fn || reinterpret_cast<uintptr_t>(fn) != base + kSetLoadSaveRva) {
        spdlog::warn("tracker: SetLoadSaveObjManager not at expected RVA ({})", core::describe_address(fn));
        return nullptr;
    }
    std::uint8_t op = 0;
    std::uint32_t imm = 0;
    const char* store = reinterpret_cast<const char*>(fn) + kSetLoadSaveStoreOff;
    if (!core::safe_read(store, op) || !core::safe_read(store + 1, imm) || op != 0xA3 ||
        imm != base + kLoadSaveMgrRva) {
        spdlog::warn("tracker: SetLoadSaveObjManager store instruction mismatch (op={:#x} imm={:#x})", op, imm);
        return nullptr;
    }
    return reinterpret_cast<void**>(imm);
}

// The load/save global is only set around save/load, so the primary source is cObjManager::AddEntity
// (cObjEntity*): Main7 imports it and its manager override calls the engine body for every spawned entity.
safetyhook::InlineHook* g_add_hook = nullptr;
std::atomic<void*> g_added_manager{nullptr};

int __fastcall hk_AddEntity(void* self, void* /*edx*/, void* ent) {
    const int r = g_add_hook->unsafe_thiscall<int>(self, ent);
    if (self != g_added_manager.load(std::memory_order_relaxed)) {
        void* prev = g_added_manager.exchange(self);
        spdlog::info("tracker: AddEntity manager {:#x} (prev {:#x}) first entity {:#x} -> {}",
                     reinterpret_cast<uintptr_t>(self), reinterpret_cast<uintptr_t>(prev),
                     reinterpret_cast<uintptr_t>(ent), r);
    }
    return r;
}

void* current_manager() {
    if (void* m = g_added_manager.load()) return m;
    void** slot = g_manager_slot.load();
    void* mgr = nullptr;
    if (!slot || !core::safe_read(slot, mgr)) return nullptr;
    return mgr;
}

bool read_vec(void* ent, uintptr_t off, Vec3& out) {
    return core::safe_read(reinterpret_cast<char*>(ent) + off, out);
}

class PlayerTrackerModule : public dh2::Module {
public:
    const char* Name() const override { return "player_tracker"; }
    int Priority() const override { return 50; }

    void OnEngineReady(HMODULE engine) override {
        void** slot = resolve_manager_slot(engine);
        g_manager_slot.store(slot);
        spdlog::info("tracker: load/save manager slot {}", core::describe_address(slot));
        g_dump_classes = core::find_export_as<DumpClassesKeysFn>(engine, "?DumpClassesKeys@cObjManager@@QAEXPBD@Z");
        void* add = core::find_export(engine, "?AddEntity@cObjManager@@UAEHPAVcObjEntity@@@Z");
        g_add_hook = core::install_hook("cObjManager::AddEntity(cObjEntity*)", add, reinterpret_cast<void*>(&hk_AddEntity));
        if (HMODULE common = GetModuleHandleA("CommonDll7.dll"))
            g_item_state = core::find_export_as<GetStateFn>(common, "?GetCurrentState@cObjItemEx@@UAEPBDXZ");
        void* setcam = core::find_export(engine, "?SetCamera@cRender@@UAEXABVcRenderCamera@@@Z");
        g_setcam_hook = core::install_hook("cRender::SetCamera", setcam, reinterpret_cast<void*>(&hk_SetCamera));
    }

    void OnGameDllLoaded(HMODULE game) override { g_game_base.store(reinterpret_cast<uintptr_t>(game)); }

    // The teardown frees every entity: drop the pointers now, so no module (or the pre-render callback) reads or
    // writes freed animals between StopServer and the next scan (first-chance faults after a map change, 20:41).
    void OnMapUnloading() override {
        g_local = {};
        g_map_ready = false;
        g_animal_list.clear();
        g_npc_list.clear();
        g_animals = 0;
        cobra_ids_logged_.clear();
    }

    void OnMapLoaded(const char* bspName) override {
        map_ = bspName ? bspName : "";
        if (diff_hunter_) g_setcam_log = 40;
        g_local = {};
        g_map_ready = false;
        g_animal_list.clear();
        g_npc_list.clear();
        g_animals = 0;
        cobra_ids_logged_.clear();
        g_hunter_model.clear();
        histogram_logged_ = false;
        animal_log_seconds_ = 0;
        since_scan_ = 1.f;  // scan soon
        spdlog::info("tracker: map '{}' loaded, manager reset", map_);
    }

    void OnFrame(float dt) override {
        g_main_tid.store(GetCurrentThreadId());
        g_map_ready = g_local.valid;
        // Present closes the frame: publish this frame's world camera, arm for the next frame.
        if (g_frame_cam_taken) {
            g_view_cam = g_frame_cam;
            g_view_cam_valid = true;
        }
        g_frame_cam_taken = false;
        // The 1 s scan's position ran 2.5-5 m behind a seat in the MAP4A truck, and a pilot flies faster: keep the
        // sample's transform and view live (same entity, read every frame; the scan still decides which entity).
        if (g_local.valid && g_local.entity) {
            read_vec(g_local.entity, kEntPos, g_local.pos);
            read_vec(g_local.entity, kEntAngle, g_local.angle);
            read_camera(g_local);
        }
        PollActions();
        since_scan_ += dt;
        if (since_scan_ < 1.f) return;
        since_scan_ = 0.f;
        Scan();
    }

private:
    void Scan() {
        void* mgr = current_manager();
        const uintptr_t game = g_game_base.load();
        if (!mgr || !game) {
            if (!missing_logged_) {
                missing_logged_ = true;
                spdlog::info("tracker: waiting (manager={:#x} gamedll={:#x})", reinterpret_cast<uintptr_t>(mgr), game);
            }
            return;
        }

        int count = 0;
        void** arr = nullptr;
        if (!core::safe_read(reinterpret_cast<char*>(mgr) + kMgrCount, count) ||
            !core::safe_read(reinterpret_cast<char*>(mgr) + kMgrArray, arr) || !arr || count <= 0 ||
            count > kMgrMaxEntities) {
            return;
        }

        const uintptr_t vtHunter = game + kVtHunterRva;
        const uintptr_t vtPilot = game + kVtPilotRva;
        const uintptr_t vtAnimal = game + kVtAnimalRva;
        const uintptr_t vtCobra = game + kVtCobraRva;
        const uintptr_t vtCroc = game + kVtCrocodileRva;
        std::map<uintptr_t, int> hist;
        std::vector<void*> animals;
        std::vector<void*> npcs;
        std::vector<void*> children;  // candidates; filtered to the hunter's children below
        void* hunter = nullptr;
        bool pilot = false;

        for (int i = 0; i < count; ++i) {
            void* ent = nullptr;
            if (!core::safe_read(arr + i, ent) || !ent) continue;
            uintptr_t vt = 0;
            if (!core::safe_read(ent, vt) || !vt) continue;
            ++hist[vt];
            if ((vt == vtHunter || vt == vtPilot) && !hunter) {
                hunter = ent;
                pilot = vt == vtPilot;
            }
            children.push_back(ent);
            if (diff_hunter_) DiffEntityAngle(i, ent, vt);
            if (vt == vtAnimal || vt == vtCobra || vt == vtCroc) animals.push_back(ent);
            char* model = nullptr;
            if (core::safe_read(reinterpret_cast<char*>(ent) + kEntModelName, model) && model) {
                std::string path = core::safe_cstr(model, 160);
                for (char& c : path) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (path.find("/npc/") != std::string::npos) npcs.push_back(ent);
            }
        }
        g_animals = static_cast<int>(animals.size());
        g_animal_list = animals;
        g_npc_list = npcs;

        std::string cobra_ids;
        for (void* a : animals) {
            uintptr_t vt = 0;
            if (!core::safe_read(a, vt) || vt != vtCobra) continue;
            int16_t id = -1;
            core::safe_read(reinterpret_cast<char*>(a) + kEntId, id);
            if (!cobra_ids.empty()) cobra_ids += ',';
            cobra_ids += std::to_string(id);
        }
        if (!cobra_ids.empty() && cobra_ids != cobra_ids_logged_) {
            cobra_ids_logged_ = cobra_ids;
            spdlog::info("tracker: cobras at map load ids={}", cobra_ids);
        }

        if (!histogram_logged_) {
            histogram_logged_ = true;
            DumpClassesOnce(mgr);
            std::vector<std::pair<int, uintptr_t>> rows;
            for (auto& [vt, n] : hist) rows.emplace_back(n, vt);
            std::sort(rows.rbegin(), rows.rend());
            spdlog::info("tracker: manager {:#x} entities={} distinct vtables={} (map '{}') [hunter/animal vt {}/{}]",
                         reinterpret_cast<uintptr_t>(mgr), count, hist.size(), map_,
                         core::describe_address(reinterpret_cast<void*>(vtHunter)),
                         core::describe_address(reinterpret_cast<void*>(vtAnimal)));
            for (size_t r = 0; r < rows.size() && r < 40; ++r) {
                spdlog::info("tracker:   vt {} x{}", core::describe_address(reinterpret_cast<void*>(rows[r].second)),
                             rows[r].first);
            }
        }

        if (hunter) {
            LocalPlayerSample s;
            s.valid = true;
            s.entity = hunter;
            s.pilot = pilot;
            core::safe_read(reinterpret_cast<char*>(hunter) + kEntId, s.id);
            read_vec(hunter, kEntPos, s.pos);
            read_vec(hunter, kEntAngle, s.angle);
            read_camera(s);
            int16_t parent = 0;
            core::safe_read(reinterpret_cast<char*>(hunter) + kEntParent, parent);
            if (!g_local.valid) {
                const char* model = nullptr;
                core::safe_read(reinterpret_cast<char*>(hunter) + kEntModelName, model);
                g_hunter_model = model ? core::safe_cstr(model, 200) : "";
                spdlog::info("tracker: local {} entity {:#x} id={} parent={} model_name='{}'", pilot ? "pilot" : "hunter",
                             reinterpret_cast<uintptr_t>(hunter), s.id, parent, g_hunter_model);
            }
            g_local = s;
            items_.clear();
            for (void* e : children) {
                int16_t parent = -1;
                if (e != hunter && core::safe_read(static_cast<char*>(e) + kEntParent, parent) && parent == s.id)
                    items_.push_back(e);
            }
            if (diff_hunter_) DiffHunter(hunter);
            spdlog::info("tracker: hunter pos=({:.2f},{:.2f},{:.2f}) ang=({:.3f},{:.3f},{:.3f}) view {} yaw={:.2f} "
                         "pitch={:.2f} pos=({:.2f},{:.2f},{:.2f})",
                         s.pos.x, s.pos.y, s.pos.z, s.angle.x, s.angle.y, s.angle.z, s.viewValid, s.viewYaw,
                         s.viewPitch, s.viewPos.x, s.viewPos.y, s.viewPos.z);
        } else if (g_local.valid) {
            g_local = {};
            spdlog::info("tracker: local hunter lost");
        }

        // Animal samples for ~60 s per map: used to derive angle component order/units from motion.
        if (animal_log_seconds_ < 60) {
            ++animal_log_seconds_;
            for (void* a : animals) {
                int16_t id = -1;
                Vec3 p, ang;
                float hp = 0.f;
                core::safe_read(reinterpret_cast<char*>(a) + kEntId, id);
                read_vec(a, kEntPos, p);
                read_vec(a, kEntAngle, ang);
                core::safe_read(reinterpret_cast<char*>(a) + kEntHealth, hp);
                spdlog::info("tracker: animal id={} pos=({:.2f},{:.2f},{:.2f}) ang=({:.3f},{:.3f},{:.3f}) hp={:.1f}", id,
                             p.x, p.y, p.z, ang.x, ang.y, ang.z, hp);
            }
        }
    }

    // Diagnostic ([Tracker] DiffHunter=1): log every float in the hunter object (cScr_ModHunter, 0x838 bytes)
    // that changed since the last scan — used to find the view yaw/pitch fields.
    void DiffHunter(void* hunter) {
        constexpr size_t kSize = 0x838;
        std::vector<std::uint8_t> cur(kSize);
        if (!core::safe_copy(hunter, cur.data(), kSize)) return;
        if (diff_prev_.size() == kSize) {
            int lines = 0;
            for (size_t off = 0; off + 4 <= kSize && lines < 48; off += 4) {
                float a, b;
                std::memcpy(&a, &diff_prev_[off], 4);
                std::memcpy(&b, &cur[off], 4);
                if (!std::isfinite(a) || !std::isfinite(b) || std::fabs(a) > 1e5f || std::fabs(b) > 1e5f) continue;
                if (std::fabs(b - a) < 1e-4f || (std::fabs(a) < 1e-20f && std::fabs(b) < 1e-20f)) continue;
                spdlog::info("tracker: diff +{:#05x} {:.4f} -> {:.4f}", off, a, b);
                ++lines;
            }
        }
        diff_prev_ = std::move(cur);
    }

    // Diagnostic: entities whose +0xF4 angle changed since the last scan (finds the view-angle holder).
    void DiffEntityAngle(int index, void* ent, uintptr_t vt) {
        Vec3 a;
        if (!read_vec(ent, kEntAngle, a)) return;
        auto it = angle_prev_.find(ent);
        if (it != angle_prev_.end()) {
            const Vec3& b = it->second;
            if (std::fabs(a.x - b.x) > 1e-3f || std::fabs(a.y - b.y) > 1e-3f || std::fabs(a.z - b.z) > 1e-3f) {
                int16_t parent = -1;
                core::safe_read(reinterpret_cast<char*>(ent) + kEntParent, parent);
                spdlog::info("tracker: angdiff idx={} ent={:#x} vt={} parent={} ({:.3f},{:.3f},{:.3f}) -> ({:.3f},{:.3f},{:.3f})",
                             index, reinterpret_cast<uintptr_t>(ent), core::describe_address(reinterpret_cast<void*>(vt)),
                             parent, b.x, b.y, b.z, a.x, a.y, a.z);
            }
        }
        angle_prev_[ent] = a;
    }

    // Logs changes of crouch / current item / weapon state every frame (short events like fire would be missed
    // by the 1 s scan). Diagnostic for networking actions to puppets.
    void PollActions() {
        if (!g_local.valid || !g_local.entity) return;
        const char* h = static_cast<const char*>(g_local.entity);
        // Crouch from eye height (view camera y - hunter feet y), with hysteresis.
        Vec3 feet;
        if (g_view_cam_valid && read_vec(g_local.entity, kEntPos, feet)) {
            const float eye = g_view_cam.pos.y - feet.y;
            if (!g_act.crouched && eye < 1.40f) g_act.crouched = true;
            else if (g_act.crouched && eye > 1.50f) g_act.crouched = false;
        }
        int item = -1;
        core::safe_read(h + 0x3c0, item);
        if (g_act.crouched != act_crouched_ || item != act_item_) {
            spdlog::info("tracker: action crouched={} item={}", g_act.crouched, item);
            act_crouched_ = g_act.crouched;
            act_item_ = item;
        }
        // Active item (weapon in hand): first child item with activation state 1/2.
        bool found = false;
        for (void* it : items_) {
            int active = -1;
            core::safe_read(static_cast<char*>(it) + 0x1f0, active);
            if (active != 1 && active != 2) continue;
            found = true;
            const char* st = g_item_state ? guarded_item_state(it) : nullptr;
            const std::string state = st ? core::safe_cstr(st, 64) : "";
            const char* model = nullptr;
            core::safe_read(static_cast<char*>(it) + kEntModelName, model);
            const uint8_t kind = weapon_kind(model ? core::safe_cstr(model, 160).c_str() : nullptr);
            const bool firing = state == "Fire";
            if (firing && !g_act.firing) g_act.shots = static_cast<uint8_t>((g_act.shots + 1) & 0x0F);
            g_act.firing = firing;
            // WEAPON.SCR SecFire: the kick / butt swing (re/notes/knockdown.md section 4).
            const bool melee = state == "SecFire";
            if (melee && !g_act.meleeing) ++g_act.melees;
            g_act.meleeing = melee;
            g_act.reloading = state.rfind("Reload", 0) == 0;
            if (kind != g_act.weapon) {
                spdlog::info("tracker: action weapon kind {} model='{}'", kind, model ? core::safe_cstr(model, 160) : "");
                g_act.weapon = kind;
            }
            auto& prev = act_item_state_[it];
            if (prev != state && state != "Idle" && state != "Walk" && state != "Run") {
                spdlog::info("tracker: action weapon state '{}' shots={}", state, g_act.shots);
            }
            prev = state;
            break;
        }
        if (!found) {
            g_act.weapon = 0;
            g_act.firing = g_act.reloading = false;
        }
    }
    std::vector<void*> items_;
    bool act_crouched_ = false;
    int act_item_ = -2;
    std::map<void*, std::string> act_item_state_;
    std::map<void*, Vec3> angle_prev_;
    bool diff_hunter_ = GetPrivateProfileIntA("Tracker", "DiffHunter", 0,
                                              (core::self_dir() / "dh2coop.ini").string().c_str()) != 0;
    std::vector<std::uint8_t> diff_prev_;
    std::string map_;
    bool missing_logged_ = false;
    bool histogram_logged_ = false;
    int animal_log_seconds_ = 0;
    std::string cobra_ids_logged_;
    float since_scan_ = 0.f;
};

DH2_MODULE(PlayerTrackerModule)

}  // namespace

const LocalPlayerSample& LocalPlayer() { return g_local; }
void* EntityManager() { return current_manager(); }

void* EntityById(int id) {
    void* mgr = current_manager();
    int count = 0;
    void** arr = nullptr;
    if (!mgr || !core::safe_read(reinterpret_cast<char*>(mgr) + kMgrCount, count) ||
        !core::safe_read(reinterpret_cast<char*>(mgr) + kMgrArray, arr) || !arr || count <= 0 || count > kMgrMaxEntities)
        return nullptr;
    // cObjManager::AddEntity writes the array index into +0x04, so id == index (review M3); the scan is the fallback.
    if (id >= 0 && id < count) {
        void* ent = nullptr;
        int16_t eid = -1;
        if (core::safe_read(arr + id, ent) && ent && core::safe_read(static_cast<char*>(ent) + 0x04, eid) && eid == id)
            return ent;
    }
    for (int i = 0; i < count; ++i) {
        void* ent = nullptr;
        int16_t eid = -1;
        if (!core::safe_read(arr + i, ent) || !ent) continue;
        if (core::safe_read(static_cast<char*>(ent) + 0x04, eid) && eid == id) return ent;
    }
    return nullptr;
}
const char* HunterModelName() { return g_hunter_model.c_str(); }

bool SampleLocalNow(LocalPlayerSample& out) {
    if (!g_local.valid || !g_local.entity) return false;
    LocalPlayerSample s = g_local;
    if (!read_vec(s.entity, kEntPos, s.pos) || !read_vec(s.entity, kEntAngle, s.angle)) return false;
    read_camera(s);
    out = s;
    return true;
}
ViewCamera LastViewCamera() {
    ViewCamera v;
    if (!g_view_cam_valid) return v;
    v.valid = true;
    v.pos = g_view_cam.pos;
    v.x = g_view_cam.x;
    v.z = g_view_cam.z;
    v.fov = g_view_cam.fov;
    v.proj = g_view_cam.proj;
    // x × y = z with x right and z forward, so y = z × x (matches cRenderCamera Y at +0x94; see the one-shot log).
    v.y.x = v.z.y * v.x.z - v.z.z * v.x.y;
    v.y.y = v.z.z * v.x.x - v.z.x * v.x.z;
    v.y.z = v.z.x * v.x.y - v.z.y * v.x.x;
    return v;
}

int AnimalCount() { return g_animals; }
const std::vector<void*>& Animals() { return g_animal_list; }
const std::vector<void*>& Npcs() { return g_npc_list; }
void SetPreRenderCallback(void (*fn)()) { g_prerender = fn; }

}  // namespace dh2::coop
