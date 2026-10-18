# Player-count difficulty (research, 2026-10-17)

How to scale story fights for 2, 3, and 4 players. This is the method to build. It is not in the mod yet.

Two rules, from the request:

| Players | Extra players | One large animal, or a boss | Pack or smaller animal |
|---|---|---|---|
| 1 (solo, or co-op not connected) | 0 | unchanged | unchanged |
| 2 | 1 | ×1.25 health, still one body | +1 body, health unchanged |
| 3 | 2 | ×1.50 health, still one body | +2 bodies, health unchanged |
| 4 | 3 | ×1.75 health, still one body | +3 bodies, health unchanged |

`extra = connected - 1`. Health factor is `1 + 0.25 * extra`. MAP2A's opening is wolf1 and wolf2, so the pack becomes 3, 4, or 5 wolves. MAP2B's grizzly stays one bear at 350 × the factor.

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

### 1. Health, one body

Three cases share this path. None of them gain a reserve animal.

1. **Boss.** Entity `+0x3C0` becomes 1. That is `MakeBoss(TRUE)`: yeti (2500), hogzilla (1500), big grim (1500).
2. **One large animal.** The wave (section 2) contains exactly one script animal, and its species is large. MAP2B `cScrTask03` starts one `gbear` on `AIHuntTask_AttackBears`. MAP2C `cScrTask08` does the same. MAP2B's cougar is one `mlion1` on `AIHuntTask_AttackCats`. Each stays a single body.
3. A large species in a wave of two or more is a pack. It takes the body path and does not also gain health. Two lions stay two lions plus the extra copies. One lion gains health.

**Large species** (health when the wave size is 1): `ANIMAL_GBEAR` (350), `ANIMAL_BBEAR` (200), `ANIMAL_ABEAR` (350), `ANIMAL_PBEAR` (400), `ANIMAL_LION` (170), `ANIMAL_LIONESSE` (170), `ANIMAL_MOUNTAINLION` (80), `ANIMAL_PUMA` (100), `ANIMAL_BTIGER` (250), `ANIMAL_RHINO` (450), `ANIMAL_ELEPHANT` (500), `ANIMAL_HIPO` (400), `ANIMAL_SCRUBBULL` (500). Numbers are the `SetHealth` in each species `InitModel`.

**Smaller and mid-game species** stay on the body path even when the wave is one animal: `ANIMAL_GWOLF` (50), `ANIMAL_HYENA` (40), `ANIMAL_DINGO` (30), `ANIMAL_LEOPARD` (150), `ANIMAL_WILDBOAR` (250), `ANIMAL_WILDBOAR3` (250), `ANIMAL_COBRA`. MAP2B's "kill 1 wolf" is still a wolf, so two players get two wolves. Classify by species id at entity `+0x434`, captured from `AddAnimal`. Do not classify by the current `+0x19C`. MAP2A `wolf3`'s `SetHealth(600)` is still a grey wolf. `Make_Scared` writes 10000 through `SetHealth` and would look like a boss if the test were the live value (`GBEAR.SCR`, `re/notes/assets.md`).

**Base max.** At the end of spawn `0x1e170`, store `baseMax = +0x4C0`. That is the species health after the retail difficulty multiply, before a later `SetHealth(600)` or `Make_Scared`.

**When.** Same write for bosses and for a one-large wave:

```
max    = baseMax * (1 + 0.25 * extra)
+0x4C0 = max
+0x19C = baseMax == 0 ? max : +0x19C * (max / baseMax)
```

Use `baseMax` as the denominator, not whatever `+0x19C` was set to in the same trigger. Hook `MakeBoss` `0x16d20` and apply it there. For a one-large wave, apply it when the wave is classified (after `BeginCutScene`, or after `OnActivate` when the trigger has no `BeginCutScene`).

Also hook `0x14610`. If that animal is already marked, then after the original copies `+0x4C0` into `+0x19C`, write both fields from `baseMax * factor` again. MAP2B's bear calls `Make_Scared` in `OnActivate` (current health 10000) and `MoveToPos` in `BeginCutScene`. Retail `MoveToPos` already replaces the 10000 with `+0x4C0` (350 times the retail factor). The player fights that 350. The co-op factor multiplies that value: 437.5, 525, or 612.5 before the retail 1.0/1.1/1.2. Applying the factor to the 10000 and then letting `MoveToPos` restore an unscaled max would drop the bonus on the first frame of the fight.

Do this on every peer, from the same `extra`. The host's `+0x19C` is what the snapshot already broadcasts. The client's `+0x4C0` has to move too, because a boss bar divides locally. A grizzly has no boss bar. Its current health still has to match, and the next `MoveToPos` reads `+0x4C0`.

If `SetHealth` runs on a marked animal after that, hook `0x15ff0` and write the scaled value into both `+0x19C` and `+0x4C0`, and update `baseMax` to the script's unscaled argument times the retail factor. Species `InitModel` runs inside `0x1e170`, before either hook, so the spawn-time `SetHealth(350)` or `SetHealth(2500)` needs no special case.

