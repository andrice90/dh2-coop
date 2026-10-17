# Shootable world props (bottles, cans, stumps)

**Progress:** done. Questions 1–4 answered from the MAP1 BSP lump, story scripts, and GameDll7 slot-W decompiles (`re/notes/props-target.c`, `props-mission.c`, `props-stumps.c`, `props-strings.txt`). No game launch. Runtime id match for these entities is UNVERIFIED.

## 1. What these props are

They are BSP entities of class `obj_target` (`cObjTarget`, GameDll7). They are not `cScrTrap`, not `cObjCustomCharacterEx`, and not a script task object. `TARGET_CAN` is the species id the mission task watches (script constant **0x25**).

MAP1 `BSP\MAP1.BSP` places five `obj_target`s. None has a script class or script file.

| model | `target_name` | `generic/cheat` | role |
|---|---|---|---|
| `objects/level/msn/msn1/others/msn1bottle2.skin` (×4) | empty | `0` | the four cans/bottles |
| `objects/level/msn/msn1/trees/msn1trunk.skin` (×1) | `stump` | `1` | the cheat stump |

`MISSION01.SCR` task 3 is `CreateTask(TASK_SHOOT)` / `SetTaskAnimalToShoot(TARGET_CAN, ANIMAL_KILL, 4)` / `SetCompleteCondition(NEED_AMMO_FOR_COMPLETE)` / `ActivateTriggerOnEnd("cScrTask05")`. The triggers in `TRIGGERMISSION01.SCR` and `LEVELS/MAP1/WORLD.SCR` do not place the bottles. `WORLD.SCR` binds `trap1` as `cScrTrap` and `faketrap1` as `cScrCustomCharacterEx`. The BSP has `faketrap1` (`obj_cschex`, `objects/world/trap/trap.skin`) and a spawn named `foottrap1`. It has no entity named `trap1`. Those traps are a different prop from the bottles.

`generic/cheat` is a `cObjTarget` key (`HandleKey` at GameDll `0x26590`). The registration site pushes the string `generic/cheat` (RVA `0x101b50`, help text `used to specify generic/cheat bottle`) into the key at `DAT_10186250`. Value `0` is a bottle. Value `1` (also the constructor default at `+0x21c`) is a stump. Constructor `0x265e0` also stores species `0x25` at `+0x218`.

`TARGET_CAN = 0x25` is the script constant registered in `FUN_1000ba30`: at RVA `0xc92b` the bytes are `6a 25` then `push` of the `TARGET_CAN` string at `0xfd388`.

The same `obj_target` + `msn1trunk.skin` + `generic/cheat` `1` pattern is the stump on the other story maps: 5 on MAP2A, MAP2B, MAP2C, MAP3A, MAP3B, MAP3C, MAP4B, MAP5, MAP6, MAP7, MAP10, and MAP12A; 4 on MAP9, MAP12B, and MAP12C; 2 on MAP11B. MAP8 names them `sticla1`–`sticla5` but they are still trunk + cheat `1`, so they take the stump path. MAP1 is the only map with `msn1bottle2.skin` and cheat `0`. MAP4A, MAP11A, MAP11C, and the hunt BSPs have no `obj_target`. `msn1stump1.skin` / `msn1stump2.skin` occur in the MAP1 BSP bytes and are not an `entity("obj_target")` model. Whether those are static scenery is UNVERIFIED.

`mod_shootstumps` (`FUN_10005180`, profiler name `MOD_ShootStumps`) does not shoot anything. If GameDll global `DAT_101657f0` is set, it copies the level stump total `DAT_1016584c` into the hunter profile mission-status field `+0x90`.

## 2. How a hit is detected and counted

Not `cObjScriptedEntity::OnHurt` (`+0x14c` on this vtable is `0xec9e0`, a base thunk). Not `cObjTriggerHit`. The class overrides:

| method | RVA | vtable | convention |
|---|---|---|---|
| `HandleKey` | `0x26590` | `+0x14` | `__thiscall(this, char* key, char* value)` |
| `Precache` | `0x26be0` | `+0x54` | `__thiscall(this)` |
| `Spawn` | `0x26dc0` | `+0x58` | `__thiscall(this)` |
| `Update` | `0x26ee0` | `+0x44` | `__thiscall(this, float dt)` |
| `Draw` | `0x27280` | `+0x28` | `__thiscall(this)` |
| `OnHit` | `0x277f0` | `+0x8c` | `__thiscall(this, cObjEntity* other)` |
| `RayHit` | `0x27440` | `+0x94` | `__thiscall(this, float* a, float* b, …, hit*, …, int* filter)` |
| `Boom` | `0x27040` | `+0x188` | `__thiscall(this)` |
| vtable | `0x1017c8` | | `PTR_FUN_101017c8` |

