#pragma once
// D3D8 device chain: Direct3DCreate8 -> IDirect3D8::CreateDevice -> Present / Reset.
// OnFrame is dispatched from Present on the game thread, before the original call.
#include <windows.h>

#include <cstdint>

namespace dh2::d3d8 {

struct Stats {
    std::uint32_t create_calls = 0;
    std::uint32_t device_calls = 0;
    std::uint32_t reset_calls = 0;
    std::uint64_t present_calls = 0;
    bool create_hooked = false;
    bool device_hooked = false;
};

// Safe to call repeatedly from the ASI init thread.
void TryInstall();
// If cRenderDX::m_d3dd is already filled, hook that device. `engine` may be null.
void TryLateDevice(HMODULE engine);

void* Device();
// HWND passed to IDirect3D8::CreateDevice (hDeviceWindow, else the focus window). Null until then.
HWND DeviceWindow();
Stats GetStats();

// Overlay hook point. Runs on the game thread after module OnFrame and before the real Present.
using PresentCallback = void (*)(void* device, const RECT* src, const RECT* dst, HWND window, const void* dirty);
void AddPresentCallback(PresentCallback fn);

// Reset stub for the overlay. phase 0 = before IDirect3DDevice8::Reset, phase 1 = after.
// present_params points at the 52-byte D3D8 present-parameters struct (no d3d8.h in the SDK).
using ResetCallback = void (*)(void* device, void* present_params, int phase);
void AddResetCallback(ResetCallback fn);

// A second IDirect3D8::CreateDevice: the game builds a new device for a resolution / window-mode change instead of
// Reset (2026-10-18, CreateDevice #2 and #3 from the options menu). phase 0 runs before the new device is made:
// release everything tied to the old one (it was already released by the game; anything still holding it keeps
// it alive). phase 1 runs after, with the new device (null when creation failed). Not called for the first device.
using DeviceChangeCallback = void (*)(void* new_device, int phase);
void AddDeviceChangeCallback(DeviceChangeCallback fn);

}  // namespace dh2::d3d8
