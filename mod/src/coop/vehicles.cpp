#include "coop/vehicles.h"

#include <windows.h>

#include <chrono>
#include <cmath>
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

namespace dh2::coop::vehicles {
namespace {

// CommonDll7 cObjVehicle: +0x118 cPhysVehicle (a cPhysicsObject), +0x33c bit 8 = driving to a waypoint (Update calls
// PhysDrive while set). cPhysicsObject +4 = rigid body: +0x04 m3x3 orientation (rows), +0x28 position.
// re/notes: decompiles of cObjVehicle::Update 0x5a9c0, Freeze 0x53d20, cPhysicsObject::SetPosition / SetOrientation.
constexpr uintptr_t kVehPhys = 0x118;
constexpr uintptr_t kVehDriveFlags = 0x33c;
constexpr uint8_t kVehDriving = 8;
constexpr uintptr_t kBodyRot = 0x04;
constexpr uintptr_t kBodyPos = 0x28;
constexpr uintptr_t kBodyLinVel = 0x34;  // Freeze zeroes +0x34 and +0x40 (linear / angular velocity)
constexpr uintptr_t kBodyAngVel = 0x40;
constexpr uintptr_t kPhysWheels = 0xcc;      // cPhysVehicle: wheel object array (each +4 = its body)
constexpr uintptr_t kPhysWheelCount = 0xe8;  // as cPhysVehicle::SetPosition walks them
constexpr uint32_t kSendMs = 33;     // ~30 Hz
constexpr uint32_t kInterpMs = 100;  // clients draw the host's pose of 100 ms ago (two to three samples of slack)
constexpr uint32_t kMaxExtrapMs = 150;
constexpr int kPoses = 16;

using VehUpdateFn = void(__thiscall*)(void* veh, float dt);
using VehFreezeFn = void(__thiscall*)(void* veh);
using PhysSetVecFn = void(__thiscall*)(void* phys, const float* v);

safetyhook::InlineHook* g_update = nullptr;
VehFreezeFn g_freeze = nullptr;
PhysSetVecFn g_set_pos = nullptr;  // cPhysicsObject::SetPosition (the base: cPhysVehicle's override shifts the wheels)
PhysSetVecFn g_set_rot = nullptr;  // cPhysicsObject::SetOrientation(m3x3)
bool g_enabled = true;

struct Pose {
    uint32_t tick = 0;  // the host's clock when it sampled the pose
    float rot[9] = {};
    float pos[3] = {};
};

// Interpolation runs on the host's clock: a packet's arrival time jitters with frames and bursts, which snapped the
// truck (it looked like a low frame rate). offset = local - host, the smallest seen (the least delayed packet).
struct Track {
    void* ent = nullptr;
    Pose poses[kPoses];  // oldest .. newest, `samples` of them
    int samples = 0;
    int32_t offset = 0;
    bool have_offset = false;
    bool frozen = false;
    bool logged_update = false;
    // Smoothness diagnostics (client): per-frame step of the applied position, logged every 2 s for a minute.
    float last_applied[3] = {};
    bool have_applied = false;
    float step_min = 1e9f, step_max = 0.f, step_sum = 0.f;
    int steps = 0;
    uint32_t stats_since = 0;
    int stats_logged = 0;
};

std::map<uint16_t, Track> g_tracks;
uint32_t g_last_send = 0;

uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

int16_t id_of(void* ent) {
    int16_t id = -1;
    core::safe_read(static_cast<char*>(ent) + 0x04, id);
    return id;
}

char* body_of(void* veh) {
    char* phys = nullptr;
    char* body = nullptr;
    if (!core::safe_read(static_cast<char*>(veh) + kVehPhys, phys) || !phys) return nullptr;
    if (!core::safe_read(phys + 4, body)) return nullptr;
    return body;
}

// Rows of a rotation after a blend: Gram-Schmidt keeps them orthonormal.
void orthonormalize(float* m) {
    auto norm = [](float* v) {
        const float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (l > 1e-6f) {
            v[0] /= l;
            v[1] /= l;
            v[2] /= l;
        }
    };
    float* a = m;
    float* b = m + 3;
    float* c = m + 6;
    norm(a);
    const float d = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    for (int i = 0; i < 3; ++i) b[i] -= d * a[i];
    norm(b);
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
}

Pose blend(const Track& t, uint32_t now) {
    const Pose& newest = t.poses[t.samples - 1];
    if (t.samples < 2) return newest;
    const uint32_t render = now - static_cast<uint32_t>(t.offset) - kInterpMs;  // on the host's clock
    // The pair around `render`; past the newest, the last pair extrapolated a little.
    int i = t.samples - 2;
    for (int k = 0; k + 1 < t.samples; ++k) {
        if (static_cast<int32_t>(render - t.poses[k + 1].tick) <= 0) {
            i = k;
            break;
        }
    }
    const Pose& a0 = t.poses[i];
    const Pose& a1 = t.poses[i + 1];
    const int32_t span = static_cast<int32_t>(a1.tick - a0.tick);
    if (span <= 0) return a1;
    float a = static_cast<float>(static_cast<int32_t>(render - a0.tick)) / static_cast<float>(span);
    if (a < 0.f) a = 0.f;
    const float max_a = 1.f + static_cast<float>(kMaxExtrapMs) / static_cast<float>(span);
    if (a > max_a) a = max_a;
    Pose p;
    p.tick = render;
    for (int k = 0; k < 3; ++k) p.pos[k] = a0.pos[k] + (a1.pos[k] - a0.pos[k]) * a;
    for (int k = 0; k < 9; ++k) p.rot[k] = a0.rot[k] + (a1.rot[k] - a0.rot[k]) * a;
    float third[3] = {p.rot[6], p.rot[7], p.rot[8]};
    orthonormalize(p.rot);
    // Gram-Schmidt rebuilt the third row as a x b; flip it if the blended one pointed the other way.
    if (third[0] * p.rot[6] + third[1] * p.rot[7] + third[2] * p.rot[8] < 0.f) {
        for (int i = 6; i < 9; ++i) p.rot[i] = -p.rot[i];
    }
    return p;
}

// out = a * b (row-major 3x3).
void mul3(const float* a, const float* b, float* out) {
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            out[r * 3 + c] = a[r * 3] * b[c] + a[r * 3 + 1] * b[3 + c] + a[r * 3 + 2] * b[6 + c];
}

// The wheels are bodies of their own, held by the suspension. Moving only the chassis left them on the road behind
// it, stretched into long rods (MAP4A client, 2026-10-18 00:3x). Carry each one rigidly with the chassis: the same
// rotation change about the old chassis position, then the new position; no velocity.
void carry_wheels(char* phys, const float* old_rot, const float* old_pos, const Pose& p, bool log) {
    uint32_t count = 0;
    char* wheels = nullptr;
    if (!core::safe_read(phys + kPhysWheelCount, count) || !core::safe_read(phys + kPhysWheels, wheels) || !wheels ||
        count > 8)
        return;
    // delta = R_new^T-free form: rows are the body axes, so a world offset w maps as w' = (w * old^T) * new.
    float old_t[9];
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) old_t[r * 3 + c] = old_rot[c * 3 + r];
    float delta[9];
    mul3(old_t, p.rot, delta);
    for (uint32_t i = 0; i < count; ++i) {
        char* wheel = nullptr;
        char* body = nullptr;
        if (!core::safe_read(wheels + i * 4, wheel) || !wheel || !core::safe_read(wheel + 4, body) || !body) continue;
        float pos[3], rot[9];
        if (!core::safe_copy(body + kBodyPos, pos, sizeof pos) || !core::safe_copy(body + kBodyRot, rot, sizeof rot))
            continue;
        const float rel[3] = {pos[0] - old_pos[0], pos[1] - old_pos[1], pos[2] - old_pos[2]};
        float npos[3];
        for (int c = 0; c < 3; ++c)
            npos[c] = p.pos[c] + rel[0] * delta[c] + rel[1] * delta[3 + c] + rel[2] * delta[6 + c];
        float nrot[9];
        mul3(rot, delta, nrot);
        std::memcpy(body + kBodyPos, npos, sizeof npos);
        std::memcpy(body + kBodyRot, nrot, sizeof nrot);
        const float zero[6] = {};
        std::memcpy(body + kBodyLinVel, zero, 12);
        std::memcpy(body + kBodyAngVel, zero, 12);
        if (log)
            spdlog::info("vehicles: wheel {} offset ({:.2f},{:.2f},{:.2f}) from the chassis", i, rel[0], rel[1], rel[2]);
    }
}

// Client: put our copy where the host's is. Freeze once (zero velocity, no gravity), stop it driving itself, then
// write the chassis pose; the original Update after this carries the model, the riders and the AABB along.
void follow(Track& t, void* veh, uint32_t now) {
    char* phys = nullptr;
    if (!core::safe_read(static_cast<char*>(veh) + kVehPhys, phys) || !phys || !g_set_pos || !g_set_rot) return;
    const bool first = !t.frozen;
    if (!t.frozen && g_freeze) {
        g_freeze(veh);
        t.frozen = true;
        spdlog::info("vehicles: id={} follows the host's truck (physics frozen)", id_of(veh));
    }
    uint8_t flags = 0;
    if (core::safe_read(static_cast<char*>(veh) + kVehDriveFlags, flags) && (flags & kVehDriving))
        *(reinterpret_cast<uint8_t*>(veh) + kVehDriveFlags) = static_cast<uint8_t>(flags & ~kVehDriving);
    char* body = nullptr;
    float old_rot[9], old_pos[3];
    const bool have_old = core::safe_read(phys + 4, body) && body &&
                          core::safe_copy(body + kBodyRot, old_rot, sizeof old_rot) &&
                          core::safe_copy(body + kBodyPos, old_pos, sizeof old_pos);
    const Pose p = blend(t, now);
    g_set_rot(phys, p.rot);
    g_set_pos(phys, p.pos);
    if (t.have_applied && t.stats_logged < 30) {
        const float dx = p.pos[0] - t.last_applied[0], dy = p.pos[1] - t.last_applied[1],
                    dz = p.pos[2] - t.last_applied[2];
        const float step = std::sqrt(dx * dx + dy * dy + dz * dz);
        t.step_min = step < t.step_min ? step : t.step_min;
        t.step_max = step > t.step_max ? step : t.step_max;
        t.step_sum += step;
        ++t.steps;
        if (t.stats_since == 0) t.stats_since = now;
        if (now - t.stats_since >= 2000 && t.steps > 0) {
            spdlog::info("vehicles: id={} frame steps over {} frames: min {:.3f} avg {:.3f} max {:.3f} m", id_of(veh),
                         t.steps, t.step_min, t.step_sum / static_cast<float>(t.steps), t.step_max);
            t.step_min = 1e9f;
            t.step_max = t.step_sum = 0.f;
            t.steps = 0;
            t.stats_since = now;
            ++t.stats_logged;
        }
    }
    std::memcpy(t.last_applied, p.pos, sizeof p.pos);
    t.have_applied = true;
    if (have_old) {
        carry_wheels(phys, old_rot, old_pos, p, first);
        const float zero[3] = {};
        std::memcpy(body + kBodyLinVel, zero, sizeof zero);
        std::memcpy(body + kBodyAngVel, zero, sizeof zero);
    }
}

void __fastcall hk_Update(void* self, void*, float dt) {
    const int16_t id = id_of(self);
    if (id >= 0) {
        Track& t = g_tracks[static_cast<uint16_t>(id)];
        t.ent = self;
        if (!t.logged_update) {
            t.logged_update = true;
            spdlog::info("vehicles: vehicle id={} entity {:#x} updates (role {})", id, reinterpret_cast<uintptr_t>(self),
                         NetIsHost() ? "host" : (NetIsOnline() ? "client" : "offline"));
        }
        if (g_enabled && NetIsOnline() && !NetIsHost() && t.samples > 0 && events::SameMapAsHost()) {
            follow(t, self, now_ms());
        }
    }
    g_update->unsafe_thiscall<void>(self, dt);
}

class VehiclesModule : public dh2::Module {
public:
    const char* Name() const override { return "vehicles"; }
    int Priority() const override { return 46; }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        g_enabled = GetPrivateProfileIntA("Coop", "VehicleSync", 1, ini.c_str()) != 0;
        spdlog::info("vehicles: VehicleSync={}", g_enabled ? 1 : 0);
    }

