# Phase 5 — shared combat: lead plan (2026-10-16 20:25, static analysis only)

Problem today:
- A client's animals are frozen mirrors, so nothing on a client can hurt that client: clients are invulnerable.
- On the host, animals attack only the host hunter.

Goal: host animals attack the nearest player, and the damage lands on that player's own hunter.

## Engine facts (CommonDll7, decompiled `re/notes/lead-p5-*.c`, gitignored)

- **Attack target list:** the `cAIHuntAnimalInterface` singleton `DAT_101aea7c` (CommonDll7 RVA `0x1aea7c`).
  - `AddAttackTarget(cObjEntity*)` `0x9f640` (thiscall): appends to the list at `+0x6c` unless it is already there.
  - `RemoveAttackTarget(cObjEntity*)` `0x9f770` (thiscall): removes it, and also clears it from the per-animal assignments in `DAT_101aeb34`, then re-targets those animals to list entry 0.
  - `GetAttackTargetCount`/`GetAttackTargetAt`/`IsAttackTargetInList`/`RemoveAllAttackTargets` are exported too.
- `cAIHuntBrain_Animal::GetClosestAttackTarget` `0x81ac0` returns the list entry closest to the animal's entity (`brain+0x68`), by squared distance of `+0xE8`. **Animals already choose the nearest target from this list**, so co-op targeting only needs the puppets in the list on the host.
- `cAIHuntBrain_Animal::SetTarget(ent, float)` `0x80ce0` stores the target **entity id** at `brain+0x254` and a float at `+0x258`. `HaveValidAttackTarget` resolves the id and drops it when entity `+0xC1 & 4`, the dead/disabled flag also checked in OnHurt.
- `cAIHuntBrain_Animal::OnHurt(attacker)` `0x83ec0` switches to the attacker when it is in the attack list and is a `cObjCharacterEx` (vt `+0xfc` alive check). A puppet (`obj_humanEx`, a `cObjCharacterEx`) qualifies.
- **Damage entry:** `cObjScriptedEntity::OnHurt(attacker, float dmg, int type, const tVect3& pos, const tVect3& dir)` `0x3e290`, virtual.
  - Type 1: `health -= dmg`. Type 2: `health -= dmg * health * k`.
  - Clamps health at 0. At 0 with `+0xC1 & 4` clear it calls vt `+0x154` (death).
  - It always calls vt `+0x150(attacker, dmg, type)` (hurt reaction / script `ScrHurt`).

## Plan (P5a–P5d)

1. **P5a, host targeting.**
   - After a puppet spawns on the host: `AddAttackTarget(puppet)`. Before a puppet is removed, and on map load: `RemoveAttackTarget(puppet)`. A dangling list entry would crash `GetClosestAttackTarget`.
   - Verify on MAPARENA or the HUNTMAP4 puma (`SpawnPredator` / script `SetPredatorAttack`):
     - the predator runs at the puppet when it is closer than the host;
     - the animal attack animation plays on the client mirror (state names replicate).
   - Open: puppets have no physics body (`UnlinkPhysics`). The attack may need a physics contact; check by logging puppet `OnHurt` calls.
2. **P5b, damage to the right player.**
   - Hook OnHurt for puppets. Either use a vtable slot hook on the puppet class, or an inline hook on `0x3e290` filtered to puppet entity pointers; if humanEx overrides OnHurt, hook that override.
   - On the host, for a puppet: do not let the puppet's health drop (or reset it). Send `PlayerHurt {playerId, attackerEntityId, dmg, type, pos, dir}` reliably to that client: a new `GameEvent` kind, i0 = attacker id, f0 = dmg, key = type.
   - Client: call its own hunter's vt OnHurt with the attacker = its mirror of that animal. The hunter's health, hurt camera and death then run locally.
3. **P5c, player health and death.**
   - `PlayerState.health` currently sends a constant 100. Send the hunter's real `+0x19C`.
   - Host: a dead remote player's puppet gets `+0xC1 |= 4` or `RemoveAttackTarget`, so animals stop chasing it.
   - Death policy (game over for all, or spectate until the map ends): user decision, open.
4. **P5d, scripts' "hunter".**
   - Host-side hook on the `GetDistBetween2Ais` native (GameDll7 `0x9200`, cdecl(name, name) → float). When either name is `"hunter"`, return the minimum distance over all players. This covers spook distance, trophy distance and predator safe distance (`re/notes/lead-mission.md`).
   - Task params with target `"hunter"` (for example `SetPredatorAttack`) still pick the host. With the attack list, the brain re-targets to the closest anyway (`GetClosestAttackTarget`); verify.
5. QTE / melee / struggle (story): see `re/notes/story-inventory.md` (worker job) before designing.

## Risks

- Hunter-only checks inside C++ (player flags, stealth level, `PLAYER_CAN_CROUCH`, HUD) are per process and fine.
- The attack logic may read hunter-only fields from its target, for example the struggle/QTE path. Test with the puma first.
