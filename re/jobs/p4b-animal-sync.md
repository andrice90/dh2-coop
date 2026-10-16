# Job: P4b — shared animals: host broadcast + client apply (YOU OWN GAME LAUNCHES; no Ghidra)

Prerequisite: P4a finished (`AnimalStates`/`AnimalHit` in `mod/src/net`, tests pass).
Read first:
- `re/jobs/_rules.md`
- `STATUS.md`
- `re/notes/lead-animals.md`: the design and the verified recipe. **Follow it exactly.**
- `mod/src/coop/animals.cpp`: the lead's experiment module, which you extend.
- `mod/src/coop/puppets.cpp`: how remote state is buffered and applied.
- `mod/src/coop/coop.cpp`: session pump and event dispatch.
- `mod/src/engine/entity_api.h`

## Goal
Every client shows the host's animals (position, facing, animation state, health/death). The client's own animal AI is off.

## Host (in `animals.cpp`, only when `coop` is hosting with ≥1 client)
- Every 50 ms, build `AnimalStates` from `Animals()` (tracker list) and send it with `Session::SendAnimalStates`. Per animal:
  - `id` = entity id (int16 at `+0x04`)
  - `pos` = `+0xE8`
  - `health` = `+0x19C`
  - `state` = `entity::BodyStateName(animal)`
  - `flags`: bit0 dead when health ≤ 0; bit1 parked when `pos.y < -1000`
  - `yaw`: the animal `+0xF4` is unreliable. Use the heading of movement: `atan2(dx, dz)` in degrees from the position change since the last send, updated only when it moved more than 0.05 m. Otherwise keep the last yaw (start with `+0xF4.y`).
- Expose a way for `coop.cpp` to call this. Prefer a small header `coop/animals.h` with `void HostTick(uint32_t nowMs)`, `void OnAnimalStates(const net::AnimalStates&, uint32_t localMs)`, `void OnAnimalHit(const net::AnimalHit&)`, `void Reset()`.
  - Wire it in `coop.cpp`: host tick in the existing frame pump; client events from `DrainEvents`.

## Client
- Buffer per animal id with a 100 ms interpolation delay, like `net::SnapshotBuffer` for players. Write a small `AnimalBuffer`: lerp position, shortest-path lerp yaw, take state/health/flags from the newer snapshot.
- On first data for an animal: `entity::FreezeBrain(animal, true)`. **Do not** `UnlinkPhysics`: animals render from their physics body (lead finding).
- Apply in the **pre-render callback** (`SetPreRenderCallback` in `player_tracker.h`), not in `OnFrame`. Applying in OnFrame causes flicker. Per animal:
  - `entity::HandleKey(animal, "position", "%.2f %.2f %.2f")` with metres × 100
  - `entity::HandleKey(animal, "angle", "0 %.2f 0")` with the yaw
  - world AABB refresh + `entity::BspUpdate` (as in `animals.cpp` `Place()`)
  - when the state name changes: `entity::SetBodyState(animal, name, false)`
- Health: write the host value to `+0x19C` after reading the local value (see Hits).
- Disable the lead's `[Animals] Spike` path when a session is active (keep it for solo experiments).

## Hits (client → host)
- Client, each frame, before writing the host health: if local `+0x19C` < last written host health − 0.01, send `AnimalHit {animalId, damage = difference}`, then write the host health back.
- Host on `AnimalHit`: call `cAIHuntAnimalInterface::FuncInflictDamage` (CommonDll7 export `?FuncInflictDamage@cAIHuntAnimalInterface@@KAXPAXM@Z`, `static __cdecl void(void* animal, float dmg)`). Add a wrapper in `entity_api` that resolves it from CommonDll7. Log it.

## Test (two instances; tools are in `tools-src/`)
1. `.\tools-src\move-test.ps1` style: launch the pair. Log on both sides every 2 s for each animal: id, pos, state.
   - The client's animal positions must match the host's within 0.5 m. Their state names must follow the host's with < 300 ms lag.
   - Write a comparison table into the notes.
2. Screenshot both windows (`capture-window.ps1 -NoFocus`) when a deer is in view, if one comes near the spawn. Report honestly if none is visible.
3. Hit test, if you can arrange it: shoot (scripted `LMB`) at a deer in view on the client. The host must log `AnimalHit` and apply damage. If no deer is reachable, say so; the user will test by hand.

## Rules
- Edit `mod/src/coop/*`, `mod/src/engine/entity_api.*`, `mod/dh2coop.ini` (new keys documented). Do not edit `mod/src/net/*`, except bug fixes you report.
- Notes: `re/notes/p4b-animal-sync.md`: what you built, test tables, screenshots, problems, `## Proposed MEMORY facts`.
- Never commit. Never edit `STATUS.md`/`MEMORY.md`.