    void OnEngineReady(HMODULE engine) override {
        g_set_pos = reinterpret_cast<PhysSetVecFn>(
            core::find_export(engine, "?SetPosition@cPhysicsObject@@UAEXABUtVect3@@@Z"));
        g_set_rot = reinterpret_cast<PhysSetVecFn>(
            core::find_export(engine, "?SetOrientation@cPhysicsObject@@UAEXABUm3x3@@@Z"));
    }

    void OnGameDllLoaded(HMODULE) override {
        HMODULE common = GetModuleHandleA("CommonDll7.dll");
        if (!common) {
            spdlog::error("vehicles: CommonDll7.dll not loaded; vehicle sync off");
            return;
        }
        g_freeze = reinterpret_cast<VehFreezeFn>(core::find_export(common, "?Freeze@cObjVehicle@@UAEXXZ"));
        g_update = core::install_hook("cObjVehicle::Update", core::find_export(common, "?Update@cObjVehicle@@UAEXM@Z"),
                                      reinterpret_cast<void*>(&hk_Update));
        spdlog::info("vehicles: freeze={} setpos={} setrot={}", g_freeze != nullptr, g_set_pos != nullptr,
                     g_set_rot != nullptr);
    }

    void OnMapUnloading() override { g_tracks.clear(); }
    void OnMapLoaded(const char*) override { g_tracks.clear(); }

