# Animal park at (-10000, -10000, -10000)

**Progress:** questions 1–4 written. Constant scan, Ghidra decompiles, and the AttackCats state machine are done. Which of the two flee triggers fired for MAP2A lion 157 is UNVERIFIED (no `reached flee_point` line in the kept logs). No game launch.

Raw dumps: `re/notes/park-game-probe.txt`, `park-caller.c`, `park-flag.c`, `park-common.c`, `park-flee-set.c`, `park-pred.c`, `park-cats.c`, `park-ca8e0.c`, `park-state4.c`, `park-state800.c`, `park-unpark.c`.

## Question 1 — what writes the sentinel

The live position write is one function. `cScrAnimal` (GameDll vtable `0xff470`) slot `+0x234` is `FUN_1001be20` (RVA `0x1be20`). It builds a stack vector `(-10000, -10000, -10000)` (`0xC61C4000` three times) and calls entity vfunc `+0xA4`.

That slot is `cObjScriptedEntity::Move` (CommonDll export, RVA `0x3e700`). `Move` calls vfunc `+0xA8`, which is `cObjEntity::SetPos`. `SetPos` writes entity `+0xE8`, so the position becomes exactly the sentinel, not a delta. The same function then:

- sets the brain byte `+0x79` to 1 (the `FreezeBrain` flag; the body-state name is not changed, which is why the log stays `LookL45` / `KeepAnim`);
- calls `cAIHuntAnimalInterface::RemoveDeadAnimal` on global `DAT_101657f8`;
- clears animal `+0x4b0` and `+0x400`.

`cScrAnimal::Update` is vtable `+0x44` = `FUN_1001f3c0` (RVA `0x1f3c0`). Near the top, if animal `+0x590 == 1` and `+0x600 == 0`, it sets `+0x600 = 1` and calls vfunc `+0x234`. That is the runtime park. A second call of `+0x234` is later in the same function, only after health `+0x19C` has already reached 0 and two countdowns (`+0x6F4`, then `+0x450`) expire. Lion 157 was logged at hp 80, so that second call is not this bug.

`+0x590` is set in exactly one place: GameDll `FUN_10014940` (RVA `0x14940`). If the entity `IsEntityKindOf("obj_animal")`, it stores 1 at `+0x590` and `SendToLog`s `Animal %s reached flee_point`. GameDll installs that function as `cAIHuntAnimalInterface+8` (`mov [eax+8], 0x10014940` at RVA `0x1890`; the interface pointer is `[0x101657f8]`). CommonDll `cAIHuntAnimalInterface::Reset` (`0xa0f50`) first stores the empty export `FuncOnFleePointReached` (`0x9a660`, a cookie check and `ret`) at `+8`. The export itself does not move the animal.

Spawn uses the same park. `cScrAnimal` vfunc `+0x58` = `FUN_1001e170` (RVA `0x1e170`) saves health, writes `+0x19C = 0`, calls `+0x234`, then restores health. That is why a freshly spawned story animal sits at the sentinel with its real hp. `data.arc` has no `-10000` float or text (scripts are compressed; the literal is not in the DLL string table either).

Other `-10000` hits are not this animal:

| Site | What it is |
|---|---|
| GameDll `FUN_100379f0` `0x379f0` | weapon/item spawn, `Move`s the item to the sentinel |
| GameDll `FUN_10087410` `0x87410` | cage spawn, same `Move` |
| GameDll `FUN_10086d40` `0x86d40` | item ctor, fields `+0x240/+0x244/+0x248`, not `+0xE8` |
| GameDll `FUN_10087850` `0x87850` | `Move`s to the sentinel, then copies a saved point onto another entity if it is not the sentinel |
| GameDll `FUN_1003af50` `0x3af50`, `FUN_10088790` `0x88790` | `Move(&DAT_10162008)`; the data triple is RVA `0x162008` |
| GameDll `FUN_10015120` `0x15120` | one float, terrain-height failure sentinel |
| CommonDll file `0x38841`, `0x3895e`, `0x389a1` | one component `-10000` followed by a `0`, not the position triple |
| EngineDll file `0x1451a7` | one component |

