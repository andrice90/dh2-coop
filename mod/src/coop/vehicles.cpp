#include "coop/vehicles.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
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
    int refrozen = 0;          // client: times something gave the copy its gravity back (DriveTo calls UnFreeze)
    int uprighted = 0;         // host: times the rollover guard stood the chassis up
    int seats[4] = {-1, -1, -1, -1};  // host: last seat occupants seen (+0x298), for kVehicleSeat
    bool seats_known = false;
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
    // cObjVehicle::DriveTo ends in vt +0x228 = UnFreeze (gravity back from +0x344). hk_DriveTo keeps a following
    // copy from driving, but anything else that thaws it gets frozen again here.
    if (t.frozen && g_freeze) {
        char* b = nullptr;
        uint32_t gravity = 0;
        if (core::safe_read(phys + 4, b) && b && core::safe_read(b + 0x110, gravity) && gravity != 0) {
            g_freeze(veh);
            if (t.refrozen++ < 5) spdlog::info("vehicles: id={} had its gravity back (unfrozen); frozen again", id_of(veh));
        }
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

// ---- rollover guard (host and offline) ----
// MAP4A's truck rolls over on the ramp after PathNode23 in about one ride in six (host-only runs too, 2 of 6, so not
// the puppets): roll 46, 136, 180 degrees within two seconds, then it drives on its roof with every rider's camera
// upside down. Normal rides stay under 27 degrees of roll. When the chassis' up axis points more than 70 degrees away
// from vertical, stand it up again: same heading, same spot (+0.3 m), no spin, wheels carried along. Clients follow
// the host's pose, so they see the same recovery.
constexpr float kUprightBelow = 0.34f;  // chassis up.y (rotation row 1): cos(70 degrees)
bool g_upright = false;  // [Vehicles] Upright (default off: forced-flip test righted it, then the truck went erratic)

void cross3(const float* a, const float* b, float* out) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

void make_upright(Track& t, void* veh) {
    char* phys = nullptr;
    char* body = nullptr;
    float rot[9], pos[3];
    if (!g_set_pos || !g_set_rot || !core::safe_read(static_cast<char*>(veh) + kVehPhys, phys) || !phys ||
        !core::safe_read(phys + 4, body) || !body || !core::safe_copy(body + kBodyRot, rot, sizeof rot) ||
        !core::safe_copy(body + kBodyPos, pos, sizeof pos))
        return;
    if (rot[4] >= kUprightBelow) return;
    // A rolled truck keeps its forward axis horizontal and loses its up axis: keep the more horizontal of rows 0 / 2.
    const float h0 = std::sqrt(rot[0] * rot[0] + rot[2] * rot[2]);
    const float h2 = std::sqrt(rot[6] * rot[6] + rot[8] * rot[8]);
    if (std::max(h0, h2) < 0.2f) return;  // nose or tail straight up: no heading to keep
    const float det = rot[0] * (rot[4] * rot[8] - rot[5] * rot[7]) - rot[1] * (rot[3] * rot[8] - rot[5] * rot[6]) +
                      rot[2] * (rot[3] * rot[7] - rot[4] * rot[6]);
    const float s = det < 0.f ? -1.f : 1.f;  // keep the matrix' handedness
    const float up[3] = {0.f, 1.f, 0.f};
    float nrot[9], tmp[3];
    if (h0 >= h2) {
        const float a[3] = {rot[0] / h0, 0.f, rot[2] / h0};
        cross3(a, up, tmp);
        std::memcpy(nrot, a, sizeof a);
        std::memcpy(nrot + 3, up, sizeof up);
        for (int k = 0; k < 3; ++k) nrot[6 + k] = s * tmp[k];
    } else {
        const float c[3] = {rot[6] / h2, 0.f, rot[8] / h2};
        cross3(up, c, tmp);
        for (int k = 0; k < 3; ++k) nrot[k] = s * tmp[k];
        std::memcpy(nrot + 3, up, sizeof up);
        std::memcpy(nrot + 6, c, sizeof c);
    }
    Pose p;
    std::memcpy(p.rot, nrot, sizeof nrot);
    std::memcpy(p.pos, pos, sizeof pos);
    p.pos[1] += 0.3f;
    g_set_rot(phys, p.rot);
    g_set_pos(phys, p.pos);
    const float zero[3] = {};
    std::memcpy(body + kBodyAngVel, zero, sizeof zero);
    carry_wheels(phys, rot, pos, p, false);
    if (t.uprighted++ < 5)
        spdlog::info("vehicles: id={} rolled over (up.y {:.2f}): stood upright at ({:.1f},{:.1f},{:.1f}) #{}", id_of(veh),
                     rot[4], pos[0], pos[1], pos[2], t.uprighted);
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
    if (id >= 0 && g_enabled && g_upright && (!NetIsOnline() || NetIsHost())) make_upright(g_tracks[static_cast<uint16_t>(id)], self);
}

// ---- seats and boarding ----
// cObjVehicle (CommonDll7): +0x298 int[4] = entity id sitting at place_N (-1 empty; CheckPlace / CheckUser / GetIn).
// GetIn(character, place) attaches the character to tag "place_<place>" and sets its seat body state; GetOut
// detaches it beside the vehicle. Script player.GetInCar(n) is GameDll7 cObjHunter::GetInCar 0x48d10: the app's
// vehicle (*(0x1657f0)+0x104) vt +0x158 = cObjVehicle::Use(n, hunter) -> GetIn(hunter, n - 1), then hunter vt +0x284(1)
// (the in-vehicle bit +0x4d8 & 4). cObjHunter::OnDie (0x50350) gets a seated hunter out (GetOut) before "Death".
// re/notes/vehicles.md, decompiles re/notes/veh-*.c.
constexpr uintptr_t kVehSeats = 0x298;
constexpr int kPlaces = 4;
constexpr uintptr_t kEntParent = 0xA4;   // int16 parent entity id (-1 = free in the world)
constexpr uintptr_t kHunterDead = 0x6CC;
constexpr uintptr_t kRvaGetInCar = 0x48d10;   // GameDll7, thiscall(hunter, int n)
constexpr uintptr_t kRvaApp = 0x1657f0;       // GameDll7 app object pointer; +0x104 = the level's vehicle
constexpr uintptr_t kAppVehicle = 0x104;
constexpr int kPlayerSeat = 2;                // the scripts' player.GetInCar(2): place_1, beside the driver
constexpr uintptr_t kRvaTryPlaySA = 0xcadf0;  // CommonDll7 cAIHuntTask_AttackBase::TryPlaySA, fastcall(task) -> int

using GetInFn = int(__thiscall*)(void* veh, void* character, int place);
using GetInCarFn = void(__thiscall*)(void* hunter, int n);
GetInFn g_get_in = nullptr;
GetInFn g_get_out = nullptr;
GetInCarFn g_get_in_car = nullptr;
safetyhook::InlineHook* g_driveto_hook = nullptr;
safetyhook::InlineHook* g_trysa_hook = nullptr;
uintptr_t g_game = 0;

struct Cfg {
    bool board_with_host = true;   // [Vehicles] BoardWithHost: client, seat our hunter when the host rides
    bool seat_sync = true;         // [Vehicles] SeatSync: host NPC seats (the driver) mirrored on clients
    bool board_request = true;     // [Vehicles] BoardRequest: a client's own boarding makes the host board
    bool rider_targets = false;    // [Vehicles] RiderTargets: riders stay on the attack-target list. Off: a lion going
                                   // for a rider cannot start its tackle on a puppet and keeps ramming the truck (user saw
                                   // the truck pushed off course, MAP4A 2026-10-18 16:05)
    bool rider_knock = true;       // [Vehicles] RiderKnock: a lion that would tackle a rider knocks its hunter down
    bool split_riders = true;      // [Vehicles] SplitRiders: lions after someone in the truck are shared out
    uint32_t rider_knock_ms = 2500;  // [Vehicles] RiderKnockMs: how long the truck knock keeps the rider down
    bool host_ride_share = true;   // [Vehicles] HostRideToughness: see hk_HunterHurt
};
Cfg g_cfg;

// Host hunter's hurt entry (cScr_ModHunter vt +0x14C, GameDll 0x530c0): thiscall(hunter, attacker, damage, type, pos,
// dir). With RiderTargets off every animal in a truck chase goes for the host, while player-count scaling adds about one
// animal per extra player to each pack. So while the host sits in a vehicle, animal damage to the host is divided by the
// number of players: the host has that many times its health for the ride (user request, MAP4A 2026-10-18).
constexpr uintptr_t kRvaHunterHurt = 0x530c0;
constexpr uintptr_t kEntKind = 0xA7;  // entity kind byte: 0x42 animal
safetyhook::InlineHook* g_hunter_hurt_hook = nullptr;
int g_share_logged = 0;

void __fastcall hk_HunterHurt(void* self, void*, void* attacker, float dmg, int type, const float* pos, const float* dir) {
    float use = dmg;
    if (g_cfg.host_ride_share && !g_cfg.rider_targets && NetIsHost() && NetIsOnline() && self &&
        self == LocalPlayer().entity && dmg > 0.f) {
        int16_t parent = -1;
        uint8_t kind = 0;
        core::safe_read(static_cast<char*>(self) + kEntParent, parent);
        if (attacker) core::safe_read(static_cast<char*>(attacker) + kEntKind, kind);
        int16_t aid = -1;
        if (attacker) core::safe_read(static_cast<char*>(attacker) + 0x04, aid);
        const int players = ConnectedPlayers();
        if (parent >= 0 && kind == 0x42 && players > 1) {
            use = dmg / static_cast<float>(players);
            if (g_share_logged++ < 20)
                spdlog::info("vehicles: host in vehicle {} takes {:.1f} of {:.1f} from animal id {} ({} players)", parent,
                             use, dmg, aid, players);
        }
    }
    g_hunter_hurt_hook->unsafe_thiscall<void>(self, attacker, use, type, pos, dir);
}

int16_t parent_of(void* ent) {
    int16_t p = -1;
    if (ent) core::safe_read(static_cast<char*>(ent) + kEntParent, p);
    return p;
}

bool hunter_dead(void* h) {
    int d = 0;
    return h && core::safe_read(static_cast<char*>(h) + kHunterDead, d) && d != 0;
}

int seat_of(void* veh, int place) {
    int id = -1;
    if (veh && place >= 0 && place < kPlaces) core::safe_read(static_cast<char*>(veh) + kVehSeats + place * 4, id);
    return id;
}

void* level_vehicle() {
    char* app = nullptr;
    void* veh = nullptr;
    if (!g_game || !core::safe_read(reinterpret_cast<char*>(g_game + kRvaApp), app) || !app) return nullptr;
    core::safe_read(app + kAppVehicle, veh);
    return veh;
}

bool driving(void* veh) {
    uint8_t flags = 0;
    return veh && core::safe_read(static_cast<char*>(veh) + kVehDriveFlags, flags) && (flags & kVehDriving);
}

// The local camera is on a cutscene rail (story.cpp PlayerInCutscene: the view more than 4 m from the feet).
bool in_cutscene() {
    const LocalPlayerSample& p = LocalPlayer();
    if (!p.valid || !p.viewValid) return false;
    const float dx = p.viewPos.x - p.pos.x, dy = p.viewPos.y - p.pos.y, dz = p.viewPos.z - p.pos.z;
    return dx * dx + dy * dy + dz * dz > 16.f;
}

// An NPC by its model path, the tracker's own test (".../npc/..."), read from the entity: not Npcs() and not
// LocalPlayer().entity, which are filled by the tracker's 1 s scan after the map load. MAP4A's driver and hunter are
// seated in the first frames; the first version of seat sync swallowed the driver (list still empty) and committed the
// host's own hunter as an NPC (LocalPlayer() still unset).
bool is_npc(int id) {
    void* ent = id >= 0 ? EntityById(id) : nullptr;
    if (!ent || ent == LocalPlayer().entity || puppets::Owns(ent)) return false;
    char* model = nullptr;
    if (!core::safe_read(static_cast<char*>(ent) + 0xE0, model) || !model) return false;
    std::string path = core::safe_cstr(model, 160);
    for (char& c : path) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return path.find("/npc/") != std::string::npos;
}

// Client: the host's last PlayerState flags.
bool host_flags(uint16_t& flags) {
    const SessionView v = Snapshot();
    for (int i = 0; i < v.remoteCount; ++i) {
        if (v.remotes[i].id == 0 && v.remotes[i].packets > 0) {
            flags = v.remotes[i].flags;
            return true;
        }
    }
    return false;
}

bool host_rides_now() {
    uint16_t f = 0;
    return host_flags(f) && (f & net::kFlagRiding) && !(f & net::kFlagDead);
}

bool guarded_get_in_car(void* hunter, int n) {
    __try {
        g_get_in_car(hunter, n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int guarded_seat(GetInFn fn, void* veh, void* character, int place) {
    __try {
        return fn(veh, character, place);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

int16_t g_last_parent = -1;  // the local hunter's parent last frame (boarding detection)
bool g_host_rides = false;   // client: the host's hunter sat in a vehicle at the last seat pump

// cObjVehicle::Use (CommonDll7 0x5adc0): the character's vt +0x1d0 (AreFreeSlots(slot, 0), slot = byte at
// vehicleDef +0x80 + place * 2, count +0x88) decides: free -> GetIn at once; busy -> vt +0x1c8 (ClearSlots) and the
// boarding is queued in the character (+0x1e8 vehicle id, +0x1ec slot, +0x1f0 place + 1) for its own update.
// F9 / coop_tohost run, 2026-10-18 15:10: GetInCar returned "ok" ten times and parent stayed -1.
constexpr uintptr_t kVehDef = 0x1d8;
constexpr uintptr_t kHunterAreFreeSlots = 0x1d0;
constexpr uintptr_t kHunterSetInVehicle = 0x284;  // GetInCar's second call (GameDll 0x548c0): +0x4d8 bit 4, +0xc1c

int place_slot(void* veh, int place) {
    char* def = nullptr;
    int count = 0;
    uint8_t slot = 0;
    if (!core::safe_read(static_cast<char*>(veh) + kVehDef, def) || !def || !core::safe_read(def + 0x88, count)) return -1;
    if (place >= count) return 0;
    return core::safe_read(def + 0x80 + place * 2, slot) ? slot : -1;
}

int hunter_free_slots(void* hunter, int slot) {
    __try {
        using Fn = int(__thiscall*)(void*, int, int);
        return reinterpret_cast<Fn>((*reinterpret_cast<void***>(hunter))[kHunterAreFreeSlots / 4])(hunter, slot, 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

void hunter_set_in_vehicle(void* hunter, bool in) {
    __try {
        using Fn = void(__thiscall*)(void*, int);
        reinterpret_cast<Fn>((*reinterpret_cast<void***>(hunter))[kHunterSetInVehicle / 4])(hunter, in ? 1 : 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

// Seat the local hunter the way the boarding scripts do (player.GetInCar(2)), in this process's copy.
bool seat_local(const char* why) {
    void* h = LocalPlayer().entity;
    if (!h || !g_get_in_car) return false;
    if (parent_of(h) >= 0) return true;
    if (hunter_dead(h)) {
        spdlog::info("vehicles: seat ({}) skipped: our hunter is dead", why);
        return false;
    }
    void* veh = level_vehicle();
    if (!veh) {
        spdlog::info("vehicles: seat ({}) skipped: this map has no vehicle", why);
        return false;
    }
    const int slot = place_slot(veh, kPlayerSeat - 1);
    const int free_before = hunter_free_slots(h, slot);
    const bool ok = guarded_get_in_car(h, kPlayerSeat);
    int16_t par = parent_of(h);
    bool direct = false;
    if (par < 0) {
        // Use() found the hunter's slot busy (a weapon in hand, or a revived hunter's item state) and only queued the
        // boarding for the hunter's own update, which never finishes it for a revived hunter. Do what Use does when the
        // slot is free: GetIn at place_1, then the hunter's in-vehicle bit (what GetInCar sets after Use).
        const int r = guarded_seat(g_get_in, veh, h, kPlayerSeat - 1);
        direct = r == 1;
        if (direct) hunter_set_in_vehicle(h, true);
        par = parent_of(h);
        spdlog::info("vehicles: Use() left our hunter outside (slot {} free={}); direct GetIn -> {} parent={}", slot,
                     free_before, r, par);
    }
    g_last_parent = par;  // not a boarding of our own script (pump_board_request)
    spdlog::info("vehicles: seat our hunter ({}) in vehicle id={}: call ok={} direct={} parent={} place_1={}", why,
                 id_of(veh), ok, direct, par, seat_of(veh, 1));
    return par >= 0;
}

// Client: the host rides and our hunter does not (a boarding cutscene that ran only on the host, a late join that
// skipped the opening cutscene, a revive). Seat it once our copy follows the host's.
uint32_t g_host_riding_since = 0;
uint32_t g_board_try_ms = 0;
int g_board_tries = 0;

void pump_board_with_host(uint32_t now, bool host_rides) {
    if (!g_cfg.board_with_host || NetIsHost() || !NetIsOnline() || !events::SameMapAsHost()) return;
    if (!host_rides) {
        g_host_riding_since = 0;
        g_board_tries = 0;
        return;
    }
    if (!g_host_riding_since) g_host_riding_since = now;
    void* h = LocalPlayer().entity;
    if (!h || parent_of(h) >= 0 || hunter_dead(h) || in_cutscene()) return;
    if (now - g_host_riding_since < 2000 || now - g_board_try_ms < 3000 || g_board_tries >= 10) return;
    void* veh = level_vehicle();
    const auto it = veh ? g_tracks.find(static_cast<uint16_t>(id_of(veh))) : g_tracks.end();
    if (it == g_tracks.end() || it->second.samples == 0) return;  // our copy does not follow the host's yet
    g_board_try_ms = now;
    ++g_board_tries;
    seat_local("the host rides");
}

// Host: an NPC (the driver) got in or out: clients seat their copy of it the same way (kVehicleSeat, journaled, so a
// late joiner seats it too). Players are not mirrored: each one sits in its own copy.
uint32_t g_seat_serial = 0;

void pump_seat_commits() {
    if (!g_cfg.seat_sync || !NetIsHost()) return;
    for (auto& [id, t] : g_tracks) {
        if (!t.ent || EntityById(id) != t.ent) continue;
        for (int p = 0; p < kPlaces; ++p) {
            const int occ = seat_of(t.ent, p);
            const int prev = t.seats_known ? t.seats[p] : -1;
            if (t.seats_known && occ == prev) continue;
            t.seats[p] = occ;
            if (!is_npc(occ) && !(occ < 0 && is_npc(prev))) continue;
            const std::string key = fmt::format("p{}={}#{}", p, occ, ++g_seat_serial);
            const uint32_t packed = static_cast<uint32_t>(p & 0xFF) | (static_cast<uint32_t>(static_cast<uint16_t>(occ)) << 16);
            events::Commit(events::kVehicleSeat, id, packed, 0.f, key, 0);
            spdlog::info("vehicles: host seat commit vehicle id={} place_{} = {} (was {})", id, p, occ, prev);
        }
        t.seats_known = true;
    }
}

// Client: the host's NPC seats, applied to our copies when they differ.
struct WantSeat {
    int occ = -1;
    bool dirty = false;
    int tries = 0;
};
std::map<uint16_t, std::array<WantSeat, kPlaces>> g_want;

void OnSeatCommit(const net::GameEvent& ev) {
    const int place = static_cast<int>(ev.i0 & 0xFF);
    if (place < 0 || place >= kPlaces) return;
    WantSeat& w = g_want[ev.subjectId][place];
    w.occ = static_cast<int16_t>(ev.i0 >> 16);
    w.dirty = true;
    w.tries = 0;
    spdlog::info("vehicles: host seat vehicle id={} place_{} = {} ('{}')", ev.subjectId, place, w.occ, ev.key);
}

void pump_seat_apply() {
    if (NetIsHost() || !NetIsOnline() || !g_get_in || !g_get_out || in_cutscene()) return;
    for (auto& [vid, seats] : g_want) {
        void* veh = EntityById(vid);
        if (!veh) continue;
        for (int p = 0; p < kPlaces; ++p) {
            WantSeat& w = seats[p];
            if (!w.dirty) continue;
            if (++w.tries > 20) {
                w.dirty = false;
                spdlog::warn("vehicles: gave up seating {} at place_{} of vehicle id={}", w.occ, p, vid);
                continue;
            }
            const int have = seat_of(veh, p);
            if (w.occ >= 0) {
                void* ent = EntityById(w.occ);
                if (!ent) continue;  // not spawned here yet
                if (have == w.occ || parent_of(ent) == static_cast<int16_t>(vid)) {
                    w.dirty = false;
                    continue;
                }
                if (have >= 0 || ent == LocalPlayer().entity) {
                    spdlog::warn("vehicles: place_{} of vehicle id={} holds {} here; not seating {}", p, vid, have, w.occ);
                    w.dirty = false;
                    continue;
                }
                const int r = guarded_seat(g_get_in, veh, ent, p);
                spdlog::info("vehicles: seated {} at place_{} of our vehicle id={} -> {} (parent {})", w.occ, p, vid, r,
                             parent_of(ent));
                w.dirty = parent_of(ent) != static_cast<int16_t>(vid);
            } else {
                if (have >= 0 && is_npc(have)) {
                    const int r = guarded_seat(g_get_out, veh, EntityById(have), p);
                    spdlog::info("vehicles: {} got out of place_{} of our vehicle id={} -> {}", have, p, vid, r);
                }
                w.dirty = false;
            }
        }
    }
}

// The last waypoint a script sent each vehicle to (cObjVehicle::DriveTo, CommonDll7 export, thiscall(veh, name)).
// A copy that follows the host's never drives: DriveTo would set the drive bit and call UnFreeze (vt +0x228).
std::map<uint16_t, std::string> g_drive_name;
std::map<uint16_t, bool> g_host_drove;

int __fastcall hk_DriveTo(void* veh, void*, const char* name) {
    const int16_t id = id_of(veh);
    const std::string wp = core::safe_cstr(name, 64);
    if (id >= 0) g_drive_name[static_cast<uint16_t>(id)] = wp;
    if (g_enabled && NetIsOnline() && !NetIsHost() && id >= 0) {
        const auto it = g_tracks.find(static_cast<uint16_t>(id));
        if (it != g_tracks.end() && it->second.samples > 0) {
            spdlog::info("vehicles: our script drives vehicle id={} to '{}': not run, the host's copy drives", id, wp);
            return 1;
        }
    }
    if (NetIsHost() && id >= 0) g_host_drove[static_cast<uint16_t>(id)] = true;
    spdlog::info("vehicles: vehicle id={} drives to '{}' ({})", id, wp, NetIsHost() ? "host" : (NetIsOnline() ? "client" : "offline"));
    return g_driveto_hook->unsafe_thiscall<int>(veh, name);
}

// Client: our own script seated our hunter (it ran a boarding cutscene the host did not run, MAP5 cScrTask06 before
// kTriggerHit) while the host's hunter is not seated. Without the host the truck never leaves: ask it to board.
void pump_board_request(bool host_rides) {
    if (NetIsHost() || !NetIsOnline()) return;
    void* h = LocalPlayer().entity;
    if (!h) return;
    const int16_t par = parent_of(h);
    if (par >= 0 && g_last_parent < 0 && g_cfg.board_request && !host_rides) {
        void* veh = EntityById(par);
        const int driver = seat_of(veh, 0);
        const auto wp = g_drive_name.find(static_cast<uint16_t>(par));
        const std::string name = wp != g_drive_name.end() ? wp->second : std::string();
        events::Request(events::kVehicleBoard, static_cast<uint16_t>(par),
                        static_cast<uint32_t>(static_cast<uint16_t>(driver)), 0.f, name);
        spdlog::info("vehicles: our script seated us in vehicle id={} (driver {}, drive to '{}') before the host: asked "
                     "the host to board",
                     par, driver, name);
    }
    g_last_parent = par;
}

// Host: a client's script boarded before ours. Give our own script 5 s (the same boarding cutscene may be about to
// end here), then do what it would have done: driver in, our hunter in, drive to the client's waypoint.
struct BoardAsk {
    uint16_t veh = 0;
    int driver = -1;
    std::string waypoint;
    uint8_t player = 0;
    uint32_t due = 0;
    uint32_t since = 0;
};
std::vector<BoardAsk> g_board_asks;

void OnBoardRequest(const net::GameEvent& ev) {
    if (!g_cfg.board_request) return;
    BoardAsk a;
    a.veh = ev.subjectId;
    a.driver = static_cast<int16_t>(ev.i0 & 0xFFFF);
    a.waypoint = ev.key;
    a.player = ev.actorId;
    a.since = GetTickCount();
    a.due = a.since + 5000;
    spdlog::info("vehicles: player {} boarded vehicle id={} first (driver {}, drive to '{}'); boarding here in 5 s "
                 "unless our own script does",
                 a.player, a.veh, a.driver, a.waypoint);
    g_board_asks.push_back(a);
}

void pump_board_asks(uint32_t now) {
    for (auto it = g_board_asks.begin(); it != g_board_asks.end();) {
        BoardAsk& a = *it;
        if (static_cast<int32_t>(now - a.due) < 0) {
            ++it;
            continue;
        }
        void* h = LocalPlayer().entity;
        void* veh = EntityById(a.veh);
        if (!h || !veh || hunter_dead(h) || parent_of(h) >= 0 || now - a.since > 120000) {
            spdlog::info("vehicles: board request of player {} done: host hunter {}", a.player,
                         !h ? "missing" : (parent_of(h) >= 0 ? "seated" : "not seated (gave up)"));
            it = g_board_asks.erase(it);
            continue;
        }
        if (in_cutscene()) {  // our own boarding cutscene is running: let it finish
            a.due = now + 2000;
            ++it;
            continue;
        }
        if (a.driver >= 0 && seat_of(veh, 0) < 0 && g_get_in) {
            void* npc = EntityById(a.driver);
            if (npc && parent_of(npc) < 0) {
                const int r = guarded_seat(g_get_in, veh, npc, 0);
                spdlog::info("vehicles: driver {} in place_0 of vehicle id={} -> {}", a.driver, a.veh, r);
            }
        }
        seat_local("a client boarded first");
        if (!a.waypoint.empty() && !driving(veh) && !g_host_drove[a.veh] && g_driveto_hook) {
            g_host_drove[a.veh] = true;
            const int r = g_driveto_hook->unsafe_thiscall<int>(veh, a.waypoint.c_str());
            spdlog::info("vehicles: host drives vehicle id={} to '{}' for player {} -> {}", a.veh, a.waypoint, a.player, r);
        }
        it = g_board_asks.erase(it);
    }
}

// ---- riders as targets (host) ----
// A riding puppet is drawn in the bed of the host's truck and is not attached to it, so a sync attack on it took the
// ground kind ("attack" -> pl_ln_syncattack1: TryPlaySA asks the target's GetCar), linked the puppet's legs to the
// lion and left both on the road. A seated hunter takes the "truck" kind instead (the lion attaches to the truck at
// the hunter's place). TryPlaySA (CommonDll7 0xcadf0) is where every attack task starts one: for a rider it does not
// start; the rider's own hunter gets the truck clip from the host (kPlayerKnock "syncattacktruck", the clip the
// MAP4A teen pak has) and claw hits keep arriving through ForwardPuppetHurt.
constexpr uintptr_t kTaskBrain = 0x08;
constexpr uintptr_t kBrainTargetId = 0x254;
constexpr uintptr_t kBrainEntity = 0x68;
constexpr float kTackleMetres = 4.f;
constexpr uint32_t kTackleEveryMs = 8000;
uint32_t g_blocked_sa = 0;
std::map<uint8_t, uint32_t> g_tackle_ms;   // rider -> last knock sent
std::map<uint8_t, uint32_t> g_tackle_end;  // rider -> when to send its kPlayerKnockEnd

bool map_has_truck_clip() {
    const std::string& m = events::CurrentMap();
    return m == "MAP4A" || m == "MAP4B" || m == "MAP3C" || m == "MAPARENA";
}

void rider_tackle(void* brain, void* rider) {
    const uint8_t who = puppets::PlayerOf(rider);
    ++g_blocked_sa;
    void* animal = nullptr;
    core::safe_read(static_cast<char*>(brain) + kBrainEntity, animal);
    float a[3] = {}, r[3] = {};
    if (!animal || !core::safe_copy(static_cast<char*>(animal) + 0xE8, a, sizeof a) ||
        !core::safe_copy(static_cast<char*>(rider) + 0xE8, r, sizeof r))
        return;
    const float dx = a[0] - r[0], dz = a[2] - r[2];
    const float dist = std::sqrt(dx * dx + dz * dz);
    const uint32_t now = GetTickCount();
    const auto last = g_tackle_ms.find(who);
    const bool due = last == g_tackle_ms.end() || now - last->second >= kTackleEveryMs;
    if (g_blocked_sa <= 5 || g_blocked_sa % 200 == 0)
        spdlog::info("vehicles: sync attack of animal id {} on rider {} not started ({:.1f} m, #{})", id_of(animal), who, dist,
                     g_blocked_sa);
    if (!g_cfg.rider_knock || !due || dist > kTackleMetres || !map_has_truck_clip()) return;
    g_tackle_ms[who] = now;
    g_tackle_end[who] = now + g_cfg.rider_knock_ms;
    // The knock faces the attacker (combat.cpp StartKnock basis: yaw of the victim's forward).
    const float yaw = std::atan2(dx, dz) * 57.2957795f;
    spdlog::info("vehicles: animal id {} tackles rider {} in the truck ({:.1f} m): truck knock, yaw {:.1f}", id_of(animal),
                 who, dist, yaw);
    events::Send(events::kPlayerKnock, who, 0, yaw, "syncattacktruck", 0);
}

int __fastcall hk_TryPlaySA(void* task) {
    if (g_cfg.rider_targets && NetIsHost()) {
        void* brain = nullptr;
        int tid = -1;
        if (core::safe_read(static_cast<char*>(task) + kTaskBrain, brain) && brain &&
            core::safe_read(static_cast<char*>(brain) + kBrainTargetId, tid) && tid >= 0) {
            void* target = EntityById(tid);
            if (target && puppets::IsRidingPuppet(target)) {
                rider_tackle(brain, target);
                return 0;
            }
        }
    }
    return g_trysa_hook->unsafe_fastcall<int>(task);
}

void pump_tackle_ends(uint32_t now) {
    for (auto it = g_tackle_end.begin(); it != g_tackle_end.end();) {
        if (static_cast<int32_t>(now - it->second) >= 0) {
            events::Send(events::kPlayerKnockEnd, it->first, 0, 0.f, "", 0);
            spdlog::info("vehicles: rider {} gets up", it->first);
            it = g_tackle_end.erase(it);
        } else {
            ++it;
        }
    }
}

// Host: the lions that go for someone in the truck are shared out between everyone in it (host and riders), by
// animal id, instead of all on the host (an unattended host died twice on MAP4A: mission failed). The same switch as
// combat.cpp RetargetToNearestPlayer and the engine's ProcessAttackStuck: task vt +0xf8(old, new) in task state 1,
// then SetTarget. In a truck the players are 1-2 m apart, under combat.cpp's 3 m nearest-player margin, so the two
// rules do not fight.
using GetEntFn = void*(__thiscall*)(void* brain);
using SetTargetFn = void(__thiscall*)(void* brain, void* ent, float f);
GetEntFn g_get_target = nullptr;
SetTargetFn g_set_target = nullptr;

void* guarded_get(void* brain) {
    __try {
        return g_get_target(brain);
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

void split_riders() {
    if (!g_cfg.split_riders || !g_cfg.rider_targets || !NetIsHost() || !g_get_target || !g_set_target) return;
    void* host = LocalPlayer().entity;
    if (!host || parent_of(host) < 0 || hunter_dead(host)) return;
    std::vector<void*> crew{host};
    for (uint8_t id = 1; id < 4; ++id) {
        void* pe = puppets::EntityOf(id);
        if (pe && puppets::IsRidingPuppet(pe)) crew.push_back(pe);
    }
    if (crew.size() < 2) return;
    for (void* animal : Animals()) {
        void* brain = nullptr;
        if (!core::safe_read(static_cast<char*>(animal) + 0x200, brain) || !brain) continue;
        void* cur = guarded_get(brain);
        if (std::find(crew.begin(), crew.end(), cur) == crew.end()) continue;  // not after someone in the truck
        const int16_t aid = id_of(animal);
        void* want = crew[static_cast<uint16_t>(aid) % crew.size()];
        if (want == cur) continue;
        void* task = nullptr;
        core::safe_read(static_cast<char*>(brain) + 0x158, task);
        int state = 0;
        if (task) core::safe_read(static_cast<char*>(task) + 4, state);
        if (task && state != 1) continue;
        const bool ok = guarded_switch(brain, task, cur, want);
        const uint8_t who = want == host ? 0 : puppets::PlayerOf(want);
        spdlog::info("vehicles: animal id {} goes for player {} in the truck (shared out, {} aboard) ok={}", aid, who,
                     crew.size(), ok);
    }
}

// ---- test commands ----
void __cdecl cmd_board() { seat_local("coop_board"); }

void __cdecl cmd_unboard() {
    void* h = LocalPlayer().entity;
    const int16_t par = parent_of(h);
    void* veh = par >= 0 ? EntityById(par) : nullptr;
    if (!veh || !g_get_out) {
        spdlog::info("coop_unboard: not seated");
        return;
    }
    const int r = guarded_seat(g_get_out, veh, h, 1);
    spdlog::info("coop_unboard: out of vehicle id={} -> {} parent={}", par, r, parent_of(h));
}

// coop_rideknock <player id>: host sends the truck knock to that player as a lion tackle would.
void __cdecl cmd_rideknock(const char* arg) {
    const int who = std::atoi(core::safe_cstr(arg, 16).c_str());
    if (!NetIsHost() || who <= 0 || who > 3) {
        spdlog::warn("coop_rideknock: host only, usage coop_rideknock <player id 1-3>");
        return;
    }
    const uint32_t now = GetTickCount();
    g_tackle_ms[static_cast<uint8_t>(who)] = now;
    g_tackle_end[static_cast<uint8_t>(who)] = now + g_cfg.rider_knock_ms;
    events::Send(events::kPlayerKnock, static_cast<uint16_t>(who), 0, 0.f, "syncattacktruck", 0);
    spdlog::info("coop_rideknock: truck knock to player {}", who);
}

void __cdecl cmd_vehinfo() {
    void* h = LocalPlayer().entity;
    spdlog::info("coop_vehinfo: level vehicle id={} our hunter parent={} dead={} host rides={} riders targets={}",
                 level_vehicle() ? id_of(level_vehicle()) : -1, parent_of(h), hunter_dead(h), host_rides_now(),
                 g_cfg.rider_targets);
    for (auto& [id, t] : g_tracks) {
        if (!t.ent || EntityById(id) != t.ent) continue;
        float pos[3] = {}, rot[9] = {};
        char* body = body_of(t.ent);
        if (body) {
            core::safe_copy(body + kBodyPos, pos, sizeof pos);
            core::safe_copy(body + kBodyRot, rot, sizeof rot);
        }
        spdlog::info("coop_vehinfo: vehicle id={} pos=({:.1f},{:.1f},{:.1f}) driving={} frozen={} samples={} seats {} {} {} {}",
                     id, pos[0], pos[1], pos[2], driving(t.ent), t.frozen, t.samples, seat_of(t.ent, 0),
                     seat_of(t.ent, 1), seat_of(t.ent, 2), seat_of(t.ent, 3));
        spdlog::info("coop_vehinfo: vehicle id={} rot rows ({:.2f},{:.2f},{:.2f}) ({:.2f},{:.2f},{:.2f}) ({:.2f},{:.2f},{:.2f}) "
                     "uprighted={}",
                     id, rot[0], rot[1], rot[2], rot[3], rot[4], rot[5], rot[6], rot[7], rot[8], t.uprighted);
    }
}

// coop_flipveh (host or offline): roll every vehicle 180 degrees about its first axis, as the MAP4A ramp does.
void __cdecl cmd_flipveh() {
    for (auto& [id, t] : g_tracks) {
        if (!t.ent || EntityById(id) != t.ent || !g_set_rot) continue;
        char* body = body_of(t.ent);
        char* phys = nullptr;
        float rot[9];
        if (!body || !core::safe_read(static_cast<char*>(t.ent) + kVehPhys, phys) || !phys ||
            !core::safe_copy(body + kBodyRot, rot, sizeof rot))
            continue;
        for (int k = 3; k < 9; ++k) rot[k] = -rot[k];
        g_set_rot(phys, rot);
        spdlog::info("coop_flipveh: vehicle id={} rolled 180 degrees (guard {})", id, g_upright ? "on" : "off");
    }
}

class VehiclesModule : public dh2::Module {
public:
    const char* Name() const override { return "vehicles"; }
    int Priority() const override { return 46; }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        g_enabled = GetPrivateProfileIntA("Coop", "VehicleSync", 1, ini.c_str()) != 0;
        auto key = [&](const char* name, bool def) {
            return GetPrivateProfileIntA("Vehicles", name, def ? 1 : 0, ini.c_str()) != 0;
        };
        g_cfg.board_with_host = key("BoardWithHost", true);
        g_cfg.seat_sync = key("SeatSync", true);
        g_cfg.board_request = key("BoardRequest", true);
        g_cfg.rider_targets = key("RiderTargets", false);
        g_cfg.rider_knock = key("RiderKnock", true);
        g_cfg.split_riders = key("SplitRiders", true);
        g_cfg.host_ride_share = key("HostRideToughness", true);
        g_upright = key("Upright", false);
        g_cfg.rider_knock_ms = static_cast<uint32_t>(GetPrivateProfileIntA("Vehicles", "RiderKnockMs", 2500, ini.c_str()));
        events::SetHandler(events::kVehicleSeat, nullptr, &OnSeatCommit);
        events::SetHandler(events::kVehicleBoard, &OnBoardRequest, nullptr);
        spdlog::info("vehicles: VehicleSync={} BoardWithHost={} SeatSync={} BoardRequest={} RiderTargets={} RiderKnock={} "
                     "({} ms) SplitRiders={} Upright={}",
                     g_enabled ? 1 : 0, g_cfg.board_with_host, g_cfg.seat_sync, g_cfg.board_request, g_cfg.rider_targets,
                     g_cfg.rider_knock, g_cfg.rider_knock_ms, g_cfg.split_riders, g_upright);
    }

    void OnEngineReady(HMODULE engine) override {
        g_set_pos = reinterpret_cast<PhysSetVecFn>(
            core::find_export(engine, "?SetPosition@cPhysicsObject@@UAEXABUtVect3@@@Z"));
        g_set_rot = reinterpret_cast<PhysSetVecFn>(
            core::find_export(engine, "?SetOrientation@cPhysicsObject@@UAEXABUm3x3@@@Z"));
    }

    void OnGameDllLoaded(HMODULE game) override {
        HMODULE common = GetModuleHandleA("CommonDll7.dll");
        if (!common) {
            spdlog::error("vehicles: CommonDll7.dll not loaded; vehicle sync off");
            return;
        }
        g_game = reinterpret_cast<uintptr_t>(game);
        g_freeze = reinterpret_cast<VehFreezeFn>(core::find_export(common, "?Freeze@cObjVehicle@@UAEXXZ"));
        g_update = core::install_hook("cObjVehicle::Update", core::find_export(common, "?Update@cObjVehicle@@UAEXM@Z"),
                                      reinterpret_cast<void*>(&hk_Update));
        g_get_in = reinterpret_cast<GetInFn>(
            core::find_export(common, "?GetIn@cObjVehicle@@QAEHPAVcObjCharacterEx@@H@Z"));
        g_get_out = reinterpret_cast<GetInFn>(
            core::find_export(common, "?GetOut@cObjVehicle@@QAEHPAVcObjCharacterEx@@H@Z"));
        g_driveto_hook = core::install_hook("cObjVehicle::DriveTo",
                                            core::find_export(common, "?DriveTo@cObjVehicle@@QAEHPBD@Z"),
                                            reinterpret_cast<void*>(&hk_DriveTo));
        g_get_target = reinterpret_cast<GetEntFn>(
            core::find_export(common, "?GetTarget@cAIHuntBrain_Animal@@QBEPAVcObjEntity@@XZ"));
        g_set_target = reinterpret_cast<SetTargetFn>(
            core::find_export(common, "?SetTarget@cAIHuntBrain_Animal@@QAEXPAVcObjEntity@@M@Z"));
        // Both bodies start with the standard SEH frame (v1.0 build 434); anything else is another build.
        static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
        uint8_t got[5] = {};
        void* gic = reinterpret_cast<char*>(game) + kRvaGetInCar;
        if (core::safe_copy(gic, got, sizeof got) && std::memcmp(got, kPrologue, sizeof got) == 0)
            g_get_in_car = reinterpret_cast<GetInCarFn>(gic);
        else
            spdlog::error("vehicles: GetInCar at GameDll7+{:#x} has unexpected bytes; no seating", kRvaGetInCar);
        void* tsa = reinterpret_cast<char*>(common) + kRvaTryPlaySA;
        if (core::safe_copy(tsa, got, sizeof got) && std::memcmp(got, kPrologue, sizeof got) == 0)
            g_trysa_hook = core::install_hook("cAIHuntTask_AttackBase::TryPlaySA (0xcadf0)", tsa,
                                              reinterpret_cast<void*>(&hk_TryPlaySA));
        else
            spdlog::error("vehicles: TryPlaySA at CommonDll7+{:#x} has unexpected bytes; riders not protected",
                          kRvaTryPlaySA);
        if (!g_trysa_hook) g_cfg.rider_targets = false;  // never a rider on the list without the guard
        void* hh = reinterpret_cast<char*>(game) + kRvaHunterHurt;
        if (core::safe_copy(hh, got, sizeof got) && std::memcmp(got, kPrologue, sizeof got) == 0)
            g_hunter_hurt_hook = core::install_hook("hunter OnHurt (host ride toughness, GameDll 0x530c0)", hh,
                                                    reinterpret_cast<void*>(&hk_HunterHurt));
        else
            spdlog::error("vehicles: hunter OnHurt at GameDll7+{:#x} has unexpected bytes ({:02x} {:02x}); no ride toughness",
                          kRvaHunterHurt, got[0], got[1]);
        spdlog::info("vehicles: freeze={} setpos={} setrot={} getin={} getout={} getincar={} driveto={} trysa={}",
                     g_freeze != nullptr, g_set_pos != nullptr, g_set_rot != nullptr, g_get_in != nullptr,
                     g_get_out != nullptr, g_get_in_car != nullptr, g_driveto_hook != nullptr, g_trysa_hook != nullptr);
    }

    static void ResetMap() {
        g_tracks.clear();
        g_want.clear();
        g_board_asks.clear();
        g_drive_name.clear();
        g_host_drove.clear();
        g_tackle_end.clear();
        g_tackle_ms.clear();
        g_last_parent = -1;
        g_host_riding_since = 0;
        g_board_tries = 0;
    }
    void OnMapUnloading() override { ResetMap(); }
    void OnMapLoaded(const char*) override { ResetMap(); }

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

    // Seats, boarding and riders, 5 times a second.
    void PumpSeats(uint32_t now) {
        if (!commands_ && console_cmds::Ready()) {
            commands_ = console_cmds::AddCommand0("coop_board", &cmd_board);
            console_cmds::AddCommand0("coop_unboard", &cmd_unboard);
            console_cmds::AddCommandStr("coop_rideknock", &cmd_rideknock);
            console_cmds::AddCommand0("coop_vehinfo", &cmd_vehinfo);
            console_cmds::AddCommand0("coop_flipveh", &cmd_flipveh);
        }
        if (!NetIsOnline()) {
            g_last_parent = parent_of(LocalPlayer().entity);
            return;
        }
        if (NetIsHost()) pump_tackle_ends(GetTickCount());
        if (now - last_seat_pump_ < 200) return;
        last_seat_pump_ = now;
        if (NetIsHost()) {
            pump_seat_commits();
            pump_board_asks(GetTickCount());
            if (now - last_split_ >= 1000) {
                last_split_ = now;
                split_riders();
            }
            g_last_parent = parent_of(LocalPlayer().entity);
            return;
        }
        const bool host_rides = host_rides_now();
        g_host_rides = host_rides;
        pump_board_request(host_rides);
        pump_board_with_host(now, host_rides);
        pump_seat_apply();
    }

    // Host: the chassis pose of every vehicle that updated this map, 30 times a second.
    void OnFrame(float) override {
        if (g_enabled && NetIsOnline()) LogRide(now_ms());
        if (g_enabled) PumpSeats(now_ms());
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
    uint32_t last_seat_pump_ = 0;
    uint32_t last_split_ = 0;
    bool commands_ = false;
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

bool RideWithHost(const char* why) {
    if (!g_enabled || NetIsHost() || !NetIsOnline()) return false;
    void* h = LocalPlayer().entity;
    if (!h) return false;
    const int16_t par = parent_of(h);
    if (par >= 0) {
        spdlog::info("vehicles: {}: our hunter sits in vehicle id={}; not moved", why ? why : "", par);
        return true;
    }
    if (!g_cfg.board_with_host || !host_rides_now()) return false;
    return seat_local(why ? why : "ride with host");
}

bool HostRides() { return !NetIsHost() && NetIsOnline() && host_rides_now(); }

bool RidersAreTargets() { return g_cfg.rider_targets; }

}  // namespace dh2::coop::vehicles
