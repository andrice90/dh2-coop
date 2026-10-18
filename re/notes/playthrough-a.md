# Playthrough research A — MAP1, MAP2A, MAP2B, MAP2C, MAP3A, MAP3B, MAP3C (2026-10-18)

Job `research-A`, wave "playthrough" (`re/jobs/_wave-playthrough.md`). Static research only: scripts (`re/extracted/SCRIPTS`), BSP entity lumps, model clip lists, decompiles, and existing logs from other worktrees (read-only). No build, no game run, no git writes. Code read at HEAD `a3322d1` (`mod/src/coop/*.cpp`).

Legend: **VERIFIED** = seen in code, data, decompile, or a log line quoted here. **UNVERIFIED** = inferred. Severity: **BLOCKS** (someone cannot continue: soft lock, crash, stuck camera), **DESYNC** (host and clients disagree but play goes on), **COSMETIC**. Gap ids: `audit-a Gn` = `re/notes/audit-maps-a.md`, `audit-b Gn` = `audit-maps-b.md`, `audit-c Gn` = `audit-maps-c.md`. New items here are `PA-n`.

Entity ids: story BSP entities keep their BSP index as id, and `AddAnimalOnMap` animals follow the last BSP entity (VERIFIED again on MAP1/2A/2B/2C from logs below). Scale reserves (`coop_<SPECIES>_n`) come right after the script animals.

Coordinates in test recipes: x/z are from the BSP entity `position` (cm ÷ 100); trigger brushes report a fixed y (51.3 / 37.8 ...), so y is taken from a nearby waypoint or item and padded +1.5 m. If a `coop_tp` lands under the terrain, add 2 m.

---

## 0. Cross-cutting findings (new or sharper than the audits)

| # | Finding | Evidence | V/U |
|---|---|---|---|
| X1 | **`OnHit` and `BeginCutScene` still run only on the process whose own hunter entered** (audit-b 0.1, audit-a G1). Main has no `kTriggerHit`. On MAP1 this decides three mission tasks and two animal spawns (section 1). | `story.cpp` hooks only `0x68eb0` / `0x68130` (l.723-728); `0x68800` state 4 runs `+0x210` (audit-b 0.1) | VERIFIED code |
| X2 | **`CheckMissionStatus` (`0x94e50`) silently does nothing unless every non-bonus task is state 6.** It sets status 2, walks the task list, and puts status back to 1 at the first task with `+0x5c == 0` and `+0x68 != 6`; no message, no retry. A map that ends with `CheckMissionStatus()` (MAP1, MAP2C, MAP3C) therefore soft-locks on the host if any required task was completed only on a client. | `re/notes/lead-mission2.c` (decompile of `0x94e50`) | VERIFIED |
| X3 | **Kill credit**: present-death `0x1e8b0` reports `0x91e80(mission, +0x420, +0x434)` and then clears `+0x420`; only the bullet `OnHit` `0x1a680` writes `+0x420 = 1`. The host's handling of a client `AnimalHit` (`animals.cpp` `OnAnimalHit` l.804-817: `InflictDamage` + `PresentDeath`) never sets it, so a client's kill reports action 0 and no `cTaskShoot` counts it. The client's own mission *does* count its own kill (its local bullet set `+0x420` on the mirror and the local death ran), so the two missions drift the other way too. `0x1e8b0` returns early when `+0x600` (dead) is set, so no kill is ever counted twice. | `objmodel-decomp-game-animal.c` l.1032-1090; `auditb-animal-onhit-tranq.c` l.65/81; `scale.md` | VERIFIED code, runtime UNVERIFIED |
| X4 | **Regression risk from the MAP4A fix (`e9f1d75`): a client cutscene that waits on an animal task hangs.** `hk_MoveToPos` refreezes a mirrored animal right after any `MoveToPos`, and `ApplyMirror` refreezes a woken brain unless the *host* sample is `CutScene`. A `BeginCutScene` that runs only on a client (X1) does `MoveToPos` + `StartTask(AIHuntTask_PlaySA/PlayAnim/GoToEntity)` + `WaitForTask` on an animal: the frozen brain never runs the task, the client's camera stays on the rail, its trigger never reaches state 2 (so the host never gets the fire request), and every later host fire is deferred (`PlayerInCutscene()`). Escape: the client's own skip key (a client skip stays local). This is exactly the G5→G3 trade-off audit-a warned about. Animal waits on my maps: MAP1 cs4 (mlion `PlaySA`) and cs5 (bear `PlayAnim`), MAP2B cs1 (grizzly `PlaySA`) and cs3 (wolves `GoToEntity`), MAP2C cs4 (grizzly `PlayAnim`). MAP2A cs6, MAP3A cs3/cs4 and MAP3C cs1/cs3 only `Wait(n)` on their animals (no hang). A `PlaySA` wait may self-heal when the host plays the same cutscene at the same time (the host's `kPlaySA` puts the client's copy in `CutScene`, and `ApplyMirror` thaws a copy that is `CutScene` on both sides); `PlayAnim`/`GoToEntity` waits have no such commit. **The planned `kTriggerHit` client replay (`[Story] ReplayOnHit`) would run every `BeginCutScene` on every client and turn this into a hang for all clients on those five cutscenes.** | `animals.cpp` l.1014-1028, l.1329-1342; cutscene scripts; MAP12C yeti hang (`STATUS.md` 22:53) is the same mechanism | VERIFIED code, runtime UNVERIFIED |
| X5 | **NPC `WaitForTask` does complete on a `CharacterBrainFreeze`d NPC** (open question in audit-a). MAP2A client: `cScrTask01` armed 23:40:36.776, `mirror freeze id=75 ok=true npc=true` (Hugh) 23:40:37.166, the client's own `cScrTask01` FIRE 23:41:00.602: cutscene 1 (Hugh `AITaskNPC_PlayAnim cs01` + `WaitForTask`, 23.6 s of `Wait`s) ran to its end with Hugh frozen 0.4 s in. So NPC-only cutscene waits are not a hang risk; animal waits are (X4). | `D:\scary hunt\dh2coop-trig\game2\Bin\dh2coop.log` | VERIFIED (one map) |
| X6 | **The failing-body-state limit covers animals only.** The NPC branch of `ApplyMirror` calls `SetBodyState(state, reset=true)` every frame while the live name differs (l.1130-1136), with no counter. An NPC whose state machine leaves a forced state at once (the girl/nephew legs script switches state from its own movement test, and half its clips are missing on `teengirl`/`nephew`) is re-forced every frame: the MAP4A flood pattern. | `animals.cpp` l.1130-1136 vs l.1152-1172; `GIRLLEGS.SCR`; clip lists below | VERIFIED code, runtime UNVERIFIED |
| X7 | **Clip facts.** `ANIMALLEGS.SCR` states `CutScene`, `IdleWait`, `PostPlayAnim` play `idleattack1`; `Idle`/`IdleNonAttack`/`Graze` pick `idle1..3`; `Walk*` need `walk`/`idletowalk`/`walktoidle`/`runtowalk`/`walktorun`. Model clip lists (`.MODEL` files, `ModelAnimBone` records): GWOLF, HYENA, LEOPARD, MOUNTAINLION have **no `idle1/2/3`** and no walk set; LEOPARD and LIONESSE have no `hurt2`; WHITETAIL has no `hurt`/`synchurt`/`runtoidle`/`idletorun`/turns. **LIONESSE.MODEL does contain `idleattack1`** (and LION too), so the MAP4A "lioness has no idleattack1" failure was more likely a part left on a sync-attack partner's pack (the MAP12C pattern) than a missing clip — worth a correction in MEMORY (UNVERIFIED cause). The 3-try limit keeps any of these to a cosmetic stuck pose on animals. | `SCRIPTS/MODELS/ANIMALS/ANIMALLEGS.SCR`; clip scan of `OBJECTS/ANIMALS/*/*.MODEL` | VERIFIED data |
| X8 | **Mission-only triggers sit in a column far from play** (MAP1 x≈-122: `cScrTask01/05/09/10`; MAP2A x≈-299: `01/07`; MAP2B x≈-267: `01/04/13` (+ scriptless 09-11); MAP2C x≈-25: `01/07/09`; MAP3A x≈-239: `01/08`; MAP3B x≈-262/-288: `01/03`; MAP3C x≈-298: `04/05`). Only a mission's task-end hit (`0x98a50`/`0x96da0` → trigger vt `+0x8c` with that mission's hunter) reaches them. These are the soft-lock points. | BSP lumps (section per map) | VERIFIED data |
| X9 | **Client-mission fallback.** A client's mission counts that client's own kills and finds (X1, X3), and its task-end hit runs `OnHit` on the client, whose fire then becomes a request the host runs (`OnFireRequest` ignores the host's `WaitTask`). So a gate opens if *one* process has the whole count. It fails when the count is split between processes (MAP2B 10 wolves, MAP3A 3 finds / 2 leopards, MAP3C 2 lionesses, MAP1 2 whitetails) and when the map ends with `CheckMissionStatus` on the host's own task list (X2: MAP1, MAP3C). MAP2C survives because its end trigger runs `CompleteTask(1..7)` first. | scripts + `story.cpp` l.348-373 | VERIFIED code |
| X10 | **audit-a G4 is worse than "stays dead".** After a death the client copy is unfrozen (`[Animals] UnfreezeDead` default 1, `PresentDeathOnce` l.1195) and `death_presented` makes `ApplyMirror` skip it for good (l.959-963: health pinned 0, no placement, no wake check). When the replayed `OnActivate` then `MoveToPos`es it, `hk_MoveToPos` does not refreeze (`m.frozen` is false) and the respawn clears the dead bits: the client gets a **local live copy** at the waypoint, running its own `StartTask` (attacks the client's own hunter, or walks the MAP2B siege route), never placed by the host, health written 0 every frame (it may also fall over again at once if its own update reacts to health 0; either way it is not the host's animal). The host's real animal of that id is invisible to that client, and a host retarget can still send it at that client's puppet (damage from an animal the client cannot see). A late joiner gets the same (journal `kAnimalDied` first, revival fire after). The journal also dedupes the second death (`Commit` keys on kind+subject+key, key ""), and `announced_` never re-arms. | `animals.cpp` l.959-963, 1195-1198, 1329-1340, 674; `events.cpp` l.531-537 | VERIFIED code, runtime UNVERIFIED |
| X11 | **A host-run `OnHit` for a client request goes through the host's own `WaitTask` gate** (`0x6b140` checks the mask `+0x200` against the *host's* mission, audit-a F2). If `kTriggerHit` swallows the client's `OnHit` and asks the host, a gate that only the client's mission has passed (client kill, X3/X9) refuses on the host, and the trigger never fires anywhere: today's client fallback is lost. Gates on my maps: MAP1 `cScrTask08` `WaitTask(5)`, MAP2B `03` `WaitTask(2)` and `08` `WaitTask(5)`, MAP3A `14` `WaitTask(8)`. Fix kill credit first, and let a requested host `OnHit` that is refused only by `WaitTask` fall back to the old path (client runs its own `OnHit`, host runs the fire). | `0x6b140` (audit-a F1/F2) | VERIFIED code |
| X12 | **A reliable "local cutscene running" flag exists.** `ActivateHUD` (`0x6b8b0`) sets trigger state 5 and `*(*(GameDll+0x1657f0) + 0x30) |= 8`; `DeactivateHUD` (`0x69970`) clears bit 8, sets state 3 and resets local player `+0x6c8`. Both start with `55 8b ec 6a ff`. `story.cpp` `PlayerInCutscene()` uses the camera-to-feet distance (> 4 m), which a rider at speed can also exceed (`LocalPlayer().pos` lags 2.5-5 m, MAP4A note), so fires could be deferred and animals thawed mid-ride if X4's fix keys on it. | `auditb-trigger-hud-exec.c` (decompiles), prologue bytes via `petool.py dis` | VERIFIED code; that every cutscene path calls `0x6b8b0` is UNVERIFIED |
| X13 | **Hook sites verified** (`petool.py dis`, all `55 8b ec 6a ff` + SEH push): `0x68800` trigger Update, `0x6b140` OnHit, `0x6b8b0` ActivateHUD, `0x69970` DeactivateHUD, `0x68da0` ForceEndCutScene, `0x1e8b0` present death, `0x6c420`/`0x6c610` level exits, `0x49ec0` `SetCanBeAttacked`. | `game/Data/GameDll7.dll` | VERIFIED |

