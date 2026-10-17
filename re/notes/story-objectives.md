# Story objectives — mechanics, co-op risk, test checklist

## Progress
Done: campaign order, mechanic catalogue, per-map objectives and NPCs, top risks, test checklist, proposed facts. No game launch, no Ghidra. Map-exit *destinations* are BSP data; only MAP3B `exit_1` → `map2a.bsp` is already in the worker log.

Ratings use the current model: the host runs scripts and decides trigger arm/fire; clients replay by entity id and name; animals and `/npc/` characters are host-simulated and mirrored (brains frozen); script `"hunter"` is the local player, so on the host it is only the host.

## 1. Campaign order

`SetMission(n)` is only on the opener of each chapter. Later parts of the same chapter do not call it. No script contains `LoadMap`, `spmap`, a checkpoint, or a save. The next map is `ActivateTriggerLevel` / `LatentActivateTriggerLevel` with an exit token. The BSP entity behind that token picks the map. Destinations below are **UNVERIFIED** except the one runtime line.

| Order | Map | `SetMission` | Exit token in the trigger script | Where it goes |
|---|---|---|---|---|
| 1 | MAP1 | 1 | none; `CheckMissionStatus` | menu / results (C++) |
| 2 | MAP2A | 2 | `cScrTask08` → `exit_1` | UNVERIFIED (name fits MAP2B) |
| 2b | MAP2B | — | `cScrTask18` → `exit_1` | UNVERIFIED (name fits MAP2C) |
| 2c | MAP2C | — | none; completes all tasks, `CheckMissionStatus` | chapter end |
| 3 | MAP3A | 3 | `cScrTask14` arms **both** `exit_1` and `exit_2` | branch. UNVERIFIED which token is 3B vs 3C |
| 3b | MAP3B | — | `cScrTask05` → `exit_1` | **`map2a.bsp`**, host log in `STATUS.md` (23:07). Not re-run here |
| 3c | MAP3C | — | none; `CheckMissionStatus` | chapter end |
| 4 | MAP4A | 4 | `cScrTask09` → `exit4b` (after `NextTrig("cScrTask09")`) | UNVERIFIED; token name is `exit4b` |
| 4b | MAP4B | — | none; `CheckMissionStatus` | chapter end |
| 5 | MAP5 | 5 | none; `CheckMissionStatus` | chapter end |
| 6 | MAP6 | 6 | none; `CheckMissionStatus` | chapter end |
| 7 | MAP7 | 7 | none; `CheckMissionStatus` | chapter end |
| 8 | MAP8 | 8 | none; `CheckMissionStatus` | chapter end |
| 9 | MAP9 | 9 | none; `CheckMissionStatus` | chapter end |
| 10 | MAP10 | 10 | none; `CheckMissionStatus` | chapter end |
| 11 | MAP11A | 11 | `cScrTask08` → `exit_1`; fail path `cScrFailed` | UNVERIFIED |
| 11b | MAP11B | — | `cScrTask09` → `exit_1` | UNVERIFIED |
| 11c | MAP11C | — | none; `CheckMissionStatus`; also has `cScrFailed` | chapter end or a parallel 11 path. Chain 11A→11B→11C is UNVERIFIED |
| 12 | MAP12A | 12 | `cScrTask09` → `exit_1` | UNVERIFIED |
| 12b | MAP12B | — | `cScrTask02` → `exit_2` (not `exit_1`) | UNVERIFIED |
| 12c | MAP12C | — | none; `CheckMissionStatus` after the yeti | chapter end |

Bonus maps are separate missions: MAPARENA 17, MAPISLAND 18, MAPTSAVO 19. One `TASK_RACE`, completed from the arena script. Not in the story chain.

`OpenMenuPage(n)` on MAP1, MAP3A, MAP3B, MAP6 is a page index, not a map load (`re/notes/story-inventory.md` section 7).

The 3A fork is the only place a script arms two exits at once (`TRIGGERMISSION03A.SCR` `cScrTask14`). MAP3B's opening objective is `radio_tower`, which is the objective MAP3A sets on that same trigger, and MAP3C opens on `radio1` then `abby3c`. That is why 3B and 3C read as the two branches. Which exit token is which map is still UNVERIFIED. MAP3B's own `exit_1` loading MAP2A does not match a linear 3B→3C reading; treat that link as what the BSP did in that session.

## 2. Objective mechanics catalogue

