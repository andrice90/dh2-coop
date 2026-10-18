# Pilot maps MAP11A / MAP11C (job `pilot`, wave "playthrough", 2026-10-18)

Worktree `D:\scary hunt\dh2coop-pilot`, branch `pilot-maps`, port 27660, Ghidra slot P. Stopped early on the lead's
request (session limit): MAP11A is tested start to exit; MAP11C only through its first wave.

## Commits
- `99ac420` Pilot maps: accept cObjPilot as the local player (`player_tracker.cpp/.h`, `combat.cpp`, `coop.cpp`)
- `1d4cb29` Pilot maps: the host's helicopter flies, clients follow its clip time (new `heli.cpp/.h`; `protocol.*`,
  `session.*`, `coop.cpp/.h` plumbing for msg 12; `net-test`; `dh2coop.ini` `[Coop] HeliSync=1`)
- `bbb94e6` Pilot maps: stand the other pilots beside us, not in the door (`puppets.cpp`)

Files outside my ownership, small edits: `combat.cpp` (LocalHunter accepts the pilot; StartKnock refuses a pilot),
`coop.cpp` (F9 and TeleportToHost skip a pilot; HeliStates dispatch + `BroadcastHeliStates`), `coop.h`,
`protocol.h/.cpp`, `session.h/.cpp` (msg id 12 `HeliStates`, as allocated), `puppets.cpp` (`pilot_spot`, two ini keys).
No `PlayerState` flag and no event kinds were needed. Note for the lead: main already uses `PlayerState` bits
`1<<4` (`kFlagDead`) and `1<<5` (`kFlagKnocked`), so the wave table's "pilot 1<<4, flow 1<<5-1<<6" collides with main.

## What was done
1. **Local player = pilot** (audit-c G1). Tracker accepts GameDll vtable `0x109d48` (`cObjPilot`) next to `0x106ff0`
   and sets `LocalPlayerSample::pilot`. The tracker now refreshes the local `pos`/`angle`/view every frame (the 1 s scan
   only chooses the entity), so everything reading `LocalPlayer().pos` is live. `combat.cpp` `LocalHunter()` accepts the
   pilot; `StartKnock` refuses on a pilot. `TeleportToHost` (join catch-up, re-check, after-cutscene gather,
   `coop_tohost`) and F9 do nothing for a pilot. `SendLocalState` needed no change (the pilot's `+0xA4` = helicopter id
   gives `kFlagRiding`).
2. **Helicopter** (`heli.cpp`). The flight is a baked clip on the fly model (`cObjHelicopter +0x210`, part 0, body
   machine `cBodyHelicopter`). `cObjHelicopter::Update` (`0x233f0`) advances the model, then puts the entity on the
   `masscenter` bone (pos -> vt `+0xa4`, angles -> vt `+0xb0`); the pilot is attached to the entity. So clip + clip time
   is the whole pose. Host streams `HeliStates` (msg 12, unreliable, 20 Hz): state, clip, clip time (`part +0x84`),
   speed (`+0x88`), clip length, entity pos (for drift logs), flags. Client hooks `0x233f0` and before the original:
   holds the script task flag (`cScrHelicopter +0x30`, via `GetScrObject`) at 0 so only the host ends a loop; on a host
   state edge (or 500 ms mismatch) calls the game's BSMGoToState body `0x23cc0`; on a clip edge (or 300 ms) part
   vt `+0x58` SetAnim; snaps the clip time (part vt `+0x6c`) when it is > 50 ms off the host's extrapolated time.
   `[Coop] HeliSync=0` = old behaviour, drift still logged (`heli: drift`).
3. **Other pilots**: riding puppets (kFlagRiding) on a pilot map stand beside the local pilot, 1.2 m to the side and
   0.5 m back in the helicopter's yaw frame (`+0xF4.y` = the pilot's resting view yaw), side alternating by player id
   (`[Puppets] PilotSide` / `PilotBack`). Already true before: riding puppets are never attack targets (host log has no
   `attack target add`), collision/gravity off, `PickVictim` returns nobody for an attached local player.

