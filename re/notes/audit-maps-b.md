# Audit MAP5–MAP10 (job `audit-maps-b`, 2026-10-17)

## Progress
Done. All six maps read from scripts (trigger, mission, world, cutscenes), BSP entity lumps, and GameDll7 decompiles (Ghidra slot B). No game launched. No code edited.

Evidence files (gitignored, `re/notes/auditb-*`):
- `auditb-trigger-*.c/txt`: trigger state machine, OnHit, HUD/ExecuteScript, vtable `0x10afa0`, Update listing.
- `auditb-animal-*.c`: animal respawn/park, OnHit and tranq.
- `auditb-addevent.c`: event pools. `auditb-trap-cage-car.c`: traps, cages, vehicle.
- `auditb-species-table.txt`, `auditb-croc-girl-ctor.c`: species classes and constructors.
- `auditb-hook-bytes.txt`: prologue bytes. `auditb-xref-strings.txt`: string xrefs.
- `auditb-ents-MAPn.txt`: BSP entity lists. `auditb-triggers-0n.txt`: condensed trigger bodies.

Legend:
- **VERIFIED** means I saw it in code or data. **UNVERIFIED** means inferred.
- Status: **OK** = covered (code ref), **RISK** = covered with a known risk, **GAP** = not handled.
- Code refs are `mod/src/coop/*.cpp:line`. RVAs are GameDll7 unless marked C (CommonDll7).

How the BSP entity lump reads: text after the first `entity("` in the BSP. Each record is `entity("<class>")` followed by `property("<key>", "<value>")` lines. Positions are in cm, and the axes match the world (y up). Index = order in the lump. Runtime entity ids follow that order but are not equal to it.

---

## 0. Engine findings that drive every map (new, VERIFIED unless marked)

### 0.1 Trigger state machine: `OnHit` and `BeginCutScene` are not part of the fire we replay
`cObjTriggerEvent` (vtable `0x10afa0`):
- slot `+0x44` Update = `0x68800`
- slot `+0x8c` OnHit = `0x6b140`
- slot `+0x188` OnActivate (fire) = `0x68130`

Fields: `+0x1e4` armed, `+0x1e8` OnHit ran, `+0x1fc` state, `+0x204` delay, `+0x208` script `OnHit`, `+0x20c` `OnActivate`, `+0x210` `BeginCutScene` (function object, released after one run), `+0x214` `UpdateTrigger`, `+0x224` ActivateBy name.

**OnHit** (`0x6b140`) is collision-driven: it runs on the process whose entity touched the volume. It requires all of:
- the trigger is armed;
- every `WaitTask` bit in `+0x200` is complete in **this process's** mission;
- the activator's name (`+0x198`) equals `+0x224`, for example `"hunter"` or `"car1"`;
- state `!= 1` and `+0x1e8 == 0`.

Then it disarms (`0x68fd0`: `+0x1e4 = 0`, mission unregister), runs the script `OnHit`, and sets `+0x1e8 = 1`.

Script `ActivateAfter(t)` (`0x694c0`) calls the state setter `0x68d70(trigger, 3)`. If a `BeginCutScene` exists, the setter writes **4** instead of 3.

Update `0x68800`, by state:

| State | What Update does |
|---|---|
| 4 | Runs `BeginCutScene` (this is where `ExecuteScript("cScrCutSceneN")` lives) |
| 5 | Waiting for the cutscene. `ActivateHUD` (`0x6b8b0`) sets it |
| 3 | Counts the delay down. `DeactivateHUD` (`0x69970`) and ForceEndCutScene (`0x68da0`) set it when the cutscene ends |
| 2 | Calls `0x68130` with the **local** player, which runs `OnActivate` and sets the state to 1 |

Update also calls the script `UpdateTrigger` every frame while armed **and the local player (`0x6c30`) is inside the box** (`0x68740`).

Source: `auditb-trigger-update-onhit.c`, `auditb-trigger-state.c`, `auditb-trigger-hud-exec.c`, `auditb-trigger-update-listing.txt`.

What the sync does today:
- `story.cpp` replays only arm (`0x68eb0`) and fire (`0x68130`): `ApplyFireCommit` and `OnFireRequest` call `Fire()`, which is `0x68130` (story.cpp:340, 375, 348).
- So `OnHit` and `BeginCutScene` run **only on the process whose own hunter or vehicle entered**.
- The other process gets `OnActivate` alone. It never runs the `OnHit` script, never runs `BeginCutScene`, and never sees that cutscene.

This corrects the checklist line "Cutscenes: started by a fire". Only `OnActivate`-started and map-start cutscenes replay. Cutscenes in `BeginCutScene` do not.

MAP1/MAP2 probably felt solid because both players walked into the volumes together: each process then ran its own `OnHit`. UNVERIFIED.

### 0.2 Stuck state 2 on clients (log flood)
When a client's own trigger reaches state 2 and the host fire has already run there (`g_ran_fire`), `hk_TriggerFire` returns without calling the original (story.cpp:469). `+0x1fc` stays 2, so Update calls `0x68130` every frame forever.

Evidence: `game3\Bin\dh2coop.log` (MAP2A client, 15:57:55–15:58:36) has 3742 of its 4622 lines as `story: [client] trigger FIRE id=117 name='cScrTask01'`.

### 0.3 Script `MoveToPos` on an animal is a full respawn; `Kill()` is a park
- `cScrCharacterEx::MoveToPos` (`0x161a0`) calls animal vt `+0x158(3,0)`, which is `0x14610`, then sets the position.
- `0x14610` resets health to max (`+0x4c0`), clears tranq state `+0x4f0` and tranq hurt `+0x4f8` (0 unless the script set it), sets body `IdleAttack`, sets active `+0x4b0 = 1`, and writes **brain `+0x79 = 0`**. That byte is the `FreezeBrain` flag the mirror uses.
- `Kill()` (`0x16a40`) calls vt `+0x234`, which is `0x1be20`: inactive `+0x4b0 = 0`, brain `+0x79 = 1`, position `(-10000,-10000,-10000)`, `RemoveDeadAnimal`.
- `CanBeRespawned()` = `!vt+0xdc` = `!+0x4b0`.
- Every MAP5–10 wave reuses the same few animals through `MoveToPos`. There are no runtime animal spawns.

Source: `auditb-animal-respawn.c`, `auditb-animal-kill-moveto.c`.

### 0.4 Event pools are created at map load (ids match across processes)
`AddEvent` (`0x76b0` → `0x82f30`) creates fixed pools, then precaches, spawns, names (`"<name><n>"`) and registers each entity:

| Event | Creates |
|---|---|
| `EVENT_TRAP` (1) | 10× `obj_trap` named `trap1..10` |
| `EVENT_CAGE` (10) | 10× `obj_cage` |
| `EVENT_BEES` (4) | 5× `obj_bees` |
| `EVENT_FIRE` (5) | 5× `obj_fire_event` |
| `EVENT_MINE` (6) | 10× `obj_mine` |
| `EVENT_ANIMAL` (7) | 1× `obj_animal` (the given species) named `animal1` |
| `EVENT_SNAKE`, `EVENT_BIRDS`, `EVENT_BATS`, `EVENT_CAVEDUST` | 1 each |

