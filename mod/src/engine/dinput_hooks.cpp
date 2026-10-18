#include "engine/dinput_hooks.h"

#include <windows.h>

#include <atomic>
#include <cstring>
#include <mutex>
#include <string>

#include <spdlog/spdlog.h>

#include "core/exports.h"
#include "core/hook.h"
#include "core/log.h"
#include "core/safe_mem.h"
#include "engine/d3d8_hooks.h"

namespace dh2::dinput {
namespace {

// IDirectInput8A::CreateDevice is vtable slot 3.
// IDirectInputDevice8A: GetDeviceState 9, GetDeviceData 10, SetCooperativeLevel 13, Release 2.
constexpr int kVtCreateDevice = 3;
constexpr int kVtRelease = 2;
constexpr int kVtGetDeviceState = 9;
constexpr int kVtGetDeviceData = 10;
constexpr int kVtSetCooperativeLevel = 13;

constexpr DWORD kDisclExclusive = 0x1;
constexpr DWORD kDisclNonExclusive = 0x2;
constexpr DWORD kDisclForeground = 0x4;
constexpr DWORD kDisclBackground = 0x8;
constexpr DWORD kDisclNoWinKey = 0x10;

// GUID_SysKeyboard / GUID_SysMouse (DirectInput system devices).
constexpr uint8_t kSysKeyboard[16] = {0x61, 0x2B, 0x1D, 0x6F, 0xA0, 0xD5, 0xCF, 0x11,
                                      0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
constexpr uint8_t kSysMouse[16] = {0x60, 0x2B, 0x1D, 0x6F, 0xA0, 0xD5, 0xCF, 0x11,
                                   0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};

using CreateFn = HRESULT(__stdcall*)(HINSTANCE, DWORD, const void*, void**, void*);
using CreateDeviceFn = HRESULT(__stdcall*)(void*, const void*, void**, void*);
using GetStateFn = HRESULT(__stdcall*)(void*, DWORD, void*);
using GetDataFn = HRESULT(__stdcall*)(void*, DWORD, void*, DWORD*, DWORD);
using SetCoopFn = HRESULT(__stdcall*)(void*, HWND, DWORD);
using ReleaseFn = ULONG(__stdcall*)(void*);

enum Kind : int { kOther = 0, kKeyboard = 1, kMouse = 2 };

struct VtFns {
    void** vt = nullptr;
    GetStateFn state = nullptr;
    GetDataFn data = nullptr;
    SetCoopFn coop = nullptr;
    ReleaseFn release = nullptr;
};

constexpr int kMaxVt = 4;
constexpr int kMaxDev = 16;

safetyhook::InlineHook* g_create = nullptr;
safetyhook::InlineHook* g_create_device = nullptr;
std::atomic<bool> g_factory_hooked{false};
std::atomic<bool> g_only_focused{true};
std::atomic<bool> g_ui_capture{false};  // SetUiCapture: the lobby overlay is open
std::atomic<bool> g_install_logged{false};
std::atomic<int> g_vt_n{0};
VtFns g_vt[kMaxVt]{};
std::atomic<void*> g_devs[kMaxDev]{};
std::atomic<int> g_dev_kind[kMaxDev]{};
std::atomic<HWND> g_coop_hwnd{nullptr};
std::atomic<bool> g_logged_state{false};
std::atomic<bool> g_logged_data{false};
std::atomic<uint8_t> g_coop_logged[kMaxDev]{};
std::atomic<int> g_focus{-1};
std::mutex g_reg_mu;

uintptr_t A(const void* p) { return reinterpret_cast<uintptr_t>(p); }

bool zero_bytes(void* p, DWORD n) {
    if (!p || n == 0 || n > 4096) return false;
    __try {
        std::memset(p, 0, n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int kind_of(const uint8_t guid[16]) {
    if (std::memcmp(guid, kSysKeyboard, 16) == 0) return kKeyboard;
    if (std::memcmp(guid, kSysMouse, 16) == 0) return kMouse;
    return kOther;
}

const char* kind_name(int kind) {
    if (kind == kKeyboard) return "keyboard";
    if (kind == kMouse) return "mouse";
    return "other";
}

std::string coop_text(DWORD flags) {
    std::string s;
    auto add = [&](DWORD bit, const char* name) {
        if ((flags & bit) == 0) return;
        if (!s.empty()) s += '|';
        s += name;
    };
    add(kDisclExclusive, "EXCLUSIVE");
    add(kDisclNonExclusive, "NONEXCLUSIVE");
    add(kDisclForeground, "FOREGROUND");
    add(kDisclBackground, "BACKGROUND");
    add(kDisclNoWinKey, "NOWINKEY");
    if (s.empty()) s = "0";
    return s;
}

bool is_foreground(HWND hwnd) {
    if (!hwnd) return false;
    const HWND fg = GetForegroundWindow();
    if (!fg) return false;
    if (fg == hwnd) return true;
    const HWND root = GetAncestor(hwnd, GA_ROOT);
    return root != nullptr && fg == root;
}

HWND game_window() {
    if (HWND w = d3d8::DeviceWindow()) return w;
    return g_coop_hwnd.load();
}

void note_focus(bool focused, HWND hwnd) {
    const int next = focused ? 1 : 0;
    int prev = g_focus.load();
    if (prev == next) return;
    if (!g_focus.compare_exchange_strong(prev, next)) return;
    spdlog::info("dinput: window {} input (hwnd={:#x})", focused ? "gained" : "lost", A(hwnd));
}

// True when this call should look like every key and button is up.
bool block_input() {
    if (g_ui_capture.load()) return true;
    if (!g_only_focused.load()) return false;
    const HWND hwnd = game_window();
    if (!hwnd) return false;
    const bool focused = is_foreground(hwnd);
    note_focus(focused, hwnd);
    return !focused;
}

int find_dev(void* dev) {
    for (int i = 0; i < kMaxDev; ++i) {
        if (g_devs[i].load(std::memory_order_acquire) == dev) return i;
    }
    return -1;
}

bool watched(void* dev) { return find_dev(dev) >= 0; }

const VtFns* find_vt(void** vt) {
    const int n = g_vt_n.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i) {
        if (g_vt[i].vt == vt) return &g_vt[i];
    }
    return nullptr;
}

void log_api_once(std::atomic<bool>& flag, const char* api, DWORD cb, const void* caller) {
    bool expected = false;
    if (!flag.compare_exchange_strong(expected, true)) return;
    spdlog::info("dinput: {} cb={} caller={}", api, cb, core::describe_address(caller));
}

HRESULT __stdcall hk_GetDeviceState(void* self, DWORD cb, void* data) {
    void** vt = nullptr;
    if (!core::safe_read(self, vt)) return static_cast<HRESULT>(0x80070057);
    const VtFns* fns = find_vt(vt);
    if (!fns || !fns->state) return static_cast<HRESULT>(0x80004001);
    if (watched(self)) {
        log_api_once(g_logged_state, "GetDeviceState", cb, _ReturnAddress());
        if (block_input()) {
            zero_bytes(data, cb);
            return S_OK;
        }
    }
    return fns->state(self, cb, data);
}

HRESULT __stdcall hk_GetDeviceData(void* self, DWORD cb, void* items, DWORD* count, DWORD flags) {
    void** vt = nullptr;
    if (!core::safe_read(self, vt)) return static_cast<HRESULT>(0x80070057);
    const VtFns* fns = find_vt(vt);
    if (!fns || !fns->data) return static_cast<HRESULT>(0x80004001);
    if (watched(self)) {
        log_api_once(g_logged_data, "GetDeviceData", cb, _ReturnAddress());
        if (block_input()) {
            // Drain so keys that arrived while this window was behind do not replay on focus.
            if (cb >= 16 && cb <= 32) {
                uint8_t sink[32 * 32];
                DWORD n = 32;
                fns->data(self, cb, sink, &n, 0);
            }
            if (count) *count = 0;
            return S_OK;
        }
    }
    return fns->data(self, cb, items, count, flags);
}

HRESULT __stdcall hk_SetCooperativeLevel(void* self, HWND hwnd, DWORD flags) {
    void** vt = nullptr;
    if (!core::safe_read(self, vt)) return static_cast<HRESULT>(0x80070057);
    const VtFns* fns = find_vt(vt);
    if (!fns || !fns->coop) return static_cast<HRESULT>(0x80004001);
    if (hwnd) g_coop_hwnd.store(hwnd);
    const int slot = find_dev(self);
    if (slot >= 0) {
        uint8_t expected = 0;
        if (g_coop_logged[slot].compare_exchange_strong(expected, 1)) {
            spdlog::info("dinput: SetCooperativeLevel {} hwnd={:#x} flags={:#x} ({})",
                         kind_name(g_dev_kind[slot].load()), A(hwnd), flags, coop_text(flags));
        }
    }
    return fns->coop(self, hwnd, flags);
}

ULONG __stdcall hk_Release(void* self) {
    void** vt = nullptr;
    if (!core::safe_read(self, vt)) return 0;
    const VtFns* fns = find_vt(vt);
    if (!fns || !fns->release) return 0;
    const ULONG left = fns->release(self);
    if (left == 0) {
        const int slot = find_dev(self);
        if (slot >= 0) g_devs[slot].store(nullptr);
    }
    return left;
}

bool patch_slot(void** vt, int slot, void* fn) {
    DWORD old = 0;
    if (!VirtualProtect(&vt[slot], sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) return false;
    vt[slot] = fn;
    DWORD ignored = 0;
    VirtualProtect(&vt[slot], sizeof(void*), old, &ignored);
    return true;
}

void remember_device(void* dev, int kind) {
    const int existing = find_dev(dev);
    if (existing >= 0) {
        g_dev_kind[existing].store(kind);
        return;
    }
    for (int i = 0; i < kMaxDev; ++i) {
        void* empty = nullptr;
        if (g_devs[i].compare_exchange_strong(empty, dev)) {
            g_dev_kind[i].store(kind);
            return;
        }
    }
    spdlog::warn("dinput: device table full, {} {:#x} not filtered", kind_name(kind), A(dev));
}

void hook_device_vt(void* dev, int kind) {
    std::lock_guard lock(g_reg_mu);
    void** vt = nullptr;
    if (!core::safe_read(dev, vt) || !vt) return;
    if (find_vt(vt)) {
        remember_device(dev, kind);
        return;
    }
    int idx = g_vt_n.load();
    if (idx >= kMaxVt) {
        spdlog::warn("dinput: vtable table full");
        return;
    }
    VtFns fns{};
    fns.vt = vt;
    if (!core::safe_read(vt + kVtGetDeviceState, fns.state) || !core::safe_read(vt + kVtGetDeviceData, fns.data) ||
        !core::safe_read(vt + kVtSetCooperativeLevel, fns.coop) || !core::safe_read(vt + kVtRelease, fns.release)) {
        spdlog::warn("dinput: device vtable {:#x} unreadable", A(vt));
        return;
    }
    // Publish originals before the slots point at us.
    g_vt[idx] = fns;
    g_vt_n.store(idx + 1, std::memory_order_release);
    const bool ok = patch_slot(vt, kVtGetDeviceState, reinterpret_cast<void*>(&hk_GetDeviceState)) &&
                    patch_slot(vt, kVtGetDeviceData, reinterpret_cast<void*>(&hk_GetDeviceData)) &&
                    patch_slot(vt, kVtSetCooperativeLevel, reinterpret_cast<void*>(&hk_SetCooperativeLevel)) &&
                    patch_slot(vt, kVtRelease, reinterpret_cast<void*>(&hk_Release));
    if (!ok) {
        spdlog::error("dinput: VirtualProtect failed on device vtable {:#x}", A(vt));
        return;
    }
    remember_device(dev, kind);
    spdlog::info("dinput: hooked {} vtable {:#x} GetDeviceState={} GetDeviceData={}", kind_name(kind), A(vt),
                 core::describe_address(fns.state), core::describe_address(fns.data));
}

HRESULT __stdcall hk_CreateDevice(void* self, const void* guid, void** out, void* outer) {
    HRESULT hr = g_create_device->unsafe_stdcall<HRESULT>(self, guid, out, outer);
    uint8_t bytes[16]{};
    const bool have_guid = guid && core::safe_read(guid, bytes);
    const int kind = have_guid ? kind_of(bytes) : kOther;
    void* dev = (SUCCEEDED(hr) && out) ? *out : nullptr;
    spdlog::info("dinput: CreateDevice {} -> hr={:#010x} dev={:#x}", kind_name(kind), static_cast<unsigned>(hr),
                 A(dev));
    if (dev && kind != kOther) hook_device_vt(dev, kind);
    return hr;
}

void hook_factory(void* obj) {
    bool expected = false;
    if (!obj || !g_factory_hooked.compare_exchange_strong(expected, true)) return;
    void** vt = nullptr;
    void* create_device = nullptr;
    if (!core::safe_read(obj, vt) || !vt || !core::safe_read(vt + kVtCreateDevice, create_device)) {
        g_factory_hooked.store(false);
        return;
    }
    g_create_device = core::install_hook("IDirectInput8::CreateDevice", create_device,
                                         reinterpret_cast<void*>(&hk_CreateDevice));
    if (!g_create_device) g_factory_hooked.store(false);
}

HRESULT __stdcall hk_DirectInput8Create(HINSTANCE inst, DWORD ver, const void* iid, void** out, void* outer) {
    HRESULT hr = g_create->unsafe_stdcall<HRESULT>(inst, ver, iid, out, outer);
    void* obj = (SUCCEEDED(hr) && out) ? *out : nullptr;
    spdlog::info("dinput: DirectInput8Create ver={:#x} -> hr={:#010x} obj={:#x} caller={}", ver,
                 static_cast<unsigned>(hr), A(obj), core::describe_address(_ReturnAddress()));
    if (obj) hook_factory(obj);
    return hr;
}

void log_installed_once() {
    if (!g_create || g_create == reinterpret_cast<safetyhook::InlineHook*>(1)) return;
    auto logger = spdlog::default_logger();
    // DllMain runs before log_init replaces the default logger. Wait for the file logger.
    if (!logger || logger->name() != "dh2coop") return;
    bool expected = false;
    if (!g_install_logged.compare_exchange_strong(expected, true)) return;
    spdlog::info("dinput: hook installed InputOnlyWhenFocused={}", g_only_focused.load() ? 1 : 0);
}

// The game unacquires DirectInput on WM_KILLFOCUS, so device reads can stop while the
// window is behind. Sample focus from the ASI lifecycle thread as well as from the reads.
void PollFocus() {
    if (!g_only_focused.load()) return;
    const HWND hwnd = game_window();
    if (!hwnd) return;
    note_focus(is_foreground(hwnd), hwnd);
}

}  // namespace

void SetUiCapture(bool captured) {
    if (g_ui_capture.exchange(captured) != captured) spdlog::info("dinput: lobby {} game input", captured ? "holds" : "released");
}

void TryInstall() {
    log_installed_once();
    if (g_create) {
        PollFocus();
        return;
    }
    const std::string ini = (core::self_dir() / "dh2coop.ini").string();
    g_only_focused.store(GetPrivateProfileIntA("Coop", "InputOnlyWhenFocused", 1, ini.c_str()) != 0);
    HMODULE di = GetModuleHandleA("dinput8.dll");
    void* target = di ? reinterpret_cast<void*>(GetProcAddress(di, "DirectInput8Create")) : nullptr;
    g_create = core::install_hook("DirectInput8Create", target, reinterpret_cast<void*>(&hk_DirectInput8Create));
    if (!g_create) g_create = reinterpret_cast<safetyhook::InlineHook*>(1);
    log_installed_once();
}

}  // namespace dh2::dinput
