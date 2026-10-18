# Player-count difficulty (research, 2026-10-17)

How to scale story fights for 2, 3, and 4 players. This is the method to build. It is not in the mod yet.

Two rules, from the request:

| Players | Extra players | Boss / special health | Extra bodies in a scripted attack |
|---|---|---|---|
| 1 (solo, or co-op not connected) | 0 | unchanged | 0 |
| 2 | 1 | ×1.25 | +1 |
| 3 | 2 | ×1.50 | +2 |
| 4 | 3 | ×1.75 | +3 |

`extra = connected - 1`. Health factor is `1 + 0.25 * extra`. MAP2A's opening is wolf1 and wolf2, so the pack becomes 3, 4, or 5 wolves.

Connected players are `1 + g_remotes.size()` in `mod/src/coop/coop.cpp`. The host's map holds clients. A client's map holds the host (player 0) and the other clients. Solo play leaves the map empty, so the factor stays 1.

The host decides. Clients already copy animal position and `+0x19C` from the host snapshot (`mod/src/coop/animals.cpp`).

## What the game actually does

Story animals are not spawned when the ambush starts. `WORLD.SCR` `AddAnimalOnMap` creates every one at load. MAP2A creates `wolf1`, `wolf2`, `wolf3`, `mlion1`, `mlion2`, and `gbear` (`re/extracted/SCRIPTS/LEVELS/MAP2A/WORLD.SCR`). There is no `SpawnEntity` on these maps (`re/notes/audit-maps-a.md`, F16). `wolf3` is the later fight, not a spare for the opening.

`AddAnimal` is GameDll `FUN_1008d970` (RVA `0x8d970`). It `CreateEntity`s the species class, stores the species at entity `+0x434`, calls spawn, and names the entity. Spawn (`cScrAnimal` vfunc `+0x58` = `FUN_1001e170`, RVA `0x1e170`) copies current health into max health and parks the body:

- `+0x4C0` = `+0x19C` (max / spawn health)
- then vfunc `+0x234` parks at `(-10000, -10000, -10000)` with the brain frozen (`brain +0x79 = 1`)
- then `+0x19C` is restored from `+0x4C0`

`MoveToPos` on an animal (`FUN_100161a0` → vfunc `+0x158` = `FUN_10014610`) is the unpark. It sets `+0x19C = +0x4C0`, clears the dead bits, sets body state `IdleAttack`, sets active `+0x4B0 = 1`, and clears brain `+0x79`. A later `MoveToPos` also revives a dead animal back to max health. Evidence: `re/notes/audit-maps-a.md` F3, `re/notes/objmodel-decomp-game-animal.c`.

The opening ambush is a trigger, not a spawner. `cScrTask02` in `TRIGGERMISSION02A.SCR`:

- `OnActivate`: `wolf1.StartTask("AIHuntTask_AttackDogs")` target `"hunter"`, then `wolf2.MoveToPos("wolfs_t2_02")` and the same task with target `"hugs2a"`.
- `BeginCutScene`: `wolf1.MoveToPos("wolfs_t2_01")`, then the cutscene.

`BeginCutScene` is not inside `OnActivate`. Fire (`FUN_10068130`, RVA `0x68130`) runs the script function stored at trigger `+0x20C` (`OnActivate`). The update (`FUN_10068800`) runs `+0x210` (`BeginCutScene`) later, when trigger state `+0x1FC` is 4. Wolf1 is placed in that later call. Wolf2 is placed in `OnActivate`.

`StartTask("AIHuntTask_Attack…")` is the scripted-attack signal. Counts across the extracted scripts: AttackDogs 94, AttackCats 88, AttackChargers 84, AttackChasers 17, AttackFloaters 16, AttackBears 14, AttackTraps 4, AttackThrowers 3, AttackObjectMovers 2. `IdleWait`, `PlaySA`, `GoToEntity`, `HeadLook`, and `PlayAnim` are not attacks.

