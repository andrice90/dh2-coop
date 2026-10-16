# Player tracker — lead analysis (2026-10-16)

Source: `re/notes/lead-tracker-decomp.c` (EngineDll7, slot L) + Track B (`object-model.md`).

## Entity transform (`cObjEntity`, EngineDll7)
| Offset | Field | Evidence |
|---|---|---|
| `+0x0C` | local position tVect3 (relative to parent when attached) | `SetPos` `0xbc130` |
| `+0xA4` | int16 parent/attach index, `-1` = world-space entity | `SetPos`/`SetAngle` branch |
| `+0xB4` | `cObjManager*` | `GetObjMgr` |
| `+0xE4` | model ptr | Track B |
| `+0xE8` | **world position** tVect3 | `SetPos` writes, `GetEyePos` `0xb8380` reads |
| `+0xF4` | **world angle** tVect3 (Euler; component order and units UNVERIFIED) | `SetAngle` `0xbc460` writes when `+0xA4 == -1` |
| `+0x100` | last-applied position (change check) | `SetPos` compare |
| `+0x10C` | last-applied angle (change check) | `SetAngle` compare |

## Manager access
- `cObjManager::Get(int)` (`0xaae30`, non-virtual, exported): array `+0x04`, count `+0x08` (`GetEntityCount` `0xaae20`). Temp entities: array `+0x14`, count `+0x18`. Phys manager `+0x20`.
- The live manager is the Main7 `cSpServer` object (derived from `cObjManager`, vtable replaced by Main7), so **virtual** `AddEntity` overloads may not route through the EngineDll7 export → don't rely on hooking them.
- Capture: hook `cObjManager::Get` (called through the import by game code), store the first `this` whose count is in `[1, 0x400]` and whose array pointer is readable; flip a flag so the hook becomes a pass-through, and remove the hook later from `OnFrame` (never unhook inside the detour). Reset the capture on every `OnMapLoaded`.

## Classification
- Local hunter: entity vtable == GameDll7 base + `0x106ff0` (`cScr_ModHunter`). Fallback: `cObjHumanEx::IsPlayer` (bit 3 of `+0x3B4`) only after confirming the entity is a `cObjHumanEx` subclass.
- Animals: vtable == GameDll7 base + `0xff470` (`cScrAnimal`) — per-species subclasses may have other vtables; log a vtable histogram (vtable RVA by module) on first full scan to learn the real set.
- Health: scripted entity float `+0x19C` (animals). Hunter health = profile attribute 9 (not `+0x19C`).

## Plan (implemented in `mod/src/coop/player_tracker.*`)
- `OnMapLoaded`: reset. `OnFrame`: if manager known, every 1 s scan `Get(i)` for i < count with SEH-guarded reads; cache local player and animal list; log local pos/angle and, once per map, the vtable histogram + each animal's id/pos.
- Expose `coop::LocalPlayer()` → `{entity*, pos, angle}` for the net layer (PlayerState) and future puppets.

## Runtime questions to settle
1. Angle component order (pitch/yaw/roll) and units (degrees vs radians): turn in place in-game and watch which component changes.
2. Does the hunter's `+0xF4` follow the camera yaw, or is camera yaw separate (`m_pActiveCamera`)?
3. Is there more than one `cObjManager` calling `Get` (validate by vtable histogram)?
