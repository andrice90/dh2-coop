# Puppets (Phase 3 ghost mode) — lead design notes (2026-10-16)

Goal: one visible third-person human per remote player, positioned/rotated from interpolated `PlayerState`, no AI.

## How the game spawns map entities (Main7, lead decompile `re/notes/lead-puppet-decomp*.c`)
The BSP entity lump is a Lua chunk executed with two bindings:
- `entity("<class>")` = Main7 `FUN_00422410`: if an entity is pending, `cObjManager::Set(mgr, index, pending)` then `index++`; then `pending = cObjManager::CreateEntity(className, 0)` (EngineDll7 static cdecl export `?CreateEntity@cObjManager@@SAPAVcObjEntity@@PBDH@Z`, RVA `0xbd370`).
- `property("<key>", "<value>")` = Main7 `FUN_00422770`: `pending->vfunc[+0x14](key, value)` = `cObjEntity::HandleKey(char*, char*)` (thiscall; base RVA `0xbb660`, derived classes extend it).
Base `HandleKey` keys: `position` (cm → ×0.01 → `+0xE8`, then vfunc `+0xA8` SetPos, phys `+0x118` too), `angle` (degrees → `+0xF4`, vfunc `+0xB0` SetAngle), `mass`, `model_name` (string → `+0xE0`, vfunc `+0x68` SetModel). `cObjEntity::Spawn` (vfunc, base RVA `0xb7f50`) is empty in the base class.
Entity example (MAP2A guide): `entity("obj_guide")` + `model_name "objects/npc/oldhugh/oldhugh.skin,map2a.ap"`, `script class name "cScrHugs2a"`, `script file name "scripts/models/npc/guide/guide.spj"`, `position`, `angle`, `health`.

## Coordinates
BSP `position` is in centimetres; runtime is metres and relative to the map origin (HUNTMAP4 player spawn BSP (-6042, 299, 8604) cm → runtime hunter (0, -1.69, 0); yaw 120 preserved). Remote players share the same map → send/apply runtime coordinates directly.

## Candidate puppet model
`objects/characters/teenm/body/teenmm.skin` (third-person; `teenmm1st.skin` is first-person arms), outfits `autumn.skin` / `winter.skin`; animation packs `game.ap`, per-weapon (`rifle1.ap` …), per-map (`huntmap4.ap`, `maparena.ap`). The hunter's own `model_name` (`+0xE0`) is logged by the tracker to confirm.

## Spawn plan (to verify at runtime, game thread, after map load)
1. `ent = CreateEntity(cls, 0)` — class from `dh2coop-classes.txt` (`DumpClassesKeys` output): prefer a plain animated-character class without script brain (candidates to check: `obj_guide`/NPC class with no `script class name`, or a static model class for a first "box" spike).
2. `HandleKey("model_name", "objects/characters/teenm/body/teenmm.skin,huntmap4.ap")`, `HandleKey("position", "<x*100> <y*100> <z*100>")`? — NO: position via HandleKey is BSP/cm space; after load use `SetPos` (vfunc `+0xA8`) with runtime metres instead.
3. Insert into the manager: replicate `cObjManager::Set(mgr, freeIndex, ent)` or call the manager's `AddEntity(ent)` through its vtable (Main7 override → engine body; the AddEntity hook will log it).
4. `ent->Spawn()` via vtable.
5. Each frame: `SetPos` (vfunc `+0xA8`) / `SetAngle` (vfunc `+0xB0`) from `net::Interp` samples; brain null / frozen.
6. Animation: NPC body commands (`BODYCMD_NPCL_FORWARD/RUN/...` strings in GameDll7) if the class supports them; otherwise idle pose first.

## Animation control (exports)
- Puppet: `cObjScriptedEntity::SetAnim(char const*)` (CommonDll7, virtual) / `cScrEntity::SetAnim` — names as used by NPC scripts (`walk/forward`, `run/forward`, `walktoidle`, `idletorun`, weapon-prefixed `shotgun1/run`, ...). Engine level: `cBodyStateMachine::SetAnim/SetAnimSmooth/SetAnimBlend/SetAnimSpeedFactor`, `cModelPart::SetAnim`. Choose idle/walk/run from interpolated speed (walk ≈ 5 m/s observed for the hunter).
- Phase 4 animals: `cAIHuntBrain_Animal::{GetBodyCommands, SetBodyCommand, AddBodyCommand, ResetBodyCommands, FreezeBodyCommands, UpdateBodyCommands}` (CommonDll7) — host can broadcast body-command bitmasks; clients apply them to frozen-brain animals so locomotion anims match.

## SOLVED 2026-10-16 ~11:45 — puppet visible (user-confirmed)
- `cObjEntity::Draw` hook (diag, `Spike=1` only) showed the puppet **was drawn every frame** (200 draws / 2 s) → not a culling/linking problem.
- Root cause: the player's model `Objects/Characters/Teenm/Body/teenmm.skin` only references `teenmm1st.skin` = **first-person arms ("fpshands1")**; the game has **no third-person player body** (user: "we never see ourselves, even in cutscenes"). The puppet was drawing invisible floating arms.
- Fix: puppets use a full NPC body + its generic anim pack: `[Puppets] Model=objects/npc/mountainguide/mountainguide.skin,game.ap` (default). Any `OBJECTS\NPC\<NAME>\<NAME>.SKIN` with `GAME.AP` works the same way (e.g. Uncle Hugh: `objects/npc/oldhugh/oldhugh.skin,game.ap`; also ausranger1-3, poacher, reginald, siberian1/2, survivor …). The NPC model loads on demand on HUNTMAP4 (no precache needed). World AABB with it: ~1.1 × 1.9 × 1.0 m (real human size).
- Still valid: BSP relink + AABB refresh each frame (`place()` in `puppets.cpp`).