Grey wolf `SetHealth(50)` is in `GWOLF.SCR`. Yeti is 2500, hogzilla 1500, big grim 1500. Those three are the only `MakeBoss(TRUE, …)` calls (MAP11B, MAP12B, MAP12C).

### Retail difficulty is a different knob

`cMapWorldBase::SetDifficulty` (`MAPWORLDBASE.SCR`) calls `SetDiffProps(HEALTH_ANIMAL, 10, 11, 12)` for easy, medium, hard. `HEALTH_ANIMAL` is constant 3.

`SetDiffProps` is GameDll `FUN_1000a530` (RVA `0xa530`). It stores `integer * DAT_100fc46c`. That constant is `0.1` (`0x3DCCCCCD` at GameDll RVA `0xFC46C`). The three health factors are **1.0, 1.1, and 1.2**.

`SetHealth` (`FUN_10015ff0`, RVA `0x15ff0`) multiplies its argument by `FUN_1000a680(3, DAT_10186ec0)` and writes **only** `scriptObject + 0x28` → entity `+0x19C`. It does not write `+0x4C0`. Index 0/1/2 picks the easy/medium/hard slot. Any other index returns `1.0` (`DAT_100fcf1c`).

That table applies to every `SetHealth`, including the 50-point wolves. It cannot add a body. Player scaling should leave it alone.

Decompile: `re/notes/scale-decomp.c`.

### Boss bar and the max-health field

`MakeBoss` is `FUN_10016d20` (RVA `0x16d20`). It passes the entity at script `+0x28` to `FUN_10012380`, which sets **entity `+0x3C0`** to 1 or 0 and shows HUD item `0x10` (`cHUDBossHealth`). The fill is `+0x19C / +0x4C0` (`FUN_10013d80`, `re/notes/npc-health.md`). Scaling current health without scaling `+0x4C0` fills the bar past full, and the next `MoveToPos` copies the old max back over the scaled value. Every boss trigger does that `MoveToPos` in `BeginCutScene` (yeti, hogzilla, big grim).

The animal snapshot sends `health` as a float (`mod/src/net/protocol.h` `AnimalState`). The client writes that into `+0x19C` and does not write `+0x4C0`. Both processes have to apply the same factor to `+0x4C0`, or the client bar uses the solo max.

Death reports the species into the mission. Present-death `FUN_1001e8b0` (RVA `0x1e8b0`) calls `FUN_10091e80(mission, animal+0x420, animal+0x434)` when `+0x500 != 2`. `+0x434` is the `AddAnimal` species. `SetTaskAnimalToShoot` (`FUN_10092780`) stores species at task `+0xF0`, mode at `+0xFC` (`ANIMAL_KILL` is 2), and the count in both `+0xF4` and `+0xF8`. MAP2A task 5 is `SetTaskAnimalToShoot(ANIMAL_GWOLF, ANIMAL_KILL, 3)`. The function that decrements that count was not isolated. The death report itself is verified (`re/notes/lead-mission.md`).

The mirror lists every `cScrAnimal` (vtable GameDll `0xFF470`) and mission cobra (`0x10E920`) by walking the entity manager (`player_tracker.cpp`). A new animal created with `AddAnimal` on both sides is mirrored with no tracker change. `cObjCrocodile` / water crocodile is not in that list.

The snapshot holds at most `kMaxAnimals` (32), and that 32 includes NPCs (`animals.cpp`). Parked reserves still occupy a slot (`y < -1000` is only a flag). Review L2: a large unreliable animal packet can be sent as reliable ENet fragments (`re/notes/review-2026-10-17.md`).

## Method

One new module, host-authoritative, same `AddAnimal` calls on every peer. Prologue-check every hook the way `events.cpp` already does.

### 1. Boss and special health

**Who.** An animal whose entity `+0x3C0` becomes 1. That is `MakeBoss(TRUE)`. Today that is the yeti, hogzilla, and big grim. Keep a short name list next to the hook (empty by default) for a later special that never calls `MakeBoss`. Anything on that list uses this health path and is skipped by the body path.

