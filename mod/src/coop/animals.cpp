// Phase 4 shared animals.
// Host (session active, ≥1 client): every 50 ms, AnimalStates from the tracker list.
// Client: FreezeBrain, keep physics, and in the pre-render callback HandleKey position/angle
// (map-load path: entity + physics body). OnFrame placement flickers. UnlinkPhysics hides animals.
// [Animals] Spike=1 remains a solo experiment and is skipped while a session is active.
#include "coop/animals.h"

#include "coop/events.h"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "coop/coop.h"
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
constexpr uintptr_t kEntAngle = 0xF4;
constexpr uintptr_t kEntWorldBox = 0xC4;
constexpr uintptr_t kEntHealth = 0x19C;
constexpr float kYawMoveM = 0.05f;
constexpr uint32_t kInterpDelayMs = 100;
constexpr uint32_t kInterpHistoryMs = 1000;
constexpr int kTeleportRepeats = 3;

class AnimalsModule;
extern AnimalsModule* g_self;
void prerender();

safetyhook::InlineHook* g_draw_hook = nullptr;
std::atomic<void*> g_watch{nullptr};
std::atomic<uint32_t> g_draws{0};
void __fastcall hk_Draw(void* self, void* /*edx*/) {
    if (self == g_watch.load(std::memory_order_relaxed)) g_draws.fetch_add(1, std::memory_order_relaxed);
    g_draw_hook->unsafe_thiscall<void>(self);
}

bool g_log_states = true;
bool g_sync_log = true;
bool g_spike = false;
bool g_spike_unlink = true;
float g_spike_y = 0.7f;

std::string ini_path() { return (core::self_dir() / "dh2coop.ini").string(); }

uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

float NormYaw(float yaw) {
    yaw = std::fmod(yaw, 360.f);
    if (yaw < 0.f) yaw += 360.f;
    return yaw;
}

float Wrap180(float deg) {
    float x = std::fmod(deg + 180.f, 360.f);
    if (x < 0.f) x += 360.f;
    return x - 180.f;
}

float LerpYaw(float a, float b, float t) { return NormYaw(a + Wrap180(b - a) * t); }

struct AnimalSample {
    float pos[3] = {};
    float yaw = 0.f;
    float health = 0.f;
    uint8_t flags = 0;
    std::string state;
};

class AnimalBuffer {
public:
    void Clear() { snaps_.clear(); }

    void Push(uint32_t localMs, const net::AnimalState& state) {
        if (!snaps_.empty() && localMs < snaps_.back().timeMs) return;
        if (!snaps_.empty() && localMs == snaps_.back().timeMs) {
            snaps_.back().state = state;
            return;
        }
        snaps_.push_back(Snap{localMs, state});
        while (snaps_.size() > 1 && snaps_.back().timeMs - snaps_.front().timeMs > kInterpHistoryMs) snaps_.pop_front();
    }

    // 100 ms delay. Position and yaw lerp. State, health, and flags come from the newer snapshot.
    bool Sample(uint32_t renderMs, AnimalSample& out) const {
        if (snaps_.empty()) return false;
        const uint32_t sampleMs = renderMs > kInterpDelayMs ? renderMs - kInterpDelayMs : 0;
        const Snap& newest = snaps_.back();
        if (snaps_.size() == 1 || sampleMs >= newest.timeMs) return Copy(newest.state, out);
        if (sampleMs <= snaps_.front().timeMs) return Copy(snaps_.front().state, out);

        size_t hi = 1;
        while (hi + 1 < snaps_.size() && snaps_[hi].timeMs < sampleMs) ++hi;
        const Snap& a = snaps_[hi - 1];
        const Snap& b = snaps_[hi];
        const uint32_t span = b.timeMs - a.timeMs;
        const float t = span == 0 ? 1.f : static_cast<float>(sampleMs - a.timeMs) / static_cast<float>(span);
        for (int i = 0; i < 3; ++i) out.pos[i] = a.state.pos[i] + (b.state.pos[i] - a.state.pos[i]) * t;
        out.yaw = LerpYaw(a.state.yaw, b.state.yaw, t);
        out.health = b.state.health;
        out.flags = b.state.flags;
        out.state = b.state.state;
        return true;
    }

private:
    struct Snap {
        uint32_t timeMs = 0;
        net::AnimalState state{};
    };

