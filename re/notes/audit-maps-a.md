# Map audit A — MAP2B, MAP2C, MAP3A, MAP3B, MAP3C, MAP4A, MAP4B (2026-10-17)

Job `audit-maps-a` (research, read-only on code, Ghidra slot A). Goal: carry the MAP1/MAP2A co-op knowledge (`re/notes/coop-checklist.md`) to the next seven story maps, list every mechanic they use, and say for each whether `mod/src/coop/*` covers it.

**Method.** Read every trigger, mission, world and cutscene script of the seven maps (`re/extracted/SCRIPTS/...`), parsed the BSP entity lumps (plain Lua text after the first `entity("`; scratch parser, nothing committed), listed the teen-body sync-attack packs (`re/scripts/ap_list.py`), and decompiled the GameDll7/CommonDll7 functions the scripts reach (slot A, `DecompileAt` / `DecompileByName` / `DataRefs` / `XrefStrings`; RVAs are module-relative). No game launch. **VERIFIED** = seen in code, data or decompile. **UNVERIFIED** = inferred.

Code references are `file:function (line)` in `mod/src/coop/` at the time of the audit.

---

## 0. Engine findings from this audit (apply to every map)

These are new or sharper than the checklist. Most gaps in section 8 come from them.

| # | Finding | Evidence | V/U |
|---|---|---|---|
| F1 | **Trigger `OnHit` runs only on the process whose own activator entered.** `cObjTriggerEvent` vt `+0x8c` = GameDll `0x6b140`. It needs the trigger armed (`+0x1e4`), every `WaitTask` bit satisfied, and the activator's target name (`+0x198`) equal to the `ActivateBy` name (`+0x224`, e.g. `"hunter"`, `"car1"`); then it runs the script `OnHit` (function index `+0x208`) once (`+0x1e8 = 1`) and only while not yet fired (`+0x1fc != 1`). The mod hooks arm `0x68eb0` and fire `0x68130` only. The fire runs the script function at `+0x20c` (the `OnActivate` entry; `BeginCutScene` cutscenes still play from a replayed fire, per the checklist) and never `OnHit`. Whatever a script puts in `OnHit` (`UpdateMissionStatus`, `DisableStimulus`, stealth, hunt-area points) happens on one process only, and after the fire it can never happen on the others. | decompile `0x6b140`; `story.cpp` `Fire` (l.340), `OnFireRequest` (l.348) | VERIFIED |
| F2 | **`WaitTask(n)` gates `OnHit` on the local mission:** `FUN_100690e0` sets bit `n` in trigger `+0x200`; `0x6b140` returns early unless task `n` is complete (`FUN_100956f0`: task `+0x5c != 0 && +0x68 == 5`, or `+0x68 == 6`). A host fire applied for a client's request skips this gate on the host. | decompile `0x6ac90`, `0x690e0`, `0x6b140`, `0x956f0` | VERIFIED |
| F3 | **`MoveToPos` on an animal is a respawn.** Script `MoveToPos` (`FUN_100161a0`) calls animal vt `+0x158(3,0)` = `FUN_10014610`: health `+0x19C` := `+0x4c0` (spawn health), dead/hidden props bits cleared (`+0xC0 &= 0xfb7f`), body state `IdleAttack`, `+0x4b0 = 1`, and **brain `+0x79 = 0` (the `FreezeBrain` byte)**, then `SetPos` to the waypoint. | decompile `0x161a0`, `0x14610` | VERIFIED |
| F4 | **`Kill()` parks, it does not kill.** Animal vt `+0x234` = `FUN_1001be20`: `SetPos(-10000,-10000,-10000)`, brain `+0x79 = 1`, `+0x4b0 = 0`, `RemoveDeadAnimal`. Health is untouched. The same function runs after a dead animal's fade (`cScrAnimal::Update` `0x1f3c0`, `re/notes/animal-park.md`). | decompile `0x1be20` | VERIFIED |
| F5 | **`CanBeRespawned()` = animal `+0x4b0 == 0`** (`FUN_100164b0` → vt `+0xdc` = `FUN_100194d0`). True after a park or after a corpse fade. Scripts that recycle animals: `CanBeRespawned` → `MoveToPos` → `StartTask`. | decompile `0x164b0`, `0x194d0` | VERIFIED |
| F6 | **Level exits ignore the BSP map name.** `trigger_level` = `cObjTriggerLevel` (vtable GameDll `0x10bab0`, size `0x1ec`). `+0x188` = `0x6c420` (walk-in, needs `+0x1e8` armed), `+0x18c` = `0x6c610` (`ActivateTriggerLevel`: immediate for the local player from `FUN_10006c30`). Both `strtok` the `target` property (`"map3b,human2"`), **discard the first token**, store the second (spawn point) at cMODApp `+0xbc` (`FUN_100067b0`), call `SetMapIsComplete` on the activator's profile, and write message `0x87` code **7** "Checkpoint reached" straight into the stream `DAT_10185ea8`, then `FUN_10050760(hunter, 2.0f)`. Not through `0x91c20`. Which map loads next is decided elsewhere (UNVERIFIED: profile progress; fits the 2026-10-16 run where MAP3B `exit_1` loaded MAP2A while the BSP says `map3c`). `LatentActivateTriggerLevel` (`0x6a610`) only arms (`+0x1e8 = 1`). | decompile `0x6a4d0`, `0x6a610`, `0x6c420`, `0x6c610`, vtable dump `0x10bab0` | VERIFIED (next-map rule UNVERIFIED) |
| F7 | **Four GameDll functions write the end message without `0x91c20`:** `0x5fde0` `cObjNPC::OnDie` (guide, code 6), `0x6c420` / `0x6c610` (level exit, code 7), `0x96750` (re-emits 5/6 from mission status; referenced from four vtables at `0x111b74`, `0x111c2c`, `0x111cb8`, `0x111d28`, caller unknown). | `DataRefs` on `0x185ea8`; decompile `0x96750` | VERIFIED |
| F8 | **World items act for the local player only.** `cObjWorldItem` use callback `FUN_100432c0`: ammo → `AddExtraAmmo`, medikit → profile attr 9, `radio`/`bino` special, else `BuyItem`, all on the user's profile; then the `next trigger` property is **armed** through `FUN_10092210` → `0x68eb0` (hooked). Update `FUN_100430c0`: "use action 2" items are used by `FUN_10006c30` (local player) inside the item's box. | decompile `0x432c0`, `0x430c0`, `0x92210`, `0x3ae60`, `0x3ae80` | VERIFIED |
| F9 | **Mission task end hits the trigger with the mission's hunter.** `FUN_10098a50` / `FUN_10096da0`: end trigger is a `trigger_event` → vt `+0x8c(mission+0x258, 0, 0)` (= F1 `OnHit`; the force variant first stores the name at mission `+0x25c`); a `trigger_level` → `+0x1e8 = 1` and `+0x188(hunter)`. Each process's mission does this for its own count. | decompile `0x98a50`, `0x96da0`, `0x927f0`, `0x92a30`, `0x91ac0` | VERIFIED |
| F10 | **`player.SetCanBeAttacked(n)`** = `FUN_10049ec0`: hunter `+0x644 = n`, then `cAIHuntAnimalInterface::AddAttackTarget` / `RemoveAttackTarget` on `DAT_101657f8` for that hunter only. Puppets stay in the list. | decompile `0x4bf80`, `0x49ec0` | VERIFIED |
| F11 | **Species → entity class table** at GameDll `0x163dd8` (index = `ANIMAL_*` order in `SCRIPTER.EXP`, 0-based): every species is `obj_animal` (`cObjAnimal`, size `0x71c`, the "cScrAnimal" vtable `0xff470`) except index 19 `ANIMAL_COBRA` → `obj_cobra` and index 31 `ANIMAL_WATERCROCODILE` → `obj_crocodile` (`cObjCrocodile`, size `0x72c`, its own vtable). The tracker mirrors only vtables `0xff470` and `0x10e920`. | `DumpPtrs` `0x163dd0`, `0x160f20`; `player_tracker.cpp` `Scan` (l.329) | VERIFIED (cobra index cross-checks the order; croc vtable value not read) |
| F12 | **Vehicles drive themselves.** `cObjVehicle::Update` (CommonDll `0x5a9c0`) calls `PhysDrive` while `+0x33c & 8`; `DriveTo` (`0x59b00`) sets that bit and the next waypoint. `GetIn` (`0x5ab60`) `cObjEntity::Attach`es the character to tag `place_<seat>`. Script `GetInCar` (`FUN_10048d10`) seats the calling character in the cMODApp vehicle (`DAT_101657f0+0x104`); script `DriveTo` (`FUN_10048e40`) drives the first `cMOD_ObjVehicle`. No driver brain needed. | decompiles above | VERIFIED |
| F13 | **Entity ids = BSP entity index**; `AddAnimal` animals follow the last BSP entity in `AddAnimalOnMap` order. Cross-check: MAP3B BSP has 47 entities (triggers 39–45 = runtime ids 39–45; leopards 47/48, cobras 49–51 in `STATUS.md`). Other maps may shift if runtime entities (NPC weapons) are created before `AddAnimalOnMap`. | BSP parse vs `STATUS.md` | VERIFIED for MAP3B, UNVERIFIED elsewhere |
| F14 | **`ForceBrainActivate(x)`** → brain vt `+0x68` = `cAIHuntBrain_Animal::ForceActivate` (CommonDll `0x83ce0`): flag word `+0xa0`, `+0x84 |= 0x4000`. It does not touch the freeze byte `+0x79`. | decompile | VERIFIED |
| F15 | **Girl and fiancée NPC packs lack the guide clip set.** `objects/npc/teengirl` and `nephew` `GAME.AP` have 15 clips (`idle`, `walk/forward`, `run/forward`, `conversation`, `point`, turns…; no `idlenormalneutral`, no side/back walks, no `fall`/`liedead`). `fiance/GAME.AP` has 13 (`defaultpose`, `cover`, `coveridle`, `hit`, `scare`, `hurtdown`, sync-attack clips; no idle or walk). `animals.cpp` `NpcLegClip` (l.291) maps `Idle` → `idlenormalneutral`. | `ap_list.py`; `GIRLLEGS.SCR` plays `idle` | VERIFIED |
| F16 | No `SpawnEntity` in any of the seven maps: every animal comes from `AddAnimal` at map load (deterministic ids). | script scan | VERIFIED |