A grey wolf, including MAP2A `wolf3`'s later `SetHealth(600)`, stays on the body path. `SetHealth(600)` writes `+0x19C` only. `Make_Scared` sets 10000 on ordinary animals (`re/notes/assets.md`). A health threshold would mark those as bosses.

**When.** Hook `MakeBoss` `0x16d20`. On entry, read `+0x4C0` as the base max if this entity has no stored base yet. Set

```
max   = baseMax * (1 + 0.25 * extra)
+0x4C0 = max
+0x19C = +0x19C * (max / oldMax)    // keeps the current ratio
```

Do this on every peer, from the same `extra`. The host's `+0x19C` is what the snapshot already broadcasts. The client's `+0x4C0` has to move too, because the bar divides locally.

`MoveToPos` / `0x14610` then restores `+0x19C` from the scaled `+0x4C0`. The cutscene respawn keeps the bonus.

If `SetHealth` runs on that entity after `MakeBoss`, hook `0x15ff0` and, when `+0x3C0` is set or the name is on the list, write the scaled value into both `+0x19C` and `+0x4C0`. Species `InitModel` runs inside `0x1e170`, before `MakeBoss`, so the spawn-time `SetHealth(2500)` needs no special case.

**Join and leave.** Store `baseMax` per entity id. On `PlayerJoined` / `PlayerLeft`, recompute max from `baseMax` and scale current health by `newFactor / oldFactor`. A join during the fight does not reset the boss to full. Do not scale a second time inside `MakeBoss` if `baseMax` is already stored.

A boss gets no extra body.

### 2. Extra animals for scripted attacks

**Pool.** After the map script's own `AddAnimal`s, each peer calls `AddAnimal` again for a reserve pool. Do it from `OnMapLoaded`, before puppets are created, in the same order on every process. Those calls append entity ids. Matching order is what already makes cobra ids 49–51 match on MAP3B (`re/notes/snake-sync.md`).

For each species the map's `AddAnimal` list contains, create `3` reserves (the 4-player maximum), except:

- Skip a species that is only used as a boss (yeti, hogzilla, big grim). One body, scaled health.
- Skip `ANIMAL_WATERCROCODILE` and `ANIMAL_CROCODILE` until those classes are mirrored. Local-only extras would fight each client alone.
- Name them `coop_<species>_<0..2>` so `FindEntScrObject("wolf1")` still finds the script wolf.

Spawn already parks them at the sentinel with brain `+0x79 = 1`. Leave them there until a wave needs them.

Worst case in the current scripts, if every species got 3 reserves: MAP4B reaches 30 animal entities before NPCs, MAPARENA 29. Both sit under 32 until guides are added to the same snapshot. Log when `animals.size()` hits `kMaxAnimals`. Raising that cap belongs in the same change as the pool, together with the L2 unreliable-fragment flag if the packet grows past one MTU.

**What counts as a wave.** A scripted attack is a trigger bringing a non-boss, non-reserve animal of a mirrored class from inactive to an `AIHuntTask_Attack*` task.

Hook these and record while the call is on the stack:

- Fire `0x68130` (`OnActivate`).
- `BeginCutScene` (the class-manager call of trigger `+0x210` inside `0x68800` case 4).
- Script `StartTask` on `cScrCharacterEx` (CommonDll RVA `0xe000`) when the name starts with `AIHuntTask_Attack`.
- Script `SetTaskParam` (CommonDll RVA `0xe0c0`).
- Script `MoveToPos` (`0x161a0`) on an animal.

An animal joins the wave when, during one of those two script calls, it `StartTask`s an attack task and it was inactive just before (`+0x4B0 == 0` or position y `< -1000`). Re-issuing `StartTask` on a wolf that is already in the fight is a retarget. MAP2A `cScrTask03` does that to wolf1 and wolf2. It does not add another pack.

