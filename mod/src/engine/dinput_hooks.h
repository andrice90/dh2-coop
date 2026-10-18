#pragma once
// DirectInput 8: keyboard and mouse state only while this process's game window is in front.
// Installed from DllMain so it is in place before EngineDll7 calls DirectInput8Create.

namespace dh2::dinput {

void TryInstall();
// The co-op lobby window has the keyboard and mouse: the game sees every key and button up meanwhile.
void SetUiCapture(bool captured);

}  // namespace dh2::dinput
