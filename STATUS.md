# STATUS — resume here

**Last updated:** 2026-10-16 23:00 (America/Chicago) by the Cursor worker (Grok). User confirmed puppet arms and gun at 23:00. Lead's 21:35 handoff is below.

## Cursor worker log (lead away)
- **21:35** Previous MAP3B pair had already shut down cleanly. That session killed both leopards and never logged a mission end.
- **21:35** `tools-src\story-digest.ps1` written. It reads logs with `FileShare.ReadWrite` because the game keeps `dh2coop.log` open.
- **21:35** Fresh pair: host pid 16668, client pid 8932, MAP3B. Client welcomed as player 1, `animals: client received 2`. Both armed `cScrTask01` (fired), `02`, `06`, `07` before join.
- **21:47** User finished. Report and digest: `re/notes/story-s1-map3b.md`. Shots: `re/notes/coop-local/story-s1-map3b-start-*.png`, `story-s1-map3b-end-*.png`.
  - First attempt: mission failed (code 6) synced, then a same-map `spmap MAP3B` replay.
  - Second attempt: triggers followed through `cScrTask05`. No `CompleteTask` line and no mission-success message. Host loaded `map2a.bsp` itself and committed `kMapStart`. Client ran `spmap MAP2A` and crashed in `cSndDrvDS::WaveRelease` (the known post-replay teardown). Client window stuck on LOADING. Host is in the forest, overlay `animals 7`, `no remotes`.
  - Snakes were not in the 2-animal `cScrAnimal` mirror.
  - In-game pause menus are not synced. Not a small fix; user accepts the desync. See Next steps.
- **21:56** Closed that pair and smoke-tested MAP12B, MAP12C, MAP4A, MAP11B (30 s each). All four clients were welcomed. Animals received: 1, 1, 6, 8. MAP11B's post-join `cScrTask02` arm was applied on the client. MAP12C's client crashed in `cObjNPC::Update` (guide model `mountainguide.skin`, missing `cBodyShotgun1npc`); no client screenshot. The other three had no exception in the kept lines. Shots in `re/notes/coop-local/story-s1-map*.png`.
- **23:00** Puppet arms and gun. Legs already played `walk/forward`. Arms and the shotgun stayed in the idle pose because entity `SetAnim("shotgun1/...")` is the call that creates the missing `cBodyShotgun1npc`. Arm clips are now `cModelPart::SetAnim` on every part except the legs, and the gun mesh gets the matching short clip. 23:00 client log: `arms 'shotgun1/walkforwardalert' parts=3`, `weapon anim 'idlealertneutral' parts=1`. User: "animations, weapon model, all look good now!"
- **22:53** MAP12C client controllable. The opening cutscene waits on the yeti's `AIHuntTask_PlaySA`. Freezing that brain meant `EndCamera` / `DeactivateCutSceneMode` never ran. The client now leaves the yeti alone while its body state is `CutScene`, and holds the host's `cScrTask01` fire until the camera is back on the hunter. 22:52 client log: weapon state left `CutScene` and became `Fire`; hunter moved off the spawn with the view camera on the body. The running pair is this build. A follow-up (not installed yet) ignores duplicate copies of that deferred fire.
- **22:49** Client cutscene camera: `cScrTask01` had `+0x1fc` flag 5 without `0x68130` ever running, so the host fire was skipped. `OnFireCommit` now runs OnActivate when this process has not. Relaunched 22:48. Client log: `flag=5 but OnActivate did not run; clearing it` then `applied host fire 'cScrTask01'`. Host hunter left the spawn and fired. Client view yaw stopped moving at that same moment (cutscene camera released; the client window had no input).
- **22:46** MAP12C user test: client joined ~8s after the host, after `cScrTask01` had already fired (22:42:16 host, 22:42:18 client welcome). Client hunter stayed in `CutScene` and could not play. Yeti position synced (drift 0) but local body state stayed `CutScene` while `SetBodyState` of `Attack`/`Run`/`IdleAttack`/`Hurt` returned -1, so it slid without an animation. `run-coop-local.ps1` now starts the client when the host logs `coop: host start`, not at engine time 8s. Relaunched 22:45: client welcomed in the same second as the host, still inside the yeti's `CutScene`; the host fired `cScrTask01` 5s later. The client applied the arm of `cScrTask02` and did not log `applied host fire` for `cScrTask01` (the trigger was already marked fired). Yeti `SetBodyState` still returns -1 after that.
- **22:17** MAP12C client crash fixed in `puppets.cpp`. Solo MAP12C does not log `cBodyShotgun1npc`. Co-op did, because the puppet body played `shotgun1/idlealertneutral` and the gun mesh played `idlealertneutral`; both ask for `cBodyShotgun1npc`, which this map does not register. Body clips are unarmed `GAME.AP` names again. The gun mesh stays attached and is not `SetAnim`'d. Retest: `run-coop-local.ps1 -Map MAP12C -Seconds 25` exited 0 (client alive the whole window). One `cBodyShotgun1npc` line remains at weapon spawn. The client yeti (`id=24`) logs `SetBodyState 'Attack'/'IdleAttack' -> -1` and does not take the process down. Shutdown still logs the usual nvgpu first-chance.

