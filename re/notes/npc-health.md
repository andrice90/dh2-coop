# NPC health (MAP2A Uncle Hugh / protect bar)

**Progress:** questions 1–4 written from GameDll slot V. Proposed fix written. Placed C++ class of `hugs2a` (cObjNPC vs cObjGuide) is still UNVERIFIED. No game launches.

Raw dumps: `re/notes/npch-*.c` and `re/notes/npch-*.txt`. Helper: `re/scripts/ScanDisp.java`.

## 1. The real health

Two different floats. The one that sticks is not `+0x19C`.

`cObjNPC` (factory `CreatecObjNPC` `FUN_10064070` RVA `0x64070`, ctor `FUN_10062720` RVA `0x62720`, size `0xc50`) stores a `cProfile` at entity `+0x1F8` (`malloc(0x11e8)`, `cProfile::cProfile`, vtable `PTR_FUN_101095f8`). `cObjGuide` (factory `FUN_100641a0` RVA `0x641a0`, ctor `FUN_10063ed0` RVA `0x63ed0`) calls that ctor and then swaps the vtable to `PTR_FUN_10108a50`. Both vtables use the same hurt and die slots (checked in the DLL):

| Slot | cObjNPC `0x108270` | cObjGuide `0x108a50` | Function |
|---|---|---|---|
| `+0x14C` | `0x5eeb0` | `0x5eeb0` | `cObjNPC::OnHurt` |
| `+0x154` | `0x5fde0` | `0x5fde0` | `cObjNPC::OnDie` |

Script `SetAttribute` is `cScrNPC` export `FUN_10059160` RVA `0x59160`. It multiplies the script integer by `0.001` (`_DAT_100fe594`) and calls `FUN_100577b0` RVA `0x577b0`. Attribute ids are the constants in `FUN_10063510`:

| Id | Name | Write |
|---|---|---|
| 1 | `ATTRIBUTE_HEALTH` | profile `+0x154` int max, and profile `+0x54` float current set to that same int |
| 2 | `ATTRIBUTE_HEALTH_CURRENT` | profile `+0x54` float |
| 3 | `ATTRIBUTE_HEALTH_RECOVER` | entity `+0x880` float (HP per second) |

Ctor calls case 1 with `100.0`, so max and current start at 100, and case 3 with `0.3`. `cScrHugs2a::Spawn` then sets `ATTRIBUTE_HEALTH` 100000 and `ATTRIBUTE_HEALTH_RECOVER` 1000, which are 100.0 and 1.0 after the `0.001` scale. `ATTRIBUTE_HEALTH_CURRENT` is commented out in `GUIDE.SCR`. The host log value 50 is not this default.

`cObjNPC::Update` is `FUN_100630a0` RVA `0x630a0`. Every call it reaches `cObjNPC::DecideCurentCondition` (`FUN_10062ac0` RVA `0x62ac0`), whose first health line is:

`entity+0x19C = profile+0x54`

That is the rewrite. `FuncInflictDamage` and `SetHealth` only store `+0x19C`, so the next `Update` puts the profile float back. `SetHealth` (`cScrCharacterEx`, `FUN_10015ff0` RVA `0x15ff0`) writes `entity+0x19C`, optionally times difficulty `FUN_1000a680(3, ...)`. MAP2A's `SetHealth(600)` is `wolf3`, not Hugh. Wolf `SetHealth(50.0)` in `GWOLF.SCR` is the wolf's own `+0x19C`.

While current `> 0`, the same function adds `dt * entity+0x880` onto profile `+0x54` and clamps to `(float)profile+0x154`. It skips that add only when the brain at `+0x200` is set and the task at entity `+0xc48` matches one specific state (`task+0x1a4 == 4` and a follow-up brain flag). If entity `+0xc24 != 0`, current is forced to max.

It also writes a band to entity `+0xb58` from current versus max: thresholds `0.53`, `0.50`, and `0.05` (`0x109648`, `0xfc650`, `0xfee44`), then `<= 0`. The `<= 0` arm sets `+0xb58 = 0x10` and calls virtual `+0x154` (`OnDie`). The other bands are 8 / 4 / 2 / 1. The compare direction above 0 is the decompiler's float idiom; the zero arm is the one that calls `OnDie`.

## 2. The bar

`cHUDNpcStatus::Update` RVA `0xdf6c0` and `cHUDNpcStatus::Draw` RVA `0xdf3b0`.

The entity is the pointer at HUD `+0x68`. Both functions read `*(profile+0x54) / (float)*(profile+0x154)`. Update multiplies that ratio by `100.0` (`0x100100`) and writes the bar rectangle width from it. Draw packs the ratio through `FUN_100c5e20` RVA `0xc5e20` into the sprite color and draws the portrait with the UVs at entity `+0x838..+0x844` (`SetIconCoords`, `FUN_10059000`).

