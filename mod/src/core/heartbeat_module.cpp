#include "core/module.h"

#include <cstdint>

#include <spdlog/spdlog.h>

namespace {

class HeartbeatModule : public dh2::Module {
public:
    const char* Name() const override { return "heartbeat"; }
    int Priority() const override { return 1000; }

    void OnFrame(float dtSeconds) override {
        ++frames_;
        ++window_frames_;
        window_dt_ += dtSeconds;
        if (window_dt_ < 10.f) return;
        const float avg = window_frames_ ? window_dt_ / static_cast<float>(window_frames_) : 0.f;
        spdlog::info("heartbeat frames={} window_frames={} avg_dt={:.5f}s", frames_, window_frames_, avg);
        window_frames_ = 0;
        window_dt_ = 0.f;
    }

private:
    std::uint64_t frames_ = 0;
    std::uint32_t window_frames_ = 0;
    float window_dt_ = 0.f;
};

DH2_MODULE(HeartbeatModule)

}  // namespace
