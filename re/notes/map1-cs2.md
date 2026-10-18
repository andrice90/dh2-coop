# MAP1 cutscene 2 "host hang" (agent `map1-cs2`, 2026-10-18)

Worktree `D:\scary hunt\dh2coop-pilot`, branch `pilot-maps` (reset to main `12fa78b`), port 27700, Ghidra slot P.
Evidence (gitignored): `re/notes/coop-local/map1cs2-run1..run4/` (host/client `dh2coop.log`, `debug.log`) and
`map1cs2-*.png` next to it.

## Verdict
**Nothing hangs.** Hugh's `AITaskNPC_PlayAnim cs02` is fine on the host. What stops the host is a **modal hint page**:
about 1.5 s into cutscene 2, `cScrTask04`'s `OnActivate` calls `OpenMenuPage(3)` ("PRESS E TO ENTER AND EXIT AIM MODE",
RETURN continue). The page pushes the "gamemenu" modal, which stops the world: the cutscene script, Hugh's task and the
mission all stand still until RETURN. RETURN is also the cutscene skip key, so the same press ends the page and skips the
cutscene. Solo (mod loaded, no session) does exactly the same. The triggers test pressed RETURN at the wrong moment and
read the paused world as "PlayAnim never ends"; the client (own deferral of 04, below) runs its cutscene to the end.

Why 04 fires inside cutscene 2: `cScrTask03.BeginCutScene` runs `UpdateMissionStatus(REACHED_POSITION, 1)` (log
`mission report action=3` at +0.01 s), which (inferred) ends mission task 1; the engine's task-end path (`GameDll7+0x92269`,
the `trigger event activate id=95` caller) arms `cScrTask04` 1.5 s into the cutscene. 04's box (x -73.1..-45.2, z -18.6..6.6)
nearly contains 03's (x -68.3..-49.9, z -10.3..7.3), so whoever hit 03 is inside an armed 04: OnHit + fire in the same
frame, OnActivate opens page 3. In unmodded play that should be identical (UNVERIFIED, below).

## Fix (commit `2bc4f4a`)
`story.cpp`: hook `cModGameMenu::OpenMenuPage` (`GameDll7+0xbf1b0`, cdecl(int page), the script native; the modal push is
`0xbdc70`, menu-open flag int at `GameDll7+0x186cf4`). Online, a page requested while a trigger cutscene (any trigger event in
state 4/5) or the camera rail (`PlayerInCutscene()`) is up is held; `PumpHeldHints` opens it once the cutscene is over and no
menu is up, or after `[Story] HoldHintsMaxSec` (90). `[Story] HoldHintsInCutscene=1` (0 = old behavior). Solo is untouched.
Only `cScrTask04`'s page 3 is affected on MAP1 (the other `OpenMenuPage` sites are in `OnActivate`, which runs after the
cutscene). Other call sites: MAP3A 8, MAP3B 9/10, MAP6 11, MAPISLAND 13.

