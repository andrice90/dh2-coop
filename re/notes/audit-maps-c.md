# Audit: MAP11A, MAP11B, MAP11C, MAP12A, MAP12B, MAP12C, MAPARENA, MAPISLAND, MAPTSAVO (job `audit-maps-c`, 2026-10-17)

Research only: no code edited, no game launched. Sources: `re/extracted/SCRIPTS` (WORLD/MISSION/TRIGGER/CUTSCENE/MODELS), BSP entity lumps (`re/extracted/BSP/*.BSP`, parsed with a scratch script equivalent to `re/scripts/scan_props.py`), teen paks via `re/scripts/ap_list.py`, Ghidra slot C on GameDll7 (decompiles kept in the session scratchpad, not committed). Script line numbers are 1-based in the extracted files. RVAs are GameDll7 unless marked. Code refs are `mod/src/coop/<file>:<line>` on main as of `3e59288` (committed files; `coop.cpp` is cited by function because the working copy has uncommitted edits).

Legend: **OK** = covered (code cited). **RISK** = covered with a known weak spot. **GAP** = not handled. Every claim is marked VERIFIED (seen in code/data) or UNVERIFIED (inferred).

---

## 0. Cross-map engine facts found during this audit (they drive most gaps)

These apply to every map, not only the nine below.

1. **The player on MAP11A and MAP11C is not `cScr_ModHunter`.** `WORLD.SCR` sets `mod_player_ent = obj_pilot`, `mod_player_cls = cObjPilot` (MAP11A `WORLD.SCR:93-94`, MAP11C `:85-86`). `cObjPilot` ctor `0x64660` calls the hunter ctor `0x4d8e0` (vtable `0x106ff0`) and then writes its own vtable **`0x109d48`**, script class `cScr_ModPilot` (`MODELS/CHARACTERS/PILOT.SCR`), alloc `0x840` (`0x650f0`). `cObjPilot::Spawn` `0x65370` attaches the pilot to the entity `helicopter` at tag `masscenter`, sets body state `Heli`, and **removes the pilot from the animals' stimulus and attack-target lists** (`RemoveStimulus`, `RemoveAttackTarget`). VERIFIED (decompile).
   Our tracker accepts only vtable `0x106ff0` (`player_tracker.cpp:53`, `:326`), so on these two maps `LocalPlayer().valid` stays false. That gates: the client's `kSyncRequest` (`events.cpp:353`), the pre-render mirror callback (`player_tracker.cpp:85` via `g_map_ready`, `:277`), `SendLocalState` (`coop.cpp` `SendLocalState`), puppets (`puppets.cpp:559`), and `combat.cpp:144` `LocalHunter()`. A client that never syncs gets no live commits at all (`events.cpp:518`, commits go to `g_synced` only; only `kMapStart` goes to everyone). VERIFIED (code). Runtime UNVERIFIED (no MAP11A/11C co-op run exists in any log on this PC).

2. **Script `MoveToPos` on an animal is a full respawn, and it unfreezes the brain.** `cScrCharacterEx::MoveToPos` `0x161a0` calls entity vt `+0x158(3, 0)` and then `Move` (vt `+0xA4`). On `cScrAnimal` (vtable `0xff470`) slot `+0x158` is `0x14610` (dump at `0xff5c4`): health `+0x19C = +0x4C0` (max), death guard `+0x600 = 0`, dead bits cleared (`+0xC0 &= 0xfb7f`), body state `IdleAttack` (`0x12cc0`), brain `+0x79 = 0`, `+0x71 = 0`, `+0x84 = 0`, `+0x4B0 = 1`. Brain `+0x79` is the `FreezeBrain` byte (MEMORY). Script `Kill()` (`0x16a40`) is vt `+0x234` = park `0x1be20`: `+0x4B0 = 0`, brain `+0x79 = 1`, move to `-10000`, `RemoveDeadAnimal`. `CanBeRespawned()` = `+0x4B0 == 0` (`0x164b0` → `0x194d0`). VERIFIED (decompile). This closes the open item in `re/notes/animal-park.md:113` ("which call clears brain +0x79 after MoveToPos"). It also answers open question 1 of `re/notes/review-2026-10-17.md` (H1): yes, `MoveToPos` alone revives a dead animal (health, death guard, dead bits, state, brain).
   Consequences in our code (VERIFIED by reading): the client mirror freezes once (`animals.cpp:965`, `if (!m.frozen)`) and never re-freezes, so every replayed fire that `MoveToPos`es an already-mirrored animal leaves that client's brain running (local chase, local damage, local sync attacks on the client's own hunter while snapshots overwrite the position). And death is terminal on both sides: host `announced_` (`animals.cpp:659-663`) plus journal de-dup on `(kind, subject, key)` (`events.cpp:521`) never send a second `kAnimalDied`, and the client pins hp 0 forever once `death_presented` (`animals.cpp:929-933`). A respawned animal is invisible/unkillable for clients. Maps that reuse animals: MAP11A/11B/11C (boar waves), MAP12A (wolf1 dies up to 4×, pbear 2×), all three bonus maps (every wave). Likely also MAP2B, MAP5 (UNVERIFIED, other audits). Hypothesis, UNVERIFIED: the MAP2A lion-pounce client crash (STATUS two-PC table) is this: `mlion1.MoveToPos` unfroze the client's lion and its local sync attack ran against stale state.
3. **A trigger's script `OnHit` runs only on the process whose activator touched it.** C++ OnHit handler `0x6b140` (thiscall(trigger, activator)): requires armed `+0x1E4`; checks the `WaitTask` bitmask `+0x200` against the mission's tasks; if `ActivateBy` name `+0x224` is set, compares it with the activator's name `+0x198`, else requires vt `+0xFC` (IsPlayer); then disarms (`0x68fd0`, not hooked), runs script OnHit (`+0x208` via class manager vt `+0x28`), sets `+0x1E8 = 1`. Our hooks are arm `0x68eb0` and fire `0x68130` only (`story.cpp:702-707`); a client's request makes the host run `OnActivate` only (`story.cpp:348-373`). Anything a script does inside `OnHit` other than `ActivateAfter` is local. VERIFIED.
4. **Task end/fail triggers fire without a touch.** `cMissionTask::End` `0x96da0`: on state 3 with an OnEnd trigger (`+0xD4`) or state 5 with an OnFail trigger (`+0xD8`), a `trigger_event` gets vt `+0x8C` (the OnHit handler) with the mission hunter `+0x258` as activator; a `trigger_level` gets `+0x1E8 = 1`. That is why MAP11A/11C `cScrTask06/07/08/09` and `cScrFailed` sit far outside the flight path (x ≈ -580 m). VERIFIED.
5. **Exits ignore the BSP map name.** `cObjTriggerLevel` activation (`0x6c420`, `0x6c610`, vtable slots at `0x10bc38/0x10bc3c`) splits `target` on `,`, discards the first token (the map) and passes the second (spawn point) to `0x67b0`; then `SetMapIsComplete`, then writes server message `0x87` **code 7 "Checkpoint reached" straight to the stream `DAT_10185ea8`** (not through `0x91c20`), then `0x50760(hunter, 2.0f)`. The next map comes from the profile's progression. `ActivateTriggerLevel` `0x6a4d0` calls it with the local player (`0x6c30`). VERIFIED. This explains the old MAP3B → `map2a.bsp` runtime line (BSP says `map3c`). BSP targets for the record: MAP11A `exit_1 → map11b,human`, MAP11B `exit_1 → map11c,human`, MAP11C `exit_1 → map12a,human` (no script activates it), MAP12A `exit_1 → map12b,human`, MAP12B `exit_2 → map12b,human` (self; harmless since the token is ignored).
6. **`AddEvent` pools are created at map load.** `0x76b0` → `0x82f30`: EVENT_TRAP → 10× `obj_trap` named `trap1..10`; EVENT_MINE → 10× `obj_mine`; cages ×10, bees ×5, fire events ×5; EVENT_ANIMAL → one `obj_animal` named `animal1`, species from the arg, parked at once (vt `+0x234`); snake/birds/bats/cavedust/fake event ×1. Creation order is fixed, so ids match across processes. VERIFIED.
7. **Mission natives and constants.** `MISSION_ACTION_KILL 1`, `KILL_PROTECTED 2`, `REACHED_POSITION 3`, `FIND_SOMETHING 4`, `LOSE_TARGET 5`; `TASK_SHOOT 0`, `TASK_RACE 2`; `NEED_AMMO_FOR_COMPLETE 1` (`re/notes/story-natives-gamedll.c:298-303`, `:1549-1556`). `cTaskShoot::UpdateTaskStatus` `0x999a0`: action 1 + species decrements the count, **action 2 fails the task (state 5)**, action 5 decrements the available pool. `cTaskShoot::CanBeCompleted` `0x979f0`: condition 1 fails the task when `0x2f530` says the **local** player (`0x6c30` → profile `GetAmmoCount`) has no ammo. `cTaskRace::UpdateTaskStatus` `0x9a170`: action 3 with the task id completes it. State-5 tasks reach `0x91c20(6)` from `0x990b0` (hooked). VERIFIED.
8. **Mission-end writes that bypass `0x91c20`** (so `kMissionEnd` never carries them): `cObjNPC::OnDie` `0x5fde0` for class `cObjGuide` (code 6, text `+0x848` + " hunt has ended.", `re/notes/npc-health.md:77`); `cObjTriggerLevel` code 7 (item 5); `0x96750` (in four task vtables at `0x111b74/0x111c2c/0x111cb8/0x111d28`) writes code 6 or 5 directly when mission status `+0x254` is 3 or 2 — when that slot runs is UNVERIFIED. `0x965e0` (status 3 + `0x91c20(6)`) is called by `0x50350` (hunter `OnDie`) and twice by `0x1a680`. VERIFIED (xrefs/decompile).
9. **World items.** `cObjWorldItem` use `0x432c0`: adds ammo/medikit/item to the **user's** profile; property `next trigger` (`+0x1E0`) → `0x92210` → trigger arm `0x68eb0`; `force trigger activation` (`+0x1F4`) → that trigger's vt `+0x8C`. Pickup collision `0x45190` accepts only `cObjHunter` + IsPlayer. Each process has its own copy of every item. VERIFIED.

