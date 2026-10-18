# Display size and HUD shape

The world camera already takes its aspect from the viewport (`height / width`, `SetAspect` at EngineDll7 `0x357128`) and stores a **horizontal** FOV at `cRenderCamera+0xD0`. A wider back buffer is not stretched. Leaving the FOV at the 4:3 value crops the top and bottom. `cRenderCamera::UpdatePerspectiveProjectionFrustum` (`0x2fdb0`) is wrapped so the projection uses a wider horizontal angle that keeps the 4:3 vertical view. The stored FOV is restored before the call returns. Player markers read that same widened angle (`EffectiveHorizontalFov`).

The HUD is a virtual 640×480. `GuiSetDisplay2D` (CommonDll7 `0xe07b0`) sets `x scale = pixelWidth/640` (RVA `0xac7dc`) and `y scale = pixelHeight/480` (`0xac7e0`). Those two numbers match only at 4:3, so 3440×1440 draws the HUD about 1.8× too wide. After `GuiSetDisplay2D` both scales become `min(width/640, height/480)` and the D3D8 viewport (device slot 40) moves to the centered 4:3 rectangle. The viewport is put back on Present, before the next world pass. `cGuiEvent::GetMouseEvent` (`0xe6810`) maps cursor pixels across the full buffer (`pixel / width * 640`); the same inset is applied so menu clicks land on the centered controls.

Resolution is the `cRenderInterface` cvars `CreateRender` (`0xa3d40`) copies: width `+0x1fc`, height `+0x200`, windowed `+0x208`. `[Display]` in `dh2coop.ini` overwrites them. Width or Height 0 is the desktop work area, shrunk a little so the caption still fits. `run-coop-local.ps1` writes a tile instead when more than one window is started and Width is still 0.

The standalone FOV-fix ASI mid-hooks the same `fld` at EngineDll7 `0x2fe07` and would apply the widen a second time. It is renamed to `.asi.off` in this checkout's `game`, `game2`, `game3`, and `game4` bins. A copy remains under `tools\downloads\fovfix\`.

## Proposed MEMORY facts

- HUD virtual space is 640×480. `GuiSetDisplay2D` (CommonDll7 `0xe07b0`) stores pixel width at RVA `0x1ac7d4`, pixel height at `0x1ac7d8`, `width/640` at `0x1ac7dc`, `height/480` at `0x1ac7e0`. The constants `1/640` and `1/480` are at RVA `0x137f84` and `0x137f80`.
- `cGuiEvent::GetMouseEvent` (CommonDll7 `0xe6810`) writes virtual x,y as `pixel / viewportSize * {640,480}`. Raw pixels are the ints at RVA `0x1b35ac` and `0x1b35b0`.