Source: `auditb-addevent.c`.

### 0.5 Species → class
The table at `0x163dd8` maps every species to `obj_animal`, except:
- `0x13` → `obj_cobra`;
- `0x1f` `ANIMAL_WATERCROCODILE` → `obj_crocodile`.

`cObjCrocodile` (ctor `0x1d720`) is the `cScrAnimal` ctor plus its own **vtable `0xffa50`**. The tracker's `Animals()` matches only `0xff470` and `0x10e920` (player_tracker.cpp:329), so water crocodiles are not mirrored.

`ANIMAL_CROCODILE` (0x18) is a plain `obj_animal` and is mirrored.

Source: `auditb-species-table.txt`, `auditb-croc-girl-ctor.c`.

### 0.6 Tranquilizer hits never reach the host
`cScrAnimal::OnHit` (`0x1a680`, vt `+0x8c`) on a `mod_obj_tranq_bullet`:
- sets tranq state `+0x4f0 = 2`;
- damages by `+0x4f8`, which is 0 unless `SetTranqHurt` (BTIGER and ELEPHANT set 0.0);
- fails the mission on a second dart into a protected animal (`+0x604`) through `0x965e0`.

A client dart on a mirror therefore leaves health unchanged, and the mirror's hit detection (health drop, animals.cpp:1015) sends nothing.

Source: `auditb-animal-onhit-tranq.c`, `MODELS/ANIMALS/BTIGER/BTIGER.SCR`, `ELEPHANT.SCR`.

### 0.7 Traps, cages, vehicle
- **Trap.** `cObjTrap::OnCollision` (`0x85d40`) reacts only to `IsEntityKindOf("obj_player")`. Puppets are `obj_humanEx`, so they never close a trap. Guides (`cObjNPC`) do, and get the `CloseGuide`/`synctrap` state. `HurtHuman` (`0x86b00`) hurts the trapped entity (vt `+0x14c`, type 2).
- **Cage.** `cObjCage` falls on and hurts its `+0x214` target (20, type 7). The cage is kicked open by melee (`cage_kick`); a bullet makes it fall (`cage_fall`).
- **Vehicle.** `cObjHunter::GetInCar` (`0x48d10`) seats through the vehicle manager (`*(0x1657f0)+0x104`). `cObjHunter::DriveTo` (`0x48e40`) takes the first `cMOD_ObjVehicle` and calls `cObjVehicle::DriveTo`. UNVERIFIED: that the vehicle then drives itself along waypoints (it has `OnTargetReached`/`GetTargetRefSpd` exports). `cMOD_ObjVehicle::Update` (`0x3b710`) hurts the entity at `+0x29c` by 100 when its vt `+0xfc` says it is a player and the up-axis dot is under a threshold. UNVERIFIED reading: the seated player dies when the truck tips over. `cMOD_ObjVehicle::OnCollision` (`0x3b8c0`) calls trigger `OnHit` (vt `+0x8c`) with the vehicle as the activator.

Source: `auditb-trap-cage-car.c`.

### 0.8 Other C++ facts used below
- `cObjGirl` (ctor `0x63f30`) = `cObjNPC` ctor `0x62720` + vtable `0x108d38`, with `+0xb64 = 3` (= `AI_CHILD_ID`). It is not `cObjGuide`, so the girl's death does not post fail code 6 (`npc-health.md`).
- `UpdateMissionStatus` (`0x8390` → `0x91e80`) only records action/value at mission `+0x1dc/+0x1e0` for the running task. The protected-species kill fails through `0x91c20`.
- Task list: mission `+0x1f8` (count `0xa5f40`, element `0xa6080`). Task id `+0x64`, state `+0x68`. Done = 6, or 5 with `+0x5c` set. Source: `auditb-task.c`.
- Prologue `55 8b ec 6a ff` (the `CheckedGameRva` pattern) at `0x6b140`, `0x14610`, `0x1a680`, `0x68fd0`, `0x1be20`, `0x3b710`. `0x68d70` is a leaf with no SEH frame. Source: `auditb-hook-bytes.txt`.

---

## MAP5 — Hamisi, the poacher, hyenas, then the truck stampede

Tasks (MISSION05):
1. race (`TASK_RACE`)
2. kill 1 hyena
3. race (stampede)

Triggers:
- `cScrTask01` (map start) arms 2, 4, 6–20 and protects Hamisi.
- 02–05 are the camp reach steps.
- `06`'s `BeginCutScene` runs CompleteTask(2), parks the elephants, and starts **cutscene 3** (poacher/Hamisi `play_sa` cs03, then GetInCar 1/2, `DriveTo("PathNode12")`, `TakeItem("tranqgun1_1st")`).
- 07–18 are `cStampedeTriggerMap5` with `ActivateBy("car1")`. Their `UpdateTrigger` respawns elephants (`rand`, `CanBeRespawned`, `MoveToPos`, `SetHealth(100)`, `AttackChargers` with target `"hunter"`).
- 19 (`car1`) brings in the first two elephants.
- `20` (`car1`) `BeginCutScene` runs CompleteTask(1,2,3) and cutscene 4, then `OnActivate` runs CheckMissionStatus.

