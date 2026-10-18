# Triggers (wave "playthrough", agent `triggers`, 2026-10-18)

Worktree `D:\scary hunt\dh2coop-trig`, branch `triggers`, port 27610, Ghidra slot T. Session 1 built it and verified one map; session 2 (2026-10-18, rebased onto main `a5f60c4`) ran the regressions and
the recipes below. All results are from two windows (host + client), `godMode 1` on the host.

## Commits (rebased onto main `a5f60c4`; `coop_tasks` `4572759` and the firewall commit `eccc1ef` are already in main)
- `a37f385` WIP: host-decided trigger OnHit (kTriggerHit), fire guards, journal serials, spawner volumes.
- `0d0c43e` Notes (session 1).
- `245cf5e` Story: client cutscene watchdog (a cutscene that waits on frozen NPC tasks is skipped locally).

## What was built (`mod/src/coop/story.cpp`, `events.cpp`, `events.h`, `mod/dh2coop.ini` `[Story]`)
1. `coop_tasks`: logs mission status and each task (id, shoot/find/race from the vtable, TASK_* state 1-6,
   target, count left, bonus, end/fail trigger names).
2. OnHit hook `0x6b140` (`hk_TriggerHit`). Host: if the engine's tests pass (armed, ActivateBy name, state != 1,
   script OnHit, `+0x1e8 == 0`, WaitTask in the host mission), commit `kTriggerHit` (18, i0 = OnHit serial) and
   then run the original. Client: a touch that would run becomes a request (one per 3 s); task-end OnHits (return
   addresses `0x9705d`, `0x970df`, `0x98e54`) and touches by puppets / mirrored animals / NPCs are swallowed.
   Host request handler runs the original with `FindEntity(+0x224)` (`0x6d80`) or the local hunter.
   Client apply: disarm `0x68fd0`, class manager vt `+0x28` cdecl(mgr, `+0x208`, `+0x1b4`), `+0x1e8 = 1`.
3. Client commit queue (`Submit` / `PumpDeferred`): arm, OnHit, fire in arrival order; OnHit and fire wait for the
   cutscene camera; a fire also waits (max 30 s) while that trigger's own state is 4/5 here. Late join: an old
   OnHit (journal age f0 > 3 s) gets `WatchSkip` like old fires.
4. Host fire requests (G5/G6): states 2-5, and armed idle with SyncOnHit, wait for the host's own fire; forced after
   `FireRequestForceSec` (45), the host's own later fire then skipped once. Not armed and idle -> rejected.
   A client arm request never re-arms a trigger the host already armed once.
5. Serials (G8/M5): kinds 5, 6, 18, 7, 8 carry a per-id serial in i0; `FindJournal` compares i0 for them. Clients
   ignore commits with a serial they already applied (stops a handed-back arm from re-arming a used trigger).
   Prop anim / PlaySA: same key again counts only after a 2 s pause, max 16 commits per entity per map.
6. Spawners (audit-b G8): Update hook `0x68800`; host sets `+0x1f8 = 1` when a living puppet is inside the AABB
   (`+0xc4`/`+0xd0`, entity `+0xe8`) of an armed trigger with UpdateTrigger.
7. Test commands: `coop_triggers` (all trigger events: armed/hit/state/ActivateBy/functions H B A U, WaitTask
   mask, box x:ybottom:z and size, serials), `coop_hit <name>` (OnHit through the hook with this process's
   activator; on a client it sends the request).

## Engine facts checked in Ghidra (slot T)
- `0x6b140` OnHit ends `RET 0xc`: thiscall(trigger, activator, pos*, dir*). All callers pass `nullVect` for pos/dir
  (task end `0x96da0`/`0x98a50`, vehicle `0x3b8c0`, world item force `0x432c0` with the item as activator).
  An activator with a null name skips the ActivateBy compare. VERIFIED.
