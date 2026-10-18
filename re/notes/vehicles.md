# Vehicles and movers — playthrough wave (vehicles agent, 2026-10-18)

Worktree `D:\scary hunt\dh2coop-truck`, branch `trucks`, port 27650, Ghidra slot N. Stopped early (session usage limit): one baseline game run, then code built but **not run**.

Commits:
- `3b4d04c` WIP: truck seats and boarding for every player, riders as lion targets (untested)
- `98a4211` Test: truck-test.ps1, MAP5 / MAP4A ride scenarios on this checkout's games
- (this note)

## 1. What was found

### MAP5 baseline run (VERIFIED, `truck-test.ps1 -Scenario map5-host`, 09:57–09:59, main code + nothing new)
- MAP5 `car1` is entity **id 126** (BSP index 126), the same class as MAP4A's (`obj_mod_vehicle`, `cMOD_ObjVehicle`). Host log `vehicles: vehicle id=126 … updates`; client `vehicles: id=126 frame steps … min 0.000` (frozen, following from map load). So `vehicles.cpp` covers it.
- Host walked into cScrTask06 (`coop_tp 605.9:2:135.8`): host `CompleteTask 2` (BeginCutScene), cutscene 3 (~35 s), fire at +35 s, then `vehicles: ride id=126 hunter 1.69 m … npc 133 0.75 m (parent 126)` (host + Hamisi seated). The truck drove; the client's copy followed (`frame steps … avg 0.006`).
- **Client left on foot at the camp** (G4 confirmed): client got only `applied host fire 'cScrTask06'`, never sat (`puppets: rider 0 seated=false`), no tranq rifle. The client had even been gathered into the cScrTask06 volume (`coop: teleport to host (after cutscene)` after cutscene 1) but its own OnHit never ran there (teleported, standing still; reason not found).
- Stampede triggers 07–09: the client's truck copy hits each volume ~100 ms after the host's (host `trigger event activate id=143` 16.393, client 16.502). Because their OnHit re-arms (`Activate()`), the host never fires them itself; the client's request makes the host run the (empty) OnActivate (`[host] ran 'cScrTask08' (id 143) for player 1`). Harmless here. Decided: leave the client copy's trigger hits as they are (the host is always 100 ms ahead); not suppressed.
- Hamisi (id 133) is seated on the host; on the client his mirror is not attached (no seat sync existed).

### Engine facts (VERIFIED by decompile, files `re/notes/veh-*.c|txt`, gitignored)
- `cObjVehicle::Use(n, ent)` (CommonDll `0x5adc0`, vt `+0x158`): n = 0 GetOut, else `GetIn(ent, n-1)` (or a deferred get-in when `ent vt+0x1d0` refuses). Script `player.GetInCar(2)` = GameDll `0x48d10` → `(*(0x1657f0)+0x104)->Use(2, hunter)` + `hunter vt+0x284(1)` (= `0x548c0`, sets `+0x4d8 |= 4`, `+0xc1c`). So the player seat is **place_1**, the driver place_0.
- `cObjVehicle::GetIn(character, place)` `0x5ab60`: Attach to tag `place_<n>`, seat array **`+0x298` int[4]** (entity ids, -1 empty), body state `TurnDrive` (place 0) / `TurnCoDrive`. `GetOut` `0x5a260` detaches beside the vehicle.
- `cObjVehicle::DriveTo` `0x59b00` ends with vt `+0x228` = **UnFreeze**: a client's own DriveTo (MAP5 cutscene 3 `hamisi5.DriveTo`) after the copy was frozen gives it its gravity back. Fixed in the WIP (hook + refreeze).
- cObjVehicle vtable CommonDll `0x123d28` (Update `+0x44`, Use `+0x158`, Freeze `+0x224`, UnFreeze `+0x228`); cMOD_ObjVehicle vtable GameDll `0x104930` (Update `+0x44` = `0x3b710`).
- `cObjHunter::OnDie` (GameDll `0x50350`): a seated hunter (`+0x4d8 & 4`) gets `vt+0x284(0)` and `GetOut` before Death, so a rider who dies lies on the road; F9 then placed it at the host (moving seat) → road behind the truck.
- TryPlaySA (CommonDll `0xcadf0`, fastcall(task)): kind `"truck"` only when the target's GetCar (vt `+0x1c0`) finds a vehicle and vt `+0xfc` says player; then the lion is `Attach`ed to the truck at `place_<CheckUser(target)>`. A riding puppet is not attached → ground kind `pl_ln_syncattack1`, target part0 sync-linked to the lion (`vt+0x38`): the "puppet thrown off the truck". SA end `0xc3960` unlinks and Detaches the lion. Brain = `*(task+8)`, target id `brain+0x254`, animal `brain+0x68`.
- Lion/lioness chaser truck SA `syncattacktruck` has **no damage** in `MakeChaser` (no `sadmg*`); the damage that killed the host came from claw/leap attacks (`dmgval`, `impval`). Teen paks with `syncattacktruck`: MAP3C, MAP4A, MAP4B, MAPARENA (MAP5 has only `pl_hy_syncattack1`). `HUMANLEGS.SCR` PlaySA returns to `TurnCoDrive` when `player.IsInVehicle()`: the seated hunter supports the knock clip by design (runtime UNVERIFIED).