MAP2B's siege calls `SpawnWolves` repeatedly and recycles wolf1–3 with `CanBeRespawned` / `MoveToPos`. Treat that as one pack, not one new wolf per recycle. The rule that does this: for each species, while at least one script-owned animal of that species is active on an attack task, keep `extra` reserves active. If the reserves are already up, a recycled `MoveToPos` does not release more. When a reserve dies it may be used again the next time the species goes from "none active" to "some active".

**Release (host only).** After `BeginCutScene` returns, or after `OnActivate` if the trigger's `+0x210` is 0:

1. Group the wave by species. Remember each animal's `StartTask` name and the `SetTaskParam` pairs that followed it (`target`, `state`, `surpriseid`, `health`, `dmgint`, …). Wolf1's target and wolf2's target differ. Give each reserve one template, round-robin.
2. Take the first parked reserve of that species.
3. Call `FUN_10014610(reserve, 3, 0)` so it gets a fresh max health, `IdleAttack`, and an unfrozen brain, the same path as script `MoveToPos`.
4. Copy the template's position and yaw. Offset x/z by about 1.5 m times the reserve index so they do not stack in one capsule.
5. On the reserve's script object, call the same `StartTask` and replay that template's `SetTaskParam` list.

The script object is `cObjScriptedEntity::GetScrObject` (CommonDll `0x1aa0`): entity `+0x1B4` is the class-manager index, and the record's `+8` is the `cScrEntity`. `SetHealth` already uses script `+0x28` as the entity, so the link is two-way. The brain pointer is entity `+0x200`. `cAIBrain::StartTask` (CommonDll `0x6b460`) is the lower call. Prefer the script natives. They are the calls the map already makes, including `cAIHuntTask_AttackBase::SetParam` (CommonDll `0xbc3d0`), which resolves `"target"` by entity name.

Clients do not run this release. Their reserves stay parked and frozen. The next host snapshot carries the new position (y is no longer below -1000), and `ApplyMirror` places and freezes the local copy the same way it places wolf1. `FindAnimal` looks up by id, so the ids have to match. First test is that log line: both processes print the same reserve ids after load.

**Quotas.** Extra wolves are pressure. They should not finish "kill 3 wolves" early, and they should not count toward MAP2B's kill-10. Hook present-death `0x1e8b0`. When `this` is a reserve, set a flag, call the original, clear the flag. Hook `0x91e80` and return immediately while the flag is set, so the mission never sees that species event. Script wolves still report. Confirm on MAP2A: after the extras are dead, task 5 still waits for the three script wolves.

Leaving the flag unset makes extras count as `ANIMAL_GWOLF` and shortens shoot tasks. That is the wrong default for this request.

**Who simulates.** Host brain only, same as the animals that already exist. Client `FreezeBrain` on the new id is the current mirror path. Do not `MoveToPos` the reserve on the client. That call clears `+0x79` (audit F3 / gap G5).

**Hunt maps.** Leave them unchanged. `HUNTMAPBASE.SCR` binds one `the_predator`, and the claim / end script follows that one animal. An extra predator is a different feature. The `MakeBoss` path does not fire on hunt maps.

**Late join.** A player who connects after the trigger has fired does not add a body to a wave that already released. The boss factor does recompute from `baseMax` (section 1). A player who is already in `g_remotes` when the trigger fires counts.

**Drop-out.** Leave the extra body in the fight and do not heal the boss. Removing a wolf from under the players is a pop-out. Recompute boss max downward with the ratio rule if a leave should ease the rest of the fight. Pick one before shipping. The ratio recompute is the consistent choice. The pop-out is the one to avoid.

## MAP2A opening, end to end

