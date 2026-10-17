# NPC follow / guide — where "the player" is stored

## Progress
Done. Slot W. No game launches. Questions 1–5 answered from GameDll / CommonDll decompiles. Cobra task class not opened. `FUN_100aae00` storing the queued entity, and where follow-task `+0x114` is filled, are still UNVERIFIED.

Raw dumps: `re/notes/npcf-*.c` and `re/notes/npcf-*.txt` (gitignored).

## 1. SetAIType

Script `cScrNPC::SetAIType` is GameDll `FUN_10060db0` (RVA `0x60db0`), registered by `FUN_10063510` (`0x63510`, profiler `cScrNPC::ScrExport`). If the script object `+0x28` is non-zero it calls `FUN_1005fcb0(*(this+0x28), type, name)`. `+0x28` is the C++ entity.

`cObjNPC::SetAIType` is `FUN_1005fcb0` (RVA `0x5fcb0`). It stores the type flags at entity `+0xb60`. The second argument is read only when bit 0 is set (`AITYPE_COMPANION` = 1):

- `FUN_10006c30()` (local player) is stored at entity `+0xb68` when non-null.
- `FUN_10057600(this, name)` then scans the list at entity `+0xb4` (count `+8`, array `+4`) for another `cObjNPC` whose name at `+0x198` matches the string by byte compare. On a match it replaces `+0xb68` with that NPC.

`AITYPE_GUIDE` is 0, so guide (including `AITYPE_GUIDE|AITYPE_COVERSTAY` = 2) does not read the string and does not write `+0xb68`. `"a"` and `""` matter only for a companion, and only if some `cObjNPC` is actually named that. `"hunter"` is `cObjHunter`, not `cObjNPC`, so the companion pointer stays the local player.

Constants from the same registrar: `AITYPE_COMPANION` = 1, `AITYPE_COVERSTAY` = 2, `AITYPE_IDLESCARE` = 4, `AITYPE_COVERRUN` = 0, `AITYPE_IDLENORMAL` = 0.

What is stored is an entity pointer, not an id, not a script index, and not a name resolved later. The only later name step is the companion `cObjNPC` override inside `SetAIType` itself.

`cObjGuide` ctor `FUN_10062720` zeroes `+0xb68` and calls `FUN_1005fcb0(this, 0, NULL)`.

Writers of `+0xb68` in GameDll (DispHits): `FUN_1005fcb0` (`0x5fcf9`), `FUN_10057646` (`0x576cc`, the name override inside `0x57600`), `FUN_10062720` (`0x627bf`, the zero). The only reader is `FUN_100ad050` (three reads). `FUN_1005f460`'s `PUSH 0xb60` is a constant, not this field. `FUN_100ab7c0` tests bit 2 of `+0xb60` (cover stay) and does not read `+0xb68`.

## 2. Follow / guide tasks

Hello and follow distances are on the C++ brain, not the stored target.

- Brain script methods `FUN_100a4f40` / `FUN_100a4f50` (registrar `FUN_100ad9e0`, the `brain.SetHelloDistance` / `brain.SetFollowDistance` path) write `*(script+0x28)+0x130` and `+0x134`. `script+0x28` is the C++ brain.
- NPC-object methods `FUN_10059600` / `FUN_10059620` write the same offsets through `entity+0xc48` (brain pointer). GUIDE.SCR calls them on `brain`.

Per-frame player read is `FUN_100ad050` (RVA `0xad050`), `cAIBrain_NPC` vtable slot `+0x7c`. Vtable RVA `0x112d30`; slot `+0x1c` is `FUN_100ac8e0` (`cAIBrain_NPC::Update`), which calls `(*(vt+0x7c))(dt)`. `0xad050` is the only data xref of that slot. Animals use `cAIHuntBrain_Animal`, not this vtable.

`FUN_100ad050` gets the player once:

```
uVar5 = (*DAT_10185e64)();
piVar6 = FUN_10006ae0(uVar5);   // call at RVA 0xad093
```