Interplay to keep in mind before fixing anything: F3 means every replayed `MoveToPos` on a client **thaws** that mirrored animal's brain. That is a desync (gap G5), but it is also why most client cutscenes that `WaitForTask` an animal task (MAP2B cs3, MAP2C cs4, MAP4B cs5–7) can finish today: `BeginCutScene` `MoveToPos`es the actor first. MAP12C's yeti hung because nothing thawed it (`STATUS.md` 22:53). A naive re-freeze would turn G5 into G3.

---

## 1. MAP2B (Hugh, wolves, grizzly, cougar, fire siege)

BSP 171 entities: 25 `trigger_event`, 1 `trigger_level`, `obj_guide` hugs2b (`objects/npc/oldhugh`, `cScrHugs2b`), 7 `obj_aifx` (`cScrFx` fire/smoke/sparkles), 5 `obj_target` stumps, 3 `obj_world_item` (handgun5 ammo, rifle1 ammo, medkit), 4 `trigger_hurt` (100), 11 `info_static_event` (birds), `obj_camera` c1cam1. Animals: wolf1–6 (`ANIMAL_GWOLF`, only 1–3 scripted), mlion1, gbear. Hunter gear: `handgun5_1st`, Hugh's rifle via `TakeItem`. Traps trap1–9.

Mission (`MISSION02B`): 1 reach, 2 kill 1 wolf, 3 kill 1 grizzly (`ForceActivateTriggerOnEnd cScrTask04`), 4 reach, 5 kill 1 cougar, 6 kill **10** wolves (`ForceActivateTriggerOnEnd cScrTask13`), 7 reach, 8 reach. Flee action `MISSION_ACTION_KILL` on 2/3/5/6.

