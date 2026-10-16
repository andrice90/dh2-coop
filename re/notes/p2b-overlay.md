# P2b — in-game overlay (Dear ImGui on Direct3D 8)

## Progress

Done. `dh2coop.asi` built clean (1352 KB, no warnings from `mod/src/ui`). Single-instance 70s, two-instance 90s, and an F8 hide/show check all ran. Screenshots and log excerpts are under Runtime proof.

## Design

Read-only status window drawn in the game, toggled with F8. No mouse capture and no WndProc subclass. The module `overlay` (`mod/src/ui/overlay_module.cpp`) registers `dh2::d3d8::AddPresentCallback` and `AddResetCallback` from `OnInit`. The present callback runs only on the game's main thread, after `OnFrame` and before the real `Present`.

Each visible frame:

1. Edge-detect F8 with `GetAsyncKeyState` only while the game window (or its root) is foreground.
2. `dh2::coop::Snapshot()` for the text. The call returns a copy.
3. Feed ImGui `DisplaySize` (backbuffer, else client rect) and `DeltaTime` from `QueryPerformanceCounter`.
4. `ImGui::Render()`, then upload geometry and capture device state.
5. `BeginScene` / draw / `EndScene`. The game has already ended its scene.
6. `ApplyStateBlock` + `DeleteStateBlock`, then write back the viewport, transforms, vertex and pixel shader handles, stream 0, indices, and texture stage 0.

Default visible is on. `[Overlay] Enabled` is read with `GetPrivateProfileIntA` from `dh2coop.ini` next to the ASI (`core::self_dir()`). Missing key means 1. The ini file is not modified by this job. ImGui does not write `imgui.ini` (`io.IniFilename = nullptr`).

The window is top-left, about 300 px wide, background alpha 0.62. Title text is `co-op    F8 hide`. Body: role, local id, port, hunter position (1 decimal) and yaw or "not found", a remote table (id, name, position, yaw, packets, RTT ms), animals, FPS. Default font only. `ImGuiConfigFlags_NoMouse` and `NoMouseCursorChange` are set. The window uses `NoInputs`.

Reset phase 0 releases the vertex buffer, index buffer, and font texture. Phase 1 lets the next frame recreate them (`ImTextureData::SetStatus(Destroyed)` flips back to `WantCreate` while pixels are still alive).

## D3D8 headers (license)

`mingw-w64` commit `04f322c0ea8c91de333149aba384bf80370007a0` (2025-05-09, "headers: Update imported headers from Wine master") has `mingw-w64-headers/include/d3d8.h`. The file itself starts with `Copyright (C) 2002 Jason Edmeades` and the GNU LGPL 2.1 text, and the include guard is `__WINE_D3D8_H`. The repo `DISCLAIMER` is the Zope Public License; `DISCLAIMER.PD` covers other files that say they were placed in the public domain. These three headers are not those files.

The job says to vendor that path and also says not to use Wine's headers (LGPL) or the Microsoft DirectX 8 SDK. Copying the current mingw file would be copying Wine's header. `mod/third_party/d3d8/` is therefore a short original declaration of the COM methods and enum values this overlay calls. Method order was checked against that mingw/Wine header so the vtable slots match `d3d8.dll` (device `Reset` is slot 14, `Present` is slot 15). Nothing links `d3d8.lib`. See `mod/third_party/d3d8/README.md`.

The ASI compile defines `WIN32_LEAN_AND_MEAN`, so `windows.h` does not declare `IUnknown`. `d3d8.h` includes `<unknwn.h>` and defines `D3D_OK` as `S_OK`. The first build failed with `C2504: 'IUnknown': base class undefined` until that include was added.

## D3D8 port notes (vs imgui v1.93.0 `imgui_impl_dx9.cpp`)

Ported from `mod/build/_deps/imgui-src/backends/imgui_impl_dx9.cpp` at the fetched tag `v1.93.0`.