    static bool Copy(const net::AnimalState& s, AnimalSample& out) {
        out.pos[0] = s.pos[0];
        out.pos[1] = s.pos[1];
        out.pos[2] = s.pos[2];
        out.yaw = s.yaw;
        out.health = s.health;
        out.flags = s.flags;
        out.state = s.state;
        return true;
    }

    std::deque<Snap> snaps_;
};

struct YawTrack {
    Vec3 prev{};
    float yaw = 0.f;
    bool have = false;
};

struct Mirror {
    AnimalBuffer buffer;
    bool frozen = false;
    bool freeze_logged = false;
    bool missing_logged = false;
    float rel[6] = {};
    bool have_rel = false;
    std::string applied_state;
    std::string warned_state;
    float last_host_hp = 0.f;
    bool have_hp = false;
    int place_burst = 0;
    bool have_first = false;
    entity::Vec3 first_pos{};
    float first_yaw = 0.f;
    entity::Vec3 last_target{};
    float last_yaw = 0.f;
    bool have_target = false;
    bool death_presented = false;
};

void* FindAnimal(uint16_t id) {
    for (void* a : Animals()) {
        int16_t eid = -1;
        if (core::safe_read(static_cast<char*>(a) + kEntId, eid) && eid >= 0 && static_cast<uint16_t>(eid) == id)
            return a;
    }
    return nullptr;
}

bool CaptureRel(void* ent, float rel[6]) {
    float box[6] = {};
    Vec3 p{};
    if (!core::safe_copy(static_cast<char*>(ent) + kEntWorldBox, box, sizeof box)) return false;
    if (!core::safe_read(static_cast<char*>(ent) + kEntPos, p)) return false;
    const float* c = &p.x;
    for (int i = 0; i < 3; ++i) {
        rel[i] = box[i] - c[i];
        rel[i + 3] = box[i + 3] - c[i];
    }
    return true;
}

void PlaceEntity(void* ent, const entity::Vec3& pos, float yawDeg, const float rel[6]) {
    char buf[96];
    std::snprintf(buf, sizeof buf, "%.2f %.2f %.2f", pos.x * 100.f, pos.y * 100.f, pos.z * 100.f);
    entity::HandleKey(ent, "position", buf);
    std::snprintf(buf, sizeof buf, "0 %.2f 0", yawDeg);
    entity::HandleKey(ent, "angle", buf);
    float* box = reinterpret_cast<float*>(static_cast<char*>(ent) + kEntWorldBox);
    box[0] = pos.x + rel[0];
    box[1] = pos.y + rel[1];
    box[2] = pos.z + rel[2];
    box[3] = pos.x + rel[3];
    box[4] = pos.y + rel[4];
    box[5] = pos.z + rel[5];
    entity::BspUpdate(ent);
}

class AnimalsModule : public dh2::Module {
public:
    const char* Name() const override { return "animals"; }
    int Priority() const override { return 46; }