| Mechanic | Where | Covered by / risk / gap | V/U |
|---|---|---|---|
| Arm/fire chain (01→02→03…→18) | `TRIGGERMISSION02B` | `story.cpp` `hk_TriggerEvent` (l.478), `hk_TriggerFire` (l.446) | VERIFIED |
| `rand()%2` / `rand()%3` waypoint picks inside `OnActivate` and `SpawnWolves` | cScrTask01/02/07/08/22/24/25 | Each process picks its own; harmless for animals (host position mirrored), so OK | VERIFIED |
| Reach tasks completed in **`OnHit`** (`UpdateMissionStatus REACHED 1` cScrTask03, `REACHED 7` cScrTask16) | F1 | **GAP G1** (host mission misses it when a client enters first; HUD only here: map ends by exit) | VERIFIED |
| `WaitTask(2)` cScrTask03, `WaitTask(5)` cScrTask08 | F2 | Gate on each process's own kill count; host bypasses on a client request | VERIFIED |
| Mission-only triggers cScrTask04/13 (task-end hit, brushes at x≈-267 m with 09/10/11) | F9 | Host mission hits → fire → commit; client mission hits → request (dup) | VERIFIED |
| `Kill()` + reuse of wolves 1–3 (BeginCutScene cScrTask03, cs3 end) | F4 | Park → mirror `kParked` → OK (`animals.cpp` `HostTick` l.658) | VERIFIED |
| **10-wolf siege: wolves 1–3 recycled by `CanBeRespawned`/`MoveToPos` up to 10×** (cScrTask08 `SpawnWolves`, cScrTask19/20/21) | F3, F5 | **GAP G4**: after a wolf's first real death the client keeps it dead forever (`dead_`, `death_presented`; host `announced_` once). Client can't see or shoot respawned wolves | VERIFIED |
| Same replays thaw client wolf brains (`MoveToPos` + `StartTask AttackDogs target hunter/hugs2b`) | F3 | **GAP G5** (client-local AI) | VERIFIED mechanism |
| `player.SetCanBeAttacked(0)` during the siege, `(1)` in state `NoChange` | cScrTask08 | **GAP G8**: only the host leaves the attack list; client puppets stay targets | VERIFIED |
| Client's own siege state machine counts `nWolvesCounter`/`CanBeRespawned` on mirrors | cScrTask08 Update | Diverges; may never reach `NoChange` on a client (its own `SetCanBeAttacked(1)` irrelevant). Cosmetic | UNVERIFIED |
| Hugh protected (`AddAIToBeProtected` 01, removed 03), health via `SetAttribute` 50000/40000/10000 | | `animals.cpp` profile health mirror (l.678, l.1028), client `cObjNPC::OnHurt` blocked (`hk_NpcHurt` l.131) | VERIFIED |
| Hugh is `obj_guide` → death writes fail code 6 itself | F7 | Known bypass (checklist). **G12** | VERIFIED |
| Hugh downed: `AITaskNPC_PlayAnim` `hurtsit`/`hurtdown`, `SetPhysNPC(0)`, later `syncguidegetup` (cScrTask04 Activate state, cScrTask14) | | Mirror copies body-state name + `NpcLegClip`; no mapping for these anims → client pose likely wrong. The client's state also blocks at its first `WaitForTask(hugs2b, …)` (frozen NPC) | UNVERIFIED (cosmetic) |
| Hugh path-lead `AITaskNPC_MoveToPos` start1/fire_hugh/start2 | | Position mirror | VERIFIED |
| Hugh weapons `handgun3,map2b` + `rifle1,map2b`, weapon swaps | `EquipAI` | Not mirrored (`npc-arms` job) | VERIFIED |
| Cutscene 1 (cScrTask03): bear `AIHuntTask_PlaySA` `cs03` partner hugs2b, `WaitForTask` | | `story.cpp` `hk_PlaySA` (l.552) → `kPlaySA`; client thaws an animal in `CutScene` (`animals.cpp` l.944) | VERIFIED |
| Cutscene 2 (cScrTask04): Hugh `cs04`, `WaitForTask(hugs2b PlayAnim)`, **`player.TakeItem("rifle1_1st")`** (also in Skip/End), `player.MoveToPos("c4_2")` | | Rifle reaches the client only if its cutscene completes or skips. NPC `WaitForTask` has the MAP1/MAP2A precedent (works) | UNVERIFIED |
| Cutscene 3 (cScrTask08): wolves `GoToEntity` + 2× `WaitForTask` | | Works only because `BeginCutScene` `MoveToPos` thawed them (F3). **G3** if G5 is fixed naively | VERIFIED mechanism |
| Fire FX `SetCountDownTime(400)` / `StartCountDown` / `RestoreLight` (`cScrFx` `+0x200`/`+0x1fc`) | cScrTask07, 14 | Runs from the replayed fire on each process; FX only | VERIFIED |
| Foot traps trap1–9 `MoveToPos("foottrapN")`; trap body `Close`/`CloseGuide` → `HurtHuman(20)`, `SetAnim trapclose/synctrap` | `TRAP.SCR` | Per process; trap `SetAnim` is committed as `kPropAnim` (`hk_SetAnim` l.581). Host trap on a puppet → `ForwardPuppetHurt` → client hurt again. **G16** | UNVERIFIED (puppet part) |
| `trigger_hurt` ×4 (100) | BSP | Local; same puppet-forward risk. **G16** | UNVERIFIED |
| Hunt-area flee points `escapemlion0–6` (`(De)ActivateAIHuntAreaPoint` in `OnActivate`) | | Replayed; host sim decides | VERIFIED |
| `CustomProps("Make_Scared"/"Make_Killer")` | | Host brains | VERIFIED |
| Sync attacks: `pl_gw_syncattack1`, `pl_ml_syncattack1` in `TEENM/BODY/MAP2B.AP` | | `combat.cpp` knock forward (l.108); table row `MAP2B` (l.268) | VERIFIED |
| Pickups (ammo, medkit) | BSP | Per process (each player gets their own) | VERIFIED |
| Exit: cScrTask18 `ActivateTriggerLevel("exit_1")`, BSP target **`map2b,human`** (token ignored, F6) | | Runs on host and, via the replayed fire, on each client (own `SetMapIsComplete`, own code 7, own next map). Clients then follow `kMapStart` (`events.cpp` `OnMapStartCommit` l.242). **G2** | VERIFIED |

## 2. MAP2C (Hugh, Erin, Danny, boar claim, grizzly)

BSP 98: 8 `trigger_event` (cScrTask01–05, 07, 08, 09), no `trigger_level`, `obj_guide` hugs2c (oldhugh), 2 `obj_girl` erin (`teengirl`, `cScrErin`) and danny (`nephew`, `cScrDanny`), `obj_world_item` **cave_meat** (`next trigger=cScrTask08`, use action 2, no model) and a medkit, 5 stumps, 2 `trigger_hurt` (100000), 7 `obj_aifx` (waterfall dust), `obj_sunray`. Animals: wolf1, wolf2, gbear, wildboar1. Hunter: `handgun5_1st`, `rifle1_1st`, Hugh's shotgun via `TakeItem`.

Mission: 1 reach, 2 kill 2 wolves (script `CompleteTask(2)` early in BeginCutScene 03), 3 reach, 4 kill 1 boar (`ForceActivateTriggerOnEnd cScrTask07`, a trigger with no script class), 5 reach, 6 reach, 7 kill 1 grizzly (`ActivateTriggerOnEnd cScrTask09`). End: cScrTask09 `CompleteTask(1..7)` + `CheckMissionStatus`.

| Mechanic | Where | Covered by / risk / gap | V/U |
|---|---|---|---|
| Arm/fire chain | | `story.cpp` | VERIFIED |
| `OnHit` side effects: `REACHED 1` (03), `REACHED 3` (04), `REACHED 6` (08), `hugs2c.DisableStimulus()` + `player.DisableStimulus()` (05) | F1 | **G1** (HUD and host stimulus only; the map end force-completes every task) | VERIFIED |
| Hugh protected 01→04, wolf2 targets hugs2c | | Health mirror; guide death code 6 (**G12**) | VERIFIED |
| Erin/Danny path walk to `end1` (03) | | NPC mirror (model path `/npc/`) | VERIFIED |
| Erin/Danny/Hugh leg clips on clients | F15 | **G13**: `NpcLegClip("Idle")` → `idlenormalneutral` does not exist on teengirl/nephew → bind pose likely | VERIFIED clip absence, look UNVERIFIED |
| Cutscene 1 (03): Danny `AITaskNPC_PlayAnim cs06` with `play_sa` = erin, Hugh `cs06`, two NPC `WaitForTask`, gbear `Kill()` (park) | | `kPlaySA`; NPC waits as MAP1/MAP2A | VERIFIED / NPC wait UNVERIFIED |
| Cutscene 2 (04): Hugh `cs07`, **`player.TakeItem("shotgun1_1st")`** after `WaitForTask` (also in Skip) | | Same as MAP2B rifle | UNVERIFIED |
| Cutscene 4 (08): gbear `AIHuntTask_PlayAnim cs08` + `WaitForTask` | | Thawed by `BeginCutScene MoveToPos` (F3) | VERIFIED mechanism |
| Cutscene 3 (`cScrTask06`): not in `MAIN.SPJ`, no trigger | | Dead code | VERIFIED |
| Stealth: `SetStealthLevels`, world `EnableStealth`/`DisableStealth`, `GetDistBetween2Ais(center,"hunter")` loop | 05, WORLD | Local HUD per process; animals sense the host's stimulus only (**G18**) | VERIFIED |
| Boar `SetCanBeClaimed(TRUE)` + `SetScrClaimCallback(cScrTask05,"ClaimCallback")` (`+0x404`, `+0x408/+0x40c`) | 05 | `events.cpp` `hk_ClaimCallback` (l.439) → host runs the claim → `kTrophyClaim` | VERIFIED |
| Claim callback body (`DisableStealth`, `StartTask 5/6`, **`cave_meat.Activate()`**, `setstate(overanddone)`) runs on the host only | | **G9**: client's cScrTask05 stays in `wildboarrespawn` and can respawn its dead boar locally (F3/F5); client's cave_meat stays deactivated (02 deactivated it) so only the host can use it; client stealth HUD stays on | VERIFIED |
| Boar respawn loop `CanBeRespawned` → `MoveToPos boars_t6_0x` (`rand`) | 05 state | Host respawns an unclaimed corpse → client keeps it dead (**G4**) | VERIFIED |
| cave_meat use (use action 2) arms cScrTask08 (F8) | BSP | Through hooked `0x68eb0` → host decides. Only the host can use it today (G9) | VERIFIED |
| Grizzly `AttackBears target hunter` (08) | | `combat.cpp` `RetargetToNearestPlayer` (l.499) | VERIFIED |
| `trigger_hurt` ×2 (100000, falls) | BSP | Local kill zones; puppet-forward double hit (**G16**) | UNVERIFIED |
| Sync attacks: `pl_gw_syncattack1`, `pl_wb_syncattack1` | MAP2C.AP | `combat.cpp` | VERIFIED |
| End: `CheckMissionStatus` (09) | | Client swallows (`hk_CheckMission` l.427); host `kMissionEnd` | VERIFIED |