- Script call in OnHit: push obj, fn, mgr; `CALL [vt+0x28]`; `ADD ESP,0xc` (cdecl). VERIFIED.
- `0x68130` fire: `RET 4`, runs `+0x20c`, `+0x1fc = 1`, stores NextTrig; does not disarm. `0x68800` Update `RET 4`.
- `ActivateTrigger` native `0x84a0` = FindEntity + arm `0x68eb0` (it never fires). VERIFIED.
- Task list: `+0x1f8` +4 = last index, +0x10 = array; vtables shoot `0x111c0c`, find `0x111c98`, race
  `0x111d08`, base `0x111b54`; script constants TASK_STARTED 2 ... TASK_HAS_END 6 (`0xc3b7`). VERIFIED runtime.

## Session 2 results (evidence under `re/notes/coop-local/triggers-<tag>/`, gitignored)
| Test | Verdict | Decisive lines |
|---|---|---|
| MAP1 opening, 02, 03, 04 (`map1-run1`, `map1-run2`) | VERIFIED, no regression | host `OnHit 'cScrTask02' ... #1` + commit kind=18; client `applied host OnHit ... state 0 -> 3`; 03 client state 0 -> 4 (cutscene); both missions `task 1 race state=6`; client 04 OnHit/fire deferred while its 03 cutscene runs, then applied |
| MAP1 client fire at map start | behavior change, better | client `[client] trigger FIRE id=92` -> host `fire request 'cScrTask01' ... host state 5 armed, waiting for the host's fire (0 s / 3 s)` -> host's own fire 9 s later. Base (main) ran OnActivate at once on the client request, which opened the hint page (pause) in the middle of the host's intro cutscene (`map1-base-run`) |
| MAP2A bridge, client first (`map2a-bridge`, `map2a-postrebase`) | VERIFIED, fixes PA-2 | client `OnHit 'cScrTask03' ... sent to host`; host `OnHit ... for player 1`, `PlaySA id=75 'cs02|fallbridge' #1`, commit kind=8; client `applied PlaySA`; both screenshots show the collapsed bridge |
| MAP4A truck (`map4a-truck`) | VERIFIED, no regression | host truck `OnHit 'cScrTask02'..'cScrTask10' ... by id=68 name='car1'` (9 hits, one per trigger), client `applied host OnHit ... state 0 -> 3/4` for each; no client-originated hit; chase ran 2.5 min |
| MAP3A find x3 -> cScrTask08 (`map3a-find3`) | VERIFIED | host water (04) + Food1 (02), client shelter (07): host `OnHit 'cScrTask07' for player 1`, `OnHit 'cScrTask08' ... (task end) #1`, commit kind=18 for both; `coop_tasks` on both `task id=1 find state=6 (ended) left=0`, task 5 race started on both; client applied host OnHit 08 and fire 08 |
| MAP8 find the guide, client first (`map8`) | VERIFIED, fixes B-14 | host `OnHit 'cScrTask08' ... for player 1`; host and client `task id=1 find state=6 (ended)` |
| MAP8 end trigger cScrTask15, client first (`map8`) | VERIFIED, fixes B-15 | host `OnHit 'cScrTask15' ... for player 1`, `CompleteTask 6..9`, `CompleteTask 10`, then `host mission message code=5 'MISSION SUCCESSFUL!'` (commit kind=2), client `applied host mission end code=5`. Also: `player 1 inside spawner 'cScrTask23'... UpdateTrigger runs for them on the host` (G8 spawner volumes work) |
| MAP7 girl cScrTask07 + task 3 in cScrTask04, client first (`map7-girl`) | VERIFIED, fixes B-11 and B-12 | host `OnHit 'cScrTask04' ... for player 1` then `CompleteTask 3`; chain 04 -> 05 -> 12 (arms 07; `CompleteTask 4/5`) -> `OnHit 'cScrTask07' ... for player 1`; host and client `task id=1 find state=6` |
| Client cutscene watchdog (`map8-watchdog`) | VERIFIED | MAP8 client first at 08: client cutscene 2 never ended (frozen tribesmen MoveToPos waits; host's took 34 s; client still in it after 2.4 min until RETURN). With the watchdog: `will skip the cutscene of trigger id 150 (its cutscene still runs after the host's fire)` 20 s after the host fire, `skipped cutscene ... ok=true`, host fire applied with flag=3 |
| MAP4B two endings (`map4b-double-end`) | observation | host takes 15, client takes 16 during the host's cutscene: both OnHit run (16 still armed), host fires 15 and 16 in the same frame (audit-b B-5 double ending, not a soft lock). The client was held 90 s behind its local cutscene of 16 and released by the max-time watchdog (`its cutscene runs over the time limit`). The G6 line `rejected: not armed` did not occur: the client only requests a fire for a trigger whose OnHit it already ran |
| Late join mid-cutscene, 3rd client (`map1-latejoin`) | VERIFIED | `synced with host on 'MAP1' (9 commits replayed)`, `will skip the cutscene of trigger id 94 (old OnHit during catch-up)` + `id 93/95 old fire`, OnHits applied in order, `teleport to host (join catch-up)` |
| Post-rebase smoke MAP2A (`map2a-postrebase`) | VERIFIED | same as the bridge row on main `a5f60c4` |

Test facts learnt (see Proposed MEMORY facts): MAP1 hint pages (`OpenMenuPage` in OnActivate) pause the game of that
process until RETURN, and RETURN also skips a cutscene: after a fire press `coop_key enter` once per window (check the
hunter does not fall after a `coop_tp`). Box y values printed by `coop_triggers` are the brush bottom, not terrain
height: use the hunter's real `pos=` from the log (+3 m) as the `coop_tp` y.

## Open observations (not fixed)
- **Resolved 2026-10-18 (`re/notes/map1-cs2.md`): not a hang.** `cScrTask04`'s OnActivate opens modal hint page 3 about 1.5 s into cutscene 2 and pauses the host's world until RETURN; main now holds hint pages until a cutscene ends (`[Story] HoldHintsInCutscene`). Original observation:
- **MAP1 cs02 (cScrTask03) hangs on the host in every build, base included**: `AITaskNPC_PlayAnim cs02` on Hugh never
  ends (Hugh stays `Idle`, trigger state 5 for 100 s+, also with Hugh settled at his path end 70 s). The client's
  cutscene ends by itself (~27 s, frozen NPC). RETURN skips it (host skip is forwarded). Not caused by this branch
  (`3e33695` base build hangs the same way); unknown whether it also hangs in a vanilla game.
- With `kTriggerHit` a client's task-end OnHit is swallowed, so a trigger gated by a task that only the client's mission
  completed (client kill, audit-a X3/X9) never fires: the old client fallback is gone. Needs the kill-credit fix (life).
  Same for a host-refused OnHit (`WaitTask` mask not met in the host mission): no fallback.
- The 30 s forced fire (`kOwnSequenceWaitMs`) used to leave the cutscene script alive; with the watchdog it is skipped
  first. `PlayerInCutscene()` (camera > 4 m from the feet) is false during MAP8 cutscene 2, so the "gather" teleport
  after a cutscene did not run for the client there.
- One first-chance access violation in `dh2coop.asi` (+0x1280bd) while the host fired MAP8 `cScrTask69`
  (`not forwarding SetAnim 'ani1'`, SEH-guarded, harmless there).

## Per-map results (session 1, kept)
| Map | Test | Verdict |
|---|---|---|
| MAP3A | baseline (old code): host water + food, client shelter; client find never counts, host gets only its own | VERIFIED bug (`coop-local/triggers-baseline-map3a`) |
| MAP3A | new code: client teleports into cScrTask07 (shelter) first | VERIFIED fixed: host `OnHit 'cScrTask07' ... for player 1 #1`, commit kind=18, client `applied host OnHit ... state 0 -> 3`, host fire, client fire #1; find task 1 left 3 -> 2 on both (`coop-local/triggers-map3a-onhit`) |
| MAP3A | map start: client fire of cScrTask01 deferred until its own cutscene state left 5 | VERIFIED (same logs) |
| MAP3A | full find x3 -> cScrTask08 task-end hit | VERIFIED in session 2 (above) |
| MAP8, MAP7 | audit-b G1 recipes | VERIFIED in session 2 (above) |
| MAP9 REACHED 1 (cScrTask11), MAP11B, MAP2B, MAP2C | recipes in audit-b G1 / audit-c G3 | UNVERIFIED (not run) |
| MAP4B | alternative triggers (G6) | not reproducible, see session 2 |
| MAP1, MAP2A, MAP4A | regression | VERIFIED in session 2 |
| G8 spawner, late join mid-cutscene | | VERIFIED in session 2 |
| G5 double OnActivate (client fire requests wait), serial re-arm, disconnect | | waits seen (3 s, resolved by the host fire), never forced; serial re-arm and disconnect UNVERIFIED |

