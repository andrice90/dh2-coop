# Snakes (cObjSnake / cObjCobra) — mirror notes

**Progress:** done. Slot W, GameDll7 only (CommonDll7 and EngineDll7 have no snake/cobra strings). No game launches. Dumps: `re/notes/snake-probe.txt`, `snake-vt.txt`, `snake-vt2.txt`, `snake-funcs.c`, `snake-funcs2.c`, `snake-death.c`.

Two classes. Mission cobras (`AddAnimal(ANIMAL_COBRA)`) are `cObjCobra`. Ambient `AddEvent(EVENT_SNAKE)` props are `cObjSnake`. They do not share a vtable or a layout. Only `cObjCobra` is a `cScrAnimal` descendant.

## 1. Classes and vtables

All of these are GameDll7. CommonDll7 has no `cObjSnake` / `cObjCobra` vtable.

| Class | Kind string | Vtable RVA | Alloc | Base (ctor) | Script class set in the ctor |
|---|---|---|---|---|---|
| `cScrAnimal` | (existing) | `0xff470` | — | `cObjCharacterEx` | `cScrAnimal` |
| `cObjCobra` | `obj_cobra` | `0x10e920` | `0x73c` | `cScrAnimal` (`FUN_10018c80`) then its own vtable | `cScrCobra` |
| `cObjSnake` | `obj_snake` | `0x10e470` | `0x224` | `cObjScriptedEntity` only | `cScrEntSnake` |
| `cScrSnake` (script object, not the entity) | — | `0x10e788` | `0x30` | `cScrEntity` | — |

Evidence: ctors `FUN_1007f010` (`0x7f010`), `FUN_10018c80` (`0x18c80`, writes `PTR_FUN_100ff470` and `"cScrAnimal"`), `FUN_1007d7f0` (`0x7d7f0`, writes `PTR_FUN_1010e470`). Factories `CreatecObjCobra` `FUN_10080d80` (`0x80d80`), `CreatecObjSnake` `FUN_10080b20` (`0x80b20`), `cScrSnake::Create` `FUN_10080c50` (`0x80c50`).

Script files (identifiers only): `cCobra` extends `cAnimalBrain` extends `cScrAnimal` (`COBRAENT.SCR`, `ANIMAL.SCR`). `cScrEntSnake` extends `cScrSnake` (`SNAKEENT.SCR`). The C++ entity script name for a cobra is `cScrCobra`; the brain script class is `cCobra`.

### What the mirror's calls hit

`cObjCobra` keeps the `cScrAnimal` / `cObjCharacterEx` prefix, including the brain pointer at `+0x200`. Its ctor does not write `+0x200`. It adds floats at `+0x71c/+0x720/+0x724`, the hunter pointer at `+0x728`, a turn timer at `+0x734`, and a death/attack flag at `+0x738`.

Slots that differ from `cScrAnimal` (`0xff470`), same index:

| Slot | `cObjCobra` | `cScrAnimal` | Role |
|---|---|---|---|
| `+0x44` | `FUN_10081620` `0x81620` | `FUN_1001f3c0` `0x1f3c0` | Entity `Update`. Cobra does not call the animal update |
| `+0x58` | `FUN_1007f530` `0x7f530` | `FUN_1001e170` `0x1e170` | `Spawn`. Cobra calls `0x1e170`, then body state `Rise`, then stores `"hunter"` |
| `+0x8c` | `FUN_10080440` `0x80440` | `FUN_1001a680` `0x1a680` | `OnHit` |
| `+0x154` | `FUN_10080320` `0x80320` | `FUN_1001e8b0` `0x1e8b0` | Death vfunc the mirror calls as `PresentDeath` |
| `+0x14c` | `0x194b0` | `0x194b0` | Same hurt slot as `cScrAnimal` |
| `+0x23c` | `FUN_1007f780` `0x7f780` (`Die`) | past the animal vtable | Cobra-only |

`FreezeBrain` on `*(entity+0x200)` matches the animal layout: cobra spawn goes through `cObjCharacterEx::Spawn` inside `FUN_1001e170`, and `FUN_10012cc0` calls `*(brain+0)` slot `+0x84`. The brain object is the one `cScrAnimal` spawn installs. This job did not read a live brain vtable, so the class name `cAIHuntBrain_Animal` is the animal path's class, not a second type found on the cobra.

`cObjSnake` is not that layout:

- `+0x1fc/+0x200/+0x204` are set to float `1.0` (`0x3f800000`) in the ctor. `FreezeBrain` would treat `+0x200` as a brain pointer.
- Health `+0x19c` is not a field this class uses. `PresentDeath` slot `+0x154` is `cObjSnake::OnDie` `FUN_10081370` (`0x81370`), not `FUN_1001e8b0` and not `FUN_10080320`.
- Hurt slot `+0x14c` is `LAB_100ec9e0`, not `0x194b0`.
- No `cObjAnimal::ClaimCallback` path. `ClaimCallback` is not among the cobra slot overrides either; cobra death does not call it.

