# Job `life` (wave playthrough, 2026-10-18): kill credit, second lives, scaling interplay

Worktree `D:\scary hunt\dh2coop-life`, branch `journal-life` (rebased onto main `79d0545`), port 27620, Ghidra slot J.
Stopped early (user's session limit). **Everything below is built (compiles, `mod\build.ps1` OK) but NOT run in game.**
Committed as one `WIP:` commit. No game was started by this job.

## 1. Kill credit: the real cause (VERIFIED by decompile, fix UNVERIFIED in game)

- `cScrAnimal::OnHit` `0x1a680` writes `+0x420 = 1` (MISSION_ACTION_KILL) for every player bullet, dart and knife hit
  (two writes, `0x1a7c3`, `0x1a874`). Nothing else writes `+0x420` except the ctor (`0x18c80`, 0) and present-death.
- Present-death `0x1e8b0`: returns at once if `+0x600 != 0` or `+0xC1 & 4`; else, unless `+0x500 == 2` (event animal),
  `0x91e80(mission, +0x420, +0x434)` and `+0x420 = 0`; then Dead state, `+0x600 = 1`, hp 0, `+0x4f0 = 1`, AddDeadAnimal.
- `0x91e80` records ONE report at mission `+0x1dc` (action) / `+0x1e0` (species) (plus a species-limit fail check on
  `+0x1f0/+0x1e8/+0x1e4`). `cMission` Update (`0x94ba0`, vtable slot at `0x1112f4`, called as vt `+0x44`) hands it to
  every active task (`+0x20c` list, task vt `+0x10` = `cTaskShoot::UpdateTaskStatus` `0x999a0`) and resets it.
  **A second report before that Update overwrites the first: one kill lost.** The engine's flee path itself does
  report + `vt+0x44(0)` at once.
- `0x999a0`: action 1 + species → count `+0xf4`--, pool `+0x58`--; action 2 → task state 5 (fail); action 5 → pool--.
  So the host's handling of a client hit (`FuncInflictDamage` + `PresentDeath`, `+0x420` still 0) reported action 0:
  ignored. Same root cause for `coop_kill` on the host (use `coop_killshot`).
- KILL_PROTECTED (2): no animal path writes `+0x420 = 2`; no script uses it. Protected animals (BTIGER, ELEPHANT:
  `IsProtected()` → `+0x604 = 1`) fail inside OnHit: non-tranq, non-melee bullet with a bone hit → `0x965e0(mission,
  "...killed a protected animal!")`; second dart while `+0x4f0 == 2` → overdose fail. On a client this runs on the
  client's copy only (status 3, message swallowed): the host never failed.
- Tranq: dart sets `+0x4f0 = 2`, damage `+0x4f8` (0 on most species); `cScrAnimal::Update` `0x1f3c0` counts `+0x4f4`
  down and calls vt `+0x154` → the death reports `+0x420` (1) → a tranq "kill" (MAP6/MAP7 BTIGER tasks are ANIMAL_KILL).
  A client's dart did no damage → no AnimalHit → never reached the host.
- Flee: GameDll `0x14940` ("Animal %s reached flee_point") sets `+0x590 = 1`; next Update parks, `+0x600 = 1`, reports
  mission `+0x1ec` (SetMissionOnFleePointReachedAction: KILL on 2A/2B/2C/4B/7/12A/12B, LOSE_TARGET on MAP1) + Update(0).
- Cobra: vt `+0x154` `0x80320` always reports action 1; cobra OnHit `0x80440` only sets Death + `+0x738 = 50`
  (no health change), so a client's real shot never reached the host (MAP7 "kill 1 cobra").

## 2. What the WIP code does

Host (`animals.cpp` `OnAnimalHit`): writes `+0x420 = 1` like OnHit; kind from `AnimalHit.hitPos[0]` (unused on the
wire until now; **lead: rename the field at the next protocol bump**): 0 unknown damage, 1 bullet, 2 dart, 3 melee.
Bullet on a protected animal → `0x965e0` fail (then `kMissionEnd` as usual). Dart → `+0x4f0 = 2` (or overdose fail),
damage = host `+0x4f8`, damage 0 accepted.

Client: hook `0x1a680` on mirrors classifies the hit (tranq/melee/bullet, NPC shooter via `+0x4d8` bit 8) and puts
`+0x4f0` back (host owns the tranq timer); hook `0x80440` reports cobra hits. `ReportLocalHits` accumulates damage held
back by the 150 ms limit, drops damage no player shot explains while the camera is on a cutscene rail or from an NPC
bullet (review M8), sends darts with damage 0.

Mission reports (both roles): hooks `0x91e80` and `0x94ba0` (moved here from `scale.cpp`). A report while one is
waiting, or inside the mission Update, is queued and handed over right after the next Update with report + Update(0)
(the engine's flee-path pattern). Reserve deaths are still kept out (`scale::IsReserve`).

Lives: host/solo `lives_[id]` (life 1..n). `ScanLives` (every frame, all animals, fixes L8) commits each end once:
`kAnimalDied` i0 = `life | report<<16 | fled<<24`, f0 = species, key `"life <n>"` (journal dedup no longer swallows
the next death; `events.cpp` untouched except the one-line handler now passing the event). `hk_MoveToPos` on the host
commits a pending end, then `kAnimalRevive` (20) with the new life. Client: death commit of an older life ignored;
the host's report is given to the client mission once per (id, life) (`reported_`, survives a rejoin: M6), so client
HUD / WaitTask gates count the same kills; client present-death of a mirror is swallowed unless it is the commit's
(`PresentDeathOnce`), and never reports. Revive commit: mirror reset, local respawn (vt `+0x158(3,0)`), FreezeBrain,
snapshot "dead" ignored for 500 ms. `KeepCorpse`: a client corpse is parked when the host's is; a local script
MoveToPos the host did not make (per-process siege timers, MAP2B `SpawnWolves`) is undone (parked / dead again).

Scale interplay (`scale.cpp`): reserve health = species max, or the template's scripted `SetHealth` when lower
(MAP4A lions 100 vs 170); raised script health (MAP2A wolf3 600, MAP3C lions 500) is not copied — extras are ordinary
pack members, and bosses already get health scaling. `AfterRespawn` clears the remembered SetHealth (MoveToPos resets
health). Reserves that reach a flee point are parked without a mission report (before: a fleeing reserve counted as a
kill on flee=KILL maps). `[Scale] Bodies/Reserves`: host sends `kScaleConfig` (21, not journaled) to each client once
per connection (also at the main menu); a client uses them for every later map, and logs an error if the current map's
pool was built differently. ReuseKilled stays 0 (code and ini) until the revive path is verified.

Review M1/M2/L1 (lead's request) ported from `npc-arms` `d5fe70e`: NPC deaths not presented the animal way, dead NPC
stays at 0 and keeps being placed; `Reset` thaws NPCs with CharacterBrainFreeze and gives cobras their bite target
back; NPC profile health only for cObjNPC subclasses (vt `+0x154` == `0x5fde0`, covers girl `0x108d38` too);
`entity::FreezeBrain` refuses brains whose vtable is outside CommonDll7 (small edit in `entity_api.cpp`, not my file).

Not done: item 4 (MAP4A lioness fallback state), all in-game tests.

## 3. Next steps (exact)

1. `.\mod\build.ps1`; `.\run-coop-local.ps1 -Map MAP2A -Port 27620`. Grep both logs for `not hooked` (prologue checks
   of 0x1a680, 0x80440, 0x1e8b0, 0x91e80, 0x94ba0, 0x965e0), `FreezeBrain refused` (must not appear for animals).
2. Kill credit, MAP2A task 5 (scale.md recipe): wolves attacking, client `coop_kill <wolf>` ×3. Expect host
   `AnimalHit … kind=damage`, `animals: mission report action=1`, `death commit … report=2`, client `mission report
   action=1 … (host death commit)`, `coop_tasks` equal on both, `trigger FIRE … 'cScrTask07'` on the host.
3. Lives, MAP2B siege (`coop_trigger cScrTask08`) / MAP3A (audit-a G4) / MAP9 (audit-b G2) / MAP11B (audit-c G2):
   kill, wait for the respawn; expect host `revive commit id=N life=2`, client `revive id=N life 1 -> 2 … respawn=true
   frozen=true`, then `sync` drift small, and a second `death commit … life=2` that the client presents.
   `coop_lives` dumps both sides.
4. Same-frame reports: two `coop_kill` in one cmd burst; expect `queued` then `(queued)` reports and both counted.
5. Tranq/protected need real input (MAP6/MAP7): user test from the client window.
6. If all pass: ReuseKilled=1 in code and `mod/dh2coop.ini`; then item 4 (`Mirror.state_fails` fallback state).

Risk notes: `hk_PresentDeath` swallowing client deaths changes the client death path (a lethal client shot now waits
for the host commit, ~50–150 ms). Explicit client reports run `cMission` Update(0) at Present time (same context as
replayed fires). MAP12C/hunt maps untested.

## Proposed MEMORY facts (decompile-verified, slot J)

- `+0x420` (MISSION_ACTION the death reports) is written only by `cScrAnimal::OnHit` `0x1a680` (1) and cleared by
  present-death `0x1e8b0` after `0x91e80(mission, +0x420, +0x434)`; skipped when `+0x500 == 2` or already dead.
- `cMission` keeps one pending report (`+0x1dc/+0x1e0`); `cMission` Update `0x94ba0` (vt `+0x44`) passes it to the
  active tasks (`+0x20c`) and resets it. Two reports before an Update lose the first. Task list `+0x1f8`: count
  `*(+4)+1`, array `*(+0x10)`; task id `+0x64`, state `+0x68`; cTaskShoot species `+0xf0`, left `+0xf4`, pool `+0x58`.
- Flee point: `0x14940` sets animal `+0x590 = 1`; `cScrAnimal::Update` parks, `+0x600 = 1`, reports mission `+0x1ec`,
  then mission vt `+0x44(0)`.
- Protected (`+0x604`): bullet with bone hit → `0x965e0` "killed a protected animal"; second dart in tranq state 2 →
  overdose fail. Tranq state `+0x4f0 = 2`, timer `+0x4f4`, dart damage `+0x4f8`.
- Cobra death `0x80320` always reports action 1; cobra OnHit `0x80440` never changes health.
