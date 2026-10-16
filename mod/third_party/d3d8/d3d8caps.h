// Minimal Direct3D 8 caps stand-in for the co-op overlay.
// Not the Wine/mingw-w64 header and not the Microsoft SDK. See README.md.
// The overlay does not call GetDeviceCaps. D3DCAPS8 stays an incomplete type
// so a wrong struct size cannot be stack-allocated and passed to the device.
#pragma once

struct D3DCAPS8;
