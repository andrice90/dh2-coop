// UPnP port forward glue (net/upnp.*): asks the home router to forward the game's UDP port while this process
// hosts, and takes the forward away again when hosting stops or the game exits. Everything slow runs on the
// worker thread in net/upnp.cpp; this module only watches the host state and owns the exit hook.
// Ini: [Coop] UPnP, UPnPLeaseSec, UPnPOnLoopback. Console: coop_upnp [off|on|clean|retry]. re/notes/upnp.md.

#include <windows.h>
#include <intrin.h>

#include <atomic>
#include <chrono>
#include <string>

#include <spdlog/spdlog.h>

#include "coop/coop.h"
#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/module.h"
#include "core/safe_mem.h"
#include "engine/console_cmds.h"
#include "net/upnp.h"

namespace dh2::coop {
namespace {

bool g_enabled = true;
bool g_loopback_bind = false;     // [Coop] BindAddress is 127.x: a local test copy, no one can join from outside
bool g_on_loopback = false;       // [Coop] UPnPOnLoopback=1 (test only): run the router flow anyway
uint16_t g_last_want = 0;
bool g_loopback_logged = false;
std::atomic<bool> g_exiting{false};
safetyhook::InlineHook* g_exit_hook = nullptr;

uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

// The one place that sees every normal way out of the game (window X, main menu Quit, the Exit console command):
// the CRT ends in ExitProcess on the thread that quit, with every other thread still alive. The lifecycle thread's
// OnShutdown never runs in a normal exit (ExitProcess kills that thread before DLL_PROCESS_DETACH), and nothing may
// do COM work inside DllMain, so the removal has to start here. Bounded: quitting is never held up for long.
void WINAPI hk_ExitProcess(UINT code) {
    if (!g_exiting.exchange(true)) {
        spdlog::info("upnp: ExitProcess({}) called from {}: removing the port forward", code,
                     core::describe_address(_ReturnAddress()));
        net::upnp::Shutdown(1500);
    }
    g_exit_hook->unsafe_stdcall<void>(code);
}

const char* state_name(net::upnp::State s) {
    switch (s) {
        case net::upnp::State::Off: return "off";
        case net::upnp::State::Working: return "working";
        case net::upnp::State::Open: return "open";
        default: return "failed";
    }
}

void __cdecl cmd_upnp(const char* arg) {
    const std::string a = arg ? core::safe_cstr(arg, 32) : std::string();
    if (a == "off") {
        net::upnp::SetUserOff(true);
    } else if (a == "on") {
        net::upnp::SetUserOff(false);
    } else if (a == "clean") {
        net::upnp::CleanStale();
    } else if (a == "retry") {
        net::upnp::Refresh();
    } else if (!a.empty()) {
        spdlog::warn("coop_upnp: usage coop_upnp [off | on | clean | retry]");
        return;
    }
    const net::upnp::Status s = net::upnp::GetStatus();
    spdlog::info("coop_upnp: {} port {}->{} wan {} lease {}s mapped={} | {}", state_name(s.state), s.externalPort,
                 s.internalPort, s.publicIp.empty() ? std::string("-") : net::upnp::MaskIp(s.publicIp), s.leaseSec,
                 s.mapped, s.message);
}

class UpnpModule : public dh2::Module {
public:
    const char* Name() const override { return "upnp"; }
    int Priority() const override { return 47; }

    void OnInit() override {
        const std::string ini = (core::self_dir() / "dh2coop.ini").string();
        char bind[64] = {};
        GetPrivateProfileStringA("Coop", "BindAddress", "", bind, sizeof bind, ini.c_str());
        g_enabled = GetPrivateProfileIntA("Coop", "UPnP", 1, ini.c_str()) != 0;
        g_on_loopback = GetPrivateProfileIntA("Coop", "UPnPOnLoopback", 0, ini.c_str()) != 0;
        g_loopback_bind = std::string(bind).rfind("127.", 0) == 0;

        net::upnp::Config cfg;
        cfg.enabled = g_enabled;
        cfg.leaseSec = net::upnp::NormalizeLease(
            static_cast<int>(GetPrivateProfileIntA("Coop", "UPnPLeaseSec", 3600, ini.c_str())));
        cfg.stateFile = (core::log_dir() / "dh2coop-upnp.state").string();
        if (bind[0] && !g_loopback_bind) cfg.bindAddress = bind;
        net::upnp::Configure(cfg);
        if (!g_enabled) net::upnp::NoteOff("Internet: off ([Coop] UPnP=0)");

        spdlog::info("upnp: UPnP={} UPnPLeaseSec={} UPnPOnLoopback={} (bind '{}')", g_enabled, cfg.leaseSec,
                     g_on_loopback, bind);

        HMODULE k = GetModuleHandleA("kernel32.dll");
        void* target = k ? reinterpret_cast<void*>(GetProcAddress(k, "ExitProcess")) : nullptr;
        g_exit_hook = core::install_hook("ExitProcess (remove the UPnP port forward)", target,
                                         reinterpret_cast<void*>(&hk_ExitProcess));
    }

    void OnFrame(float) override {
        if (!commands_registered_ && console_cmds::Ready()) {
            commands_registered_ = true;
            console_cmds::AddCommandStr("coop_upnp", &cmd_upnp);
        }
        const uint32_t now = now_ms();
        if (now - last_poll_ < 250) return;
        last_poll_ = now;

        uint16_t want = NetHostPort();
        if (want != 0 && g_loopback_bind && !g_on_loopback) {
            // A test copy bound to 127.0.0.1: no friend can reach it, and a mapping would point at nothing.
            if (!g_loopback_logged) {
                g_loopback_logged = true;
                spdlog::info("upnp: skipped, sockets bind to loopback only (set [Coop] UPnPOnLoopback=1 to test)");
                net::upnp::NoteOff("Internet: off (this copy only listens on this PC)");
            }
            want = 0;
        }
        if (want == g_last_want) return;
        g_last_want = want;
        if (want != 0) spdlog::info("upnp: hosting on UDP {}", want);
        else spdlog::info("upnp: no longer hosting");
        net::upnp::Want(want);
    }

    // The lifecycle thread: a no-op after the ExitProcess hook already did the work.
    void OnShutdown() override { net::upnp::Shutdown(1500); }

private:
    bool commands_registered_ = false;
    uint32_t last_poll_ = 0;
};

DH2_MODULE(UpnpModule)

}  // namespace
}  // namespace dh2::coop