| Mechanic | Where | Status |
|---|---|---|
| Map-start trigger + cutscene 1 | Task01 `Spawn` `Activate`+`ActivateAfter(0)` | **OK**: runs on every process locally (each spawns the same way). VERIFIED |
| Reach steps 02–05 | `OnHit` `ActivateAfter(0)`; Task05 `OnHit` REACHED 1; Task04 `OnHit` `SetCanBeAttacked(0)` + `RemoveAIToBeProtected("hamisi5")` | **GAP G1**: whoever does not enter never runs these `OnHit` bodies. If the client enters 04 first, the host keeps Hamisi protected and attackable during the hyena surprise. A Hamisi death is code 6 (G9). VERIFIED (script + 0x6b140) |
| Hyena attacks | Task03/Task04 `MoveToPos` (random spot) + `AttackDogs` target `"hunter"`; Task04 state `Kill()`, `MoveToPos` respawn | **RISK** (combat.cpp:499 retargets to the nearest player). **GAP G2** for the respawn: replayed `MoveToPos` wakes the client brain, and a hyena killed then reused stays dead on the client. VERIFIED (0x14610) |
| Truck cutscene and boarding | Task06 `BeginCutScene` → cutscene 3 | **GAP G1+G4 (blocks)**: only the process that walked in boards its own truck and gets the tranq rifle. The other stays on foot at the camp. VERIFIED (scripts + 0.1) |
| Vehicle `car1` (`obj_mod_vehicle`, resource `truck1`, BSP lump 126) | `cMOD_ObjVehicle`, `cObjVehicle::DriveTo` | **GAP G4**: not mirrored (not an animal, no `/npc/` model). Each process drives its own truck. VERIFIED (BSP + tracker filter) |
| Car-activated triggers 07–20 | `ActivateBy("car1")` → vehicle `OnCollision` → trigger `OnHit` | **GAP G1/G8**. The `UpdateTrigger` spawners run only where the local player sits in the volume. If the client's truck reaches Task20 first, the host runs only `OnActivate` (CheckMissionStatus with tasks 1–3 open) and its `+0x1fc = 1` then blocks its own `OnHit`, so **the host can never finish the map**. VERIFIED code path, runtime UNVERIFIED |
| Elephants (`AttackChargers`, respawn) | `UpdateTrigger` | **GAP G2, G3, G8** |
| Tranq weapons | `EquipAIWith("hunter","blowdarts_1st")`, tranq rifle by `TakeItem`; elephant `SetTranqHurt(0)` | **GAP G3**: client darts never reach the host. VERIFIED |
| Paired cs03 poacher↔Hamisi | `play_sa` in cutscene 3 | **OK** by `kPlaySA` (story.cpp:557) when the host runs it. The client gets the PlaySA without its own cutscene: odd but harmless |
| Hamisi (`obj_guide` = `cObjGuide`, `objects/npc/hamisi`) | path-lead `MoveToPos`, protected | **OK/RISK**: mirrored NPC (player_tracker.cpp:334). Health mirrored (animals.cpp:1028). Death = code 6 (G9). His pack has `crouchidlecar`, `idlenormalneutral`. VERIFIED |
| Poacher (`obj_poarcher`, `cScrPoachera5`) | cutscene partner, then `MoveToPos("ci_3")` | **RISK G11**: `GAME.AP` has only `blink` (and `MAP5_POACHER.AP` only `cs03`). The mirror's `NpcLegClip` names do not exist on it. VERIFIED (ap_list) |
| Pickups | none in the BSP | — |
| `"hunter"` reads | `ActivateBy` ×6, attack `target` ×7, cutscene `player` | host-only targets; retarget in combat.cpp |
| Exit | none. CheckMissionStatus in Task20 `OnActivate` | **OK** via `kMissionEnd` (events.cpp:410), provided the host reaches it |

## MAP6 — Reggie and the rangers, leopards and a bear, tranq gun only

Tasks:
1. race
2. kill 1 BTIGER
3. kill 2 BTIGER
4. kill 1 ABEAR
5. kill 2 BTIGER
6. race
7. kill 4 BTIGER
8. kill 1 ABEAR

The end trigger `cScrTask09` (`WaitTask` 7 and 8) completes 1–9 in its `OnActivate` state, then runs CheckMissionStatus. So the mission is **resilient to G1**.

| Mechanic | Where | Status |
|---|---|---|
| Leopard/bear waves | Task02–08 `MoveToPos` + `AttackCats`/`AttackBears` target `"hunter"`. leopard1 reused 02/05/07, leopard2/3 03/05/07, abear 04/08. Cutscene 2 `Kill()`s the leftovers | **GAP G2** (respawn), **RISK** host target (combat.cpp retarget). VERIFIED |
| Weapon | `EquipAIWith("hunter","tranqgun1_1st")` only; BTIGER `SetTranqHurt(0)` | **GAP G3**: the client cannot affect any animal. VERIFIED |
| `BeginCutScene` side effects | Task02 (leopard1 `MoveToPos`, cs4), Task04 (abear, cs5), Task06 (cs2), Task08 (REACHED 6, Reggie `DenyTask`, cs3) | **GAP G1** (client misses these cutscenes; mission still completes) |
| `OnHit` REACHED 1 | Task07 | G1, but resilient (Task09 completes all) |
| Reggie (`obj_guide`) | protected (never removed), `SetCanBeAttacked(0)`, path-lead | **OK/RISK G9** |
| redshirt1/3 (`obj_guide`, `ausranger1/3`); redshirt2 is declared but absent from the BSP | bear `target "redshirt1"` (Task08); cs3 `play_sa` redshirt1↔reggie6; `SetAttribute` on redshirt3 | **RISK**: an NPC killed by the host bear is code 6 for both (G9). `ausranger3` lacks `walk/left/right/backward` (G11). PlaySA **OK** (`kPlaySA`). VERIFIED (BSP) |
| Cages | 22 `info_static_event` with `ai=obj_cage`; `EVENT_CAGE` pool of 10 | **G13** per-process hazard. VERIFIED (BSP + 0x82f30) |
| Bees, birds, ambient animal | `EVENT_BEES`/`EVENT_BIRDS`/`EVENT_ANIMAL BTIGER` | ambient. There is no `EVENT_MAP6.MAT` (assets.md), so the animal event probably never activates. UNVERIFIED |
| `OpenMenuPage(11)` | Task10 `OnActivate` | replays on both (MAP1 precedent). UNVERIFIED whether it pauses the client |
| Stumps | 5 `obj_target` | **OK** `props.cpp` (`kPropHit`) |
| Exit | none. Task09 CheckMissionStatus | **OK** (`kMissionEnd`) |

## MAP7 — find the girl, tigers, a cobra, a rhino, tranq gun only

Tasks:
1. `TASK_FIND` `AI_CHILD_ID`
2–5. tiger kills
6. kill 1 cobra (`ActivateTriggerOnEnd("cScrTask08")`)
7. race
8. kill 1 rhino (flee → `MISSION_ACTION_KILL`, `ActivateTriggerOnEnd("cScrTask10")`)
9. race

| Mechanic | Where | Status |
|---|---|---|
| **Find the child** | only `cScrTask07` `OnHit` runs `UpdateMissionStatus(FIND_SOMETHING, AI_CHILD_ID)`. Nothing else completes task 1 (Task11 completes 7–9) | **GAP G1, blocks progress**: if the client enters Task07 first, the host's task 1 never completes, the host's `+0x1fc = 1` blocks its own later `OnHit`, and Task11's CheckMissionStatus can never succeed. VERIFIED (script + 0x6b140 + story.cpp:348) |
| Girl `indie_girl` | BSP `obj_girl`, class `cScrGirl`, `girl.spj`, `objects/npc/indiangirl` → `cObjGirl` (`cObjNPC` subclass) | **OK** mirrored as an NPC (`/npc/`). **RISK G11**: her `GAME.AP` has `idle`, `walk/forward`, `run/forward`, `jump`, `point`, `conversation`, `crying`, turns. It has no `idlenormalneutral` and no sideways walks, so the mirror's leg clip (animals.cpp:291) is missing. VERIFIED |
| Cobra `cobra1` | `AddAnimal(ANIMAL_COBRA)`, Task07 state `Task_CobraAttack` target `"hunter"` + `MoveToPos` | **OK** mirrored (`IsCobra` paths, animals.cpp:372–425, 985). **RISK G12**: bites only the host (`CobraBitesNearest=0`) |
| Rhino flee = kill | `SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL)` | **RISK**: host simulation. Mirror placed on the host's position. UNVERIFIED whether the client's own flee check also fires |
| Tigers | `MoveToPos` reuse (tiger1 02/05, tiger2/3 03/06); Task04 `rand()` boat spot | **GAP G2**, host targets |
| Weapon | `tranqgun1_1st` only | **GAP G3** |
| `BeginCutScene` | Task04 (CompleteTask(3), escape1, cs2), Task09 (rhino `MoveToPos`, cs6) | **GAP G1**: CompleteTask(3) is lost on the host when the client is first. Task 3 is a kill task, so it still completes when both tigers die on the host. It **blocks only if they timed out** (the script sets `time 1.0` on them). No later trigger re-completes 3. VERIFIED script; RISK |
| `OnHit` REACHED 9 | Task11 | resilient (CompleteTask 9 in the state) |
| Reggie | protected 01→09, path-lead | **OK/RISK G9** |
| Traps | `EVENT_TRAP` pool, `trap1.MoveToPos("capcana3")`; BSP `obj_trap capcana1` (`cScrTrapEnt`) | **G13** per process (puppets never trigger). VERIFIED (0x85d40) |
| Cages ×9, bees, birds, ambient snake, ambient BTIGER | static events, `EVENT_*`; `EVENT_MAP7.MAT` exists | **G13**. The ambient `animal1` is mirrored by id, but its event placement near the local player calls the respawn → **G2** brain wake. UNVERIFIED |
| BSP `trigger_level` "cutscene_outro" → `map8a,human` | not referenced by any script; no `MAP8A.BSP` | dead entity. VERIFIED |
| Exit | none. Task11 state CheckMissionStatus | **OK** if G1 does not hit |