    void OnInit() override {
        g_log_states = GetPrivateProfileIntA("Animals", "LogStates", 1, ini_path().c_str()) != 0;
        g_sync_log = GetPrivateProfileIntA("Animals", "SyncLog", 1, ini_path().c_str()) != 0;
        g_spike = GetPrivateProfileIntA("Animals", "Spike", 0, ini_path().c_str()) != 0;
        g_spike_unlink = GetPrivateProfileIntA("Animals", "SpikeUnlink", 1, ini_path().c_str()) != 0;
        char y[32] = {};
        GetPrivateProfileStringA("Animals", "SpikeYOffset", "0.7", y, sizeof y, ini_path().c_str());
        g_spike_y = static_cast<float>(std::atof(y));
        spdlog::info("animals: logStates={} syncLog={} spike={}", g_log_states, g_sync_log, g_spike);
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
        state_t_ = 0.f;
        mirrors_.clear();
        yaws_.clear();
        dead_.clear();
        announced_.clear();
        send_started_ = false;
        logged_tx_ = false;
        logged_rx_ = false;
        log_sync_ = 2.f;
    }

    void OnFrame(float dt) override {
        const auto& local = LocalPlayer();
        if (!local.valid) return;
        const auto& animals = Animals();
        if (g_log_states) LogStates(animals);
        if (session_active_) {
            if (g_sync_log) {
                log_sync_ += dt;
                if (log_sync_ >= 2.f) {
                    log_sync_ = 0.f;
                    LogSync(animals);
                }
            }
            return;
        }
        if (g_spike) Spike(dt, local, animals);
    }

    void SetSessionActive(bool active, bool hosting) {
        if (session_active_ && !active) Reset();
        if (active && !session_active_ && spike_)
            spdlog::warn("animals: session started while the solo spike held a deer; that deer stays as the spike left it");
        session_active_ = active;
        hosting_ = hosting;
    }

    void HostTick(uint32_t nowMs) {
        if (!session_active_ || !hosting_) return;
        if (send_started_ && nowMs - last_send_ms_ < net::kStateIntervalMs) return;
        send_started_ = true;
        last_send_ms_ = nowMs;

        net::AnimalStates msg;
        msg.seq = ++seq_;
        msg.tickMs = nowMs;
        for (void* a : Animals()) {
            if (msg.animals.size() >= net::kMaxAnimals) break;
            int16_t id = -1;
            Vec3 p{};
            Vec3 ang{};
            float hp = 0.f;
            if (!core::safe_read(static_cast<char*>(a) + kEntId, id) || id < 0) continue;
            if (!core::safe_read(static_cast<char*>(a) + kEntPos, p)) continue;
            core::safe_read(static_cast<char*>(a) + kEntAngle, ang);
            core::safe_read(static_cast<char*>(a) + kEntHealth, hp);

            const auto uid = static_cast<uint16_t>(id);
            YawTrack& tr = yaws_[uid];
            if (!tr.have) {
                tr.have = true;
                tr.prev = p;
                tr.yaw = NormYaw(ang.y);
            } else {
                const float dx = p.x - tr.prev.x;
                const float dz = p.z - tr.prev.z;
                if (dx * dx + dz * dz > kYawMoveM * kYawMoveM) tr.yaw = NormYaw(std::atan2(dx, dz) * 57.2957795f);
                tr.prev = p;
            }

            net::AnimalState st;
            st.id = uid;
            st.pos[0] = p.x;
            st.pos[1] = p.y;
            st.pos[2] = p.z;
            st.yaw = tr.yaw;
            st.health = hp;
            st.state = entity::BodyStateName(a);
            if (hp <= 0.f) st.flags |= net::animal_flag::kDead;
            if (p.y < -1000.f) st.flags |= net::animal_flag::kParked;
            if ((st.flags & net::animal_flag::kDead) != 0 && (st.flags & net::animal_flag::kParked) == 0 &&
                announced_.insert(uid).second) {
                spdlog::info("animals: death commit id={} hp={:.1f} state='{}'", uid, hp, st.state);
                events::Commit(events::kAnimalDied, uid, 0, hp, "", 0);
            }
            msg.animals.push_back(std::move(st));
        }
        if (!logged_tx_ && !msg.animals.empty()) {
            logged_tx_ = true;
            spdlog::info("animals: host snapshot {} animals seq={}", msg.animals.size(), msg.seq);
        }
        if (!BroadcastAnimalStates(msg) && !logged_send_fail_) {
            logged_send_fail_ = true;
            spdlog::warn("animals: SendAnimalStates failed ({} animals)", msg.animals.size());
        }
    }