---

## 1. MAP1 (Hugh, bottles, whitetail, mountain lion, black bear)

BSP 104 entities. Ids (VERIFIED, `D:\scary hunt\dh2coop-smoke\re\notes\coop-local\smoke-test\game-dh2coop.log` 09:44): Hugh `hugsm1` 82 (`obj_guide`, oldhugh, shotgun1), triggers `cScrTask01..12` = 92..103 (01 92, 02 93, 03 94, 04 95, 05 96, 06 97, 07 98, 08 99, 09 100, 10 101, 11 102, 12 103), bear 104 (hp 200), mlion 105 (80), wtail1..4 106..109 (10), event mountain lion `animal1` 110. Other: `obj_camera c1cam1` 80, `obj_cschex faketrap1` 81 (trap model, `cScrCustomCharacterEx`), 4 bottles 83-86, 1 stump 87, items: handgun5 (89, `next trigger=cScrTask04`), rifle2 (91, `next trigger=cScrTask12`), ammo, medkit. `EVENT_TRAP` pool (trap1..10) and `EVENT_ANIMAL MOUNTAINLION`. No vehicle, no exit (`trigger_level`), no `trigger_hurt`. Scale: no pool; bear/mlion get lone-large health.

Mission (`MISSION01.SCR`): 1 race; 2 find guide (`AI_GUIDE_ID` = 1, ×1); 3 shoot 4 cans (`TARGET_CAN` 0x25, `NEED_AMMO_FOR_COMPLETE`, end → `cScrTask05`); 4 bonus 2 whitetail (flee = `LOSE_TARGET`, `SetTaskAvailableTarget(2)`, end **and** fail → `cScrTask10`); 5 kill 1 mountain lion; 6 kill 1 black bear (end → `cScrTask09`). End: `cScrTask09` Activate state → `CheckMissionStatus()` (X2: needs 1, 2, 3, 5, 6 done on the host).

### Sequence