| Mechanic | How the scripts do it | Maps | Co-op |
|---|---|---|---|
| Reach a trigger zone | `cScrTriggerEvent`: `ActivateBy("hunter")`, `Activate` / `OnHit`+`ActivateAfter(0)`, then `OnActivate`. Often `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, taskId)` or `player.StartTask` | every story map | **OK.** Host arm (`0x68eb0`) and fire (`0x68130`) replay by id+name. A client walking in sends a request; the host fires. Already played on MAP3B and MAP2A |
| Kill N / a named animal | `CreateTask(TASK_SHOOT)` + `SetTaskAnimalToShoot(species, ANIMAL_KILL, n)`. Scripts also `MoveToPos` + `StartTask("AIHuntTask_…")` + `SetTaskParam("target","hunter")` | every story map except the pure-race steps | **OK** for the kill count if the host's animal dies (mirror presents death). **RISK** for who they chase: `"hunter"` is the host, so aggro and leash use the host's body. A client can shoot the mirror; the host applies damage |
| Flee-point fail | `SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL` or `LOSE_TARGET)` | MAP1 (lose), MAP2A/2B/2C, MAP4B, MAP7, MAP12A, MAP12B | **RISK.** The flee test is host simulation. If it ends the mission through the mission message, clients follow `kMissionEnd`. Whether a flee uses the host hunter only is UNVERIFIED in C++ |
| Find N of a named NPC | `CreateTask(TASK_FIND)` + `SetTaskObjToFind(AI_GUIDE_ID` or `AI_CHILD_ID, count)` and `UpdateMissionStatus(MISSION_ACTION_FIND_SOMETHING, that id)` | MAP1 (guide), MAP3A (guide, five finds), MAP7 (child), MAP8 (guide) | **OK** if the find is a host trigger fire (that is how the scripts advance it). **RISK** if the C++ find task also measures distance to `"hunter"`: only the host counts. Not visible as a distance call in these scripts |
| NPC guides (leads, waits) | `SetAIType(AITYPE_GUIDE…)`, brain task `AITaskNPC_Follow`, `SetFollowDistance(15)` / `SetHelloDistance(15)` in `GUIDE.SCR` | MAP3A Abby (`AITYPE_GUIDE\|AITYPE_COVERSTAY`, empty second arg). MAP9 Reggie and MAP10 Reggie (`AITYPE_GUIDE,"a"`) for a stretch, then switched back to companion | **RISK.** Client brains are frozen, so the lead runs on the host. MAP3A does not pass `"hunter"`; MAP9/MAP10 pass `"a"`. Whether the guide task still measures the local hunter is **UNVERIFIED**, but follow distance lives on `brain_npc`. Expect the NPC to wait for the host and ignore the client |
| NPC follows the player | `SetAIType(AITYPE_COMPANION,"hunter")` or `AITYPE_COMPANION\|AITYPE_COVERSTAY,"hunter"`. Path leads use `StartTask("AITaskNPC_MoveToPos")` + `SetTaskParam("target", node)` and do not read `"hunter"` | Companion string: MAP8 tribesmen, MAP9 Reggie, MAP10 Reggie then Wirake, MAP12A Reggie. Path-lead (no `"hunter"`): Hugh on MAP1–2C, Hamisi MAP5, Reggie MAP6/7/11B, Wirake MAP10/12A | **RISK** for companion: follows the host hunter only; the client can be left behind. Path-lead **OK**: both sides run the same `MoveToPos` from the host fire, and the mirror copies position. MAP2A Hugh is the played case |
| Protect / escort can die | `AddAIToBeProtected(name)`, sometimes `SetCanBeAttacked(0/1)` | MAP1, MAP2A, MAP2B, MAP2C, MAP5, MAP6, MAP7, MAP9, MAP10, MAP11B, MAP12A | **RISK.** Health is copied onto the client mirror, so a host-side death should show on both. A client shot at the NPC is not a separate request (animal hits are). If the engine fails the mission when a protected NPC dies, that should ride `kMissionEnd`; that path is **UNVERIFIED** from scripts alone |
| NPC scripted only | `StartTask("AITaskNPC_PlayAnim")`, `LookToEntity`, cutscene `MoveToPos`, `play_sa` | MAP4B fiancée (idle/cs only), MAP2C Erin and Danny (one walk, then a paired cutscene), MAP11A/11C Siberians (no behavior lines), MAP12C guide (no behavior lines) | **OK** for poses the host script starts. **RISK** if `LookToEntity` target is `"hunter"` (MAP1, MAP6, MAP7, MAP8, MAP9): they look at the host |
| Timed objective | `SetTaskTimeLimit(0)` on every story task in the mission scripts. MAP2B `SetCountDownTime(400)` is on fire/smoke FX, not a task | all story maps; FX timer on MAP2B | **OK.** No timed fail in the scripts. The FX timer runs inside the host-fired `OnActivate` on both processes |
| Collect / use / claim | `TakeItem` / `AddItemAmmo` in a few triggers and cutscenes. MAP2C `SetCanBeClaimed` + `SetScrClaimCallback` on `wildboar1`. MAP1 task 3 is `TARGET_CAN` with `NEED_AMMO_FOR_COMPLETE` | MAP1 cans; MAP2A ammo; MAP2C claim; MAP5 tranq pickup in dialogue; MAP11A/11C `NEED_AMMO_FOR_COMPLETE` | **OK** for claim: `kTrophyClaim` already makes the host run the claim. **RISK** for ammo: each process has its own hunter inventory, so a client can run out while the host still completes an ammo-gated task |
| Track (footprints, blood) | no footprint or blood script. `SetSoundToBePlayed("track_map4")` is a sting on MAP4A task 1 and MAP5 task 3. "Tracks" appears in dialogue only | MAP4A, MAP5 (audio); MAP10/MAP12A (dialogue) | **OK.** Nothing to desync |
| Defend / waves | sequential triggers spawn the next group. MAP2B gates on `wolfN.IsDead()`. MAP5 respawns two elephants (`CanBeRespawned`) on a chain of stampede triggers | MAP2B, MAP4B (three alternate lion cutscenes), MAP5, and the long shoot maps (6, 7, 8, 10, 12A) | **OK** if the host fire and host deaths stay in order. **RISK** on MAP5: chasers `SetTaskParam("target","hunter")` and respawn on the host brain |
| Vehicle | `GetInCar` + `DriveTo` | MAP4A live (`tleader4a` seat 1, `player` seat 2, `DriveTo("startcar1")`). MAP5 the same calls are commented out; the stampede is on foot | **BROKEN (expected).** The truck is `cMap4Truck` (`cScrEntity`), not an NPC mirror. The host hunter is put in the car; the client hunter is a different entity and will not be in that seat. UNVERIFIED whether `GetInCar` on the host also locks the client camera |
| Helicopter shoot | `BSMGoToState`, `helicopter.CompleteTask` / `UncompleteTask`, `NEED_AMMO_FOR_COMPLETE`, fail trigger `cScrFailed` | MAP11A, MAP11C. MAP3C and MAP9 have a `cScrHelicopter` instance; MAP12C loads the helicopter script | **RISK.** The heli script runs on both sides from the host fire, but ammo and the fail trigger are per hunter. A client who cannot shoot the host's boars can still be the one whose local ammo check would have failed; the host decides the task |
| Climb / rope | no calls | — | not present |
| QTE / struggle / melee | no QTE, mash, or struggle calls. Knife range `SetMaxKnifeDistance` on MAP3A/3B/3C/4A/10. `Pumafight` is an anim state | those maps (knife); no QTE map | **OK** for "no QTE to sync". Knife range is local |
| Sniping a target | no sniper task. Zoom is shared weapon script (`HGUN.SCR`), used heavily as a file on MAP9 and MAP12B | not an objective | **OK** |
| Cutscene-gated progress | `ExecuteScript("cScrCutSceneN")` from `BeginCutScene` / `OnActivate`. Inside: `ActivateCutSceneMode`, camera, `PlaySA` via `play_sa` | every story map except MAP3B (the `ExecuteScript` method is not called from `OnActivate`, and `MAIN.SPJ` lists no cutscene file) | **OK** to start: the host fire runs the same `ExecuteScript` on the client. **RISK** to skip: skip is per process. Paired meshes (`kPlaySA`) and prop `SetAnim` (`kPropAnim`) are already replayed. MAP12C's opening hold is a special case already patched |
| Checkpoints / restart | none in the scripts | — | not present. Same-map `spmap` replay is a known crash; restart the process |
| Player-death fail | no death call in story scripts. Host death ends the level; client death does not (F9) | all | **OK** under the stated policy. **BROKEN (expected)** if a map was designed so any hunter death fails it: the client can die and keep playing |
| NPC-death fail | no script calls a fail when an NPC dies. The protect list is the only hook | protect maps above | **UNVERIFIED** that the engine fails the mission. If it does, it should be the host message |
| Snakes | `AddAnimal(ANIMAL_COBRA)` and `Task_CobraAttack` with target `"hunter"`. Class is not `cScrAnimal` | MAP3B (3), MAP3C (4), MAP7 (1, and it is a shoot task) | **BROKEN (expected).** One snake per process; not in the animal mirror |
| Boss | `MakeBoss(TRUE, name)` | MAP11B `hogzilla1` "HOGZILLA", MAP12B `pbear` "BIG GRIMM", MAP12C `yeti` "YETI" | **RISK.** Body state names on the yeti already return -1 on the client for Attack/IdleAttack/Run. Position can still mirror if the yeti is a `cScrAnimal` |
| Mines | `cScrMine` ×10 on MAP10 | MAP10 | **RISK.** Not a mirrored actor. A mine that damages the local hunter hurts that process only; the host dying fails everyone |
| Traps | `cScrTrap` on MAP1, MAP2A (bridge prop is `cScrCustomCharacterEx fallbridge`), MAP2B, MAP2C, MAP7, MAP12A, MAP12B | those | **OK** for the MAP2A bridge (`PlaySA` / `SetAnim` already replayed). Other traps are UNVERIFIED if they are physics-only on each process |
| Stealth | `SetStealthLevels`, `DisableStimulus` | MAP1, MAP2C, MAP4B, MAP12A | **RISK.** Stealth is on `::globals.player`, so each window has its own meter. Animals still react to the host hunter |
| Mission end | `CheckMissionStatus()` after the last `CompleteTask` / `UpdateMissionStatus` | maps that call it (see section 1) | **OK.** Client swallows its own check; host `kMissionEnd` shows the results on both |