`cObjAnimal::ClaimCallback` (`0x1cd40`) is the hunt-end callback. Do not call it for either class.

## 2. Placement

`ANIMAL_COBRA` is script constant `0x13` (19). `SCRIPTER.EXP` stores that dword immediately after the 12-byte name. `AddAnimal` is GameDll `FUN_1008d970` (`0x8d970`):

1. `FUN_1008d000` indexes the string table at RVA `0x163dd8`. Index `0x13` is `"obj_cobra"`. Every other story species in that table is `"obj_animal"` except index `0x1f` (`"obj_crocodile"`).
2. `cObjManager::CreateEntity("obj_cobra")`.
3. Writes the species id to entity `+0x434`, sets `+0x4a8 = 1`, calls vtable `+0x15c` with the script name from the table at `0x163d30` (index `0x13` is `"cCobra"`), then precache, spawn, and `+0xec` with the name argument (`cobra1`, …).

So a mission cobra is created when the level script runs `AddAnimal`, then named. It is not a `cScrAnimal` BSP record. MAP3B's `AddAnimalOnMap` order is leopard, leopard, cobra, cobra, cobra; MAP3C adds four cobras; MAP7 adds one (`WORLD.SCR`). Those are the same native that already creates the leopards the mirror syncs by entity id. Cross-process cobra ids were not logged in this job (no launch). UNVERIFIED that a cobra id matches until a run shows it; the allocator is the same `CreateEntity` sequence.

`EVENT_SNAKE` is script constant `2`. `FUN_10082f30` case 2 creates one `"obj_snake"` and names it `sprintf("%s%d", name, 1)` (`snake` → `snake1`). That is the ambient prop. It is not `ANIMAL_COBRA` and it is not in the species table.

## 3. Brain and attack

`Task_CobraAttack` is not a string in GameDll7, CommonDll7, or EngineDll7, and no script class defines it. It appears only as the argument of `StartTask` / `SetTaskParam` in `TRIGGERMISSION03B.SCR` and `TRIGGERMISSION07.SCR` (and the packed `MAIN.XSC` for MAP3B and MAP7). MAP3C's XSC has no `Task_CobraAttack`.

The bite does not use `cAIHuntTask_AttackBase::SetParam` (`0xbc3d0`) or `DAT_101aea7c`.

`cObjCobra::Spawn` (`0x7f530`), after animal spawn:

- `FUN_10012cc0(this, "Rise")` — body-state helper, not the animal brain.
- If `DAT_101657f0 != 0`, `FUN_10006d80("hunter")` is stored at entity `+0x728`. That is the GameDll name lookup, once, at spawn. It is not the attack-list test.

`cObjCobra::Update` (`0x81620`) returns immediately when `+0x728` is null. Otherwise:

- Calls `FUN_1001f3c0` (the animal physics update that `cScrAnimal` uses as its own `+0x44`).
- If `+0x738 > 0`, subtracts `dt * 20` (float at RVA `0x106b40`). Crossing to `<= 0` calls vtable `+0x154` (`FUN_10080320`).
- If `+0x738` still has the ctor bit pattern `0xbf800000` (float `-1`), and the hunter is within squared distance `25` (float at `0xfc648`, so 5 world units) and the body-state name begins with `Idle` (`0x656c6449` from `IsInIdle` `0x7ff40`), it sets body state `Attack`. `LoockToTarget` (`0x80080`) sets `TurnLeft` / `TurnRight` when the yaw error passes ±0.5236 rad (±30°).

`FreezeBrain` stops `cAIHuntBrain_Animal::Update`. It does not stop `cObjCobra::Update`. A client cobra with `+0x728` still pointing at that process's `"hunter"` keeps biting and can run the death timer by itself.

`OnHit` (`0x80440`) accepts only `mod_obj_bullet`. A bone ray (`model +0x10c`) returning 1 sets `+0x738 = 50.0` (`0x42480000`) and body state `Death`. It does not subtract `+0x19c` and does not call `+0x154`. The 50-unit timer at 20 per second is 2.5 s, then `FUN_10080320`. Health is only read here to bump the trophy counters at `+0x4c4/+0x4c8` when it is already `<= 0`.