| Beat | Trigger / script | What it does | Co-op on main | Gap |
|---|---|---|---|---|
| Map start | `cScrTask01` Spawn: `Activate` + `ActivateAfter(0)`; BeginCutScene = cs1 (Hugh `cs01`, NPC wait) | every process runs it locally; OnActivate: hint page 1, arm 02, `faketrap1.MoveToPos`, Hugh path, `StartTask("1")`, `hugsm1.SetCanBeAttacked(0)` | OK: host fire committed, client applies (log: `host fire 'cScrTask01' (id 92) flag=2 but OnActivate did not run; clearing it` then `applied host fire`). NPC wait OK (X5) | — |
| Walk out | `cScrTask02` (near spawn, x/z -21.2/-1.8) | OnHit `ActivateAfter(0)`; OnActivate arms 03, sound | OK | — |
| Reach the bottles | `cScrTask03` (-59.1/-1.5) | **BeginCutScene: `UpdateMissionStatus(REACHED, 1)` + cs2** (Hugh `cs02`); OnActivate hint 2, Activate state `StartTask("2")`, Hugh moves | **task 1 completes only on the process that walked in** | **PA-1 (X1) BLOCKS at the end** |
| Handgun pickup | item 89 (`use action=1`, manual) arms `cScrTask04` through `0x92210` → `0x68eb0` | arm becomes a request, host arms and commits | OK (arm) | — |
| Find Hugh | `cScrTask04` (-59.1/-6.0; the handgun lies inside it) | **OnHit: `UpdateMissionStatus(FIND_SOMETHING, AI_GUIDE_ID)`**; OnActivate hint 3, Activate state: arm 05, `StartTask("3")`, `rifle2.MoveToPos("rifle")` | **task 2 completes only where the picker stood** (the one who picked up the handgun is the one in the volume) | **PA-1 BLOCKS at the end** |
| Bottles | 4 `obj_target` | host counts every bottle (`props.cpp` `CountBottle`, host-only), clients only break them; task-3 end hits `cScrTask05` on the host's mission | OK. Client missions never finish task 3 (HUD only). `NEED_AMMO`: host dry = everyone fails (audit-c G6) | COSMETIC / audit-c G6 |
| | `cScrTask05` (mission-only, x -121.5) | `ActivateAfter(2)`; OnActivate arms 06, Hugh path | OK (host mission hit → fire → commit) | — |
| Whitetail | `cScrTask06` (-70.3/-44.8) | **OnHit: `player.DisableStimulus()`, `hugsm1.DisableStimulus()`; BeginCutScene: `wtail1/2.MoveToPos("capra_1/2")` + `ForceStartTask(IdleScare)` + cs3** (Hugh `cs03`); OnActivate hint 4, arm 10, stealth levels, `EnableStealth`, Hugh deny tasks, `StartTask("4")` | **If a client walks in first, the host's whitetails stay parked** (no other script moves them) → task 4 can neither end nor fail → `cScrTask10` (mission-only) never fires → `cScrTask07` is never armed: **story stops** | **PA-2 (X1) BLOCKS** |
| Whitetail result | `cScrTask10` (mission-only, x -121.7), task 4 end or fail | OnHit `hugsm1.EnableStimulus()`; OnActivate hint text from `GetTaskStatus(4)` (each process its own), `DisableStealth`, Hugh path, arm 07 | OK once the host's task 4 ends | **PA-3 (X3) BLOCKS** if a client kills a whitetail: action 0 → no count, no flee → task 4 stuck → same stop as PA-2 |
| Mountain lion | `cScrTask07` (-67.7/-92.7) | **BeginCutScene: `mlion.MoveToPos("pumas_t8_01")`, `ForceBrainActivate(0)`, cs4** (mlion `AIHuntTask_PlaySA` partner Hugh, `WaitForTask`); OnActivate hint 5, Activate state `StartTask("5")`, arm 11, `AddAIToBeProtected("hugsm1")`, Hugh path, `mlion.StartTask(AttackCats, target hunter)` | **If a client walks in first: the host's mlion is never moved in** (OnActivate only starts the attack) → task 5 impossible; and the client's cs4 hangs (X4) so it never even sends the fire | **PA-2 BLOCKS**, **PA-4 (X4) BLOCKS (client camera)** |
| | kill the lion | task 5 | **client kill = action 0** (X3) | **PA-3 BLOCKS at the end** |
| | `cScrTask11` (-62.1/-155.4) | arms 08 | OK | — |
| Bear | `cScrTask08` (-4.7/-168.2), `WaitTask(5)` | BeginCutScene: `faketrap1.MoveToPos`, `bear.MoveToPos`, `ForceBrainActivate(0)`, cs5 (Hugh `cs04a` `play_sa faketrap1`, bear `AIHuntTask_PlayAnim cs04b` + `WaitForTask`, bear `MoveToPos("ci_1")`); OnActivate hint 7, `StartTask("6")`, arm 09, `trap1.MoveToPos("foottrap1")`, `bear.MoveToPos` + `AttackBears target hunter` | The host can enter only if *its* task 5 is done (WaitTask is per mission): after a client lion kill the host cannot open it, the client can (its own mission counted the kill). Bear is moved in by OnActivate, so a client-first entry still brings the bear on the host. Client-only cs5 hangs on the bear (X4) | **PA-4 BLOCKS (client camera)** |
| | kill the bear | task 6, end → `cScrTask09` (mission-only, x -121.9) | **client kill = action 0** on the host; the client's mission counts it and hits `cScrTask09`, the host runs it, then `CheckMissionStatus` finds 5/6 open | **PA-3 BLOCKS at the end** |
| End | `cScrTask09` Activate state | Hugh look/anim (`WaitForTask` on the host's real Hugh), `CheckMissionStatus()` | Host success → `kMissionEnd` → clients' results screen. Fails silently on any open task (X2) | **PA-1/PA-3 BLOCK here** |
| Hints | `OpenMenuPage(1..7, 12)` in OnActivate | replayed on every client wherever it stands | COSMETIC (audit-a G14) |
| Hugh | protected from `cScrTask07`; no animal targets him on MAP1 | guide death = code 6 outside `0x91c20` (audit-a G12) | low risk here |

### MAP4A checklist (MAP1)
- **Movers**: none (no vehicle, no heli). `faketrap1` is moved by `MoveToPos` and posed by the host's `PlaySA` (Hugh `cs04a`, partner `faketrap1`, replayed as `kPlaySA`). Falling rock/tree are commented out. OK.
- **Attached**: Hugh's shotgun (child entity, not driven on clients: `npc-arms`). OK.
- **Shared spots**: none (no seats). The foot trap `trap1` is per process and only closes on `obj_player` (audit-b 0.7); the host's trap `SetAnim` is committed (`kPropAnim`), so a client sees the host's trap close while its own copy is still armed. COSMETIC.
- **MoveToPos wakes**: wtail1/2 (06 BeginCutScene), mlion (07 BeginCutScene), bear (08 BeginCutScene ×2 + OnActivate), `animal1` (event spawner, local). All `cScrAnimal` → covered by the `0x14610` hook. **Second lives: none** (no animal is moved after a death on MAP1) → audit-a G4 does not bite here.
- **Clip coverage**: bear (BBEAR: idle1-3, beginchain; no walk set), mlion (no idle1-3, no beginchain/loop/endchain, no walk set), whitetail (no hurt/synchurt/runtoidle/idletorun/turns), Hugh (guide set, 332 clips). With the 3-try limit at most a frozen pose. COSMETIC.
- **Trigger spins**: state-2 spin fix applies (seen on MAP1 smoke: `flag=2 but OnActivate did not run`). OK.
- **Aggro**: lion and bear `target hunter` (host); `RetargetToNearestPlayer` only once they chase a player. `hugsm1.SetCanBeAttacked(0)` is Hugh's own flag. OK.
- **Kill credit**: whitetail (story-gating bonus task), lion, bear — **PA-3**.
- **Scaling**: no reserves; bear and lion get `+25 %` health per extra player when they attack alone (lone large). No kill-count interplay. OK.
- **F5**: menu hop then `mod_spmap MAP1` (no same-map replay). Safe. Restart also resets each client's mission, so a restart after PA-1 is a valid recovery if the host walks first.

### "hunter" and per-process
- `ActivateBy("hunter")` on every trigger; `GetDistBetween2Ais(stealth center, "hunter")` world stealth loop (local HUD); `PlayHint` text from each process's own `GetTaskStatus(4)`; `cScrTask09` Hugh `LookToEntity` faces the host.
- Pickups: handgun5 (gates task 2 by position, PA-1), rifle2 (hint 12), ammo, medkit — each player has their own copy (G17). Bottles shared.

### Verdict MAP1
**Not finishable today unless the host is first into `cScrTask03`, `cScrTask04`, `cScrTask06`, `cScrTask07`, and the host lands the killing shot on a whitetail, the mountain lion and the bear.** Recovery: F6 (next level) or F5 with the host leading.

Note on whitetail task 4 (`cTaskShoot` decompile `re/notes/playthrough-a-task.c`): kills decrement the count `+0xf4` and the available pool `+0x58`, a flee (action 5) only the pool; `CanBeCompleted` (`0x979f0`) ends a `NEED_ANIMAL_FOR_COMPLETE` task only when the pool reaches 0 (state 3 = done if at least one kill counted, 5 = failed otherwise). An action-0 death touches neither, so one client-killed whitetail freezes the task for good. VERIFIED (decompile).

---

## 2. MAP2A (Hugh, wolf ambush, bridge, mountain lion)

BSP 154 entities. Ids (VERIFIED, scale logs `D:\scary hunt\dh2coop-trig\re\notes\coop-local\scale\run12-map2a-game.log` and `dh2coop-trig\game2\Bin\dh2coop.log`): wolf1/2/3 154/155/156, mlion1 157, mlion2 158 (unused), gbear 159 (unused), reserves `coop_GWOLF_0..2` 160-162, Hugh `hugs2a` 75 (`obj_guide`, handgun3 + rifle1), `fallbridge` 74 (`obj_cschex`), `c1cam1` 73, `exit_1` 147 (`map2b,human`), triggers 01 117, 02 118, 03 119, 04 120, 06 121, 07 122, 08 123, 09 124, 10 148, 11 149, 12 150, 13 151, 14 152, 05 153. 22 `trigger_hurt` (river/rocks, 5…10000). Pools: `EVENT_BIRDS`, `EVENT_TRAP` (traps not placed: the `MoveToPos` lines are commented out), `EVENT_ANIMAL GWOLF` (`animal1`). Cutscenes 3/4/5 are in `MAIN.SPJ` but never executed (dead code, includes Hugh's `TakeItem("rifle1_1st")` of cs4).

Mission (`MISSION02A.SCR`): 1-4 race, 5 kill 3 wolves (flee = `KILL`, end → `cScrTask07`), 6 race. **No `CheckMissionStatus`: the map ends at `cScrTask08` → `ActivateTriggerLevel("exit_1")`, and `cScrTask08` is armed at map start** (`cScrTask01` OnActivate). So no mission state gates the exit.

| Beat | Trigger | What it does | Co-op on main | Gap |
|---|---|---|---|---|
| Map start | `01` (mission-only, x -299) Spawn arm+ActivateAfter; BeginCutScene `DeactivateWeaponIn` + cs1 (Hugh `cs01`, 23.6 s) | OnActivate: ammo -5 (local), arm 02/08/12/13, Hugh path, `StartTask 1`, protect Hugh. Its OnHit (escape points off, `SetDenySATime`) never runs anywhere (nobody touches x -299) | OK. NPC wait verified on a client (X5) | — |
| Wolf ambush | `02` (-145.1/123.9, 20 m from spawn) | **BeginCutScene: `wolf1.MoveToPos("wolfs_t2_01")` + cs6** (wolf `intro`, `Wait(3.5)`); OnActivate: REACHED 1, arm 03, `wolf1` attack hunter (no move), `wolf2.MoveToPos` + attack **hugs2a**, Hugh path | Client first → host's wolf1 stays parked (only wolf2 comes). cs6 does not wait on the wolf: no hang | **PA-2 DESYNC** |
| Bridge | `03` (-151.1/104.1) | **BeginCutScene: cs2** (Hugh `cs02` `play_sa fallbridge`); OnActivate: arm 04, `StartTask 2`, both wolves attack hunter, Hugh deny Hello, unprotect Hugh | Host first: `kPlaySA` collapses every client's bridge (VERIFIED 2026-10-16 23:38). **Client first: only that client's bridge collapses; the host's stays whole** (host never runs cs2, client `PlaySA` is not committed). Whether the paths then differ is UNVERIFIED | **PA-2 DESYNC (maybe BLOCK for one side)** |
| Lion | `04` (-42.1/74.9) | OnHit: `ActivateAIHuntAreaPoint(escapemlion1/2)`; OnActivate: REACHED 2, `rand()` spot for mlion1 (host's wins), `AttackCats` (`health 40`, `time 1.0`), arm 05/14, Hugh `MoveToPos c2_3` | Client first → host never opens the lion's flee points → it may fight on instead of running (UNVERIFIED) | PA-2 DESYNC |
| Fork | `05` / `14` | each `Deactivate`s the other, both arm 09 + `StartTask 3` | Both can run on the host (audit-a G6); same effect, harmless | — |
| | `09` (-52.3/12.3) | OnHit REACHED 3 (local); OnActivate arm 06/11, Hugh Hello, `StartTask 4` | HUD only | — |
| Wolves again | `06` (-87.3/42.1) | REACHED 4, arm 07/10, **wolf1/wolf2 `MoveToPos` again** + attack (wolf2 → Hugh), `StartTask 5` | **Wolves shot in the 02/03 fight come back as client-local copies (X10)**; clients cannot see or shoot the host's pair; a retargeted wolf can hurt a client unseen | **audit-a G4 / PA-5 DESYNC (high)** |
| | `10`, `11`, `12` | wolf3 `MoveToPos` (10 only if `CanBeRespawned`), attack Hugh / hunter; 11/12 toggle wolf/lion escape points; 12 `SetHealth(600)`, `time 0.1` (scared wolf) | wolf3 reused 12 → 10/11: same X10 if killed in between | G4 |
| | `13` (-98.0/11.8) | OnHit REACHED 4 (local) | HUD | — |
| Wolf count | task 5 (3 wolves) → `07` (mission-only, x -299) | Hugh path to start2, `StartTask 6` | Client kills = action 0 → `07` may never fire → Hugh never leads to the exit. **Not a block: `08` is armed from the start** | PA-3 DESYNC |
| Exit | `08` (-137.3/-22.4) | REACHED 6, `ActivateTriggerLevel("exit_1")` | Replayed on every process: each loads its own next map, then follows `kMapStart` (audit-a G2, flow) | G2 |
| Hugh | protected 01→03; wolf2/wolf3 target him | death = code 6 written by `cObjNPC::OnDie` on every process whose Hugh reaches 0 (client mirror health → 0 → its own OnDie) | G12 (flow); both fail together in practice |

### MAP4A checklist (MAP2A)
- **Movers**: `fallbridge` (posed by `PlaySA`, see bridge row). No vehicle. Skip state of cs2 also does `fallbridge.SetAnim("broken")` (host commit `kPropAnim`).
- **Attached / shared spots**: none.
- **MoveToPos wakes / second lives**: wolves 1-3 (02 BeginCutScene, 02, 06, 10, 11, 12), mlion1 (04). Wakes covered by the `0x14610` hook. **Second lives are routine here (X10).**
- **Clips**: GWOLF has no `idle1-3`/`beginchain`/walk set; MOUNTAINLION same. Hugh guide set. Bounded by the 3-try limit. COSMETIC.
- **Aggro**: wolf2/wolf3 target `hugs2a`. **Scale clones the template's `SetTaskParam` calls, so reserves cloned from wolf2/wolf3 also go for Hugh** (`scale.cpp` `Release` replays `t.tmpl.params`; round-robin over attackers sorted by id). At `10`/`11` wolf3 attacks alone → every reserve (1 per extra player) targets Hugh. Hugh is protected until 03 and his death fails the map (G12) → more players = more Hugh deaths. **PA-6 DESYNC/balance.**
- **Kill credit**: task 5 only moves Hugh (PA-3, desync).
- **Scaling**: GWOLF pool, 160-162 (VERIFIED). Reserve kills stay out of task 5 (VERIFIED, scale run3).
- **F5**: verified on MAP2A (scale run13: menu hop, reserves recreated with the same ids).
- **Per process**: 22 `trigger_hurt` zones (if a host zone also reaches a puppet, `ForwardPuppetHurt` doubles the damage: audit-a G16, UNVERIFIED whether `trigger_hurt` touches `obj_humanEx`); ambient `animal1` wolf mirrored.

### Verdict MAP2A
Finishable (exit armed from the start). Expected desyncs: invisible/second-life wolves at `06` (high), the bridge if a client reaches it first, Hugh not leading when clients took wolf kills, extra wolves on Hugh with 3-4 players.

---

## 3. MAP2B (Hugh, wolves, grizzly scare, rifle, fire siege, cougar)

BSP 171. Ids (VERIFIED, `run10-map2b-game.log`): wolf1..6 171..176 (only 1-3 scripted), mlion1 177, gbear 178, reserves 179-181, Hugh `hugs2b` 128, `c1cam1` 127, `exit_1` 141 (`map2b,human`), triggers 01 142, 02 143, 03 144, **04 145 (mission-only)**, 05 146, 06 147, 07 148, 08 149, (09/10/11 150-152, no script class), 12 153, **13 154 (mission-only)**, 14 155, 15 156, 16 157, 17 158, 18 159, 19/20/21 160-162 (no activator, siege helpers), 22 163, 23 164, 24 165, 25 166. Items: handgun5 ammo, rifle1 ammo, medkit. 4 `trigger_hurt` (100). Pools: birds, traps (trap1..9 placed by `01`), `EVENT_ANIMAL GWOLF`.

Mission (`MISSION02B.SCR`): 1 race; 2 kill 1 wolf (flee = KILL); 3 kill 1 grizzly (flee = KILL, **force** end → `04`); 4 race; 5 kill 1 cougar (flee = KILL); 6 **kill 10 wolves** (flee = KILL, **force** end → `13`); 7, 8 race. Ends at `18` → `ActivateTriggerLevel("exit_1")`, no `CheckMissionStatus`.

| Beat | Trigger | What it does | Co-op on main | Gap |
|---|---|---|---|---|
| Start | `01` | arm 02, escape points, `rand()` wolf1 or wolf3 to `wolfs_t1_01`, both attack hunter (wolf1 `time 0.1`, `health 20`: a quick flee), place 9 foot traps, Hugh path, protect Hugh, `wolf1revival` state polls health | OK (host positions win over each client's own `rand()`; the client's script state polls the mirror) | — |
| | `02`, `22`, `23`, `24` | more `rand()` wolf placements + attacks; escape-point toggles | OK, G4 for reused wolves | G4 |
| Grizzly scare | `03` (-67.2/-54.7) `WaitTask(2)` | OnHit REACHED 1; **BeginCutScene: wolves 1-3 `MoveToPos` + `Kill()` (park), `gbear.MoveToPos("bears_t5_01")`, cs1** (gbear `AIHuntTask_PlaySA cs03` with Hugh + `WaitForTask` on the bear); OnActivate: wolves stop, Hugh 50000 hp, `gbear` attack hunter + `CustomProps("Make_Scared")`, `StartTask 3`, unprotect Hugh | WaitTask is per mission: the host can enter only after a *host-counted* wolf kill or flee (wolf1 flees fast, usually fine). **Client first → host's grizzly is never moved in** → it cannot flee → task 3 never ends → forced `04` never fires: **story stops**; and the client's cs1 hangs on the frozen bear (X4) | **PA-2 BLOCKS**, **PA-4 BLOCKS (client)** |
| Hugh's rifle | `04` (mission-only, forced by task 3) | BeginCutScene: Hugh weapon swap + **cs2: `player.MoveToPos("c4_2")`, Hugh `cs04`, `player.TakeItem("rifle1_1st")`** (also in Skip); OnActivate Activate state: arm 05/23/24, hint "You received Hugh's Rifle", `StartTask 4`, Hugh 40000 hp, `SetCanBeAttacked(0)` (Hugh), Hugh walk/look/`hurtsit`/`hurtdown` + `SetPhysNPC(0)` | Only the process whose mission ended task 3 (the host: the grizzly's flee is host AI) runs cs2. **Clients never get Hugh's rifle on MAP2B** and see the hint anyway. Hugh's `hurtsit`/`hurtdown` `AITaskNPC_PlayAnim` is not mirrored as a clip (audit-c G9) | **PA-7 DESYNC** (gameplay), G9 cosmetic |
| | `05` | wolves 1-3 `MoveToPos` + attack, `SetHealth(80)` | X10 second lives | G4 |
| Cougar | `07` | Activate state: `StartTask 5`, arm 08, fire FX countdowns (400 s, per process), Hugh attackable again, `rand()` mlion spot, attack hunter (`time 0.1`) | OK (cougar flees fast → host flee counts) | — |
| Siege | `08` (-19.7/-69.7) `WaitTask(5)` | **BeginCutScene: wolf1/2 `MoveToPos` + cs3** (wolves `GoToEntity` + 2× `WaitForTask`, then `Kill()`); OnActivate: arm 25, wolves attack **hugs2b**; Activate state: arm 12/13, hint, REACHED 4, `StartTask 6`, **`player.SetCanBeAttacked(0)`**, then `SpawnWolves` (wolves 1-3 recycled through `CanBeRespawned` → `MoveToPos("wolfs_t8_0n")` → `GoToEntity` a fire waypoint; helpers 19/20/21 then `IdleWait` 5 s and attack Hugh), at most 10 spawns, `NoChange` re-enables `SetCanBeAttacked(1)` | cs3 hangs a client that entered first (X4). **10 wolf lives for a 10-kill task: every client kill (action 0) is a life lost → host count ≤ 9 → forced `13` never fires → story stops** (X3; the client's own count cannot reach 10 alone either, X9). On clients each recycled wolf becomes a local copy after its first death (X10): clients shoot local copies while the host's wolves attack unseen. `SetCanBeAttacked(0)` leaves puppets in the attack list (audit-a G8). Reserves cloned from the siege wolves attack Hugh too (PA-6) and their deaths stay out of the count (VERIFIED) | **PA-3 BLOCKS (likely)**, **PA-5 DESYNC (high)**, PA-4 BLOCKS (client), G8 |
| | `12` (-6.0/-96.0) | wolves to `killer1-3`, `Make_Killer`, Hugh 10000 hp | OK | — |
| After siege | `13` (mission-only, forced by task 6) → `14` | 13: arm 14, disarm 12, `StartTask 7`. 14: Hugh `syncguidegetup` PlayAnim, `RestoreLight` on the fire FX, **mlion1 `MoveToPos` + attack hunter (`time 1000`)** | mlion1 killed at 07 comes back as a client-local copy (X10); `syncguidegetup` not mirrored (G9) | G4, G9 |
| | `15`, `16`, `17` | 16 OnHit REACHED 7 (local); 17: wolf1/wolf3 `MoveToPos` + attack | X10 | G4 |
| Exit | `18` → `exit_1` | per-process exit (G2) | G2 |

### MAP4A checklist (MAP2B)
- **Movers**: none. Fire/smoke/sparkle FX (`cScrFx`) count down 400 s per process: cosmetic drift.
- **Shared spots**: foot traps trap1-9 (per process, close only on `obj_player`; a client's Hugh mirror also closes its local trap: `CloseGuide`/`synctrap` on a mirrored NPC fights the mirror's pose; client NPC damage is blocked). COSMETIC.
- **MoveToPos wakes / second lives**: wolves 1-3 (every trigger + 10 siege lives), mlion1 (07, 25, 14), gbear (03 BeginCutScene). **The heaviest X10 map in my range.**
- **Clips**: GWOLF, MOUNTAINLION (no idle1-3), GBEAR (no walk set; `cs03` in `GBEAR/MAP2B.AP`). Bounded. COSMETIC.
- **Aggro**: siege wolves target Hugh; `SetCanBeAttacked(0)` host only (G8); reserve clones target Hugh (PA-6).
- **Kill credit**: task 6 (PA-3 BLOCK), task 2/5 WaitTask gates (usually closed by host-side flees).
- **Scaling**: GWOLF 179-181; grizzly is a 10000-hp scare (×1.25 with 2 players, VERIFIED) — irrelevant to the flee.
- **F5**: verified on MAP2B (d14e897). Restart resets the siege.
- **Trigger spins**: client `cScrTask19/20/21` `WaitForTask(GoToEntity)` on a frozen copy never ends: that helper script waits forever on clients (no camera, no fire; harmless).

### Verdict MAP2B
Blocks in normal 2-player play at the siege (any client wolf kill), and if a client is first into `03`. Clients lack the rifle.

---

## 4. MAP2C (Hugh, Erin, Danny, wolves, boar claim, cave meat, grizzly)

BSP 98. Ids (VERIFIED, `run4-map2c-game.log`): wolf1 98, wolf2 99, gbear 100, wildboar1 101, reserves 102-104, Danny 77 (`obj_girl`, nephew), Erin 78 (`obj_girl`, teengirl), Hugh 79, triggers 01 88, 02 89, 03 90, 04 91, 05 92, 07 93 (mission-only, no script class), 08 94, 09 95 (mission-only). `cave_meat` 86 (`use action 2` = automatic, bound 5 m, `next trigger=cScrTask08`, sits inside `08`'s volume at 200.4/89.0). 2 `trigger_hurt` (100000, falls). Pools: birds, `EVENT_ANIMAL GWOLF`.

Mission: 1 race, 2 kill 2 wolves, 3 race, 4 kill 1 boar (force end → `07`, no script), 5/6 race, 7 kill 1 grizzly (flee = KILL, end → `09`). **End `09` runs `CompleteTask(1..7)` then `CheckMissionStatus()`**, so only `09` itself has to fire.

| Beat | Trigger | What it does | Co-op on main | Gap |
|---|---|---|---|---|
| Start | `01` | protect Hugh, escape points off, Hugh path, `StartTask 1`, arm 02 | OK | — |
| Wolves | `02` (83.0/147.2) | `cave_meat.Deactivate()`, escape points on, wolf1 attack hunter / wolf2 attack **hugs2c** (`time 3`), `StartTask 2`, arm 03 | OK. Reserves cloned from wolf2 go for protected Hugh (PA-6) | PA-6 |
| Family | `03` (123.9/78.6) | OnHit REACHED 1; BeginCutScene: `CompleteTask(2)`, gbear `MoveToPos`, cs1 (Danny `cs06` `play_sa erin`, Hugh `cs06`, NPC waits, gbear `Kill()`); OnActivate: `StartTask 3`, arm 04, Hugh/Erin/Danny walk to `end1` | Client first: host skips cs1 (no gameplay effect: `09` completes everything). NPC waits OK (X5) | COSMETIC |
| Girls walking | Erin/Danny mirrors | `GIRLLEGS.SCR` states vs 15-clip packs (`teengirl`/`nephew`: no side/back walks, no `walktoidle`/`runtoidle`/`idletowalk`/`idletorun`, no `hurt`, no `turn180left/right`) | NPC branch forces the host's state every frame with no fail limit (X6); `NpcLegClip("Idle")` → `idlenormalneutral` missing (audit-a G13) | **PA-8 DESYNC (crash risk, UNVERIFIED)** |
| Shotgun | `04` (73.0/150.5) | OnHit REACHED 3; **BeginCutScene: cs2 (`player.MoveToPos("c1_13")`, Hugh `cs07`, `player.TakeItem("shotgun1_1st")`)**; OnActivate: unprotect Hugh, `StartTask 4`, hint "You received Hugh's shotgun", arm 05 | Only the process that walked in gets the shotgun | **PA-7 DESYNC** |
| Boar | `05` (86.5/109.5) | OnHit `DisableStimulus` (Hugh, player); OnActivate: stealth levels, `EnableStealth`, boar `MoveToPos` + `IdleScare`, `SetCanBeClaimed`, claim callback → `cScrTask05.ClaimCallback`, state `wildboarrespawn` (re-places the boar whenever `CanBeRespawned`) | Claim: client request → host runs the callback (VERIFIED path). **Client side never runs the callback** (`OnClaimCommit` only clears `+0x404`): its `cScrTask05` stays in `wildboarrespawn`, so its boar copy, once faded and parked, is respawned *locally* (X10 pattern) and keeps reappearing; its `cave_meat` stays off | audit-a G9 (beasts) DESYNC |
| Cave meat | item 86 → arms `08` | `use action 2` inside its box: only a process whose `cave_meat` is active can use it → only the host after the claim | Host must walk to the meat (clients cannot) | G9 |
| Grizzly | `08` (200.4/89.0) | OnHit REACHED 6; BeginCutScene: gbear `MoveToPos` + **cs4 (gbear `AIHuntTask_PlayAnim cs08` + `WaitForTask`)**; OnActivate: gbear `MoveToPos` + attack hunter, `StartTask 7`, arm 09 | OnActivate places the bear, so the host gets it whoever entered. A client inside `08` without the host hangs in cs4 (X4) | **PA-4 BLOCKS (client)** |
| End | task 7 end → `09` (mission-only) | `CompleteTask(1..7)`, `CheckMissionStatus()` | Host kill or flee: host mission hits `09`. Client kill: action 0 on the host, but the client's own mission counts its kill and hits `09` → request → host runs it → success (X9). **OK** | — |

### MAP4A checklist (MAP2C)
- Movers/attached/shared spots: none. MoveToPos: wolves once (no second life), gbear (03 BeginCutScene, cs1 `Kill`, 08), boar respawn loop (host only after its corpse fades unclaimed → G4; client loop G9).
- Clips: girls (X6/PA-8), GWOLF, GBEAR (`cs08` in `GBEAR/MAP2C.AP`), WILDBOAR (has idle1-3; no walk set). Hugh guide set.
- Aggro: wolf2 + its reserve clones → Hugh while protected (PA-6). Stealth/`DisableStimulus` host-only (audit-a G18).
- Kill credit: tolerated (X9) because `09` force-completes.
- Scaling: GWOLF 102-104; gbear lone large 350 → 437.5 (VERIFIED).
- F5: verified on MAP2C (MEMORY "Restarting the loaded level safely").

### Verdict MAP2C
Finishable. Desyncs: shotgun only for the player who walked into `04`, client-side boar respawn after the claim, girls' leg clips (watch for a client script-VM fault while Erin/Danny walk).

---

## 5. MAP3A (Abby, find food/water/shelter, hyenas, crocodiles, leopards, fork exits)

BSP 95. Ids (predicted from the BSP rule, matches audit-a; no MAP3A log exists): hyena1..4 95..98, leopard1 99, leopard2 100, crocodile1..5 101..105, reserves `coop_HYENA_0..2` 106-108 and `coop_LEOPARD_0..2` 109-111 (UNVERIFIED). Abby 66 (`obj_guide`, abby set), Food1 76 (`medikit_food`, `use action 1` = manual pickup, `next trigger=cScrTask02`; the item lies inside `02`'s volume), triggers 01 77, 02 78, 03 79, 04 80, 05 81, 06 82, 07 83, **08 84 (mission-only)**, 09 85, 10 86, 11 87, 12 88, 13 89, 14 90, 15 91, 16 94, `exit_1` 92 (`map3b,human1`), `exit_2` 93 (`map3b,human2`). Pools: snake (ambient `cObjSnake`, not mirrored), birds, bees, cave dust.

Mission: 1 **find ×3** (`AI_GUIDE_ID`, end → `08`), 2/3/4 kill 2 hyenas each, 5 race, 6 kill 3 hyenas, 7 race, **8 kill 2 leopards** (gate: `14` `WaitTask(8)`), 9 race. No `CheckMissionStatus`: the map ends at an exit armed by `14`.

| Beat | Trigger | What it does | Co-op on main | Gap |
|---|---|---|---|---|
| Start | `01` | `StartTask 1`, arm 03-10/15/16, Abby cover/`SetCanBeAttacked(0)`/`GUIDE|COVERSTAY` | OK | — |
| Finds | `02` food (-91.5/150.0, armed by Food1), `04`/`05`/`06` water (-66.1/72.2, -27.6/50.3, -43.4/28.6; each OnActivate deactivates the other two), `07` shelter (-22.0/64.0) | **OnHit: `UpdateMissionStatus(FIND_SOMETHING, AI_GUIDE_ID)`**; OnActivate moves hyena pairs in (`MoveToPos` + attack hunter), 07 also `StartTask 3`, arm 11 | Each find counts only on the process that stood in the volume. One process with all 3 → its mission hits `08` → fire request → host runs it (X9). **Split finds → no mission reaches 3 → `08` never fires → no Abby return, no leopards, no exits: story stops.** | **PA-1 BLOCKS** (audit-a G1, still open) |
| | `03`, `09`, `11`, `15` | hyena waves (`MoveToPos` + attack), `StartTask 2/4/6`; 15 BeginCutScene `hyena4.MoveToPos` + cs3 (`IdleWait`, `Wait(3)`, no hang) | Hyenas 1-4 serve 9 kills: X10 second lives on clients | G4 / PA-5 DESYNC |
| Crocodiles | `10` (-44.0/21.5) | crocodile1/2 `MoveToPos` + `AttackFloaters target hunter` | Not mirrored (`cObjCrocodile` vt `0xffa50`, audit-a G7, beasts): every process runs its own crocs on its own hunter; the host's crocs can also pick a client puppet (forwarded hurt/knock on top of the client's own crocs) | G7 DESYNC |
| Abby back | `08` (mission-only, task 1 end) → `12` (-8.6/176.8) | 08: `StartTask 5`, arm 12. 12: OnHit REACHED 5 (local), Abby `COMPANION|COVERSTAY "hunter"` (follows the host; `npc_follow` off), arm 13, `StartTask 7` | OK once 08 fires | npc_follow (hunter) |
| Leopards | `13` (-90.2/125.1) | **BeginCutScene: `leopard1.MoveToPos("leopards_t13_01")` + cs4** (`IdleWait`, `Wait(3)`); OnActivate: leopard1 attack (no move), leopard2 `MoveToPos` + attack, `StartTask 8`, arm 14 | **Client first → host's leopard1 stays parked → only one leopard exists → task 8 can never complete anywhere → `14` never opens → no exits: story stops.** | **PA-2 BLOCKS** |
| | task 8 → `14` (-22.4/64.0) `WaitTask(8)` | BeginCutScene: REACHED 7 + cs2 (Abby `cs02`, NPC wait); OnActivate: `StartTask 9`, `LatentActivateTriggerLevel(exit_1)`, `(exit_2)` | **Leopard kills split host/client → neither mission passes `WaitTask(8)` → `14` never opens: story stops.** One process with both kills opens it (X9). | **PA-3 BLOCKS (likely)** |
| Exits | `exit_1` (-40.2/-22.8) → `map3b,human1`; `exit_2` (-130.0/-11.3) → `map3b,human2` | latent: whoever walks in leaves (local player) | A client walking in loads MAP3B alone; the host follows later; `kMapStart` "arrived alone" keeps the client (audit-a G2) | G2 DESYNC |
| Hints | `16` `OpenMenuPage(8)` | replayed everywhere | G14 cosmetic |

### MAP4A checklist (MAP3A)
- Movers/attached/shared spots: none. Falling rocks (`obj_cavedust`), bees (`obj_bees`), snake events: per-process hazards with local damage. COSMETIC.
- MoveToPos / second lives: hyenas (9 kills from 4 bodies), leopard1 (13), leopard2 (13), crocodiles (unmirrored). X10 heavy.
- Clips: HYENA, LEOPARD (no idle1-3, LEOPARD no `hurt2`), CROCODILE_W (9 clips: `intro idle hurt attack1 run rundisabled pl_cr_syncattackcroc cr_syncattackcroc defaultpose` — irrelevant until G7 mirrors it; a mirror would then need a state whitelist like the cobra's). Abby guide set.
- Aggro: everything targets `hunter`; Abby companion to the host only.
- Kill credit: task 8 gate (PA-3). Hyena tasks gate nothing.
- Scaling: HYENA + LEOPARD pools (UNVERIFIED ids). Reserves do not count for task 8 (by design), so extra leopards do not help the gate.
- F5: menu-hop restart; not run on MAP3A (UNVERIFIED but same path as MAP2C).

### Verdict MAP3A
Blocks unless one process makes all three finds and one process (any) kills both leopards, and the host is first into `13`.

---

## 6. MAP3B (radio tower, cobras, leopards) — played co-op 2026-10-16 (`re/notes/story-s1-map3b.md`)

BSP 47. Ids (VERIFIED, `D:\scary hunt\dh2coop-polish\game\Bin\dh2coop.log` and main `game2`): leopard1 47, leopard2 48, cobra1..3 49..51, reserves `coop_LEOPARD_0..2` 52-54, triggers 01 39, 03 40 (mission-only), 04 41, 05 42, 02 44, 07 45, 06 46, `exit_1` 43 (`map3c,human`). Hunter id moved from 60 (2026-10-16) to 63 (2026-10-18 09:51 log) because of the reserves. Items: ammo, 2 medkits, Food1 37 (`next trigger=cScrTask02`). No NPC.

Mission: 1 race (end → `03`), 2 kill 1 leopard (no end trigger). Every story effect is in `OnActivate`; no BeginCutScene except the map-start one (`cScrCutScene1` is referenced but not in `MAIN.SPJ`: no-op). Exit `05` → `ActivateTriggerLevel("exit_1")`.

| Beat | Co-op on main | Gap |
|---|---|---|
| `01` start: cobras 1-3 `MoveToPos` + `Task_CobraAttack target hunter` | Cobras mirrored (`cObjCobra`, kill/death fixed since the 2026-10-16 run); bites only the host's hunter unless `[Animals] CobraBitesNearest=1` (default 0) | DESYNC (clients are never bitten) |
| Food1 → `02` (-58.6/-83.5): REACHED 1 in OnActivate, leopards in | OK (task 1 replays on every mission; each mission hits `03`, the host's runs) | — |
| `03` (mission-only) → `04` (36.9/-87.7) → `05` (27.9/-49.6) | leopards reused 02 → 03 → 04 (`MoveToPos`): a leopard shot before `04` becomes a client-local copy (X10); task 2 gates nothing | G4 DESYNC |
| `06`/`07` hint pages 9/10 | G14 | cosmetic |
| Exit `05` | per-process exit (G2). 2026-10-16 the host's exit loaded MAP2A (BSP says `map3c,human`): the next map comes from the profile, so a run started with `-spmap MAP3B` goes where that profile thinks it is | G2 |

Old MAP3B findings re-checked: snakes per process → cobras now mirrored, ambient `cObjSnake` still local (by design); same-map replay crash → F5 now goes through the menu (safe); repeated client fire requests → `ShouldRequest` 3 s limit + state-2 fix; the 21:37:58 "Mission failed / Your hunt has ended!" (caller `0x96636` in `0x965e0`) fits a hunter death (the cobras bite the host) — not a script fail.

MAP4A checklist: no movers, no seats; leopards (no idle1-3, no `hurt2`), cobras (whitelisted states); kill credit irrelevant (no gate); scaling LEOPARD 52-54 (VERIFIED ids); F5 safe. **Verdict: finishable**; desyncs = cobras bite only the host, second-life leopards.

---

## 7. MAP3C (Abby defense, lionesses, lion, radio, helicopter)

BSP 49. Ids (BSP rule; UNVERIFIED, no MAP3C log): lion1 49, lion2 50 (both `ANIMAL_LIONESSE`), lion3 51 (`ANIMAL_LION`), cobra1..4 52..55 (no task), reserves `coop_LIONESSE_0..2` 56-58; Abby `abby3c` 33, `heli1` 34 (`obj_helicopter`, `cScrHelicopter`), radio1 42 (`use action 2`, `next trigger=cScrTask03`), triggers 01 43, 02 44, 03 45, **04 46, 05 47 (both mission-only)**, `exit_1` 48 (`map3b,m3ba1`, never activated).

Mission: 1 race (REACHED in `03` OnActivate), 2 kill 2 lionesses (end → `04`), 3 kill 1 lion (end → `05`). **End `05`: Activate state `Wait(2)` + `CheckMissionStatus()` — no `CompleteTask` fallback (X2).**

| Beat | Trigger | What it does | Co-op on main | Gap |
|---|---|---|---|---|
| Start | `01` | `StartTask 1`, arm 02 | OK | — |
| Defend Abby | `02` (-120.7/23.9) | **`player.SetCanBeAttacked(0)`**, `SetMaxAttackerNo(2)`, Abby 30000 hp, lionesses `MoveToPos` + attack **abby3c** (`SetHealth(500)`, `maxdmgval 20000`) | Host only leaves the attack list (G8): lions can still pick client puppets, and a client's hit turns the lion on that client (`OnClientHitAnimal`). Reserves cloned from lion1/2 attack Abby (PA-6, harmless at 30000 hp) | G8 DESYNC |
| Radio | radio1 → arms `03` (2.9/-1.1) | OnActivate: REACHED 1, arm 04, disarm 02, lionesses attack Abby again (no move), `StartTask 2`. BeginCutScene: lionesses `MoveToPos("lions_t3_0n")` + cs1 (Abby `cs03`, lions `IdleWait`/`GoToEntity`, waits on Abby only) | Client first: the host's lionesses stay at their `02` spots (they are already up): cosmetic | COSMETIC |
| Lionesses | task 2 → `04` (mission-only) | BeginCutScene: cs3 (**`lion3.MoveToPos("lions_t4_03")`**, `IdleWait`, `Wait(3)`); OnActivate: arm 05, `StartTask 3`, lion3 attack hunter (no move), `SetHealth(200)` | **Split lioness kills → no mission reaches 2 → `04` never fires: story stops.** If one *client* has both kills, its mission hits `04`, so **only that client runs cs3 and moves lion3; the host's lion3 stays parked → task 3 impossible**. And any client kill leaves the host's task 2 open for the final check | **PA-3 BLOCKS**, **PA-2 BLOCKS** |
| Lion | task 3 → `05` (mission-only) | BeginCutScene: cs2 (Abby `cs04`, `heli1.SetAnim("m3_rescue")`, Abby `SetAIType(GUIDE,"a")`, `passenger3c` does not exist); OnActivate: `Wait(2)`, `CheckMissionStatus()` | A client lion kill: its mission hits `05`, the host runs `CheckMissionStatus` with task 3 (and maybe 2) open → nothing (X2). The heli anim is committed only when the host runs cs2 | **PA-3 BLOCKS at the end** |

### MAP4A checklist (MAP3C)
- **Movers**: `heli1` is posed by one `SetAnim` in the host's cs2 (`kPropAnim` to clients). No flight on MAP3C. Whether `vehicles.cpp` (hooks `cObjVehicle::Update`) also streams it depends on `cObjHelicopter` reaching that export (UNVERIFIED); harmless either way because it does not move by physics here.
- Attached/shared: none. Pickups: radio (gates `03` by position; the arm is host-decided, the BeginCutScene part is local — cosmetic here).
- MoveToPos: lionesses (02, 03 BeginCutScene), lion3 (04 BeginCutScene only!). Second lives: none expected (lionesses die once).
- Clips: LIONESSE has `idleattack1` (see X7), no `hurt2`; LION full set. Cobras whitelisted. Abby guide set.
- Aggro: lions → Abby; `SetCanBeAttacked(0)` host-only (G8); `SetMaxAttackerNo(2)` may leave released reserves idle (UNVERIFIED).
- **Kill credit**: lionesses and lion (PA-3 BLOCKS).
- Scaling: LIONESSE pool (lion family). lion3 alone after the lionesses → lone large health.
- F5: menu-hop restart (UNVERIFIED on this map).

### Verdict MAP3C
Not finishable today unless the host lands every lion kill. The only other route is F6.

---

## 8. Ranked list (2-4 player playthrough MAP1 → MAP3C)

| # | Item | Sev | Maps | Owner | Fix (addresses VERIFIED unless noted) | Test recipe |
|---|---|---|---|---|---|---|
| PA-3 | **Kill credit**: client kill reports action 0 (X3) | BLOCKS | MAP1 (whitetail, lion, bear), MAP2B siege, MAP3A leopards, MAP3C lions; desync MAP2A; tolerated MAP2C/3B | life | `animals.cpp` `OnAnimalHit`: before `InflictDamage`, `*(int*)(ent+0x420) = 1` (MISSION_ACTION_KILL) for every non-NPC, non-cobra hit; present-death `0x1e8b0` reads and clears it. Keep it out of reserves (scale already skips their report). | `-Map MAP3C`: host `coop_trigger cScrTask02` (fires), host `coop_trigger cScrTask03` ×2 (arm, fire → `StartTask 2`); client `coop_killshot 49`, `coop_killshot 50` (ids from `scale: map 'MAP3C' story animals`). Bug: host `scale: mission report action 0 value 28` ×2, no `trigger FIRE id=46`. Fixed: `action 1 value 28`, host `trigger FIRE id=46 name='cScrTask04'`. |
| PA-1 | `OnHit` / `BeginCutScene` local only (X1, audit-a G1) for mission counts | BLOCKS | MAP1 tasks 1/2 (end soft lock), MAP3A finds | triggers | `kTriggerHit` (18) hook on `0x6b140` as audit-a G1, **plus** X11: if the host's `0x6b140` refuses only because of `WaitTask` (`+0x200`), fall back to the old path. Land after PA-3. | `-Map MAP1`: client `coop_tp -21.2:61.5:-1.8` (02), then `coop_tp -59.1:61.5:-1.5` (03). Bug: client log `scale: mission report action 3 value 1`, host none. Fixed: host has it too. Task 2: client `coop_trigger cScrTask04` then `coop_tp -59.1:61.5:-6.0` → `action 4 value 1` on client only today. |
| PA-2 | `BeginCutScene`-only animal placement: the host's animal never comes when a client enters first | BLOCKS | MAP1 07 (lion), 06 (whitetails); MAP2B 03 (grizzly); MAP3A 13 (leopard1); MAP3C 04 (lion3, via the client's mission); desync MAP2A 02/03 | triggers | Same `kTriggerHit` (host runs the requested `OnHit` → its own state machine runs `BeginCutScene`). | `-Map MAP1`: host `coop_trigger cScrTask07` (arm only); client `coop_tp -67.7:40:-92.7`. Bug: host `animals: id=105` stays `pos=(-10000…)` after the client's fire request; fixed: host `id=105` at `pumas_t8_01` (-62.8,38.7,-100.6). |
| PA-4 | Client cutscene hangs on an animal `WaitForTask` (X4, regression risk of `e9f1d75`) | BLOCKS (client camera; skip key escapes) | MAP1 cs4/cs5, MAP2B cs1/cs3, MAP2C cs4 — every client once `kTriggerHit` replays `OnHit` | life (+ triggers flag) | Track a local cutscene with `ActivateHUD 0x6b8b0` / `DeactivateHUD 0x69970` (or read `*(*(GameDll+0x1657f0)+0x30) & 8`, X12). While set: `hk_MoveToPos` and the `ApplyMirror` wake check do not refreeze, and pose writes skip animals the local cutscene drives; refreeze when it clears. Also use the flag in `PlayerInCutscene()` instead of the 4 m camera test (riders). | `-Map MAP1` as PA-2 (host stays at spawn). Bug: client `animals: MoveToPos on mirror id=105: brain frozen again at once`, no client `trigger FIRE id=98` within 30 s, later `story: defer host fire`. Fixed: client FIRE id=98 ~8 s after entry. |
| PA-5 | Second lives become client-local copies (X10, audit-a G4) | DESYNC (high) | MAP2A 06, MAP2B (siege, 14, 17), MAP3A hyenas, MAP3B leopards | life | `kAnimalRevive` (20): host `HostTick` sees an `announced_` id alive and not parked → erase, life++, commit with key `life N` (death commits keyed the same so the journal keeps both). Client: erase `dead_`, reset the `Mirror` (death_presented/frozen/have_first/state_fails), `FreezeBrain(true)`. And `hk_MoveToPos` should refreeze any non-NPC mirror (not only `m.frozen`). | `-Map MAP2A`: host `coop_tp -145.1:42:123.9`, after the wolves come `coop_killshot 154`, `coop_killshot 155`, wait 20 s, `coop_trigger cScrTask06` ×2. Bug: host `id=154 … hp=50`, client `sync role=client id=154 … hp=0.0`, no new `mirror first place id=154`. Fixed: client revive line, drift ≈ 0. |
| PA-8 | NPC mirror re-forces body states every frame (X6) on 15-clip girl packs | DESYNC (crash risk, UNVERIFIED) | MAP2C Erin/Danny (also fiancée/indiangirl maps) | life | Give the NPC branch the same per-state fail counter; skip `NpcLegClip` for `/npc/teengirl/`, `/npc/nephew/`, `/npc/fiance/`, `/npc/indiangirl/` (audit-a G13). | `-Map MAP2C`: host `coop_trigger cScrTask02`, `coop_trigger cScrTask03` (fire): girls walk. Watch client `debug.log` "Could not forward animation" rate and any `cVMachine::SysMain` fault. |
| PA-7 | Hugh's rifle (MAP2B cs2) / shotgun (MAP2C cs2) only for the process that ran the BeginCutScene | DESYNC | MAP2B, MAP2C | triggers | Comes with `kTriggerHit` replay; or replay `TakeItem` from the host's cutscene. | MAP2C: host `coop_tp 73:3:150.5` (04); client inventory lacks `shotgun1_1st`. |
| PA-6 | Scale reserves clone NPC targets (`hugs2a`, `hugs2b`, `hugs2c`, `abby3c`) | DESYNC/balance (guide death fails the map, G12) | MAP2A 02/06/10/11, MAP2B siege, MAP2C 02, MAP3C | life (scale interplay) | In `Release`, rewrite a cloned `target` that names an NPC to `hunter` (or leave it, by design decision). | MAP2A 3-4 players, host `coop_trigger cScrTask10` ×2: `scale: release … target=hugs2a`. |
| G8 | `SetCanBeAttacked(0)` host-only | DESYNC | MAP2B siege, MAP3C | hunter | audit-a G8 (`0x49ec0`, prologue VERIFIED). | audit-a G8 recipe. |
| G9 | Claim callback never runs on clients | DESYNC | MAP2C | beasts | audit-a G9. | audit-a G9 recipe. |
| G7 | Water crocodiles unmirrored | DESYNC | MAP3A | beasts | audit-a G7 (vt `0xffa50`); needs a state whitelist (9-clip model). | audit-a G7 recipe. |
| G2 | Per-process exits | DESYNC | MAP2A, MAP2B, MAP3A (fork), MAP3B | flow | audit-a G2 (`0x6c420`/`0x6c610`, prologues VERIFIED). | audit-a G2 recipe. |
| G12 | Guide death code 6 outside `0x91c20` | DESYNC | MAP1, 2A, 2B, 2C, 3A, 3C | flow | audit-a G12. | — |
| — | Cobras bite only the host (`CobraBitesNearest=0`) | DESYNC | MAP3B, MAP3C | beasts | test and default `CobraBitesNearest=1`. | — |
| — | Client-killed animal may stand in the host's last pose (local death ran first, the later `PresentDeath` returns early on `+0x600`) | COSMETIC (UNVERIFIED) | all | life | In `ApplyMirror`, when the local copy has `+0x600 != 0`, stop forcing states/health and wait for the host's death. | Real client bullet needed (no input in tests). |
| G14/G16/G17 | hint pages replayed, puppet `trigger_hurt` double damage, per-process pickups/traps | COSMETIC | all | — | audit-a | — |

## 9. Cross-reference (pattern → maps in my range)
- Kill-gated progress, split counts (PA-3/X9): MAP1, MAP2B, MAP3A, MAP3C; end-of-map `CheckMissionStatus` without `CompleteTask` (X2): MAP1, MAP3C.
- BeginCutScene-only animal placement (PA-2): MAP1 06/07, MAP2A 02, MAP2B 03, MAP3A 13/15, MAP3C 03/04.
- Animal `WaitForTask` in a BeginCutScene (PA-4): MAP1, MAP2B, MAP2C.
- Second lives (PA-5): MAP2A, MAP2B, MAP3A, MAP3B (MAP1 none, MAP2C boar only, MAP3C none).
- Item from a BeginCutScene (PA-7): MAP2B rifle, MAP2C shotgun.
- Reserves cloning NPC targets (PA-6): MAP2A, MAP2B, MAP2C, MAP3C.
- Girl-pack NPCs (PA-8): MAP2C.
- Host-only `SetCanBeAttacked(0)` (G8): MAP2B, MAP3C.
- WaitTask gates (X11): MAP1 08, MAP2B 03/08, MAP3A 14.

## Routing
- **life**: PA-3 kill credit `+0x420=1` in `OnAnimalHit` (first, blocks 4 maps); PA-5 `kAnimalRevive` + refreeze any revived mirror; PA-4 no refreeze while a local cutscene runs (flag from X12); PA-8 NPC state-fail counter + girl clip skip; PA-6 reserve target rewrite (decision); client-killed corpse pose.
- **triggers**: PA-1/PA-2/PA-7 `kTriggerHit`, with the X11 `WaitTask` fallback and after PA-3; expose a "local cutscene" flag (`0x6b8b0`/`0x69970`) and use it in `PlayerInCutscene()`.
- **beasts**: G7 crocodiles (MAP3A), G9 client claim callback (MAP2C), cobra bites nearest (MAP3B/3C).
- **hunter**: G8 `SetCanBeAttacked` for puppets (MAP2B siege, MAP3C); Abby/Hugh follow nearest (`npc_follow`).
- **flow**: G2 host-decided exits (MAP2A/2B/3A/3B), G12 guide death (all maps with Hugh/Abby).
- **vehicles / pilot / bosses**: nothing in MAP1-MAP3C (MAP3C heli is one prop anim).

## Proposed MEMORY facts
- `CheckMissionStatus` (`0x94e50`) succeeds only if every task with `+0x5c == 0` has state `+0x68 == 6`; otherwise it sets status back to 1 and returns silently. — `re/notes/lead-mission2.c`.
- `cTaskShoot`: `UpdateTaskStatus` `0x999a0` (action 1 + species: count `+0xf4`--, pool `+0x58`--; action 2: fail; action 5: pool--), `CanBeCompleted` `0x979f0` (condition 1 ammo via `0x2f530`; condition 2 ends the task when the pool is 0: done if a kill counted, failed otherwise). An action-0 death changes nothing. — `re/notes/playthrough-a-task.c`.
- Present-death `0x1e8b0` returns at once if `+0x600` (dead) or props `+0xC1 & 4`; reports `0x91e80(mission, +0x420, +0x434)` then clears `+0x420`. Bullet `OnHit` `0x1a680` sets `+0x420 = 1` for any `mod_obj_bullet` (NPC shots too). — `objmodel-decomp-game-animal.c`, `auditb-animal-onhit-tranq.c`.
- `ActivateHUD` `0x6b8b0` sets trigger state 5 and `*(*(GameDll+0x1657f0)+0x30) |= 8`; `DeactivateHUD` `0x69970` clears bit 8 and sets state 3. — `auditb-trigger-hud-exec.c`.
- An NPC `AITaskNPC_PlayAnim` + `WaitForTask` completes while the NPC is `CharacterBrainFreeze`d (MAP2A client cutscene 1 ran its 24 s with Hugh frozen 0.4 s in). — `dh2coop-trig\game2\Bin\dh2coop.log` 23:40:36-23:41:00.
- Animal `.MODEL` clip lists: GWOLF, HYENA, LEOPARD, MOUNTAINLION have no `idle1-3`; LIONESSE and LION do contain `idleattack1`; CROCODILE_W has 9 clips. `teengirl`/`nephew` `GAME.AP` 15 clips. — `ModelAnimBone` scan of `re/extracted/OBJECTS`.
- Ids: MAP1 bear 104, mlion 105, wtail 106-109, event lion 110, Hugh 82, triggers 92-103; MAP2C Danny 77, Erin 78, Hugh 79; MAP3B reserves 52-54 (hunter 63). — logs cited above.
- Mission-only trigger columns per map (X8). — BSP lumps.

Status: all seven maps audited (static). Nothing built or run.



