# Player markers

**Progress:** done. Camera accessor, overlay markers, ini keys, solo HUNTMAP4 play test (pid 14376, closed after the shots).

## Design

- Remote marker point = feet (`SessionView::remotes[].pos`) + 2.0 m on world Y. Test animals use entity `+0xE8` the same way. Distance is from the local hunter feet, whole metres.
- `LastViewCamera()` is the first `cRender::SetCamera` of the last completed frame. Up is `y = z × x` (x right, y up, z forward).
- On screen when `cz > 0.1` and the pixel is inside the back buffer. Otherwise a 14 px arrow on the rectangle inset 24 px from the edges. Behind the camera the 2D direction is `(-cx, -cy)` before the screen-Y flip, so a point straight behind and just above the eye sits on the middle of the bottom edge. A zero direction (exactly on the camera axis behind) also falls through to the bottom edge.
- F7 toggles markers, foreground-gated like F8. F8 toggles only the status panel. Both off skips ImGui for that frame.
- No remote marker when the view is invalid, `proj != 0`, or that remote has `packets == 0`. `MarkerTest=1` still draws animals with no remotes. Animals at the parked sentinel (x and z &lt; -5000, the inactive puma) are skipped.
- Colours by player id: 0 gold `(255,200,60)`, 1 sky `(90,170,255)`, 2 green `(110,220,110)`, 3 coral `(255,110,100)`. Animals are grey `(180,180,180)`. Alpha is `max(0.5, 1 - dist/160)`.

## FOV

`+0xD0` is a **horizontal field of view in degrees**.

Disassembly (`EngineDll7.dll`, image base `0x10000000`, read from the retail install):

- `cRenderCamera::GetFov` (`0xaef0`) is `fld dword [ecx+0xD0]`.
- `UpdatePerspectiveProjectionFrustum` (`0x2fdb0`) does `tan(fov * k)` with `k` at VA `0x10408338` = `0.008726646` = π/360, and stores that in `+0xDC` (`GetZMullX`). `+0xE0` (`GetZMullY`) = `(+0xD4 / +0xD8) * tan(half)`.
- `GetAspect` is `+0xD4`, `GetRatio` is `+0xD8`. `SetAspect` (`0x32280`) is called from `0x357108` with `(viewportBottom - viewportTop) / (viewportRight - viewportLeft)`, so aspect here is **height/width**. With ratio 1, `ZMullY = (h/w) * tan(half)`.
- `CalculatePerspectiveMatrix` (`0x30e50`) writes `_11 = 1/ZMullX = cot(fov/2)` and `_22 = 1/ZMullY`. That is a horizontal FOV: the raw angle divides the width, and the height/width aspect makes the Y scale match square pixels.
- `Transform3dTo2dCameraSpace` (`0x26ed0`) is `sx = width/2 + cx * width / (2 * ZMullX * cz)` and the same scale on Y. Constructor site `0x34004` pushes `0x42b40000` (90.0) into `SetPerspective`.

Runtime, same launch, `Data\Cfg\console.cfg` has `cam_fov = 90`, back buffer 640×480:

```
tracker: view y stored=(0.000,1.000,0.000) cross(z,x)=(0.000,1.000,0.000) fov=90.000 proj=0
overlay: display 640x480
overlay: ini Enabled=1 Markers=1 MarkerTest=1
```

Stored Y (`+0x94`) matched `z × x` on that frame. `proj` 0 is perspective.

Projection used by the overlay:

```
tanHalf = tan(fov * pi / 360)
scale   = (backBufferWidth / 2) / tanHalf
sx      = width/2  + (cx / cz) * scale
sy      = height/2 - (cy / cz) * scale
```

`cx = dot(d, x)` positive is camera-right, so screen X increases to the right.

## Test record

Solo `.\run-game.ps1 -ExtraArgs '-nointro -developper -cheats -spmap HUNTMAP4'`, pid 14376. CHOOSE ITEM closed with Enter, Left, Enter. No other Main7 was running. Process closed at the end. Shipped `mod/dh2coop.ini` keeps `MarkerTest=0` (the `game\Bin` copy was set to 1 only for this run).

| Shot | View | What it shows |
|---|---|---|
| `player-markers-1.png` | yaw 120.2, still at the spawn | Grey edge arrows. id 120 (63 m) and id 121 (56 m) are behind and to camera-left; their labels sat on the left edge. Not mirrored. |
| `player-markers-4.png` | hunter `(-17.0, 1.4, 56.6)`, yaw -144.1 | id 121 at 11 m is on screen: grey diamond just above the elk, label `animal 1  11m` under it. id 123 (~106 m) is behind to the right; its arrow is on the right edge pointing right. id 122 (91 m) is a second on-screen diamond further along the view. |

Left/right was checked against `yaw = atan2(forward.x, forward.z)` and `right = (cos yaw, 0, -sin yaw)`, which is the basis `y = z × x`. Arrows for animals on the left went left; the behind animal on the right went right.

## Screenshots

- `re/notes/player-markers-1.png` — off-screen arrows at the spawn.
- `re/notes/player-markers-4.png` — elk under the on-screen diamond, plus a right-edge arrow.

## Proposed MEMORY facts

- `cRenderCamera` FOV at `+0xD0` is degrees and is the **horizontal** field of view. Evidence: `GetFov` `fld [ecx+0xD0]` (EngineDll7 `0xaef0`); `UpdatePerspectiveProjectionFrustum` (`0x2fdb0`) computes `tan(fov * 0.008726646)` and `0.008726646` at VA `0x10408338` is π/360; `CalculatePerspectiveMatrix` (`0x30e50`) sets `_11 = cot(fov/2)`; `SetAspect` at `0x357108` stores height/width. Runtime log `fov=90.000 proj=0` with `console.cfg` `cam_fov = 90` and `overlay: display 640x480`.
- `GetYAxis` is `+0x94` (`0xaf90`). On the first world/menu camera of this run it equalled `z × x`: log `view y stored=(0.000,1.000,0.000) cross(z,x)=(0.000,1.000,0.000)`.
- `GetZMullX` is `+0xDC` = `tan(fov/2)` (`0xafb0`). `GetZMullY` is `+0xE0` (`0xafc0`). `Transform3dTo2dCameraSpace` (`0x26ed0`) places `sx = width/2 + x_cam * width / (2 * ZMullX * z_cam)`.
