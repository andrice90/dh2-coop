# Job: P3 — side-by-side two-instance test windows + stall measurement. YOU OWN GAME LAUNCHES. No Ghidra.

Read first: `STATUS.md` (section "User feedback on the two-instance run"), `docs/mod-learnings.md` (Testing loop), `run-coop-local.ps1`, `tools-src/capture-window.ps1`.

## Goal
Make local two-instance tests observable. Both game windows must be fully visible next to each other and must not overlap.

## Tasks
1. Edit `run-coop-local.ps1`. After both `Main7.exe` windows exist (poll `MainWindowHandle` until non-zero, with a timeout), position them side by side with `SetWindowPos` (user32, via `Add-Type` C#; same style as `tools-src/capture-window.ps1`):
   - game at (0, 0)
   - game2 directly to the right of game's outer window rect, plus a 10 px gap
   - both keep their size (windowed 640x480 client area via the loose `Data\Cfg\console.cfg` the script already writes)

   Print both rects. Keep the existing behavior and parameters, and add `-NoArrange` to skip the positioning.
2. Add `-Capture` (optional switch). Every 10 s while running, capture both windows to `re/notes/coop-local/<time>-game.png` and `<time>-game2.png`, using `tools-src/capture-window.ps1` with absolute paths. Add `re/notes/coop-local/` to `.gitignore`.
3. Measure the stall. Run `.\run-coop-local.ps1 -Seconds 120 -Capture` once. Do not click either window during the run; the foreground window will be whichever started last. Then, from `game\Bin\dh2coop.log` and `game2\Bin\dh2coop.log`, report:
   - `heartbeat` `avg_dt` over time for each instance
   - `coop: remote` rx counts over time
   - which instance was foreground
   - whether the background instance throttles or stops, and after how long

   **Do not attempt to fix the stall.** That is lead work. Only measure and report it.
4. Note what the screenshots show: whether both windows are visible, whether each shows the other's puppet (an NPC body, the mountain guide), and the "CHOOSE ITEM" menu if it is still up.

## Rules
- Edit only `run-coop-local.ps1` and `.gitignore`, and create files under `re/notes/coop-local/`. Write your notes to `re/notes/p3-side-by-side.md`.
- Do not edit `mod/`, `STATUS.md` or `MEMORY.md`.

## Deliverables
The updated script, and `re/notes/p3-side-by-side.md`: usage, measurement tables (avg_dt and rx per 10 s per instance), screenshot observations, and `## Proposed MEMORY facts`.