There is no scripted "player is looking at X" test. The only distance native in story scripts is `GetDistBetween2Ais` on MAP1 (stealth center vs `"hunter"`), MAP2C, MAP4B, and MAP12A (two calls each). Those four read the host hunter.

## 3. Per map

NPC rows: script object name, script class (the class in the NPC file that matches that name), brain. Every guide/girl/poacher/redshirt class that calls `SetBrain` uses `"brain_npc"` and a `cScrBrain_NPC brain` member. The skin is a BSP `model_name`. Verified skins: MAP2A Hugh `objects/npc/oldhugh/oldhugh.skin` (`cScrHugs2a`); MAP12C guide `objects/npc/mountainguide/mountainguide.skin`. Other skins are **UNVERIFIED**.

**MAP1.** Tasks: (1) race to Hugh, (2) find the guide, (3) shoot 4 cans and have ammo, (4) bonus 2 whitetail or they flee and the task fails into the same trigger, (5) 1 mountain lion, (6) 1 black bear. Trigger order that matters: `cScrTask01` objective `hugsm1` + cutscene 1; `03` reached + cutscene 2; `04` find-guide; `06` task 4 + cutscene 3; `07` task 5, protect Hugh, cutscene 4; `08` task 6 + cutscene 5; `09` `CheckMissionStatus`. Hugh (`hugsm1`, `cScrHugsm1`) path-leads (`MoveToPos` to `start1/2/3/6`) and is protected from task 5. `faketrap1` is a `cScrCustomCharacterEx`, not a person. Rating: **RISK** (protect + `"hunter"` look-at), not a guide-wait map.