`FUN_10006ae0` (RVA `0x6ae0`) resolves that index through `DAT_10185e28` / `DAT_10185e38` of `+0x3c`, requires vtable `+0xfc()` non-zero, calls `FUN_1004ddd0()`, and returns the entity or 0. `FUN_10006c30` (`0x6c30`) is the same resolution with an `index >= 0` check. `SetAIType` calls `0x6c30` once. The brain tick calls `0x6ae0` every time `+0x1a4 == 0` reaches the distance block. Neither is the animal attack list.

Brain `+0x190` (decompiler `this+400`) is the NPC entity. `local_18` is the 3D distance from that entity's `+0xe8` to the resolved player's `+0xe8` (`piVar6[0x3a/0x3b/0x3c]`). If companion bit 0 is set and `entity+0xb68 != 0`, `local_1c` is the distance to the stored pointer.

The stored pointer is used only when the task list at brain `+0xc8` (`this+200`) has count 0, `+0xb68 != 0`, bit 0 is set, and `local_1c > DAT_101132dc`. Those four bytes at RVA `0x1132dc` are `00 00 00 41` (float 8.0). That branch sets the queued target `local_a4 = *(entity+0xb68)`. Every other path, including every guide (bit 0 clear), queues `piVar6` and compares `local_18` to brain `+0x130` (hello) and `+0x134` (follow). Bit 2 (`AITYPE_COVERSTAY`) is tested in `FUN_100ab7c0`, which takes an entity and reads `+0xe8`; it does not call `0x6ae0`. Bit 4 plays `"IdleScare"`.

An earlier branch in the same function (interaction state 2 or 8) also queues `piVar6` as `local_a4` with `local_b4 = 5`, via `FUN_100aae00`.

`cAITaskNPC_Follow::Update` is `FUN_100baab0` (`0xbaab0`). The target entity is task `+0x10`. Position is task `+0xa8`, or `*(task+0x10)+0xe8` when that vector is null. `HasReachedTheTarget` is `FUN_100ba870` (`0xba870`): XZ distance from that position to `(*(task+8)+0x68)+0xe8`, compared to task `+0x114`, not brain `+0x134`. Where `+0x114` is assigned was not traced (UNVERIFIED). `Begin` (`FUN_100b2390`, `0xb2390`) sets state 4 and returns if task `+0x10` is 0.

`cAITaskNPC_Hello::Update` is `FUN_100b7200` (`0xb7200`). It distances NPC `task+0xb8` `+0xe8` against target `task+0x10` `+0xe8` and brain `task+0xb4` `+0x130`. No class check on the target.

`FUN_100aae00` is the enqueue that receives `local_a4`. That it writes task `+0x10` is assumed from the tasks reading `+0x10` afterwards, and is UNVERIFIED until `0xaae00` is decompiled.

`cAITaskNPC_Follow::SetParam` (`FUN_100b2fe0`, `0xb2fe0`) resolves the value with `FUN_10006d80` and classifies `cObjWayPoint`, `cObjAnimal`, `cObjHunter`, `cObjGuide`, and the other named NPC classes. That is the script `SetTaskParam` path, not the per-frame tick. The decompiler then appears to overwrite the class type before `FUN_100aae00`; treat that fall-through as UNVERIFIED. A puppet's class string is not `cObjHunter`, so this SetParam path can reject it. The brain tick passes the pointer into `FUN_100aae00` with `local_b4 = 5` and no class check.

## 3. Retarget hook point

Recommended site: the instruction after the `CALL FUN_10006ae0` in `FUN_100ad050`.

Bytes at GameDll RVA `0xad088`: `64 5e 18 10  8b 0d f0 57 16 10  50  e8 48 9a f5 ff`. The `E8` is at `0xad093`; rel32 lands on `0x6ae0`. Next instruction `0xad098` is `8b d8  8b ce  89 5d e4` (`mov ebx,eax; mov ecx,esi; mov [ebp-0x1c], ebx`). ESI is the brain. Replacing EAX before that `mov` replaces the player used for guide, for the near-companion path, and for the earlier state 2/8 enqueue.