## 3. MAP3A (Abby guide, find food/water/shelter, hyenas, crocs, leopards, fork)

BSP 95: 16 `trigger_event`, **2 `trigger_level`** (`exit_1` → `map3b,human1`, `exit_2` → `map3b,human2`), `obj_guide` abby (`objects/npc/abby`, `cScrAbby`, no weapon), 5 `obj_world_item` (2 handgun3 ammo, 2 medkits, **Food1** `medikit_food`, `next trigger=cScrTask02`), 5 stumps, 21 `info_static_event` (6 falling rocks `obj_cavedust`, 8 birds, 7 bees `obj_bees`), `obj_camera`. Animals: hyena1–4, leopard1–2, crocodile1–5 (`ANIMAL_WATERCROCODILE`, only 1–2 scripted). Hunter: `macheta_1st`, `handgun3_1st`, knife range 2.0.

Mission: **1 `TASK_FIND` `SetTaskObjToFind(AI_GUIDE_ID, 3)` with `ActivateTriggerOnEnd("cScrTask08")`**, 2/3/4 kill 2 hyenas each, 5 reach, 6 kill 3 hyenas, 7 reach, 8 kill 2 leopards, 9 reach. No `CheckMissionStatus`; ends by exit.

| Mechanic | Where | Covered by / risk / gap | V/U |
|---|---|---|---|
| **Find ×3** = `UpdateMissionStatus(FIND_SOMETHING, AI_GUIDE_ID)` in **`OnHit`** of food (02), water 1/2/3 (04/05/06, mutually `Deactivate`d) and shelter (07); exactly 3 can count | F1, F9 | **GAP G1 — blocks progress**: counts land on whichever mission's hunter entered. If the host gets 2 and a client 1, no mission reaches 3, cScrTask08 is never hit (its BSP position is next to the map-start trigger, ~240 m from the hunter spawn, so only the mission's task-end hit reaches it; brush extent UNVERIFIED), "return to Abby" never starts. The finds cannot be recounted (F1: `OnHit` refuses after the fire) | VERIFIED path, runtime UNVERIFIED |
| Food1 pickup arms cScrTask02 | F8 | Hooked arm → host decides | VERIFIED |
| Water triggers deactivate each other | 04/05/06 | **G6**: a client request for 05 after the host fired 04 still fires 05 on the host (no arm check) | VERIFIED code |
| Abby: `SetAIType(GUIDE|COVERSTAY,"")`, cover `avioncover`, `SetCanBeAttacked(0)`; 12: `SetAIType(COMPANION|COVERSTAY,"hunter")`, cover leocover1/2; cs2: `SetAIType(GUIDE,"a")` | 01, 12, cs2 | Host brain; follows the host. `npc_follow.cpp` (nearest player) built, **off** (`[Story] NpcFollowNearest`) | VERIFIED |
| Abby `obj_guide` | F7 | **G12** (protected by `SetCanBeAttacked(0)`) | VERIFIED |
| Hyenas 1–4 recycled for 9 kills (tasks 2,3,4,6) by `MoveToPos` in 02/03/04/05/06/07/09/11/15 | F3 | **GAP G4**: every hyena killed once stays dead on clients; clients cannot see or shoot the later waves. Plus **G5** on each replay | VERIFIED |
| Crocodiles `AIHuntTask_AttackFloaters target hunter` (10) | F11 | **GAP G7**: `cObjCrocodile` not mirrored → each process runs its own crocs (local AI, local attacks, `pl_cr_syncattackcroc` on the local hunter) | VERIFIED class, runtime UNVERIFIED |
| Leopards (13) `AttackCats target hunter` | | `combat.cpp` retarget | VERIFIED |
| `WaitTask(8)` on cScrTask14 | F2 | Local gate | VERIFIED |
| `OpenMenuPage(8)` (bees hint, 16) | | Replayed on clients by the host's fire: popup on a client that is elsewhere. **G14** | VERIFIED |
| Bees/falling rocks/birds static events | BSP | Per process, local damage | VERIFIED |
| Cutscenes: cs1 (01, map start, Abby `cs01` + NPC `WaitForTask`), cs2 (14, Abby `cs02`), cs3 (15, hyena4 IdleWait 3 s), cs4 (13, leopard1 IdleWait 3 s) | | Fire replay; NPC wait as MAP1 cs1 (same pattern, works) | VERIFIED / UNVERIFIED |
| Sync attacks: `pl_l_*` (4 directions), `pl_hy_syncattack1`, `pl_cr_syncattackcroc` (4 dirs) | MAP3A.AP | `combat.cpp` row `MAP3A` (l.270) | VERIFIED |
| **Fork exits** 14: `LatentActivateTriggerLevel("exit_1")` and `("exit_2")` → both armed; whoever walks in leaves (`0x6c420`, local player) | F6 | **GAP G2**: a client walking into an exit loads MAP3B alone (`events.cpp` "arrived alone" l.257 recovers later); the host is not asked. Both exits go to MAP3B (spawn human1 / human2) | VERIFIED |

## 4. MAP3B (radio tower, cobras, leopards)

BSP 47 (ids VERIFIED, F13): 7 `trigger_event`, `trigger_level exit_1` → `map3c,human`, spawn points `human`, `human1`, `human2`, 4 `obj_world_item` (handgun3 ammo, 2 medkits, **Food1** `next trigger=cScrTask02`), 5 stumps, 12 static events. Animals: leopard1–2 (47/48), cobra1–3 (49–51). No NPC. Played in co-op before (`STATUS.md`).

Mission: 1 reach (`ActivateTriggerOnEnd cScrTask03`), 2 kill 1 leopard.

| Mechanic | Where | Covered by / risk / gap | V/U |
|---|---|---|---|
| Chain 01→02 (via Food1)→03→04→05 | | `story.cpp`; reach in `OnActivate` | VERIFIED |
| Cobras `Task_CobraAttack target hunter` | 01 | `cObjCobra` mirrored (`player_tracker.cpp` l.329, `animals.cpp` cobra path); bites only the host unless `[Animals] CobraBitesNearest=1` (`AimCobras` l.473, default off) | VERIFIED |
| Leopard1 reused 02→03→04 with `MoveToPos` | F3 | **G4** if killed before the last move; **G5** | VERIFIED |
| `OpenMenuPage(9)`, `(10)` | 06, 07 | **G14** | VERIFIED |
| cs1 referenced, no cutscene file in `MAIN.SPJ` | | No-op | VERIFIED |
| Sync attacks `pl_l_*` ×4 | MAP3B.AP | `combat.cpp` | VERIFIED |
| Exit 05 `ActivateTriggerLevel("exit_1")`, BSP `map3c,human`; runtime 2026-10-16 loaded MAP2A | F6 | **G2** (per-profile next map; follow by `kMapStart`) | VERIFIED |