## MAP8 — find the guide, three tribesmen, scrub-bull stampede, crocodiles

Tasks:
1. `TASK_FIND` `AI_GUIDE_ID`
2–5. scrub bull kills
6. race
7–9. bull kills
10. race

| Mechanic | Where | Status |
|---|---|---|
| **Find the guide** | only `cScrTask08` `OnHit` runs `UpdateMissionStatus(FIND_SOMETHING, AI_GUIDE_ID)` | **GAP G1, blocks progress**. VERIFIED |
| **End trigger** `cScrTask15` | `OnHit`: CompleteTask(6–9) + REACHED 6 + `ActivateAfter(1)`. `BeginCutScene`: CompleteTask(10), REACHED 10, tribesmen companion, cs3. `OnActivate` state: CheckMissionStatus | **GAP G1, blocks progress** when the client enters first. VERIFIED |
| Tribesmen ×3 (`obj_guide`, `abo1..3`) | 01 `SetCanBeAttacked(0)`; 08 `SetAIType(AITYPE_COMPANION,"hunter")` + `goinwater` off; 17 `SetCanBeAttacked(1)` + `COMPANION\|IDLESCARE` | **GAP G7**: they follow the host only (npc_follow.cpp, `[Story] NpcFollowNearest` default 0, npc_follow.cpp:102). After 17 they can die → code 6 (G9). Mirrored (`/npc/`). VERIFIED |
| Scrub bulls | waves 02–05/09–11 reuse scrubbul1–3; stampede 12/13/14/23/24 (`cStampedeTriggerMap8`) `UpdateTrigger` with `rand`, `CanBeRespawned`, `MoveToPos`; `SetMaxAttackerNo(5)` in 12 `OnHit` | **GAP G2** (reuse), **GAP G8** (stampede only where the host stands). The hunt area points are toggled in `UpdateTrigger` too. VERIFIED |
| Crocodiles `crocodile1..3` (`ANIMAL_CROCODILE` 0x18) | Task06 `MoveToPos` + `AttackTraps` | **OK** mirrored (`obj_animal`); G2 on reuse |
| Water crocodiles `croco1..3` (`ANIMAL_WATERCROCODILE` 0x1f) | Task07 `UpdateTrigger` `SpawnCrocs` every 8 s (`rand`, `AttackFloaters`) | **GAP G6**: `cObjCrocodile` vt `0xffa50` is not in `Animals()`. Each process runs its own crocs, live brain included. VERIFIED (0.5) |
| Sync attacks | `pl_sb_*`, `pl_cr_syncattackcroc` in `TEENM\BODY\MAP8.AP` | **OK**: the clip name rides `kPlayerKnock` (combat.cpp hk_PlaySA). kMapClips MAP8 front/back exist. VERIFIED (ap_list) |
| `SetDenySATime(0.1 / 15)` | Task06/Task17 on `player` | **G14**: local hunter only. The host's puppet can still be pounced during the 15 s window |
| Props `rockt1`, `atree`, `fxtree` (`obj_scripted_entity` `cScrEntity`) | `SetAnim` in 01/69/16 `OnActivate` | **OK**: both run `OnActivate`, plus `kPropAnim` (story.cpp:581) |
| `trigger_hurt` ×4 (5000/100/6/100), 32 `obj_aifx` fires, `EVENT_FIRE` pool | BSP | **G13** per process; fires are static FX |
| Pickups (3 ammo, 5 medkits) | `obj_world_item reuseable=0` | per process (each player gets their own copy). OK by design |
| Stumps (named `sticla1–5`, trunk, cheat 1) | `obj_target` | **OK** `props.cpp` |
| Exit | none. Task15 state CheckMissionStatus | **OK** if G1 does not hit |

## MAP9 — Reggie companion/guide, dingoes, ice

Tasks:
1. race
2–6. dingo kills (2, 2, 2, 3, 2) with **3 dingoes in total**

Task11: `OnHit` REACHED 1; its state runs CompleteTask(2–6), `heli1.SetAnim("m9_rescue")` and CheckMissionStatus.

| Mechanic | Where | Status |
|---|---|---|
| **Final reach** `cScrTask11` | task 1 is completed only by its `OnHit` REACHED 1 | **GAP G1, blocks progress** when the client enters first. VERIFIED |
| Dingoes (3 for 11 kills) | `MoveToPos` respawn in 03/04/05/06/08/10 | **GAP G2, severe**: after the first deaths, the client sees and can shoot nothing. VERIFIED (0x14610 + animals.cpp:929/660) |
| Reggie (`obj_guide`, reginald) | 02 companion of `"hunter"` + protected; **07 `AITYPE_GUIDE,"a"` + `MoveToPos c3_1`**; 08 companion; `gooutofpath` follow cmd; cs1 `SetAttribute` health | **GAP G7** (follow/wait on the host only; npc_follow off by default). Health **OK** (mirrored) |
| Weapons | starts with `knife1_1st`; `handgun2_1st` and `mbranch_1st` are BSP pickups (`AiCanFindItem`) | per process: each player finds their own. Knife/branch hits on mirrors drop health → `AnimalHit`. UNVERIFIED |
| Ice ×3 (`obj_ice`) over `trigger_hurt` 100000 | near the start (-124..-132, 18..20, -9..0) | **G13**: cracking is per process; the host falling is the level end (policy). UNVERIFIED crack trigger |
| Helicopter `heli1`/`heli1_test` (`obj_helicopter`, `cScrHelicopter`) | only `SetAnim("m9_rescue")` in Task11's state | **OK**: runs on both via the `OnActivate` replay. `cScrHelicopter::SetAnim` is its own export, so `kPropAnim` probably does not see it. UNVERIFIED, irrelevant |
| Bats, birds, cavedust, ambient dingo (`EVENT_MAP9.MAT` exists) | events | ambient. Ambient dingo → G2 brain wake. UNVERIFIED |
| `BeginCutScene` | 02 (cs1), 03 (dingo1 `MoveToPos`, cs4), 04 (cs2), 08 (cs3) | **GAP G1** (client misses these cutscenes) |
| Exit | none. Task11 | **OK** if G1 does not hit |