## 2. What the WIP code does (`3b4d04c`, built, UNVERIFIED in a run)
- **BoardWithHost** (client): host's PlayerState has `kFlagRiding` for 2 s, our hunter alive, not seated, no cutscene camera, our copy has host samples → `GetInCar(2)` (0x48d10, prologue checked). Up to 10 tries, 3 s apart.
- **SeatSync**: host polls `+0x298` 5×/s; an NPC getting in/out → `Commit(kVehicleSeat=28, veh, place | id<<16, key "p<n>=<id>#<serial>")` (journaled: late joiners). Client reconciles with `cObjVehicle::GetIn/GetOut` (not during its cutscene camera, 20 tries).
- **BoardRequest**: client's own script seated it (parent -1 → veh) while the host does not ride → `Request(kVehicleBoard=29, veh, driver id, key = last DriveTo waypoint)`. Host waits 5 s (and while its cutscene camera is up), then driver into place_0, `GetInCar(2)` for the host hunter, `DriveTo(waypoint)` unless its own script already drove. This removes the MAP5 "client boards first → host never boards → soft lock" without `kTriggerHit`.
- **DriveTo hook**: a following copy's own DriveTo is not run; `follow()` re-freezes a copy whose body gravity came back.
- **RideWithHost** in `coop.cpp` (lead's file, 2 small edits): `TeleportToHost` (gather after cutscene, join catch-up, `coop_tohost`) never moves a seated hunter and seats it when the host rides; F9 revive keeps a seated hunter in place and seats a revived one when the host rides.
- **RiderTargets** (default **0** until tested): riders stay on the target list; TryPlaySA hook returns 0 for a riding puppet (no SA → no glued puppet); **RiderKnock** sends `kPlayerKnock "syncattacktruck"` (lion within 4 m, every 8 s max) and `kPlayerKnockEnd` after RiderKnockMs; **SplitRiders** shares lions targeting anyone in the truck by `animal id % crew` (same task-switch as combat.cpp; under its 3 m margin).
- Test commands: `coop_board`, `coop_unboard`, `coop_rideknock <id>` (host), `coop_vehinfo`.

## 3. Mover sweep (BSP entity lists of all 21 story maps, scratch script)
| Map | Mover | Who drives it | Verdict |
|---|---|---|---|
| MAP4A, MAP5 | `car1` truck | host, streamed (msg 11) | covered; seats WIP |
| MAP3C | `heli1` | `SetAnim("m3_rescue")` (kPropAnim) | OK (audit-a) |
| MAP9 | `heli1`, `heli1_test` | `cScrHelicopter::SetAnim` in replayed OnActivate | OK, runs on both (audit-b) |
| MAP11A/C | helicopters, `tele` cable car | pilot agent / kPropAnim | not mine |
| MAP2A | `fallbridge` | PlaySA partner (kPlaySA) | OK |
| MAP1 | `faketrap1` | PlaySA partner in cutscene 5 | OK (kPlaySA) |
| MAP8 | `atree`, `fxtree`, `rockt1` | SetAnim in OnActivate | OK (audit-b) |
| **MAP12B** | 2× `obj_move_object` falling trees | Big Grimm (`AIHuntBrain_ObjectMover`, SA `faltree`) on the host only | **DIVERGES, UNVERIFIED**: client bear is frozen, the matrix PlaySA on the tree is not replayed (kPlaySA hooks the name overload). Bosses agent's bear; needs a replay of the tree's PlaySA |
| MAP9, MAP12A | `obj_ice` (crack per process) | each player's own hunter | per-player hazard, consistent per player |
| MAP6, MAP7 | cages (`info_static_event ai=obj_cage`) | each process, its own hunter in radius | per-player hazard; others do not see it (cosmetic) |
| MAP7, MAP10, MAP12B | traps (`obj_trap`, EVENT_TRAP) | `obj_player` only | per-player (puppets never close them) |
| MAP3A/3B/4B/9 | `falling_rocks*` = `obj_cavedust` FX | per process | cosmetic |
No train, boat, or rolling rock exists in any story BSP.

## 4. Verdicts
| Map | Item | Verdict |
|---|---|---|
| MAP5 | truck pose sync covers car1 | VERIFIED (run) |
| MAP5 | client left behind when host boards | VERIFIED bug; WIP fix UNVERIFIED |
| MAP5 | client boards first → host never boards | VERIFIED by code path (audit-b G1/G4); WIP fix UNVERIFIED |
| MAP5 | car1 volumes: host hits first | VERIFIED (100 ms lead in the run) |
| MAP4A | riders as targets / knock / split | UNVERIFIED, default off |
| MAP4A | late join seated | UNVERIFIED (WIP: seat sync + BoardWithHost + teleport guard) |
| MAP4A | F9 / TeleportToHost while seated | UNVERIFIED (WIP) |
| MAP4A | regression | NOT RUN after the WIP |
| MAP12B | falling trees | DIVERGES (route to bosses) |

## 5. Next steps for whoever resumes (in order)
1. `.\mod\build.ps1`, then `.\tools-src\truck-test.ps1 -Scenario map4a -Watch 120` — **regression first** (user-confirmed smooth ride and seated riders must hold). Grep `vehicles:` for `seat our hunter`, `host seat commit … place_0 = 67`, `our script drives … not run`.
2. `-Scenario map5-host -Watch 90`: expect client `vehicles: seat our hunter (the host rides) … parent=126`, `seated 133 at place_0`; screenshots `re\notes\coop-local\truck-map5-host-*.png` both in the truck.
3. `-Scenario map5-client`: expect client `asked the host to board`, host `host drives vehicle id=126 to 'PathNode12'`; the host has no tranq rifle in this path (TakeItem is in cutscene 3; only kTriggerHit gives it).
4. `-Scenario map4a-late -LateJoin 25`: late joiner seated (`seat our hunter` or its own BeginCutScene), `our hunter sits in vehicle … not moved` instead of `teleport to host (join catch-up)`.
5. F9: `-Scenario map4a -Cmd 'game2:coop_down@40','game2:coop_respawn@48'` → `F9 respawn … seated=true`.
6. Riders as targets: set `RiderTargets=1` in `game\Bin\dh2coop.ini`, `-Scenario map4a -Watch 150`; expect host `combat: animal id N hurt player 1` (claw damage reaches the rider), `vehicles: sync attack … on rider 1 not started`, `animal id N goes for player 1 in the truck`; client `combat: knocked down (host animal), clip 'syncattacktruck' … ok=true` then `knock over (host animal finished)` and still `parent=68`. Also test `coop_rideknock 1` from the host while riding. If the knock unseats the hunter, set `RiderKnock=0` (damage only). Pick the default from that.
7. Open: MAP12B tree replay (bosses); `LocalPlayer().pos` lags 2.5–5 m at speed, so `story.cpp` `PlayerInCutscene()` (view > 4 m from pos) can defer host fires during a fast ride (triggers / pilot: use `+0xE8` read at render time or skip the test while seated).

## Depends on others
- **triggers** (`kTriggerHit`): with it, both run MAP5 cutscene 3 (rifle for both, cutscene 4 at the end). Without it, the WIP seats everyone but only the boarding player's process runs the cutscene and TakeItem.
- **life**: B-F2 in `playthrough-b.md` (client lion copies thaw in host SAs and attach to the client's truck: the stuck lioness) is in `animals.cpp`; riders-as-targets does not touch it.
- **hunter**: owns attack-target rules in `combat.cpp`; SplitRiders lives in `vehicles.cpp` and only switches animals already after someone in the truck.

## Proposed MEMORY facts
- `cObjVehicle` seat array `+0x298` int[4] (entity id per `place_N`, -1 empty). Script `GetInCar(n)` (GameDll `0x48d10`) = app vehicle `*(0x1657f0)+0x104` → `cObjVehicle::Use(n, ent)` (CommonDll `0x5adc0`, vt `+0x158`) → `GetIn(ent, n-1)` (`0x5ab60`) + hunter vt `+0x284(1)`; the player sits at place_1, the driver at place_0. VERIFIED (decompile).
- `cObjVehicle::DriveTo` (`0x59b00`) ends with vt `+0x228` = `UnFreeze`: a frozen follower copy gets its gravity back if its own script drives it. VERIFIED (decompile + vtable dump).
- `cObjHunter::OnDie` (`0x50350`) gets a seated hunter out of the vehicle (`GetOut`) before Death. VERIFIED (decompile).
- MAP5 `car1` = entity id 126, same `cMOD_ObjVehicle` as MAP4A (id 68); the client copy follows the host from map load; the client's copy enters car1 trigger volumes ~100 ms after the host's. VERIFIED (run 2026-10-18 09:57).
- TryPlaySA (CommonDll `0xcadf0`): truck kind needs the target's GetCar and IsPlayer; then the animal is attached to the vehicle at `place_<CheckUser(target)>`; brain `*(task+8)`, target id `brain+0x254`. VERIFIED (decompile).