## Question 2 — the condition

The park is "flee point reached", not a metres check against `GetPlayer`.

`cAIHuntBrain_Animal::Update` (CommonDll `0x95530`) calls `interface+8` with `brain+0x68` (the animal) when `brain+0x288 == 1` and brain `+0x79` is not already 1, then clears `+0x288`. Two path tasks set `+0x288` when the path index runs off the end and brain `+0xA0` does not have bit `0x1000`: `FUN_100a8510` (`0xa8510`) and `FUN_100acac0` (`0xacac0`).

MAP2A's lion is `AIHuntTask_AttackCats`. The trigger sets `SetTaskParam("state", "2048")`, which is task `+0x18 == 0x800`. The task vtable is CommonDll `0x136460`. `Update` is slot `+0x24` = `FUN_100cb8f0`, which calls `FUN_100ca8e0` (`0xca8e0`). That function switches on `+0x18`:

| `+0x18` | Slot | Function | Effect |
|---|---|---|---|
| `4` | `+0x8C` | `FUN_100c8d60` | calls `interface+8` immediately (the park callback) |
| `0x1000` | `+0x94` | `FUN_100c9080` | calls `interface+8` if task `+0x17C == -1` |
| `0x800` | `+0x90` | `FUN_100c5bf0` | the scripted attack state |

`FUN_100c5bf0` walks `cAIHuntAnimalInterface::GetAttackTargetAt`. For each entry it compares XZ distance to an anchor at task `+0x180/+0x188` (filled by `FUN_100c59c0` when the anchor is still the null vector; what that function stores is UNVERIFIED) against a radius at the current attack record `+0x10` (metres UNVERIFIED). The first entry inside that radius becomes the target (`SetTarget` if it is not already). It then sets `+0x18 = 0x1000` and calls `interface+0xC` (`FuncOnPlayJumpSurprise`, not the park). The attack list already includes puppets when combat has added them. This loop does not call `GetPlayer`.

State `4` is stored in one place: `FUN_100c1920` (`0xc1920`, slot `+0x84`, the state-`2` handler) when task `+0x54 == -1`. State `2` is set when a path query's result `+0xC` is 0 (`FUN_100c1bc0` at `0xc1bc0`, and the same pattern in `FUN_100c46a0` / `FUN_100c87f0`). `+0x54` is initialised to `-1` and set back to `-1` inside `FUN_100cadf0` when a sync attack starts.

The player test that does **not** see a puppet is entity vfunc `+0xFC` (returns 1 on `cScr_ModHunter`, 0 on a puppet; see `re/notes/npc-follow.md`). Two attack-task uses:

1. `cAIHuntBrain_Animal::ProcessAttackState` (CommonDll `0x92710`). Call at `0x92792`: `8B 06 8B CE FF 90 FC 00 00 00` then `83 F8 01 74 1A` at `0x92798`. If `+0xFC` returns 1 it returns and does not call `TellTargetIsUnderAttack` / `DoDesperateAttack` / `DoDesorient`. A puppet fails the test, so the attack state falls through. ESI is the target.
2. `FUN_100cadf0` (`0xcadf0`), the sync-attack predicate, called only from slot `+0xBC` = `FUN_100cba60` (`0xcba60`). The gate is `(task+0x9C <= task+0xA0) || (vt+0xFC() != 1)`. The call is at `0xcaecb`: `8B 13 8B CB FF 92 FC 00 00 00`, then `83 F8 01 0F 84 0D 01 00 00` at `0xcaed1`. EBX is the target. `MOUNTAINLION.SCR` says `minsatime` `5.0` is "only for the player". That matches this OR. Which task field holds the `5.0` is UNVERIFIED (`+0xA0` is the compare's second float). A puppet skips the wait. On success the function sets task `+0x54 = -1`, task `+0x18 = 0x2000`, and `FUN_100cba60` sets `+0x17C = -1`. A third `+0xFC` call at `0xcb157` (`83 F8 01 75 06` then `89 BE 9C 00 00 00`) zeros `+0x9C` only when the return is 1, so only a real player resets the timer.

`GetPlayer` is not this path. `FUN_10006c30` (`0x6c30`) appears in `FUN_1001f3c0` only when health is already 0 and `+0x404 != 0`: distance to that one entity under `5.0` (`DAT_100ff82c`) calls vfunc `+0x238`, the claim, not the park. `FUN_10006ae0` has no caller inside the attack task. `DAT_100fc648` is `25.0` used as a squared distance (5 m) between two animals when picking collision words.

Host-at-100 m is therefore not a coded leash of 100. Animal ctor field `+0x70C = 100.0` (`0x42C80000`) is never read back as a displacement in either DLL. The session shape (IdleAttack / LookL45 next to the client, host ~120 m away, then the sentinel with the same state name and hp 80) matches the flee callback: freeze, `Move` the sentinel, leave the clip alone. Which trigger set `+0x288` or task state `4` / `0x1000` with `+0x17C == -1` for entity 157 is UNVERIFIED.

## Question 3 — what un-parks

Walking back into range does not. After `FUN_1001be20`, `+0x4b0` is 0, so `FUN_1001f3c0` returns before the rest of the update, and the brain `+0x79` freeze skips senses. Nothing in that update writes a real position back.

`cScrCharacterEx::MoveToPos` is GameDll `FUN_100161a0` (RVA `0x161a0`). It resolves the name with `FUN_10006d80` and calls the entity's `Move` (`vfunc +0xA4`) with that entity's `+0xE8`, then copies `+0xF4`. MAP2A `mlion1.MoveToPos("wolfs_t4_02")` is that call. It does not search for the player. What clears brain `+0x79` afterwards is UNVERIFIED (`MoveToPos` also calls entity vfunc `+0x158(3, 0)` first).

## Question 4 — smallest change that counts every player

Do not nop `FUN_1001be20`. Spawn and a real flee both use it.

The player test to widen is vfunc `+0xFC` on the **current target**, not `FUN_10006ae0` / `FUN_10006c30`. Those getters are the local hunter for HUD, weapons, mission scripts, and the dead-animal 5 m claim. `re/notes/npc-follow.md` already rules out hooking them.

Same substitution as `npc_follow.cpp`: a mid hook after the `call [vt+0xFC]`, and if the target is a living puppet (`puppets::NearestLivingPuppet` is the wrong helper here; test `puppets::PlayerOf(target) != kInvalidPlayerId` and the dead byte `+0x6CC` the way `npc_follow` tests the hunter), set EAX to 1 before the `cmp eax, 1`.

**Site A, the attack-state fallthrough** (this is the one that lets a puppet leave the "keep attacking" path):

- CommonDll RVA `0x92792`, bytes `FF 90 FC 00 00 00`.
- Next instruction `0x92798`: `83 F8 01 74 1A`.
- Verify the 14 bytes at `0x9278e`: `8B 06 8B CE FF 90 FC 00 00 00 83 F8 01 74 1A`.
- ESI is the target (`8B CE` before the call; ESI is callee-saved).
- EAX = 1 takes the `je`, which returns and skips `DoDesperateAttack`. A hunter already returns 1, so this hook does not change the host. Animals that are not puppets stay on the old path.

**Site B, the sync-attack timer** (the `minsatime` "only for the player" gate). Same EAX = 1 substitution:

- Call at `0xcaecb`: `FF 92 FC 00 00 00`. Next at `0xcaed1`: `83 F8 01 0F 84 0D 01 00 00`.
- Verify at `0xcaec7`: `8B 13 8B CB FF 92 FC 00 00 00 83 F8 01 0F 84 0D 01 00 00`.
- EBX is the target.
- The timer reset is a second call in the same function, at `0xcb157`: `FF 90 FC 00 00 00`, next `0xcb15d` `83 F8 01 75 06 89 BE 9C 00 00 00`. Apply the same EAX = 1 there or a puppet that does get a sync attack never clears `+0x9C`.

`FUN_100c5bf0`'s attack-list walk already sees every entry combat registered. It does not need this hook.

What else uses `+0xFC` at these two functions: only these calls. `ProcessAttackState` has the one call. `FUN_100cadf0` has the gate, a second call at `0xcaf01` (`FF 90 FC 00 00 00`) that chooses the `"attack"` clip versus `"truck"`, and the timer reset. Forcing the `0xcaf01` call to 1 is harmless on foot: the truck branch also requires vfunc `+0x1C0` (the vehicle) to be non-null. Leave `FUN_10006ae0` / `FUN_10006c30` alone.

## Proposed change

Hook CommonDll `0x92798` (and, for the timer, `0xcaed1` and `0xcb15d`) as mid hooks. Bytes to verify are in question 4. If the target is a living puppet, set EAX to 1 so the existing `cmp eax, 1` treats it as the host hunter. Do not change `FUN_1001be20`, `FUN_10014940`, or the player getters.

## Proposed MEMORY facts

- GameDll `cScrAnimal` vtable `0xff470` slot `+0x234` = `FUN_1001be20` (`0x1be20`) calls `Move` (vfunc `+0xA4` = CommonDll `cObjScriptedEntity::Move` `0x3e700`) with `(-10000,-10000,-10000)`, sets brain `+0x79 = 1`, and calls `RemoveDeadAnimal`. Evidence: `re/notes/park-game-probe.txt`, IAT `0xfb188` = `Move`, `Move` decompile in `park-common.c`.
- `cScrAnimal` slot `+0x44` = `FUN_1001f3c0` (`0x1f3c0`) calls `+0x234` when `+0x590 == 1`. `FUN_10014940` (`0x14940`) is what sets `+0x590` for `obj_animal`, and GameDll `0x1890` stores that pointer at `cAIHuntAnimalInterface+8`. Evidence: `park-caller.c`, `park-flag.c`, bytes at `0x1890`.
- `cScrAnimal` slot `+0x58` = `FUN_1001e170` (`0x1e170`) calls `+0x234` during spawn, then restores `+0x19C`. Evidence: `park-unpark.c`.
- `cScrCharacterEx::MoveToPos` = GameDll `FUN_100161a0` (`0x161a0`): name via `FUN_10006d80`, then `Move` to that entity's `+0xE8`. Evidence: `park-unpark.c` and `re/notes/story-inventory.md`.
- CommonDll `AIHuntTask_AttackCats` vtable `0x136460`. State `+0x18 == 4` calls `FUN_100c8d60` (`0xc8d60`), which calls `interface+8`. State `0x800` is `FUN_100c5bf0`. `ProcessAttackState` (`0x92710`) returns early only when the target's vfunc `+0xFC` returns 1; the call is at `0x92792`. Evidence: `park-ca8e0.c`, `park-state800.c`, `park-common.c`, bytes at `0x9278e`.

## Open questions

- Which trigger parked lion 157: brain `+0x288` from a finished path, task state `4`, or state `0x1000` with `+0x17C == -1`. The flee log line was not in the kept `debug.log`.
- Whether `DoDesperateAttack` / `DoDesorient` (the non-player fallthrough) is what sets state `2` or `4`. Not decompiled.
- The metres value of the `FUN_100c5bf0` anchor radius, and what `FUN_100c59c0` writes at task `+0x180`.
- Which call clears brain `+0x79` after `MoveToPos`.