| DX9 | DX8 in this backend |
|---|---|
| `SetFVF(fvf)` | `SetVertexShader(fvf)` (`D3DFVF_XYZ\|D3DFVF_DIFFUSE\|D3DFVF_TEX1`) |
| `SetVertexShader(nullptr)` / `SetPixelShader(nullptr)` | `SetPixelShader(0)` then `SetVertexShader(fvf)`. Handles are `DWORD`, not COM pointers |
| `CreateVertexBuffer(..., &vb, nullptr)` shared handle | `CreateVertexBuffer(length, usage, fvf, pool, &vb)` |
| `CreateIndexBuffer(..., &ib, nullptr)` | `CreateIndexBuffer(length, usage, format, pool, &ib)` |
| `Lock(..., void** data, flags)` | `Lock(..., BYTE** data, flags)` |
| `SetStreamSource(0, vb, 0, stride)` offset parameter | `SetStreamSource(0, vb, stride)` |
| `SetIndices(ib)` | `SetIndices(ib, baseVertexIndex)` per draw |
| `DrawIndexedPrimitive(type, baseVertex, minIndex, numVertices, startIndex, primCount)` | `DrawIndexedPrimitive(type, minIndex, numVertices, startIndex, primCount)`. Base vertex goes through `SetIndices` |
| `CreateTexture(..., &tex, nullptr)` | `CreateTexture(w, h, levels, usage, format, pool, &tex)`. Font texture uses `D3DPOOL_MANAGED`, usage 0, `D3DFMT_A8R8G8B8`. Fallback: `D3DUSAGE_DYNAMIC` + `D3DPOOL_DEFAULT` |
| `D3DFMT_A8B8G8R8` when the device supports it | Not used. CPU swizzle RGBA to BGRA (`IMGUI_COL_TO_DX_ARGB`) unless `IMGUI_USE_BGRA_PACKED_COLOR` |
| `SetSamplerState` min/mag/address | `SetTextureStageState` `D3DTSS_MINFILTER` (17), `MAGFILTER` (16), `MIPFILTER` (18), `ADDRESSU` (13), `ADDRESSV` (14). Also `TEXCOORDINDEX` 0 and texture-transform flags 0. There are no sampler objects |
| `D3DRS_SCISSORTESTENABLE` + `SetScissorRect` | No scissor. Each draw sets the viewport to the clipped pixel rect and an ortho projection of that rect, with `D3DRS_CLIPPING` on, so triangles outside the rect clip |
| `D3DRS_SEPARATEALPHABLENDENABLE`, `SRCBLENDALPHA`, `DESTBLENDALPHA` | Not in D3D8. Blend is `SRCBLEND = SRCALPHA`, `DESTBLEND = INVSRCALPHA`, `BLENDOP = ADD` |
| `IDirect3DStateBlock9` + `Capture()` + `Apply()` + `Release()` | `CreateStateBlock(D3DSBT_ALL, &token)` captures immediately (no separate Capture), `ApplyStateBlock(token)`, `DeleteStateBlock(token)`. Token is a `DWORD` |
| State block only, plus manual transform restore | Same, and after `ApplyStateBlock` we also restore viewport, world/view/projection, vertex shader, pixel shader, stream 0, indices, and texture 0. Those are the states this pass changes. Render states and texture-stage states are left to the block |
| Draw inside the app's scene | `BeginScene` / `EndScene` in the present callback. Texture `LockRect` and `CreateStateBlock` run before `BeginScene` |

`ImGuiBackendFlags_RendererHasVtxOffset` stays on: 16-bit indices plus `SetIndices` base vertex. `ImGuiBackendFlags_RendererHasTextures` stays on (dynamic font atlas, same `ImTextureStatus` path as DX9).

## Ini

```
[Overlay]
Enabled=1
```

`Enabled=0` starts hidden. F8 still toggles while the game window is in the foreground. Documented here only; `mod/dh2coop.ini` was not edited.

## Runtime proof

Build: `mod\build.ps1` (Release, Win32). Output `mod\build\dh2coop.asi` (1352 KB), installed to `game\Bin`. The compile log for `mod/src/ui/imgui_impl_dx8.cpp` and `overlay_module.cpp` has no `warning C` lines. `imgui_impl_dx9.cpp` is not in `imgui_lib`. `imgui_impl_win32.cpp` is still compiled there and is not called.

Windowed runs used a loose `game\Data\Cfg\console.cfg` copied from `tools-src\console.windowed.cfg`. That path already existed and was already `r_windowed = 1`. `game2\Data\Cfg\console.cfg` also already existed. Both were backed up and restored after the runs. Device log from the single-instance run: `D3D8 params in : 640x480 fmt=21 windowed=1`, `behavior=0x84`.

