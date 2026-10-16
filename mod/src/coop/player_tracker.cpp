#include "coop/player_tracker.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <map>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "core/exports.h"
#include "core/hook.h"
#include "core/module.h"
#include "core/safe_mem.h"

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
// GameDll7 vtable RVAs (Track B)
constexpr uintptr_t kVtHunterRva = 0x106ff0;  // cScr_ModHunter
constexpr uintptr_t kVtAnimalRva = 0xff470;   // cScrAnimal

// SetLoadSaveObjManager(cObjManager*) (EngineDll7 export, RVA 0xbf6a0) stores Main7's live manager in an
// unexported global (RVA 0x5826ec). Its body has `A3 <imm32>` (mov [global], eax) at +0x1A; we verify that
// instruction and read the global instead of hooking (cObjManager::Get is never called via the export).
constexpr uintptr_t kSetLoadSaveRva = 0xbf6a0;
constexpr uintptr_t kSetLoadSaveStoreOff = 0x1A;
constexpr uintptr_t kLoadSaveMgrRva = 0x5826ec;

std::atomic<void**> g_manager_slot{nullptr};
std::atomic<uintptr_t> g_game_base{0};

LocalPlayerSample g_local;
int g_animals = 0;

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
        void* add = core::find_export(engine, "?AddEntity@cObjManager@@UAEHPAVcObjEntity@@@Z");
        g_add_hook = core::install_hook("cObjManager::AddEntity(cObjEntity*)", add, reinterpret_cast<void*>(&hk_AddEntity));
    }

    void OnGameDllLoaded(HMODULE game) override { g_game_base.store(reinterpret_cast<uintptr_t>(game)); }

    void OnMapLoaded(const char* bspName) override {
        map_ = bspName ? bspName : "";
        g_local = {};
        histogram_logged_ = false;
        animal_log_seconds_ = 0;
        since_scan_ = 1.f;  // scan soon
        spdlog::info("tracker: map '{}' loaded, manager reset", map_);
    }

    void OnFrame(float dt) override {
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
        const uintptr_t vtAnimal = game + kVtAnimalRva;
        std::map<uintptr_t, int> hist;
        std::vector<void*> animals;
        void* hunter = nullptr;

        for (int i = 0; i < count; ++i) {
            void* ent = nullptr;
            if (!core::safe_read(arr + i, ent) || !ent) continue;
            uintptr_t vt = 0;
            if (!core::safe_read(ent, vt) || !vt) continue;
            ++hist[vt];
            if (vt == vtHunter && !hunter) hunter = ent;
            if (vt == vtAnimal) animals.push_back(ent);
        }
        g_animals = static_cast<int>(animals.size());

        if (!histogram_logged_) {
            histogram_logged_ = true;
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
            core::safe_read(reinterpret_cast<char*>(hunter) + kEntId, s.id);
            read_vec(hunter, kEntPos, s.pos);
            read_vec(hunter, kEntAngle, s.angle);
            int16_t parent = 0;
            core::safe_read(reinterpret_cast<char*>(hunter) + kEntParent, parent);
            if (!g_local.valid) {
                spdlog::info("tracker: local hunter entity {:#x} id={} parent={}", reinterpret_cast<uintptr_t>(hunter),
                             s.id, parent);
            }
            g_local = s;
            spdlog::info("tracker: hunter pos=({:.2f},{:.2f},{:.2f}) ang=({:.3f},{:.3f},{:.3f})", s.pos.x, s.pos.y,
                         s.pos.z, s.angle.x, s.angle.y, s.angle.z);
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

    std::string map_;
    bool missing_logged_ = false;
    bool histogram_logged_ = false;
    int animal_log_seconds_ = 0;
    float since_scan_ = 0.f;
};

DH2_MODULE(PlayerTrackerModule)

}  // namespace

const LocalPlayerSample& LocalPlayer() { return g_local; }

bool SampleLocalNow(LocalPlayerSample& out) {
    if (!g_local.valid || !g_local.entity) return false;
    LocalPlayerSample s = g_local;
    if (!read_vec(s.entity, kEntPos, s.pos) || !read_vec(s.entity, kEntAngle, s.angle)) return false;
    out = s;
    return true;
}
int AnimalCount() { return g_animals; }

}  // namespace dh2::coop