## MAP10 — Wirake and Reggie, pumas, boars, "mines"

Tasks:
1. race
2–8. puma/boar kills
9. race
10–11. kills; task 11 (2 pumas) runs `ActivateTriggerOnEnd("cScrTask17")`

`cScrTask17` sits at (-155.5, 22.2, 314.6), outside the play area. Its `OnActivate` completes 1–11, then runs CheckMissionStatus. The mission is **resilient to G1**.

| Mechanic | Where | Status |
|---|---|---|
| **"Mines"** | `cScrMine mine1..10` is commented out in `WORLD.SCR`; `AddEvent(EVENT_MINE)` is commented out. The `mine_*` names are `info_spawn_point`s that receive **traps**: `EVENT_TRAP` pool `trap1..10` via `MoveToPos` in 01/04/05/08 `OnActivate` | **G13**: cObjTrap per process. Puppets never trigger them; mirrored guides can trigger the client copy (`CloseGuide`, hurt blocked by animals.cpp:131). Positions identical (replayed `MoveToPos`). VERIFIED |
| Puma/boar waves | wildboar1/2 reused 04/06/08/10/14, puma1 02/07/16, puma4 22/09/11/12; cs4/cs5 `Kill()` park the leftovers | **GAP G2** |
| Reggie (`obj_guide`, reginald) | 01 `COMPANION\|COVERSTAY,"hunter"`; **09 `AITYPE_GUIDE,"a"` + `MoveToPos("c5_1")`**; 17 companion again; weapon `mbranch` | **GAP G7** |
| Wirake (`obj_guide`, wirake) | protected 01, path-lead; 09 `COMPANION,"hunter"`; 14 boar `target "wirake10"` | **OK/RISK G7/G9** |
| Paired anims | cs1 `wirake10 play_sa reggie10` (map start); cs5 `reggie10 play_sa puma1` (Task16 `BeginCutScene`) | **OK** `kPlaySA` + `HoldCutsceneNamed` (story.cpp:577–578) |
| `BeginCutScene` | 22 (puma4 `MoveToPos`, cs3), 21 (boars, cs2), 09 (puma4, cs4), 16 (REACHED 9, park/aim, cs5) | **GAP G1** (client misses these cutscenes; mission resilient) |
| Weapons | `dbrifle1_1st` + `macheta_1st` | puppet shows the rifle1 mesh with the flame bone (G15) |
| Pickups (2 ammo, 3 medkits) | `obj_world_item` | per process, OK |
| Sync attacks | `pl_p_*`, `pl_wb_syncattack1` in `MAP10.AP` | **OK** |
| Exit | none. Task17 | **OK** |

---

## Cross-map: every `"hunter"` read (MAP5–10)

| Read | Where | Who it means today | Covered |
|---|---|---|---|
| `ActivateBy("hunter")` | every trigger (6/12/19/29/16/22 per map) | the colliding entity's name `+0x198` (`0x6b140`). Each process tests its own hunter; puppets never match | requests (story.cpp:446/478); G1 for `OnHit` |
| Animal task `target "hunter"` | 7/10/9/18/13/24 per map | host hunter (FindEntity + attack list, npc-follow.md §5) | combat.cpp:499 retarget (needs `AttackTarget=1`, the default) |
| `SetAIType(COMPANION…,"hunter")` | MAP8 ×9, MAP9 ×2, MAP10 ×3 | host hunter pointer at `+0xb68` | npc_follow.cpp (off by default) → G7 |
| `target_cutscene "hunter"` (LookToEntity) | MAP6/7/9/10 cutscenes and states | host | cosmetic |
| `PlaySoundEffect(…,"hunter")` | many | local hunter | OK |
| `FindEntScrObject("hunter")` → `player` | every `WORLD`/cutscene | local hunter: `StartTask` HUD, `TakeItem`, `GetInCar`, `SetDenySATime`, `SetRecoverSpeed`, knife distance | OK local; G4/G14 |
| C++ trigger Update `0x6c30` | `UpdateTrigger`, trigger hint | local player inside the box | G8 |
| C++ cobra spawn `0x6d80("hunter")` → `+0x728` | MAP7 | host | animals.cpp:473 (opt-in) → G12 |
| C++ animal respawn `0x14610(1,…)` `0x6d80("hunter")` | event surprise spawns | local | G2 |
| `GetDistBetween2Ais` | not used on MAP5–10 | — | — |

## Exit tokens and fail paths

- **No exit token** on MAP5–MAP10. There is no `ActivateTriggerLevel` or `exit_*` in the six trigger scripts. VERIFIED. Each ends with `CheckMissionStatus` → `0x94e50` → `0x91c20` code 5 → `kMissionEnd` (events.cpp:410). The next map comes from the host's results/menu → `kMapStart`.
- MAP7's BSP `trigger_level` to `map8a` is unreferenced. VERIFIED.
- Fails through `0x91c20` (covered): tranq overdose and protected-animal kill (`0x965e0`), protected species kill (`0x91e80`).
- **Fails that bypass `0x91c20`** (G9): `cObjNPC::OnDie` (`0x5fde0`) for any `cObjGuide` posts code 6 directly. That covers Hamisi, Reggie ×4 maps, the redshirts, the tribesmen ×3 and Wirake. It works today only because the client's mirrored profile health reaches 0 and its own `OnDie` posts the same code (animals.cpp:1028). Girl and poacher deaths do not fail.
- No `NEED_AMMO_FOR_COMPLETE`, no countdowns (`SetTaskTimeLimit(0)` everywhere), no bosses, no stealth on MAP5–10. VERIFIED (native counts).

---

## Ranked gap list

Severity: **BLOCKS** = the map cannot be finished; **DESYNC** = the players see or play different games; **COSMETIC**.

### G1 — `OnHit` and `BeginCutScene` are never replicated (BLOCKS: MAP5, MAP7, MAP8, MAP9; DESYNC on all)
**What happens.** Section 0.1. When the client's hunter (or truck) enters first:
- The client runs the `OnHit` script, `BeginCutScene` and its cutscene locally.
- The host runs only `OnActivate` (`OnFireRequest` → `Fire` → `0x68130`).
- Its `+0x1fc = 1` then blocks the host's own `OnHit` forever.

Lost on the host:
- MAP7: FIND child (Task07), CompleteTask(3) (Task04 `BeginCutScene`).
- MAP8: FIND guide (Task08); CompleteTask 6–10 (Task15).
- MAP9: REACHED 1 (Task11).
- MAP5: truck boarding (Task06), CompleteTask 1–3 (Task20), protect removal (Task04).

