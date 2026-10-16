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

### Placement timing
- **Place in pre-render, not OnFrame.** The frame order is sim/physics → render → Present (our OnFrame). Positions set in OnFrame are moved by physics before the next draw, which caused the user-seen above/below flashing.
- `player_tracker` now calls `SetPreRenderCallback(fn)` from the first `cRender::SetCamera` of each frame (game thread, map loaded). After the move, the user saw "better", with the deer circling and running.
- Remaining spike artifacts, which are test-only:
  - Fixed y (feet + 0.7 m) sinks into the terrain.
  - The circle follows the hunter.
  - The deer is invisible over part of the circle (user, by clock position: visible 10–1 o'clock, gone 2–6, one frame at 6). **The user says it is not underground ("not obscured by ground mesh, it's above that") → culling.**
  - Our world box is not the cause: it is symmetric (`box rel (-2.66,-1.67,-2.66)-(2.66,4.04,2.66)`, 15:40).
  - **Hypotheses:**
    - (a) The renderer culls animals by their dynamic-skeleton/physics-body bounds, which lag a teleported root.
    - (b) BSP leaf visibility for the leaf the entity is linked to.
  - **Experiment (lead):** hook `cObjEntity::Draw` (diag hook exists in `puppets.cpp`, `Spike=1`; set the watched entity to the spike deer) and log draws/frame against the circle angle. Also log `+0x124` (BSP link) and `+0xC4` box per frame.
    - Draw not called on the bad side: a BSP/link issue.
    - Draw called: a model-level frustum test (skeleton bounds).
  - Re-check in P4b with real, smoothly moving host positions; it may not occur there.
- **Draw-hook result (15:47–15:50, `animals: diag` lines in `game\Bin\dh2coop.log`):**
  - `cObjEntity::Draw` was called for the deer on **every frame** (133× `50/50` per half second, 4× BSP link 216, otherwise 222). That includes the phases where the user saw it vanish. **Not BSP/entity culling**: the model is drawn but invisible, so it is model-level culling or a bad skeleton.
  - **User:** "visible for the whole circle until I moved to a new location, then the last few revolutions it appeared and disappeared."
    - The spike's circle center follows the view yaw, so turning teleports the deer by up to ~16 m in one frame.
    - **Hypothesis:** large teleports leave the dynamic skeleton / physics bones (and the model bounds derived from them) behind, so the model gets frustum-culled with stale bounds.
  - **Impact on P4b:** the first sync teleports each client deer from its random local spawn to the host's position.
    - If deer flicker after that, look into resetting the skeleton after a teleport. Candidates: EngineDll7 `cDynaSkeleton*` exports (`GetPhysModelPos`, `CreatePhysModel`), the model vt `+0x3c` SetPos, or the phys object's own teleport/velocity reset.
    - Workaround to try first: run the first placement while the deer is hidden (`+0xC0` bit 0), or apply it 2–3 times on consecutive frames.

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

### Hits: simpler variant (preferred, no hook)
- `cAIHuntAnimalInterface::FuncInflictDamage(void* animal, float dmg)` (CommonDll7 `0x9a540`, export `?FuncInflictDamage@cAIHuntAnimalInterface@@KAXPAXM@Z`, static `__cdecl`) only does `*(float*)(animal+0x19C) -= dmg`.
- **Client:** each frame, compare every animal's local health `+0x19C` with the last value written from the host.
  - A drop of `d` means a local bullet hit: send `AnimalHit {id, d}` reliably, then write the host value back.
  - The brain is frozen, so no local death logic runs. Verify that a frozen animal still takes bullet damage.
- **Host:** on `AnimalHit`, call `FuncInflictDamage(animal, d)`. The host's own scripts handle hurt/flee/death. Health and state replicate in `AnimalStates`.
- Open: hit-reaction triggers (flee) may come from the hit event, not health. Check on the host after a remote hit whether the deer flees. If not, also call the hit event (`cScrEntity::Hurt` path / animal vfunc `+0x14c`).

### Open questions (next experiments)
1. Does a frozen brain plus our per-frame position fully stop the deer's own locomotion? Check over 30 s, with drift logged per frame.
2. Animal yaw on the host: `+0xF4` is unreliable. Find the real heading (phys matrix or model), or use velocity.
3. `FuncInflictDamage` signature and the caller chain from a bullet hit (hook and log on a shot at a deer).
4. Death: does `SetBodyState("<death state>")` plus health 0 reproduce the death/ragdoll? Score/trophy is host-only.
5. Puma activation (parked until triggered): the host decides; clients follow positions.
