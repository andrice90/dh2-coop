// Phase 4 (shared animals) — groundwork and experiment.
// [Animals] LogStates=1: log every animal's body-state name changes (what the host will broadcast).
// [Animals] Spike=1: take local control of one deer the way a client will: FreezeBrain + UnlinkPhysics, then drive
// it with SetPos/SetAngle (circle 8 m in front of the hunter) and cycle its body state through names seen on the
// free animals. Answers: does a frozen, physics-less animal stay where we put it, render, and animate by state name?
#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "coop/player_tracker.h"
#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/entity_api.h"

namespace dh2::coop::animals {
namespace {

constexpr uintptr_t kEntId = 0x04;
constexpr uintptr_t kEntPos = 0xE8;
constexpr uintptr_t kEntWorldBox = 0xC4;
constexpr uintptr_t kEntHealth = 0x19C;

class AnimalsModule;
extern AnimalsModule* g_self;
void prerender();

// Spike diagnostic: was cObjEntity::Draw called for the controlled deer? (culling investigation)
safetyhook::InlineHook* g_draw_hook = nullptr;
std::atomic<void*> g_watch{nullptr};
std::atomic<uint32_t> g_draws{0};
void __fastcall hk_Draw(void* self, void* /*edx*/) {
    if (self == g_watch.load(std::memory_order_relaxed)) g_draws.fetch_add(1, std::memory_order_relaxed);
    g_draw_hook->unsafe_thiscall<void>(self);
}

bool g_log_states = true;
bool g_spike = false;
bool g_spike_unlink = true;   // [Animals] SpikeUnlink
float g_spike_y = 0.7f;       // [Animals] SpikeYOffset (m above hunter feet; HUNTMAP4 ground ahead is ~+0.7)

std::string ini_path() { return (core::self_dir() / "dh2coop.ini").string(); }

class AnimalsModule : public dh2::Module {
public:
    const char* Name() const override { return "animals"; }
    int Priority() const override { return 46; }

    void OnInit() override {
        g_log_states = GetPrivateProfileIntA("Animals", "LogStates", 1, ini_path().c_str()) != 0;
        g_spike = GetPrivateProfileIntA("Animals", "Spike", 0, ini_path().c_str()) != 0;
        g_spike_unlink = GetPrivateProfileIntA("Animals", "SpikeUnlink", 1, ini_path().c_str()) != 0;
        char y[32] = {};
        GetPrivateProfileStringA("Animals", "SpikeYOffset", "0.7", y, sizeof y, ini_path().c_str());
        g_spike_y = static_cast<float>(std::atof(y));
        spdlog::info("animals: logStates={} spike={}", g_log_states, g_spike);
        g_self = this;
        SetPreRenderCallback(&prerender);
    }

    void OnEngineReady(HMODULE engine) override {
        if (g_spike) {
            void* draw = core::find_export(engine, "?Draw@cObjEntity@@UAEXXZ");
            g_draw_hook = core::install_hook("cObjEntity::Draw (animal diag)", draw, reinterpret_cast<void*>(&hk_Draw));
        }
    }

    void OnMapLoaded(const char*) override {
        states_.clear();
        seen_.clear();
        spike_ = nullptr;
        wait_ = 0.f;
        t_ = 0.f;
    }

    void OnFrame(float dt) override {
        const auto& local = LocalPlayer();
        if (!local.valid) return;
        const auto& animals = Animals();
        if (g_log_states) LogStates(animals);
        if (g_spike) Spike(dt, local, animals);
    }

private:
    void LogStates(const std::vector<void*>& animals) {
        for (void* a : animals) {
            if (a == spike_) continue;
            const std::string st = entity::BodyStateName(a);
            auto& prev = states_[a];
            if (st == prev) continue;
            prev = st;
            if (!st.empty()) seen_.insert(st);
            int16_t id = -1;
            Vec3 p;
            float hp = 0.f;
            core::safe_read(static_cast<char*>(a) + kEntId, id);
            core::safe_read(static_cast<char*>(a) + kEntPos, p);
            core::safe_read(static_cast<char*>(a) + kEntHealth, hp);
            spdlog::info("animals: id={} state='{}' pos=({:.1f},{:.1f},{:.1f}) hp={:.0f}", id, st, p.x, p.y, p.z, hp);
        }
    }