When the host enters first, clients never see `BeginCutScene` cutscenes and their mission HUD lags.

**Fix.**
1. Hook `cObjTriggerEvent::OnHit` (GameDll `0x6b140`, vt `+0x8c` of `0x10afa0`, thiscall(trigger, activator); prologue `55 8b ec 6a ff` VERIFIED).
2. In the hook: read `+0x1e8`, call the original, and if `+0x1e8` went 0 → 1:
   - on the host, `Commit(kTriggerHit = new kind, id, name)`;
   - on a client, `Request(kTriggerHit)`.
3. Apply where `+0x1e8 == 0 && +0x208 != 0 && +0x1fc != 1`:
   1. `0x68fd0(trigger)` (fastcall disarm);
   2. `cdecl g_pClassManager->vt[+0x28](mgr, *(trigger+0x208), *(trigger+0x1b4))` (cdecl with explicit mgr, `ADD ESP,0xc` in the listing);
   3. `+0x1e8 = 1`.
   Then each process's own state machine runs `BeginCutScene` → cutscene → `OnActivate`. The host also applies a client's request this way instead of `Fire()`.
4. Journal `kTriggerHit` before `kTriggerFire`. A late joiner skips old cutscenes the same way (`WatchSkip`).

**Test recipe** (today, shows the bug).
1. `.\run-coop-local.ps1 -Map MAP8`.
2. Wait for the client line `story: applied host fire 'cScrTask01'`.
3. Write `coop_tp 58.4:0:223.1` to `game2\Bin\dh2coop.cmd` (cScrTask08 volume, BSP `(5843,-128,22312)`/100).
4. Expect:
   - client `story: [client] trigger FIRE … name='cScrTask08'` after its cutscene;
   - host `story: [host] ran 'cScrTask08' (id N) for player 1`;
   - host window not in a cutscene (`-Capture`).
5. Then `coop_tp` the host into the same volume: there is no new host fire line, and the guide task stays open on the host HUD.
6. After the fix: host `story: … kTriggerHit` line and the host plays cutscene 2.

A `coop_tasks` console dump (mission `+0x1f8` list, id `+0x64`, state `+0x68`) is the cheap way to read task state in logs. Propose adding it with the fix.

Caveats for every recipe below:
- `coop_trigger` arms and fires through `hk_TriggerEvent`/`hk_TriggerFire` (story.cpp:611) and **skips `OnHit` and `BeginCutScene` even on the host**. Use `coop_tp` into the volume for real flows.
- `coop_trigger X` fires when X is already armed (log `coop_trigger 'X' -> id N … armed=true … -> fire`) and arms otherwise. Most mid-map triggers are armed by `cScrTask01`'s `OnActivate`, so one call fires them. Repeat only when the log says `-> arm`; a second call on an armed trigger runs `OnActivate` again.

### G2 — Animal respawn (`MoveToPos` = `0x14610`) is not mirrored, and it wakes client brains (DESYNC on all six maps, worst on MAP9)
**What happens.**
- After a death, the client mirror sets `death_presented` and ignores that id forever (animals.cpp:929). The host's later death of the same id is not committed again (`announced_` set, animals.cpp:660, plus the `Commit` dedup).
- Every replayed `MoveToPos` (in `OnActivate`) also clears brain `+0x79` on the client while the mod still thinks the mirror is frozen (`m.frozen`, animals.cpp:965). That client animal runs its own AI and `AttackX` task against the client's own hunter, while the snapshot keeps teleporting it.
- Maps: MAP9 (3 dingoes for 11 kills), MAP8 bulls, MAP10 boars/pumas, MAP6 leopards/bear, MAP7 tigers, MAP5 hyenas/elephants.

**Fix.**
1. Host: hook `0x14610` (prologue VERIFIED), or detect `+0x4b0` 0 → 1 or health 0 → >0 in `HostTick`. Bump a per-id life serial and send it in `AnimalState` (new byte or flag). Clear `announced_` for that id and use the serial in the `kAnimalDied` key so the next death commits.
2. Client: when the serial changes, reset the `Mirror` (`death_presented`, `dead_`, `applied_state`, `hold_pose`) and re-freeze.
3. Every frame in `ApplyMirror`, re-freeze any animal whose brain byte `+0x79` is 0 while `m.frozen` (one byte read). Log `animals: mirror id=N brain woke`.

**Test recipe.**
1. `.\run-coop-local.ps1 -Map MAP9`.
2. Host cmd: `coop_trigger cScrTask03` once (armed by Task01; check `-> fire`).
3. Read the dingo ids from the host log `animals: id=<n> state=`.
4. Host: `coop_kill <dingo1>`. Expect host `animals: death commit id=<n>`, client `animals: present death id=<n> ok=true`.
5. Host: `coop_trigger cScrTask04` once.
6. Expect host `animals: sync role=host id=<n> … hp=<max>` (alive) and client `sync role=client id=<n> … hp=0.0` (stays dead).

### G3 — Client tranquilizer hits do nothing (DESYNC; MAP6 and MAP7 use only the tranq gun, MAP5 stampede)
**What happens.** Section 0.6: tranq darts do 0 health damage, so `AnimalHit` is never sent. The client player is a spectator on two whole maps.

**Fix.**
1. Client: hook `cScrAnimal::OnHit` (`0x1a680`, vt `+0x8c` on `0xff470`; prologue VERIFIED). If the bullet `IsEntityKindOf("mod_obj_tranq_bullet")` and the target is a mirror, send a tranq hit (new `AnimalHit` kind, or `GameEvent kTranqHit`).
2. Host: emulate the tranq branch:
   - if `+0x4f0 == 2 && +0x604 == 1` → let `0x965e0` fail (it rides `kMissionEnd`);
   - otherwise `+0x4f0 = 2`, vt `+0x228(+0x4f8, shooter)`, and brain vt `+0x88(shooter)`.
   The brain's use of `+0x4f0` is UNVERIFIED. Test that the animal limps and sleeps.

**Test recipe.**
1. `.\run-coop-local.ps1 -Map MAP6`.
2. Host: `coop_trigger cScrTask02` once (leopard1 attacks).
3. The user shoots leopard1 from the client window (input needed).
4. Bug: no client `animals: local hit id=` and no host `animals: AnimalHit`.

### G4 — MAP5 truck is not synced (BLOCKS if the client boards first; otherwise the client is left on foot)
**What happens.** `car1` is `cMOD_ObjVehicle`. Boarding and `TakeItem` happen in cutscene 3 (G1). Each process drives its own truck.

**Fix.**
- Short term: G1 makes both processes run cutscene 3. Each player then rides their own truck on the same waypoint path, with drift.
- Proper: on the client, freeze the vehicle (`cObjVehicle::Freeze`/`UnFreeze` EngineDll exports, listed in GameDll imports) and drive its transform from host snapshots the way animals are driven. The seated client hunter follows its seat.
- Host-only `DriveTo`: hook `cObjHunter::DriveTo` `0x48e40` and swallow it on clients.
- UNVERIFIED that the seat follows a moved vehicle.