The bite itself is the body script, after the attack clip: `cScrSnake::HurtHuman` (`0x7ed00`). For `obj_cobra` it reads the target at `+0x728` and compares horizontal distance (target `+0xe8/+0xf0` vs the cobra) to the double at RVA `0x10e8f0` (`1.75`). For `obj_snake` the target is `+0x208` and the double at `0x10e8f8` is `1.5`. On a hit it calls the **target's** vtable `+0x14c` (hunter hurt) with the script float, damage type `3`, and two null vectors. `cBodyRSnake` passes `0.0`. Whether a 0-damage hunter hurt still staggers is UNVERIFIED.

No cobra or snake function in these dumps calls `PlaySA`. The bite is `SetAnim("attack1")` inside the body state `Attack`, not a sync attack.

`cObjSnake::SetTarget` (`0x7e4d0`) stores an `obj_hunter` at `+0x208`. Its `Update` (`0x7e060`) only calls `cObjScriptedEntity::Update`. The ambient sequence is the body script, starting from spawn's `Inactive`.

## 4. Death and kill count

`SetTaskAnimalToShoot` records the species on the task. The decrement is `FUN_10091e80` (`0x91e80`): when `mission+0x1f0 != 0`, the species equals `mission+0x1e8`, and the action is `1`, it decrements `mission+0x1e4`. If that count is already 0 it reports mission failed. It then stores the species at `mission+0x1e0`.

`cScrAnimal` present-death `FUN_1001e8b0` calls `FUN_10091e80(mission, +0x420, +0x434)`, sets body state `Dead`, sets `+0x19c = 0`, and sets `+0x600`. Cobra does not use that function.

`cObjCobra` slot `+0x154` is `FUN_10080320`:

- `+0x4ac = 1`
- `+0x19c = 0`
- `FUN_1001ba60`
- `FUN_10091e80(DAT_101869e8, 1, entity+0x434)` — action is the constant `1`, species is the `AddAnimal` id (`0x13` for a cobra)

It does not set a body state, does not set `+0x600`, and does not return early if it already ran. A second call on the same mission, once the remaining count is 0, takes the failure branch in `FUN_10091e80`.

`entity::PresentDeath` calls slot `+0x154`, so on a cobra it already reaches `FUN_10080320` and the mission counter. It must run once per process.

The host snapshot commits `kAnimalDied` when `+0x19c <= 0`. That becomes true inside `FUN_10080320`, so the existing health check still fires on the next host tick, after the host has already counted. The client apply calls `PresentDeath` once when the sample health is `<= 0`, which is the client's one `FUN_10080320`. That part of `kAnimalDied` / the health snapshot covers a cobra **if** the client does not also run `cObjCobra::Update` through the timer.

`AnimalHit` does not cover the bullet. The host applies `FuncInflictDamage` (subtracts `+0x19c`) and, at `<= 0`, calls `PresentDeath`. A real bullet never does that: `OnHit` starts the 2.5 s `Death` timer and `Update` calls `+0x154` once. A lethal `AnimalHit` counts immediately, skips `Death`, and a later timer call counts again. Non-lethal `AnimalHit` only changes health, which `OnHit` does not use to decide the kill.

Ambient `cObjSnake::OnDie` does not call `FUN_10091e80`. Killing it does not advance `ANIMAL_COBRA`.

## 5. Body-state names

`cBodyRSnake` (`COBRA.SCR`), the cobra body. C++ uses the starred names:

| State | Clip / note |
|---|---|
| `Rise` | `rise`. Spawn sets this |
| `Idle` | `idle1`. `IsInIdle` tests this prefix before `Attack` |
| `Attack` | `attack1`, then `HurtHuman(0.0)`, then `Idle` |
| `TurnRight` | `turnright30` |
| `TurnLeft` | `turnleft30` |
| `Death` | `death`. `OnHit` sets this. `FUN_10080320` does not |
| `NoChange` | empty |

`cBodySnake` (`SNAKE.SCR`), the ambient prop. Spawn sets `Inactive`.

| State | Note |
|---|---|
| `Activate` | goes to `Rise` |
| `Rise` | `rise`, then `Attack1` |
| `Attack1` | `attack1`, `HurtHuman(0.0)`, then `Idle` |
| `Idle` | `idle1`, then `Lower` |
| `Lower` | `lower`, then `Die` |
| `Die` | `UnlinkWithPhysics`, then `Destroy` |
| `Nothing`, `Inactive` | empty. Spawn sets `Inactive` |

`cScrAnimal` present-death's name `Dead` is not a cobra state. Copying the host name `Death` is what plays the fall. `SetBodyState` in the mirror writes the state machine directly, so it does not need `FUN_10012cc0`.

## Proposed change

Tracker: treat GameDll7 vtable `+0x10e920` (`cObjCobra`) as an animal, next to `cScrAnimal` `+0xff470`. Do not add `cObjSnake` `+0x10e470`. One test, not a class-name string:

- `cObjCobra` — include
- `cObjSnake` — exclude (`+0x200` is a float; `FreezeBrain` and `+0x19c` death do not apply)

Mirror, for `cObjCobra` only:

1. `FreezeBrain` on `+0x200` is the right call (same slot as the other animals). It is not sufficient.
2. On a client, after spawn, clear `+0x728`. `Update` then returns before the bite, the `FUN_1001f3c0` call, and the death timer. Leaving the local `"hunter"` there makes a second cobra. Position and yaw stay on the existing `HandleKey` path.
3. Copy body-state names from section 5 (`Rise`, `Idle`, `Attack`, `TurnLeft`, `TurnRight`, `Death`). Do not write `Dead`.
4. Keep the host health snapshot. `FUN_10080320` stores `0` at `+0x19c`, so `kAnimalDied` and the client's one `PresentDeath` still count `ANIMAL_COBRA` via `+0x434`. Call that vfunc once per process. Do not also call `FUN_1001e8b0` or `ClaimCallback`.
5. `AnimalHit` on a cobra must not take the current "subtract health, `PresentDeath` at 0" path. Match `OnHit`: set body state `Death` and `+0x738 = 50.0`, and let the host `Update` call `+0x154` once. A second `PresentDeath` on that same mission can fail it when the remaining count is 0.
6. The existing attack-list retarget (`DAT_101aea7c`) does not move a cobra. The bite target is `+0x728`, set once to the entity named `"hunter"`. Sharing the body does not by itself make it bite every player. Pointing `+0x728` at another hunter is a separate change.

Ambient `cObjSnake` is a different feature: no brain freeze, body names from `cBodySnake`, and no mission counter. Leave it out of `Animals()`.

## Proposed MEMORY facts

- `cObjCobra` vtable GameDll7 RVA `0x10e920`. Ctor `FUN_1007f010` (`0x7f010`) calls `cScrAnimal` ctor `FUN_10018c80` (`0x18c80`, vtable `0xff470`) and then `SetScriptClassName("cScrCobra")`. Allocation `0x73c`. Kind string `obj_cobra`. Evidence: `re/notes/snake-funcs2.c`, `re/notes/snake-vt.txt`.
- `cObjSnake` vtable GameDll7 RVA `0x10e470`. Ctor `FUN_1007d7f0` (`0x7d7f0`) calls `cObjScriptedEntity::cObjScriptedEntity` only, writes float `1.0` at `+0x200`, and `SetScriptClassName("cScrEntSnake")`. Allocation `0x224`. Kind string `obj_snake`. Evidence: `re/notes/snake-funcs.c`, `re/notes/snake-vt2.txt`.
- `AddAnimal` `FUN_1008d970` (`0x8d970`) maps species `0x13` (`ANIMAL_COBRA` in `SCRIPTER.EXP`) through the table at `0x163dd8` to `"obj_cobra"` and the table at `0x163d30` to `"cCobra"`, and stores the species at entity `+0x434`. Evidence: `re/notes/snake-funcs.c`, `re/notes/snake-funcs2.c`.
- `cObjCobra` slot `+0x154` is `FUN_10080320` (`0x80320`): sets `+0x19c = 0` and calls `FUN_10091e80(mission, 1, +0x434)`. It is not `cScrAnimal` present-death `FUN_1001e8b0`. Evidence: `re/notes/snake-death.c`, `re/notes/snake-vt.txt`.
- `cObjCobra::Spawn` (`0x7f530`) stores `FUN_10006d80("hunter")` at `+0x728`. `Update` (`0x81620`) bites that pointer when `+0x738` is still `-1` and distance² `< 25`. It does not call `cAIHuntTask_AttackBase::SetParam`. Evidence: `re/notes/snake-funcs.c`.
- `cObjCobra::OnHit` (`0x80440`) on a connecting bullet sets `+0x738 = 50.0` and body state `Death`. `Update` then calls `+0x154` when that timer crosses 0. Evidence: `re/notes/snake-funcs.c`.

## Open questions

- Cross-process entity ids for cobras were not logged (no launch). The create path is the same `AddAnimal` as the leopards.
- `StartTask("Task_CobraAttack")` has no class in the binaries or the script sources. UNVERIFIED whether that call no-ops. The C++ bite does not read it.
- `HurtHuman`'s hunter call passes the script float `0.0` and type `3`. UNVERIFIED whether the hunter hurt vfunc treats 0 as a real hit.
- Clearing client `+0x728` also skips `FUN_1001f3c0` inside cobra `Update`. UNVERIFIED whether `HandleKey` alone still draws the cobra the way it does for `cScrAnimal`.
- Default `+0x19c` at cobra spawn was not found as a literal. `OnHit` does not use it to decide death.