### Single instance, 70s

`.\run-game.ps1 -Seconds 70 -ExtraArgs '-nointro -developper -cheats -spmap HUNTMAP4'`. Exit 0. `Main7.exe` pid 10836. Log was `game\Bin\dh2coop.log` (truncated on the later runs; lines below were read from that file before the coop launch):

```
overlay: ini Enabled=1
overlay: imgui dx8 initialized
overlay: display 640x480
overlay: font texture created 512x128 fmt=A8R8G8B8 pool=managed
overlay: CreateStateBlock(D3DSBT_ALL) hr=0x00000000
heartbeat frames=969 window_frames=969 avg_dt=0.01032s
heartbeat frames=1970 window_frames=1001 avg_dt=0.01000s
heartbeat frames=5968 window_frames=1000 avg_dt=0.01001s
```

No `[error]` lines and no exception text. No `overlay: reset phase` line, so Reset did not run. Screenshot `re/notes/p2b-overlay.png` (client 640x480, window title `DNGH2`, foreground). Top-left semi-transparent panel reads: title `co-op    F8 hide`, `role offline`, `local id -`, `port 27420`, `hunter 0.0 -1.7 0.0  yaw 120.0`, remote header with `no remotes`, `animals 5`, `fps 100`. The weapon-select screen (CHOOSE ITEM, three guns) is visible around the panel.

### Two instances, 90s

`.\run-coop-local.ps1 -Seconds 90`. Exit 0. Host pid 14280 (`game\Bin`, `-coophost`), client pid 10780 (`game2\Bin`, `-coopjoin 127.0.0.1`). Closed after 90s. Host log lines, read before the later F8 run truncated `game\Bin\dh2coop.log`:

```
coop: hosting on UDP 27420 -> true
coop: player 1 'Hunter' joined
overlay: font texture created 512x128 fmt=A8R8G8B8 pool=managed
overlay: CreateStateBlock(D3DSBT_ALL) hr=0x00000000
heartbeat ... avg_dt=0.01032s then 0.01000s through 0.01001s
```

One host window was `avg_dt=0.01187s` (846 frames) at 11:02:00, then the next window was `0.01001s`. Client log `game2\Bin\dh2coop.log` still has:

```
coop: joining 127.0.0.1:27420 -> true
coop: welcomed as player 1 (max 4)
coop: player 0 'Hunter' joined
overlay: font texture created 512x128 fmt=A8R8G8B8 pool=managed
heartbeat frames=970 window_frames=970 avg_dt=0.01031s
heartbeat frames=7963 window_frames=998 avg_dt=0.01002s
```

No `[error]` lines in that client log. Neither log has `overlay: reset phase`.

`re/notes/p2b-overlay-host.png`: `role host`, `local id 0`, `port 27420`, hunter `0.0 -1.7 0.0 yaw 120.0`, one remote row `1 | Hunter | 0.0 -1.7 0.0 | 120.0 | 677 | 15`, `animals 5`, `fps 100`.

`re/notes/p2b-overlay-client.png`: `role client`, `local id 1`, `port 27420`, one remote row `0 | Hunter | 0.0 -1.7 0.0 | 120.0 | 685 | 14`, `animals 5`, `fps 100`. The host window overlaps the left edge of this capture. The client panel itself is fully on screen.

### F8

A later 25s `run-game.ps1` with the same map args. After the font texture existed and the window was foreground, F8 was held for about 180 ms. `game\Bin\dh2coop.log`:

```
overlay: visible=0
overlay: visible=1
```

(about 0.7 s apart, thread t2748). `re/notes/p2b-overlay-hidden.png` is the hidden frame: no panel, and the CHOOSE ITEM header plus the three weapon rows are complete where the panel had been. Showing it again logged `visible=1`.

## Known limits

