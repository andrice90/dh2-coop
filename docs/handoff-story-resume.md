# Handoff — story co-op, pick up on the next map

**Written:** 2026-10-17 00:05 (America/Chicago). **Last commit:** `28cb014` plus this note.
User is Andrew. They play the two windows and will say what looked wrong. Left window is the host (`game\`), right is the client (`game2\`).

Read `STATUS.md` first (section **Next steps — pick up here**). This file is the short version. Older handoff: `docs/handoff-cursor-story.md` (MAP3B, historical).

## What is confirmed

- Puppet legs, arms, and gun look right.
- MAP3B `exit_1` loads MAP2A on both sides when it is a fresh load.
- Uncle Hugh (`obj_guide`, id 75 on MAP2A) stays matched for the map. Legs follow the host body-state name. Gun and arms do not.
- Objectives sync (host trigger arm `kTriggerEvent` / fire `kTriggerFire`).
- Paired cutscene meshes sync. MAP2A bridge is `PlaySA("cs02", "fallbridge")` on Hugh. The client cutscene camera plays. User: the model changed and the cutscene played.
- Every `PlaySA` pauses pose writes on the caller and, if we mirror them, the partner, until the host body-state name changes.
- Animal deaths sync, including one that was still in `CutScene` on the client. That skip now applies only while the host sample is also `CutScene`.
- **F9 works** (user, 00:03). A dead client stands up at the host with health 100. The dead mesh is cleared on every model part (`GameDll +0x480f0`, the same call death uses with `"Death"`). Hunter `+0x6CC` is cleared (that flag skips player update and mouse look) and `+0xC1` bit 2 is cleared. Host death still ends the level. A client death does not.

## Do not

- Write `coop_trigger` into a session Andrew is playing. It arms and fires a one-shot trigger (`cScrTask03` is the MAP2A bridge). Both sides then skip the event when he walks there.
- `SetBodyState` / `SetAnim` an NPC's weapon, or `SetAnimOnParts` with a `handgun3/` (or rifle/shotgun) prefix on that gun. The engine pops a fatal dialog: `Could not forward animation normaltoalert from handgun3/`. Client crash 23:32.
- `spmap` the map that is already loaded. The next teardown crashes in `cSndDrvDS::WaveRelease`. Restart the process.
- Edit `MEMORY.md`. Proposed facts go in a note.
- Write to `D:\scary hunt\Activision Value\...`.
- Close windows he is playing unless he asked, or the test you launched is stuck.

## How to run

- Build: `.\mod\build.ps1`. Install fails while `Main7.exe` is running.
- Two windows: `.\run-coop-local.ps1 -Map MAP2B` (or whichever map). No CHOOSE ITEM on story maps.
- Console into a live game: `[IO.File]::WriteAllLines` with an **absolute** path to `game\Bin\dh2coop.cmd` or `game2\Bin\dh2coop.cmd`. PowerShell `[IO.File]` resolves relative paths against `C:\Users\andri`.
- Test commands: `coop_down` (local hunter to `Dying`, does not end the level), `coop_kill <id>[:dmg]`, `coop_claim <id>`, `coop_trigger <name>`.
- Keys: F8 hides the overlay. F9 is the client revive (`tools-src\game-input.ps1` knows `F9`). `GetAsyncKeyState` sees F9 even when the game window is not in front, which matters because these windows fight for focus.
- Logs: `game\Bin\dh2coop.log`, `game2\Bin\dh2coop.log`. A new launch truncates them. Engine dialog text is `Bin\debug.log`.

## Where the code is

| What | Where |
|---|---|
| Trigger arm/fire | `mod/src/coop/story.cpp` |
| `PlaySA` replay (`kPlaySA` = 8) and prop `SetAnim` (`kPropAnim` = 7) | `story.cpp`, kinds in `mod/src/coop/events.h` |
| NPC mirror (`kNpc` on the animal snapshot) | `mod/src/coop/animals.cpp` |
| Cutscene pose hold | `animals::HoldCutscene` / `HoldCutsceneNamed`, called from `OnPlaySACommit` |
| F9 revive | `PollClientRespawn` in `mod/src/coop/coop.cpp` |
| Part state names | `entity::PartStateName` in `mod/src/engine/entity_api.cpp` |

`AnimalState.arms` is on the wire (weapon body-state name). Clients do not apply it. Do not start applying it to the gun.

## Next

Play MAP2B and the maps after it. Watch objectives, `PlaySA` meshes, NPC position, and animal deaths. F9 if the client dies.

Still open, in the order in `STATUS.md`: NPC arms and gun (without touching the weapon script), snakes, host animals hurting clients, MAP12C yeti pose names, pause menus, same-map replay crash, `kMapStart` arriving before the client has loaded the map.
