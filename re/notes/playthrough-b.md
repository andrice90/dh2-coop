# Playthrough research B — MAP4A, MAP4B, MAP5, MAP6, MAP7, MAP8 (2026-10-18)

Job `research-B`, wave "playthrough" (`re/jobs/_wave-playthrough.md`). Static research only: no build, no game run, no git writes. Ghidra slot B. Code read at `main` HEAD `a3322d1`.

**Legend.** VERIFIED = seen in code, data or decompile. UNVERIFIED = inferred. Severity: **BLOCKS** = someone cannot finish the map; **DESYNC** = players play different games; **COSMETIC**. Gap ids: `a-Gn` = `audit-maps-a.md`, `b-Gn` = `audit-maps-b.md`, `B-n` = new here. RVAs are module-relative (GameDll7 unless marked C = CommonDll7, E = EngineDll7).

Scratch evidence (gitignored-style names, nothing to commit):
- `re/notes/playthrough-b-dec-game1.c` (cScrAnimal::Update `0x1f3c0`, present death `0x1e8b0`, mission report `0x91e80`, `GetInCar` `0x48d10`, `DriveTo` `0x48e40`, `cMOD_ObjVehicle::Update` `0x3b710`, `OnCollision` `0x3b8c0`)
- `playthrough-b-dec-common1.c` (TryPlaySA C `0xcadf0`, ProcessAttackState C `0x92710`), `playthrough-b-dec-common2.c` (CanPlaySA, GetCar, both PlaySA overloads, OnEndSA, SA end C `0xc3960`)
- `playthrough-b-dec-engine1.c` (`cModelPart::SetAnim` E `0x1e73b0`), `playthrough-b-dec-natives.c` (cScrHunter natives table `0x53e80`), `playthrough-b-dec-game2/3.c`
- `playthrough-b-xref-*.txt` (string xrefs), `playthrough-b-ents-<MAP>.txt` (BSP entity lists), `playthrough-b-triggers.txt` (trigger / NPC / item positions in metres)

---

## 0. Engine findings that change the picture (all maps)