    void OnAnimalStates(const net::AnimalStates& states, uint32_t localMs) {
        if (hosting_ || !session_active_) return;
        for (const net::AnimalState& a : states.animals) mirrors_[a.id].buffer.Push(localMs, a);
        if (!logged_rx_ && !states.animals.empty()) {
            logged_rx_ = true;
            spdlog::info("animals: client received {} animals seq={}", states.animals.size(), states.seq);
        }
    }

    void OnAnimalHit(const net::AnimalHit& hit) {
        if (!hosting_) {
            spdlog::warn("animals: AnimalHit ignored (not host) id={} dmg={:.2f}", hit.animalId, hit.damage);
            return;
        }
        if (!(hit.damage > 0.f)) return;
        void* ent = FindAnimal(hit.animalId);
        if (!ent) {
            spdlog::warn("animals: AnimalHit id={} shooter={} dmg={:.2f} no entity", hit.animalId, hit.shooterId,
                         hit.damage);
            return;
        }
        float before = 0.f;
        core::safe_read(static_cast<char*>(ent) + kEntHealth, before);
        const bool ok = entity::InflictDamage(ent, hit.damage);
        float after = before;
        core::safe_read(static_cast<char*>(ent) + kEntHealth, after);
        spdlog::info("animals: AnimalHit id={} shooter={} dmg={:.2f} hp {:.1f}->{:.1f} ok={}", hit.animalId,
                     hit.shooterId, hit.damage, before, after, ok);
    }

    void Reset() {
        for (auto& [id, m] : mirrors_) {
            if (!m.frozen) continue;
            if (void* ent = FindAnimal(id)) entity::FreezeBrain(ent, false);
        }
        mirrors_.clear();
        yaws_.clear();
        dead_.clear();
        announced_.clear();
        send_started_ = false;
        logged_tx_ = false;
        logged_rx_ = false;
    }

    void NoteDeath(uint16_t id) {
        if (hosting_ || !session_active_) return;
        if (dead_.insert(id).second) spdlog::info("animals: death event id={}", id);
    }