## 5. MAP3C (Abby3c defense, lionesses, lion, helicopter)

BSP 49: 5 `trigger_event`, `trigger_level exit_1` → `map3b,m3ba1` (no script activates it; MAP3B has no spawn `m3ba1`), `obj_guide` abby3c, **`obj_helicopter` heli1** (`cScrHelicopter`), 3 `obj_world_item` (2 medkits, **radio1** `next trigger=cScrTask03`, use action 2), 5 stumps, 6 static events. Animals: lion1–2 (`LIONESSE`), lion3 (`LION`), cobra1–4 (no task). Hunter: macheta, handgun3, `AiCanFindItem bino`.

Mission: 1 reach, 2 kill 2 lionesses (`ActivateTriggerOnEnd cScrTask04`), 3 kill 1 lion (`ActivateTriggerOnEnd cScrTask05`). End: cScrTask05 Activate state `Wait(2)` + `CheckMissionStatus`.

| Mechanic | Where | Covered by / risk / gap | V/U |
|---|---|---|---|
| Radio use arms cScrTask03 | F8 | Hooked arm | VERIFIED |
| **Abby defense:** 02 `player.SetCanBeAttacked(0)` (never re-enabled), `SetMaxAttackerNo(2)`, lions target `abby3c` (health 500, `maxdmgval 20000`), Abby health 30000 | 02 | **G8**: client puppets stay attack targets while the host is out of the list; Abby `obj_guide` death = code 6 (**G12**) | VERIFIED |
| Lions moved again in BeginCutScene 03 (`MoveToPos` + `ForceBrainActivate(0)`) | F3/F14 | **G4** if killed during 02; **G5** | VERIFIED |
| Cutscene 1 (03): Abby `cs03` + NPC `WaitForTask`; lions `GoToEntity` (not waited) | | OK / NPC wait UNVERIFIED | |
| Cutscene 2 (05): Abby `cs04`, **`heli1.SetAnim("m3_rescue")`**, `SetAIType(GUIDE,"a")`, `passenger3c` (no such entity) | | `hk_SetAnim` → `kPropAnim` (also run locally: double apply, harmless) | VERIFIED |
| Cutscene 3 (04): lion3 IdleWait | | OK | VERIFIED |
| Helicopter | | Prop anim only; no flight/shooting on this map | VERIFIED |
| Cobras ×4 untasked | | Mirrored | VERIFIED |
| Sync attacks `pl_ln_syncattack1`, `pl_lns_syncattack1` | MAP3C.AP | `combat.cpp` | VERIFIED |
| End `CheckMissionStatus` | 05 | `kMissionEnd` | VERIFIED |

## 6. MAP4A (truck chase)

BSP 80: 10 `trigger_event` **all `ActivateBy("car1")`** except 01, `trigger_level exit4b` → `map4b,human`, `obj_guide` tleader4a (`triballeader`, driver), **`obj_mod_vehicle` car1** (`resource=truck1`; script type `cMap4Truck extends cScrEntity`, `TRUCK.SCR` state `TurnCoDrive`), no stumps, no pickups. Animals: lion1–6 (lion/lioness). Hunter: **`bow_1st`**, `knife1_1st`.

Mission: 1 reach (sting `track_map4`), 2 reach, 3 kill 1 lion. The exit does not wait for 3.

| Mechanic | Where | Covered by / risk / gap | V/U |
|---|---|---|---|
| BeginCutScene 01: `tleader4a.GetInCar(1)`, `player.GetInCar(2)`, `tleader4a.DriveTo("startcar1")`, `SetMaxAttackerNo(4)` | F12 | Runs on every process (replayed fire): **each process drives its own truck** with its own hunter in seat 2. Trucks diverge with physics (lion bumps); late joiner's truck starts at the start | VERIFIED code, runtime UNVERIFIED |
| Triggers 02–10 hit by **car1** | F1 | Each process's own truck hits its own triggers; host decides via requests | VERIFIED |
| Lions `CustomProps("MakeChaser")`, `AttackChasers target hunter`, lion1/lion2 reused (06, 08) | F3 | **G4**: a lion shot at 02 and reused at 06/08 (the task-3 lion) is invisible on clients; **G5** | VERIFIED |
| tleader4a NPC mirror vs attachment | F12 | Client places him at the host's seat position every frame while he is attached to the client's truck → floats if trucks diverge (**G10**) | UNVERIFIED |
| Client hunter in a vehicle and `PlayerInCutscene()` (view > 4 m from feet), `PumpGather`/catch-up `TeleportToHost` | `story.cpp` l.88, l.427; `events.cpp` l.350 | **G10**: if the seated hunter's `+0xE8` does not follow the seat, every host fire is deferred for the whole ride; a catch-up teleport while seated is untested | UNVERIFIED |
| `NextTrig("cScrTask09")` (08) | `0x68270`, `0x68130` → mission `+0x25c` | Name only | VERIFIED |
| Exit 09 Activate state `Wait(1)` + `ActivateTriggerLevel("exit4b")` | F6 | **G2** | VERIFIED |
| Puppet weapon = bow | | Kind "other" → no model (**G15**) | VERIFIED |
| Cutscenes 1/2: dialogue only, no camera rail | | OK | VERIFIED |
| Sync attacks `pl_ln_*`, `pl_lns_*` | MAP4A.AP | `combat.cpp` | VERIFIED |

## 7. MAP4B (fiancée, hippo, crocs, stealth antelope, three lion endings)

BSP 111: 20 `trigger_event`, no `trigger_level`, `obj_guide` fiancee4b (`objects/npc/fiance`, `blowdarts,map4b`), 5 `obj_world_item` (2 bow ammo, 3 medkits), 5 stumps, 48 static events (falling rocks, bees, birds). Animals: lion1–5, hippo (`ANIMAL_HIPO`), crocodile1–5 (water), wtail1–4 (`ANIMAL_PANTELOPE`). Hunter: bow, knife.

Mission: 1 reach, 2 bonus kill 4 antelope (flee `KILL`), 3 kill 2 lionesses (`ActivateTriggerOnEnd cScrTask19`), 4 kill 1 lion (`ActivateTriggerOnEnd cScrTask20`). End: 20 `CompleteTask(1..4)` + `CheckMissionStatus`.

