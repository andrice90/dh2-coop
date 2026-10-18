# Job `life` (wave playthrough, 2026-10-18): kill credit, second lives, scaling interplay

Worktree `D:\scary hunt\dh2coop-life`, branch `journal-life` (rebased onto main `13b5efa`), port 27620, Ghidra slot J.
First session stopped early (user's session limit) with the code built but not run. Second session (2026-10-18, Claude
Sonnet 5.5, "life-test"): rebased, ran section 3 steps 1–4 in game (two windows, `godMode 1` on the host, client
`coop_kill`), fixed what the logs showed. Results in section 4: kill credit, second lives and same-frame reports are
VERIFIED; sections 1–3 below are the original design notes.

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

## 4. Test results (2026-10-18, rebased onto main `13b5efa`, 2 windows, host `godMode 1`, kills = client `coop_kill <id>`)

Reference ids: MAP2A wolf1/2/3 154/155/156, reserves 160–162; MAP2B wolf1–3 171–173, reserves 179–181; MAP3A hyena1–4
95–98; MAP11B wildboar1–3 82–84; MAP4B lion1 111, lioness 112/113; MAP7 tiger1–3 234–236.

| Test | Verdict | Decisive log lines |
|---|---|---|
| Hook installs (step 1) | VERIFIED after a fix | First run: `beasts: animal OnHit at GameDll7+0x1a680 has unexpected bytes; not used` (animals.cpp hooks 0x1a680 first at priority 46, beasts' prologue check then failed: its dart hook was silently off). Fix: beasts' check accepts an `e9` prologue for the two functions it hooks; safetyhook relocates the jmp. Now `beasts: animal OnHit ... already hooked by another module; chaining`, both hooks `installed`, no `not hooked` / `FreezeBrain refused` on either side. |
| Kill credit MAP2A task 5 (`coop_tasks` on both, `cScrTask07`) | VERIFIED | Client `coop_kill 154`: host `animals: mission report action=1 species=0`, `AnimalHit id=154 shooter=1 kind=damage`, `death commit id=154 life=1 report=2`; client `mission report action=1 species=0 (host death commit)`, `client copy's death kept out of the mission`. After 154, 155: both `task id=5 shoot state=2 left=1`. After 156: host `trigger FIRE id=122 name='cScrTask07'`, `story: trigger 'cScrTask07' (id 122) fired by player 1`; both `task id=5 ... state=6 (ended) left=0`. |
| Same-frame reports | VERIFIED | One cmd file with `coop_kill 154/155/156` (client): host `mission report action=1` then `queued (one is waiting; 1 queued)`, `(2 queued)`, then `mission report ... (queued) ok=true` twice; task 5 ended `left=0` on both, `cScrTask07` fired. |
| Mixed kills, MAP4B tasks 3/4 | VERIFIED | Host `coop_killshot 112` + client `coop_kill 113`: `task id=3 ... state=6 left=0` both sides, host `trigger FIRE id=109 name='cScrTask19'`, client `applied host fire 'cScrTask19'`; client `coop_kill 111` then `trigger FIRE id=110 name='cScrTask20'`, `CompleteTask 1..4`, `events: host mission message code=5 text='MISSION SUCCESSFUL!'`. (This closes playthrough-b B-4.) |
| Kill credit, other species | VERIFIED | MAP11B boars (species 10): three client kills end task 2, `state=6 left=0` on both. |
| Second lives, MAP2A (script MoveToPos) | VERIFIED | `coop_trigger cScrTask10` on a dead wolf3: host `revive commit id=156 life=2 (MoveToPos)`; client `revive id=156 life 1 -> 2 (host revive) was_dead=true respawn=true frozen=true`; `sync ... drift=0.000`; second `coop_kill 156`: host `death commit id=156 life=2`, client `death event id=156 life=2`, `present death id=156 ok=true`. |
| Second lives, MAP2B siege | VERIFIED | Siege `coop_trigger cScrTask08`, kill 171–173: host `revive commit id=171/172/173 life=2`, later `flee commit id=171 life=2 report=2 (reached a flee point)` and `revive commit id=171 life=3`; client `revive id=171 life 1 -> 2`, `flee commit id=171 life=2 (the snapshot parks it)`, `revive id=171 life 2 -> 3`; `coop_lives` equal on both. The client's own siege timer revived copies first: `mirror id=172 came back here but not on the host (life 1): parked again` / `dead again at the host's corpse`, then the host's revive commit applied. |
| Second lives, MAP3A hyenas (audit-a G4) | VERIFIED | `coop_trigger cScrTask04`, client `coop_kill 95`, 18 s later `coop_trigger cScrTask09`: host `revive commit id=95 life=2 (MoveToPos)`; client `revive id=95 life 1 -> 2 ... respawn=true frozen=true`, `mirror first place id=95`, `sync ... drift=0.000`. |
| Second lives, MAP11B boars (audit-c G2) | VERIFIED | Kill 82–84, `coop_trigger cScrTask06` x2: host `revive commit id=82/83/84 life=2`; client `revive id=82..84 life 1 -> 2`, `drift=0.000`; second `coop_kill 82`: `death commit id=82 life=2`, client presents it. |
| `[Scale] ReuseKilled=1` | VERIFIED on MAP2B, default now 1 | Three reserve kills (`reserve death kept out of the mission`), then the 4th release is `animals: revive commit id=179 life=2 (MoveToPos)` + `scale: release coop_GWOLF_0 id=179`; client `revive id=179 life 1 -> 2`, drift 0. Without it the siege ran out (`no GWOLF reserve left`) after the third reserve death. One-line revert: `[Scale] ReuseKilled=0`. |
| `kScaleConfig` (21) | VERIFIED | Client `events: apply commit kind=21 ... key='bodies=1 reserves=3'`, `scale: host [Scale] bodies=true reserves=3 (this ini bodies=true reserves=3)`. |
| Tranq dart from a client with both OnHit hooks (MAP7) | VERIFIED | Client `coop_tranq 235`: `beasts: client dart id=235 bone=0 ... -> host`; host `beasts: host ran player 1's dart on id=235: tranq 0->2 sleeps in 2.0 s protected=1`, `death commit id=235 life=1`; client `present death id=235`. No `AnimalHit` for the dart (animals.cpp no longer reports darts: the host would run the tranq branch twice, and a second dart into a protected animal is an overdose). |
| Client corpse fades away before the host's (lead's MAP7 note) | FIXED, VERIFIED by position | Tiger 234: host death 14:42:46.9; client's own fade parked it ~14:42:55, host's fade at ~14:43:10 (+24 s). New `KeepCorpse` branch: `mirror id=234 corpse faded away here but not on the host: put back at (-97.7,2.1,-143.5)` (after 3 s), `sync role=client ... pos=(-97.75,2.06,-143.53) drift=0.474`, then `corpse parked like the host's` when the host parked. MAP2A wolves (run after the fix): no put-back line, `mirror id=156 corpse parked like the host's`. Not seen on screen (the client was still in the opening cutscene). |
| Client BULLET on a protected animal (host `kind=bullet` -> `0x965e0`) | UNVERIFIED | `coop_kill` reports kind `damage`, so this path needs a real shot (user test, MAP6/MAP7 tiger). Client dart overdose (two darts within the 2.0 s sleep) also needs real input. |
| Cobra hit from a client | UNVERIFIED | animals.cpp's cobra hook was dropped (beasts.cpp owns `0x80440`); not run here. |