**Test recipe.**
1. `.\run-coop-local.ps1 -Map MAP5 -Capture`.
2. Host: `coop_tp 605.9:2:135.8` (cScrTask06 volume).
3. After cutscene 3 the host rides away. Client screenshots show the client on foot at the camp and the client truck at `(602.7, 1.5, 109.6)`. Client log: `story: applied host fire 'cScrTask06'`, no camera change.

### G5 — Host runs `OnActivate` twice when a client request arrives during the host's own sequence (DESYNC on all maps)
**What happens.**
- `OnFireRequest` treats any nonzero `+0x1fc` as "fired" and zeroes it (story.cpp:360–367). States 2–5 are pending (cutscene running or delay).
- The host runs `OnActivate` now. Later its own `DeactivateHUD` → 3 → 2 → `0x68130` runs `OnActivate` again (hk_TriggerFire host path, story.cpp:460).
- Effects: double `MoveToPos` (respawn, health reset mid-fight), double task starts.

**Fix.** In `OnFireRequest`, when `+0x1fc` is 2–5, record the request and let the host's own fire commit. Force `Fire` only after a timeout (for example 30 s, the MAP12C stuck-cutscene case). Only states 0 and 1 without `g_ran_fire` should fire at once.

**Test recipe.**
1. `.\run-coop-local.ps1 -Map MAP6`.
2. Write `coop_tp 154.1:6:-76.0` (cScrTask02, has a cutscene) into both `game\Bin\dh2coop.cmd` and `game2\Bin\dh2coop.cmd` within 1 s.
3. Host log shows `story: [host] ran 'cScrTask02' … for player 1` **and** a later `story: [host] trigger FIRE id=… name='cScrTask02'` without `(host commit)`: that is `OnActivate` twice.

### G6 — MAP8 water crocodiles are not mirrored (DESYNC)
**What happens.** `cObjCrocodile` vt `0xffa50` is missing from the `Animals()` filter (player_tracker.cpp:329). Each process has its own crocs with live brains: double attacks on the client, client kills not shared.

**Fix.** Add `kVtCrocodileRva = 0xffa50` to the tracker. It has the `cScrAnimal` layout (same ctor `0x18c80`) and brain `+0x200`. Check that its death slot `+0x154` and body-state names work with `PresentDeath`/`SetBodyState`. UNVERIFIED.

**Test recipe.**
1. `.\run-coop-local.ps1 -Map MAP8`.
2. Host log at load: `tracker:   vt GAMEDLL7.DLL+0xffa50 x3` in the vtable histogram.
3. Host: `coop_tp 85.0:-4:143.1` (cScrTask07 croc pool), wait 10 s.
4. Bug: no client `animals: mirror freeze id=` for those three ids.

### G7 — Companions and guide stretches follow only the host (DESYNC)
**Where.** MAP8 tribesmen ×3, MAP9 Reggie (guide at Task07), MAP10 Reggie (guide at Task09) and Wirake.

**Fix.** Already built: npc_follow.cpp (mid hook `0xad098`). Turn on `[Story] NpcFollowNearest=1` and test.

**Test recipe.**
1. `.\run-coop-local.ps1 -Map MAP8` with `NpcFollowNearest=1` in `mod\dh2coop.ini`.
2. Host: `coop_trigger cScrTask08` once.
3. Client: `coop_tp` 40 m away and walk.
4. Expect host `npc_follow: NPC id N follows player 1`. Without the key the tribesmen stay with the host.

### G8 — Spawners in `UpdateTrigger` run only for the host's own presence (DESYNC/gameplay)
**Where.** MAP5 elephant stampede (and `car1` triggers), MAP8 bull stampede (12/13/14/23/24, hunt-area toggles) and croc pool (07).

**What happens.** Update calls `UpdateTrigger` only while the local player is inside (`0x68800`/`0x68740`). A client standing alone in a zone gets no spawns on the host.

**Fix.** Host: hook trigger Update `0x68800`. When an armed trigger with `+0x214` has a living puppet inside the box (`0x68740` with the puppet), run `UpdateTrigger` too, once per frame, the same way `0x68800` does.

**Test recipe.**
1. `.\run-coop-local.ps1 -Map MAP8`.
2. Host: `coop_trigger cScrTask10` once (its `OnActivate` arms 12–14).
3. Client: `coop_tp -21.9:13:110.8` (cScrTask12). Host stays away.
4. Bug: no host `animals: id=<scrubbul> state='…Run…'`.

### G9 — `cObjGuide` deaths fail the mission outside `0x91c20` (RISK on all six maps)
**What happens.** The client fails only if its mirrored health reaches 0, which a late joiner or a missed final snapshot breaks.

**Fix.** Host: hook `cObjNPC::OnDie` (`0x5fde0`). When `GetClassType == "cObjGuide"` on the host, `Commit(kMissionEnd, code 6, "<name> hunt has ended.")`. On clients, swallow their own code-6 write while online, or accept either.

**Test recipe.** User-driven. MAP8 after Task17 (`coop_trigger cScrTask17` once on the host). Let a bull kill a tribesman. Host log has no `events: host mission message code=6`; both windows show MISSION FAILED.

### G10 — Client triggers spin in state 2 and flood the log (COSMETIC, CPU)
**Fix.** In `hk_TriggerFire`, on the client path when `g_ran_fire` already has the id, write `+0x1fc = 1` before returning (story.cpp:469). Rate-limit the FIRE log.

**Test recipe.** Any map late join, for example `.\run-coop-local.ps1 -Map MAP2A -Players 2 -LateJoin 15`. `Select-String -Path game2\Bin\dh2coop.log -Pattern '\[client\] trigger FIRE' | Measure-Object`: thousands today, a handful after the fix.

### G11 — Mirror leg clips missing on some NPC packs (COSMETIC, possible fatal)
**Where.** `indiangirl` (MAP7), `poacher` (MAP5), `ausranger3` (MAP6) lack `idlenormalneutral` and/or the side and back walks used by `NpcLegClip` (animals.cpp:291).

**Fix.** At first freeze, check the clip exists in the model's packs (or fall back `Idle` → `idle`). Skip `SetAnim` when missing.

**Test recipe.**
1. `.\run-coop-local.ps1 -Map MAP7 -Capture`.
2. Host: `coop_tp 158:3:-45` (the girl).
3. Client screenshot pose; `Select-String debug.log 'Could not'`.

### G12 — MAP7 cobra bites only the host (COSMETIC/gameplay)
**Fix.** `[Animals] CobraBitesNearest=1` (animals.cpp:473); test it.

**Test recipe.**
1. `.\run-coop-local.ps1 -Map MAP7`.
2. Host: `coop_trigger cScrTask07`; repeat once if the log says `-> arm` (Task07 is armed by Task12, not at map start).
3. Client: `coop_tp 156.3:2:-41.2` (cobras_t7_01).
4. Host log `animals: cobra id=… bite target -> puppet` only with the key on.

