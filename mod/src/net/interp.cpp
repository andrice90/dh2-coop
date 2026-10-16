#include "net/interp.h"

#include <cmath>

namespace net {
namespace {

float Wrap180(float deg) {
    // (-180, 180]. 180 and -180 are the same direction; 180 maps to -180.
    float x = std::fmod(deg + 180.f, 360.f);
    if (x < 0.f) x += 360.f;
    return x - 180.f;
}

float Lerp(float a, float b, float t) { return a + (b - a) * t; }

float LerpYaw(float a, float b, float t) {
    const float d = Wrap180(b - a);
    return Wrap180(a + d * t);
}

void CopyDiscrete(InterpSample& out, const PlayerState& s) {
    out.flags = s.flags;
    out.weaponId = s.weaponId;
    out.health = s.health;
}

void CopyContinuous(InterpSample& out, const PlayerState& s) {
    out.pos[0] = s.pos[0];
    out.pos[1] = s.pos[1];
    out.pos[2] = s.pos[2];
    out.yaw = s.yaw;
    out.pitch = s.pitch;
    out.velocity[0] = s.velocity[0];
    out.velocity[1] = s.velocity[1];
    out.velocity[2] = s.velocity[2];
}

}  // namespace

SnapshotBuffer::SnapshotBuffer() : SnapshotBuffer(InterpConfig{}) {}

SnapshotBuffer::SnapshotBuffer(InterpConfig cfg) : cfg_(cfg) {}

void SnapshotBuffer::Clear() { snaps_.clear(); }

void SnapshotBuffer::Push(uint32_t localTimeMs, const PlayerState& state) {
    if (!snaps_.empty() && localTimeMs < snaps_.back().timeMs) return;
    if (!snaps_.empty() && localTimeMs == snaps_.back().timeMs) {
        snaps_.back().state = state;
        return;
    }
    snaps_.push_back(Snap{localTimeMs, state});
    while (snaps_.size() > 1 && snaps_.back().timeMs - snaps_.front().timeMs > cfg_.historyMs) {
        snaps_.pop_front();
    }
}

bool SnapshotBuffer::Sample(uint32_t renderTimeMs, InterpSample& out) const {
    if (snaps_.empty()) return false;

    const uint32_t sampleMs = renderTimeMs > cfg_.delayMs ? (renderTimeMs - cfg_.delayMs) : 0;
    const Snap& newest = snaps_.back();

    if (sampleMs >= newest.timeMs) {
        const uint32_t ahead = sampleMs - newest.timeMs;
        const uint32_t dt = ahead > cfg_.extrapolateCapMs ? cfg_.extrapolateCapMs : ahead;
        const float sec = static_cast<float>(dt) / 1000.f;
        const PlayerState& s = newest.state;
        out.pos[0] = s.pos[0] + s.velocity[0] * sec;
        out.pos[1] = s.pos[1] + s.velocity[1] * sec;
        out.pos[2] = s.pos[2] + s.velocity[2] * sec;
        out.yaw = s.yaw;
        out.pitch = s.pitch;
        out.velocity[0] = s.velocity[0];
        out.velocity[1] = s.velocity[1];
        out.velocity[2] = s.velocity[2];
        CopyDiscrete(out, s);
        out.extrapolated = dt > 0;
        return true;
    }

    if (sampleMs <= snaps_.front().timeMs) {
        CopyContinuous(out, snaps_.front().state);
        CopyDiscrete(out, snaps_.front().state);
        out.extrapolated = false;
        return true;
    }

    size_t hi = 1;
    while (hi + 1 < snaps_.size() && snaps_[hi].timeMs < sampleMs) ++hi;
    const Snap& a = snaps_[hi - 1];
    const Snap& b = snaps_[hi];
    const uint32_t span = b.timeMs - a.timeMs;
    const float t = span == 0 ? 1.f : static_cast<float>(sampleMs - a.timeMs) / static_cast<float>(span);
    out.pos[0] = Lerp(a.state.pos[0], b.state.pos[0], t);
    out.pos[1] = Lerp(a.state.pos[1], b.state.pos[1], t);
    out.pos[2] = Lerp(a.state.pos[2], b.state.pos[2], t);
    out.yaw = LerpYaw(a.state.yaw, b.state.yaw, t);
    out.pitch = Lerp(a.state.pitch, b.state.pitch, t);
    out.velocity[0] = Lerp(a.state.velocity[0], b.state.velocity[0], t);
    out.velocity[1] = Lerp(a.state.velocity[1], b.state.velocity[1], t);
    out.velocity[2] = Lerp(a.state.velocity[2], b.state.velocity[2], t);
    // Flags, weapon, and health stay on the older snapshot until the sample time reaches the newer one.
    CopyDiscrete(out, sampleMs >= b.timeMs ? b.state : a.state);
    out.extrapolated = false;
    return true;
}

}  // namespace net