**MAP2A.** Tasks: four reach-points, then kill 3 grey wolves, then a final reach, then `exit_1`. Triggers: `01` objective `hugs2a`, protect, cutscene 1; `02` reach 1 + cutscene 6; `03` task 2 + cutscene 2 (bridge `PlaySA` on `fallbridge`); `04` reach 2; `05`/`14` task 3; `09` reach 3; `06`/`13` reach 4 and task 5; `07` task 6; `08` reach 6 + `exit_1`. Hugh (`hugs2a`, `cScrHugs2a`) path-leads. Played in co-op: position mirror, bridge, wolves. Rating: **OK** for the objectives already played. His weapon is still not mirrored.

**MAP2B.** Tasks: reach, kill 1 wolf, kill 1 grizzly (forces `cScrTask04`), reach, kill 1 cougar, kill 10 wolves (forces `cScrTask13`), reach, reach, `exit_1`. Hugh (`hugs2b`, `cScrHugs2b`) path-leads, is protected, then is knocked down (`hurtdown` / `hurtsit`) and later `syncguidegetup`. Wolf `IsDead` gates the fire fight. Fire FX countdown 400s. Rating: **RISK** (downed escort, death gates, many host-targeted wolves).

**MAP2C.** Tasks: reach, kill 2 wolves (script also `CompleteTask(2)` early), reach, kill 1 boar (claim), reach, reach, kill 1 grizzly, then complete-all. Hugh (`hugs2c`, `cScrHugs2c`) protected and path-leads. Erin (`erin`, `cScrErin`) and Danny (`danny`, `cScrDanny`) walk to `end1` and play `cs06` as a pair. Boar claim is a real `kTrophyClaim` case. Rating: **RISK** (claim plus two extra NPCs).