---

## 1. MAP11A (chapter 11 opener, `SetMission(11)`; helicopter rail shooter)

Flow (`TRIGGERMISSION11A.SCR`): `cScrTask01` fires at spawn (`ActivateBy("helicopter")`, `Activate`, `ActivateAfter(0)`, `:6-8`) → heli `BSMGoToState("Ani1")` (`:17`), cable car `tele.SetAnim("idle1")` (`:18`), task 1, arms `cScrTask09` and `cScrFailed` (`:21-22`), cutscene 1 → `cScrTask09` (heli touch) completes race task 1 (`:43`, in OnActivate), boar wave t21 (wildboar2 → siberian2), task 2, arms `cScrTask06` → task 2 end forces `cScrTask06` (`WaitTask(2)`, `:76`) → `helicopter.CompleteTask()` (`:86`), arms `cScrTask03` → heli touch: wave t3 (wildboar1/2/3 → siberian3), task 3 → task 3 end → `cScrTask07` → `CompleteTask`, arms `cScrTask10` → heli touch: cutscene 2 (`tele.SetAnim("ani1")` `:164`), arms `cScrTask04` → heli touch: wave t4 (wildboar1/2/3 → siberian4), task 4 → task 4 end → `cScrTask08` (`ActivateAfter(3)`) → `ActivateTriggerLevel("exit_1")` (`:215`) + cutscene 3.
Mission (`MISSION11A.SCR`): task 1 `TASK_RACE` with `ActivateTriggerOnFail("cScrFailed")`; tasks 2/3/4 `TASK_SHOOT` wildboar 1/3/3, `NEED_AMMO_FOR_COMPLETE`, OnEnd `cScrTask06/07/08`, OnFail `cScrFailed` (`:18-50`).

