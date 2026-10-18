# Triggers (wave "playthrough", agent `triggers`, 2026-10-18)

Worktree `D:\scary hunt\dh2coop-trig`, branch `triggers`, port 27610, Ghidra slot T. The session was cut short
(usage limit): the core OnHit flow is verified on one map; most of the rest is built but not run.

## Commits
- `4572759` Story: coop_tasks console dump of the mission task list (VERIFIED; safe to cherry-pick alone).
- `781769a` WIP: host-decided trigger OnHit (kTriggerHit), fire guards, journal serials, spawner volumes.

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

## Per-map results
| Map | Test | Verdict |
|---|---|---|
| MAP3A | baseline (old code): host water + food, client shelter; client find never counts, host gets only its own | VERIFIED bug (`coop-local/triggers-baseline-map3a`) |
| MAP3A | new code: client teleports into cScrTask07 (shelter) first | VERIFIED fixed: host `OnHit 'cScrTask07' ... for player 1 #1`, commit kind=18, client `applied host OnHit ... state 0 -> 3`, host fire, client fire #1; find task 1 left 3 -> 2 on both (`coop-local/triggers-map3a-onhit`) |
| MAP3A | map start: client fire of cScrTask01 deferred until its own cutscene state left 5 | VERIFIED (same logs) |
| MAP3A | full find x3 -> cScrTask08 task-end hit | UNVERIFIED (not run) |
| MAP8, MAP7, MAP9, MAP11B, MAP2B | recipes in audit-b G1 / audit-c G3 | UNVERIFIED (not run) |
| MAP4B | alternative triggers (G6) | UNVERIFIED |
| MAP1, MAP2A, MAP4A | regression | UNVERIFIED: must run before merging |
| G5 double OnActivate, G8 spawner, serial re-arm, late join mid-cutscene, disconnect | | UNVERIFIED |

Test note: `coop_tp` exactly into a volume does not touch it; teleport ~2-3 m above the box so the hunter falls in
(the shelter fired only from y 4.0, not 1.5). Box bottoms from `coop_triggers`.

## Open items / next steps for whoever resumes
1. Regression first: MAP1, MAP2A (bridge PlaySA, now with serials), MAP4A (car1 OnHit through the host truck).
2. MAP3A: finish the recipe (host water, host Food1, client shelter) and check the forced cScrTask08 hit commits on
   the host and the client logs `task-end OnHit ... left to the host's mission`.
3. MAP8 audit-b G1 recipe (`coop_tp 58.4:2:223.1`), MAP4B G6 recipe (expect `rejected: not armed on the host`),
   MAP8 G8 recipe (expect `player 1 inside spawner`).
4. Late join: `-LateJoin 30` during an OnHit cutscene; expect `will skip the cutscene ... old OnHit during catch-up`.
5. Possible issue to watch: client fire request for an armed idle trigger now waits up to 45 s (it fired at once
   before). If a map relies on a client-only fire path without OnHit, lower `FireRequestForceSec` or revisit.
6. Not done: client-side `UpdateTrigger` still runs locally (spawner MoveToPos on mirrors, G2 interplay); the
   `coop_triggers` label says centre but prints the box bottom for y.
7. Scratch tools (not committed): `scratchpad\trig\t.ps1` (Send-Cmd/Save-Logs/Stop-OwnGames), `TrigListing.java`.

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
