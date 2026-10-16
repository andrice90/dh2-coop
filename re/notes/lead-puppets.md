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

## Risks
- Post-load spawn may skip per-entity init done after the Lua lump (model precache, script binding, phys registration). Spike with one entity first; watch for crashes (crash handler writes `dh2coop-crash.dmp`).
- Model/AP must be loaded or loadable at runtime (`SetModel` may load on demand; `PrecacheModel` export exists on cObjManager).
