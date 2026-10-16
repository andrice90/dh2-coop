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
Stats GetStats();

// Overlay hook point. Runs on the game thread after module OnFrame and before the real Present.
using PresentCallback = void (*)(void* device, const RECT* src, const RECT* dst, HWND window, const void* dirty);
void AddPresentCallback(PresentCallback fn);

// Reset stub for the overlay. phase 0 = before IDirect3DDevice8::Reset, phase 1 = after.
// present_params points at the 52-byte D3D8 present-parameters struct (no d3d8.h in the SDK).
using ResetCallback = void (*)(void* device, void* present_params, int phase);
void AddResetCallback(ResetCallback fn);

}  // namespace dh2::d3d8
