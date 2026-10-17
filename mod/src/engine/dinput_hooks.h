#pragma once
// DirectInput 8: keyboard and mouse state only while this process's game window is in front.
// Installed from DllMain so it is in place before EngineDll7 calls DirectInput8Create.

namespace dh2::dinput {

void TryInstall();

}  // namespace dh2::dinput
