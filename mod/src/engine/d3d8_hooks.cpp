#include "engine/d3d8_hooks.h"

#include <intrin.h>

#include <atomic>
#include <exception>
#include <mutex>
#include <vector>

#include <spdlog/spdlog.h>

#include "core/exports.h"
#include "core/hook.h"
#include "core/module.h"
#include "core/safe_mem.h"

namespace dh2::d3d8 {
namespace {

// D3D8 D3DPRESENT_PARAMETERS. d3d8.h is not in the Windows 10/11 SDK.
struct D3D8Params {
    UINT BackBufferWidth, BackBufferHeight;
    UINT BackBufferFormat;
    UINT BackBufferCount;
    UINT MultiSampleType;
    UINT SwapEffect;
    HWND hDeviceWindow;
    BOOL Windowed;
    BOOL EnableAutoDepthStencil;
    UINT AutoDepthStencilFormat;
    DWORD Flags;
    UINT FullScreen_RefreshRateInHz;
    UINT FullScreen_PresentationInterval;
};
static_assert(sizeof(D3D8Params) == 52, "D3D8 present params");

constexpr int kVtCreateDevice = 15;  // IDirect3D8
constexpr int kVtReset = 14;         // IDirect3DDevice8
constexpr int kVtPresent = 15;

using CreateFn = void*(__stdcall*)(UINT);
using CreateDeviceFn = HRESULT(__stdcall*)(void*, UINT, UINT, HWND, DWORD, void*, void**);
using PresentFn = HRESULT(__stdcall*)(void*, const RECT*, const RECT*, HWND, const void*);
using ResetFn = HRESULT(__stdcall*)(void*, void*);

safetyhook::InlineHook* g_create = nullptr;
safetyhook::InlineHook* g_create_device = nullptr;
safetyhook::InlineHook* g_present = nullptr;
safetyhook::InlineHook* g_reset = nullptr;

std::atomic<void*> g_device{nullptr};
std::atomic<HWND> g_device_window{nullptr};
std::atomic<bool> g_object_hooked{false};
std::atomic<std::uint32_t> g_n_create{0};
std::atomic<std::uint32_t> g_n_device{0};
std::atomic<std::uint32_t> g_n_reset{0};
std::atomic<std::uint64_t> g_n_present{0};
std::atomic<bool> g_device_hooked{false};
// Thread that owns the device window = the game's main loop thread (cApplication::Run pumps that window).
// The loading screen calls Present from a second thread while the main thread is inside cBspTree::Load;
// module OnFrame must never run there (lead finding 2026-10-16, re/notes/lead-player-tracker.md).
std::atomic<DWORD> g_main_tid{0};
std::atomic<bool> g_foreign_present_logged{false};

std::mutex g_cb_mu;
std::vector<PresentCallback> g_present_cbs;
std::vector<ResetCallback> g_reset_cbs;
std::vector<DeviceChangeCallback> g_device_cbs;

void notify_device_change(void* dev, int phase) {
    std::vector<DeviceChangeCallback> cbs;
    {
        std::lock_guard lock(g_cb_mu);
        cbs = g_device_cbs;
    }
    for (DeviceChangeCallback cb : cbs) {
        try {
            if (cb) cb(dev, phase);
        } catch (const std::exception& ex) {
            spdlog::error("Device change callback (phase {}) threw: {}", phase, ex.what());
        } catch (...) {
            spdlog::error("Device change callback (phase {}) threw", phase);
        }
    }
}

uintptr_t A(const void* p) { return reinterpret_cast<uintptr_t>(p); }

std::string fmt_params(const void* pp) {
    D3D8Params p{};
    if (!pp || !core::safe_read(pp, p)) return "<unreadable>";
    return fmt::format("{}x{} fmt={} windowed={} hwnd={:#x} hz={} interval={:#x}", p.BackBufferWidth,
                       p.BackBufferHeight, p.BackBufferFormat, p.Windowed, A(p.hDeviceWindow),
                       p.FullScreen_RefreshRateInHz, p.FullScreen_PresentationInterval);
}

float frame_dt() {
    static LARGE_INTEGER freq = [] {
        LARGE_INTEGER f{};
        QueryPerformanceFrequency(&f);
        return f;
    }();
    static LARGE_INTEGER last{};
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    float dt = 0.f;
    if (last.QuadPart != 0 && freq.QuadPart != 0) {
        dt = static_cast<float>(static_cast<double>(now.QuadPart - last.QuadPart) /
                                static_cast<double>(freq.QuadPart));
    }
    last = now;
    if (dt < 0.f) dt = 0.f;
    return dt;
}

void hook_device(void* dev, const char* how);

HRESULT __stdcall hk_Reset(void* self, void* pp) {
    const std::uint32_t n = ++g_n_reset;
    if (n <= 4) spdlog::info("D3D8 Device::Reset #{} dev={:#x} {}", n, A(self), fmt_params(pp));
    std::vector<ResetCallback> cbs;
    {
        std::lock_guard lock(g_cb_mu);
        cbs = g_reset_cbs;
    }
    for (ResetCallback cb : cbs) {
        try {
            if (cb) cb(self, pp, 0);
        } catch (const std::exception& ex) {
            spdlog::error("Reset callback (before) threw: {}", ex.what());
        } catch (...) {
            spdlog::error("Reset callback (before) threw");
        }
    }
    HRESULT hr = g_reset->unsafe_stdcall<HRESULT>(self, pp);
    for (ResetCallback cb : cbs) {
        try {
            if (cb) cb(self, pp, 1);
        } catch (const std::exception& ex) {
            spdlog::error("Reset callback (after) threw: {}", ex.what());
        } catch (...) {
            spdlog::error("Reset callback (after) threw");
        }
    }
    return hr;
}

HRESULT __stdcall hk_Present(void* self, const RECT* src, const RECT* dst, HWND wnd, const void* dirty) {
    const std::uint64_t n = ++g_n_present;
    if (n == 1) {
        spdlog::info("D3D8 Device::Present #1 dev={:#x} hwnd={:#x} caller={}", A(self), A(wnd),
                     core::describe_address(_ReturnAddress()));
    }
    const DWORD tid = GetCurrentThreadId();
    DWORD main = g_main_tid.load();
    if (main == 0 && wnd == nullptr) {
        // No device-window thread recorded yet: fall back to the first thread that presents.
        DWORD expected = 0;
        g_main_tid.compare_exchange_strong(expected, tid);
        main = g_main_tid.load();
    }
    if (main != 0 && tid != main) {
        bool expected = false;
        if (g_foreign_present_logged.compare_exchange_strong(expected, true)) {
            spdlog::info("D3D8 Present from non-main thread t{} (main t{}): module frame dispatch skipped", tid, main);
        }
        return g_present->unsafe_stdcall<HRESULT>(self, src, dst, wnd, dirty);
    }
    const float dt = frame_dt();
    try {
        modules::DispatchFrame(dt);
        std::vector<PresentCallback> cbs;
        {
            std::lock_guard lock(g_cb_mu);
            cbs = g_present_cbs;
        }
        for (PresentCallback cb : cbs) {
            if (cb) cb(self, src, dst, wnd, dirty);
        }
    } catch (const std::exception& ex) {
        spdlog::error("Present pre-hook threw: {}", ex.what());
    } catch (...) {
        spdlog::error("Present pre-hook threw");
    }
    return g_present->unsafe_stdcall<HRESULT>(self, src, dst, wnd, dirty);
}

HRESULT __stdcall hk_CreateDevice(void* self, UINT adapter, UINT type, HWND focus, DWORD flags, void* pp,
                                  void** out) {
    const std::uint32_t n = ++g_n_device;
    spdlog::info("D3D8 CreateDevice #{} adapter={} devtype={} behavior={:#x} caller={}", n, adapter, type, flags,
                 core::describe_address(_ReturnAddress()));
    spdlog::info("D3D8   params in : {}", fmt_params(pp));
    HWND dev_wnd = focus;
    if (pp) {
        D3D8Params params{};
        if (core::safe_read(pp, params) && params.hDeviceWindow) dev_wnd = params.hDeviceWindow;
    }
    if (dev_wnd) g_device_window.store(dev_wnd);
    if (focus) {
        const DWORD owner = GetWindowThreadProcessId(focus, nullptr);
        if (owner) {
            g_main_tid.store(owner);
            spdlog::info("D3D8   main (window) thread t{}; CreateDevice called on t{}", owner, GetCurrentThreadId());
        }
    }
    const bool replacing = g_device.load() != nullptr;
    if (replacing) notify_device_change(nullptr, 0);
    HRESULT hr = g_create_device->unsafe_stdcall<HRESULT>(self, adapter, type, focus, flags, pp, out);
    spdlog::info("D3D8   -> hr={:#010x} device={:#x} params out: {}", static_cast<std::uint32_t>(hr),
                 out ? A(*out) : 0, fmt_params(pp));
    void* dev = (SUCCEEDED(hr) && out) ? *out : nullptr;
    if (dev) {
        hook_device(dev, "CreateDevice");  // first device only; later ones share the hooked vtable code
        g_device.store(dev);
    }
    if (replacing) notify_device_change(dev, 1);
    return hr;
}

void* __stdcall hk_Direct3DCreate8(UINT sdk) {
    const std::uint32_t n = ++g_n_create;
    void* obj = g_create->unsafe_stdcall<void*>(sdk);
    spdlog::info("Direct3DCreate8(sdk={:#x}) #{} -> {:#x} caller={}", sdk, n, A(obj),
                 core::describe_address(_ReturnAddress()));
    if (obj) {
        bool expected = false;
        if (g_object_hooked.compare_exchange_strong(expected, true)) {
            void** vt = *reinterpret_cast<void***>(obj);
            g_create_device = core::install_hook("IDirect3D8::CreateDevice", vt[kVtCreateDevice],
                                                 reinterpret_cast<void*>(&hk_CreateDevice));
        }
    }
    return obj;
}

void hook_device(void* dev, const char* how) {
    bool expected = false;
    if (!dev || !g_device_hooked.compare_exchange_strong(expected, true)) return;
    g_device.store(dev);
    void** vt = *reinterpret_cast<void***>(dev);
    spdlog::info("D3D8 device {:#x} vtable {:#x} ({}) Reset={} Present={}", A(dev), A(vt), how,
                 core::describe_address(vt[kVtReset]), core::describe_address(vt[kVtPresent]));
    g_reset = core::install_hook("D3D8 Device::Reset", vt[kVtReset], reinterpret_cast<void*>(&hk_Reset));
    g_present = core::install_hook("D3D8 Device::Present", vt[kVtPresent], reinterpret_cast<void*>(&hk_Present));
    if (!g_present) g_device_hooked.store(false);
}

}  // namespace

void TryInstall() {
    if (g_create) return;
    HMODULE d3d = GetModuleHandleA("d3d8.dll");
    if (!d3d) return;
    void* target = core::find_export(d3d, "Direct3DCreate8");
    g_create = core::install_hook("Direct3DCreate8", target, reinterpret_cast<void*>(&hk_Direct3DCreate8));
    if (!g_create) g_create = reinterpret_cast<safetyhook::InlineHook*>(1);  // do not retry; never dereferenced
}

void TryLateDevice(HMODULE engine) {
    if (g_device_hooked.load() || !engine) return;
    auto** slot = core::find_export_as<void**>(engine, "?m_d3dd@cRenderDX@@2PAUIDirect3DDevice8@@A", false);
    void* dev = nullptr;
    if (!slot || !core::safe_read(slot, dev) || !dev) return;
    spdlog::info("D3D8 late attach via cRenderDX::m_d3dd={:#x}", A(dev));
    hook_device(dev, "m_d3dd static (late)");
}

void* Device() { return g_device.load(); }

HWND DeviceWindow() { return g_device_window.load(); }

Stats GetStats() {
    Stats s;
    s.create_calls = g_n_create.load();
    s.device_calls = g_n_device.load();
    s.reset_calls = g_n_reset.load();
    s.present_calls = g_n_present.load();
    s.create_hooked = g_create != nullptr && g_create != reinterpret_cast<safetyhook::InlineHook*>(1);
    s.device_hooked = g_device_hooked.load();
    return s;
}

void AddPresentCallback(PresentCallback fn) {
    if (!fn) return;
    std::lock_guard lock(g_cb_mu);
    g_present_cbs.push_back(fn);
}

void AddResetCallback(ResetCallback fn) {
    if (!fn) return;
    std::lock_guard lock(g_cb_mu);
    g_reset_cbs.push_back(fn);
}

void AddDeviceChangeCallback(DeviceChangeCallback fn) {
    if (!fn) return;
    std::lock_guard lock(g_cb_mu);
    g_device_cbs.push_back(fn);
}

}  // namespace dh2::d3d8
