# Job: P2b — in-game overlay (Dear ImGui on Direct3D 8). YOU OWN GAME LAUNCHES. No Ghidra.

Goal: a read-only debug and status overlay drawn in the game, toggled with F8. It must not interfere with the game's input or rendering state.

Read first:
- `docs/mod-architecture.md`, `docs/mod-learnings.md` (Testing loop, Hooking, game-thread rule)
- `mod/src/engine/d3d8_hooks.h/.cpp`
- `mod/src/coop/coop.h` (the `Snapshot()` API)
- `mod/CMakeLists.txt` (imgui is already fetched as `imgui_lib`)

## File ownership
- **You own:** `mod/src/ui/**` (new), `mod/third_party/d3d8/**` (new, headers only), and `re/notes/p2b-overlay.md` (new).
- **You may edit `mod/CMakeLists.txt` only to:** remove `imgui_impl_dx9.cpp` from `imgui_lib` (there is no D3D9 device), and add the `mod/third_party/d3d8` include dir.
- **Do not edit anything else.** That includes `mod/src/coop/**`, `mod/src/engine/**` and `mod/src/core/**`. If you need a framework change, describe it in your notes instead.

## Constraints (important)
- **The renderer is Direct3D 8.** One `IDirect3DDevice8`, fullscreen or windowed 640x480. Dear ImGui has **no** official DX8 backend.
  - Write `mod/src/ui/imgui_impl_dx8.cpp/.h`, ported from upstream `imgui_impl_dx9.cpp` at the fetched tag. Use D3D8 equivalents:
    - `SetVertexShader(FVF)` instead of `SetFVF`
    - `CreateVertexBuffer` / `CreateIndexBuffer` with the D3D8 signatures, and the `Lock` signature with `BYTE**`
    - `SetIndices(ib, baseVertexIndex)`
    - `DrawIndexedPrimitive(type, minIndex, numVertices, startIndex, primCount)` with no BaseVertexIndex parameter; pass the base vertex through `SetIndices`
    - `CreateTexture` with no shared-handle parameter
    - render states set via `SetRenderState` and `SetTextureStageState`; D3D8 has no sampler states, so filters are texture stage states
    - for scissor, D3D8 has no scissor rect: clip by viewport, or skip clipping (acceptable for a debug overlay)
  - Save and restore all device state you touch with a D3D8 state block (`CreateStateBlock(D3DSBT_ALL, &token)` / `ApplyStateBlock` / `DeleteStateBlock`), so the game's rendering is unaffected.
- **D3D8 headers:** the Windows SDK has no `d3d8.h`. Vendor `d3d8.h`, `d3d8types.h` and `d3d8caps.h` from **mingw-w64** (`mingw-w64-headers/include`, which are public domain per the mingw-w64 DISCLAIMER) into `mod/third_party/d3d8/`, with a README noting the source URL, commit and license. **Do not** use Wine's headers (LGPL) or the Microsoft DirectX 8 SDK. Do not link `d3d8.lib`: the device pointer comes from our hook and we only call its methods.
- **Hook point:** call `dh2::d3d8::AddPresentCallback` once from your module's `OnInit`. The callback already runs only on the game's main thread, right before the real `Present`. Begin and end your own scene inside the callback (`BeginScene`/`EndScene`), because the game has already ended its scene. Handle device `Reset` via `AddResetCallback`: invalidate your buffers and font texture before the reset, and recreate them after.
- **Input:** read-only overlay, no mouse capture and no WndProc subclassing in this job. Toggle with **F8**, detected with `GetAsyncKeyState` edge detection inside the present callback, and only when the game window is the foreground window. Feed ImGui the display size and the delta time from QueryPerformanceCounter. `imgui_impl_win32` may stay out entirely.
- **Content (one small window, top-left, semi-transparent, about 300 px wide at 640x480):**
  - title "co-op", plus "F8 hide"
  - session: role, local id, port
  - local hunter: position (1 decimal) and yaw, or "not found"
  - remote players table: id, name, position, yaw, packets, RTT ms
  - animals count, FPS
  - Data comes only from `dh2::coop::Snapshot()`, which returns a copy and is safe to call in the callback.
- Default visible = on. Add an ini key `[Overlay] Enabled=1` (read with `GetPrivateProfileIntA` from `dh2coop.ini` next to the ASI, via `core::self_dir()`). Document the key in your notes; do not edit `mod/dh2coop.ini`.
- **Fonts:** ImGui's default font only. Keep memory small.

## Verify (runtime)
1. Build with `mod\build.ps1` (cleanly, with no new warnings from our sources).
2. Single instance, using a 70-second run: `.\run-game.ps1 -Seconds 70 -ExtraArgs '-nointro -developper -cheats -spmap HUNTMAP4'`. Check that:
   - `dh2coop.log` shows the overlay initialized (font texture created) and no D3D errors
   - the game keeps running at about 100 fps (heartbeat avg_dt about 0.010)
   - there are no exceptions
3. Take a screenshot to prove it draws. Capture the game window to `re/notes/p2b-overlay.png`, for example with a PowerShell `System.Drawing` `CopyFromScreen` of the game window rect, taken while the game is foreground in windowed mode. Windowed mode needs a loose `game\Data\Cfg\console.cfg`; copy `tools-src\console.windowed.cfg` there for this run, then delete it afterwards. Fullscreen capture may come out black, so use windowed. Look at the image yourself and describe what you see in the notes.
4. Two instances: `.\run-coop-local.ps1 -Seconds 90`. Both overlays should show role host/client and one remote row. Screenshot both windows if feasible.

## Deliverables
- `mod/src/ui/**`, `mod/third_party/d3d8/**` and the CMake edits above.
- `re/notes/p2b-overlay.md`: design, D3D8 port notes (every API difference you handled), runtime proof (log excerpts, screenshot paths and what they show), known limits, and `## Proposed MEMORY facts`.