**Join and leave.** On `PlayerJoined` / `PlayerLeft`, recompute max from `baseMax` and scale current health by `newFactor / oldFactor`. A join during the fight does not refill the bar. Do not scale a second time inside `MakeBoss` or the wave classifier if `baseMax` is already stored and the factor has been applied for this `extra`.

### 2. Extra animals for scripted attacks

**Pool.** After the map script's own `AddAnimal`s, each peer calls `AddAnimal` again for a reserve pool. Do it from `OnMapLoaded`, before puppets are created, in the same order on every process. Those calls append entity ids. Matching order is what already makes cobra ids 49–51 match on MAP3B (`re/notes/snake-sync.md`).

Create `3` reserves (the 4-player maximum) for each smaller or mid-game species the map's `AddAnimal` list contains. Also create them for a large species when that map starts an attack on two or more of that species in one trigger. A map whose only bear is one `gbear` creates no bear reserves.

- Skip yeti, hogzilla, and big grim. One body, scaled health.
- Skip a large species that this map only ever sends in alone. The grizzly and the MAP2B cougar are this case.
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

1. Group the wave by species. A group of one large animal is section 1: mark it, scale `baseMax`, release nothing. Remember each other animal's `StartTask` name and the `SetTaskParam` pairs that followed it (`target`, `state`, `surpriseid`, `health`, `dmgint`, …). Wolf1's target and wolf2's target differ. Give each reserve one template, round-robin.
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

1. Load. Both peers already have wolf1, wolf2, wolf3, mlion1, mlion2, and gbear. `OnMapLoaded` adds `coop_GWOLF_0..2` only. The mountain lion and the bear are large and this map attacks them one at a time (`mlion2`'s start is commented out, and `gbear` is not started). Ids match in the two logs. Snapshot count is 6 + 3 animals, plus Hugh, under 32.
2. Two players are connected. `extra = 1`.
3. Host crosses `cScrTask02`. `OnActivate` starts both wolves. `BeginCutScene` places wolf1.
4. After `BeginCutScene`, wolf1 and wolf2 are the wave. One reserve is unparked beside the template (wolf1's target `"hunter"`, or wolf2's `"hugs2a"` if the round-robin lands there), `AIHuntTask_AttackDogs`, species health still 50 × the retail factor.
5. The client snapshot moves that id off the sentinel. The client shows three wolves. The client's own script still only knows wolf1 and wolf2.
6. `cScrTask03` starts the same two script wolves again. The reserve is already active, so the pack stays at three.
7. Later, `cScrTask04` unparks one `mlion1` into `AIHuntTask_AttackCats`. That is a single mountain lion. Health becomes 80 × 1.25 = 100, times the retail factor. No second lion.
8. MAP2B `cScrTask03`: one grizzly. After `BeginCutScene`'s `MoveToPos`, health is 350 × the factor (437.5 / 525 / 612.5) times the retail factor. The log shows no `coop_GBEAR_*` leaving the sentinel. MAP2B's single `mlion1` is the same kind of fight at base 80. MAP2B's single wolf is not: it gains a body.
9. MAP12C: the yeti hits `MakeBoss`, `+0x3C0 = 1`, health 2500 → 3125 with two players, 3750 with three, 4375 with four. No second yeti. `BeginCutScene`'s `MoveToPos` restores the scaled max.

## Test

- `run-coop-local.ps1 -Map MAP2A -Players 2`, then 3, then 4. After `cScrTask02`, the host log shows one, two, or three `coop_GWOLF_*` leaving y=-10000, and each client mirror line for those ids has the same position within a step. Pack on screen is 3, 4, 5. Wolf health stays 50 (times the retail 1.0/1.1/1.2).
- `Players 2` on MAP2B through the grizzly (`cScrTask03`) and the cougar. One `gbear`, `+0x19C` and `+0x4C0` at 437.5 after the cutscene `MoveToPos`. One `mlion1` at 100. No reserve id for either species. The opening single wolf still adds one `coop_GWOLF_*`.
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
| One large animal scales health and adds no body | High as a design, untested in game | MAP2B `cScrTask03` is one `gbear` and `AttackBears`. MAP2C `cScrTask08` is the same. MAP2B's cougar is one `mlion1`. Species `SetHealth` values are in the species scripts. The live factor has not been logged. |
| Wave rule (release `extra` once while a smaller species is in an attack, not on every `StartTask`) | High as a design, untested in game | The call sites and the MAP2A / MAP2B scripts are verified. The grouping has not been logged on a live trigger. |
| Skipping `0x91e80` keeps shoot-task quotas | Medium-high | The death report and the task fields are verified. The decrement site was not decompiled. The MAP2A task-5 test is the check. |
| Crocodile extras | Out of scope | Those entities are not mirrored. |

Editing `TRIGGERMISSION02A.SCR` to name three more wolves would miss every other map, would run the extra `StartTask` on the client (unfreeze, gap G5), and would feed the kill counter. The reserve pool uses the spawn the game already has and stays inside the host simulation the mirror already copies.
