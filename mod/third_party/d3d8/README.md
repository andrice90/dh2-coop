# Direct3D 8 declarations

Original, short COM declarations for the overlay in `mod/src/ui`. They are not a copy of the Microsoft DirectX 8 SDK and not a copy of Wine or mingw-w64's `d3d8.h`.

## Why this is not the mingw-w64 file

The job asked for `mingw-w64-headers/include/{d3d8.h,d3d8types.h,d3d8caps.h}` and described them as public domain under the mingw-w64 disclaimer. Checked 2026-10-16:

- Tree: <https://github.com/mingw-w64/mingw-w64>
- Commit: `04f322c0ea8c91de333149aba384bf80370007a0` (2025-05-09, "headers: Update imported headers from Wine master")
- Raw `d3d8.h`: <https://github.com/mingw-w64/mingw-w64/blob/04f322c0ea8c91de333149aba384bf80370007a0/mingw-w64-headers/include/d3d8.h>
- That file's banner is Copyright (C) 2002 Jason Edmeades, GNU LGPL 2.1, include guard `__WINE_D3D8_H`. `d3d8types.h` and `d3d8caps.h` in the same commit have the same banner.
- mingw-w64 `DISCLAIMER` at that repo is the Zope Public License for the package. `DISCLAIMER.PD` is for files that say they were placed in the public domain. These three headers are not those files.

The job also says not to use Wine's headers (LGPL) or the Microsoft DirectX 8 SDK. Vendoring that commit would vendor Wine's header, so these files are a new declaration of the methods and numeric values the overlay calls.

Method order was checked against that header so the vtable matches `d3d8.dll`. `IDirect3DDevice8::Reset` is vtable slot 14 and `Present` is slot 15, which is what `mod/src/engine/d3d8_hooks.cpp` already hooks. Nothing here is linked against `d3d8.lib`. The device pointer comes from the game.

`d3d8.h` includes `<unknwn.h>` and defines `D3D_OK` as `S_OK`. The ASI is built with `WIN32_LEAN_AND_MEAN`, and that macro keeps `IUnknown` out of `windows.h`.