## Runtime status (2026-10-16 11:30, historical) — puppet spawns + collides, NOT yet seen on screen
Implemented in `mod/src/coop/puppets.cpp` + `mod/src/engine/entity_api.*`; enabled with `[Puppets] Spike=1` (local test puppet, no network needed).
- **Works (log + user):** `CreateEntity("obj_humanEx")` → `HandleKey("model_name", <hunter model>)` → manager `AddEntity` (vt `+0x10`, index 127) → `Precache` (+0x54) → `Spawn` (+0x58) → `SetAnim("walk/forward")` (+0x174) all return OK, no crash. Model pointer `+0xE4` non-null, phys `+0x118` non-null, model name = `Objects/Characters/Teenm/Body/teenmm.skin,game.ap,HUNTMAP4.ap` (same as the hunter). **The user physically collides with it in game** → it exists in the world with collision.
- **Not visible yet.** Fixes applied so far: per-frame world AABB (`+0xC4..+0xD8` = spawn-relative box + pos) and `cBspTree::UpdateEntity(*g_bsp, ent)` → BSP link `+0x124` now valid (234/235, like animals 230 / hunter 221). Visibility flag `+0xC0 & 1` = 0 (visible). Draw-gate byte `+0x08 & 8` set (0x18, same as a rendered deer).
- **Direction ruled out (user, 11:32):** the user looked both ways — not rendered anywhere. The puppet was found by collision behind the user once; the 11:31 build places it on the other side (`[Puppets] SpikeSign=-1`, default) and leaves the kind byte alone (`Kind=-1`), but orientation is not the cause. **It is a rendering problem → start at step 2 (Draw hook).**
- Kind byte `+0xA7` (entity type): set per class (GameDll human spawn writes 4 = player, animals 0x42; ladder 2, vehicle 3/5/6, train 7, world 8, scope 9). A bare `obj_humanEx` keeps 0. Setting 4 (tested 11:2x) made no visible difference and may mean "player" (first-person body is never drawn). Try 0x42 only if the flip doesn't fix it.
- Game's player spawn `cObjPlayer::Spawn` (GameDll7 `FUN_10065cb0`, `MOD_ObjPlayer.cpp`): `cObjHumanEx::Spawn` → `SetModel(this+0x3c8)` → pathfinder blocker registration via `g_bsp+0xa7c` (fields `+0x4cc`, `+0x4d0`) → model AABB (model vt `+0xcc`) + `tAABB::Inflate` → `byte +0x08 &= ~0x10` → `+0xA7 = 4`.
- `cObjEntity::Draw` (EngineDll7 `0xbbd30`) draws unconditionally if model `+0xE4` set and `+0xC0&1 == 0` (calls `UpdatePos/UpdateAngle`, lights if `+0x08&8`, then model vt `+0x84`). So if still invisible after the flip, **`Draw` is not being called for the puppet** → find who iterates entities to draw (BSP leaf entity lists per `useServer` side, EngineDll7 global RVA `0x6963dc`; value was 1 during our Present-time calls).

## Next steps (ordered; each is a short experiment)
1. (Done 11:32 — not visible in any direction.) Test command for later re-checks: `.\run-game.ps1 -ExtraArgs '-nointro -developper -cheats -spmap HUNTMAP4'` with `game\Bin\dh2coop.ini` `[Puppets] Spike=1`; pick a weapon; the puppet stays ~3 m from the hunter, swaying sideways. Once visible: set Spike=0 and run the two-instance ghost test (`run-coop-local.ps1`; puppets spawn for each remote player automatically).
2. If not visible: hook `cObjEntity::Draw` (EngineDll7 export `?Draw@cObjEntity@@UAEXXZ`, RVA `0xbbd30`) and/or `cObjCharacterEx::Draw` (CommonDll7) and log when `this` == puppet → tells whether the draw is skipped or the model draws off-screen/invisible.
3. If Draw is never called for the puppet: compare BSP linkage with an animal for both `useServer` sides — call `cBspTree::UseServer(0)` before `UpdateEntity`, then restore 1 (`UseServer` is a static cdecl export; global at EngineDll7 RVA `0x6963dc`). Check whether the render pass runs with useServer=0 (log the value inside the Draw hook for a deer).
4. If Draw is called but nothing shows: compare model state with the hunter/deer — model vt `+0xcc` AABB, `+0x08` bit 0x10 (player spawn clears it), kind `+0xA7` (try 0x42), LOD/distance, and whether `cObjHumanEx::Spawn` needs `SetModel` again after AddEntity (player spawn calls vt `+0x68` with its default model after Spawn).
5. Alternative class if humanEx stays invisible: `obj_cschex` (`cObjCustomCharacterEx`, animated custom character used for scripted props) via `[Puppets] Class=obj_cschex`.

## Risks
- Post-load spawn may skip per-entity init done after the Lua lump (model precache, script binding, phys registration). Spike with one entity first; watch for crashes (crash handler writes `dh2coop-crash.dmp`).
- Model/AP must be loaded or loadable at runtime (`SetModel` may load on demand; `PrecacheModel` export exists on cObjManager).