> **Lead away (~80% of the 5-hour limit). Worker handoff: `docs/handoff-cursor-story.md`** (Cursor CLI, Grok 4.7 Medium Fast; job `re/jobs/story-s1-playtest.md`).
> **Story S1 works on MAP3B:**
> - host-decided trigger arming/firing (`kTriggerEvent` 5 / `kTriggerFire` 6, `mod/src/coop/story.cpp`);
> - the user saw leopards aggro and interact with both players (21:28).
>
> See "PIVOT" below and `re/notes/lead-story-coop.md`.

New session? Follow **Resume protocol** at the bottom. This file is the single source of truth for *where we are*. `PLAN.md` = roadmap (**primary goal: co-op story mode**, hunt maps first), `MEMORY.md` = verified engine facts, `docs/mod-learnings.md` = mod/build/tooling learnings, `re/notes/*.md` = per-topic detail. Grok's Phase 4 write-up: `GROK_p4-session.md`.

## Current phase
**Phase 4 (shared animals + world events): core done, verified with two instances 20:04.**
- **P4b (Grok, `b8c4032`):** clients mirror the host's animals: position, movement yaw, body-state name, health. User-confirmed 16:13: "position looked good".
- **Event sync (lead rework of Grok's P4c `926d351`, 20:04):**
  - Engine map: `re/notes/lead-mission.md`.
  - Every mission end on every map goes through cMission's message call (GameDll `0x91c20`, "MISSION SUCCESSFUL!"/"MISSION FAILED!"). Hooked:
    - The host runs it and commits `kMissionEnd`.
    - A client swallows its own call, and its `CheckMissionStatus` (`0x94e50`) is skipped while online.
    - The client applies the host's commit (status, `SetMapIsComplete` on its own profile, then the message that brings up the results screen).
  - Trophy claim (`cObjAnimal::ClaimCallback` `0x1cd40`): a client's claim becomes a `kTrophyClaim` request. The host runs the real claim, so the host's script ends the hunt.
  - **Test 20:04** (`tools-src\event-test.ps1`):
    - `coop_kill 120` on the host → client `present death ok=true` and the "Go claim your trophy!" hint.
    - `coop_claim 120` on the client → host claim → host mission message → **MISSION SUCCESSFUL results screen in both windows** (`re/notes/coop-local/event-200320-*-2claim1.png`).
- The 16:36/16:53 "both crashed" runs were the Grok Build shell's job object killing the launched games, not the mod. `run-coop-local.ps1` now starts games with `Win32_Process.Create`.
- **Phase 3 (ghost mode): DONE**, user-confirmed 15:30.

### Later results (20:09–20:35)
- **Client hit (verified 20:11):** `coop_kill 120:20` on the client hits the local mirror → `AnimalHit` → host hp 45→25 → host deer state `Run` (flees).
- **Built, compiled, NOT runtime-tested yet** (the user's test windows were open, so no install):
  - **Map flow.**
    - Main7 `cSpServer::StartGame` (`0x1ed80`) hook dispatches `OnMapLoaded` on same-map replays (no `cBspTree::Load` then).
    - `StopServer` (`0x1b060`) hook adds a new module callback `OnMapUnloading` (entities still valid).
    - Host commits `kMapStart` (4); clients follow with `spmap`/`menumap`. A same-name commit means replay.
    - Animal mirror is gated on same map.
    - Test: `tools-src\mapflow-test.ps1`.
  - **Friends UX.** `[Coop] AutoRole=off|host|join`, `JoinAddress`, `JoinRetrySeconds` (retry while offline; `coop_leave` stops it).
  - **P5a.** On the host, puppets are added to the animals' attack-target list (`cAIHuntAnimalInterface::AddAttackTarget`, CommonDll singleton `0x1aea7c`) and removed before teardown. Animals natively choose the closest target. `[Puppets] AttackTarget=1`. Plan: `re/notes/lead-combat.md`.
- **Story prep:**
  - Worker inventory `re/notes/story-inventory.md` (cursor-agent Grok 4.7, 15 min; gitignored: quotes ~2000 script lines; regenerate with `re\scripts\story_inventory.py`).
  - Lead design `re/notes/lead-story-coop.md`: host decides, everyone runs the scripts; sync trigger-volume activations (`cObjTrigger::OnActivate` CommonDll `0x4dee0`). First story map: MAP3B.

## In flight
| Job | Owner | Log | Deliverable |
|---|---|---|---|
| (none) | | | story-inventory finished 20:23, lead-reviewed |

Worker jobs now run through `tools-src\cursorjob.ps1` (Cursor CLI, Grok 4.7 Medium Fast). User decision 2026-10-16: Grok Build performance not good enough; `grokjob.ps1` retired.

## PIVOT 2026-10-16 21:20 (user): story mode first
User: "the real goal is singleplayer… switch to getting story mode missions to sync". Hunt-map map-change polish is parked.
- **Story S1 started.**
  - `run-coop-local.ps1 -Map MAP3B` loads the first-choice story map on both instances. They join, see each other's puppet and mirror its 2 animals (screens `re/notes/coop-local/story-3b-*-0.png`).
  - New module `mod/src/coop/story.cpp` logs every:
    - L1 trigger-volume activation (`cObjTrigger::OnActivate`);
    - L2 script trigger event (`0x68eb0`);
    - `CompleteTask`.
  - At map start, both sides activate the same L2 events: `cScrTask01/02/06/07`.
  - Next: the user plays MAP3B co-op; read the logs to see which decisions diverge, then build L1 sync (`re/notes/lead-story-coop.md`).
- **Hunt-map state at the pivot:**
  - Kill, death fall, client hit, claim and mission end all work in automated two-window tests (20:38–21:16).
  - Map following works for A→B→A.
  - **Known issue:** after a same-map replay (`spmap` of the current map), the *next* world teardown crashes in `cSndDrvDS::WaveRelease` (both windows).
    - It is not the puppet-weapon refcount ring (fixed, balanced), not the BSP reset path (a forced full reload still crashes), and not the stale command file.
    - The "weapons off" controls were invalid (`mod/dh2coop.ini` has no `Weapons` key), so whether puppet weapons are involved is still unknown.
    - Workaround: don't replay the same map; restart the game instead.

## Next steps (in order) — story first (after the 21:20 pivot)
**Worker (now):**
- **S1a:** done. Play-through and the four-map smoke table are in `re/notes/story-s1-map3b.md`. MAP12C's client crashed in `cObjNPC::Update` (mountain guide); the other three joined.

**Lead (next session), in order:**
1. Review `re/notes/story-s1-map3b.md`.
2. **Story map transition (observed 21:43).** MAP3B's successful ending did not call the mission message. The host loaded `Bsp\map2a.bsp` on its own, then committed `kMapStart` `MAP2A`. The client applied that map's `cScrTask01` arm *before* `spmap` finished (`story: trigger commit id=117 'cScrTask01': no such trigger here`), then crashed in `cSndDrvDS::WaveRelease` during `StopServer`. That crash matches the known same-map-replay teardown: the attempt before this one was `Mission failed` plus `spmap MAP3B` while MAP3B was already loaded. Question: which call loads the next story map, and can `kMapStart` be committed only after the client has loaded it?
3. **Snakes.** User: "I killed them as the client and the host and saw one per game instance, not synced across both." MAP3B's mirror tracked exactly 2 `cScrAnimal` entities (vt `GAMEDLL7.DLL+0xff470`, ids 47 and 48, the leopards). Their deaths did sync. `cObjSnake` / `cObjCobra` are separate classes. Question: which vtable are the MAP3B snakes, and should the mirror include them?
4. **In-game pause / overlays.** User: a pause overlay does not pause in-game events; when the host pauses, client animals stop animating; they want a host pause to pause every client, and a client pause to pause nobody else. They accept the desync if it is not a small fix. It is not: the mod has no pause message. A host modal blocks the host sim, so animal snapshots freeze (that part already matches what they saw). A client modal does not block the host, so events keep arriving. Sending "host paused" requires detecting menu-open, and the game thread is inside the modal so the usual frame hook is not running. `cHUDSlowMotion::Enable` is already a no-op in co-op; this is the game's own menu, not slow-mo. Leave it for a later design.
5. **MAP12C yeti states.** The client crash is fixed (see the worker log). Still open: the mirrored yeti rejects `Attack`, `IdleAttack`, and `Run` (`SetBodyState -> -1`). Those names are the deer set. Question: which body-state names does the MAP12C yeti actually have?
6. **Player damage to clients (Phase 5 for story):**
   - Host animals must be able to hurt client players. Puppets can't be attack targets (no physics → `ProcessAttackStuck` crash).
   - Options: give puppets a physics body that physics doesn't pull back to spawn; or redirect damage meant for the host hunter.
   - Plan: `re/notes/lead-combat.md`.
7. **Cutscenes:** they run on both via fired triggers. Decide skip/timing policy; check `ExecuteScript` side effects. The user did reach the next-mission cutscene on the second MAP3B attempt.
8. **Tasks/HUD:** `CompleteTask` never logged during the whole MAP3B play-through (the hook was installed). Either this map does not call it, or the success path uses something else. Check the HUD objectives against that.
9. **NPCs** (guide, companions) on other maps: generalize the animal mirror. Same question as the snakes above.
10. **Death policy** (user decision). The first attempt ended in `Mission failed` / `Your hunt has ended!` (code 6), and that message did sync.
11. The hunt-map same-map-replay teardown crash. Reproduced here: MAP3B replay, then the MAP2A load killed the client in `cSndDrvDS::WaveRelease`.

**Hunt-map leftovers (lower priority now):**
0. Map flow beyond A→B→A and the P5a puma test are parked.
1. **User play check (hunt map, two windows `.\run-coop-local.ps1`):**
   - Host kills a deer: does the client see it **fall**? Log: client `animals: present death id=… ok=true`. If it stays standing, try unfreezing the brain after `PresentDeath`.
   - Client walks to the corpse and presses **F** (real key): both windows should show MISSION SUCCESSFUL. Log: client `events: client claim id=…`.
   - Client **shoots** a deer: host log `animals: AnimalHit id=… hp a->b`. Does the host deer flee? Death should follow on lethal damage. Never seen in a log yet.
2. **Hunt-map gaps (lead):**
   - Scripts' `"hunter"` = local player only (spook distance, trophy distance, predator target / safe distance). Option: a host-side `GetDistBetween2Ais` (`0x9200`) hook that resolves `"hunter"` to the nearest player.
   - The animal AI's own perception of the hunter (C++ brain) sees only the host hunter. That is Phase 5.
   - **Results screen:** ESC replay / RETURN continue are per-process. The host's choice should drive the clients.
   - **Map flow:** the host's menu map choice should load the same map on the clients. Today both use `-spmap HUNTMAP4` on the command line. Join UX for friends (ini or overlay, not command line).
   - Other hunt maps: check animal ids and counts per map (HUNTMAP1–n).
3. **Story-mode prep:**
   - Inventory the story map scripts (natives, `"hunter"` uses, cutscenes/QTE, triggers, companions, spawns). That is a worker job → `re/jobs/story-inventory.md` via `cursorjob.ps1`.
   - Then the lead writes the co-op story architecture: host runs scripts, clients mirror entities, outcome natives host-only.
4. Leftover Phase 3 checks (low): shotgun/rifle in the left hand, crouch-pose verdict, crouch key.
5. Polish (low): feet slide, puppet pitch, remote weapon sounds.
6. Cleanup: the Draw diagnostic hook and spike code (solo only, off by default).

## Done
- **Phase 4:**
  - P4a net messages (`14250e0`).
  - P4b animal mirror (`b8c4032`).
  - Event envelope `GameEvent` id 48 (`926d351`).
  - Mission end + trophy claim sync (lead, 2026-10-16 20:04).
  - Automation channel `Bin\dh2coop.cmd` + `coop_kill` / `coop_claim`.
- **Phase 3:**
  - Puppet spawn recipe + physics unlink.
  - NPC body.
  - Directional clips.
  - Render-camera view yaw/pitch.
  - Action flags.
  - Armed clip sets.
  - Weapon model attach.
  - Test tooling (`game-input.ps1`, `move-test.ps1`, `focus-test.ps1`, `capture-window.ps1 -NoFocus`, `event-test.ps1`).
- **P2b overlay** (F8), **Phase 2 core**, **Phase 1** (Route E), **Phase 0**: see MEMORY.md / PLAN.md.

## Blockers / risks
- Claude usage limits: keep Claude on expert work and review. Delegate defined jobs to Grok 4.7 via `tools-src\cursorjob.ps1`. When the 5-hour window nears 90%, write a handoff doc for the worker.

## Resume protocol (new session)
1. Open the session in `D:\scary hunt\dh2coop` (loads `CLAUDE.md`; the REA MCP is registered for this folder only). Ghidra headless also works through `re\scripts\ghrun.ps1` / `decomp.ps1` (slot L = lead).
2. Read this file, then the `PLAN.md` decisions, then skim `MEMORY.md`, then `re/notes/lead-mission.md` and `re/notes/lead-animals.md`.
3. Check jobs:
   - For each in-flight log `re\jobs\<job>.out.log`, look for the `=== end` line.
   - `Get-Process node` with `cursor-agent` in the command line means a worker is running.
4. Run `git status` / `git log -5` to see uncommitted work since the last checkpoint.
5. Build and test:
   - Build: `.\mod\build.ps1` (fails to install while `Main7.exe` runs).
   - Solo test: `.\run-game.ps1 -ExtraArgs '-nointro -developper -cheats -spmap HUNTMAP4'` (no `-Seconds` when the user plays).
   - Two windows for the user: `.\run-coop-local.ps1`. Close CHOOSE ITEM with Enter, Left, Enter.
   - Automated: `.\tools-src\event-test.ps1` (kill + claim + mission end), `.\tools-src\move-test.ps1`, `.\tools-src\focus-test.ps1`.
   - Drive a running game: write console lines to `game\Bin\dh2coop.cmd` / `game2\Bin\dh2coop.cmd`.
   - Logs: `game\Bin\dh2coop.log`, `game2\Bin\dh2coop.log`. A new launch truncates them: quote what you need first.
6. Continue with **Next steps** above. Update this file after every step and commit (timestamps from `Get-Date`). Edit docs with the Write/Edit tools, not PowerShell `Get-Content`/`Set-Content`.