1. Load. Both peers already have wolf1, wolf2, wolf3. `OnMapLoaded` adds `coop_GWOLF_0..2`, `coop_MOUNTAINLION_0..2`, `coop_GBEAR_0..2`, parked. Ids match in the two logs. Snapshot count is 6 + 9 animals, plus Hugh, under 32.
2. Two players are connected. `extra = 1`.
3. Host crosses `cScrTask02`. `OnActivate` starts both wolves. `BeginCutScene` places wolf1.
4. After `BeginCutScene`, wolf1 and wolf2 are the wave. One reserve is unparked beside the template (wolf1's target `"hunter"`, or wolf2's `"hugs2a"` if the round-robin lands there), `AIHuntTask_AttackDogs`, species health still 50 × the retail factor.
5. The client snapshot moves that id off the sentinel. The client shows three wolves. The client's own script still only knows wolf1 and wolf2.
6. `cScrTask03` starts the same two script wolves again. The reserve is already active, so the pack stays at three.
7. Later, `cScrTask04` unparks `mlion1` into `AIHuntTask_AttackCats`. One extra lion. `mlion2` stays the map's unused lion. The reserve does not take its name.
8. MAP12C: the yeti hits `MakeBoss`, `+0x3C0 = 1`, health 2500 → 3125 with two players, 3750 with three, 4375 with four. No second yeti. `BeginCutScene`'s `MoveToPos` restores the scaled max.

## Test

- `run-coop-local.ps1 -Map MAP2A -Players 2`, then 3, then 4. After `cScrTask02`, the host log shows one, two, or three `coop_GWOLF_*` leaving y=-10000, and each client mirror line for those ids has the same position within a step. Pack on screen is 3, 4, 5. Wolf health stays 50 (times the retail 1.0/1.1/1.2).
- `Players 2` on MAP12C. Yeti `+0x19C` and `+0x4C0` are 3125 after `MakeBoss`, and still 3125 after the cutscene `MoveToPos`. The client boss bar matches the host ratio.
- Kill only the reserves on MAP2A before task 5. The "kill 3 wolves" task stays open until wolf1–3 die.
- MAP2B siege (`cScrTask08` `SpawnWolves`): the extra count stays at `extra` while the script keeps recycling three wolves. It does not climb by one per recycle.
- Both logs: `tracker` animal count and the reserve id list are identical. A client line `mirror id=… waiting for a local entity` means the `AddAnimal` order diverged.
- Snapshot log never stops at exactly 32 on MAP4B or MAPARENA with a guide alive. If it does, raise `kMaxAnimals` before calling the pool done.

## Confidence

| Piece | Confidence | Why |
|---|---|---|
| `+0x19C` current, `+0x4C0` max, `MoveToPos` restores max | High | Decompiled `0x1e170`, `0x14610`, `0x15ff0`. Runtime deer hp and the park sentinel are in `MEMORY.md`. |
| `MakeBoss` sets `+0x3C0` and the bar reads `+0x19C/+0x4C0` | High | Decompiled `0x16d20`, `0x12380`. Bar ratio already in `npc-health.md`. |
| Retail factors 1.0 / 1.1 / 1.2 | High | `0xa530` × float `0.1` at RVA `0xFC46C`. |
| Player count | High | `g_remotes` is what the 4-player session already uses. |
| `AddAnimal` creates a real parked `cScrAnimal`, mirrored if both peers call it in order | High | `0x8d970` decompile. Cobra ids matched across processes. Tracker scans the vtable, not a fixed list. |
| Wave rule (release `extra` once while that species is in an attack, not on every `StartTask`) | High as a design, untested in game | The call sites and the MAP2A / MAP2B scripts are verified. The grouping has not been logged on a live trigger. |
| Skipping `0x91e80` keeps shoot-task quotas | Medium-high | The death report and the task fields are verified. The decrement site was not decompiled. The MAP2A task-5 test is the check. |
| Crocodile extras | Out of scope | Those entities are not mirrored. |

Editing `TRIGGERMISSION02A.SCR` to name three more wolves would miss every other map, would run the extra `StartTask` on the client (unfreeze, gap G5), and would feed the kill counter. The reserve pool uses the spawn the game already has and stays inside the host simulation the mirror already copies.