`cHUDObjective::Draw` (`0xd8510`) is the objective text, not this bar. `cHUDBossHealth::SetLevel` (`0xcac90`) is the animal bar (`+0x19C / +0x4c0` in `FUN_10013d80`).

`AddAIToBeProtected` is `cMODApp::AddAIToBeProtected` `FUN_100077c0` RVA `0x77c0`: name lookup `FUN_10006d80` RVA `0x6d80` (`stricmp` of entity `+0x198`), then `cGameRules::AddAiToBeProtected` `FUN_1008f8a0` RVA `0x8f8a0` on global `DAT_10165808`. That pushes the entity onto the list object at `cGameRules+0x50` (virtual `+4`). `RemoveAIToBeProtected` is `FUN_100078f0` / `cGameRules::RemoveAIToBeProtected` `FUN_10090f70` RVA `0x90f70`, which scans that list and removes the matching pointer. Which instruction stores the list entry into `cHUDNpcStatus+0x68` was not found. UNVERIFIED that the bar's entity is that list entry, though both exist only for the protect objective.

## 3. What damages it

The subtract that survives the next frame is profile `+0x54`.

**`cObjNPC::OnHurt`** `FUN_1005eeb0` RVA `0x5eeb0` (vtable `+0x14C`). Returns immediately if entity `+0x6cc != 0`. Otherwise:

- If `DAT_101657f0 != 0`, multiplies damage by `FUN_1000a680(2, DAT_10186ec0)` unless that factor is 0.
- If the attacker `IsClassKindOf("cObjAnimal")` and the attacker pointer equals `*(cGameRules+0xe8)`, doubles damage.
- Subtracts from profile `+0x54`. If the result is `<= 0`, stores 0 and calls `OnDie`.

Hurt type `0x40` still applies that subtract; it only skips the impulse and the hurt-task start. Callers are through the virtual slot (Ghidra listed no direct caller). The attack-task timed hit `FUN_100c2ee0` (CommonDll `0xc2ee0`) calls the target's `+0x14C` (already mapped in `re/notes/knockdown.md`). Grey-wolf sync clips in `GWOLF.SCR` set `sadmgval` 10 and 15. MAP2A also starts `AIHuntTask_AttackDogs` with `target` `hugs2a`. Whether that task damages only by the sync-anim clock or also inside the brain tick was not decompiled. UNVERIFIED.

**Script, MAP2A only** (identifiers, not bodies):

- `cScrHugs2a::Spawn`: `ATTRIBUTE_HEALTH` 100000, `ATTRIBUTE_HEALTH_RECOVER` 1000. No `SetHealth`, no `ATTRIBUTE_HEALTH_CURRENT`.
- `TRIGGERMISSION02A.SCR`: `AddAIToBeProtected("hugs2a")` then later `RemoveAIToBeProtected("hugs2a")`. Wolves get `SetTaskParam("target", "hugs2a")`. `SetHealth(600)` is `wolf3`.
- `CUTSCENE4.SCR`: `SetAttribute(ATTRIBUTE_HEALTH_CURRENT, 10000)` and `ATTRIBUTE_HEALTH_RECOVER` 100. That is current `10.0` and recover `0.1`, written straight to profile `+0x54` and entity `+0x880`.

**Does not move the bar.** `FuncInflictDamage` (CommonDll `0x9a540`) and `cObjScriptedEntity::OnHurt` (CommonDll `0x3e290`) write `+0x19C` only. `DecideCurentCondition` replaces that from profile `+0x54` on the next `cObjNPC::Update`. That matches the host log: `+0x19C` was 50, the mod's hit took it to 0, and the next frame it was 50 again. 50 was profile `+0x54` at that moment, not a second default.

**Client, brain frozen.** `cObjNPC::Update` does not test the brain-freeze flag. It still calls `DecideCurentCondition`, so the copy and the recover add still run. `OnHurt` is not inside the brain update, so a local bullet or a sync-anim damage tick (`FUN_100c2ee0` from `ProcessPlaySA`) still subtracts profile `+0x54` on that client. Animal `FreezeBrain` skips the brain tick (MEMORY); it does not skip `ProcessPlaySA`. Whether a frozen client wolf still crosses `sadmgtime` is UNVERIFIED. Client scripts still run, including the cutscene `SetAttribute`.

## 4. What makes him dead

Same field as the bar. `DecideCurentCondition` calls `OnDie` when profile `+0x54 <= 0`. `OnHurt` does the same when the subtract crosses 0.