**MAP3A.** Tasks: find the guide (3 finds required), then four hyena pairs, a reach, 3 hyenas, a reach, 2 leopards, a reach. Abby (`abby`, `cScrAbby`) starts as **guide** (`AITYPE_GUIDE|AITYPE_COVERSTAY`, cover list `avioncover`), objective `campsite` then `Food1`. After the return, `cScrTask12` switches her to **companion of `"hunter"`**. `cScrTask14` sets `radio_tower` and arms `exit_1` and `exit_2`. Rating: **RISK.** This is the guide-wait map.

**MAP3B.** Tasks: reach, kill 1 leopard. Triggers `01` (cobras attack `"hunter"`, objective `radio_tower`), `02` reach, `03`/`04` leopard moves, `05` `exit_1`, `06`/`07` menu pages. No NPC instance. Played. Rating: **OK** except cobras **BROKEN (expected)**.

**MAP3C.** Tasks: reach, kill 2 lionesses, kill 1 lion. Abby (`abby3c`, `cScrAbby3c`) is cutscene-only (walk + `cs03`/`cs04`). Cobras ×4. Helicopter instance `heli1` (arrival in the cutscene, not a drive). Rating: **RISK** (cobras, lion aggro on the host).

**MAP4A.** Tasks: reach (track sting), reach, kill 1 lion, then `exit4b`. Tribe leader (`tleader4a`, `cScrTleader4a`) is put in the truck and `DriveTo("startcar1")`; the player is seat 2. Cutscenes 1 and 2. Truck class `cMap4Truck` in `MAP4A/WORLD.SCR`. Rating: **BROKEN (expected)** for the drive. The lion kill after the exit trigger is an ordinary shoot.

**MAP4B.** Tasks: reach, bonus 4 antelope, kill 2 lionesses, kill 1 lion, then complete-all. Fiancée (`fiancee4b`, `cScrFiancee4b`) plays idle and three alternate cutscenes (`cScrTask15/16/17` → cutscenes 5/6/7); she does not follow. Objective stays `fiancee4b`. Rating: **RISK** (which of the three lion triggers fires is a host volume; the other two `Deactivate`).

**MAP5.** Tasks: reach, kill 1 hyena (script completes it early), reach through the stampede, then complete-all. Hamisi (`hamisi5`, `cScrHamisi5`) path-leads and is protected. Poacher (`poachera5`, `cScrPoachera5`) is a cutscene partner (`play_sa` on Hamisi). Elephants chase `"hunter"` and respawn. The car calls are comments. Rating: **RISK** (stampede target is the host).

**MAP6.** Tasks: reach, then tiger/bear shoot blocks the script completes by trigger, a reach, a 4-tiger block, a last bear, then complete-all (it also completes a task 9 that the mission file does not define). Reggie (`reggie6`, `cScrReggie6`) path-leads and is protected. Redshirts (`redshirt1/2/3`, `cScrRedshirt1/2/3`) are present; only `redshirt3` is marked unattackable in the opening. Objective flips to `red1`. Rating: **RISK** (protect, host-targeted cats).

**MAP7.** Tasks: find the child, several tiger kills, kill 1 cobra (arms `cScrTask08`), reach, kill 1 rhino, reach, then complete 7–9. Reggie (`reggie7`, `cScrReggie7`) path-leads and is protected. `indie_girl` is the find target (`AI_CHILD_ID`); she calls for help, jumps, then path-walks. `MAP7/MAIN.SPJ` includes `GirlLegs.scr` and does **not** include `Girl.scr`, so her script class on the entity is **UNVERIFIED** (`cScrGirl` is the only girl class that is not Erin/Danny). Cobra is a shoot objective. Rating: **RISK** for the find; cobra **BROKEN (expected)**.