- No mouse, no keyboard to the widgets, no `imgui_impl_win32`. F8 is the only input, and only as a toggle. `GetAsyncKeyState` does not consume the key, so the game still sees F8. UNVERIFIED whether the retail game binds F8.
- Clip is by viewport, not a scissor rect. A clip rectangle that is not the full window changes the ortho matrix for that draw. Sub-pixel clip edges are truncated to integers.
- No separate alpha blend. The overlay does not try to preserve the backbuffer alpha.
- State restore writes back viewport, transforms, shader handles, stream 0, indices, and texture 0 after `ApplyStateBlock(D3DSBT_ALL)`. On the weapon-select screen, hiding the panel left the menu and the backdrop intact (see the hidden screenshot). Other scenes (lights, clip planes, render targets, a full 3D view with no menu) were not compared. Reset invalidate/recreate was not hit in these runs.
- The loading-screen thread's `Present` does not run this callback (existing hook). On these runs the main thread presented within about a second, so the overlay was up before the 70s mark.
- `OnShutdown` does not touch the device. The backend `AddRef`s the device; a normal process exit drops it. The test launcher uses `TerminateProcess` after `CloseMainWindow`, which skips `OnShutdown`.
- One ImGui context, default font, no ini, no multi-viewport. `CreateStateBlock(D3DSBT_ALL)` runs every visible frame. It did not move the steady heartbeat off ~0.010 s.
- Headers are a subset. They are not a drop-in replacement for a full D3D8 SDK.

## Proposed MEMORY facts

- Overlay module name is `overlay`, priority 200. It registers `AddPresentCallback` and `AddResetCallback` from `OnInit` and draws only on the game's main thread. Evidence: `modules: 6 registered [probe(10), coop(40), player_tracker(50), slowmo(60), overlay(200), heartbeat(1000)]` and `overlay: ini Enabled=1` in `game\Bin\dh2coop.log` (single run 2026-10-16 10:58:18.076, and again on the host and client logs).
- First main-thread Present creates the DX8 ImGui backend and one managed font texture. Evidence: `overlay: imgui dx8 initialized`, `overlay: display 640x480`, `overlay: font texture created 512x128 fmt=A8R8G8B8 pool=managed`. Same run's device line: `D3D8 params in : 640x480 fmt=21 windowed=1`, `behavior=0x84`.
- `CreateStateBlock(D3DSBT_ALL)` returns `S_OK` on that device. Evidence: `overlay: CreateStateBlock(D3DSBT_ALL) hr=0x00000000` (single, host, and client logs). No `overlay:` error line followed in the 70s or 90s runs.
- A visible overlay keeps the game near the 100 fps cap. Evidence: single-instance heartbeats `avg_dt=0.01032s` then `0.01000s`–`0.01002s` across six windows; client coop heartbeats `0.01031s` down to `0.01002s` at `frames=7963` in `game2\Bin\dh2coop.log`. One host window was `avg_dt=0.01187s`, then `0.01001s`.
- F8 toggles visibility only while the game window is foreground. Evidence: `overlay: visible=0` then `overlay: visible=1` in `game\Bin\dh2coop.log` at 11:03:28.587 and 11:03:29.247. Hidden frame: `re/notes/p2b-overlay-hidden.png`.
- `dh2::coop::Snapshot()` feeds the panel. Offline: `re/notes/p2b-overlay.png` shows `role offline`, `local id -`, `port 27420`. Host: `re/notes/p2b-overlay-host.png` shows `role host`, local id 0, remote id 1 `Hunter`, rtt 15. Client: `re/notes/p2b-overlay-client.png` shows `role client`, local id 1, remote id 0 `Hunter`, rtt 14. Matching log lines: `coop: hosting on UDP 27420 -> true`, `coop: player 1 'Hunter' joined`, and in `game2\Bin\dh2coop.log` `coop: joining 127.0.0.1:27420 -> true`, `coop: welcomed as player 1 (max 4)`.
- `[Overlay] Enabled` defaults to 1 via `GetPrivateProfileIntA` on `dh2coop.ini` next to the ASI. Evidence: `overlay: ini Enabled=1` with `mod/dh2coop.ini` not edited (no Overlay section there). Missing key uses the default 1.
- `mod/third_party/d3d8` is an original subset, not mingw-w64. mingw-w64 commit `04f322c0ea8c91de333149aba384bf80370007a0` `mingw-w64-headers/include/d3d8.h` is Wine LGPL (`__WINE_D3D8_H`, Jason Edmeades). The ASI does not link `d3d8.lib`. `d3d8.h` includes `<unknwn.h>` because `WIN32_LEAN_AND_MEAN` hides `IUnknown`.