### B-F1 "Could not forward animation" is a stale sync link, not a missing clip (VERIFIED)
- The fatal comes from one place: `cModelPart::SetAnim(int,float,float)` E `0x1e73b0`. The part plays its clip, then walks its link ring (`part+0xf4`, built by `cModelPart::SetSyncWith` E `0x1e7050`) and asks every linked part for `"<part>/<clip>"` or `"<clip>"`. If the linked part's model has neither, it calls `SendToLogRaiseException(0x17, "Could not forward animation %s from %s to %s model %s")`.
- A clip missing on the part's **own** model is silent: the index check at the top returns 0 (vt `+0x5c(0)`, no log). So a body state whose clip the model lacks gives a wrong pose, not a crash.
- The MAP4A lioness has `idleattack1`: `lionesse.skin` points at `objects/animals/lion/lion.model`, and both `LION.MODEL` and `LIONESSE.MODEL` list `idleattack1..3`, `closeattack*`, `syncattacktruck`, `pl_ln(s)_syncattack1` (length-prefixed names in the FBMF model). The yeti model has `idleattack1` too, and the log-sweep line is "from Legs to Legs model yeti.skin". **The MEMORY line "The MAP4A lioness has no idleattack1" is wrong**: the failing calls were forwards through a sync link to a partner model that lacks the clip.
- Who links: animal TryPlaySA C `0xcadf0` (lines 159–170 of the decompile) calls `target part0 vt+0x38(animal part0)` and `target part1 vt+0x38(target part0)` (vt `+0x38` = SetSyncWith). Only the SA end C `0xc3960` unlinks (`vt+0x38(0)` on the target's parts), calls both `+0x214` (OnEndSA), and `cObjEntity::Detach(animal)` for a truck SA.
- Consequence for "clip coverage": a model lacking a clip is COSMETIC unless that part is linked (a sync attack in progress or abandoned, or an NPC weapon linked with `LinkPart`). The checks below list missing clips as cosmetic and the stale links as the crash risk.

### B-F2 Client animal copies thaw during every host sync attack (VERIFIED code path, runtime UNVERIFIED)
- `cObjCharacterEx::PlaySA(char*, m4x4&)` C `0x27f10` sets `+0x368 = 1` and every part's body state to `CutScene` (decompile; matches `knockdown.md`). TryPlaySA calls it on the animal (`+0x210`). So a host animal inside an attack SA streams state `"CutScene"`.
- `animals.cpp` `ApplyMirror` (l.969–981): host state `CutScene` and copy state `CutScene` → `FreezeBrain(false)` and **no placement** until the host leaves `CutScene`. The copy enters `CutScene` one frame earlier through the normal state apply (l.1139).
- The thawed copy runs its replayed attack task against **this process's** `"hunter"`: own claw damage on the client hunter, and its own TryPlaySA. On MAP4A that is kind `"truck"` (`GetCar` C `0x1bc50` finds the seated hunter's vehicle) → `syncattacktruck` → `cObjEntity::Attach(copy, client truck, place_<seat>)` and a sync link to the client hunter's legs.
- When the host's lion leaves `CutScene`, the client freezes the copy again mid-SA. Its task never reaches the end `0xc3960`, so the copy stays attached (`+0xA4` = truck id), linked, `+0x368 = 1`. `ApplyMirror` then skips placement for attached actors (l.1112: rule written for the MAP4A driver), and every forced state forwards to the hunter's model → "Could not forward animation idleattack1 …" → the script VM fault the 3-tries rule now suppresses. That is the open "client lioness copy stands in one pose after a tackle, sliding along".
- Same mechanism on every map with animal sync attacks: MAP4B (lions, hippo `pl_h_*`), MAP5 (hyenas `pl_hy`, elephants), MAP6/MAP7 (tigers `pl_t_*`, MAP6 bear), MAP7 rhino, MAP8 scrub bulls `pl_sb_*`.
- Fix (owner life, `animals.cpp`):
  1. Thaw a copy for `CutScene` (l.974) **only while this process's own cutscene camera is up** (`story::PlayerInCutscene()`, expose it). The thaw exists for client cutscenes whose script waits on an animal task (MAP12C yeti; MAP6 cs3 bear `AIHuntTask_PlaySA` on redshirt3, MAP6 cs4, MAP4B cs5–7, MAP7/MAP8 below). An attack SA never puts the client's camera in a cutscene. Testing `+0x368` alone is not enough: the MAP6 cs3 bear is a cutscene SA with `+0x368 = 1` that the client must run. Optional extra: host flag `kInSyncAttack = 1u<<4` in `net::animal_flag` (lead's file, same byte) from the host animal's `+0x368`, for logs.
  2. Client repair, once per frame for an animal copy outside a local cutscene: if `+0x368 == 1` or `+0xA4 >= 0` while the host's state is not `CutScene` → `cModelPart::SetSyncWith(nullptr)` (E `0x1e7050`) on copy part 0/1 and on the local hunter's part 0/1 (model vt `+0xf0(i)`; skip the hunter while `combat.cpp` `g_knocked`), `cObjEntity::Detach` (E `0xb8d20`), base `cObjCharacterEx::OnEndSA` (C `0x28360`; not on the yeti, `animals.cpp` l.986), log `animals: mirror id=N stale sync attack repaired`. Keep the "attached → no placement" rule (l.1112) for NPCs only.
- Test: `run-coop-local.ps1 -Map MAP4A -Port <p>`; ride; let a lioness jump the host truck. Today: client `animals: mirror thaw id=<lion> while state is CutScene` followed within seconds by `SetBodyState id=<lion> 'IdleAttack' -> 0 … not tried again`, `debug.log` "Could not forward animation". Fixed: no thaw line for SA, `animals: mirror id=<lion> … host in sync attack: kept frozen`, no "Could not forward".

### B-F3 Kill credit and tranq credit (VERIFIED)
- Present death `0x1e8b0`: unless `+0x500 == 2`, it calls `0x91e80(mission, *(ent+0x420), species +0x434)` and clears `+0x420`. Only the bullet `OnHit` `0x1a680` writes `+0x420 = 1` (both bullet and tranq branches). `OnAnimalHit` (`animals.cpp` l.770) applies a client's hit with `InflictDamage` + `PresentDeath` and never writes `+0x420`, so a kill the host never shot itself reports action 0 and **does not count** for `SetTaskAnimalToShoot(…, ANIMAL_KILL, n)`.
- The shoot task's update (`TASK_SHOOT` vtable `0x111c0c` slot `+0x10` = `0x999a0`) decrements its remaining count `+0xf4` only for action 1 with the task's species `+0xf0`; at 0 the task state `+0x68 = 3`. Action 0 does nothing (VERIFIED).
- Each process's mission counts only its own credits: a client copy that a client bullet hit has `+0x420 = 1`, so the client's own mission counts that kill when the host's death commit is presented there. Mixed kills split the count: host 1 + client 1 of a "kill 2" task completes on **neither** mission.
- Tranq: `cScrAnimal::Update` `0x1f3c0` l.134: while `+0x4f0 == 2`, `+0x4f4` (tranq time; respawn sets it from `SetTranqTime` or 5.0) counts down; at 0 it calls vt `+0x154` (present death) → the tranq sleep **is** the kill, credited with the `+0x420 = 1` the dart wrote. Tranq hurt `+0x4f8` = script `SetTranqHurt` value or 0 (respawn `0x14610`), so a dart changes no health and the mirror sends no `AnimalHit` (b-G3).
- On a client, the dart still runs `OnHit` on the copy (`+0x4f0 = 2`, `+0x420 = 1`), and the copy's own `Update` presents a local death after `+0x4f4` s: the client's mission counts a kill the host never had, and the copy lies "dead" while the host's animal keeps attacking.
- Fix kill credit (owner life): in `OnAnimalHit`, before a lethal `PresentDeath`, `*(int*)(ent+0x420) = 1` (only when `+0x500 != 2`, which `0x1e8b0` checks itself).
- Fix tranq (owner beasts, `beasts.cpp`): client hook `0x1a680` (prologue `55 8b ec 6a ff`, b-hook bytes): bullet `IsEntityKindOf("mod_obj_tranq_bullet")` on an `IsFrozenMirrorAnimal` → skip the original (no local fake sleep), `Request(kTranqHit = 22, id, shooter)`. Host: if `+0x4f0 == 2 && +0x604 == 1` → `0x965e0(g_mission, "MISSION FAILED\nYou've injected an overdose \nand the animal died!")` (rides `kMissionEnd`); else `+0x4f0 = 2`, `+0x420 = 1`, vt `+0x228(+0x4f8, shooter puppet)` (damage 0 still runs the hurt reaction), brain vt `+0x88(puppet)` when hp > 0. The host's own `Update` then sleeps it and credits the kill.

### B-F4 Sync-attack rules (VERIFIED, TryPlaySA C `0xcadf0`)
- Starts only if animal `+0x368 != 1`, target `+0x368 != 1`, task `+0x144 != 1`, target vt `+0x218` (CanPlaySA, base C `0x14a60` returns 1) non-zero. Kind `"truck"` when target vt `+0x1c0` (GetCar) returns a vehicle and the target is the player (vt `+0xfc == 1`), else `"attack"`.
- Riders today are removed from the attack list (`puppets.cpp` l.703), so on MAP4A/MAP5 every chaser goes for the host. A cheaper rule that keeps riders as targets: while `kFlagRiding`, hold the puppet's `+0x368 = 1` every frame (combat.cpp already writes it for a busy puppet). Then no animal starts an SA on a rider (no tackle into the truck bed), and claw hits still arrive at `ForwardPuppetHurt` → `kPlayerHurt`. UNVERIFIED that a chaser does claw damage to a rider at all (the truck SA may be its only hit).
- `SetDenySATime` (MAP8) = cScrHunter native `0x4bf60`: hunter `+0x67c = t`. Its reader is not in TryPlaySA (UNVERIFIED where it is read).

### B-F5 Vehicles (VERIFIED)
- MAP5 `car1` is the same class as MAP4A's: BSP `obj_mod_vehicle`, `resource=truck1` (BSP index 126). `cMOD_ObjVehicle::Update` `0x3b710` calls `cObjVehicle::Update` first (l.532), so `vehicles.cpp`'s hook on the CommonDll export covers it, and the host streams it from map load (any vehicle that updated).
- `cMOD_ObjVehicle::Update` then hurts the entity at `+0x29c` by 100 (type 1) when it is a player and the up axis tips below the threshold. A client copy follows the host's pose, so both seated hunters die together if the host's truck rolls. No new risk.
- `cObjHunter::DriveTo` `0x48e40`: first `cMOD_ObjVehicle` in the entity list → `cObjVehicle::DriveTo` + route object `veh+0x3a0` seeded at the caller's position. A client's own `DriveTo` sets the drive bit; `vehicles.cpp` `follow` clears it every frame once a host sample exists (always, the host streams parked trucks too). Covered.
- `cObjHunter::GetInCar` `0x48d10`: `*(0x1657f0)+0x104` vt `+0x158(seat, this)`, then `this` vt `+0x284(1)`. Runs locally per process; that is what seats each hunter in its own copy.
- `cMOD_ObjVehicle::OnCollision` `0x3b8c0`: a `trigger_event` → vt `+0x8c` (OnHit) with the vehicle as activator. Client copies hit triggers too (the MAP4A state-2 spin proves it).

### B-F6 Stampede triggers re-arm themselves (VERIFIED, MAP5 07–17)
- `OnHit` = `ActivateAfter(0); Activate();`. OnHit disarms first (`0x68fd0`), the script re-arms, and `UpdateTrigger` (elephant respawner) then runs every frame while armed **and the local player is inside** (b-G8). On a client the truck copy carries the client hunter through the same boxes, so the client's spawner runs on its copies: `CanBeRespawned` reads the copy's own `+0x4b0`, which only the client's own `MoveToPos`/`Kill` set. Mostly inert (host positions overwrite), but see B-F7.

### B-F7 A dead copy that a script respawns becomes a live ghost (VERIFIED code path)
- After `PresentDeathOnce` the copy is unfrozen (`UnfreezeDead=1`, l.1195) and `death_presented` makes `ApplyMirror` pin hp 0 and `continue` forever (l.959).
- A later replayed `MoveToPos` (`0x14610`) restores hp, clears the dead bits, `+0x600`, sets `IdleAttack` and clears brain `+0x79`. `hk_MoveToPos` re-freezes only `IsFrozenMirrorAnimal` (needs `m.frozen`), and the per-frame wake check (l.1014) is never reached past `death_presented`. The copy runs the replayed `StartTask(AttackX, target "hunter")` against the **client's** hunter with its hp pinned at 0 until its fade timer parks it (`0x1f3c0` l.152–169).
- This is a-G4/b-G2 (respawn = new life) made worse: not only invisible on the client, but a short-lived ghost that can claw and tackle the client locally.
- Maps here: MAP4B lion1 (killed early, respawned at Task19 for task 4), lion2 (Task02/04 then ending), MAP5 hyenas, MAP6 leopards/bear, MAP7 tigers, MAP8 bulls.
- Fix (owner life): the a-G4 revive (`kAnimalRevive = 20`), plus in `hk_MoveToPos`: any mirror animal (not only frozen ones) → `FreezeBrain(true)` and, if `death_presented`, wait for the host's revive (keep it parked at the host position) instead of leaving it live.

### B-F8 MoveToPos re-freeze vs client cutscenes that wait on an animal task (interplay, UNVERIFIED runtime)
- a-F3 said client cutscenes with `WaitForTask(animal)` finished only because `BeginCutScene`'s `MoveToPos` thawed the copy. Since `e9f1d75`, `hk_MoveToPos` freezes it again at once. The animal's cutscene task (`AIHuntTask_PlayAnim`, `IdleWait`, `GoToEntity`) then runs only while the CutScene thaw (l.974) applies, i.e. while the **host's** copy is in `CutScene` too.
- Today (no `kTriggerHit`) a client runs a `BeginCutScene` cutscene only when it entered first; the host is then not in that cutscene and the client hangs in it (skip key escapes; every host fire is deferred meanwhile). With the planned `kTriggerHit`, both run it and the host's animal is in `CutScene` → thaw → the wait completes, provided B-F2's flag rule only excludes `+0x368` SAs.
- Maps here: MAP4B cutscenes 5/6/7 (lions 2/3 `AIHuntTask_PlayAnim` + `WaitForTask`). Others listed per map.
- Rule for triggers + life: ship `kTriggerHit` and the B-F2 thaw rule together; test MAP4B cs5 on the client.

### B-F9 Mission end, client fails (VERIFIED, `events.cpp`)
- A client's own `0x91c20` is swallowed (`hk_MissionMessage`), its `CheckMissionStatus` skipped. So a client bullet into a protected animal (`+0x604`, MAP5 elephants) or a second dart into a tranquilized protected animal fails only the client's swallowed mission: the host never fails. Rule bypass, COSMETIC. Optional host check in `OnAnimalHit`: non-tranq lethal hit on `+0x604 != 0` → `0x965e0(mission, "MISSION FAILED\nYou've killed a protected animal!")`.

---

## 1. MAP4A — truck chase (lions)

Entities (BSP, `playthrough-b-ents-MAP4A.txt`): 80; `tleader4a` id 67 (triballeader, driver), `car1` id 68 (VERIFIED runtime), `exit4b` (79) → `map4b,human`. Animals `lion1` lioness, `lion2` lion, `lion3` lioness, `lion4` lion, `lion5` lioness, `lion6` lion (predicted ids 80–85); scale reserves 90–95. Hunter: bow + knife. Mission: 1 race, 2 race, 3 kill 1 lion (no trigger on end).

### Sequence
| Beat | Script | Main today |
|---|---|---|
| Map start: cScrTask01 `Spawn` arms + `ActivateAfter(0)` → `BeginCutScene`: `tleader4a.GetInCar(1)`, `player.GetInCar(2)`, `tleader4a.DriveTo("startcar1")`, `SetMaxAttackerNo(4)`, cutscene 1 (dialogue only) | runs on every process locally | **OK** (each seats its own hunter in its own copy; client copy follows the host, `vehicles.cpp`) VERIFIED by user run |
| 01 `OnActivate`: `StartTask(1)`, arm 02 and 10 | replayed fire | OK |
| 02–08, 10 `ActivateBy("car1")`, `OnHit` = `ActivateAfter(0)` only | host truck hits → OnActivate → commit; the client copy hits ~100 ms + latency later, usually after the commit (its OnHit is then blocked by state 1; nothing lost: those OnHits only set the delay) | OK |
| Lion waves: 02 lion1+lion2, 03 lion3+lion4, 04 lion5, 06 lion6 + lion1 again, 08 lion2 again (`MoveToPos`, `CustomProps("MakeChaser")`, `AIHuntTask_AttackChasers` target hunter, `SetHealth(100)`) | replayed `MoveToPos` → `hk_MoveToPos` refreeze | Refreeze OK. Reuse of lion1 (06) / lion2 (08) after a death = **B-F7 ghost** (a-G4 still open) |
| 08 `StartTask(3)`, arm 09; 10 sets every lion `time 1.0` | | OK |
| 09 (car1) `BeginCutScene`: `REACHED 2`, cutscene 2 (dialogue); `OnActivate` → state `Activate`: `Wait(1)`, `ActivateTriggerLevel("exit4b")` | host and client copy both hit 09 (cutscene seconds long, so the client's own OnHit runs before the commit). Each process exits on its own (code 7) | a-G2 (flow): each process's own exit; clients also get `kMapStart`. Works as long as both choose MAP4B |

### MAP4A checklist
- **Movers**: `car1` host-driven, streamed (msg 11). Covered (B-F5). Client `DriveTo` neutralized.
- **Attached**: riders seated in their own copy; NPC driver attached → no NPC placement (l.1112). OK. **Animals attached by a client-side truck SA → B-F2** (the open lioness bug).
- **Shared spots**: knock-over off while riding (combat.cpp `PickVictim`). OK.
- **Wakes / second lives**: lion1 and lion2 are reused. B-F7.
- **Clip coverage**: lion.model has every state clip the cat legs use in an attack (`idleattack1..3`, `run`, `rundisabled`, `leap`, `closeattack*`, `longrangeattack*`, `hurt`, `synchurt`, `getupleft/right`, `circlerun`, `stalkidle`, `runtostalk`). It lacks `walk`, `idletowalk`, `alert`, `scare`, `lying`, `sitting` (states a chasing cat never streams). Crash risk only through B-F1/B-F2. `triballeader` = guide pack (OK).
- **Trigger spin**: fixed (`story.cpp` l.479).
- **Aggro**: riders unregistered → all lions on the host; reserves (`[Scale]`, pool LION/LIONESSE) add lions per player, all on the host. Host death = mission failed. **B-1**.
- **Kill credit**: task 3 is not needed (exit does not wait). OK.
- **F5 restart**: tracks cleared on map load; reserve ids kept (`8467e8f`). OK.

### MAP4A risks
- **B-1 (DESYNC→BLOCKS by attrition) Every chaser and every reserve goes for the host.** Owner vehicles (riding in `puppets.cpp`) + life (`scale.cpp`). Fix A (preferred): riders stay registered, `+0x368 = 1` held while `kFlagRiding` (B-F4); release on leaving the vehicle. Fix B (stopgap): `scale.cpp` no reserves while any player rides (MAP4A pool off), or riders' share of damage. Test: `-Map MAP4A -Players 3`, ride to cScrTask02/03 (no tp needed: the truck drives), grep host `combat: animal id … hurt player 1` (absent today, present with A) and host `combat: sync attack 'syncattacktruck'` only ever on the host.
- **B-2 (DESYNC, crash-prone) Client copy thaws in host SAs and attaches to the client truck** (B-F2). Owner life. Test above.
- **B-3 (DESYNC) Reused lions** (B-F7, a-G4). Owner life. Test: host `coop_kill <lion1 id>:500` after cScrTask02 fires, ride on to cScrTask06: client `animals: present death id=<lion1>` then no placement; with the fix `animals: revive id=<lion1>`.

---

## 2. MAP4B — fiancée, hippo, water crocs, stealth antelope, three lion endings

Entities 111 (`playthrough-b-ents-MAP4B.txt`): `fiancee4b` id 80 (`objects/npc/fiance`, `blowdarts,map4b`), items 86–90 (2 bow ammo, 3 medkits), triggers 91–110, no `trigger_level`. Animals (predicted ids 111–125): lion1 LION, lion2/lion3 LIONESSE, lion4 LION, lion5 LIONESSE, hippo, crocodile1–5 WATERCROCODILE, wtail1–4 PANTELOPE. `AddAnimPakForAI("lion2"/"lion3","map4b.ap")` (cs05/cs05b). Mission: 1 race, 2 bonus kill 4 antelope (flee = kill), **3 kill 2 lionesses → `ActivateTriggerOnEnd("cScrTask19")`**, **4 kill 1 lion → `ActivateTriggerOnEnd("cScrTask20")`**. Map ends in cScrTask20 `OnActivate` (CompleteTask 1–4 + CheckMissionStatus).

### Sequence
| Beat | Script | Main today |
|---|---|---|
| 01 map start: fiancée `PlayAnim idle`, `SetCanBeAttacked(0)`, arm 02–12, 14–18 | local everywhere | OK (fiancée mirror; her pack has no `idle`/walk clip → bind or held pose, COSMETIC, a-G13) |
| 02 / 04 (alternatives, each `Deactivate`s the other): lion1 + lion2 `AttackCats` | `OnActivate` | **a-G6 still open**: `OnFireRequest` (story.cpp l.348) fires a trigger the host already deactivated |
| 05 crocs 1–5 `AttackFloaters` | water crocs not mirrored | **a-G7 still open** (beasts): each process has its own crocs. Not a task |
| 06 / 07 (alternatives) `BeginCutScene`: hippo `MoveToPos` + cutscene 1/2 (hippo IdleWait); `OnActivate`: hippo `AttackChargers` | G1 (only the entering process plays the cutscene) | b-G1/a-G1 open; a-G6 |
| 08 / 10 `OnHit`: `DisableStimulus`, hunt-area `hypo1` / `white1`, hippo `time 0.1`, `SetStealthLevels`, `EnableStealth` | OnHit only on the entering process | a-G1 open (DESYNC: host's hippo keeps attacking, host flee points not toggled when a client goes first) |
| 10 antelope (wtail1–4 `IdleScare`) + `StartTask(2)` (bonus) | | OK (bonus; flee-point kill credit is the host's own Update `0x1f3c0` l.75) |
| 11 / 12 / 13: parks (`MoveToPos` + `Kill`), lion4 + lion5 `AttackCats`; 13 lion1 again (time 1000) | | parks OK; reuse B-F7 |
| **15 / 16 / 17 endings** (each deactivates the other two): `BeginCutScene` parks lion1/4/5, moves lion2/lion3, fiancée `StopAllTask`, cutscene 5/6/7 (lions 2/3 `AIHuntTask_PlayAnim` + fiancée `AITaskNPC_PlayAnim`, 3× `WaitForTask`); `OnActivate`: lion2/lion3 `AttackCats` (time 1000), arm 19, **`player.StartTask("3")`** | | G1 + **B-F8** (client cutscene waits on frozen lions) + a-G6 |
| 19 (task-3 end hit by the mission): `REACHED 1`, lion1 back (`maxdmgval 200`), arm 20, `StartTask(4)` | host mission completes task 3 only with host-credited lioness kills | **B-4 BLOCKS** |
| 20 (task-4 end): `ActivateAfter(2)` → CompleteTask 1–4 + CheckMissionStatus | | B-4 |

### MAP4B checklist
- **Movers**: none. **Traps/cages**: none (bees, cavedust, birds events only, per process).
- **Wakes / second lives**: lion1 (02/04 → 13 → 19), lion2 (02/04 → ending), wtail1–4 (10 → parked 11/13). B-F7 on lion1/lion2 when they were killed before.
- **Clip coverage**: lions as MAP4A. Hippo `HIPPO/MAP4B.AP` + model (not listed; COSMETIC only). `fiance` pack (13 clips: `defaultpose`, `cover`, `coveridle`, `hit`, `scare`, `hurtdown`, SA clips) has no `idlenormalneutral`/walk: `NpcLegClip` calls are silent no-ops (B-F1) → COSMETIC. Her `blowdarts` weapon is linked: the mirror does not drive weapon clips (l.1137), keep it so.
- **Aggro**: `target "hunter"` everywhere → host; `combat.cpp` nearest-player retarget covers lions on foot. Fiancée `SetCanBeAttacked(0)`.
- **Kill credit**: tasks 3 and 4 (**B-4**). **Tranq credit**: none (bow).
- **Scaling**: pool LION/LIONESSE: reserves do not count toward `SetTaskAnimalToShoot` (by design, `0x1e8b0` report skipped for reserves). Players must kill the script animals lion2/lion3 and lion1. Note it in the host notice? COSMETIC.
- **F5 restart**: safe (no vehicles, no exits).

### MAP4B risks
- **B-4 (BLOCKS) Lioness/lion kills by a client do not count on the host.** Task 3 (2 lionesses) and task 4 (1 lion) gate cScrTask19/20 through the mission's own end hit (a-F9). Client-only kills complete the client's own mission, whose task-end hit becomes a fire request and the host runs 19/20 (a lucky path). **Mixed kills (host 1 + client 1) complete neither mission → soft lock.** Owner life: B-F3 fix (`+0x420 = 1` in `OnAnimalHit`). Test: `-Map MAP4B`; host `coop_tp 255.2:0:143.1` (cScrTask15, plays cutscene 5); after it, host `coop_kill <lion2 id>:500` (host credit) and client `coop_kill <lion3 id>:500` (client hit → host `AnimalHit … lethal`). Bug: host log has no `trigger FIRE … 'cScrTask19'`; fixed: host `story: [host] trigger FIRE id=109 name='cScrTask19'` and client `applied host fire 'cScrTask19'`. (`coop_kill` on the host goes through `InflictDamage` too: check `+0x420` there as well, or shoot.)
- **B-5 (DESYNC) Two endings run.** If two players take two ending approaches, a-G6 runs a second ending's `OnActivate` on the host after the first: lion2/lion3 get their attack again, `trigger_19.Activate()` twice, `player.StartTask("3")` twice. Not a soft lock: `cScrHunter::StartTask` = `0x4ba50` → mission `0x964a0` → task vt `+8` = `0x99890` → `0x99260`, which sets task state `+0x68 = 2` and shows the HUD line but does **not** reset the remaining count `+0xf4` (VERIFIED, `playthrough-b-dec-shoottask*.c`). Owner triggers (a-G6: reject a fire request for a trigger that is not armed on the host and never ran there). Test: host `coop_tp 255.2:0:143.1` and, within 2 s, client `coop_tp 266.0:0:198.2` (cScrTask16). Bug: host `story: [host] ran 'cScrTask16'` after `cScrTask15` ran. Fixed: `rejected: not armed`.
- **B-6 (DESYNC/hang) Client ending cutscene waits on frozen lions** (B-F8). Owner triggers + life together. Test after `kTriggerHit`: host enters cScrTask15; client `story:` shows the cutscene ended (`PumpGather` line) within ~25 s.
- a-G1 (OnHit 08/10), a-G7 (crocs), B-F7 (lion1/lion2 ghosts), B-F2 (SA thaw): DESYNC.

---

## 3. MAP5 — Hamisi's camp, hyena surprise, truck stampede

Entities 156 (`playthrough-b-ents-MAP5.txt`): `car1` id 126 (`obj_mod_vehicle`, truck1, at 602.7:1.5:109.6), `poachera5` 127 (`obj_poarcher`, `poacher.skin,map5_poacher.ap`, shotgun), `hamisi5` 133 (`obj_guide`, blowdarts), triggers 136–155, no exit. Animals (predicted 156–159): hyena1, hyena2, elephant1, elephant2 (+ event `animal1` hyena). Hunter: `blowdarts_1st` (tranq), gets `tranqgun1_1st`. Mission: 1 race, 2 kill 1 hyena, 3 race.

### Sequence
| Beat | Script | Main today |
|---|---|---|
| 01 map start: Hamisi path-lead, arm 02, 04, 06–20, `AddAIToBeProtected("hamisi5")`; `BeginCutScene` cutscene 1 | local everywhere | OK |
| 02/03 reach: hyena2 (+ maybe hyena1, `rand`) `MoveToPos` + `AttackDogs` target hunter, `StartTask(2)` | `OnActivate` | OK (`rand` per process; positions come from the host) |
| **04 `OnHit`: `hamisi5.SetCanBeAttacked(0)`, `RemoveAIToBeProtected`**; `OnActivate` state: 5 s, hyena1 `Kill` + respawn (surprise, `rand` spot) | `OnHit` only where the hunter entered | b-G1 open: client first → the host's Hamisi stays attackable and protected; a hyena kill of Hamisi = `cObjGuide` code 6 (b-G9) → mission failed |
| 05 `OnHit`: `REACHED 1` | | resilient (Task20 completes 1–3) |
| **06 `BeginCutScene`: `CompleteTask(2)`, elephants `MoveToPos`, cutscene 3: poacher↔Hamisi `PlaySA cs03`, elephants `IdleWait`/`GoToEntity`, `WaitForTask(poacher PlayAnim)`, then `hamisi5.GetInCar(1)`, `player.GetInCar(2)`, `hamisi5.DriveTo("PathNode12")`, `player.TakeItem("tranqgun1_1st")` (the Skip state does the same)**; `OnActivate`: hint, hyenas parked, `StartTask(3)`, `SetRecoverSpeed` | boarding lives in `BeginCutScene` | **B-7 BLOCKS** (b-G1 + b-G4, below) |
| 19 (car1): elephants 1/2 `MoveToPos` + `AttackChargers` target hunter; hunt-area toggles | `OnActivate` | OK; elephants go for the host only (riders unregistered) |
| 07–17 stampede (`cStampedeTriggerMap5`, car1): `OnHit` re-arms; `UpdateTrigger` respawns elephants every 5 s while the local hunter is inside | B-F6 | the host's spawner runs (the host rides through every box). The client's spawner runs on its copies: inert except B-F7 |
| 18 (car1): elephants `time 0.1/2.0` | | OK |
| 20 (car1) `BeginCutScene`: `REACHED 3`, `CompleteTask(1,2,3)`, cutscene 4; `OnActivate`: `CheckMissionStatus` | the host truck hits first (the client copy lags 100 ms + latency) | OK → `kMissionEnd` |

### MAP5 checklist
- **Movers**: `car1` = `cMOD_ObjVehicle` → `vehicles.cpp` covers it (B-F5, VERIFIED by decompile; runtime UNVERIFIED on MAP5). Client `DriveTo` neutralized by `follow`.
- **Who boards**: each process's own `GetInCar(2)` inside cutscene 3, only on the process that ran `BeginCutScene`. **B-7**.
- **Attached**: the Hamisi driver is attached in the client copy → no NPC placement (as MAP4A).
- **Shared spots**: knock-over off while riding (OK).
- **Wakes / second lives**: hyena1 `Kill` + respawn in 04 and 06; elephants respawned by the stampede spawner (host). B-F7 if a hyena died before 04's respawn.
- **Clip coverage**: Hamisi full guide set (67 clips, `crouchidlecar` present). **Poacher `GAME.AP` = `blink` only** → every leg clip missing → silent no-ops, the poacher stays in a held or bind pose on clients (COSMETIC, b-G11). The poacher has a linked shotgun: keep the mirror off weapon clips. Elephant: no `AddSA`. Hyena SA clips `*hy_syncattack1` are in the MAP5 packs.
- **Aggro**: hyenas `target "hunter"` (nearest-player retarget works on foot); elephants: host only (B-1 pattern).
- **Kill credit**: task 2 is force-completed by 06/20 `BeginCutScene`, which run where `OnHit` ran (normally the host). Resilient as long as the host's truck reaches 20.
- **Tranq credit**: no tranq task. Client darts do nothing on the host (b-G3), so riders cannot stop elephants; the host carries the stampede. Elephants are `IsProtected`: a second dart on a sleeping elephant fails the mission (host only today, B-F9).
- **Scaling**: pool HYENA + ELEPHANT. Elephant reserves charge the host only (B-1). Recommend no elephant reserves while riders cannot be targeted.
- **F5 restart**: safe (tracks and journal cleared). It is the recovery for B-7 today.

### MAP5 risks
- **B-7 (BLOCKS) Truck boarding runs only on the process whose hunter entered cScrTask06.**
  - Client first: the client plays cutscene 3 and boards its copy, which follows the **parked** host truck. The host runs only `OnActivate`; its own `OnHit` is then blocked (state 1), so the host never boards, `DriveTo` never runs on the host, and car1 never reaches 07–20: soft lock.
  - Host first: the host boards and drives; the client's copy drives away under it; the client stays on foot at the camp without the tranq rifle, and nothing moves it (no client cutscene → no gather). Late joiners after 06 are on foot too.
  - Owner triggers (`kTriggerHit`, b-G1) + vehicles (catch-up boarding). Fix 1: `kTriggerHit` makes every process run 06's `BeginCutScene` (boarding + `TakeItem`). Fix 2 (vehicles, safety net, also for late joiners): when the host's `PlayerState` has `kFlagRiding` and the local hunter has had none for 3 s on the same map, call `cObjHunter::GetInCar(localHunter, 2)` (`0x48d10`, thiscall(hunter, seat)) and, on MAP5, the `TakeItem` native `0x4ad10` with the hunter's script object (`GetScrObject`) and `"tranqgun1_1st"` (signature UNVERIFIED: same `this+0x28` thunk pattern as `0x4b2c0`). Log `vehicles: catch-up boarding seat 2`.
  - Test: `-Map MAP5 -Capture`. Client `coop_tp 605.9:1:135.8` first (host stays at spawn 471.6:0.6:150.5). Bug: client `story: [client] trigger FIRE … 'cScrTask06'` after its cutscene, host `story: [host] ran 'cScrTask06' … for player 1`, then no host `vehicles: ride` lines and never a `cScrTask19` fire. Second run, host first: the client log has no `vehicles: ride id=126` line. Fixed: both logs show `vehicles: ride id=126 hunter … m` within ~35 s of the fire, then `story: … cScrTask19`.
- **B-8 (DESYNC → mission fail) Task04 `OnHit` on the wrong process keeps Hamisi attackable and protected on the host** (b-G1, b-G9). Owner triggers. Test: client `coop_tp 538.5:2:146.0` first; host `animals: sync role=host id=133 … hp=` drops when hyenas bite him; no such drop when the host enters first.
- B-1 (host-only chargers), B-F2 (hyena SA thaw), B-F7 (hyena reuse), b-G3 (darts): DESYNC.

---

## 4. MAP6 — Reggie and the rangers, tigers and a bear, tranq rifle only

Entities 194: `redshirt1` 156 (`ausranger1`), `redshirt3` 157 (`ausranger3`), `reggie6` 158 (`reginald`, `radioNPC`), triggers 182–193. Animals: leopard1–4 = `ANIMAL_BTIGER` (`IsProtected`, `SetTranqHurt(0)`, `SetTranqTime(2)`), `abear` (+ `map6.ap`), event `animal1` BTIGER, 10-cage pool (`EVENT_CAGE`), bees, birds. Hunter: `tranqgun1_1st` only. Mission: 1 race, 2 kill 1 BTIGER, 3 kill 2, 4 kill 1 ABEAR, 5 kill 2, 6 race, **7 kill 4 BTIGER**, **8 kill 1 ABEAR**.

### Sequence
| Beat | Script | Main today |
|---|---|---|
| 01 map start: protect Reggie, `SetCanBeAttacked(0)` Reggie/redshirt3, arm 02, 10, 100, 101; cutscene 1 (Reggie `PlayAnim` + NPC wait) | local | OK |
| 02 `BeginCutScene`: leopard1 `MoveToPos` + **cutscene 4** (leopard `AIHuntTask_PlayAnim intro` + `WaitForTask`); `OnActivate`: leopard1 `AttackCats`, `StartTask(2)` | | b-G1; B-F8 on a client that runs cs4 |
| 03/04/05/06 `OnActivate`: `CompleteTask(2/3/4/5)`, next wave (leopard1 reused 02→05→07, leopard2/3 03→05→07, abear 04→08) | | resilient (force-completed on the host by the replayed or own fire) |
| 04 `BeginCutScene`: abear + cutscene 5 (IdleWait, no wait) | | b-G1 |
| 06 `BeginCutScene`: cutscene 2 parks all leopards and the bear (`MoveToPos` + `Kill`), Reggie `PlayAnim` + wait | | parks OK (`Kill` in the same frame ends any B-F7 ghost) |
| 07 `OnHit` `REACHED 1`; `OnActivate`: `StartTask(7)`, leopards 1–4 surprise attacks (`time 10000`) | | resilient (09 completes 1–9) |
| 08 `BeginCutScene`: `REACHED 6`, Reggie `DenyTask`, abear `MoveToPos`, **cutscene 3** (redshirt1 `PlayAnim` play_sa reggie6; **bear `AIHuntTask_PlaySA` target redshirt3 + `WaitForTask`**); `OnActivate`: bear `AttackBears` **target `redshirt1`**, `StartTask(8)` | | B-F8 (cs3 needs the CutScene thaw while the client's camera is up); redshirt1 can die → `cObjGuide` code 6 (b-G9) |
| **09 `WaitTask(8)`, `WaitTask(7)` (local mission)**; `OnActivate` state: `CompleteTask(1..9)`, Reggie talk (NPC waits), `CheckMissionStatus` | | **B-9 BLOCKS** |
| 10: `OpenMenuPage(11)` | replayed on every client wherever it is | a-G14 open (COSMETIC; pause UNVERIFIED) |

### MAP6 checklist
- **Movers**: none. **Cages**: `EVENT_CAGE` pool (10) on 22 static spots, per process; a cage hurts its `+0x214` target on that process only (b-G13, COSMETIC). Puppets never get caged.
- **Wakes / second lives**: leopard1–3 and abear are reused across waves; tranq "kills" fade and park, then respawn: B-F7 + a-G4 on every wave. Worst map for G4 after MAP9.
- **Clip coverage**: btiger and abear models (SA `pl_t_*`, `t_syncattack*`); reginald and ausranger1 full; **ausranger3** lacks side/back walks, `fall`, `conversation`, `point`, `followme`, `cover*` → silent (COSMETIC). Crash risk only through B-F1 links (the cs3 bear↔redshirt3 SA on a client that half-ran it).
- **Aggro**: tigers `target "hunter"`; the bear targets `redshirt1` (scripted NPC target, never retargeted). Reggie and redshirt3 cannot be attacked.
- **Kill / tranq credit**: tasks 7 and 8 need host-credited darts (B-9). Tasks 2–5 are force-completed.
- **Co-op overdose**: tigers are `IsProtected`. Once darts are host-applied (B-F3), two players darting the same tiger within its 2 s tranq window = "overdose" = mission failed for everyone. **Design decision for the user**: the host ignores a second dart from another player while `+0x4f0 == 2` (proposed default), or keep the single-player rule.
- **Scaling**: pool BTIGER. Reserves do not count toward task 7: players must dart the four script tigers; reserves only add danger. Suggest a host notice ("extra animals do not count") or no reserves on tranq maps.
- **F5 restart**: safe.

### MAP6 risks
- **B-9 (BLOCKS) Tasks 7/8 complete only on the mission that got the darts.** `cScrTask09`'s `OnHit` is gated by `WaitTask(7)`/`WaitTask(8)` on the **local** mission (a-F2). Client darts never reach the host (b-G3); on the client they make local fake kills (B-F3) that complete the client's own tasks.
  - Host-only darting: OK. Client-only darting: the client's `OnHit` passes, its fire request makes the host run `OnActivate` (a fire does not check `WaitTask`) → the map ends: OK by luck. **Mixed darting (host 2 + client 2 of task 7): neither mission reaches 4 → soft lock.**
  - Owner beasts (`kTranqHit`, B-F3), with life (credit). After the fix the client's own mission never completes 7/8, so the **host** must walk into cScrTask09 (or `kTriggerHit` must let the host test `WaitTask` on its own mission when a client enters).
  - Test: `-Map MAP6`. Host `coop_tp 154.1:0:-76.0` (02, cs4 plays on the host), then host `coop_trigger cScrTask03`, `cScrTask04`, `cScrTask05`, `cScrTask06` (each arms the next; call again when the log says `-> arm`), then host `coop_tp -42.1:8:68.3` (07). Dart two of leopard1–4 from the host window and two from the client window (needs the user, or a new `coop_dart <id>` that runs the host tranq apply / the client request; beasts). Bug: no `CompleteTask 7` on the host and host `coop_tp -126.5:9:-10.0` gives no `cScrTask09` fire. Fixed: host `animals: tranq hit id=<leopard> from player 1 -> asleep`, then `CompleteTask` and `cScrTask09` lines.
- **B-10 (client hang) Cutscenes 3/4 wait on animal tasks** (B-F8). Owner triggers + life. Test after `kTriggerHit`: host `coop_tp 154.1:0:-76.0`; the client's cutscene camera ends (`PumpGather` / gather line) within 20 s and `defer host fire` lines stop.
- b-G9 redshirt1 death, a-G14 menu page, B-F2 (tiger SAs), B-F7: DESYNC.

---

## 5. MAP7 — find the girl, tigers, a cobra, a rhino, tranq rifle only

Entities 234: `indie_girl` 194 (`obj_girl`, `indiangirl`), `reggie7` 195 (`reginald`), `obj_trap capcana1` 209, 3 medkits, triggers 214–232, unreferenced `trigger_level cutscene_outro` 233 (`map8a`). Animals: tiger1–4 BTIGER (`IsProtected`, tranq 2 s), rhino1 (+ `map7.ap`), cobra1 (`cObjCobra`); events: trap pool, snake, birds, bees, cages, `animal1` BTIGER. Hunter: `tranqgun1_1st` only. Mission: **1 FIND child**, 2 kill 1 BTIGER, **3 kill 2 BTIGER**, 4 kill 1, 5 kill 2, **6 kill 1 cobra → `ActivateTriggerOnEnd("cScrTask08")`**, 7 race, **8 kill 1 rhino (flee = kill) → `ActivateTriggerOnEnd("cScrTask10")`**, 9 race. cScrTask08 (-230.4:1:197.3) and cScrTask10 (-174.4:5.3:198.0) sit next to the map-start box, away from play: only the mission's task-end hit (a-F9) reaches them.

### Sequence
| Beat | Script | Main today |
|---|---|---|
| 01 map start: protect Reggie, Reggie/girl `SetCanBeAttacked(0)`, `trap1.MoveToPos("capcana3")`, arm 02–04, 101–107; cutscene 1 | local | OK (trap per process, b-G13) |
| 02 tiger1; 03 `CompleteTask(2)`, tiger2/3, `StartTask(3)` | | OK |
| **04 `BeginCutScene`: `CompleteTask(3)`, `escape1`, tigers 1–3 `time 1.0`, cutscene 2**; `OnActivate`: Reggie, arm 05, tiger4 (`rand` boat spot) | | **B-12** (below) |
| 05 / 12 `OnActivate` states: arms 06, 12, 07; Reggie NPC waits; 12 `CompleteTask(4,5)` | replayed; the client's copies of these states wait on frozen NPC tasks (harmless: arms come from host commits) | OK |
| **07 `OnHit`: `UpdateMissionStatus(FIND_SOMETHING, AI_CHILD_ID)`** — the only completion of task 1; state: cobra `Task_CobraAttack` + `MoveToPos`, `StartTask(6)`, arm 08, girl look + wait | | **B-11 BLOCKS** (b-G1) |
| task 6 end (cobra) → mission hits 08: Reggie denies, `StartTask(7)`, arm 09, girl `PlayAnim jump` + wait | | **B-13** (cobra credit) |
| 09 `BeginCutScene`: rhino `MoveToPos`, **cutscene 6 (rhino `AIHuntTask_PlaySA` target reggie7 + `WaitForTask`)**; `OnActivate`: unprotect Reggie, `StartTask(8/9)`, arm 10/13, `escape3`, rhino `AttackChargers` target hunter | | b-G1; B-F8 on a client that runs cs6 |
| task 8 end (rhino dart, or rhino reaches `escape3` = kill) → mission hits 10: arm 11, NPCs run to `girloutro2` | | host-credited (see B-12 note) |
| 11 `OnHit` `REACHED 9`; state: `CompleteTask(7,8,9)`, 3 s, `CheckMissionStatus` | needs tasks 1–6 complete on the host | ends only if B-11/B-12/B-13 passed |

### MAP7 checklist
- **Movers**: none. **Traps**: BSP `obj_trap capcana1` + `EVENT_TRAP` pool; `cObjTrap::OnCollision` reacts only to `obj_player` (puppets never), the host's Reggie can be trapped on the host only (b-G13). Snake event (`cObjSnake`, not mirrored, per process). Cages, bees: per process.
- **Wakes / second lives**: tiger1 (02→05), tiger2/3 (03→06), tiger4 (04). Darted tigers fade, park and respawn: B-F7 + a-G4.
- **Clip coverage**: `indiangirl` lacks `idlenormalneutral`, side/back walks, `fall`, `liedead`, `hurt` (has `idle`, `walk/forward`, `run/forward`, `jump`, `point`, `conversation`): silent no-ops (B-F1), COSMETIC. Reginald full. Rhino model + `map7.ap` (cs03): cs6's rhino↔Reggie SA links the copies on a client that runs it (B-F1/B-F2 repair applies).
- **Aggro**: tigers and rhino `target "hunter"` → host, nearest retarget on foot; cobra bites the host only (`CobraBitesNearest=0`, b-G12).
- **Kill / tranq credit**: tasks 3, 6 and 8 need host credit unless their trigger ran on the host (B-12, B-13). Tasks 2, 4, 5, 7, 9 are force-completed.
- **Running-task rule (VERIFIED)**: the mission update `0x94ba0` offers every action to every started task in the list at mission `+0x20c` (`StartTask` `0x964a0` adds it there) until one finishes. A kill task that the script did not force-complete keeps counting later kills of its species, so the host can still finish task 3 later with two host-credited darts.
- **Scaling**: pool BTIGER (reserves do not count).
- **F5 restart**: safe.

### MAP7 risks
- **B-11 (BLOCKS) "Find the girl" completes only in cScrTask07's `OnHit`** on the process whose hunter entered (b-G1). Client first → the host's task 1 never completes; nothing else completes it; cScrTask11's `CheckMissionStatus` never succeeds. Owner triggers (`kTriggerHit`). Test: `-Map MAP7`; client `coop_tp 143.8:1:-37.9` (the girl, cScrTask07; it is armed by 05's state, so first host `coop_trigger cScrTask05` twice if needed). Bug: client `story: [client] trigger FIRE … 'cScrTask07'`, host `ran 'cScrTask07' for player 1`, and no host `CompleteTask`/find line; later host in cScrTask11 (`coop_tp 48.6:0:63.1`) → no `events: host mission message code=5`. Fixed: host `kTriggerHit` commit for cScrTask07 and the success message.
- **B-12 (BLOCKS) Task 3 force-completion lives in cScrTask04's `BeginCutScene`.** Client first → the host's task 3 stays open and needs two host-credited BTIGER darts later; with client-only darting (b-G3) it never completes → no success at 11. Owner triggers (`kTriggerHit`) and beasts (`kTranqHit`). Test: client `coop_tp -146.3:1:-31.8` first; host log no `CompleteTask 3` (`story: [host] CompleteTask` lines come from `hk_CompleteTask`).
- **B-13 (DESYNC; BLOCKS if the host never darts it) Client hits on the cobra never reach the host.** `cObjCobra::OnHit` `0x80440` changes no health, so `ApplyMirror`'s health-drop detection sends no `AnimalHit` (the cobra path in `OnAnimalHit` exists but only `coop_kill` feeds it); the client copy shows `Death` and the mirror puts it back. Task 6's end is what fires cScrTask08. Owner beasts: client hook `0x80440` (prologue `55 8b ec 6a ff` VERIFIED): on a mirror cobra, after the original, `ReportAnimalHit(id, 1.0)` once (host `StartCobraDeath` already handles it). Also MAP3B / MAP3C cobras (research A). Test: client darts cobra1 after 07 (needs input or a `coop_dart`); host `animals: AnimalHit cobra id=<cobra>` absent today.
- B-F8 (cs6 rhino SA wait), B-F7, B-F2, b-G12: DESYNC.

---

## 6. MAP8 — find the guide, three tribesmen, scrub-bull stampede, crocodiles

Entities 175: tribesman1–3 125–127 (`abo1..3`, `obj_guide`), props `rockt1`/`atree`/`fxtree` 128–130, ammo + medkits 136–143, triggers 144–169 and 174, `trigger_hurt` ×4 170–173. Animals: crocodile1–3 `ANIMAL_CROCODILE` (mirrored), croco1–3 `ANIMAL_WATERCROCODILE` (not mirrored), scrubbul1–5; `EVENT_FIRE`. Mission: **1 FIND guide**, 2–5 kill 1 bull each, **6 race**, 7–9 kill 1 bull each, **10 race**. End: cScrTask15 (-65.1:2.4:-94.3, next to the spawn -25.0:0.7:-70.0).

### Sequence
| Beat | Script | Main today |
|---|---|---|
| 01 map start: arm everything, hunt-area points off, props `SetAnim idle`, tribesmen unattackable/deny | local | OK |
| 02 `BeginCutScene`: scrubbul1 `MoveToPos` + **cutscene 4 (bull `AIHuntTask_PlayAnim` + `WaitForTask`)**; 03/04/05 bulls 2/3/1 `AttackChargers` | | b-G1; B-F8 |
| 69 / 06 `OnActivate`: `CompleteTask(2..5)`; 69 `rockt1.SetAnim("ani1")`; 06 `SetDenySATime(0.1)`, land crocs `AttackTraps`; 06 `BeginCutScene`: crocodile2 + **cutscene 5 (croc `PlayAnim` + wait)** | | resilient; B-F8; `kPropAnim` OK; b-G14 |
| 07 water-croc spawner (`OnHit` re-arms; `UpdateTrigger` every 8 s while the local hunter is inside) | water crocs are per process | a-G7/b-G6 open (each player has own crocs) |
| **08 `OnHit`: `UpdateMissionStatus(FIND_SOMETHING, AI_GUIDE_ID)`** (only completion of task 1); `BeginCutScene` cutscene 2 (tribesmen NPC tasks + many `WaitForTask`); `OnActivate`: `StartTask(6)`, tribesmen `COMPANION "hunter"` | | **B-14 BLOCKS** (b-G1); companions follow the host only (b-G7) |
| 09/10/11: bulls, `StartTask(7/8/9)`, arm 12–14 | | OK |
| 12/13/14/23/24 stampede (`cStampedeTriggerMap8`): `OnHit` re-arms, 12 `SetMaxAttackerNo(5)`; `UpdateTrigger` toggles hunt-area points and respawns bulls (`rand`, `CanBeRespawned`) while the local hunter is inside | | b-G8 open: a client alone in the zone gets no stampede on the host; `SetMaxAttackerNo(5)` only where `OnHit` ran |
| 16 props `atree`/`fxtree` `SetAnim`, arm 11; 17 `StartTask(10)`, `SetDenySATime(15)`, tribesmen attackable + `IDLESCARE` | | b-G14 (deny only for the host's own hunter); tribesmen deaths = code 6 (b-G9) |
| **15 `OnHit`: `CompleteTask(6..9)`, `REACHED 6`, `ActivateAfter(1)`; `BeginCutScene`: `CompleteTask(10)`, `REACHED 10`, bulls `time 1.0`, companions, cutscene 3; `OnActivate` state: `CheckMissionStatus`** | | **B-15 BLOCKS** (b-G1) |

### MAP8 checklist
- **Movers**: none (props are `SetAnim` only). **Hazards**: `trigger_hurt` ×4 and fire FX per process; puppets do not get the host's trigger damage forwarded unless `ForwardPuppetHurt` sees it (a-G16).
- **Wakes / second lives**: scrubbul1 (02→05→09), scrubbul2 (03→10), scrubbul3 (04→11), all five recycled by the stampede: B-F7 + a-G4 (worst on this map after MAP6).
- **Clip coverage**: abo1–3 full guide set (332 clips). Scrub bull SA `pl_sb_*` / `sb_*` (4 directions); land croc `cr_syncattackcroc`.
- **Aggro**: bulls `target "hunter"` (nearest retarget on foot); water crocs per process; tribesmen ignore animals after 17.
- **Kill credit**: none needed (2–5 by 69/06, 6–9 by 15's `OnHit`), as long as the host runs 15's `OnHit` (B-15).
- **Scaling**: pool SCRUBBULL; the stampede respawns only the five script bulls, reserves come with each wave.
- **F5 restart**: safe.

### MAP8 risks
- **B-14 (BLOCKS) "Find the guide" completes only in cScrTask08's `OnHit`** (b-G1). Owner triggers. Test (audit-b recipe): `-Map MAP8`; client `coop_tp 58.4:0:223.1` first. Bug: host `ran 'cScrTask08' for player 1`, the host HUD keeps task 1; at the end host `coop_tp -65.1:1:-94.3` gives no success. Fixed: host `kTriggerHit` commit, success.
- **B-15 (BLOCKS) The end trigger completes tasks 6–10 in `OnHit`/`BeginCutScene`.** cScrTask15 is next to the spawn, so whoever walks back first decides; client first → the host runs only the `CheckMissionStatus` state with tasks 6 and 10 (race tasks, nothing else completes them) open → no success. Probability ~1/2 per 2-player run. Owner triggers. Test: client `coop_tp -65.1:1:-94.3` after 17 has fired (host `coop_trigger cScrTask17` twice); host log `events: client CheckMissionStatus` is not relevant here — look for host `CompleteTask 6` (absent) and no `host mission message code=5`.
- b-G7 companions follow the host, b-G8 stampede spawner, b-G9 tribesman death, a-G7 water crocs, b-G14 deny-SA, B-F8 (cs4/cs5 animal waits), B-F2 (bull SAs): DESYNC.

---

## 7. Animal clip coverage (MAP4A–MAP8 species)

Clip names read from each species' `.model` (the `.skin` header names it; `lionesse.skin` uses `lion/lion.model`). Missing on the model itself = silent wrong pose (B-F1), same on host and client. Crash only when the clip **exists** on the copy and is forwarded to a linked partner that lacks it.

| Species (maps) | Model | Has | Lacks (generic leg-state clips) |
|---|---|---|---|
| lion / lioness (4A, 4B) | lion.model, 34 | `idleattack1..3`, `idle1..3`, `run`, `rundisabled`, `leap`, `circlerun`, `hurt`, `synchurt`, `getupleft/right`, `syncattacktruck`, `pl_ln(s)_*` | `walk`, `idletowalk`, `walktoidle`, `scare`, `alert` |
| btiger (6, 7) | btiger.model, 47 | idle/attack/getup set, `pl_t_*` (4 dirs) | `walk*`, `scare`, `alert` |
| rhino (7) | rhino.model, 21 | `idleattack*`, `closeattack1..4`, `run*`, `getup*`, `synchurt`, `pl_r_*` | `idle1`, `walk*`, `leap`, `circlerun` |
| hippo (4B) | hippo.model, 21 | as rhino | `idle1`, `walk*`, `leap`, `circlerun` |
| scrub bull (8) | scrubbull.model, 24 | `idleattack*`, `intro`, `closeattack1/2`, `run*`, `getup*`, `synchurt`, `pl_sb_*`/`sb_*` | `idle1`, `walk*` |
| hyena (5) | hyena.model, 26 | | `idle1`, `walk*` |
| asiatic bear (6) | abear.model, 24 (+ `map6.ap` cs03) | | `walk*`, `synchurt`, `leap` |
| elephant (5) | elephant.model, 10: `run`, `rundisabled`, `attack1/2`, `idletorun`, `dizzy`, `idle`, `hurt`, `hurt2` | | `idleattack1` (IdleAttack, IdleWait, CutScene states), `runtoidle`, `getup*` → poses wrong on every process, never fatal (no SA, no link) |
| land crocodile (8) | crocodile.model, 9: `intro`, `idle`, `hurt`, `attack1a/b`, `run`, `*cr_syncattackcroc` | | `idleattack1`, `idletorun`, `runtoidle`, `getup*` |
| water crocodile (4B, 8) | crocodile_w.model, 9 | | as land croc (not mirrored anyway) |
| pantelope (4B) | pantelope.model, 28 | | transitions, `getup*` |

NPCs (`ap_list.py` on `OBJECTS/NPC/<name>/GAME.AP`): hamisi (67), reginald / ausranger1 / abo1–3 / triballeader (332) have every `NpcLegClip` name. poacher (`blink` only), fiance (13), indiangirl (16), ausranger3 (25) lack most → silent COSMETIC. Weapons linked with `LinkPart` (Hamisi, fiancée blowdarts; poacher shotgun; Reggie radio) are the only NPC crash path: keep the mirror off weapon clips (as today).

---

## 8. Old audit gaps: state on `main` (my maps)

| Gap | State now | Maps here |
|---|---|---|
| a-G10 MAP4A truck | **FIXED** (`e9f1d75`: one host truck, riders seated) — new open items B-1, B-2 | 4A |
| b-G4 MAP5 truck not synced | **Half fixed**: `vehicles.cpp` covers `cMOD_ObjVehicle` (B-F5). Boarding still per process (B-7) | 5 |
| b-G10 / audit state-2 spin | **FIXED** (`story.cpp` l.479) | all |
| a-G5 / b-G2 wake part | **FIXED for frozen live copies** (`hk_MoveToPos`, per-frame check). Not for dead copies (B-F7) | all |
| a-G4 / b-G2 respawn = new life | OPEN (life) | 4A, 4B, 5, 6, 7, 8 |
| a-G1 / b-G1 `OnHit` + `BeginCutScene` | OPEN (triggers) — the main blocker family: B-7, B-11, B-12, B-14, B-15 | 4B, 5, 6, 7, 8 |
| b-G3 tranq | OPEN (beasts) — B-9, B-12; plus client fake kills (B-F3) | 5, 6, 7 |
| scale "kill credit action 0" | OPEN (life) — B-4 | 4B (also 4A task 3, harmless) |
| b-G5 host runs `OnActivate` twice | OPEN (`OnFireRequest` fires while the host's own state is 2–5; `hk_TriggerFire` host path does not check `g_ran_fire`) | all |
| a-G6 alternatives fire twice | OPEN (`OnFireRequest` has no armed check) — B-5 | 4B |
| a-G7 / b-G6 water crocs | OPEN (`player_tracker.cpp` l.332 matches only `0xff470`/`0x10e920`) | 4B, 8 |
| b-G7 companions | OPEN (`NpcFollowNearest=0`) | 8 (and 6/7 Reggie path-lead) |
| b-G8 `UpdateTrigger` spawners | OPEN | 5 (moot while riding), 8 |
| b-G9 / a-G12 guide death code 6 | OPEN (works while the profile-health mirror arrives) | 4A, 4B, 5, 6, 7, 8 |
| b-G11 / a-G13 NPC clips | Reframed: silent, COSMETIC (B-F1) | 4B, 5, 6, 7 |
| b-G12 cobra bites host | OPEN (`CobraBitesNearest=0`); new B-13 (client hits on cobras never reach the host) | 7 |
| b-G13 per-process hazards | OPEN, COSMETIC | 6, 7, 8 |
| b-G14 / audit `SetDenySATime` local | OPEN; field is hunter `+0x67c` (`0x4bf60`) | 8 |
| a-G2 exits per process | OPEN (flow); only MAP4A exits here | 4A |
| a-G14 `OpenMenuPage` replay | OPEN | 6 |

---

## 9. Cross-reference (same pattern, other maps in my range)

- **Completion only in `OnHit`/`BeginCutScene` (b-G1 BLOCKS)**: MAP5 06 (boarding), MAP7 07 (find), MAP7 04 (task 3), MAP8 08 (find), MAP8 15 (tasks 6–10). DESYNC-only: MAP4B 08/10, MAP5 04/05, MAP6 07/08, MAP7 11, MAP8 12.
- **Kill-gated triggers (mission task end → trigger)**: MAP4B 19/20 (lions), MAP7 08 (cobra), MAP7 10 (rhino). `WaitTask` on the local mission: MAP6 09.
- **Tranq-only credit**: MAP5 (none needed), MAP6 tasks 7/8, MAP7 tasks 3/6/8.
- **Client cutscene waits on an animal task (B-F8)**: MAP4B cs5/6/7, MAP6 cs3/cs4, MAP7 cs6, MAP8 cs4/cs5.
- **Animal sync attacks (B-F2)**: every map here; truck SAs on MAP4A (and MAP5 if hyenas reach the truck).
- **Reused animals (B-F7 / a-G4)**: MAP4A lion1/lion2, MAP4B lion1/lion2/wtail, MAP5 hyena1, MAP6 leopards/bear, MAP7 tigers, MAP8 bulls.
- **Self-re-arming spawner triggers (B-F6 / b-G8)**: MAP5 07–17 (elephants), MAP8 07 (water crocs), 12/13/14/23/24 (bulls).
- **Vehicles**: MAP4A, MAP5 (same class, same code path).
- **Cobras**: MAP7 (task); MAP3B/MAP3C in research A's range (B-13 applies there too).

---

## 10. Ranked list for a 2–4 player run MAP4A → MAP8

| # | Sev | Item | Maps | Owner |
|---|---|---|---|---|
| 1 | BLOCKS | b-G1 `OnHit`/`BeginCutScene` not replicated: B-7 (truck boarding), B-11, B-12, B-14, B-15 | 5, 7, 8 | triggers (+ vehicles safety net for B-7) |
| 2 | BLOCKS | Client kills report action 0 (B-F3) → B-4 lion tasks gate the MAP4B end | 4B | life |
| 3 | BLOCKS | Client darts do nothing on the host, and fake-kill locally (B-F3, b-G3) → B-9 (MAP6 tasks 7/8), B-12 | 6, 7 | beasts (+ life credit) |
| 4 | DESYNC, crash-prone | Copies thaw during host SAs (B-F2): local attacks, truck attach, stale links → "Could not forward animation" (the open MAP4A lioness) | all | life |
| 5 | DESYNC → host deaths | Riders are not targets; all chasers and reserves go for the host (B-1) | 4A, 5 | vehicles (+ life `scale.cpp`) |
| 6 | DESYNC | Reused animals: invisible on clients, live ghosts with hp 0 (B-F7, a-G4) | all | life |
| 7 | client hang | Client cutscenes that wait on an animal task (B-F8) — appears when #1 lands | 4B, 6, 7, 8 | triggers + life together |
| 8 | DESYNC (BLOCKS if host never acts) | Client hits on cobras never reach the host (B-13) | 7 (3B, 3C) | beasts |
| 9 | DESYNC | Two alternative endings run (a-G6, B-5); `OnActivate` twice (b-G5) | 4B (all) | triggers |
| 10 | DESYNC | Water crocs per process (a-G7), companions follow host (b-G7), spawners host-presence (b-G8), Hamisi protection (B-8) | 4B, 5, 8 | beasts / hunter / triggers |
| 11 | rules | Co-op overdose on protected tigers once darts are shared (design) | 6, 7 | beasts + user decision |
| 12 | COSMETIC | NPC/elephant missing clips, poacher pose, menu page replay, per-process traps/cages | 4B–8 | — |

F5 (restart through the main menu) is the recovery for every BLOCKS item above; no map in this range keeps state across a restart (no checkpoints; vehicle tracks and reserves rebuilt on load).

---

## Routing
- **triggers** (`story.cpp`): `kTriggerHit` (hook `0x6b140`, vt `+0x8c`, prologue VERIFIED) so every process runs `OnHit` + `BeginCutScene` — fixes B-7 (MAP5 boarding), B-11/B-12 (MAP7), B-14/B-15 (MAP8), B-8 (MAP5 Hamisi); host decides `WaitTask` on its own mission (MAP6 09); a-G6 armed check (B-5); b-G5 pending-state guard; ship together with life's thaw rule (B-F8).
- **life** (`animals.cpp`, `scale.cpp`): `+0x420 = 1` before a lethal client hit (B-4); CutScene thaw only during a local cutscene + stale-SA repair (B-F2, B-2); revive = new life and refreeze dead copies in `hk_MoveToPos` (B-F7, B-3); no elephant/lion reserves while riders cannot be targeted (B-1 stopgap).
- **beasts** (`beasts.cpp`): `kTranqHit` (client hook `0x1a680`, host applies `+0x4f0 = 2`, `+0x420 = 1`, overdose via `0x965e0`) (B-9, B-12, MAP5); cobra client hits via `0x80440` → `AnimalHit` (B-13); water crocs `0xffa50` (a-G7); optional protected-kill check (B-F9); `coop_dart <id>` test command.
- **vehicles** (`vehicles.cpp`, riding in `puppets.cpp`): riders stay attack targets with `+0x368` held while riding (B-1); catch-up boarding `0x48d10` + `TakeItem` `0x4ad10` when the host rides and we do not (B-7, late join).
- **hunter**: `NpcFollowNearest` test on MAP8 tribesmen (b-G7); `SetDenySATime` for puppets (b-G14, field `+0x67c`).
- **flow**: MAP4A exit per process (a-G2); guide/NPC death code 6 commit (b-G9) for Hamisi, Reggie, redshirts, tribesmen.
- **lead**: correct MEMORY "lioness has no idleattack1"; `animal_flag` bit 4 if life wants `kInSyncAttack`; user decision on co-op overdose (B-F3 / MAP6 checklist).

## Proposed MEMORY facts
- "Could not forward animation %s from %s to %s model %s" is raised only by `cModelPart::SetAnim(int,float,float)` (EngineDll7 `0x1e73b0`) while forwarding a clip through the part's link ring (`part+0xf4`, `SetSyncWith` `0x1e7050`) to a linked part whose model has neither `"<part>/<clip>"` nor `"<clip>"`. A clip missing on the part's own model returns 0 silently. — decompile `re/notes/playthrough-b-dec-engine1.c`.
- Correction: the MAP4A lioness **has** `idleattack1`: `lionesse.skin` uses `objects/animals/lion/lion.model`, which lists `idleattack1..3`, `syncattacktruck`, `pl_ln(s)_syncattack1`. The MAP4A failures were forwards through a stale sync link. — FBMF model name lists.
- Animal TryPlaySA (CommonDll7 `0xcadf0`) needs animal and target `+0x368 != 1`, task `+0x144 != 1`, target vt `+0x218` (CanPlaySA, base `0x14a60` returns 1); kind `"truck"` when target vt `+0x1c0` (GetCar `0x1bc50`) finds a vehicle and the target is the player. It links target part 0 to the animal's part 0 (vt `+0x38`), calls `+0x210` on both, and for a truck SA `cObjEntity::Attach`es the animal to the vehicle tag `place_<seat>`. Only the SA end `0xc3960` unlinks, calls both `+0x214`, and `Detach`es (EngineDll7 `0xb8d20`). — `playthrough-b-dec-common1/2.c`.
- `cObjCharacterEx::PlaySA(char*, m4x4&)` (CommonDll7 `0x27f10`) sets `+0x368 = 1` and every part to `CutScene`; so a host animal in an attack SA streams state `"CutScene"`. `PlaySA(char*, char*)` (`0x1a7a0`) looks the partner up by name and calls the m4x4 version on both. — same.
- Present death `0x1e8b0` reports `0x91e80(mission, *(ent+0x420), species +0x434)` unless `+0x500 == 2`, then clears `+0x420`. Only bullet `OnHit` `0x1a680` writes `+0x420 = 1`. Shoot task update (`TASK_SHOOT` vtable `0x111c0c` slot `+0x10` = `0x999a0`) decrements remaining `+0xf4` only for action 1 and species `+0xf0`. Mission update `0x94ba0` offers each action to every started task (list at mission `+0x20c`). — `playthrough-b-dec-game1.c`, `-shoottask*.c`, `-missionupd.c`.
- Tranq: `cScrAnimal::Update` `0x1f3c0` counts `+0x4f4` down while `+0x4f0 == 2` and then calls vt `+0x154` (present death) — the tranq sleep is the kill, credited with the dart's `+0x420 = 1`. Tranq hurt `+0x4f8` = script `SetTranqHurt` or 0. BTIGER and ELEPHANT are `IsProtected` (`+0x604`): a non-tranq bullet or a second dart while `+0x4f0 == 2` fails the mission in `OnHit`. — `0x1f3c0` l.134, `0x1a680`, `BTIGER.SCR`, `ELEPHANT.SCR`.
- `cScrHunter::StartTask` = `0x4ba50` → mission `0x964a0` → task vt `+8`; for a shoot task (`0x99890` → `0x99260`) it sets state `+0x68 = 2` and does not reset the count. `SetDenySATime` = `0x4bf60` (hunter `+0x67c`). `GetInCar` native `0x4b2c0` → `cObjHunter::GetInCar` `0x48d10` thiscall(char, seat); `DriveTo` native `0x4b3d0` → `0x48e40` (first `cMOD_ObjVehicle`, `cObjVehicle::DriveTo`, route `veh+0x3a0`). — `playthrough-b-dec-natives.c`, `-starttask.c`, `-game1.c`.
- `cMOD_ObjVehicle::Update` (`0x3b710`) calls `cObjVehicle::Update` first (so the CommonDll export hook covers MAP4A and MAP5 trucks), then hurts the seated player at `+0x29c` by 100 when the truck tips. `OnCollision` `0x3b8c0` calls a `trigger_event`'s vt `+0x8c` with the vehicle. MAP5 `car1` = BSP index 126, same `obj_mod_vehicle truck1` as MAP4A. — decompile, BSP lump.
- Prologue `55 8b ec 6a ff` at GameDll7 `0x80440` (cObjCobra::OnHit), `0x1a680`, `0x48d10`, `0x4ad10`, `0x1e8b0`, `0x965e0`. — `playthrough-b-bytes.txt`.