    void Spike(float dt, const LocalPlayerSample& local, const std::vector<void*>& animals) {
        if (!spike_) {
            wait_ += dt;
            if (wait_ < 8.f) return;  // let the hunt start (CHOOSE ITEM menu) and animals settle
            for (void* a : animals) {  // first live, placed deer (skip the parked puma at -10000)
                Vec3 p;
                float hp = 0.f;
                core::safe_read(static_cast<char*>(a) + kEntPos, p);
                core::safe_read(static_cast<char*>(a) + kEntHealth, hp);
                if (p.y > -1000.f && hp > 0.f && std::fabs(hp - 45.f) < 1.f) {
                    spike_ = a;
                    break;
                }
            }
            if (!spike_) return;
            const bool frozen = entity::FreezeBrain(spike_, true);
            const bool unlinked = g_spike_unlink && entity::UnlinkPhysics(spike_);
            float box[6] = {};
            Vec3 p;
            core::safe_copy(static_cast<char*>(spike_) + kEntWorldBox, box, sizeof box);
            core::safe_read(static_cast<char*>(spike_) + kEntPos, p);
            for (int i = 0; i < 3; ++i) {
                rel_[i] = box[i] - (&p.x)[i];
                rel_[i + 3] = box[i + 3] - (&p.x)[i];
            }
            int16_t id = -1;
            core::safe_read(static_cast<char*>(spike_) + kEntId, id);
            spdlog::info("animals: spike took id={} frozen={} unlinked={} state='{}' box rel ({:.2f},{:.2f},{:.2f})-({:.2f},{:.2f},{:.2f})",
                         id, frozen, unlinked, entity::BodyStateName(spike_), rel_[0], rel_[1], rel_[2], rel_[3],
                         rel_[4], rel_[5]);
        }
        t_ += dt;
        const float drift = drift_;
        const entity::Vec3 pos = pos_;
        // Every 4 s switch to the next state name seen on the free animals.
        state_t_ += dt;
        if (state_t_ >= 4.f && !seen_.empty()) {
            state_t_ = 0.f;
            std::vector<std::string> names(seen_.begin(), seen_.end());
            const std::string& want = names[state_idx_++ % names.size()];
            const int r = entity::SetBodyState(spike_, want.c_str(), false);
            spdlog::info("animals: spike set state '{}' -> {} now '{}' drift={:.3f} pos=({:.1f},{:.1f},{:.1f})", want, r,
                         entity::BodyStateName(spike_), drift, pos.x, pos.y, pos.z);
        }
    }

public:
    // Pre-render (after sim/physics, before drawing): put the controlled deer where we want it.
    void Place() {
        if (!spike_) return;
        const auto& local = LocalPlayer();
        if (!local.valid) return;
        // Check last frame's placement survived the sim step (physics would snap it back).
        Vec3 now;
        core::safe_read(static_cast<char*>(spike_) + kEntPos, now);
        const float drift = std::fabs(now.x - last_.x) + std::fabs(now.z - last_.z);
        // Circle of radius 3 m around a point 8 m ahead of the hunter's view.
        const float vy = (local.viewValid ? local.viewYaw : local.angle.y) * 3.14159265f / 180.f;
        const float cx = local.pos.x + 8.f * std::sin(vy), cz = local.pos.z + 8.f * std::cos(vy);
        const float a = t_ * 0.5f;
        const entity::Vec3 pos{cx + 3.f * std::sin(a), local.pos.y + g_spike_y, cz + 3.f * std::cos(a)};
        const float heading = (a + 1.5707963f) * 57.2957795f;  // tangent of the circle (yaw = atan2(dx, dz))
        if (g_spike_unlink) {
            entity::SetPos(spike_, pos);
            entity::SetAngle(spike_, {0.f, heading, 0.f});
        } else {
            // Map-load path (cObjEntity::HandleKey, EngineDll7 0xbb660): "position" (cm, x0.01) = SetPos + phys
            // vt +0xa8; "angle" (degrees) = SetAngle + phys vt +0xb0 (matrix). Moves the physics body with it.
            char buf[96];
            std::snprintf(buf, sizeof buf, "%.2f %.2f %.2f", pos.x * 100.f, pos.y * 100.f, pos.z * 100.f);
            entity::HandleKey(spike_, "position", buf);
            std::snprintf(buf, sizeof buf, "0 %.2f 0", heading);
            entity::HandleKey(spike_, "angle", buf);
        }
        float* box = reinterpret_cast<float*>(static_cast<char*>(spike_) + kEntWorldBox);
        box[0] = pos.x + rel_[0]; box[1] = pos.y + rel_[1]; box[2] = pos.z + rel_[2];
        box[3] = pos.x + rel_[3]; box[4] = pos.y + rel_[4]; box[5] = pos.z + rel_[5];
        entity::BspUpdate(spike_);
        last_ = {pos.x, pos.y, pos.z};
        g_watch.store(spike_);
        diag_t_ += 0.01f;
        if (++diag_frames_ >= 50) {  // ~0.5 s at 100 fps
            int link = -2;
            core::safe_read(static_cast<char*>(spike_) + 0x124, link);
            float clock = std::fmod(a * 57.2957795f, 360.f);  // circle angle: 0 = far point (12 o'clock)
            spdlog::info("animals: diag circle={:.0f}deg draws/50f={} bsplink={} pos=({:.1f},{:.1f},{:.1f})", clock,
                         g_draws.exchange(0), link, pos.x, pos.y, pos.z);
            diag_frames_ = 0;
        }
        drift_ = drift;
        pos_ = pos;
    }

private:
    std::map<void*, std::string> states_;
    std::set<std::string> seen_;
    void* spike_ = nullptr;
    float wait_ = 0.f, t_ = 0.f, state_t_ = 0.f;
    size_t state_idx_ = 0;
    float rel_[6] = {};
    Vec3 last_;
    float drift_ = 0.f;
    float diag_t_ = 0.f;
    int diag_frames_ = 0;
    entity::Vec3 pos_;
};

AnimalsModule* g_self = nullptr;
void prerender() {
    if (g_self) g_self->Place();
}

DH2_MODULE(AnimalsModule)

}  // namespace
}  // namespace dh2::coop::animals
