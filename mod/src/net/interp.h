#pragma once

// Per-remote-player snapshot buffer. One instance per remote id.
// Sample at renderTimeMs - delayMs. Degrees for yaw. See docs/net-protocol.md.

#include "net/protocol.h"

#include <cstddef>
#include <cstdint>
#include <deque>

namespace net {

constexpr uint32_t kInterpDelayMs = 100;
constexpr uint32_t kInterpExtrapolateCapMs = 250;
constexpr uint32_t kInterpHistoryMs = 1000;

struct InterpConfig {
    uint32_t delayMs = kInterpDelayMs;
    uint32_t extrapolateCapMs = kInterpExtrapolateCapMs;
    uint32_t historyMs = kInterpHistoryMs;
};

struct InterpSample {
    float pos[3] = {};
    float yaw = 0.f;
    float pitch = 0.f;
    float velocity[3] = {};
    uint16_t flags = 0;
    uint8_t weaponId = 0;
    uint8_t health = 0;
    bool extrapolated = false;
};

class SnapshotBuffer {
public:
    SnapshotBuffer();
    explicit SnapshotBuffer(InterpConfig cfg);

    void Clear();
    // `localTimeMs` is the receiver's steady clock at arrival, not PlayerState.tickMs.
    // A sample older than the newest one is dropped.
    void Push(uint32_t localTimeMs, const PlayerState& state);
    bool Sample(uint32_t renderTimeMs, InterpSample& out) const;

    size_t size() const { return snaps_.size(); }
    uint32_t oldest_ms() const { return snaps_.empty() ? 0 : snaps_.front().timeMs; }
    uint32_t newest_ms() const { return snaps_.empty() ? 0 : snaps_.back().timeMs; }

private:
    struct Snap {
        uint32_t timeMs = 0;
        PlayerState state{};
    };

    InterpConfig cfg_{};
    std::deque<Snap> snaps_;
};

}  // namespace net