**MAP8.** Tasks: find the guide, four scrub-bull kills the scripts can complete early, a reach, three more bulls, a final reach. Tribesmen (`tribesman1/2/3`, `cScrTribesman1/2/3`) become companions of `"hunter"` at `cScrTask08`, then `AITYPE_COMPANION|AITYPE_IDLESCARE`, and can be attacked. Crocodiles target `"hunter"`. Rating: **RISK.** Three followers on the host hunter.

**MAP9.** Tasks: one reach, then five dingo kill-counts (2, 2, 2, 3, 2); the end trigger completes 2–6 together. Reggie (`reggie9`, `cScrReggie9`) is protected, companion of `"hunter"`, then `AITYPE_GUIDE,"a"` at `cScrTask07`, then companion again. Objectives: `reggie9`, `c2_1`, `skeleton`, `camel_point`. Rating: **RISK.** Shortest map that switches guide and follow.

**MAP10.** Tasks: reach, puma and boar kills, a mid reach, more kills, last pumas arm `cScrTask17`, then complete-all. Wirake (`wirake10`, `cScrWirake10`) path-leads and is protected, then becomes companion of `"hunter"`. Reggie (`reggie10`, `cScrReggie10`) starts as companion+cover, becomes `AITYPE_GUIDE,"a"`, then companion+cover again. Ten `cScrMine`. Rating: **RISK** (guide switch + mines).

**MAP11A.** Tasks: reach, then three boar kills that need ammo, each arming the next trigger, each able to arm `cScrFailed`. Helicopter `CompleteTask` gates two of the steps. Siberians (`siberian1–4`, `cScrSiberian1–4`) have no move/talk lines in the trigger file. Ends with `exit_1`. Rating: **RISK** (ammo fail is per hunter; the heli is not a mirrored NPC).

**MAP11B.** Tasks: reach, kill 3 boars, kill 3 boars, reach, kill Hogzilla (arms `cScrTask11`), kill 3, kill 2, `exit_1`. Reggie (`reggie11b`, `cScrReggie11b`) path-leads and is protected. `hogzilla1.MakeBoss`. Rating: **RISK** (boss + protect). Animals did mirror (8) in the 30s smoke test.

**MAP11C.** Same shape as 11A (ammo, heli complete, `cScrFailed`) with one more boar task and no exit token. Same Siberians, no behavior lines. Rating: **RISK**, same as 11A.

**MAP12A.** Tasks: reach, wolf/bear blocks, a wolf that arms `cScrTask10`, more wolves, a bear that arms `cScrTask13`, then `exit_1`. Wirake (`wirake12a`, `cScrWirake12a`) path-leads, is protected, and is knocked down. Reggie (`reggie12a`, `cScrReggie12a`) is companion of `"hunter"` and is also knocked down. Rating: **RISK** (two escorts, one follows the host only).

**MAP12B.** One task: kill the boss bear, flee action kill, then `exit_2`. No NPC instance. `MakeBoss` "BIG GRIMM". One cutscene. Smoke test: client welcomed, 1 animal. Rating: **OK** if that animal is the bear and death presents. Boss anim names are UNVERIFIED.

**MAP12C.** One task: kill the yeti, then `CheckMissionStatus`. Guide (`mountainguide12c`, `cScrMountainguide12c`) has no behavior lines; the crash log names the mountainguide skin. Opening cutscene is the yeti's `PlaySA` (already special-cased). Rating: **RISK** (yeti `SetBodyState` Attack/IdleAttack/Run returns -1 on the client).

## 4. Top risks