| Mechanic | Where | Covered by / risk / gap |
|---|---|---|
| Player is `cObjPilot` (vt `0x109d48`), attached to `helicopter` | `WORLD.SCR:93-94`; `0x64660`, `0x65370` | **GAP, blocks progress.** Not tracked → client never syncs (section 0.1). VERIFIED code |
| Helicopter flight (`helicopter`, `cScrHelicopterEnt`, body machine `cBodyHelicopter` Ani1→Loop1→Ani2→Loop2→Ani3a/3b→Loop3→Ani4; `IsTaskCompleted`/`UncompleteTask` gate) | `MODELS/WORLD/HELICOPTER/HELICOPTER.SCR`; `BSMGoToState` `0x240f0`→`0x23cc0` | **RISK.** Per-process machine driven by replayed fires (`story.cpp:375-397`). The client's heli starts when its `cScrTask01` fire applies (catch-up), so it lags. If two `CompleteTask` commits land before the client's heli reaches `Ani2.Begin` (which clears the flag), the client heli waits in `Loop2` until cutscene 2's `BSMGoToState("Ani3b_frozen"/"Ani3b")` resyncs it. UNVERIFIED |
| Heli-activated triggers (`ActivateBy("helicopter")`: 01, 09, 03, 10, 04) | trigger script | **OK.** Host's heli decides, client's own touch becomes a request; `+0x1FC` stops a second OnHit after the replay. VERIFIED code |
| Task-end triggers 06/07/08 and `cScrFailed` | `0x96da0` forced OnHit (section 0.4) | **OK** (client's forced OnHit → fire request; host decides). VERIFIED code |
| Cutscenes 1/2/3 (camera `c1cam1`, `heli1.SetAnim("ms11a…")`, `helicopter.MakeVisible`) | `CUTSCENE/MAP11A/*` | **OK.** Started by replayed fires. `heli1.SetAnim` is `cScrHelicopter::SetAnim` `0x23fd0` (own native, not `cScrEntity::SetAnim`), so no `kPropAnim` and none needed. Late-join skip (`story.cpp:400-402`). VERIFIED |
| Cable car `tele` (`obj_scripted_entity`, `cScrEntity`) `SetAnim` idle1/ani1 | `:18`, `:164` | **OK** via `kPropAnim` (`story.cpp:581-599`). The client applies it twice (commit + its own replay); harmless restart. VERIFIED code |
| Boars ×6 `cScrAnimal` (AddAnimal at load), waves by `MoveToPos`+`AttackChargers` → siberians | `WORLD.SCR:130-135`; trigger `:50-52`, `:105-115`, `:184-194` | **GAP (desync).** wildboar2 is reused in waves t21/t3/t4, wildboar1/3 in t3/t4: respawn (section 0.2) → second-life boars invisible/unkillable on clients; every replayed `MoveToPos` unfreezes the client's brain. VERIFIED code |
| Kill counts (tasks 2-4) | `UpdateMissionStatus` from death vt `+0x154` `0x1e8b0` | **RISK.** Host counts host deaths (client hits forwarded, `animals.cpp:749-802`). Client's own counts drift after respawns; only harmful through gap G4 (requests trusted). VERIFIED code |
| `NEED_AMMO_FOR_COMPLETE` | `MISSION11A.SCR:28/37/46`; `0x979f0`, `0x2f530` | **RISK.** Host fails everyone when the *host* is dry (`0x990b0`→`0x91c20`, hooked, `events.cpp:410-425`); a dry client only fails locally (message swallowed) and plays on. VERIFIED code |
| `cScrFailed` (OnHit `PlayHint` "not enough ammo", empty OnActivate) | `:225-244` | **OK** (hint is local, fail itself goes through the hooked message). VERIFIED |
| Siberians 1-4 (`obj_guide` = `cObjGuide`, `cScrSiberian1-4`, models `/npc/siberian1|2/`, health 100) are the boars' targets | BSP `[30-33]`; `GUIDE.SCR:2212+` | **RISK/GAP.** Mirrored as NPCs (`player_tracker.cpp:330-335`, `animals.cpp:666-723`), client damage blocked (`animals.cpp:131-134`). A host siberian death fails the map through `0x5fde0` code 6, **not** `0x91c20` (section 0.8): no `kMissionEnd`; the client fails only via its own mirrored `OnDie` when the profile health reaches 0 (`animals.cpp:1028`). Not protected by script, but it is a de-facto protect objective. VERIFIED code; runtime UNVERIFIED |
| Animals attack players | pilot removed from attack list (`0x65370`) | **N/A for the pilot.** Puppets are still attack targets (`[Puppets] AttackTarget=1`) and would float at the remote heli's mass centre: RISK once G1 is fixed. UNVERIFIED |
| Sync-attack clip | `TEENM\BODY\MAP11A.AP` = `pl_wb_syncattack1` only | **OK** (`combat.cpp:281`). PvP knock no-ops on a pilot (`LocalHunter()` vt test, `combat.cpp:144-149`). VERIFIED |
| `"hunter"` reads | `ActivateBy("hunter")` on 06/07/08/cScrFailed (forced with mission hunter = host's pilot); `PlaySoundEffect(…, "hunter")`; cutscene `FindEntScrObject("hunter")` (local); C++ ammo check `0x2f530` (local); heli update `0x233f0` reads local player `0x6ae0` | local per process; host decides the forced ones. VERIFIED |
| Exit `exit_1` | `:215`; BSP `[54]` `map11b,human` | **RISK.** Map token ignored, next map = profile (section 0.5). Host `kMapStart` drives clients (`events.cpp:242-281`); a client replaying the fire also loads its own profile's next map (code 7, unhooked). VERIFIED code |
| `ActivateBulletWarning`, `SetMaxAttackerNo(4)`, `AddHUDItem` | `WORLD.SCR:99`, trigger `:23` | local, OK |

## 2. MAP11B (no `SetMission`; on foot; Hogzilla boss)

Flow (`TRIGGERMISSION11B.SCR`): `cScrTask01` at spawn: ammo `-35/-10` (`:20-21`), Reggie path-lead to `start1`, `AddAIToBeProtected("reggie11b")` (`:24`), task 1, arms 02, cutscene 1 → `cScrTask02` (touch): task 2, boars 1-3 at t2 (wildboar2 → `"hunter"`, others → Reggie, `:57-67`) → `cScrTask03`: boars 4-6 at t3 (wildboar5 → `"hunter"`), task 3 → `cScrTask04` (voice only; its `trigger_5.Activate()` is commented out, `:123`) → **`cScrTask05` is armed only by picking up `ammo_rifle2_1st`** (BSP `[67]` `next trigger=cScrTask05`) → touch: OnHit `UpdateMissionStatus(3, 1)` (`:137`), OnActivate objective `point2`, Reggie to `start2`, task 4, arms 10 → `cScrTask10`: objective `hogzilla1`, task 5, arms 06 and 11, `MakeBoss(TRUE,"HOGZILLA")`, Hogzilla → Reggie (`:166-178`); BeginCutScene respawns Hogzilla at `hogzilla_1` + cutscene 2 → `cScrTask06`: boars 1-3 respawned at t6 (`:230-243`), task 6 → `cScrTask07`: boars 4-5 respawned at t7, task 7 → `cScrTask08`: OnHit `UpdateMissionStatus(3, 4)` (`:296`) → `cScrTask09` (`WaitTask(5)`, Hogzilla dead) → `exit_1` (`:318`).
Mission (`MISSION11B.SCR`): race 1, shoot 3 boars, shoot 3 boars, race 4, Hogzilla 1 (OnEnd `cScrTask11`), shoot 3, shoot 2.

| Mechanic | Where | Covered by / risk / gap |
|---|---|---|
| Touch triggers, `ActivateBy("hunter")` | all | **OK** (host decides; `story.cpp:446-499`). Smoke 2026-10-16 21:56: client applied the post-join `cScrTask02` arm, 8 animals received. VERIFIED (STATUS) |
| **OnHit-only `UpdateMissionStatus`** (`cScrTask05` `:137`, `cScrTask08` `:296`) | section 0.3 | **GAP (desync).** If a client walks in first, only the client completes race tasks 1/4; the host runs OnActivate only. No `CheckMissionStatus` on 11B, so it does not block (HUD only). VERIFIED code |
| `ammo_rifle2_1st` pickup arms `cScrTask05` | BSP `[67]`; `0x432c0` → `0x92210` → `0x68eb0` | **OK.** A client pickup becomes an arm request; the host arms and commits (`story.cpp:296-306`). Ammo goes to the picker only (per-process items). VERIFIED code |
| Ammo / medkit pickups ×7 (`reuseable=0`) | BSP `[63-69]` | **OK by design** (each player has a private copy). VERIFIED code |
| Reggie (`obj_guide`, `cScrReggie11b`, `reginald_w` skin, handgun3, health 100) path-lead + protected | `:22-24`, `:145-146`, `:173-174`; `WORLD.SCR:100` | **RISK.** Position/legs/health mirrored (`animals.cpp:1082-1090`), gun not (npc-arms job). Death: protected-kill fail via `0x999a0` action 2 → `0x91c20` (hooked) **and** `cObjGuide` direct code 6 (both on host; client gets its own mirrored `OnDie`). Client cannot hurt him (`animals.cpp:761-767`). VERIFIED code |
| Boars ×6 + Hogzilla `cScrAnimal`; targets `"hunter"` (wildboar2/5) and Reggie | `WORLD.SCR:116-122` | **RISK.** `"hunter"` = host; `RetargetToNearestPlayer` (`combat.cpp:499-519`) switches only animals already chasing a player, 3 m margin. VERIFIED code |
| Boar waves t6/t7 reuse boars 1-5 (respawn) | `:230-280` | **GAP (desync)** (section 0.2). VERIFIED code |
| Hogzilla boss: `MakeBoss` → HUD item `0x10` (`0x16d20`→`0x12380`), health 1500, brain `AIHuntBrain_Charger`, sync attacks `wb/back_wb/left_wb/right_wb_syncattack1` | `HOGZILLA.SCR:7`, `:198-234` | **RISK.** Boss bar reads mirrored health each frame (UNVERIFIED that the HUD polls `+0x19C`). Teen pak `MAP11B.AP` has only `pl_wb_syncattack1`; if the host forwards a back/left/right player clip, the client's `PlaySA` gets a clip its pak lacks (possible fatal "Could not forward animation"). UNVERIFIED (retail SP would hit the same, so probably only the front clip is used) |
| Hogzilla body states on the client | `SetBodyState` from host names (`animals.cpp:1091-1111`) | **RISK.** Same species model on both, so names should resolve (unlike the MAP12C yeti). UNVERIFIED |
| Cutscenes 1/2: Reggie `AITaskNPC_PlayAnim` `cs03`/`cs04`, Hogzilla `IdleWait`, camera | `CUTSCENE/MAP11B/*` | **RISK.** Client NPC brain is frozen, so the PlayAnim task does not run there. Covered only if the task goes through `cObjCharacterEx::PlaySA(char*,char*)` (`kPlaySA`, `story.cpp:532-579`). UNVERIFIED |
| Stumps ×2 (`obj_target`, `generic/cheat=1`) | BSP `[61-62]` | **OK** (`props.cpp:224-279`, `kPropHit`). VERIFIED code |
| Hunt-area escape points `escape1/2` (De/ActivateAIHuntAreaPoint) | `:10-11`, `:54`, `:184`, `:266` | **OK** (replayed with the fires). VERIFIED |
| EVENT_ANIMAL wildboar pool (`animal1`, parked at load) | `WORLD.SCR:94`; `0x82f30` | **RISK, cosmetic.** Event triggered by each process's own hunter; the host's is mirrored, a client's own event is snapped back to the host's parked pose. UNVERIFIED |
| `"hunter"` reads | `ActivateBy`; boar targets; `AddItemAmmo` on player (local, both); cutscene player flags | as above |
| Exit `exit_1` | `:318`; BSP `[81]` `map11c,human` | **RISK** (section 0.5, gap G7) |

## 3. MAP11C (no `SetMission`; helicopter rail shooter; chapter end)

Same structure as 11A with four boars and twelve kills (tasks 2-5: 3, 3, 4, 2; all `NEED_AMMO_FOR_COMPLETE`, OnEnd `cScrTask06/07/09/08`; `MISSION11C.SCR`). `cScrTask02` OnHit completes race task 1 (`:38`) — heli-activated, so each process's own heli runs it. Waves reuse wildboar1-4 (t2, t3, t4 in `cScrTask10`'s BeginCutScene `:149-156` and `cScrTask04`, t9 in `cScrTask09`). End: `cScrTask08` (`WaitTask(5)`) → `CheckMissionStatus()` (`:233`) + cutscene 3. `cScrFailed` exists but is never armed (the OnFail lines are commented out and misspelled, no `trigger_11.Activate`). BSP `exit_1 → map12a,human` is never activated.

| Mechanic | Where | Covered by / risk / gap |
|---|---|---|
| Pilot player class | `WORLD.SCR:85-86` | **GAP, blocks progress** (as 11A). The chapter-end `kMissionEnd` only reaches synced clients (`events.cpp:518`), so an unsynced client never gets the results screen; its own `CheckMissionStatus` is swallowed (`events.cpp:427-437`). VERIFIED code |
| Heli flight, heli triggers, task-end triggers, cutscenes | as 11A | **OK/RISK** as 11A |
| OnHit `UpdateMissionStatus(3,1)` on a heli trigger | `:38` | **OK** (each process's heli touches it). VERIFIED |
| Boar reuse ×4 lives | trigger script | **GAP (desync)** (section 0.2) |
| Ammo fail | `MISSION11C.SCR` | **RISK** as 11A |
| Siberians 1-4 (`obj_guide`) | BSP `[34-37]` | **RISK/GAP** as 11A |
| Mission end | `CheckMissionStatus` → `0x94e50` → `0x91c20(5)` | **OK** on the host, replayed (`events.cpp:170-189`), once the client is synced. VERIFIED code |

## 4. MAP12A (chapter 12 opener, `SetMission(12)`)

Flow (`TRIGGERMISSION12A.SCR`): `cScrTask01` at spawn: protect Wirake (`:22`), +30 handgun ammo (`:24`), traps `trap1..10` to `foottrap1..10` (`:25-34`), **Reggie `SetAIType(AITYPE_COMPANION,"hunter")`** (`:36`), Wirake path-lead, arms 02/03/04/10/14 → `cScrTask02`: wolf1 → `"hunter"`, wolf2 → Wirake, task 2 → `cScrTask03`: `CompleteTask(2)`, pbear `AttackBears` → `"hunter"`, task 3; BeginCutScene respawns pbear + cutscene 7 → `cScrTask04`: `CompleteTask(3)`, wolves 1/2 respawned, task 4, arms 05 → `cScrTask05` (state machine): `CompleteTask(4)`, wolf1 again, task 5, Reggie knocked down (`PlayAnim "hurtdown"`, `SetPhysNPC(0)`, `:183-199`); BeginCutScene: wolf2/pbear respawn+`Kill` (park), wolf1 respawn, cutscene 2 → task 5 end → `cScrTask10`: Wirake unattackable, hint, `CompleteTask(5)`, arms 06/07/08/11/12, Reggie `hurtdown` → `cScrTask06`: wolves 1/2 respawned, task 6 → `cScrTask11`: **OnHit `player/wirake/reggie.DisableStimulus()`** (`:350-352`); wolves parked, stealth zone (`SetStealthLevels`, `EnableStealth("boar2",35,40)`), boars `IdleScare`, surprise wolves 5/6 → `"hunter"` (`:384-396`) → `cScrTask12`: **OnHit `EnableStimulus`** (`:413-414`), stealth off → `cScrTask07`: wolves 3/4 → `"hunter"`, task 7 → `cScrTask08`: `CompleteTask(7)`, pbear → `"hunter"`, task 8, Wirake knocked down; BeginCutScene parks wolves 3-6 and both boars, respawns pbear at `biggrims_t8_01` → task 8 end → `cScrTask13`: `RemoveAIToBeProtected` (`:577`), `CompleteTask(8)`, arms 09 → `cScrTask09`: `UpdateMissionStatus(3,1)` + `exit_1` (`:628-632`).
Mission: race 1; shoot wolf 2, pbear 1, wolf 2, wolf 1 (flee = kill, OnEnd 10), wolf 2, wolf 2, pbear 1 (flee = kill, OnEnd 13).

| Mechanic | Where | Covered by / risk / gap |
|---|---|---|
| Touch triggers, state-machine OnActivates (`setstate(Activate)`) | all | **OK** (the replayed OnActivate runs the same state machine on the client). VERIFIED code |
| `CompleteTask(n)` native in fires | `:96`, `:127`, `:174`, `:252`, `:366`, `:473`, `:580` | **OK** (runs on both; `story.cpp:601-606` only logs). VERIFIED |
| Wolves 1-6, pbear, boars 1-2 (`cScrAnimal`, AddAnimal at load), targets `"hunter"` / `wirake12a` | `WORLD.SCR` AddAnimalOnMap | **RISK** (host-targeted; retarget as 11B) |
| **Animal reuse** (wolf1 ×4 lives, wolf2 ×3, pbear ×2) and `Kill()` parks | above | **GAP (desync)** (section 0.2). The final bear (task 8) is a respawn: clients never see it alive. VERIFIED code |
| Reggie companion of `"hunter"` | `:36` | **RISK.** Follows the host only; `npc_follow.cpp` exists but is off (`[Story] NpcFollowNearest=0`, `npc_follow.cpp:39`, `:102`). VERIFIED code |
| Wirake path-lead, protected, `SetCanBeAttacked`, knocked down; Reggie knocked down | `:22`, `:183-199`, `:249`, `:260-262`, `:436`, `:480-495`, `:582-584` | **GAP (desync, visible).** The client NPC brain is frozen; `PlayAnim "hurtdown"` reaches the client only if the task uses `PlaySA(char*,char*)` (`kPlaySA`); the body-state mirror maps no PlayAnim clip (`animals.cpp:291-330`). Both guides are `cObjGuide`: death = direct code 6 (section 0.8) plus protected-kill fail. UNVERIFIED for the PlayAnim path |
| **OnHit-only stimulus switches** (`cScrTask11`, `cScrTask12`) | `:350-352`, `:413-414` | **GAP (desync).** If a client enters first, the host never disables the host hunter's / NPCs' stimuli, so the host's boars/wolves can still sense them in the stealth section. VERIFIED code |
| Stealth zone (`GetDistBetween2Ais(szStealthCenter,"hunter")`, crouch flag, HUD) | `WORLD.SCR` cMapTest states | **OK, local per process** (each player's own meter; animals react to the host only). VERIFIED |
| Surprise wolves (state 2048, `surpriseid`) → `"hunter"` | `:384-396` | **RISK.** Ambush springs on the host's approach only. UNVERIFIED engine detail |
| Traps `trap1..10` (EVENT_TRAP pool), `cBodyTrap` `Close`/`CloseGuide` → `HurtHuman(20)` | `WORLD.SCR:165`, `MODELS/WORLD/TRAP/TRAP.SCR` | **RISK, local.** Same placement on every process (replayed `MoveToPos`); trap state is per process (a host NPC caught by a host trap is not shown closed on clients). UNVERIFIED |
| Ice `obj_ice` ×8 (4 crack models each), `trigger_hurt` ×2 (`hurt value=100`) | BSP `[139-146]`, `[173-174]` | **RISK, local.** Cracking per process (`cObjIce::RayHit/CheckCrackModel`); not synced. A host puppet entering a host `trigger_hurt` may forward `kPlayerHurt` (`combat.cpp:89-98`) on top of the client's own hurt. UNVERIFIED |
| Cutscene 1 `wirake12a` PlayAnim `cs01` with `play_sa` partner `reggie12a` | `CUTSCENE/MAP12A/CUTSCENE1.SCR` | **OK** via `kPlaySA` (MAP2A pattern). VERIFIED pattern |
| Cutscenes 2/4: wolf1 / pbear `AIHuntTask_PlaySA` (`cs02a`, `cs04`) on Reggie / Wirake; `MoveToPos`/`Kill` on actors | `CUTSCENE2/4/5/6.SCR` | **RISK.** Animal brain task on a frozen client brain does not run — except that the cutscene's own `MoveToPos` unfreezes it (section 0.2), which is accidental. UNVERIFIED |
| Cutscene 6 `player.MoveToPos("c5_2")` | `CUTSCENE6.SCR` | **OK** (moves each process's own hunter: a natural gather). VERIFIED |
| Stumps ×5, pickups ×6 | BSP | **OK** (`props.cpp`; private items) |
| `AddAnimPakForAI("wolf1"/"pbear","map12a.ap")` | `WORLD.SCR` | OK (same on both) |
| Sync-attack clips | `MAP12A.AP`: `pl_pb`, `pl_back_pb`, `pl_left_pb`, `pl_right_pb`, `pl_gw` | **OK** (`combat.cpp:284` front/back; host-forwarded clips exist in the pak). VERIFIED |
| Exit `exit_1` | `:632`; BSP `[175]` `map12b,human` | **RISK** (G7) |

## 5. MAP12B (no `SetMission`; Big Grimm boss)

Flow (`TRIGGERMISSION12B.SCR`, 2 triggers): `cScrTask01` at spawn: traps 1-5 to `foottrap1..5` (`:15-19`), `pbear.MakeBoss(TRUE,"BIG GRIMM")` (`:21`), `AIHuntTask_AttackObjectMovers` → `"hunter"` (`:22-24`), task 1, arms 02; BeginCutScene respawns the bear at `biggrims_t1_01`, objective, cutscene 1 → task 1 end (bear killed or fled = kill) → `cScrTask02` (`WaitTask(1)`, `ActivateAfter(4)`) → `exit_2` (`:50`). Player starts unarmed: `AiCanFindItem("hunter", handgun3/rifle2/mbranch)` (`WORLD.SCR`).

| Mechanic | Where | Covered by / risk / gap |
|---|---|---|
| Big Grimm (`ANIMAL_BIGGRIM` on entity `pbear`, `cScrAnimal`, health 1500, brain `AIHuntBrain_ObjectMover`, `AddSA("faltree",…)`, clips `bg_syncattack1`/`pl_bg_syncattack1`) | `WORLD.SCR` AddAnimalOnMap; `BIGGRIM.SCR:8`, `:16`, `:90-92` | **RISK.** Mirrored animal; boss bar per process. Smoke 21:56: client got 1 animal. Respawned once by BeginCutScene (before its only death: fine). Sync clip on the player side: `MAP12B.AP` has only `pl_pb_syncattack1`, not `pl_bg_syncattack1` (UNVERIFIED what the engine plays) |
| Falling-tree attack (`obj_move_object` ×2, `msn12faltree.skin`) | BSP `[14-15]` | **UNVERIFIED.** Covered only if the bear's tree SA goes through `PlaySA(anim, partner)` (`kPlaySA`) or `cScrEntity::SetAnim` (`kPropAnim`); otherwise the host's tree falls and the client's stays up |
| Target `"hunter"` | `:24` | **RISK** (host; retarget as 11B) |
| Weapon pickups (`handgun3_1st`, `mbranch_1st`, `rifle2_1st`), ammo, medkits | BSP `[20-22]`, `[41-44]` | **OK by design** (each player picks up their own copy; `0x432c0`). VERIFIED code |
| Traps 1-5 (EVENT_TRAP pool) | `WORLD.SCR:86` | **RISK, local** (as 12A) |
| `trigger_hurt` `hurt value=10000` (fall) | BSP `[53]` | local per process (client death → F9). VERIFIED data |
| Stumps ×4 | BSP `[16-19]` | **OK** (`props.cpp`) |
| Flee = kill | `MISSION12B.SCR` `SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL)` | **OK** (host decides; fled animal parked on clients by design). VERIFIED |
| Exit `exit_2` | `:50`; BSP `[54]` `map12b,human` | **RISK** (G7; the self-target is harmless because the map token is ignored, section 0.5) |

## 6. MAP12C (no `SetMission`; yeti; game end)

Flow (`TRIGGERMISSION12C.SCR`): `cScrTask01` at spawn: yeti `AttackThrowers` → `mountainguide12c`, `MakeBoss(TRUE,"YETI")` (`:19`), task 1, arms 02; BeginCutScene respawns the yeti at `yetis_t1_01` (`:25`) + cutscene 1 (yeti `AIHuntTask_PlaySA` `yeti_syncattack1` on the guide) → task 1 end → `cScrTask02` (`WaitTask(1)`, `ActivateAfter(4)`) → `CheckMissionStatus()` (`:44`). `0xbe480` plays `Movies\omap12c` (the outro; who calls it after the results is UNVERIFIED).

| Mechanic | Where | Covered by / risk / gap |
|---|---|---|
| Yeti (`cScrAnimal`, health 2500, brain `AIHuntBrain_Thrower`, `throwtime 5`, `throwdmg 30`, sync `yeti_syncattack1..4`) | `YETI.SCR:11`, `:15`, `:90-108`, `:161+` | **RISK/open** (yeti job). Known: client `SetBodyState` Attack/IdleAttack/Run → -1; client crash in the fight (`EngineDll7+0x28b337`). New input for that job: the BeginCutScene `MoveToPos` runs the respawn `0x14610` on the client too (brain `+0x79/+0x71/+0x84 = 0`, state `IdleAttack`), before our CutScene thaw/refreeze (`animals.cpp:944-951`, `:965`). VERIFIED code. Possibly related (UNVERIFIED): the MAP12C client engine log has 1151× `Could not forward animation idleattack1 from Legs to Legs model …yeti.skin` (`re/notes/log-sweep.md` row 7), and `IdleAttack` is exactly the state the respawn sets |
| Yeti throws (`cObjAIHuntThrowObject`) vs puppets | `YETI.SCR:107-109` | **RISK.** Projectiles exist on the host only; puppet bodies use non-colliding words, so throws may never hit clients. UNVERIFIED |
| Mountain guide (`obj_redshirt`, `cScrMountainguide12c`, `/npc/mountainguide/`) | BSP `[13]` | **OK.** Mirrored NPC; `0x5fde0`'s code-6 path is `cObjGuide` only, so his death does not fail the map. VERIFIED code |
| Boss bar | `MakeBoss` `0x12380` | as 11B |
| Pickups (mbranch, shotgun ammo, medkits), stumps ×4 | BSP `[14-21]` | **OK** |
| Opening hold / cutscene skip / drop-in | `story.cpp:84-95`, `:400-413`; `animals.cpp:934-951` | **OK** (verified MAP12C 10:21, STATUS). VERIFIED |
| Mission end | `CheckMissionStatus` | **OK** (hooked; `kMissionEnd`) |

## 7. MAPARENA (bonus, `SetMission(17)`)

Mission: one bonus `TASK_RACE`. `cScrTask01` fires after 1 s (`StartTask("1")`). Everything else is the **level script `cMapTest::Update`** in `LEVELS/MAPARENA/WORLD.SCR`, running every frame on every process: species timers, `rand()%7` spawn points (`animal2..8`), `SpawnSpecies1..7` = `CanBeRespawned()` → `MoveToPos` → `Attack*` → `"hunter"` (leopards ×2, scrub bulls ×2, wolves ×3, lions ×2, grizzly, rhino), pickup timers that `MoveToPos` the three world items, three rounds, `UpdateMissionStatus(3,1)` + `EndArena()` → `trigger_1.CheckMissionStatus()`. The arena section triggers `cScrTriggerArena1..7` are never armed by script.

| Mechanic | Where | Covered by / risk / gap |
|---|---|---|
| **Per-process spawner** (timers + `rand`) | `WORLD.SCR` cMapTest | **GAP (desync).** Clients run their own waves at their own times and spawn points; every client `MoveToPos` revives and unfreezes its local copy (section 0.2) while snapshots of the host's state overwrite it. Clients get attacked by their own local animals. VERIFIED script + code |
| Respawned waves (every animal lives many times) | above | **GAP (desync)** (section 0.2) |
| Round end / success | `UpdateMissionStatus` + `CheckMissionStatus` | **OK** on the host (client's swallowed, host `kMissionEnd`). Round hints are per process. VERIFIED code |
| Pickups (`reuseable=1`) respawned by timers | `SpawnPickups` | **OK by design** (private) |
| `GetHealth()` medkit rule, `SetRecoverSpeed`, `SetMaxKnifeDistance(2.0)`, `mod_showTimer 1` | `WORLD.SCR` | local, OK |
| `cScrLavaThrower lava_thrower` | `WORLD.SCR:54`, `:112` | **N/A**: no such BSP entity (only an `obj_aifx` `volcan` FX). VERIFIED data |
| Clips | `MAPARENA.AP` has l/sb/gw/wb/ln/gb/r sets | **OK** (`combat.cpp:287`) |

## 8. MAPISLAND (bonus, `SetMission(18)`)

Tranquilizer only (`EquipAIWith("hunter","tranqgun1_1st")`). Two black tigers respawned by a per-process `cMapTest::Update` spawner (rounds 2, 2, 3, 3, 6). `cScrTask09`: OnActivate `OpenMenuPage(13)`, sets `m_active`; its `Update` calls `Activate()` again every frame while active; `UpdateTrigger` hurts the player (`player.Hurt(-1,2,0)`) after 3 s inside (an out-of-bounds/water zone at BSP `[25]`, next to `trigger_hurt` 15 `[26]`).

| Mechanic | Where | Covered by / risk / gap |
|---|---|---|
| Per-process spawner, tiger respawns | `WORLD.SCR` | **GAP (desync)** as arena |
| **Tranq darts from clients** | `cMOD_ObjTranqBullet`; mirror hit detection is a health drop (`animals.cpp:1015-1024`) | **GAP (desync).** A dart probably does not lower `+0x19C`, so no `AnimalHit` is sent; clients cannot tranquilize. UNVERIFIED |
| **Self-re-arming zone `cScrTask09`** | `TRIGGERMAPISLAND.SCR` | **GAP.** The client's re-arm is a request; the host's re-arms are de-duplicated by the journal (`events.cpp:521`), so after the first fire the client's zone never re-arms: no out-of-bounds damage for clients. And a client entering first makes the **host** run `OpenMenuPage(13)` (a help page on the host's screen; whether it pauses the host is UNVERIFIED). VERIFIED code |
| Pickups, `trigger_hurt` | BSP | local, OK |
| Clips `MAPISLAND.AP` `pl_t` set | `combat.cpp:288` | **OK** |

## 9. MAPTSAVO (bonus, `SetMission(19)`)

Lion + lioness respawned by a per-process spawner (rounds 2, 2, 3, 3, 6), pickups on timers, four `trigger_hurt` death zones (`hurt value=1000`, BSP `[17-20]`), `cScrTask01` `StartTask("1")` after 1 s.

| Mechanic | Where | Covered by / risk / gap |
|---|---|---|
| Spawner / respawns | `WORLD.SCR` | **GAP (desync)** as arena |
| Death zones | BSP | local per process; host puppet forwarding UNVERIFIED (as 12A) |
| Clips `MAPTSAVO.AP` `pl_ln`, `pl_lns` | `combat.cpp:289` | **OK** (front only) |

---

## 10. Ranked gap list

Severity first (blocks progress > desync > cosmetic), then reach. Each has a fix proposal and a test recipe. Test runs use this checkout's `run-coop-local.ps1` with your own `-Port`; drive the host with one line per write in `game\Bin\dh2coop.cmd` (~1.2 s apart); logs `game\Bin\dh2coop.log` (host) and `game2\Bin\dh2coop.log` (client). Entity ids: host log `tracker: animal id=` / `animals: id=` lines, trigger ids from `story: … id=`.

**Exit caveat for every test:** the next map is the *profile's* (section 0.5). Starting mid-chapter (`-Map MAP11B/11C/12B/12C`, no `SetMission`) sends exits wherever the profile points. Test exit destinations from MAP11A/MAP12A only.

### G1 — Pilot maps: the local player is not found (blocks progress; MAP11A, MAP11C)
- Evidence: section 0.1. VERIFIED code, runtime UNVERIFIED.
- Fix: in `player_tracker.cpp` accept GameDll7 vtable `0x109d48` (`cObjPilot`) next to `0x106ff0` (or test `cObjEntity::IsClassKindOf(ent,"cObjHunter")`, which `0x45190` uses), same in `combat.cpp:144` `LocalHunter()`. Then check: `+0xE8` of the attached pilot follows the heli (attachment), the view camera stays within the 4 m `PlayerInCutscene` threshold (`story.cpp:88-95`; otherwise every fire defers forever), and on pilot maps drop puppets from the attack list (pilot Spawn does `RemoveAttackTarget`) or hide them.
- Test: `.\run-coop-local.ps1 -Map MAP11A -Seconds 60 -Port <p>`. Now (gap): client log has `tracker:   vt GAMEDLL7.DLL+0x109d48 x1` but no `tracker: local hunter entity`; no `events: request kind=9`; host log has no `events: sync player 1`. After the fix: client `events: synced with host on 'MAP11A'`, `story: applied host fire 'cScrTask01'`, `animals: mirror freeze id=… npc=true` for the siberians.

### G2 — `MoveToPos` respawns animals: client brains unfreeze, second lives never sync (desync; MAP11A/B/C, MAP12A, bonus maps; probably most story maps)
- Evidence: section 0.2. VERIFIED code. The terminal-death half is the code review's H1 (`re/notes/review-2026-10-17.md`, with MAP2B two-PC log proof: wolves 171-173 died and came back). New here: the engine cause (`0x14610`), and that every replayed `MoveToPos` also unfreezes a **live** frozen client mirror, not only a dead one.
- Fix (three parts):
  1. Client: in `ApplyMirror`, while `m.frozen`, read brain `+0x79` every frame and re-freeze when it reads 0 (log `animals: re-freeze id=… after respawn`). Or hook `0x14610` (thiscall(animal, int mode, ent); no SEH prologue — check its bytes before hooking, `CheckedRva` assumes `55 8b ec 6a ff`) and re-freeze after the original on client mirrors.
  2. Host: in `HostTick`, when an id in `announced_` reads hp > 0 and not parked, erase it and commit a new `kAnimalRespawn` (journaled, key = life number) — and give `kAnimalDied` the life number as key too, so `FindJournal` (`events.cpp:521`) does not swallow the second death.
  3. Client on `kAnimalRespawn`: `dead_.erase(id)`, reset the mirror (`death_presented`, `have_first`, `have_hp`, `applied_state`, `frozen=false` so it re-freezes).
- Test: `.\run-coop-local.ps1 -Map MAP11B -Seconds 150 -Port <p>`. Host lines: `coop_trigger cScrTask02` → wait 3 s → `coop_kill <wildboar1 id>`, `coop_kill <wildboar2 id>`, `coop_kill <wildboar3 id>` → `coop_trigger cScrTask06` (arm) → `coop_trigger cScrTask06` (fire: boars 1-3 respawn at t6). Gap: host `animals: sync role=host id=<wb1> … hp=` > 0, client `animals: sync role=client id=<wb1> … hp=0.0`, no second `animals: death commit id=<wb1>` after a second `coop_kill <wb1>`. Brain unfreeze (no log today): after `coop_trigger cScrTask02`, wildboar2 targets `"hunter"`; on the client window it chases the client's own hunter while the host log shows no `combat: animal id <wb2> hurt player 1`.

### G3 — Trigger `OnHit` side effects run only where the activator touched (desync; MAP11B, MAP12A; general)
- Evidence: section 0.3; MAP11B `:137`, `:296`; MAP12A `:350-352`, `:413-414`. VERIFIED code.
- Fix: for a client fire request whose host trigger is still armed with OnHit not run (`+0x1E4 == 1`, `+0x1E8 == 0`), the host calls the OnHit handler `0x6b140(trigger, host hunter)` instead of `Fire` (runs the script OnHit, respects `WaitTask`, and `ActivateAfter` schedules the fire, which is then committed as today). Host hunter is named `"hunter"`, so `ActivateBy("hunter")` passes. Keep `Fire` as the fallback for already-hit triggers.
- Test: MAP11B. Host: `coop_trigger cScrTask05` (arm). Client: `coop_tp 751.3:159.1:-578.7` (inside `cScrTask05`, BSP `[73]`), host stays at spawn. Gap: client HUD "Task Complete", host none; host log `story: [host] ran 'cScrTask05' (id N) for player 1` with no task completion. After fix: both complete race task 1.

### G4 — Bonus maps: the level-script spawner runs on every client (desync; MAPARENA, MAPISLAND, MAPTSAVO)
- Evidence: sections 7-9. VERIFIED script.
- Fix: G2 parts 1-3, plus on clients undo a local respawn that the host has not made: after `0x14610` on a client mirror whose last host sample is parked or dead, park it again (vt `+0x234`) and re-freeze. Longer term: skip `cMapTest::Update`'s spawn branch on clients (script hook) — not needed if the undo works.
- Test: `.\run-coop-local.ps1 -Map MAPARENA -Seconds 120 -Port <p>`, no commands. Gap: client `animals: sync role=client … drift=` jumps; client health drops with no host `combat: animal id … hurt player 1`; client `animals: id=<leopard> state=` changes the host log does not show.

### G5 — `cObjGuide` deaths fail the map outside `0x91c20` (desync; MAP11A/11C siberians, MAP11B Reggie, MAP12A Reggie/Wirake)
- Evidence: section 0.8; siberians are the boars' targets on 11A/11C (unprotected). VERIFIED code; runtime UNVERIFIED.
- Fix: hook `cObjNPC::OnDie` `0x5fde0` (vt `+0x154` on the five NPC vtables `0x108270` …). Host: after the original, if `GetClassType` is `cObjGuide`, commit `kMissionEnd` (code 6, the composed text). Client: for mirrored NPCs, run the original but swallow its stream write (or accept the duplicate; check that two code-6 messages do not open two results screens).
- Test (after G1): `.\run-coop-local.ps1 -Map MAP11A -Seconds 120 -Port <p>`, nobody shoots; the t21 boar kills `siberian2`. Gap: host shows MISSION FAILED with no `events: host mission message` line; client fails only if its mirror reached 0 (`animals: … hp=0.0` on the siberian id). Control on MAP11B (Reggie): host log `events: host mission message code=6 … caller=GAMEDLL7.DLL+0x99107` (the protected-kill path) appears.

### G6 — Ammo-gated tasks use the host's ammo only (desync/gameplay; MAP11A, MAP11C)
- Evidence: section 0.7 (`0x979f0` → `0x2f530`, local player). VERIFIED code.
- Fix: add ammo count to `PlayerState`; host hook on `0x2f530` returns "has ammo" if any living player has ammo. Client side already harmless (local fail swallowed).
- Test (after G1): MAP11A; host fires every round (empty the rifle) while the client keeps ammo. Gap: host log `events: host mission message code=6 text='MISSION FAILED!' caller=GAMEDLL7.DLL+0x99107` and both windows fail.

### G7 — Exits: each client loads its own profile's next map (desync; every exit; blocks tests started mid-chapter)
- Evidence: section 0.5 (code 7 written directly, map token ignored). VERIFIED code.
- Fix: on clients, swallow the `cObjTriggerLevel` activation (`0x6c420`/`0x6c610`, or the `ActivateTriggerLevel` native `0x6a4d0`) while online, and let `kMapStart` (`events.cpp:242-281`) do the load. Still call `SetMapIsComplete` on the client profile so its progression matches.
- Test: `.\run-coop-local.ps1 -Map MAP11A -Seconds 90 -Port <p>` (after G1), host `coop_trigger cScrTask08` ×2. Today: client `events: host map 'MAP11B' … (this client got here on its own)` or a second `cBspTree::Load` in the client `debug.log`. From `-Map MAP11B`, compare the host's and client's loaded map names: they can differ.

### G8 — Re-armed triggers, repeated prop anims and repeated `PlaySA` are de-duplicated by the journal (desync; MAPISLAND `cScrTask09`; general)
- Evidence: `events.cpp:521-527` matches `(kind, subject, key)` over the whole map. VERIFIED code. Same finding as code review M5; MAPISLAND `cScrTask09` is a concrete map case.
- Fix: commit an arm/fire/anim serial (e.g. key suffix or a per-subject counter in `i0` that `FindJournal` also compares); for `kTriggerEvent`, re-commit when the host's trigger was disarmed since the last arm.
- Test: `.\run-coop-local.ps1 -Map MAPISLAND -Seconds 60 -Port <p>`. Client: `coop_tp 226.9:-59.7:-22.2` (`cScrTask09`, BSP `[25]`), stay 6 s. Gap: one `events: commit kind=5 subject=<cScrTask09 id>` on the host, client health unchanged, host shows help page 13.

### G9 — NPC PlayAnim beats (knockdowns, cutscene anims) on frozen client NPCs (desync, visible; MAP12A, MAP11B)
- Evidence: MAP12A `:195-196`, `:261-262`, `:491-492`, `:583-584`; MAP11B cutscenes. Mirror sends body-state names only (`animals.cpp:716`, `:1082-1090`). UNVERIFIED whether `AITaskNPC_PlayAnim` calls `PlaySA(char*,char*)`.
- Fix: if host logs show no `story: PlaySA id=<npc> 'hurtdown'`, hook the NPC PlayAnim task's clip start on the host and commit it as `kPlaySA` without partner (already supported, `story.cpp:532-579`), and hold the pose (`animals.cpp:1072-1081`).
- Test: `.\run-coop-local.ps1 -Map MAP12A -Seconds 90 -Capture -Port <p>`. Host: `coop_trigger cScrTask04` → `coop_trigger cScrTask05` ×2. Compare screenshots of Reggie; grep host `story: PlaySA id=` and client `story: applied PlaySA`.

### G10 — `"hunter"`-bound companions and chases (desync; MAP12A Reggie companion, wolves/bears/boars → `"hunter"`, surprise wolves)
- Evidence: MAP12A `:36`, `:74`, `:104`, `:300`, `:389`, `:396`, `:440`, `:476`; MAP11B `:63`, `:96`, `:237`, `:280`; MAP12B `:24`. Partial cover: `combat.cpp:499-519` (only animals already chasing a player), `npc_follow.cpp` off. VERIFIED code. Owned by the `hunter-nearest` job.
- Test: `.\run-coop-local.ps1 -Map MAP12A -Seconds 90 -Port <p>` with `[Story] NpcFollowNearest=1` in the copied ini; client walks 30 m away; host log `npc_follow: NPC id <reggie> follows player 1`.

### Further gaps (lower)
- **G11** (desync, UNVERIFIED) Client tranq darts not forwarded (MAPISLAND). Test: client darts a tiger; host log `animals: AnimalHit id=` absent.
- **G12** (desync, UNVERIFIED) Big Grimm falling tree and yeti throws exist only on the host (MAP12B, MAP12C). Test: grep host `story: PlaySA`/`story: prop anim` around the tree attack; watch whether throws hit a client (`combat: animal id <yeti> hurt player 1`).
- **G13** (desync, UNVERIFIED) Client mission status stays 3 after a client death (`0x50350` → `0x965e0`; F9 `coop.cpp` `Respawn()` does not reset `+0x254`); task vtable function `0x96750` writes MISSION FAILED directly when it runs. Hand to the `mission-flow` job. Test: `coop_down` on the client, F9 (`coop_respawn`), keep playing to the next task change; watch the client window.
- **G14** (cosmetic) Helicopter lag/stall on clients (MAP11A/11C), until cutscene 2/3 resync. Test after G1: `-Capture`, compare heli position at the first `CompleteTask`.
- **G15** (cosmetic/gameplay, UNVERIFIED) Host puppets inside host `trigger_hurt` volumes may forward `kPlayerHurt` (MAP12A/12B, MAPISLAND, MAPTSAVO). Test: client `coop_tp` into a MAPTSAVO death zone; host log `combat: animal id <trigger id> hurt player 1`.
- **G16** (cosmetic, UNVERIFIED) Hogzilla back/left/right sync clips vs the one-clip `MAP11B.AP`. Test: let Hogzilla knock the client from behind; grep host `combat: sync attack '` and client `debug.log` for `Could not forward`.
- **G17** (cosmetic) Per-process traps, ice, EVENT_ANIMAL ambient animal, world items (each player loots their own copy — acceptable).

---

## 11. Open questions
- What the pilot's camera-to-entity distance is (the `PlayerInCutscene` 4 m test) — needs a run after G1.
- Whether `AITaskNPC_PlayAnim` without `play_sa` goes through `cObjCharacterEx::PlaySA(char*,char*)`.
- Whether `OpenMenuPage(13)` pauses the host.
- When the task vtable slot holding `0x96750` runs.
- NPC vt `+0x158` is an import thunk (`0xec8b4`) on all five NPC vtables; whether NPC `MoveToPos` also clears `CharacterBrainFreeze` bits was not checked.
- Who calls `0xbe480` (MAP12C outro movie) and whether a client applying `kMissionEnd` gets it.

## 12. Proposed MEMORY facts
- MAP11A and MAP11C spawn the player as `obj_pilot`/`cObjPilot`, not `cObjHunter`: GameDll7 ctor `0x64660` (calls hunter ctor `0x4d8e0`, then vtable **`0x109d48`**, script class `cScr_ModPilot`), `CreatecObjPilot` `0x650f0` (size `0x840`). `cObjPilot::Spawn` `0x65370` attaches the pilot to entity `helicopter` at tag `masscenter`, body state `Heli`, and calls `RemoveStimulus` + `RemoveAttackTarget` on itself. Evidence: `LEVELS/MAP11A/WORLD.SCR:93-94`, `MAP11C/WORLD.SCR:85-86`, decompile.
- Script `MoveToPos` on an animal respawns it: `cScrCharacterEx::MoveToPos` `0x161a0` → vt `+0x158(3,0)` = `0x14610` (`cScrAnimal` vtable `0xff470` slot dump at `0xff5c4`): health = `+0x4C0`, `+0x600 = 0`, dead bits cleared, state `IdleAttack`, brain `+0x79/+0x71/+0x84 = 0` (unfreezes `FreezeBrain`), `+0x4B0 = 1`. Script `Kill()` (`0x16a40`) = vt `+0x234` park `0x1be20` (`+0x4B0 = 0`, brain `+0x79 = 1`, -10000). `CanBeRespawned()` = `+0x4B0 == 0` (`0x164b0`/`0x194d0`).
- `cObjTriggerEvent` C++ OnHit handler `0x6b140`(trigger, activator): needs armed `+0x1E4`; `WaitTask` mask `+0x200` vs mission tasks; `ActivateBy` name `+0x224` vs activator name `+0x198` (else activator vt `+0xFC`); then disarm `0x68fd0`, script OnHit `+0x208`, `+0x1E8 = 1`. `ActivateBy` `0x69e40`, `WaitTask` `0x6ac90`.
- `cMissionTask::End` `0x96da0` calls the OnEnd (`+0xD4`, task state 3) or OnFail (`+0xD8`, state 5) `trigger_event`'s vt `+0x8C` with the mission hunter `+0x258` — no touch needed.
- `cObjTriggerLevel` activation `0x6c420`/`0x6c610` ignores the map token of `target` (`map,spawn`), passes the spawn token to `0x67b0`, calls `SetMapIsComplete`, and writes message `0x87` code 7 "Checkpoint reached" directly to `DAT_10185ea8` (not `0x91c20`). Next map = profile progression. `ActivateTriggerLevel` = `0x6a4d0`.
- Mission-end writes that bypass `0x91c20`: `cObjNPC::OnDie` `0x5fde0` for `cObjGuide` (code 6), trigger-level code 7, and `0x96750` (in task vtables `0x111b74/0x111c2c/0x111cb8/0x111d28`; code 6/5 by status 3/2). `0x965e0` callers: `0x50350` (hunter `OnDie`), `0x1a680` (×2).
- Task internals: `cTaskShoot::UpdateTaskStatus` `0x999a0` (action 1 kill, 2 KILL_PROTECTED → fail, 5 LOSE_TARGET), `cTaskShoot::CanBeCompleted` `0x979f0` (`NEED_AMMO_FOR_COMPLETE` = local player's ammo via `0x2f530`), `cTaskRace::UpdateTaskStatus` `0x9a170` (action 3 + task id). Constants: `MISSION_ACTION_*` 1..5 = KILL, KILL_PROTECTED, REACHED_POSITION, FIND_SOMETHING, LOSE_TARGET; `TASK_SHOOT 0`, `TASK_RACE 2`, `NEED_AMMO_FOR_COMPLETE 1`.
- `cObjWorldItem` use `0x432c0` adds to the user's profile; `next trigger` (`+0x1E0`) arms via `0x92210` → `0x68eb0`; `force trigger activation` (`+0x1F4`) calls the trigger's vt `+0x8C`. Pickup collision `0x45190` = `cObjHunter` + IsPlayer only.
- `AddEvent` `0x76b0` → `0x82f30` creates its pools at map load: traps ×10 (`trap1..10`), mines ×10, cages ×10, bees ×5, fire events ×5, one parked `obj_animal` (`animal1`) for EVENT_ANIMAL, and one each of snake/birds/bats/cavedust/fake event.
- `cScrHelicopter::SetAnim` `0x23fd0` (fly model part 0) and `BSMGoToState` `0x240f0`→`0x23cc0` are their own natives, not `cScrEntity::SetAnim`.
- `cObjNPC::OnDie` `0x5fde0` is vt `+0x154` on five NPC vtables (`0x1083c4, 0x108ba4, 0x108e8c, 0x10916c, 0x10945c` hold it).
- Teen paks (`TEENM\BODY\<MAP>.AP`): MAP11A/B/C `pl_wb_syncattack1`; MAP12A `pl_pb` front/back/left/right + `pl_gw`; MAP12B/12C `pl_pb_syncattack1`; MAPISLAND `pl_t` ×4; MAPTSAVO `pl_ln`, `pl_lns`; MAPARENA l/sb/gw/wb/ln/gb/r sets plus `syncattacktruck`.
