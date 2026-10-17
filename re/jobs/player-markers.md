# Job: player-markers — on-screen markers for co-op players (C++; you work in a git worktree; solo game launches only)

## Goal
The user wants to see where the other players are: a marker over each remote player's head when on screen, and an arrow at the screen edge pointing toward them when off screen or behind the camera. Name and distance next to it.

## Where you work
You run inside the git worktree `D:\scary hunt\dh2coop-markers` (branch `markers`). The lead edits `mod/src` in the main checkout at the same time, so:
- Build with `.\mod\build.ps1` **in this worktree** (its own `mod\build`). It installs into this worktree's `game\Bin`.
- First-time setup: `robocopy "D:\scary hunt\dh2coop\game" "D:\scary hunt\dh2coop-markers\game" /MIR /XF *.log dh2coop.cmd /R:1 /W:1 /NFL /NDL /NP` (about 600 MB). Exit codes below 8 are success.
- Never write to `D:\scary hunt\dh2coop\` (the main checkout). Never write to `D:\scary hunt\Activision Value\`.

## Deliverables (edit only these, in the worktree)
- `mod/src/coop/player_tracker.h`, `mod/src/coop/player_tracker.cpp`: add a read-only accessor for the world camera of the last frame. Do not change existing behaviour.
- `mod/src/ui/overlay_module.cpp`: the marker drawing.
- `mod/dh2coop.ini`: two new keys in `[Overlay]` (below).
- `re/notes/player-markers.md`: design, the projection maths, test record, screenshots list.

## Inputs you have
- `player_tracker.cpp` already captures the world camera each frame in `hk_SetCamera`: `cRenderCamera` X axis `+0x84`, Z axis (view forward) `+0xA4`, position `+0xB4`, projection type `+0xC4`, fov `+0xD0`. The first camera of a frame is the world view. Positions are metres (standing eye height is 1.71 above the feet).
- Remote players: `dh2::coop::Snapshot()` (`coop/coop.h`) gives `remotes[i].pos` (feet, world), `name`, `id`, plus `hunterPos` for the local player.
- `overlay_module.cpp` already runs ImGui (DX8 backend) in the Present callback. F8 currently hides everything by returning early.

## What to build
1. **Camera accessor.** `struct ViewCamera { Vec3 pos, x, y, z; float fov; int proj; bool valid; }` and `ViewCamera LastViewCamera();` in `player_tracker.h`. Derive `y` (up) from the two stored axes. Find out from the code and a log whether `fov` is degrees or radians, and whether it is the horizontal or the vertical field of view: enable the existing `SetCamera` diagnostic log (`g_setcam_log`) if needed, and compare with `cam_fov = 90` in `Data\Cfg\console.cfg` and the 640x480 back buffer. Write the evidence into the note.
2. **Projection.** For a world point `P`: `d = P - cam.pos`, `cx = dot(d, x)`, `cy = dot(d, y)`, `cz = dot(d, z)`. On screen when `cz > 0.1` and the projected point lies inside the back buffer. Use the back-buffer size the overlay already reads (`display_size`). Check the sign of `cx` against reality: a marker must not be mirrored left-right.
3. **Marker.** Point = remote feet + 2.0 m up. Draw on `ImGui::GetBackgroundDrawList()`: a small filled diamond (10 px) with a dark outline, and below it `name  42m` (distance from the local hunter, whole metres). Colours by player id: 0 gold `(255,200,60)`, 1 sky blue `(90,170,255)`, 2 green `(110,220,110)`, 3 coral `(255,110,100)`. Fade alpha slightly with distance (never below 50%).
4. **Off screen / behind.** Arrow (triangle, 14 px) on an inset rectangle 24 px from the edges, pointing from the screen centre toward the player's direction. For a point behind the camera (`cz <= 0.1`), use `(-cx, -cy)` direction flipped so the arrow points to the correct side; it must sit on the bottom edge half when the player is straight behind. Name next to the arrow.
5. **Toggles.** `[Overlay] Markers=1` (default on) in `mod/dh2coop.ini`. **F7** toggles markers, foreground-gated like F8 (`is_foreground`). F8 keeps toggling only the status panel: markers must keep drawing while the panel is hidden. If both are off, skip ImGui for the frame as today.
6. **Hide while it would lie:** no markers when `LastViewCamera().valid` is false, when there are no remotes, or for a remote with `packets == 0`.
7. **Test mode.** `[Overlay] MarkerTest=0` (default 0). When 1, also draw grey markers (label `animal <index>`) for the entities in `dh2::coop::Animals()` (`player_tracker.h`), reading their position at entity `+0xE8` with `core::safe_read`. This lets you check the projection in a solo game with no second player.

Keep the code in the file's style (short comments that say why, no new dependencies).

## Test
Before launching, check `Get-Process Main7`. If one runs, wait (poll every 30 s, up to 20 min); the lead and another worker also launch games. Never close a Main7 you did not start. Only solo runs (no `-coophost`/`-coopjoin`).
1. Build. Set `MarkerTest=1` in this worktree's `game\Bin\dh2coop.ini`.
2. `.\run-game.ps1 -ExtraArgs '-nointro -developper -cheats -spmap HUNTMAP4'` (check the script's parameters first). Close CHOOSE ITEM with Enter, Left, Enter via `tools-src\game-input.ps1`.
3. Capture with `tools-src\capture-window.ps1 -NoFocus` (read its header). Then turn the view with `game-input.ps1` mouse or key input so at least one animal is on screen and one is off screen, and capture again. Check in the PNGs that the grey diamonds sit on the deer and that the off-screen arrows point the right way. If a deer is never on screen, walk or turn until one is.
4. Close the game you started. Put `MarkerTest=0` back in the ini you ship (`mod/dh2coop.ini`).

## Commit (in the worktree, branch `markers`)
`git add` your deliverables only and commit. Author `andri <andrice90@gmail.com>`, trailer `Co-Authored-By: Grok 4.7 <noreply@x.ai>`. Do not merge into `main`; the lead reviews and merges.