`OnHit`, `RayHit`, `Boom`, and `UpdateMissionStatus` (`0x8390`) all start `55 8b ec 6a ff`.

`RayHit` returns 1 only when `*filter` is 1..4 and the model vfunc `+0x10c` reports a hit. It logs `target hit` and fills a hit record with type `0x40` and `this`. What `filter` is beyond that range check is UNVERIFIED. The bullet then calls `OnHit` (the only direct `call [reg+0x8c]` inside this function's own body is not how it is entered; `OnHit` itself requires `cObjEntity::IsEntityKindOf(other, "mod_obj_bullet")`).

`OnHit` (`0x277f0`), when the other entity is a `mod_obj_bullet`:

- Profile shot counters (`mission status +0x10/+0x14`, or `+0x20/+0x24` when `bullet+0x1b4` is 2 or 4) run only if the bullet parent's vfunc `+0xfc` is non-zero, a profile exists, and `DAT_101ada80 == 0`. What `+0xfc` and `DAT_101ada80` are is UNVERIFIED.
- If `+0x21c == 1` (stump) and `+0x208 != 0`, it returns. Otherwise, still under `DAT_101ada80 == 0`, it increments profile `+0x90` and builds `Stumps: %d out of %d.` or `You shot all of the stumps from this level.` against `DAT_1016584c`.
- If `+0x208 == 0`: calls vfunc `+0x188` (`Boom`), sets `+0x208 = 1`, and if `DAT_101657f0 != 0` and `+0x21c != 1` calls `FUN_10008390(1, 0x25)`.

That call is `UpdateMissionStatus`, `__cdecl(int action, int value)` at `0x8390`. It calls `cMission::UpdateMissionStatus` at `0x91e80` (`__thiscall(mission, action, value)`, mission `DAT_101869e8`). Action `1` and value `0x25` (`TARGET_CAN`) is the can count. Stumps do not call it.

`SetTaskAnimalToShoot` (`0x92780`) writes the current task (`*(mission+0x34)`, only while `task+0x6c == 0`): species at `+0xf0`, the third argument at `+0xfc`, and the count at both `+0xf4` and `+0xf8`. How a later tick turns "remaining hits == 0" plus `NEED_AMMO_FOR_COMPLETE` into `CompleteTask` is not in `OnHit`. `0x91e80` only decrements `mission+0x1e4` when `mission+0x1f0` is set, the value equals `mission+0x1e8`, and the action is 1 (lead note `re/notes/lead-mission.md`). The ammo test itself was not located in this pass. UNVERIFIED which function applies `NEED_AMMO_FOR_COMPLETE`.

`Boom` (`0x27040`) starts the break FX already created in `Precache`, plays the sound, and sets `+0x210 = 0`. `Precache`: cheat `0` loads FX `brokenglass` and `brokenglass2` plus sound `broken_glass` (and precaches `msn1bottle1.skin`, which is not the placed `msn1bottle2.skin`). Cheat `1` loads `brokentrunk` and `bullet_wood`. `Draw` draws the model only while `+0x210 != 0`, and draws the FX while `+0x208 == 1`. `Update` ticks those FX and, after `+0x20c` expires, marks the physics body and sets `entity+0xbc |= 2`. There is no `SetAnim` and no `Hide` script call.

Ids: bottles have an empty `target_name`, so a name lookup cannot tell them apart. They are normal BSP entities (`entity("obj_target")` then `property(...)`), same lump order on every process. The mod's `EntityById` uses that id. That ids match for these five is inferred from the shared BSP spawn path (`re/notes/lead-puppets.md`, `re/notes/lead-story-coop.md`). Not logged at runtime in this job. UNVERIFIED for `obj_target` specifically.

## 3. What a client's hit does today

The client's bullet hits the client's `cObjTarget`. `OnHit` runs there: the bottle breaks on that process only (`Boom`, `+0x208 = 1`, mesh hidden via `+0x210`), and `UpdateMissionStatus(1, 0x25)` runs on the client's `cMission`. Nothing in `mod/src/coop` watches `obj_target`.

- `AnimalHit` only reports a mirrored animal whose health dropped (`animals.cpp`). A bottle is not in that set.
- `kPropAnim` commits `cScrEntity::SetAnim` on the host for entities that are not the hunter, a puppet, a mirrored animal/NPC, or a child of the hunter (`story.cpp` `hk_SetAnim`). `Boom` never calls `SetAnim`, so a host bottle break is not sent, and a client break is not a request.
- Client `CheckMissionStatus` (`0x94e50`) is skipped while online (`re/notes/lead-mission.md`). `UpdateMissionStatus` (`0x8390` / `0x91e80`) is not hooked, so the client's own counter still moves. The host's bottle stays intact and the host's task does not count the shot.

So a client shot breaks and counts locally. It is not swallowed. It is not forwarded.

## 4. Proposed design

Same shape as `AnimalHit` / `kTriggerFire`: the client asks, the host applies the real hit, everyone else replays the break.

Do not call `OnHit` on the host for a remote shot. `OnHit` returns immediately unless `other` is a `mod_obj_bullet`, and that bullet exists only in the shooter's process. Calling it would also redo profile shot counters and the stump HUD.

Host, once per entity, when `+0x208 == 0`:

1. `cObjTarget::Boom` — `__thiscall(cObjTarget*)`, GameDll RVA `0x27040`, vtable `+0x188`, no other arguments. Prolog `55 8b ec 6a ff`.
2. Set `+0x208 = 1` (OnHit does this after Boom; Boom itself does not).
3. If `+0x21c != 1` and `DAT_101657f0 != 0`, call `UpdateMissionStatus(1, 0x25)` — `__cdecl`, GameDll `0x8390`. That is the can count. Skip it for stumps.

Resolve the target with `EntityById`. Bottles have no name.

Hook to sit on: `cObjTarget::OnHit` (`0x277f0`), or the tail above if the hook can see `this` after the bullet check.

- Host's own bullet: let `OnHit` run, then commit the entity id (the break already happened; do not Boom or count again).
- Client: do not call `OnHit`. Request the entity id.
- Host on request: the three steps above, then commit.
- Client on commit: if `+0x208 == 0`, call `Boom` and set `+0x208 = 1`. Do not call `UpdateMissionStatus`. The client's `Update` then runs the FX and hides the mesh the same way.

`kPropAnim` does not already cover this. Host breaks do not reach clients today.

Must not run twice:

- `UpdateMissionStatus(1, 0x25)` — host only, and only when `+0x208` was 0. A second call decrements `mission+0x1e4` again (`0x91e80`).
- `Boom` — guard on `+0x208`. A second Boom restarts the FX and the sound.
- Profile shot counters and the stump `+0x90` HUD — they live inside `OnHit` and are per process. Replaying `OnHit` on clients would add them again, and would no-op without a local bullet anyway. If the stump tally should match, increment `+0x90` once from the commit, not from a local `OnHit` that was also allowed to run.

`NEED_AMMO_FOR_COMPLETE` stays on the host's task, because only the host's `UpdateMissionStatus` runs. Where that flag reads ammo was not found here.

## Proposed MEMORY facts

- `obj_target` / `cObjTarget` is the shootable bottle and stump. MAP1 BSP: four `msn1bottle2.skin` with `generic/cheat` `0` and empty `target_name`, one `msn1trunk.skin` named `stump` with `generic/cheat` `1`. Evidence: `re/extracted/BSP/MAP1.BSP` entity lump; class string `cObjTarget` at GameDll RVA `0x1017bc`; vtable `0x1017c8`.
- `TARGET_CAN` is script constant `0x25`. Evidence: `FUN_1000ba30` at RVA `0xc92b` (`6a 25`, then push of string RVA `0xfd388`). `cObjTarget` constructor `0x265e0` stores `0x25` at `+0x218`. `OnHit` passes that literal to `UpdateMissionStatus`.
- A bottle/stump break is `cObjTarget::OnHit` (RVA `0x277f0`, vt `+0x8c`, `__thiscall(this, cObjEntity*)`, other must be kind `mod_obj_bullet`) then `Boom` (RVA `0x27040`, vt `+0x188`, `__thiscall(this)`). Broken flag is `+0x208`. Cheat/stump flag is `+0x21c` (`generic/cheat`). Draw gate is `+0x210` (Boom clears it). Evidence: `re/notes/props-target.c`.
- Non-stump hits count with `UpdateMissionStatus(1, 0x25)`, `__cdecl` GameDll `0x8390`, which calls `cMission::UpdateMissionStatus` `0x91e80`. Evidence: call at RVA `0x279fb` inside `OnHit`; body in `re/notes/props-mission.c`.
- `kPropAnim` does not replicate this break. `Boom` does not call `SetAnim`. Evidence: `Boom` in `props-target.c`; `hk_SetAnim` in `mod/src/coop/story.cpp`.
