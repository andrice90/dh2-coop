#pragma once
// DirectInput 8: keyboard and mouse state only while this process's game window is in front.
// Installed from DllMain so it is in place before EngineDll7 calls DirectInput8Create.
#include <cstdint>

namespace dh2::dinput {

void TryInstall();
// The co-op lobby window has the keyboard and mouse: the game sees every key and button up meanwhile.
void SetUiCapture(bool captured);
// Tests (console coop_key): press and release these DIK scan codes on this process's keyboard device, one after
// the other, as buffered events. They reach the game whether or not its window is in front.
void QueueKeys(const uint8_t* scans, int n, uint32_t holdMs = 120, uint32_t gapMs = 250);

}  // namespace dh2::dinput