## Runs (two windows, host `godMode 1`; 02 at `coop_tp -21.2:75:-1.8`, then 03 at `coop_tp -59.1:75:-1.5`)
| Run | Build | Result |
|---|---|---|
| run1 | before fix | host `OnHit 'cScrTask03' 15:18:56.977`; `trigger event activate id=95 'cScrTask04' caller=GAMEDLL7.DLL+0x92269` 15:18:58.481, `OnHit` .490, `FIRE id=95` .522; `coop_triggers` still `'cScrTask03' state=5` at 15:19:27 and 15:21:13 (136 s); `coop_npcs` Hugh `queue=1` at (-60.4,58.7,-2.9) both times (unchanged). Client: `applied host OnHit 03 ... state 0 -> 4` 15:18:56.985, `defer host OnHit 'cScrTask04' ... until the local cutscene camera ends`, its `OnHit 04` applied 15:19:24.092 = cutscene **27.1 s**, then `cutscene over ... gathering`. (Screenshots of this run used `-NoFocus` = screen copy with other windows on top: not evidence.) |
| solo1 | before fix, `game\Bin\Main7.exe` without `-coophost` | `[solo] trigger event activate id=95 'cScrTask04'` 1.5 s after the hit, `03 state=5` polled every 9 s for 87 s+. `map1cs2-solo-c-game.png` (PrintWindow): hint page 3 on screen. RETURN at 15:27:30: `trigger event activate id=96 'cScrTask05'` (04's state script finally runs) and `FIRE id=94 'cScrTask03'` within 20 ms, state 1: the skip. |
| run2 | before fix, PrintWindow shots | +23 s: host window = hint page 3, client window = Hugh's cutscene, last subtitle line ("not my sharpshootin'"). RETURN on the host: `host skipped the cutscene of 'cScrTask03'` (commit kind 11), `FIRE id=94` 15:34:15.339; client `will skip the cutscene of trigger id 94 (host skipped it)` (its own had ended at 15:34:00.4), page 3 then page 2 on both. |
| run3 | fix | host OnHit 03 15:38:02.020; `trigger FIRE id=95` 15:38:03.562: `hint page 3 held until the cutscene is over (caller EngineDll7.dll+0x29974c)`; screenshot at +20 s shows Hugh's cutscene running on the host; `hint page 3 opened after 25.5 s ok=true` 15:38:29.125, i.e. host cutscene **27.1 s** like the client's (client `applied host OnHit 04` 15:38:29.130). `03 state=3` while page 3 is up; after RETURN `FIRE id=94` (15:39:05.151) and page 2 ("TO PICK UP EQUIPMENT SIMPLY WALK CLOSE TO IT"), client `applied host fire 03` .172. |
| run4 | fix | RETURN on the host at +9 s (no page up): `host skipped the cutscene of 'cScrTask03'` 15:46:57.641 (forwarded, client `skipped cutscene ... ok=true`), `hint page 3 opened after 7.7 s` 14 ms later, RETURN again -> `FIRE id=94`, page 2. No stall. |

Timeline in a normal 2-player game after the fix: 03 touched -> both play the 27 s cutscene -> page 3 on both -> RETURN ->
03's fire -> page 2 -> RETURN. A host who skips with RETURN gets page 3 right after the skip (clients skip too).

## Step 3 (mod not loaded): UNVERIFIED
Made a scratch copy without `dh2coop.asi` (ASI loader `dinput8.dll` kept), started with `+mod data -nointro -developper
-cheats -spmap MAP1`, and tried to walk it by real input (SendInput, only after checking the foreground window was my
own `Main7.exe`). It runs, shows hint page 1, RETURN works; but without the mod there is no position readout or teleport,
and the route to the target range crosses a creek and a footbridge in the dark forest: I never reached 02/03 and stopped.
The scratch copy is deleted. Evidence instead: (a) the page is engine behavior (`cModGameMenu::OpenMenuPage` was not hooked
by any module before `2bc4f4a`; nothing in the mod touches menus), (b) solo with the mod loaded behaves identically
and its hooks pass through when offline, (c) the script/engine chain above. To close it a human can walk to the target
range in an unmodded install and see whether page 3 appears 1.5 s into Hugh's cutscene.

## Open
- Late join mid-cutscene and a held page: not run (catch-up replays fires while a cutscene may be up; the page would wait
  like any other and open after it).
- `PlayerInCutscene()` is a camera-distance guess; a camera that stays > 4 m away (vehicle chases, MAP8 cutscene 2 per
  triggers.md) holds a page up to `HoldHintsMaxSec`. Only 13 `OpenMenuPage` sites exist, none inside a chase.
- Hint pages on a **host** pause everyone's world until the host presses RETURN (clients keep their own sim but the
  host's animals/NPCs stop). Known (audit G14 family); the hold only removes the mid-cutscene case.
- `re/notes/triggers.md` "Open observations" first bullet and `re/jobs/_wave-playthrough.md` test recipes should be
  updated: on MAP1, after `coop_tp` into 03 the world is paused by page 3 (no longer with the fix: page 3 comes after 27 s).

## Proposed MEMORY facts
- MAP1 cutscene 2 is not stuck: `cScrTask04`'s OnActivate opens hint page 3 (`OpenMenuPage`, `GameDll7+0xbf1b0`, modal)
  1.5 s into it (04's box contains 03's; 03's `UpdateMissionStatus(REACHED_POSITION)` arms 04 through `0x92269`); the paused
  world looked like "PlayAnim never ends". RETURN closes the page and skips the cutscene. Cutscene 2 takes 27.1 s on
  both host and client when not paused. `[Story] HoldHintsInCutscene=1` holds such pages until the cutscene is over.
- `GameDll7+0xbf1b0` `cModGameMenu::OpenMenuPage(int)` cdecl stores the page in `+0x164b2c` and calls `0xbdc70`
  (push modal "gamemenu", pause, `+0x186cf4 = 1`). `+0x186cf4` is the menu-open flag.
- `capture-window.ps1 -NoFocus` copies the screen: with overlapping agent windows it can show another agent's game (a
  foreign "MISSION FAILED" screen appeared in a MAP1 shot). Use `-NoFocus -PrintWindow` (the t.ps1 helper does).
- MAP1 test route by `coop_tp`: 02 `-21.2:75:-1.8`, 03 `-59.1:75:-1.5` (from y 75 the hunter falls into both boxes);
  Hugh waits at (-36.2,61.1,4.5) until 03's cutscene moves him to c3_1 (-60.4,58.7,-2.9).