| Mechanic | Where | Covered by / risk / gap | V/U |
|---|---|---|---|
| **`OnHit` side effects**: 08 `player.DisableStimulus()`, `ActivateAIHuntAreaPoint("hypo1")`, `hippo time 0.1`, stealth on; 10 `ActivateAIHuntAreaPoint("white1")`, `Deactivate…("hypo1")`, stealth on | F1 | **G1**: when a client enters first, the host never toggles the hunt-area points that its animals flee to | VERIFIED |
| **Alternate endings** 15/16/17 (each `Deactivate`s the other two, runs cutscene 5/6/7); also 02↔04, 06↔07 (hippo) | | **G6**: two players on two approach paths can make the host run two endings | VERIFIED code |
| Cutscenes 5/6/7: lions 2/3 `AIHuntTask_PlayAnim` + fiancée `AITaskNPC_PlayAnim`, 3× `WaitForTask` | | Lions thawed by `BeginCutScene MoveToPos` (F3) | VERIFIED mechanism |
| Cutscenes 1/2 (06/07): hippo IdleWait | | OK | VERIFIED |
| `Kill()` + `MoveToPos` reuse of lion1/4/5 and wtail1–4 (11, 13, 15–17, 19) | F3/F4 | Parks are fine; a lion killed earlier and moved back for task 4 is invisible on clients (**G4**) | VERIFIED |
| Crocodiles ×5 | F11 | **G7** | VERIFIED class |
| Fiancée idle loop `ConfigTaskCmd`/`PlayAnim "idle"`, `SetCanBeAttacked(0)`, `obj_guide` | | Mirror; clip set mismatch **G13** (no `idlenormalneutral`, no walk); death code 6 (**G12**) | VERIFIED |
| Stealth/antelope scare vs host stimulus only | | **G18** | VERIFIED |
| Sync attacks `pl_h_*` (hippo ×4), `pl_cr_*` ×4, `pl_ln_*`, `pl_lns_*` | MAP4B.AP | `combat.cpp` row `MAP4B` (l.274) | VERIFIED |
| End `CheckMissionStatus` | 20 | `kMissionEnd` | VERIFIED |

## 7b. `"hunter"` and local-player reads (only the host counts today)

| Read | Where | Effect in co-op | V/U |
|---|---|---|---|
| `ActivateBy("hunter")` name compare | `0x6b140` (F1) | `OnHit` only for the local hunter; puppets never | VERIFIED |
| `ActivateBy("car1")` | MAP4A | Each process's own truck | VERIFIED |
| `SetTaskParam("target","hunter")` (wolves, bears, cougar, hyenas, leopards, crocs, lions, hippo, cobras) | all maps | Host hunter; `combat.cpp` retarget switches to the nearest player only for animals already chasing a player | VERIFIED |
| `SetTaskParam("target","hugs2b"/"hugs2c"/"abby3c")` | MAP2B/2C/3C | NPC targets; never retargeted | VERIFIED |
| `SetAIType(AITYPE_COMPANION…,"hunter")` | MAP3A 12 | Host only; `npc_follow.cpp` off | VERIFIED |
| `GetDistBetween2Ais(center,"hunter")` | MAP2C, MAP4B world stealth loops | Local HUD only | VERIFIED |
| `FUN_10006c30` local player | `ActivateTriggerLevel` `0x6a4d0`, world-item use `0x430c0` | Local | VERIFIED |
| Mission hunter `+0x258` | task-end hits `0x98a50` | Local | VERIFIED |
| `FUN_10014610` mode 1 finds `"hunter"` | animal respawn | Not used by these scripts (mode 3) | VERIFIED |
| `player.*` (`SetCanBeAttacked`, `DisableStimulus`, `SetStealthLevels`, `TakeItem`, `GetInCar`, `MoveToPos`, `StartTask`) | all | Local hunter of each process (G8 for attack list) | VERIFIED |

## 7c. Exits

| Map | Token(s) armed by | BSP `target` | Note |
|---|---|---|---|
| MAP2A (ref) | `exit_1` | `map2b,human` | |
| MAP2B | cScrTask18 `ActivateTriggerLevel exit_1` | `map2b,human` | map token = itself, ignored (F6) |
| MAP2C | none | — | `CheckMissionStatus` |
| MAP3A | cScrTask14 `LatentActivateTriggerLevel exit_1`, `exit_2` | `map3b,human1`, `map3b,human2` | walk-in by any process |
| MAP3B | cScrTask05 `ActivateTriggerLevel exit_1` | `map3c,human` | runtime loaded MAP2A once |
| MAP3C | none (trigger_level exists) | `map3b,m3ba1` | `CheckMissionStatus` in 05 |
| MAP4A | cScrTask09 `ActivateTriggerLevel exit4b` | `map4b,human` | |
| MAP4B | none | — | `CheckMissionStatus` in 20 |

All VERIFIED from BSP + scripts. Actual destination per exit: UNVERIFIED (see the `smoke-pass` job).

---

## 8. Gap list (ranked)

Severity: **blocks** = the story cannot continue for someone; **desync** = host and client disagree; **cosmetic**.

### G1 — Trigger `OnHit` effects are local-only (blocks: MAP3A find task)
- Maps: MAP3A (find ×3 → only way to cScrTask08), MAP2B (REACHED 1/7), MAP2C (REACHED 1/3/6, `DisableStimulus`), MAP4B (hunt-area points, stealth, `DisableStimulus`). VERIFIED path.
- Fix: new event kind `kTriggerHit`. Hook GameDll7 `0x6b140` (`cObjTriggerEvent` vt `+0x8c`; check prologue `55 8b ec 6a ff` and the `ret` size: the mission passes three args `(activator, pos, dir)`).
  - Client: when the original would run the script (`+0x1e4 != 0`, `+0x1e8 == 0`, `+0x1fc != 1`, activator name = `+0x224`), swallow it and `Request(kTriggerHit, id, name)` (one per 3 s, like `ShouldRequest`).
  - Host: on request, look up the host entity named by the trigger's `+0x224` (`FUN_10006d80` `0x6d80`: `"hunter"` or `"car1"`) and call the original with it. The host's script `OnHit` then updates the host mission and runs `ActivateAfter` → the normal fire commit. Commit `kTriggerHit` after every host `OnHit` (own or requested).
  - Client apply (`[Story] ReplayOnHit=1`): call the original with the local entity of that name under `g_in_apply`, so the client's mission/HUD counts too; its `ActivateAfter` fire is then swallowed by `g_ran_fire` or becomes a duplicate request.
- Test (`.\run-coop-local.ps1 -Map MAP3A -Port <p>`, host `game\Bin\dh2coop.cmd`, client `game2\Bin\dh2coop.cmd`): after the opening cutscene, host `coop_tp -66.1:4.5:72.2` (water 1, cScrTask04), host `coop_tp -91.5:2.5:150.0` (Food1 + cScrTask02); client `coop_tp -22.0:1.5:64.0` (shelter, cScrTask07). Bug: host log has `story: [host] ran 'cScrTask07' … for player 1`, no `CompleteTask 1` on either side, no `trigger FIRE … 'cScrTask08'`. Fixed: host `story: … CompleteTask 1` then `trigger FIRE id=84 name='cScrTask08'`, client `applied host fire 'cScrTask08'`. Grep `CompleteTask`, `cScrTask08`, `kTriggerHit`/`events: commit kind=`.

### G4 — Respawned animals stay dead on clients (desync, high)
- Maps: MAP3A (9 hyena kills from 4 entities), MAP2B (10-wolf siege from 3), MAP2C (boar loop), MAP4A/4B/3B/3C (reused lions/leopards). VERIFIED: `animals.cpp` `HostTick` commits a death once (`announced_`, l.660); client `dead_` + `death_presented` pin hp 0 and skip placement forever (l.919–933, l.1001).
- Fix: host, in `HostTick`: when an id in `announced_` is alive again (hp > 0, not parked), erase it, bump a per-id life counter, `Commit(kAnimalRevive, id, life, hp, std::to_string(life))` (unique journal key; give later deaths the same key scheme). Client: on `kAnimalRevive` erase `dead_`, reset that `Mirror` (keep the buffer), `FreezeBrain(true)`, resume placement.
- Test (`-Map MAP3A`): host `coop_trigger cScrTask04` (hyenas 1/2 to water 1), host `coop_kill <hyena1 id>` (ids from `animals: id=` lines; predicted 95–98 by F13), wait 15 s (fade + park), host `coop_trigger cScrTask09` (moves hyena1 back). Bug: host `animals: sync role=host id=<h1> … hp=` > 0 while client `sync role=client id=<h1> … hp=0.0` and no new `mirror first place`. Fixed: client `animals: revive id=<h1>` then normal `sync` drift.