Test note: `coop_tp` exactly into a volume does not touch it; teleport ~2-3 m above the box so the hunter falls in
(the shelter fired only from y 4.0, not 1.5). Box bottoms from `coop_triggers`.

## Open items / next steps for whoever resumes
1. MAP9 (cScrTask11 REACHED 1), MAP2B 03/08 (`WaitTask` gates), MAP2C, MAP11B recipes (audit-b G1 / audit-c G3) are not run.
2. Kill-credit fix (life) before relying on gates that only a client's kills satisfy (MAP1 08 `WaitTask(5)`, MAP2B, MAP3A 14).
3. Decide the MAP1 cs02 host hang (see open observations); a host-side watchdog was not added (a host skip is a
   player's choice and is forwarded).
4. Not done: client-side `UpdateTrigger` still runs locally (spawner MoveToPos on mirrors, G2 interplay); the
   `coop_triggers` label says centre but prints the box bottom for y.
5. Late joiners get the replayed hint pages (they pause that process until RETURN); not addressed.
6. Scratch tools (not committed): `scratchpad\trig\t.ps1` (Send-Cmd/Save-Logs/Stop-OwnGames/Story/Hpos/Launch), `TrigListing.java`.

## Proposed MEMORY facts
- `cObjTriggerEvent::OnHit` GameDll7 `0x6b140` is thiscall(trigger, activator, pos*, dir*) with `RET 0xc`; every
  engine caller passes `nullVect` for pos/dir. An activator whose name (`+0x198`) is null passes the ActivateBy test.
  Task-end calls return to `0x9705d`/`0x970df` (cMissionTask::End `0x96da0`) and `0x98e54` (`0x98a50`).
- Running a trigger's script function: class manager vt `+0x28`, cdecl(mgr, fn `+0x208`/`+0x20c`/`+0x210`,
  obj `+0x1b4`). Applying a host OnHit on a client = disarm `0x68fd0` + that call + `+0x1e8 = 1`; the client's own
  Update then plays BeginCutScene (verified MAP3A cScrTask07, state 0 -> 3).