1. **Guide wait ignores the client.** MAP3A Abby, and the Reggie stretches on MAP9 and MAP10, run `AITaskNPC_Follow` on the host brain. Proposal: on the host, resolve the follow/hello target to the nearest living player, not the entity named `"hunter"`, and keep the client brain frozen.
2. **Companion follow is host-only.** Same native with the string `"hunter"` (MAP8, MAP9, MAP10, MAP12A). The client sees the NPC glued to the host. Proposal: same nearest-player target. Path-lead `MoveToPos` does not need it.
3. **Protected NPC death and client damage.** `AddAIToBeProtected` on eleven maps. Health is copied down; a client hit is not sent up. Proposal: treat a client shot on a `kNpc` actor like an animal hit request, and confirm a host NPC death emits `kMissionEnd` (code 6) before building a new fail path.
4. **MAP4A truck.** `GetInCar` / `DriveTo` put the host hunter in `cMap4Truck`. The client is not that hunter. Proposal: replay the truck transform as a prop, and leave the client hunter on foot beside it until there is a seat snapshot. Do not expect the client to steer.
5. **Snakes.** MAP3B, MAP3C, MAP7. `cObjCobra` is outside the `cScrAnimal` mirror, and MAP7 task 6 is "kill the cobra". Proposal: include those classes in the same snapshot, or the cobra task can only be finished on the host's snake.
6. **Ammo-gated fails (MAP11A, MAP11C, MAP1 cans).** `NEED_AMMO_FOR_COMPLETE` and `cScrFailed` run on each process's own hunter. Proposal: swallow the client's fail trigger the same way as other fires, and let the host's ammo decide.
7. **Mines (MAP10) and stealth meters (MAP1, MAP2C, MAP4B, MAP12A).** Local damage and local stealth. Proposal: mines stay host-authoritative (a client mine hit becomes a hurt request). Stealth can stay local until animal perception is shared.
8. **Stampede and flee (MAP5, and every `MISSION_ACTION_KILL` flee).** Chasers bind `"hunter"`. Proposal: same nearest-player target inside the host animal task; do not unfreeze client animal brains.
9. **Boss presentation (MAP12C yeti, then MAP11B and MAP12B).** Client body-state names from the deer set fail on the yeti. Proposal: mirror the host's state name only when `SetBodyState` succeeds; otherwise keep the last good pose and still apply death.
10. **Cutscene skip and the MAP3B exit.** Skip is per process, so one window can leave a `PlaySA` early. Proposal: host skip commits a cutscene-end and the client applies it. Separately, confirm whether MAP3B `exit_1` → MAP2A is the retail link or a bad test exit before anyone treats chapter 3 as linear.

## 5. Test checklist

Left window is the host, right is the client. F8 hides the overlay. F9 stands a dead client up at the host. Do not write `coop_trigger` into a live session. Restart the process instead of `spmap` of the same map. Success lines to watch: client `story: applied host fire '…'`, client `animals: mirror freeze id=… npc=true`, `story: PlaySA` / `story: applied PlaySA`, `events:` mission message, client `following host to '…'`. A fire that happens only in the host log is a desync.

1. **MAP3A — guide waits, then follows.** `.\run-coop-local.ps1 -Map MAP3A`. Host stands still. Client walks ahead to where Abby is leading. She should keep waiting (current model). Then the host walks up to her: she should move on both screens (`npc=true`, drift near 0). Later, after she is switched to companion (`cScrTask12`), she should stick to the host, not the client. Fail: she walks off on only one screen, or the client never gets `applied host fire 'cScrTask01'`.
2. **MAP9 — companion, then guide, then companion.** `.\run-coop-local.ps1 -Map MAP9`. Stay together through Reggie's first follow. At the guide stretch (`cScrTask07`, objective `camel_point`), repeat the MAP3A split (client ahead, host back). He must wait for the host. Dingoes should aggro the host and die on both screens when the host kills them.
3. **MAP2B — escort can go down.** `.\run-coop-local.ps1 -Map MAP2B`. Walk with Hugh until the knockdown (`hurtdown` / `hurtsit`, then `syncguidegetup`). Both screens should show the same pose. Kill the wolves on the host; the `IsDead` gate should fire the next trigger on the client too. Fail: Hugh stands on the client while he is down on the host, or only the host arms `cScrTask04` / `cScrTask13`.
4. **MAP7 — find the girl, kill a cobra.** `.\run-coop-local.ps1 -Map MAP7`. Host finds `indie_girl` (`MISSION_ACTION_FIND_SOMETHING`). Client should get the same task advance without standing on her. Then the cobra task: expect two cobras that do not match. Killing the client's cobra should not finish the task; killing the host's should. Log: no `npc=true` on the cobra; task fire `cScrTask08` only after the host kill.
5. **MAP8 — three followers.** `.\run-coop-local.ps1 -Map MAP8`. After `cScrTask08`, the three tribesmen should stack on the host. Client sprints away: they stay with the host on both screens. Crocs should chase the host. Fail: a tribesman follows the client, or only one of the three freezes (`npc=true` three times).
6. **MAP4A — truck.** `.\run-coop-local.ps1 -Map MAP4A`. When `cScrTask01` fires, the host is put in the truck (`GetInCar` / `DriveTo`). Watch whether the client stays on foot and can still look around, and whether the truck mesh moves on the client. The lion at the end is the control (ordinary kill). Fail: client camera stuck in the host's seat, or the client never leaves the opening cutscene.
7. **MAP5 — stampede.** `.\run-coop-local.ps1 -Map MAP5`. After the hyena, elephants should spawn and chase the host, including a respawn (`CanBeRespawned`). Client shoots one: host log should show the hit, then a death on both. Fail: a second herd on the client only, or the reach task (`cScrTask20`) fires on the host and not the client.
8. **MAP11A — helicopter and ammo fail.** `.\run-coop-local.ps1 -Map MAP11A`. Let the host shoot the boars. Empty the client's gun on purpose and keep firing. The mission should not fail unless the host fails (`cScrFailed` must not win on the client). Then walk the host to `exit_1` and see which map the client follows. Fail: client `MISSION FAILED` while the host is still playing, or `no such trigger here` on the follow.