### G5 — Replayed `MoveToPos` thaws mirrored animal brains (desync; mind G3)
- VERIFIED: `FUN_10014610` writes brain `+0x79 = 0`; `animals.cpp` `ApplyMirror` freezes only when `!m.frozen` (l.965). Every trigger replay that `MoveToPos`es an animal leaves the client copy with live AI (local attacks on the client's hunter on top of the forwarded host attacks).
- Fix: in `ApplyMirror`, for non-NPC frozen mirrors read brain `+0x79`; if 0, re-freeze **unless the local cutscene camera is active** (`PlayerInCutscene()`) or the host state is `CutScene`; log once `animals: brain re-frozen id=…`. Ship together with G3's fallback.
- Test (`-Map MAP3B`): host `coop_trigger cScrTask02`. Client log (with the new line) shows `brain re-frozen id=47` and `id=48`. Today: watch the client hunter for knockdowns with no `combat: knocked down (host animal)` line.

### G3 — Client cutscene hangs on `WaitForTask` of a frozen actor (blocks the client; UNVERIFIED)
- Animal waits currently pass only through G5's thaw. NPC waits (`AITaskNPC_PlayAnim` on Hugh/Abby/fiancée) follow the MAP1/MAP2A pattern that works. While a client hangs, `PlayerInCutscene()` defers every later host fire (`story.cpp` l.403).
- Fix: host hooks `DeactivateCutSceneMode` (GameDll `FUN_1004a7e0`, script native) and commits `kCutsceneEnd(trigger id of the last cutscene fire)`, except when the game auto-skipped inside `OnActivate`. Client: still in a cutscene camera 20 s after that → `WatchSkip(id)` (`0x6b350`). Plus a client watchdog: camera in cutscene for 60 s straight → skip the last applied cutscene trigger.
- Test (`-Map MAP2B`, after G5): host `coop_trigger cScrTask08` (cutscene 3). Bug: client `story: defer host fire` lines keep growing, no `cutscene over` / `PumpGather`. Fixed: `story: skipped cutscene of 'cScrTask08'`.

### G6 — Alternative triggers can both fire on the host (desync)
- `story.cpp` `OnFireRequest` (l.348) fires whenever this trigger has not run on the host; it does not check the host's arm flag. A trigger the host already `Deactivate`d still runs. Maps: MAP4B 15/16/17, 02/04, 06/07; MAP3A 04/05/06; MAP2B 12/13.
- Fix: in `OnFireRequest`, if `!IsActive(ent)` and it has not run here, log `story: fire request '…' rejected: not armed on host` and do not commit.
- Test (`-Map MAP4B`): host `coop_trigger cScrTask15`; within 2 s client `coop_trigger cScrTask16`. Bug: host `story: [host] ran 'cScrTask16' … for player 1` after `cScrTask15`. Fixed: `rejected: not armed`.

### G2 — Level exits are not host-decided (desync)
- VERIFIED F6: replayed `ActivateTriggerLevel` makes every client exit on its own (own `SetMapIsComplete`, own code 7, own next-map choice); MAP3A's latent exits let a client walk out alone. Recovery today is `kMapStart` + "arrived alone".
- Fix: hook `0x6c420` and `0x6c610` on clients: swallow, `Request(kLevelExit, id, name)`; host runs the original with its own hunter. Carry the spawn point with the map start (`kMapStart` key `"MAP3B,human2"`, ≤ 31 chars) and, before `spmap`, write it with `FUN_100067b0(DAT_101657f0, spawn)`; call `cHunterProfile::SetMapIsComplete` on the client profile when following. Coordinate with `mission-flow`.
- Test (`-Map MAP3A`): host `coop_trigger cScrTask14` (arms both exits); client `coop_tp -130.0:12:-11.3` (exit_2). Bug: client `events: journal cleared for map 'MAP3B'` while the host stays on MAP3A. Fixed: host log `cBspTree::Load('Bsp\map3b.bsp')` first, client `following host to 'MAP3B'`.

### G7 — Water crocodiles are not mirrored (desync)
- VERIFIED F11 (`obj_crocodile`, not vtable `0xff470`). MAP3A, MAP4B (and every map with `ANIMAL_WATERCROCODILE`).
- Fix: in `player_tracker.cpp` `Scan`, also accept `cObjEntity::IsClassKindOf(ent,"cObjCrocodile")` (EngineDll export; GameDll uses it in `FUN_1001e170`), or read the vtable from the load histogram (5 entities of a new GameDll vtable on MAP3A/MAP4B) and add it next to the cobra one. Check that `FreezeBrain` accepts its `AIHuntBrain_Floater`.
- Test (`-Map MAP3A`): host `coop_trigger cScrTask10`. Bug: client log has no `animals: mirror freeze` for the croc ids (predicted 101–105) and host `animals: host snapshot` count excludes them. Fixed: five more ids in both.

### G8 — Host-only `SetCanBeAttacked(0)` (desync)
- VERIFIED F10. MAP2B wolf siege (cScrTask08 → `NoChange`), MAP3C Abby fight (02, never re-enabled).
- Fix: host hook on GameDll `0x49ec0` (thiscall(player, int)): when `this` is the local hunter, remove/add every living puppet with the same `cAIHuntAnimalInterface` calls (`DAT_101657f8`) and remember the flag for puppets that join later.
- Test (`-Map MAP3C`): host `coop_trigger cScrTask02`, client walks/`coop_tp` next to Abby (`-27.7:-1.0:58.3`). Bug: host `combat: animal id … hurt player 1`. Fixed: lions stay on `abby3c`.

### G9 — Claim callback side effects not run on clients (desync)
- VERIFIED: `events.cpp` `OnClaimCommit` (l.220) only clears `+0x404`. MAP2C: client cScrTask05 stays in `wildboarrespawn` (ghost respawn via F3), client `cave_meat` stays off, stealth HUD stays.
- Fix (`[Story] ClientClaimScript=1`): on the commit, a client calls the original `0x1cd40` under `g_in_apply` (clears `+0x404`, runs the script callback); its mission end calls stay swallowed (`hk_CheckMission`, `hk_MissionMessage`).
- Test (`-Map MAP2C`): host `coop_trigger cScrTask05`, `coop_kill <wildboar1 id>`; client `coop_claim <id>`. Bug: client later logs `animals: id=<boar> state='IdleAttack'` alive, cave meat not usable by the client. Fixed: no client respawn, client can use cave_meat (its arm becomes a request).

### G10 — MAP4A truck (desync, partly UNVERIFIED)
- Two self-driving trucks (F12). Risks: `PlayerInCutscene()` deferring the whole ride; late joiner's truck starts at the start; `TeleportToHost` while seated; tleader4a mirror vs attach.
- Fix: treat a hunter attached to a vehicle (`+0xA4` parent is a `cMOD_ObjVehicle`) as not in a cutscene and never teleport it; skip the NPC position write while an NPC's parent is a vehicle; later, a host `car1` pose at 5 Hz for late joiners.
- Test (`-Map MAP4A`, `-LateJoin 20` for the second case): grep client `story: defer host fire 'cScrTask0`, `coop: teleport to host`, host `coop: remote 1 … pos=` advancing along the road.

### G12 — Fail/exit paths outside `0x91c20` are not journaled (desync)
- VERIFIED F7: guide death code 6 (all six NPCs on these maps except Erin/Danny are `obj_guide`), exit code 7, `0x96750`. Each process writes its own; a late joiner never learns of it.
- Fix: host hook `0x5fde0` (`cObjNPC::OnDie`) — after the original, if `GetClassType` is `cObjGuide`, `Commit(kMissionEnd, 0, 6, text)`; handle code 7 under G2.
- Test (`-Map MAP2B`): host `coop_trigger cScrTask08`, let the wolves kill Hugh (or reduce his profile health). Grep host `events: host mission message` (absent today) and client `applied host mission end code=6`.

### G13 — Girl/fiancée NPC clips (cosmetic)
- VERIFIED F15. MAP2C Erin/Danny, MAP4B fiancée (also MAP6/MAP7 girls).
- Fix: in `NpcLegClip` callers, skip `SetAnim` for models under `/npc/teengirl/`, `/npc/nephew/`, `/npc/fiance/`, `/npc/indiangirl/` (their body scripts already play their own clips on `SetBodyState`), or look the clip up in the model before calling.
- Test (`-Map MAP2C -Capture`): host `coop_trigger cScrTask03`; screenshot the client at `end1`.

### Lower
- **G14** `OpenMenuPage` hint pages replayed on clients (MAP3A 8, MAP3B 9/10): skip `OpenMenuPage` inside a replayed fire on clients unless the client is within ~30 m of the trigger. Cosmetic.
- **G15** Bow/knife puppets (MAP4A/4B) have no model; NPC weapons (Hugh, fiancée blowdarts) not animated (`npc-arms`). Cosmetic.
- **G16** Host traps and `trigger_hurt` reaching a puppet are forwarded as `kPlayerHurt` on top of the client's own: filter `ForwardPuppetHurt` to attackers that are mirrored animals. Cosmetic/balance.
- **G17** Pickups are per process (each player gets their own ammo/medkit/food copy). Fine for co-op; note only.
- **G18** Scare/stealth/flee perception uses the host's stimulus only (MAP2C boar, MAP4B lions/antelope bonus). Design limit.

---

## 9. Open questions
- Which map each exit really loads with two different profiles (F6). `smoke-pass` job.
- Does `AITaskNPC_*` complete on a `CharacterBrainFreeze`d NPC, or do MAP1/MAP2A NPC waits pass for another reason? Read `cAIBrain_NPC` update for `+0x204 & 0x10`.
- Does a seated hunter's `+0xE8` follow the `place_2` tag (MAP4A)?
- Body-state names during `AITaskNPC_PlayAnim hurtdown`/`syncguidegetup` (MAP2B) and `AIHuntTask_PlayAnim` (MAP2C cs4, MAP4B cs5–7).
- Caller of `FUN_10096750` (vtables `0x111b74`…).
- `cObjCrocodile` vtable RVA and whether its brain accepts `FreezeBrain`.

## Proposed MEMORY facts
- Trigger `OnHit` = `cObjTriggerEvent` vt `+0x8c`, GameDll7 `0x6b140`: needs `+0x1e4` armed, the `WaitTask` mask `+0x200` satisfied (task done), activator name `+0x198` == `ActivateBy` name `+0x224`; runs script fn `+0x208` once (`+0x1e8`) and never after the fire (`+0x1fc == 1`). The fire `0x68130` runs script fn `+0x20c`, not `+0x208`, so a replayed fire never runs `OnHit`. — decompile (re/notes/audit-maps-a.md F1/F2).
- Animal `MoveToPos` = respawn: `FUN_100161a0` → vt `+0x158(3,0)` = `FUN_10014610`: hp := `+0x4c0`, props `&= 0xfb7f`, `IdleAttack`, `+0x4b0 = 1`, brain `+0x79 = 0` (unfreezes). `Kill()` = vt `+0x234` `FUN_1001be20`: park at -10000, brain `+0x79 = 1`, `+0x4b0 = 0`, hp unchanged. `CanBeRespawned()` = `+0x4b0 == 0`. — decompile.
- `cObjTriggerLevel` vtable GameDll7 `0x10bab0`: `+0x188` `0x6c420` (armed walk-in), `+0x18c` `0x6c610` (`ActivateTriggerLevel`, immediate, local player). Both use only the second token of `target` (spawn point → cMODApp `+0xbc`), `SetMapIsComplete`, then write message `0x87` code 7 "Checkpoint reached" directly to `DAT_10185ea8` (not `0x91c20`). — decompile.
- Writers of message `0x87` in GameDll7: `0x91c20`, `0x5fde0` (guide OnDie), `0x6c420`, `0x6c610`, `0x96750`. — `DataRefs` on `0x185ea8`.
- Species → entity class table GameDll7 `0x163dd8` (index = `ANIMAL_*` 0-based): all `obj_animal` (`cObjAnimal`, size `0x71c`, vtable `0xff470`) except `ANIMAL_COBRA` (19) → `obj_cobra` and `ANIMAL_WATERCROCODILE` (31) → `obj_crocodile` (`cObjCrocodile`, size `0x72c`). — `DumpPtrs`.
- `player.SetCanBeAttacked(n)` = `FUN_10049ec0`: hunter `+0x644 = n` and `cAIHuntAnimalInterface::Add/RemoveAttackTarget` for that hunter only. — decompile.
- World items (`cObjWorldItem` use `FUN_100432c0`) act on the local player's profile and arm their `next trigger` through `FUN_10092210` → `0x68eb0`. — decompile.
- Vehicles: `cObjVehicle::Update` (CommonDll7 `0x5a9c0`) drives itself while `+0x33c & 8` (`DriveTo` `0x59b00`); `GetIn` (`0x5ab60`) attaches the character to tag `place_<n>`. — decompile.
- Story BSP entity ids equal the BSP entity index; `AddAnimal` animals follow in `AddAnimalOnMap` order (MAP3B: 47 BSP entities → leopards 47/48, cobras 49–51). — BSP parse vs runtime ids.
- BSP exit targets: MAP2A `exit_1` `map2b,human`; MAP2B `exit_1` `map2b,human`; MAP3A `exit_1` `map3b,human1`, `exit_2` `map3b,human2`; MAP3B `exit_1` `map3c,human`; MAP3C `exit_1` `map3b,m3ba1` (unused); MAP4A `exit4b` `map4b,human`. — BSP entity lumps.
- Teen-body sync-attack clips: MAP2B `pl_gw`, `pl_ml`; MAP2C `pl_gw`, `pl_wb`; MAP3A `pl_l` (4 dirs), `pl_hy`, `pl_cr_syncattackcroc` (4 dirs); MAP3B `pl_l` (4 dirs); MAP3C/MAP4A `pl_ln`, `pl_lns`; MAP4B `pl_h` (4 dirs), `pl_cr` (4 dirs), `pl_ln`, `pl_lns`. — `ap_list.py` on `TEENM/BODY/<MAP>.AP`.
- NPC packs `teengirl`/`nephew` (15 clips) and `fiance` (13 clips) do not contain the guide set (`idlenormalneutral`, side walks, `fall`, `liedead`). — `ap_list.py`.
