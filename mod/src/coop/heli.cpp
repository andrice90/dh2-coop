#include "coop/heli.h"

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
#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"

namespace dh2::coop::heli {
namespace {

// GameDll7 cObjHelicopter (ctor 0x22e70, vtable 0x100d50, "obj_helicopter"). Decompiles: re/notes/pilot.md.
//   Update 0x233f0 thiscall(this, float dt): cObjItemEx::Update, fly model (+0x210) vt +0x8c(dt) (parts and their body
//     state machines), then the "masscenter" bone (+0x214) position -> entity vt +0xa4 and its angles -> vt +0xb0.
//   0x23cc0 thiscall(this, const char* state): the BSMGoToState body (cScrHelicopter::BSMGoToState 0x240f0 calls it
//     with the script's entity +0x28): every part whose machine is a script body gets vt +0x60 (reset), vt +0x7c(state).
//   cScrHelicopter (script object, 0x34 bytes): +0x28 entity, +0x30 task flag (CompleteTask 0x22dc0 = 1,
//     UncompleteTask 0x22dd0 = 0, IsTaskCompleted 0x22db0). MakeVisible 0x24250 writes entity +0x234.
constexpr uintptr_t kRvaHeliUpdate = 0x233f0;
constexpr uintptr_t kRvaHeliGoToState = 0x23cc0;
constexpr uintptr_t kRvaHeliVt = 0x100d50;
constexpr uintptr_t kEntId = 0x04;
constexpr uintptr_t kEntPos = 0xE8;
constexpr uintptr_t kEntName = 0x198;      // target name (char*), what FindEntScrObject compares
constexpr uintptr_t kHeliModel = 0x210;    // cModel* (the fly model; the entity has no +0xE4 model)
constexpr uintptr_t kHeliVisible = 0x234;  // MakeVisible
constexpr uintptr_t kScrEntity = 0x28;
constexpr uintptr_t kScrTaskDone = 0x30;
// EngineDll7 cModelPart (decompiles of GetAnimElapsedTime 0x16ea70, GetAnimSpeedFactor 0x16ea90, GetCurrentAnim
// 0x16ea80, cModelPart::Update 0x1e6080, SetAnimElapsedTime 0x1e6410): +0x84 clip time (s), +0x88 speed factor,
// +0x90 current clip index (short), +0xd8 body state machine; vt +0x6c SetAnimElapsedTime. cModelAnim +0x4 name,
// +0xc length. cBodyStateMachine vt +0x80 GetCurrentState. The model: vt +0xf8 part count, vt +0xf0(i) part.
constexpr uintptr_t kPartElapsed = 0x84;
constexpr uintptr_t kPartSpeed = 0x88;
constexpr uintptr_t kPartAnimIndex = 0x90;
constexpr uintptr_t kPartMachine = 0xd8;
constexpr uintptr_t kAnimName = 0x04;
constexpr uintptr_t kAnimLength = 0x0c;

constexpr uint32_t kSendMs = 50;        // 20 Hz
constexpr uint32_t kMaxExtrapMs = 250;  // play on from the newest sample at most this far
constexpr uint32_t kStateHoldMs = 500;  // a state that differs this long without a host edge is switched
constexpr uint32_t kAnimHoldMs = 300;
constexpr float kSnapSeconds = 0.05f;   // clip time error that is corrected (at ~20 m/s this is ~1 m)
constexpr float kEndGuard = 0.985f;     // near a clip's end the client's own script moves on (WaitForAnimFix 990)

using GoToStateFn = void(__thiscall*)(void* heli, const char* state);
using GetAnimFn = void*(__thiscall*)(void* part, int index);
using SpeedFn = void(__thiscall*)(void* part, float f);
using GetScrFn = void*(__thiscall*)(void* ent);

safetyhook::InlineHook* g_update = nullptr;
GoToStateFn g_goto = nullptr;
GetAnimFn g_get_anim = nullptr;
SpeedFn g_set_speed = nullptr;
GetScrFn g_get_scr = nullptr;
uintptr_t g_game = 0;
bool g_enabled = true;  // [Coop] HeliSync: clients play the host's flight. 0 = only log the drift

uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

void* slot(void* obj, uintptr_t off) {
    void** vt = nullptr;
    void* fn = nullptr;
    if (!obj || !core::safe_read(obj, vt) || !vt || !core::safe_read(reinterpret_cast<char*>(vt) + off, fn)) return nullptr;
    return fn;
}

int16_t id_of(void* ent) {
    int16_t id = -1;
    core::safe_read(static_cast<char*>(ent) + kEntId, id);
    return id;
}

std::string name_of(void* ent) {
    char* name = nullptr;
    if (!ent || !core::safe_read(static_cast<char*>(ent) + kEntName, name) || !name) return {};
    return core::safe_cstr(name, 48);
}

// What one copy of the helicopter is doing: part 0 of the fly model carries the cBodyHelicopter machine and the clip.
struct Pose {
    bool ok = false;
    void* part = nullptr;
    void* machine = nullptr;
    std::string state;
    std::string anim;
    float elapsed = 0.f;
    float speed = 0.f;
    float length = 0.f;
};

struct RawPose {
    void* part = nullptr;
    void* machine = nullptr;
    char state[32] = {};
    char anim[32] = {};
    float elapsed = 0.f, speed = 0.f, length = 0.f;
};

void copy_name(char (&dst)[32], const char* src) {
    size_t i = 0;
    for (; src && i + 1 < sizeof dst && src[i]; ++i) dst[i] = src[i];
    dst[i] = 0;
}

bool guarded_pose(void* heli, RawPose& p) {
    __try {
        void* model = *reinterpret_cast<void**>(static_cast<char*>(heli) + kHeliModel);
        if (!model) return false;
        const short parts = reinterpret_cast<short(__thiscall*)(void*)>(slot(model, 0xf8))(model);
        if (parts <= 0) return false;
        void* part = reinterpret_cast<void*(__thiscall*)(void*, int)>(slot(model, 0xf0))(model, 0);
        if (!part) return false;
        p.part = part;
        p.machine = *reinterpret_cast<void**>(static_cast<char*>(part) + kPartMachine);
        if (p.machine) copy_name(p.state, reinterpret_cast<const char*(__thiscall*)(void*)>(slot(p.machine, 0x80))(p.machine));
        p.elapsed = *reinterpret_cast<float*>(static_cast<char*>(part) + kPartElapsed);
        p.speed = *reinterpret_cast<float*>(static_cast<char*>(part) + kPartSpeed);
        const short index = *reinterpret_cast<short*>(static_cast<char*>(part) + kPartAnimIndex);
        if (index >= 0 && g_get_anim) {
            char* anim = static_cast<char*>(g_get_anim(part, index));
            if (anim) {
                copy_name(p.anim, *reinterpret_cast<const char**>(anim + kAnimName));
                p.length = *reinterpret_cast<float*>(anim + kAnimLength);
            }
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool read_pose(void* heli, Pose& p) {
    p = {};
    RawPose r;
    if (!heli || !guarded_pose(heli, r)) return false;
    p.ok = true;
    p.part = r.part;
    p.machine = r.machine;
    p.state = r.state;
    p.anim = r.anim;
    p.elapsed = r.elapsed;
    p.speed = r.speed;
    p.length = r.length;
    return true;
}

bool guarded_goto(void* heli, const char* state) {
    __try {
        g_goto(heli, state);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_set_anim(void* part, const char* anim) {
    __try {
        // cScrHelicopter::SetAnim (0x23fd0) does the same: fly model part 0, vt +0x58(name).
        reinterpret_cast<int(__thiscall*)(void*, const char*)>(slot(part, 0x58))(part, anim);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_set_elapsed(void* part, float t) {
    __try {
        reinterpret_cast<void(__thiscall*)(void*, float)>(slot(part, 0x6c))(part, t);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_set_speed(void* part, float f) {
    __try {
        g_set_speed(part, f);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void* guarded_scr(void* ent) {
    __try {
        return g_get_scr(ent);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

// The script object's task flag (CompleteTask). Null when the script object does not point back at this entity.
int* task_flag(void* heli) {
    if (!g_get_scr) return nullptr;
    char* scr = static_cast<char*>(guarded_scr(heli));
    void* back = nullptr;
    if (!scr || !core::safe_read(scr + kScrEntity, back) || back != heli) return nullptr;
    return reinterpret_cast<int*>(scr + kScrTaskDone);
}

struct Sample {
    uint32_t tick = 0;  // host clock
    net::HeliState s;
};

struct ClientPos {
    uint32_t ms = 0;  // local clock
    float pos[3] = {};
};

struct Track {
    void* ent = nullptr;
    bool logged = false;
    // Host: what was sent last (transition log).
    std::string sent_state, sent_anim;
    // Client: the host's samples, newest last, and the host clock (offset = local - host, the smallest seen).
    Sample samples[4];
    int count = 0;
    int32_t offset = 0;
    bool have_offset = false;
    bool state_edge = false;  // the newest sample changed the host's state: follow at once
    bool anim_edge = false;
    uint32_t state_mismatch_ms = 0;
    uint32_t anim_mismatch_ms = 0;
    bool following = false;
    // Diagnostics: the client's own entity positions, to compare with a host sample of the same moment.
    ClientPos hist[64];
    int hist_next = 0;
    uint32_t drift_log_ms = 0;
    int drift_logs = 0;
    int corrections = 0;
    int state_switches = 0;
    int anim_switches = 0;
    float max_dt_err = 0.f;
};

std::map<uint16_t, Track> g_tracks;
uint32_t g_last_send = 0;

bool following_allowed() {
    return NetIsOnline() && !NetIsHost() && events::SameMapAsHost() && LocalPlayer().pilot;
}

// Client, before the helicopter's own update: the host's state, clip and clip time on our copy.
void follow(Track& t, void* heli, uint32_t now) {
    const Sample& newest = t.samples[t.count - 1];
    const net::HeliState& h = newest.s;
    Pose p;
    if (!read_pose(heli, p) || !p.part) return;
    if (!t.following) {
        t.following = true;
        spdlog::info("heli: id={} follows the host's helicopter (here '{}' '{}' {:.2f}s, host '{}' '{}' {:.2f}s)",
                     id_of(heli), p.state, p.anim, p.elapsed, h.state, h.anim, h.elapsed);
    }
    // Our own trigger replays still call CompleteTask; only the host decides when a loop ends.
    if (int* flag = task_flag(heli)) *flag = 0;

    if (!h.state.empty() && p.state != h.state) {
        if (!t.state_mismatch_ms) t.state_mismatch_ms = now;
        if (t.state_edge || now - t.state_mismatch_ms >= kStateHoldMs) {
            const bool ok = g_goto && guarded_goto(heli, h.state.c_str());
            ++t.state_switches;
            spdlog::info("heli: id={} state '{}' -> host '{}' ({}) ok={}", id_of(heli), p.state, h.state,
                         t.state_edge ? "host changed" : "held too long", ok);
            t.state_edge = false;
            t.state_mismatch_ms = 0;
            read_pose(heli, p);
        }
    } else {
        t.state_mismatch_ms = 0;
        t.state_edge = false;
    }
    if (p.state != h.state) return;  // the clip is the state's business until the states agree

    if (!h.anim.empty() && p.anim != h.anim) {
        if (!t.anim_mismatch_ms) t.anim_mismatch_ms = now;
        if (t.anim_edge || now - t.anim_mismatch_ms >= kAnimHoldMs) {
            const bool ok = guarded_set_anim(p.part, h.anim.c_str());
            ++t.anim_switches;
            spdlog::info("heli: id={} clip '{}' -> host '{}' in state '{}' ok={}", id_of(heli), p.anim, h.anim, h.state, ok);
            t.anim_edge = false;
            t.anim_mismatch_ms = 0;
            read_pose(heli, p);
        }
    } else {
        t.anim_mismatch_ms = 0;
        t.anim_edge = false;
    }
    if (p.anim != h.anim || h.anim.empty()) return;  // no clip yet (before cScrTask01): nothing to time

    if (g_set_speed && std::fabs(p.speed - h.speed) > 1e-3f) guarded_set_speed(p.part, h.speed);
    // The host's clip time now, on the host's clock.
    const uint32_t host_now = now - static_cast<uint32_t>(t.offset);
    int32_t ahead = static_cast<int32_t>(host_now - newest.tick);
    if (ahead < 0) ahead = 0;
    if (ahead > static_cast<int32_t>(kMaxExtrapMs)) ahead = static_cast<int32_t>(kMaxExtrapMs);
    const float target = h.elapsed + h.speed * static_cast<float>(ahead) / 1000.f;
    const float length = h.length > 0.f ? h.length : p.length;
    if (!(length > 0.f) || !(target >= 0.f) || target >= length * kEndGuard) return;  // no known clip length: no snap
    const float err = p.elapsed - target;
    if (std::fabs(err) > std::fabs(t.max_dt_err)) t.max_dt_err = err;
    if (std::fabs(err) > kSnapSeconds) {
        guarded_set_elapsed(p.part, target);
        if (t.corrections++ < 10 || (std::fabs(err) > 1.f && t.corrections < 200))
            spdlog::info("heli: id={} '{}' clip time {:.3f}s -> host {:.3f}s (error {:+.3f}s)", id_of(heli), h.anim,
                         p.elapsed, target, err);
    }
}

// Client diagnostics: our entity position against the host's at the same host moment, every 2 s for ~3 minutes.
void log_drift(Track& t, void* heli, uint32_t now) {
    float pos[3] = {};
    if (!core::safe_copy(static_cast<char*>(heli) + kEntPos, pos, sizeof pos)) return;
    ClientPos& c = t.hist[t.hist_next];
    c.ms = now;
    std::memcpy(c.pos, pos, sizeof pos);
    t.hist_next = (t.hist_next + 1) % 64;
    if (t.count == 0 || t.drift_logs >= 90 || now - t.drift_log_ms < 2000) return;
    t.drift_log_ms = now;
    ++t.drift_logs;
    // The host sample before the newest (so our history surely covers its moment).
    const Sample& smp = t.samples[t.count > 1 ? t.count - 2 : 0];
    const uint32_t local_at = smp.tick + static_cast<uint32_t>(t.offset);
    const ClientPos* best = nullptr;
    uint32_t best_d = 0xffffffff;
    for (const ClientPos& e : t.hist) {
        if (!e.ms) continue;
        const uint32_t d = e.ms > local_at ? e.ms - local_at : local_at - e.ms;
        if (d < best_d) {
            best_d = d;
            best = &e;
        }
    }
    Pose p;
    read_pose(heli, p);
    float dist = -1.f;
    if (best && best_d < 100) {
        const float dx = best->pos[0] - smp.s.pos[0], dy = best->pos[1] - smp.s.pos[1], dz = best->pos[2] - smp.s.pos[2];
        dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    const net::HeliState& h = t.samples[t.count - 1].s;
    float ang[3] = {};
    core::safe_copy(static_cast<char*>(heli) + 0xF4, ang, sizeof ang);
    spdlog::info("heli: drift id={} {:.2f} m | here '{}' '{}' {:.2f}s | host '{}' '{}' {:.2f}s | fixes {} states {} "
                 "clips {} max err {:+.2f}s | pos ({:.1f},{:.1f},{:.1f}) ang ({:.1f},{:.1f},{:.1f}){}",
                 id_of(heli), dist, p.state, p.anim, p.elapsed, h.state, h.anim, h.elapsed, t.corrections,
                 t.state_switches, t.anim_switches, t.max_dt_err, pos[0], pos[1], pos[2], ang[0], ang[1], ang[2],
                 g_enabled ? "" : " (HeliSync=0: not following)");
    t.max_dt_err = 0.f;
}

void __fastcall hk_Update(void* self, void*, float dt) {
    const int16_t id = id_of(self);
    Track* t = nullptr;
    if (id >= 0 && name_of(self) == "helicopter") {
        t = &g_tracks[static_cast<uint16_t>(id)];
        t->ent = self;
        if (!t->logged) {
            t->logged = true;
            Pose p;
            read_pose(self, p);
            spdlog::info("heli: helicopter id={} entity {:#x} updates (role {}) state '{}' clip '{}' {:.2f}/{:.2f}s",
                         id, reinterpret_cast<uintptr_t>(self),
                         NetIsHost() ? "host" : (NetIsOnline() ? "client" : "offline"), p.state, p.anim, p.elapsed,
                         p.length);
        }
    }
    const uint32_t now = now_ms();
    const bool client = t && t->count > 0 && following_allowed();
    if (client && g_enabled) follow(*t, self, now);
    g_update->unsafe_thiscall<void>(self, dt);
    if (client) log_drift(*t, self, now);
}

class HeliModule : public dh2::Module {
public:
    const char* Name() const override { return "heli"; }
    int Priority() const override { return 46; }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        g_enabled = GetPrivateProfileIntA("Coop", "HeliSync", 1, ini.c_str()) != 0;
        spdlog::info("heli: HeliSync={}", g_enabled ? 1 : 0);
    }

    void OnEngineReady(HMODULE engine) override {
        g_get_anim = reinterpret_cast<GetAnimFn>(core::find_export(engine, "?GetAnim@cModelPart@@QAEPAVcModelAnim@@H@Z"));
        g_set_speed = reinterpret_cast<SpeedFn>(core::find_export(engine, "?SetAnimSpeedFactor@cModelPart@@QAEXM@Z"));
    }

    void OnGameDllLoaded(HMODULE game) override {
        g_game = reinterpret_cast<uintptr_t>(game);
        if (HMODULE common = GetModuleHandleA("CommonDll7.dll"))
            g_get_scr = reinterpret_cast<GetScrFn>(
                core::find_export(common, "?GetScrObject@cObjScriptedEntity@@QAEPAVcScrEntity@@XZ"));
        // Both are SEH-framed functions of the v1.0 build: refuse anything else.
        static const uint8_t kPrologue[5] = {0x55, 0x8b, 0xec, 0x6a, 0xff};
        auto checked = [&](uintptr_t rva, const char* label) -> void* {
            void* p = reinterpret_cast<void*>(g_game + rva);
            uint8_t got[5] = {};
            if (!core::safe_copy(p, got, sizeof got) || std::memcmp(got, kPrologue, sizeof got) != 0) {
                spdlog::error("heli: {} at GameDll7+{:#x} has unexpected bytes; helicopter sync off", label, rva);
                return nullptr;
            }
            return p;
        };
        g_goto = reinterpret_cast<GoToStateFn>(checked(kRvaHeliGoToState, "BSMGoToState body"));
        if (void* upd = checked(kRvaHeliUpdate, "cObjHelicopter::Update"))
            g_update = core::install_hook("cObjHelicopter::Update", upd, reinterpret_cast<void*>(&hk_Update));
        spdlog::info("heli: update hook={} goto={} getanim={} speed={} scr={}", g_update != nullptr, g_goto != nullptr,
                     g_get_anim != nullptr, g_set_speed != nullptr, g_get_scr != nullptr);
    }

    void OnMapUnloading() override { g_tracks.clear(); }
    void OnMapLoaded(const char*) override { g_tracks.clear(); }

    // Host: the helicopter's state, clip and clip time, 20 times a second, on the pilot maps.
    void OnFrame(float) override {
        if (!NetIsHost() || g_tracks.empty() || !LocalPlayer().pilot) return;
        const uint32_t now = now_ms();
        if (now - g_last_send < kSendMs) return;
        g_last_send = now;
        net::HeliStates msg;
        msg.tickMs = now;
        msg.mapSerial = events::LocalMapSerial();
        for (auto& [id, t] : g_tracks) {
            if (!t.ent || EntityById(id) != t.ent) continue;
            Pose p;
            if (!read_pose(t.ent, p)) continue;
            net::HeliState h;
            h.id = id;
            h.state = p.state;
            h.anim = p.anim;
            h.elapsed = p.elapsed;
            h.speed = p.speed;
            h.length = p.length;
            core::safe_copy(static_cast<char*>(t.ent) + kEntPos, h.pos, sizeof h.pos);
            int visible = 0;
            core::safe_read(static_cast<char*>(t.ent) + kHeliVisible, visible);
            const int* flag = task_flag(t.ent);
            int done = 0;
            if (flag) core::safe_read(flag, done);
            h.flags = static_cast<uint8_t>((done ? net::heli_flag::kTaskDone : 0) | (visible ? net::heli_flag::kVisible : 0));
            if (p.state != t.sent_state || p.anim != t.sent_anim) {
                spdlog::info("heli: host id={} state '{}' -> '{}' clip '{}' -> '{}' at {:.2f}/{:.2f}s pos ({:.1f},{:.1f},{:.1f})",
                             id, t.sent_state, p.state, t.sent_anim, p.anim, p.elapsed, p.length, h.pos[0], h.pos[1],
                             h.pos[2]);
                t.sent_state = p.state;
                t.sent_anim = p.anim;
            }
            msg.helis.push_back(std::move(h));
            if (msg.helis.size() >= net::kMaxHelis) break;
        }
        if (!msg.helis.empty()) BroadcastHeliStates(msg);
    }
};

DH2_MODULE(HeliModule)

}  // namespace

void OnStates(const net::HeliStates& states, uint32_t localMs) {
    for (const net::HeliState& h : states.helis) {
        // A map has one or two helicopters; ids beyond a small bound are not ours to track.
        if (g_tracks.count(h.id) == 0 && g_tracks.size() >= 2u * net::kMaxHelis) continue;
        Track& t = g_tracks[h.id];
        // Unreliable channel: drop a packet older than the newest we have.
        if (t.count > 0 && static_cast<int32_t>(states.tickMs - t.samples[t.count - 1].tick) <= 0) continue;
        const int32_t offset = static_cast<int32_t>(localMs - states.tickMs);
        if (!t.have_offset || offset < t.offset) t.offset = offset;
        t.have_offset = true;
        if (t.count > 0) {
            const net::HeliState& prev = t.samples[t.count - 1].s;
            if (prev.state != h.state) t.state_edge = true;
            if (prev.anim != h.anim) t.anim_edge = true;
        }
        if (t.count == 4) {
            for (int k = 1; k < 4; ++k) t.samples[k - 1] = t.samples[k];
            --t.count;
        }
        Sample& s = t.samples[t.count++];
        s.tick = states.tickMs;
        s.s = h;
    }
}

}  // namespace dh2::coop::heli
