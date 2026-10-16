#include "coop/puppets.h"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

#include <spdlog/spdlog.h>

#include "coop/player_tracker.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/exports.h"
#include "core/safe_mem.h"
#include "core/module.h"
#include "engine/entity_api.h"
#include "net/interp.h"

namespace dh2::coop::puppets {
namespace {

// Class for remote players: generic human (cObjHumanEx, "obj_humanEx"), no brain → no AI, no player logic.
// Overridable via [Puppets] Class= for experiments (e.g. obj_cschex).
constexpr const char* kDefaultClass = "obj_humanEx";
constexpr float kWalkSpeed = 0.6f;  // m/s: below = idle
constexpr float kRunSpeed = 5.5f;   // m/s: above = run (hunter walks ≈ 5 m/s)

struct Puppet {
    net::SnapshotBuffer buffer;
    void* entity = nullptr;
    bool spawn_failed = false;
    std::string anim;
    float rel_box[6] = {};  // world AABB at spawn time (entity at origin) = model-relative AABB
};

constexpr uintptr_t kEntWorldBox = 0xC4;  // float[6] min xyz, max xyz — used by cBspTree::UpdateEntity
constexpr uintptr_t kEntBspLink = 0x124;  // int, -1 = not linked into BSP leaves

void capture_rel_box(Puppet& p) {
    core::safe_copy(reinterpret_cast<char*>(p.entity) + kEntWorldBox, p.rel_box, sizeof p.rel_box);
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
bool g_physics = false;         // [Puppets] Physics=0: drop the puppet's physics body (see spawn_human)
float g_y_offset = 0.f;         // [Puppets] YOffset (metres) added to the networked hunter y
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
std::string g_model = "objects/npc/mountainguide/mountainguide.skin,game.ap";

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
    }
    g_spawn_manager = mgr;
    return ent;
}

// Locomotion clip from horizontal velocity relative to the facing yaw. Names are the ones every 332-clip NPC
// GAME.AP shares (re/notes/anim-names.md): standing idle is "idlenormalneutral" ("idle" is another clip);
// there is no run-back/strafe clip, the NPC scripts reuse the walk ones.
// Facing vector (sin yaw, cos yaw) is verified (hunter walk test); right = (cos yaw, -sin yaw) assumes the
// D3D left-handed frame (x right when facing +z) — UNVERIFIED, swap left/right if strafes look mirrored.
const char* choose_clip(float vx, float vz, float yawDeg) {
    const float speed = std::sqrt(vx * vx + vz * vz);
    if (speed < kWalkSpeed) return "idlenormalneutral";
    const float y = yawDeg * 3.14159265f / 180.f;
    const float fwd = vx * std::sin(y) + vz * std::cos(y);
    const float right = vx * std::cos(y) - vz * std::sin(y);
    if (fwd >= std::fabs(right)) return speed < kRunSpeed ? "walk/forward" : "run/forward";
    if (-fwd >= std::fabs(right)) return "walk/backward";
    return right > 0.f ? "walk/right" : "walk/left";
}

void pick_anim(Puppet& p, const char* want) {
    if (!g_anim_enabled || !p.entity) return;
    if (p.anim == want) return;
    p.anim = want;
    const bool ok = entity::SetAnim(p.entity, want);
    spdlog::info("puppets: entity {:#x} anim '{}' -> {}", reinterpret_cast<uintptr_t>(p.entity), want, ok);
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
            p.spawn_failed = false;
            p.anim.clear();
        }
        g_spawn_manager = nullptr;
        g_spike_entity = nullptr;
        g_spike_wait = 0.f;
    }

    void OnFrame(float dt) override {
        if (!g_enabled) return;
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
            place(p, {s.pos[0], s.pos[1] + g_y_offset, s.pos[2]}, s.yaw);
            pick_anim(p, choose_clip(s.velocity[0], s.velocity[2], s.yaw));
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
        const float hy = local.angle.y * 3.14159265f / 180.f;
        const float sgn = static_cast<float>(spike_sign_);
        const float fx = sgn * std::sin(hy), fz = sgn * std::cos(hy);
        const float side = std::sin(g_spike_t * 0.8f);
        const entity::Vec3 pos{local.pos.x + 3.f * fx + side * fz, local.pos.y, local.pos.z + 3.f * fz - side * fx};
        const float yawDeg = spike_sign_ < 0 ? local.angle.y : local.angle.y + 180.f;  // face the camera
        place(spike_puppet_, pos, yawDeg);
        pick_anim(spike_puppet_, "walk/forward");
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
    if (it->second.entity && g_spawn_manager) {
        const bool ok = entity::RemoveEntity(g_spawn_manager, it->second.entity);
        spdlog::info("puppets: removed puppet for player {} -> {}", playerId, ok);
    }
    g_puppets.erase(it);
}

void RemoveAll() {
    for (auto& [id, p] : g_puppets) {
        if (p.entity && g_spawn_manager) entity::RemoveEntity(g_spawn_manager, p.entity);
    }
    g_puppets.clear();
}

int Count() {
    int n = 0;
    for (auto& [id, p] : g_puppets) n += p.entity ? 1 : 0;
    return n;
}

}  // namespace dh2::coop::puppets