### G13 — Per-process hazards (COSMETIC)
**Where.** Traps (MAP7, the MAP10 "mines"), cages (MAP6 ×22, MAP7 ×9), bees, ice (MAP9), `trigger_hurt` (MAP8/9), fire events. Each player meets their own copy. The other player's puppet never triggers them and never shows "caged" or "trapped".

**Fix.** Only if wanted: `kPropAnim` for the trap and cage visuals.

**Test.** Visual.

### G14 — `SetDenySATime` is local (COSMETIC/gameplay, MAP8)
**Fix.** Host: skip `kPlayerKnock` to a puppet while the host's own deny timer runs.

**Test.** MAP8 Task17 window.

### G15 — Puppet weapon models (COSMETIC)
**What happens.** `tranqgun1`, `blowdarts`, `knife`, `macheta`, `mbranch` → kind 4 (no model). `dbrifle1` → rifle1 mesh with the flame bone visible (player_tracker.cpp:147–156).

**Test.** Visual.

---

## Corrections to earlier notes
- `story-objectives.md` "Ten `cScrMine` on MAP10": the declarations are inside a `/* */` comment, and `AddEvent(EVENT_MINE)` is commented out. The "mines" are `EVENT_TRAP` foot traps moved to `mine_*` spawn points. VERIFIED (`LEVELS/MAP10/WORLD.SCR`).
- MAP5 is a vehicle map. Cutscene 3 runs `GetInCar`/`DriveTo`, and triggers 7–20 are `ActivateBy("car1")`. Only the Task06 copies are commented out. VERIFIED.
- MAP7 girl: BSP `obj_girl`, `cScrGirl`, `girl.spj`; C++ `cObjGirl`. VERIFIED.
- The MAP7 cobra is mirrored now (animals.cpp `IsCobra`). The open part is the bite target.
- MAP8 has two crocodile classes (0.5).
- `story-inventory.md` counts `cScrMine×10` for MAP10 from commented code.
- Cutscenes are not "started by a fire" when they live in `BeginCutScene` (0.1).

## Open questions
- Does `CharacterBrainFreeze` keep NPC tasks from running when a replayed `StartTask`/`MoveToPos` hits a mirrored NPC? MAP2A suggests the cutscene `WaitForTask`s still finish.
- `ForceBrainActivate` (C `0x1f610` → brain vt `+0x68`): does it clear `+0x79`?
- How `ActivateTriggerOnEnd` fires its trigger (MAP7 08/10, MAP10 17). Each process does it itself; the timing should match because the deaths come from the host.
- Rhino flee-point kill: does the client's own flee test also fire on the mirrored position?
- `cObjIce` crack condition; cage `+0x214` assignment (event manager on the local player?).
- Missing clip via `entity::SetAnim`: bind pose or fatal?

## Proposed MEMORY facts
- GameDll `cObjTriggerEvent` (vt `0x10afa0`) state `+0x1fc`: 4 = run `BeginCutScene` (`+0x210`, once), 5 = cutscene up (`ActivateHUD` `0x6b8b0`), 3 = delay (`ActivateAfter` `0x694c0`, `DeactivateHUD` `0x69970`, ForceEndCutScene `0x68da0`), 2 = fire `0x68130` with the local player, 1 = done. Setter `0x68d70` turns any requested state into 4 while `BeginCutScene` exists. Evidence: `re/notes/auditb-trigger-*.c`, listing `0x68800–0x689d6`.
- `cObjTriggerEvent::OnHit` `0x6b140` (vt `+0x8c`) is collision-driven: armed `+0x1e4`, `WaitTask` bits `+0x200`, activator name `+0x198 == +0x224`, state `!= 1`, `+0x1e8 == 0` → disarm `0x68fd0`, run script `OnHit` (`+0x208`), `+0x1e8 = 1`. Replaying `0x68130` alone never runs `OnHit` or `BeginCutScene`. Evidence: `auditb-trigger-update-onhit.c`.
- Trigger Update `0x68800` calls the script `UpdateTrigger` (`+0x214`) every frame while armed and the local player (`0x6c30`) is inside the AABB (`0x68740`). Evidence: same.
- Script `MoveToPos` on an animal (`0x161a0`) = vt `+0x158` = `0x14610` respawn: health to `+0x4c0`, tranq reset, active `+0x4b0 = 1`, brain `+0x79 = 0` (unfreezes). `Kill()` = vt `+0x234` = `0x1be20`: park at -10000, `+0x4b0 = 0`, brain `+0x79 = 1`. `CanBeRespawned` = `!+0x4b0`. Evidence: `auditb-animal-respawn.c`, `auditb-animal-kill-moveto.c`.
- `AddEvent` (`0x82f30`) creates its pools at map load: 10 `obj_trap` `trap1..10`, 10 `obj_cage`, 10 `obj_mine`, 5 `obj_bees`, 5 `obj_fire_event`, one `obj_animal` `animal1` (given species), one each of snake/birds/bats/cavedust. Evidence: `auditb-addevent.c`.
- Species class table `0x163dd8`: all `obj_animal` except `0x13` `obj_cobra` and `0x1f` `ANIMAL_WATERCROCODILE` `obj_crocodile`. `cObjCrocodile` vtable `0xffa50` (ctor `0x1d720` = `cScrAnimal` ctor + own vtable). Evidence: `auditb-species-table.txt`, `auditb-croc-girl-ctor.c`.
- `cScrAnimal::OnHit` `0x1a680`: a tranq bullet sets `+0x4f0 = 2` and damages by `+0x4f8`, which is 0 unless `SetTranqHurt` (respawn writes 0). A second dart into a protected animal (`+0x604`) fails through `0x965e0`. Evidence: `auditb-animal-onhit-tranq.c`, `auditb-animal-respawn.c`.
- `cObjTrap::OnCollision` `0x85d40` only reacts to `IsEntityKindOf("obj_player")`. `cObjNPC` victims get `CloseGuide`. Evidence: `auditb-trap-cage-car.c`.
- MAP10 has no mines: `cScrMine` and `EVENT_MINE` are commented out; the `mine_*` spawn points receive `EVENT_TRAP` traps. MAP5's `car1` is `obj_mod_vehicle` (`cMOD_ObjVehicle`). Boarding is in `CUTSCENE/MAP5/CUTSCENE3.SCR` (`GetInCar`, `DriveTo("PathNode12")`); `cMOD_ObjVehicle::OnCollision` `0x3b8c0` calls trigger `OnHit`. Evidence: `LEVELS/MAP10/WORLD.SCR`, BSP lump, `auditb-trap-cage-car.c`.
- `cObjGirl` (MAP7 `indie_girl`) = `cObjNPC` ctor `0x62720` + vtable `0x108d38`, `+0xb64 = 3`. Evidence: `auditb-croc-girl-ctor.c`, `auditb-ents-MAP7.txt`.
- No exit tokens on MAP5–MAP10; each ends with `CheckMissionStatus`. Evidence: trigger scripts; BSP `trigger_level` only on MAP7 (unreferenced, target `map8a`).
