# Helicopter arena (scope, not built)

Easter-egg fight: a helicopter that orbits the nearest player and drops explosions, plus a full-auto version of a shipped pistol. No new mesh and no new terrain. Decided 2026-10-17 from the asset and script inventory.

## What we are making

A copied small arena (start from `MAPARENA`, the Aztec bonus). One helicopter circles the nearest living player and, every few seconds, marks a spot on the ground and then hurts anyone standing there. Players spawn with a new weapon entry that reuses the handgun 3 viewmodel, a larger magazine, and a faster fire animation. Killing the helicopter ends the map.

Playtest it with `-spmap` before inserting it into the story. Story insertion is a host map-load detour around one existing exit. Retail `trigger_level` ignores the map name in the BSP and asks the profile instead.

## Arena

Do not run the retail `MAPARENA` script. Its `cMapTest::Update` respawns animals on a timer with `rand()` on every process, which desyncs co-op (`re/notes/audit-maps-c.md` section 7).

- Copy `BSP\MAPARENA.BSP` to a loose file whose script path is the same length (`maparena` → `bossmap1`).
- New `scripts/levels/bossmap1/` text project. Leave the `.XSC` out so `cVMachine::SmartLoadPrj` compiles it.
- Mission script: equip the auto gun, spawn the helicopter and the hit-point animal, `MakeBoss`, on death `CheckMissionStatus`. No animal waves.
- `MAPARENA` has no helicopter entity. Spawn `obj_helicopter` from the script or the ASI, the same `CreateEntity` → `AddEntity` → `Spawn` recipe the puppets use.

## Helicopter flight and aggro

Shipped helicopter motion is a baked clip (`Ani1` → `Loop1` → `Ani2` … in `HELICOPTER.SCR`, plus map clips like `m3_rescue`). There is no fly-to-player AI. The yeti thrower (`throwtime 5`, `throwdmg 30`) does hunt, and its rocks are host-only and do not hit puppets. Do not use it for this fight.

Host-only mover, each frame:

- Target = nearest living player (local hunter or a puppet).
- Goal = that player's x/z plus an orbit (about 18 units out, 12 up). Advance the orbit angle with time.
- Step the entity toward the goal, capped (about 15 units/s), and set yaw to face the target.
- Play a loop clip so the rotor turns. **First spike:** if that clip has root motion, it will fight `SetPos`. Then hold one frame (`SetAnimSpeedFactor(0)` after the rotor is visible) or pick a clip that stays in place.

Clients do not simulate this. Send the pose the way animals are sent. Helicopter and truck sync do not exist yet (`audit-maps-b.md`).

## Shooting the helicopter

The helicopter script has no health and no `MakeBoss`. The boss bar is an animal flag (`+0x3C0`, fill `+0x19C / +0x4C0`).

- Spawn a thrower or bear, `MakeBoss(TRUE)`, health in the hogzilla band (1500, then the player-count scale).
- Each frame, put it on the helicopter.
- `MakeVisible(0)` and `SetHitSphereRadius` so the players shoot a volume, not a floating bear. Both natives exist. **Spike:** confirm a bullet hits that sphere and drops `+0x19C`. If it does not, leave the animal visible until a real hit volume is found.
- Death is that animal's health reaching 0. Hide the helicopter (`MakeVisible(0)`), play the same effect as a bomb, then end the mission.

## Bombs

No rocket native. The damage call that does exist is `player.Hurt` (MAPISLAND's out-of-bounds zone: `::globals.player.Hurt(-1, 2, 0)`). Argument order is **unverified**; the spike prints health before and after. Traps use `HurtHuman(20)` on the trap, which is the wrong shape for a ground blast.

Host, about every 4 s while a player is inside 40 units:

1. Pick a point on the ground at the target's feet, a little ahead of their movement.
2. Telegraph ~1.2 s. Move an existing effect there (`MAPARENA` already has an `obj_aifx` named `volcan`; falling-rock `SetAnim` is the other candidate).
3. Detonate. Players inside ~4 units take damage through the existing `kPlayerHurt` path (`combat.cpp`): the host commits, the victim's hunter applies it. That already covers puppets. A local-only `Hurt` would hit one window.

Host death still fails a mission today. Recommended for this map only: a downed host stands back up the way F9 does for a client, so one knockout does not end the family fight. That exception is not built.

## Full-auto gun

Holding fire already loops. `cBodyWeapon` state `Fire` calls `Fire()`, plays `"fire"`, waits for the clip (`WaitForAnimFix(990)`), returns to `Idle`, and `Idle` enters `Fire` again while `BODYCMD_WEAPON_FIRE` is set and the clip has ammo. Rate of fire is the length of that animation. Magazine is `clipSize` on the `MODWeaponDef`.

Do not edit `handgun3_1st` in place. `ITEMS.INC` is global, and a loose copy replaces the whole archived file. Add a new def that the arena equips and the story never names.

| Field | Value |
|---|---|
| Name | `autogun_1st` (new) |
| Looks | handgun 3 skin, `handgun3.ap`, anim `handgun3` |
| Class | `obj_handgun`, new `scrclass` whose body extends `cBodyHandgun3` |
| Magazine | `clipSize` 90, `initialAmmo` 300, matching `MODAmmoDef` |
| Fire rate | overridden `Fire.Begin`: `SetAnim("fire")` then `SetAnimSpeedFactor` (start at 2.5, tune in game) |
| Damage | lower `bulDamage` than handgun 3 (`50000`) so the boss lasts |

`cBodyHandgun3` does not override `Fire`, so the new state replaces the one on `cBodyWeapon`. New `.SPJ` lists `Weapon.scr`, `hgun.scr`, and the small new file. No `.XSC`. `EquipAIWith("hunter", "autogun_1st")` on this map only.

It will look like a machine pistol. A minigun mesh is a model-compiler project and is out of this scope. The other player's puppet shows whatever third-person skin this item resolves to; confirm it, and add a puppet-weapon row if the handgun case does not match.

## Build order

1. **Gun, solo, any map.** New item, hold fire, count shots and the reload. Confirm speed factor actually shortens the cycle.
2. **Helicopter, solo.** Spawn it, orbit one player, watch for the clip dragging it off the path.
3. **Hit volume, solo.** Invisible boss animal glued to the helicopter. Shoot it. Boss bar moves.
4. **Bomb, two windows.** Telegraph, then `kPlayerHurt` on both players. Host pose is what the client draws.
5. **Win, then the story detour.** Death ends the map. Only then hook one chapter exit to load it and send the players onward.

## Out of scope

New BSP geometry, a saucer or minigun mesh, helicopter AI from the retail brain, yeti rocks as the weapon, and turning on `MAPARENA`'s animal spawner.