`cObjNPC::OnDie` `FUN_1005fde0` RVA `0x5fde0`, once, while `+0x6cc == 0`:

- Sets `entity+0xC1 |= 4` and `entity+0x6CC = 1` (further `OnHurt` / `OnDie` no-op).
- If `GetClassType` equals `"cObjGuide"` (10-byte compare including the NUL), writes stream `DAT_10185ea8` byte `0x87`, int8 `6`, then a text buffer. The buffer is filled from the literal `MISSION FAILED!` and then overwritten with the C string at entity `+0x848` plus the literal ` hunt has ended.` Code 6 is the fail code. This write does not call `cMission`'s message function `0x91c20`, so a hook on `0x91c20` does not see it.
- If the class string is not `cObjGuide`, the dead bits are still set and the stream write is skipped.

`cObjGuide` uses this `OnDie`. `cObjNPC` uses it too, but the string compare fails, so a plain `cObjNPC` death does not emit code 6. Which class the map places for `hugs2a` was not in the extracted scripts (script type is `cScrNPC` / `cScrHugs2a`; both C++ ctors end up with script class name `cScr_Guide`). UNVERIFIED.

`FUN_10057bd0` is not a protected-AI test. It checks a model pointer. Script `IsProtected` (`FUN_10016180`) stores `1` at entity `+0x604` and does not read health. Hugh's spawn does not call it.

## Proposed fix

Snapshot, on the host, the float at `*(npc+0x1F8)+0x54`, not `+0x19C`. Max, if a ratio is needed, is the int at `profile+0x154`. After `cObjNPC::Update`, `+0x19C` equals that float, but only until the next `FuncInflictDamage`.

On the client, each frame write that host float to `profile+0x54` after `cObjNPC::Update` / `DecideCurentCondition`. The next `Update` copies it into `+0x19C`. The bar (`cHUDNpcStatus`) and the `<= 0` death check both read `profile+0x54`.

Block the client's `cObjNPC::OnHurt` (`0x5eeb0`) for mirrored NPCs so local bullets and `FUN_100c2ee0` do not move the profile between writes. Leaving recover (`+0x880`) alone is fine if the host float is written after `DecideCurentCondition`, because the client's add is overwritten. Do not block `SetAttribute`: the host runs the same cutscene write, and the snapshot then carries it.

When the host float is `<= 0`, write 0 and let the client's `OnDie` run. If the entity's class is `cObjGuide`, that posts code 6 locally. The host posts the same way, and `0x91c20` is not on that path, so `kMissionEnd` from the mission hook will not carry this fail by itself.

## Proposed MEMORY facts

- GameDll `cObjNPC::DecideCurentCondition` RVA `0x62ac0`, called from `cObjNPC::Update` RVA `0x630a0`, copies `*(entity+0x1F8)+0x54` onto `entity+0x19C` every call. Evidence: `re/notes/npch-19c.c`, `re/notes/npch-hurt.c`.
- That profile float is NPC current health. Max is the int at profile `+0x154`. `SetAttribute` RVA `0x59160` scales the script int by `0.001` (`0xfe594`) and `FUN_100577b0` RVA `0x577b0` stores ids 1, 2, and 3 as max+current, current, and entity `+0x880`. Evidence: `re/notes/npch-gamedll.c`, `re/notes/npch-protect.c`, `re/notes/story-natives-gamedll.c` attribute constants.
- `cObjNPC::OnHurt` RVA `0x5eeb0` (vtable `+0x14C` on both `0x108270` and `0x108a50`) subtracts from profile `+0x54` and calls `OnDie` at `<= 0`. `cObjNPC::OnDie` RVA `0x5fde0` sets `+0xC1 |= 4` and `+0x6CC = 1`, and if `GetClassType` is `cObjGuide` writes message code 6 to `DAT_10185ea8` without calling `0x91c20`. Evidence: `re/notes/npch-hurt.c` and the vtable dwords at those RVAs.
- `cHUDNpcStatus::Update` RVA `0xdf6c0` and `Draw` RVA `0xdf3b0` size the protect bar from `profile+0x54 / (float)profile+0x154` on the entity at HUD `+0x68`. Evidence: `re/notes/npch-npcstatus.c`, `re/notes/npch-portrait.txt`.
- `AddAIToBeProtected` RVA `0x77c0` resolves the entity name and calls `cGameRules::AddAiToBeProtected` RVA `0x8f8a0` on `DAT_10165808`, which inserts into the list at `cGameRules+0x50`. Remove is RVA `0x78f0` / `0x90f70`. Evidence: `re/notes/npch-gamedll.c`, `re/notes/npch-protect.c`, profiler strings in `re/notes/npch-probe.txt`.
