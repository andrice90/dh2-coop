# Phase 4 — shared animals: lead design and findings (2026-10-16)

Goal: one set of animals for all players. The host simulates them; clients show the host's animals and
forward their hits to the host.

## Verified facts (runtime, `mod/src/coop/animals.cpp` experiment, `[Animals] Spike=1`)

### Animal identity
- HUNTMAP4 animals are entity ids 120–124 on every instance: same class (`cScrAnimal`, GameDll7 vtable
  `+0xff470`) and same order (4 red deer at hp 45, plus a puma at hp 100).
- Puma 124 is parked at (-10000,-10000,-10000).
- Spawn positions are **random per instance**: 120 and 123 differed between host and client.
- **Network animal id = entity id.** The host must send positions, because clients cannot assume the same start.

### Body states
- `entity::BodyStateName(animal)`: model part 0 → `+0xd8` state machine → vt `+0x80`.
- Names seen on deer: `Walk`, `Run`, `IdleAttack`, `CutScene`, `PostPlayAnim`. Idle grazing toggles `PostPlayAnim` ↔ `CutScene` every ~3 s.
- `entity::SetBodyState(animal, name, false)` (vt `+0x7c`) switches the state, and the read-back equals the request. **This is how clients mirror the host's animation.**

### Freezing the AI
- `entity::FreezeBrain(animal, true)`: `cAIHuntBrain_Animal::FreezeBrain` on the brain at `+0x200`.
- It returned OK. Whether the frozen brain alone stops all self-motion is UNVERIFIED (see below).

### Moving an animal: physics matters
- `SetPos`/`SetAngle` alone, physics kept: the physics step snaps the animal back (drift 16–66 m/frame).
- `UnlinkPhysics` + `SetPos`: the entity `+0xE8` holds (drift 0), but **the deer is not drawn there**. Animals render from their physics body / dynamic skeleton.
- **Working:** `HandleKey(animal, "position", "<x*100> <y*100> <z*100>")` plus `HandleKey(animal, "angle", "0 <yaw> 0")`. This is the map-load path, `cObjEntity::HandleKey` (EngineDll7 `0xbb660`):
  - `position` (cm, ×0.01) = `SetPos` + phys vt `+0xa8`(pos).
  - `angle` = `SetAngle` + phys vt `+0xb0`(matrix from `FUN_100314f0`).
- Result: drift ≤ 0.02 m (string rounding), and the deer is visible in front (antlers in frame `animal-0.png`, 15:34). User visual confirmation pending at the time of writing.
- Better later: call phys vt `+0xa8` directly with a tVect3 (no string formatting).

## Design (host-authoritative)

### Host
- Every 50 ms (20 Hz), send `AnimalStates`, one message carrying every animal. Per animal:
  - `u16 id`, pos `f32[3]`, yaw `f32`
  - `u8 health` (or f32)
  - `u8 flags`: dead, parked
  - body-state id: an index into a per-session string table, or the name truncated to 15 chars
- Unreliable channel. Reliable for death events.
- The host keeps its own animals' AI (normal game).

### Client, per animal id
1. On first state: `FreezeBrain(true)`. Keep physics (needed for drawing). Do NOT unlink.
2. Each frame: interpolate (reuse `net::SnapshotBuffer`-style 100 ms delay), then `HandleKey` position/angle.
   - The angle comes from the host's `+0xF4`. Caveat: the animal `+0xF4` is often (0,0,0) while moving (MEMORY). Derive yaw from the host's velocity, or read the physics/model heading on the host.
3. When the host state name changes: `SetBodyState(name)`.

### Hits
- Client shots hit the client's local copy (hit bones, `AddHitBone` in scripts).
- Hook the damage entry on clients (`FuncInflictDamage`, CommonDll7 `0x9a540`, subtracts `+0x19C`). Cancel it locally and send `AnimalHit {id, damage, hit bone/pos, shooter}` reliably to the host.
- The host applies it with the same function → health/death replicate via `AnimalStates` plus the death state name.

### Open questions (next experiments)
1. Does a frozen brain plus our per-frame position fully stop the deer's own locomotion? Check over 30 s, with drift logged per frame.
2. Animal yaw on the host: `+0xF4` is unreliable. Find the real heading (phys matrix or model), or use velocity.
3. `FuncInflictDamage` signature and the caller chain from a bullet hit (hook and log on a shot at a deer).
4. Death: does `SetBodyState("<death state>")` plus health 0 reproduce the death/ragdoll? Score/trophy is host-only.
5. Puma activation (parked until triggered): the host decides; clients follow positions.
