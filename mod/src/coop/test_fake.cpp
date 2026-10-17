#include "coop/test_fake.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <spdlog/spdlog.h>

#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/console_cmds.h"

namespace dh2::coop::testfake {
namespace {

// One console token per command, so pairs ride as "a:b". "off" clears the one setting.
struct Fake {
    int weapon = -1;
    bool have_pitch = false, have_yaw = false;
    float pitch = 0.f, yaw = 0.f;
    int crouch = -1, reload = -1;
    int shots = -1;  // -1 = real counter; otherwise the fake counter (bumped by coop_fakeshot)
    bool have_off = false;
    float off_x = 0.f, off_z = 0.f;
    float walk_speed = 0.f, walk_dir = 0.f, walk_dist = 0.f;
    uint32_t last_tick = 0;
} g;

constexpr float kWalkSpan = 30.f;  // metres before the walk starts over at its origin

std::string arg_text(const char* a) {
    std::string s = core::safe_cstr(a, 64);
    for (char& c : s) {
        if (c == ':' || c == ',') c = ' ';
    }
    return s;
}
bool is_off(const std::string& s) { return s.rfind("off", 0) == 0 || s.empty(); }

void __cdecl cmd_weapon(const char* a) {
    const std::string s = arg_text(a);
    g.weapon = is_off(s) ? -1 : std::atoi(s.c_str());
    spdlog::info("fake: weapon = {}", g.weapon);
}
void __cdecl cmd_pitch(const char* a) {
    const std::string s = arg_text(a);
    g.have_pitch = !is_off(s);
    g.pitch = g.have_pitch ? static_cast<float>(std::atof(s.c_str())) : 0.f;
    spdlog::info("fake: pitch = {} {:.1f}", g.have_pitch, g.pitch);
}
void __cdecl cmd_yaw(const char* a) {
    const std::string s = arg_text(a);
    g.have_yaw = !is_off(s);
    g.yaw = g.have_yaw ? static_cast<float>(std::atof(s.c_str())) : 0.f;
    spdlog::info("fake: yaw = {} {:.1f}", g.have_yaw, g.yaw);
}
void __cdecl cmd_crouch(const char* a) {
    const std::string s = arg_text(a);
    g.crouch = is_off(s) ? -1 : (std::atoi(s.c_str()) != 0);
    spdlog::info("fake: crouch = {}", g.crouch);
}
void __cdecl cmd_reload(const char* a) {
    const std::string s = arg_text(a);
    g.reload = is_off(s) ? -1 : (std::atoi(s.c_str()) != 0);
    spdlog::info("fake: reload = {}", g.reload);
}
void __cdecl cmd_shot() {
    g.shots = (g.shots < 0 ? 1 : g.shots + 1) & 0x0F;
    spdlog::info("fake: shot counter = {}", g.shots);
}
void __cdecl cmd_off(const char* a) {
    const std::string s = arg_text(a);
    float x = 0.f, z = 0.f;
    g.have_off = !is_off(s) && std::sscanf(s.c_str(), "%f %f", &x, &z) == 2;
    g.off_x = g.have_off ? x : 0.f;
    g.off_z = g.have_off ? z : 0.f;
    spdlog::info("fake: offset = {} ({:.2f}, {:.2f})", g.have_off, g.off_x, g.off_z);
}
void __cdecl cmd_walk(const char* a) {
    const std::string s = arg_text(a);
    float speed = 0.f, dir = 0.f;
    if (is_off(s) || std::sscanf(s.c_str(), "%f %f", &speed, &dir) != 2) speed = 0.f;
    g.walk_speed = speed;
    g.walk_dir = dir;
    g.walk_dist = 0.f;
    spdlog::info("fake: walk {:.2f} m/s heading {:.1f}", g.walk_speed, g.walk_dir);
}
void __cdecl cmd_clear() {
    g = Fake{};
    spdlog::info("fake: cleared");
}

class FakeModule : public dh2::Module {
public:
    const char* Name() const override { return "test_fake"; }
    int Priority() const override { return 90; }
    void OnFrame(float) override {
        if (registered_ || !console_cmds::Ready()) return;
        registered_ = true;
        console_cmds::AddCommandStr("coop_fakeweapon", &cmd_weapon);
        console_cmds::AddCommandStr("coop_fakepitch", &cmd_pitch);
        console_cmds::AddCommandStr("coop_fakeyaw", &cmd_yaw);
        console_cmds::AddCommandStr("coop_fakecrouch", &cmd_crouch);
        console_cmds::AddCommandStr("coop_fakereload", &cmd_reload);
        console_cmds::AddCommand0("coop_fakeshot", &cmd_shot);
        console_cmds::AddCommandStr("coop_fakeoff", &cmd_off);
        console_cmds::AddCommandStr("coop_fakewalk", &cmd_walk);
        console_cmds::AddCommand0("coop_fakeclear", &cmd_clear);
    }

private:
    bool registered_ = false;
};
DH2_MODULE(FakeModule)

}  // namespace

void Apply(net::PlayerState& st) {
    if (g.weapon >= 0) st.weaponId = static_cast<uint8_t>(g.weapon);
    if (g.have_pitch) st.pitch = g.pitch;
    if (g.have_yaw) st.yaw = g.yaw < 0.f ? g.yaw + 360.f : g.yaw;
    if (g.crouch >= 0) st.flags = static_cast<uint16_t>(g.crouch ? (st.flags | net::kFlagCrouched) : (st.flags & ~net::kFlagCrouched));
    if (g.reload >= 0) st.flags = static_cast<uint16_t>(g.reload ? (st.flags | net::kFlagReloading) : (st.flags & ~net::kFlagReloading));
    if (g.shots >= 0) {
        st.flags = static_cast<uint16_t>((st.flags & ~net::kShotCounterMask) |
                                         ((g.shots << net::kShotCounterShift) & net::kShotCounterMask));
    }
    if (g.have_off || g.walk_speed != 0.f) {
        const float dt = g.last_tick ? std::fmin(0.25f, static_cast<float>(st.tickMs - g.last_tick) / 1000.f) : 0.f;
        g.last_tick = st.tickMs;
        g.walk_dist += g.walk_speed * dt;
        if (g.walk_dist > kWalkSpan) g.walk_dist = 0.f;
        const float h = g.walk_dir * 3.14159265f / 180.f;
        const float dx = std::sin(h), dz = std::cos(h);  // heading convention: 0 = +z, 90 = +x
        st.pos[0] += g.off_x + dx * g.walk_dist;
        st.pos[2] += g.off_z + dz * g.walk_dist;
        st.velocity[0] = dx * g.walk_speed;
        st.velocity[1] = 0.f;
        st.velocity[2] = dz * g.walk_speed;
    }
}

}  // namespace dh2::coop::testfake