    // Ride diagnostics (both roles): how far each rider, puppet and NPC is from the chassis we ride, every 2 s.
    void LogRide(uint32_t now) {
        if (ride_logs_ >= 30 || now - last_ride_log_ < 2000) return;
        const auto& local = LocalPlayer();
        int16_t parent = -1;
        if (!local.entity || !core::safe_read(static_cast<char*>(local.entity) + 0xA4, parent) || parent < 0) return;
        void* veh = EntityById(parent);
        char* body = veh ? body_of(veh) : nullptr;
        float c[3] = {};
        if (!body || !core::safe_copy(body + kBodyPos, c, sizeof c)) return;
        last_ride_log_ = now;
        ++ride_logs_;
        auto dist = [&](void* e, int16_t* par) {
            float p[3] = {};
            if (!e || !core::safe_copy(static_cast<char*>(e) + 0xE8, p, sizeof p)) return -1.f;
            if (par) core::safe_read(static_cast<char*>(e) + 0xA4, *par);
            const float dx = p[0] - c[0], dy = p[1] - c[1], dz = p[2] - c[2];
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        };
        std::string line = fmt::format("vehicles: ride id={} hunter {:.2f} m", parent, dist(local.entity, nullptr));
        for (uint8_t id = 0; id < 4; ++id) {
            if (void* pe = puppets::EntityOf(id)) line += fmt::format(", puppet {} {:.2f} m", id, dist(pe, nullptr));
        }
        for (void* n : Npcs()) {
            int16_t par = -1;
            const float d = dist(n, &par);
            if (d >= 0.f && d < 40.f) line += fmt::format(", npc {} {:.2f} m (parent {})", id_of(n), d, par);
        }
        spdlog::info("{}", line);
    }

