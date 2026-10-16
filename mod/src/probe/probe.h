#pragma once
// Runtime probe module. Installed via DH2_MODULE. Does nothing unless [Probe] Enabled=1.
// The D3D8 Present chain, cBspTree::Load event, and DLL notifications live outside this module
// and stay on even when the probe is off.

namespace dh2::probe {

// ASI init thread poll. No-op when the probe module is off. Hooks D3D9 if it appears.
void tick();

}  // namespace dh2::probe