## Evidence (logs and screenshots under `re/notes/coop-local/`, gitignored)
- Baseline (main, `pilot-baseline/`): client histogram `vt GAMEDLL7.DLL+0x109d48 x1`, no `local hunter`, no sync;
  client keeps sending `request kind=6 'cScrTask01'`, host answers "fired by player 1 (ran here: true)" to an unsynced
  peer. G1 confirmed.
- After step 1 (`pilot-drift1/`, `pilot-drift2/`, HeliSync=0): `tracker: local pilot entity ... id=65 parent=35`,
  `events: synced with host on 'MAP11A'`, `story: applied host fire 'cScrTask01'`, `coop: teleport to host (join
  catch-up) skipped: the local player is a pilot`. **Drift without sync**: client clip time a constant ~1.6-1.7 s behind
  the host (its replayed `cScrTask01`), copies 5-66 m apart (`heli: drift id=35 66.49 m ... 46.40 m`). No stall: the
  client's late `CompleteTask` still lands before its Loop ends.
- Pilot camera: eye 1.68 m above the pilot `+0xE8` (`tracker: hunter pos=(26.98,59.84,...) view ... pos=(27.14,61.52,...)`),
  so `PlayerInCutscene()` (4 m) is fine; it reads > 4 m only in real cutscenes (cutscene 3: 37 m).
- With sync (`pilot-11a-sync/`, MAP11A start to exit, 2 windows): drift 0.00-0.51 m the whole flight, clip time error
  within +-0.07 s, 2 time fixes, state switches on host edges (`Default State->Ani1`, `Loop1->Ani2`, `Loop2->Ani3a`,
  `Ani3b->Ani4_frozen`, `Ani4_frozen->Ani4`). Cutscene 2 started on both from their own synced touch. Waves t21/t3/t4
  killed on the host with `coop_killshot` (credited); task ends 06/07/08 fired; both windows reached the exit save menu
  ("PLEASE SELECT A SAVE LOCATION", `pilot-afterexit-101147-*.png`). No faults in either log.
- Side-by-side: `pilot-wave-cScrTask09-100753-game*.png` (same view in both windows, partner off-screen, markers
  "Host 0m"/"P2 0m"); `pilot-drift2-100615-game2.png` shows the old seat (host puppet floating in the door, blocking).
- MAP11C (`pilot-11c-sync/`): synced, `local pilot ... id=59 parent=39`, heli id 39 drift 0.03-0.08 m through Ani1,
  wave t2 killed, `cScrTask06` fired. Stopped there (lead request).

## Verdicts
| Map | Verdict |
|---|---|
| MAP11A | VERIFIED (2 local windows): client syncs, applies `cScrTask01`, heli follows the host's to < 0.6 m, boars and siberians mirrored, both reach the exit save menu. The save menu needs real input; the map change to MAP11B after it is UNVERIFIED (flow / c-G7). |
| MAP11C | PARTLY VERIFIED: sync, heli follow, first wave and task-end trigger. Waves t3/t4/t9, cutscenes 2/3 and `CheckMissionStatus` UNVERIFIED. |
| Puppet seat | UNVERIFIED visually (out of view by design; needs a user to turn and look). |
| 4 players | UNVERIFIED. Slots for ids 2-3 are 0.8 m further back. |

## Open items / for other owners
- **life** (G2): wildboar2 dies in t21 and comes back in t3/t4; the host counts the second kill (action 1) but the client
  never sees that life (no second `MoveToPos on mirror id=57`, no second death commit). Visible: one boar fewer per wave
  on clients.
- **life/flow** (PC-1): kill credit is per process; `coop_killshot` on the host was used here. With real client kills
  MAP11C's `CheckMissionStatus` end still needs every kill on the host's mission (research-C 0.1: BLOCKS).