Merge notes. `coop_tasks` lives in story.cpp (animals.cpp's copy removed). animals.cpp has no hook on `0x80440` any more
and ignores darts in its `0x1a680` hook (both are beasts.cpp's); beasts.cpp got a 5-line edit (prologue check accepts an
already-hooked function). `mod/src/coop/beasts.h` `MirrorStateOk` / `NpcLegClipFor` are not called from main's
animals.cpp (beasts' branch had two call sites in `ApplyMirror`; the lead merged only the hooks): clip coverage is not
wired in main. Log noise seen once per siege wolf: `mission report action=1 species=-1` on both sides (a script
`UpdateMissionStatus`; no task counts species -1).

## Proposed MEMORY facts (decompile-verified, slot J)

- Two modules hooking one function: safetyhook chains an already-hooked (`e9`) prologue, but a "bytes are `55 8b ec 6a ff`"
  check fails for the second one. Allow `e9` when the first hook is this mod's. (animals 46 / beasts 47, `0x1a680`.)
- `cScrAnimal::Update` (`0x1f3c0`) corpse fade: health 0 and `+0x404 == 0` -> countdowns `+0x6f4`, then `+0x450`, then vt
  `+0x234` (park at -10000). Each process runs its own: a client copy can fade away ~15 s before the host's (MAP7 tiger).
  `KeepCorpse` puts it back.

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