- Mission task list `+0x1f8` (+4 last index, +0x10 array); task vtables shoot `0x111c0c`, find `0x111c98`, race
  `0x111d08`; TASK_* script constants 1..6 = not started, started, finished, paused, failed, has end.
- `coop_tp` straight into a trigger box gives no OnHit; arriving by falling a few metres does.
- Session 2: MAP1 hint pages (`OpenMenuPage` in a trigger's OnActivate) pause that process's game until RETURN, the
  same key that skips a cutscene. A test that teleports must press `coop_key enter` after such a fire and check that
  the hunter falls.
- `coop_triggers` y is the brush bottom; world height differs per map (MAP1 ground 60, box 31..71). Teleport to the
  hunter's logged ground y + 3 (MAP1 +15, MAP3A 4.5, MAP8 6, MAP2A 55 from ground 39).
- A client cutscene that waits on NPC tasks other than `PlayAnim` (MoveToPos, LookToEntity: MAP8 cutscene 2) never
  ends on a client (frozen NPC brains), contrary to the PlayAnim-only case (MAP2A cs1). `story.cpp` now skips it
  (`[Story] StuckCutsceneSec` 20 after the host's fire, `StuckCutsceneMaxSec` 90 from its start).
- Host-run OnHit for a client's touch covers: MAP2A bridge (PlaySA from the host), MAP3A shelter/task-end 08, MAP7
  cScrTask04 CompleteTask(3) and cScrTask07 find, MAP8 cScrTask08 find and cScrTask15 CompleteTask(6..10) + success.