- **flow**: exit save menu and the MAP11B load after it (G7); G6 ammo gate; guide death (G5).
- Host leads touches only by network latency (on localhost ~0). Research-C notes that a client heli touching first
  breaks `cScrTask02` OnHit (MAP11C race) and `cScrTask10` BCS spawns. Not seen in the runs (host touched first every
  time), but see next step 1.

## Exact next steps for whoever resumes
1. In `heli.cpp` `follow()`: render the host 100 ms in the past (as animals/vehicles do): pick the newest sample with
   `tick <= host_now - 100`, edge-switch on that sample's state/clip, extrapolate its clip time to `host_now - 100`.
   Keeps the host strictly first at every heli-touched trigger and matches the 100 ms animal interpolation. Keep 8
   samples instead of 4.
2. Finish MAP11C: `.\run-coop-local.ps1 -Map MAP11C -Port 27660`, then drive the host: at each `[host] trigger FIRE
   name='cScrTask02|03|04|09'` wait 4 s and `coop_killshot` wildboar ids (MAP11C: wb1-4 = 52-55, reserves 56-58; take
   released reserve ids from `scale: release coop_... id=N` after the fire). Check `heli: drift`, cutscenes 2/3 on both,
   and `kMissionEnd` on the client after `cScrTask08`. (Driver used here: a scratch PowerShell loop polling the host log;
   recreate as needed.)
3. A user look at the partner seat (turn left/right in the door) on MAP11A; tune `[Puppets] PilotSide/PilotBack`.
4. 3-4 player run on MAP11A (`-Players 3`).

## Proposed MEMORY facts
- MAP11A/MAP11C player is `cObjPilot`, GameDll vtable `0x109d48`, attached to entity `helicopter` (MAP11A id 35,
  MAP11C id 39); its `+0xE8` follows the helicopter; eye (render camera) 1.68 m above it; its `+0xF4` = helicopter
  `+0xF4` while the mouse rests. Evidence: tracker lines, `pilot-11a-sync/` logs.
- `cObjHelicopter` (GameDll ctor `0x22e70`, vtable `0x100d50`, alloc `0x238`): fly model `+0x210` (no `+0xE4` model),
  masscenter bone `+0x214`, visible flag `+0x234` (MakeVisible `0x24250`). `Update` `0x233f0`: model vt `+0x8c(dt)`,
  then bone pos -> entity vt `+0xa4`, bone angles -> vt `+0xb0`. BSMGoToState body `0x23cc0` thiscall(heli, state):
  each part with a script machine gets vt `+0x60`, vt `+0x7c(state)`. `cScrHelicopter` script object: `+0x28` entity,
  `+0x30` task flag (CompleteTask `0x22dc0`, UncompleteTask `0x22dd0`, IsTaskCompleted `0x22db0`); ScrExport `0x244f0`.
- EngineDll `cModelPart`: `+0x84` clip time (s), `+0x88` speed factor, `+0x90` current clip index (short), `+0xd8` body
  machine; vt `+0x6c` SetAnimElapsedTime; `cModelAnim +0x4` name, `+0xc` length; `cBodyStateMachine +0x48` current
  state (name at state `+4`), `+0x3c` its part. Decompiles of the exports `GetAnimElapsedTime`, `GetCurrentAnim`,
  `SetAnimElapsedTime`, `cModelPart::Update`.
- MAP11A clip lengths: ani1 15.57 s, loop1 10.0 s, ani2 56.67 s, ani3a 16.67 s, ani3b 27.23 s, ani4 4.67 s; MAP11C ani1
  46.6 s.
- Two co-op copies of the helicopter driven only by replayed fires run a constant ~1.6 s apart (5-66 m); streaming the
  clip time keeps them < 0.6 m (MAP11A runs 2026-10-18).