    void Place() {
        if (session_active_) {
            if (!hosting_) ApplyMirror();
            return;
        }
        PlaceSpike();
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
            Vec3 p{};
            float hp = 0.f;
            core::safe_read(static_cast<char*>(a) + kEntId, id);
            core::safe_read(static_cast<char*>(a) + kEntPos, p);
            core::safe_read(static_cast<char*>(a) + kEntHealth, hp);
            spdlog::info("animals: id={} state='{}' pos=({:.1f},{:.1f},{:.1f}) hp={:.0f}", id, st, p.x, p.y, p.z, hp);
        }
    }

    void LogSync(const std::vector<void*>& animals) {
        const char* role = hosting_ ? "host" : "client";
        for (void* a : animals) {
            int16_t id = -1;
            Vec3 p{};
            float hp = 0.f;
            if (!core::safe_read(static_cast<char*>(a) + kEntId, id)) continue;
            if (!core::safe_read(static_cast<char*>(a) + kEntPos, p)) continue;
            core::safe_read(static_cast<char*>(a) + kEntHealth, hp);
            const std::string st = entity::BodyStateName(a);
            float yaw = 0.f;
            if (hosting_) {
                auto it = yaws_.find(static_cast<uint16_t>(id));
                if (it != yaws_.end()) yaw = it->second.yaw;
                spdlog::info("animals: sync role={} id={} pos=({:.2f},{:.2f},{:.2f}) state='{}' hp={:.1f} yaw={:.1f}",
                             role, id, p.x, p.y, p.z, st, hp, yaw);
            } else {
                auto it = mirrors_.find(static_cast<uint16_t>(id));
                if (it != mirrors_.end() && it->second.have_target) {
                    const Mirror& m = it->second;
                    yaw = m.last_yaw;
                    const float drift = std::fabs(p.x - m.last_target.x) + std::fabs(p.z - m.last_target.z);
                    spdlog::info(
                        "animals: sync role={} id={} pos=({:.2f},{:.2f},{:.2f}) state='{}' hp={:.1f} yaw={:.1f} "
                        "target=({:.2f},{:.2f},{:.2f}) drift={:.3f}",
                        role, id, p.x, p.y, p.z, st, hp, yaw, m.last_target.x, m.last_target.y, m.last_target.z, drift);
                } else {
                    spdlog::info("animals: sync role={} id={} pos=({:.2f},{:.2f},{:.2f}) state='{}' hp={:.1f} yaw={:.1f}",
                                 role, id, p.x, p.y, p.z, st, hp, yaw);
                }
            }
        }
    }

    void ApplyMirror() {
        const uint32_t t = now_ms();
        for (uint16_t id : dead_) mirrors_[id];
        for (auto& [id, m] : mirrors_) {
            void* ent = FindAnimal(id);
            if (!ent) {
                if (!m.missing_logged) {
                    m.missing_logged = true;
                    spdlog::info("animals: mirror id={} waiting for a local entity", id);
                }
                continue;
            }
            if (!m.frozen) {
                const bool ok = entity::FreezeBrain(ent, true);
                if (!m.freeze_logged) {
                    m.freeze_logged = true;
                    spdlog::info("animals: mirror freeze id={} ok={} state='{}'", id, ok, entity::BodyStateName(ent));
                }
                if (!ok) continue;
                m.frozen = true;
                m.have_rel = CaptureRel(ent, m.rel);
                if (!m.have_rel) spdlog::warn("animals: mirror id={} world box unreadable", id);
            }
            AnimalSample sample;
            if (!m.buffer.Sample(t, sample)) continue;

            float localHp = 0.f;
            if (core::safe_read(static_cast<char*>(ent) + kEntHealth, localHp) && m.have_hp &&
                localHp < m.last_host_hp - 0.01f) {
                net::AnimalHit hit;
                hit.animalId = id;
                hit.damage = m.last_host_hp - localHp;
                const bool sent = ReportAnimalHit(hit);
                spdlog::info("animals: local hit id={} dmg={:.2f} localHp={:.2f} hostHp={:.2f} sent={}", id, hit.damage,
                             localHp, m.last_host_hp, sent);
            }
            *reinterpret_cast<float*>(static_cast<char*>(ent) + kEntHealth) = sample.health;
            m.last_host_hp = sample.health;
            m.have_hp = true;

            const bool parked = (sample.flags & net::animal_flag::kParked) != 0 || sample.pos[1] < -1000.f;
            const bool dead = !parked && ((sample.flags & net::animal_flag::kDead) != 0 || sample.health <= 0.f);
            if (dead) dead_.insert(id);
            if (dead_.count(id) != 0) {
                // One place at the corpse, then the death vfunc. Later frames must not HandleKey or
                // SetBodyState: that pins the standing pose. "Dead" is not the fall clip.
                if (!m.death_presented) {
                    entity::Vec3 pos{sample.pos[0], sample.pos[1], sample.pos[2]};
                    if (m.have_rel) PlaceEntity(ent, pos, sample.yaw, m.rel);
                    m.last_target = pos;
                    m.last_yaw = sample.yaw;
                    m.have_target = true;
                    m.death_presented = entity::PresentDeath(ent);
                    spdlog::info("animals: present death id={} ok={} pos=({:.1f},{:.1f},{:.1f})", id, m.death_presented,
                                 pos.x, pos.y, pos.z);
                }
                continue;
            }

            entity::Vec3 pos{sample.pos[0], sample.pos[1], sample.pos[2]};
            float yaw = sample.yaw;
            if (!m.have_first) {
                Vec3 spawn{};
                core::safe_read(static_cast<char*>(ent) + kEntPos, spawn);
                m.have_first = true;
                m.first_pos = pos;
                m.first_yaw = yaw;
                m.place_burst = kTeleportRepeats;
                spdlog::info(
                    "animals: mirror first place id={} spawn=({:.1f},{:.1f},{:.1f}) target=({:.1f},{:.1f},{:.1f}) "
                    "yaw={:.1f} (repeat {} frames)",
                    id, spawn.x, spawn.y, spawn.z, pos.x, pos.y, pos.z, yaw, kTeleportRepeats);
            }
            if (m.place_burst > 0) {
                pos = m.first_pos;
                yaw = m.first_yaw;
                --m.place_burst;
            }
            if (m.have_rel) PlaceEntity(ent, pos, yaw, m.rel);
            m.last_target = pos;
            m.last_yaw = yaw;
            m.have_target = true;

            // Re-apply when the live name drifts. FreezeBrain stops pathing, but the state machine can
            // still leave the host's state (deer 121 showed IdleAttack for ~4 s while the host stayed CutScene).
            if (!sample.state.empty() && entity::BodyStateName(ent) != sample.state) {
                const int r = entity::SetBodyState(ent, sample.state.c_str(), false);
                if (r == 1) {
                    m.applied_state = sample.state;
                } else if (sample.state != m.warned_state) {
                    m.warned_state = sample.state;
                    spdlog::warn("animals: SetBodyState id={} '{}' -> {}", id, sample.state, r);
                }
            }
        }
    }

    void Spike(float dt, const LocalPlayerSample& local, const std::vector<void*>& animals) {
        (void)local;
        if (!spike_) {
            wait_ += dt;
            if (wait_ < 8.f) return;
            for (void* a : animals) {
                Vec3 p{};
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
            Vec3 p{};
            core::safe_copy(static_cast<char*>(spike_) + kEntWorldBox, box, sizeof box);
            core::safe_read(static_cast<char*>(spike_) + kEntPos, p);
            for (int i = 0; i < 3; ++i) {
                rel_[i] = box[i] - (&p.x)[i];
                rel_[i + 3] = box[i + 3] - (&p.x)[i];
            }
            int16_t id = -1;
            core::safe_read(static_cast<char*>(spike_) + kEntId, id);
            spdlog::info(
                "animals: spike took id={} frozen={} unlinked={} state='{}' box rel ({:.2f},{:.2f},{:.2f})-({:.2f},{:.2f},{:.2f})",
                id, frozen, unlinked, entity::BodyStateName(spike_), rel_[0], rel_[1], rel_[2], rel_[3], rel_[4],
                rel_[5]);
        }
        t_ += dt;
        const float drift = drift_;
        const entity::Vec3 pos = pos_;
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

    void PlaceSpike() {
        if (!spike_) return;
        const auto& local = LocalPlayer();
        if (!local.valid) return;
        Vec3 now{};
        core::safe_read(static_cast<char*>(spike_) + kEntPos, now);
        const float drift = std::fabs(now.x - last_.x) + std::fabs(now.z - last_.z);
        const float vy = (local.viewValid ? local.viewYaw : local.angle.y) * 3.14159265f / 180.f;
        const float cx = local.pos.x + 8.f * std::sin(vy), cz = local.pos.z + 8.f * std::cos(vy);
        const float a = t_ * 0.5f;
        const entity::Vec3 pos{cx + 3.f * std::sin(a), local.pos.y + g_spike_y, cz + 3.f * std::cos(a)};
        const float heading = (a + 1.5707963f) * 57.2957795f;
        if (g_spike_unlink) {
            entity::SetPos(spike_, pos);
            entity::SetAngle(spike_, {0.f, heading, 0.f});
        } else {
            PlaceEntity(spike_, pos, heading, rel_);
        }
        if (g_spike_unlink) {
            float* box = reinterpret_cast<float*>(static_cast<char*>(spike_) + kEntWorldBox);
            box[0] = pos.x + rel_[0];
            box[1] = pos.y + rel_[1];
            box[2] = pos.z + rel_[2];
            box[3] = pos.x + rel_[3];
            box[4] = pos.y + rel_[4];
            box[5] = pos.z + rel_[5];
            entity::BspUpdate(spike_);
        }
        last_ = {pos.x, pos.y, pos.z};
        g_watch.store(spike_);
        if (++diag_frames_ >= 50) {
            int link = -2;
            core::safe_read(static_cast<char*>(spike_) + 0x124, link);
            float clock = std::fmod(a * 57.2957795f, 360.f);
            spdlog::info("animals: diag circle={:.0f}deg draws/50f={} bsplink={} pos=({:.1f},{:.1f},{:.1f})", clock,
                         g_draws.exchange(0), link, pos.x, pos.y, pos.z);
            diag_frames_ = 0;
        }
        drift_ = drift;
        pos_ = pos;
    }

    std::map<void*, std::string> states_;
    std::set<std::string> seen_;
    void* spike_ = nullptr;
    float wait_ = 0.f, t_ = 0.f, state_t_ = 0.f;
    size_t state_idx_ = 0;
    float rel_[6] = {};
    Vec3 last_{};
    float drift_ = 0.f;
    int diag_frames_ = 0;
    entity::Vec3 pos_{};

    bool session_active_ = false;
    bool hosting_ = false;
    bool send_started_ = false;
    bool logged_tx_ = false;
    bool logged_rx_ = false;
    bool logged_send_fail_ = false;
    uint32_t last_send_ms_ = 0;
    uint16_t seq_ = 0;
    float log_sync_ = 2.f;
    std::map<uint16_t, YawTrack> yaws_;
    std::map<uint16_t, Mirror> mirrors_;
    std::set<uint16_t> dead_;
    std::set<uint16_t> announced_;
};

AnimalsModule* g_self = nullptr;
void prerender() {
    if (g_self) g_self->Place();
}

DH2_MODULE(AnimalsModule)

}  // namespace