    // Host: the chassis pose of every vehicle that updated this map, 30 times a second.
    void OnFrame(float) override {
        if (g_enabled && NetIsOnline()) LogRide(now_ms());
        if (!g_enabled || !NetIsHost() || g_tracks.empty()) return;
        const uint32_t now = now_ms();
        if (now - g_last_send < kSendMs) return;
        g_last_send = now;
        net::VehicleStates msg;
        msg.tickMs = now;
        msg.mapSerial = events::LocalMapSerial();
        for (auto& [id, t] : g_tracks) {
            if (!t.ent || EntityById(id) != t.ent) continue;
            char* body = body_of(t.ent);
            if (!body) continue;
            net::VehicleState v;
            v.id = id;
            if (!core::safe_copy(body + kBodyRot, v.rot, sizeof v.rot) ||
                !core::safe_copy(body + kBodyPos, v.pos, sizeof v.pos))
                continue;
            msg.vehicles.push_back(v);
            if (msg.vehicles.size() >= net::kMaxVehicles) break;
        }
        if (!msg.vehicles.empty()) BroadcastVehicleStates(msg);
    }

private:
    uint32_t last_ride_log_ = 0;
    int ride_logs_ = 0;
};

DH2_MODULE(VehiclesModule)

}  // namespace

void OnStates(const net::VehicleStates& states, uint32_t localMs) {
    for (const net::VehicleState& v : states.vehicles) {
        Track& t = g_tracks[v.id];
        // Unreliable channel: drop a packet older than the newest we have.
        if (t.samples > 0 && static_cast<int32_t>(states.tickMs - t.poses[t.samples - 1].tick) <= 0) continue;
        const int32_t offset = static_cast<int32_t>(localMs - states.tickMs);
        if (!t.have_offset || offset < t.offset) t.offset = offset;
        t.have_offset = true;
        if (t.samples == kPoses) {
            for (int k = 1; k < kPoses; ++k) t.poses[k - 1] = t.poses[k];
            --t.samples;
        }
        Pose& p = t.poses[t.samples++];
        p.tick = states.tickMs;
        std::memcpy(p.rot, v.rot, sizeof v.rot);
        std::memcpy(p.pos, v.pos, sizeof v.pos);
    }
}

}  // namespace dh2::coop::vehicles