Companion-far still reads `+0xb68`, which `SetAIType` captured once. The same hook has to write `*(brain+0x190)+0xb68` when `*(byte*)(npc+0xb60) & 1` is set. That field has no other reader.

**(a) Overwrite `+0xb68` only.** Misses every guide, because guide never reads it. Safe for other systems: only `FUN_100ad050` reads the field.

**(b) Hook `FUN_10006ae0`, `FUN_10006c30`, or `FUN_10006d80`.** Too wide. `0x6ae0` and `0x6c30` are the local-player getter (HUD-range callers, animal update, `FUN_100c8800` many times). `0x6d80` is the universal name-to-entity scan (`stricmp` of `+0x198`), used by `GetDistBetween2Ais`, `FindEntScrObject`, NPC task `SetParam`, triggers, and `AddAIToBeProtected`. A `"hunter"` filter there also rebinds `setidx(player, FindEntScrObject("hunter"))` at map start. Puppets are not named `"hunter"`.

**(c) Hook only the distance compare inside `FUN_100ad050`.** The compare already uses the host hunter's `+0xe8`. Changing the threshold does not change who is followed.

**Puppet type checks.** `cScr_ModHunter` vtable GameDll `0x106ff0` slot `+0xfc` is `FUN_10065970`, which returns 1. That is the check `FUN_10006ae0` / `FUN_10006c30` apply. It is not CommonDll `cObjHumanEx::IsPlayer` (bit 3 of `+0x3B4`). A puppet whose `+0xfc` returns 0 fails those getters. Substituting the pointer after they return skips the check. The follow and hello tasks then only need a valid `+0xe8`. They do not require `cObjHunter`, `cScr_ModHunter`, the IsPlayer bit, or a physics body at `+0x118`. `cObjEntity::SetPos` (CommonDll `0xbc130`) writes `+0x0C` / `+0xE8` / `+0x100`; the physics body is separate.

`FUN_1004ddd0()` runs inside `0x6ae0` before the return. Replacing EAX does not undo that call. Whether it AddRefs the original hunter is UNVERIFIED.

## 4. Script-side "hunter" lookups

`GetDistBetween2Ais` is GameDll `FUN_10009200` (RVA `0x9200`), cdecl `(char*, char*)` to float. Both names go through `FUN_10006d80`. Distance is 3D of `+0xe8/+0xec/+0xf0`. If either name misses, it returns `_DAT_100fc64c`. It does not call `FindEntScrObject`.

`FindEntScrObject` is `FUN_10006f00` (`0x6f00`). It calls `FUN_10006d80`, then if `IsEntityKindOf "obj_scripted_entity"` returns entity `+0x1b4` (script index), else `-1`. Requires `DAT_101657f0 != 0`.

`FUN_10006d80` (`0x6d80`) walks `(*DAT_10185e34)()` via `(*DAT_10185e38)(i)`, requires `IsClassKindOf "cObjBaseEntity"`, and `stricmp(*(entity+0x198), name) == 0`. Empty name returns 0.

One hook on `0x6d80` would cover every script name lookup, including the map-start `player` bind. That is the wrong width. A hook only on `0x9200`, when one argument is `"hunter"`, covers the story distance checks (MAP1, MAP2C, MAP4B, MAP12A) and does not cover `FindEntScrObject` or NPC follow. Follow does not call `0x6d80` each frame.

## 5. Animals' "hunter" task targets

`SetTaskParam("target", ...)` on the attack tasks is CommonDll `cAIHuntTask_AttackBase::SetParam`, `FUN_100bc3d0` (RVA `0xbc3d0`, profiler catch `0xbd830`). Callers: `FUN_100c4ef0` and `FUN_100c92f0` (plus unowned xrefs). The key compare is the 7-byte string `target`.

On match, if task `+8` (the brain) is non-zero:

1. Owner entity is `*(brain+0x68)`. Its vtable `+0xf0` is `cObjBaseEntity::FindEntity` (CommonDll `0x9250` on every scripted-entity vtable that also points `SpawnEntity` at `+0x16c`). `FindEntity(name, "obj_base_entity")` walks the list at `entity+0xb4` and `stricmp`s `+0x198`. `"hunter"` hits the host hunter. A puppet with another name misses.
2. If that returns non-zero, brain vtable `+0x7c` is called. On `??_7cAIHuntBrain_Animal@@6B@` (RVA `0x123888`) that slot is `cAIHuntBrain_Animal::StartAttackTask` (`0x83830`).
3. `StartAttackTask` calls `SetTarget` (`0x80ce0`, id at brain `+0x254`) only when `cAIHuntAnimalInterface::IsAttackTargetInList(DAT_101aea7c, entity)` is true. Otherwise the param is a no-op. Default state argument is task `+0x148` (set back to 1); surprise id is task `+0x14c` (set back to -1).

So the script name is resolved once, to a named entity, and then accepted only if that entity is already on the attack list. It does not use GameDll `FUN_10006d80`.

`GetClosestAttackTarget` (`0x81ac0`) has one code caller: `ProcessAttackStuck` (`0x8b800`). That runs only when the current target (id at `+0x254`, resolved by `(*DAT_101ad8d0)`) is at least as far as `*(target+0x118)+0x7c * DAT_10130464 + *(animal+0x118)+0x7c + DAT_10121e40`. Both entities' `+0x118` are dereferenced. If the closest list entry differs from the current target and `(*(brain+0x158))[1] == 1`, it `SetTarget`s the closest. P5d "re-targets to the closest anyway" is this stuck path only, not every frame. A puppet with no physics body faults the `+0x118` read. Other callers of `GetClosestAttackTarget`: none in code; two xrefs with no function.

`cScrAnimal` vtable GameDll `0xff470` slot `+0x16c` is the import of `cObjScriptedEntity::SpawnEntity` (CommonDll `0x3e590`). The script thunk CommonDll `0xe0c0` is `jmp [eax+0x16c]`. That slot is not the task-param resolver. Do not hook it for `"target"`.

Cobra (`cObjCobra` / `Task_CobraAttack`) was not decompiled. Whether it shares `FUN_100bc3d0` is UNVERIFIED. GameDll animal spawn `FUN_10014610` calls GameDll `FUN_10006d80("hunter")` once on a spawn flag; that is a different path from the task param.

## Proposed design

On the host only, in `FUN_100ad050` at RVA `0xad098`, replace the `FUN_10006ae0` result with the nearest living player before `mov ebx,eax`. Also store that pointer at `*(brain+0x190)+0xb68` when the NPC's `+0xb60` bit 0 is set, so the far-companion branch follows the same entity.

Nearest player: among the host hunter (`cScr_ModHunter` vtable GameDll `0x106ff0`) and remote puppets that have a position at `+0xe8`, pick the minimum squared distance of `+0xe8` to the NPC entity at `brain+0x190`. Skip dead entities (`+0xC1` bit 2 is the flag `HaveValidTarget` uses; this job did not re-decompile that check). Do not require the IsPlayer bit or the `cObjHunter` class string. Leave client NPC brains frozen; the host already ticks them.

Leave these on the host hunter:

- `FindEntScrObject("hunter")` (script `player`, weapons, health, stealth).
- `FUN_10006c30` / `FUN_10006ae0` everywhere except the one call inside `FUN_100ad050`.
- Mission claim and host death.

`GetDistBetween2Ais` (`0x9200`) is a separate, narrower hook if those four maps' distance checks should also use the nearest player. It does not move follow, and it does not cover `FindEntScrObject`.

Do not retarget animals with the NPC hook. Animal `"hunter"` is `FindEntity` plus an attack-list membership test. Closest-target switching is `ProcessAttackStuck`, and it dereferences `+0x118`. Putting puppets on the attack list is the existing combat plan; this follow hook does not do that.

## Proposed MEMORY facts