void HostTick(uint32_t nowMs) {
    if (g_self) g_self->HostTick(nowMs);
}
void OnAnimalStates(const net::AnimalStates& states, uint32_t localMs) {
    if (g_self) g_self->OnAnimalStates(states, localMs);
}
void OnAnimalHit(const net::AnimalHit& hit) {
    if (g_self) g_self->OnAnimalHit(hit);
}
void OnAnimalDied(uint16_t id) {
    if (g_self) g_self->NoteDeath(id);
}
void Reset() {
    if (g_self) g_self->Reset();
}
void SetSessionActive(bool active, bool hosting) {
    if (g_self) g_self->SetSessionActive(active, hosting);
}
void* Find(uint16_t id) { return FindAnimal(id); }
int IdOf(void* animal) {
    int16_t id = -1;
    if (!animal || !core::safe_read(static_cast<char*>(animal) + kEntId, id)) return -1;
    return id;
}
bool DebugDamage(uint16_t id, float damage) {
    void* ent = FindAnimal(id);
    if (!ent) {
        spdlog::warn("animals: coop_kill id={} no entity", id);
        return false;
    }
    float before = 0.f, after = 0.f;
    core::safe_read(static_cast<char*>(ent) + kEntHealth, before);
    const bool ok = entity::InflictDamage(ent, damage);
    core::safe_read(static_cast<char*>(ent) + kEntHealth, after);
    spdlog::info("animals: coop_kill id={} dmg={:.1f} hp {:.1f}->{:.1f} ok={}", id, damage, before, after, ok);
    return ok;
}

}  // namespace dh2::coop::animals