If those eight hold, the next untested cluster is MAP10 (mines + the same guide switch), then MAP12A (two escorts, one downed), then MAP11B / MAP12B / MAP12C bosses.

## Proposed MEMORY facts

- Story chapters call `SetMission` only on MAP1, MAP2A, MAP3A, MAP4A, MAP5, MAP6, MAP7, MAP8, MAP9, MAP10, MAP11A, MAP12A (values 1–12) plus the three bonus maps (17–19). Evidence: `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:15`, `MISSION02A.SCR:14`, `MISSION03A.SCR:15`, `MISSION04A.SCR:15`, `MISSION05.SCR:15`, `MISSION06.SCR:15`, `MISSION07.SCR:14`, `MISSION08.SCR:15`, `MISSION09.SCR:15`, `MISSION10.SCR:15`, `MISSION11A.SCR:14`, `MISSION12A.SCR:14`. MAP2B/2C, MAP3B/3C, MAP4B, MAP11B/11C, MAP12B/12C have no `SetMission` (inventory section 6).
- The only script-side map links are exit tokens: MAP2A `exit_1` (`TRIGGERMISSION02A.SCR:380`), MAP2B `exit_1` (`TRIGGERMISSION02B.SCR:1061`), MAP3A `exit_1` and `exit_2` together (`TRIGGERMISSION03A.SCR:463-464`), MAP3B `exit_1` (`TRIGGERMISSION03B.SCR:125`), MAP4A `exit4b` (`TRIGGERMISSION04A.SCR:258`), MAP11A `exit_1` (`TRIGGERMISSION11A.SCR:215`), MAP11B `exit_1` (`TRIGGERMISSION11B.SCR:318`), MAP12A `exit_1` (`TRIGGERMISSION12A.SCR:632`), MAP12B `exit_2` (`TRIGGERMISSION12B.SCR:50`). No story script names the destination map.
- Guide mode is `SetAIType(AITYPE_GUIDE…)` on Abby (`TRIGGERMISSION03A.SCR:32`), Reggie on MAP9 (`TRIGGERMISSION09.SCR:231`), and Reggie on MAP10 (`TRIGGERMISSION10.SCR:484`). Companion-of-hunter is the string `"hunter"` passed to `SetAIType(AITYPE_COMPANION…)` on MAP8 (`TRIGGERMISSION08.SCR:511-513`), MAP9 (`TRIGGERMISSION09.SCR:57`), MAP10 (`TRIGGERMISSION10.SCR:31` and `:497`), and MAP12A (`TRIGGERMISSION12A.SCR:36`).
- NPC brains in these scripts are `SetBrain("brain_npc")` with member `cScrBrain_NPC`. Evidence: `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:89` and `:126` (`cScrHugsm1`). The same pair is in `GIRL.SCR:37`, `POACHER.SCR:67`, `REDSHIRT.SCR:68`.
- MAP4A puts the local player in a vehicle: `tleader4a.GetInCar(1)`, `player.GetInCar(2)`, `tleader4a.DriveTo("startcar1")` at `TRIGGERMISSION04A.SCR:22-25`. The truck type is `cMap4Truck extends cScrEntity` in `LEVELS/MAP4A/WORLD.SCR:75`.
- MAP7's cobra is a mission animal, not an ambient prop: `AddAnimal(ANIMAL_COBRA, TRUE, "cobra1")` in `LEVELS/MAP7/WORLD.SCR:144`, and task 6 is `SetTaskAnimalToShoot(ANIMAL_COBRA, ANIMAL_KILL, 1)` in `MISSIONS/MISSION07.SCR:56`.