- GameDll `cScrNPC::SetAIType` = `FUN_10060db0` RVA `0x60db0`. `cObjNPC::SetAIType` = `FUN_1005fcb0` RVA `0x5fcb0`: flags at entity `+0xb60`; companion (bit 0) stores `FUN_10006c30()` at `+0xb68`, then `FUN_10057600` may replace it with a same-named `cObjNPC`. Guide does not read the name. Evidence: `re/notes/npcf-setai.c`, `re/notes/story-natives-gamedll.c` lines 1036–1041.
- GameDll `+0xb68` readers: only `FUN_100ad050`. Writers: `0x5fcb0`, `0x57646`, ctor `0x62720`. Evidence: `re/notes/npcf-disp.txt`.
- GameDll `cAIBrain_NPC` vtable RVA `0x112d30`, `Update` slot `+0x1c` = `FUN_100ac8e0`, per-frame player slot `+0x7c` = `FUN_100ad050`. The `CALL FUN_10006ae0` is at `0xad093`; next instruction `0xad098` is `mov ebx,eax`. Evidence: `re/notes/npcf-probe4.txt`, `re/notes/npcf-brainuse.c`.
- Brain `+0x130` hello distance, `+0x134` follow distance, written by `FUN_100a4f40` / `FUN_100a4f50`. Companion stored-target branch runs only when distance to `+0xb68` is greater than the float at RVA `0x1132dc` (bytes `00 00 00 41`, 8.0). Evidence: `re/notes/npcf-resolve.c`, `re/notes/npcf-probe.txt`, `re/notes/npcf-brainuse.c`.
- `cScr_ModHunter` vt `+0xfc` = `FUN_10065970` returns 1. That is the gate in `FUN_10006ae0` / `FUN_10006c30`, not `IsPlayer` bit 3. Evidence: `re/notes/npcf-isplayer.c`, `re/notes/npcf-resolve.c`.
- `GetDistBetween2Ais` `FUN_10009200` and `FindEntScrObject` `FUN_10006f00` both resolve names with `FUN_10006d80`. Evidence: `re/notes/npcf-resolve.c`.
- CommonDll `cAIHuntTask_AttackBase::SetParam` `FUN_100bc3d0` (`0xbc3d0`) resolves `"target"` with `cObjBaseEntity::FindEntity` (`0x9250`, entity vt `+0xf0`, name at `+0x198`) and calls `StartAttackTask` (`0x83830`, brain vt `+0x7c`), which `SetTarget`s only if the entity is already in `DAT_101aea7c`. Evidence: `re/notes/npcf-attackset.c`, `re/notes/npcf-findent.c`, `re/notes/npcf-settargetvt.c`.
- `GetClosestAttackTarget` (`0x81ac0`) is called from `ProcessAttackStuck` (`0x8b800`) only, and that path dereferences both entities' `+0x118`. Evidence: `re/notes/npcf-closest.txt`, `re/notes/objmodel-decomp-common-ai.c` around line 9916.

## Open questions

- Does `FUN_100aae00` copy `local_a4` into task `+0x10`? Not decompiled.
- Where follow-task `+0x114` is initialized relative to brain `+0x134`.
- `cAITaskNPC_Follow::SetParam` class-type fall-through (decompiler structure vs assembly).
- Whether `Task_CobraAttack` uses `FUN_100bc3d0`.
- Whether `FUN_1004ddd0` inside `FUN_10006ae0` AddRefs the returned entity.
- What `entity+0xb4` is on the animal `FindEntity` walk (same list shape as the NPC name override; treated as the entity manager list). Identity of `DAT_10185e64` / `e28` / `e38` / `e34` not pinned down beyond "script/object index functions".
- Base `cObjHumanEx` vt `+0xfc` body (expected to be `IsPlayer`, not dumped here).
- `FUN_100ab7c0` is only known to test cover-stay bit 2. If its caller passes the same player pointer `FUN_100ad050` just queued, cover follows the substitute. That caller was not confirmed.
