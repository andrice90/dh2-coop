# Object model (Track B)

## Progress

Done. Hierarchy, sizes, field offsets, managers, function table, AI-skip answer, puppet strategy, time scale, and the GameDll vtable slots are written from the objmodel dumps plus slot B probes `re/notes/objmodel-probe-gap-common.txt` and `re/notes/objmodel-probe-gap-game.txt`. `mod/src/game/types_draft.h` compiles as 32-bit MSVC C++20. Left open: player slot `LAB_100ec9e0` body, player rifle fire, replicator registration, manager singleton.

RVAs are relative to the PE image base. pefile: EngineDll7, CommonDll7, and GameDll7 are `0x10000000`; Main7.exe is `0x00400000`. The DLLs relocate. Hook by export name or RVA, never a preferred absolute address.

Evidence is the decompiler dump named in each section, or a pefile export RVA. Anything not shown by that code is marked `UNVERIFIED`.

## Hierarchy

Single inheritance, MSVC, vptr at offset 0. `cClass` is the root called by `cObjEntity::cObjEntity` and `cObjManager::cObjManager`. Its size past the vptr is `UNVERIFIED`.

```
cClass
  cObjEntity                          0x194   Engine  ??_7cObjEntity@@6B@           RVA 0x412938
    cObjBaseEntity                    0x19c   Common  ??_7cObjBaseEntity@@6B@       RVA 0x11fba0
      cObjScriptedEntity              0x1d8   Common  ??_7cObjScriptedEntity@@6B@   RVA 0x1206c0
        cObjCharacter                 0x240   Common  ??_7cObjCharacter@@6B@        RVA 0x120c20
          cObjHuman                   0x258   Common  ??_7cObjHuman@@6B@            RVA 0x120df0
        cObjCharacterEx               0x390   Common  ??_7cObjCharacterEx@@6B@      RVA 0x123628
          cObjHumanEx                 0x3c8   Common  ??_7cObjHumanEx@@6B@          RVA 0x1242a8
            cScrPlayer                0x4e8   Game    vtable RVA 0x10a2e0           (not exported)
              cScr_ModHunter          0x838   Game    vtable RVA 0x106ff0           (not exported)
          cScrAnimal                  0x71c   Game    vtable RVA 0x0ff470           (not exported)
        cObjWorld                     0x210   Common  ??_7cObjWorld@@6B@            RVA 0x12d428
      cObjHuntAreaAIEntity            0x1b8   Common  ??_7cObjHuntAreaAIEntity@@6B@ RVA 0x122028
  cObjManager                         Engine base; live object is a Main7 subclass
```

`cObjCharacter` and `cObjCharacterEx` are both direct subclasses of `cObjScriptedEntity`. The CharacterEx ctor (Common RVA `0x27740`) calls `cObjScriptedEntity::cObjScriptedEntity`. Hunt animals and the hunter go through CharacterEx. The older `cObjCharacter` / `cObjHuman` / `cAIBrain_Human` line is a separate tree.

`cScrEntity` is a script instance, not an entity subclass. `cScrEntity::GetHealth` (Common RVA `0x2060`) returns `*(float*)(*(this+0x28)+0x19c)`. `this+0x28` is the `cObjScriptedEntity`. Vtable `??_7cScrEntity@@6B@` RVA `0x11fd08`. Full size `UNVERIFIED`.

### Constructor chains and malloc sizes

| Class | malloc | Factory | Ctor chain |
|---|---|---|---|
| cObjEntity | `0x194` | `CreatecObjEntity` Engine `0xacda0` cdecl | `cClass` then entity |
| cObjBaseEntity | `0x19c` | `CreatecObjBaseEntity` Common `0x7e40` cdecl | entity |
| cObjScriptedEntity | `0x1d8` | `CreatecObjScriptedEntity` Common `0x16f0` cdecl | base entity |
| cObjCharacter | `0x240` | `CreatecObjCharacter` Common `0x35d0` cdecl | scripted. Script name `cScrCharacter` |
| cObjHuman | `600` (`0x258`) | `CreatecObjHuman` Common `0x3fd0` cdecl | character. Script name `cScrHuman` |
| cObjCharacterEx | `0x390` | `CreatecObjCharacterEx` Common `0x2a7e0` cdecl | scripted. No script name in this ctor |
| cObjHumanEx | `0x3c8` | `CreatecObjHumanEx` Common `0x2a9a0` cdecl | CharacterEx. Script name `cScrHumanEx` |
| cObjHuntAreaAIEntity | `0x1b8` | `CreatecObjHuntAreaAIEntity` Common `0x13550` cdecl | base entity |
| cObjWorld | `0x210` | `CreatecObjWorld` Common `0x67b40` cdecl | scripted. Spawn writes name `world` |
| cScrAnimal | `0x71c` | Game `FUN_1001dec0` RVA `0x1dec0` | `cObjCharacterEx::cObjCharacterEx`, vtable `PTR_FUN_100ff470`, script `cScrAnimal` |
| cScrPlayer | `0x4e8` | Game `FUN_10067270` RVA `0x67270` | `cObjHumanEx::cObjHumanEx`, vtable `PTR_FUN_1010a2e0`, script `cScrPlayer` |
| cScr_ModHunter | `0x838` | Game `FUN_100521a0` RVA `0x521a0` | calls `FUN_100655e0` (cScrPlayer ctor), vtable `PTR_FUN_10106ff0`, script `cScr_ModHunter` |
| cAIHuntBrain_Animal | `0x290` | `CreateModel` Common `0x912a0` cdecl | brain ctor `0x87f50` |
| cAIBrain_Human | `0x78` | `CreateModel` Common `0x7b6f0` cdecl | `Init` calls `cAIBrain::Init(cObjCharacter*)` |

Game vtable addresses are `PTR_FUN_10xxxxxx` with image base `0x10000000`, so the RVAs above are `address - 0x10000000`. Slot contents are probe `vt:` output in `re/notes/objmodel-probe-gap-game.txt`.

`cScrEntity::Hurt` calls entity vfunc `+0x14c`. Those three slots:

| Class | Vtable RVA | Slot `+0x14c` | What it is |
|---|---|---|---|
| cScrAnimal | `0xff470` | `FUN_100194b0` RVA `0x194b0` | empty `return;` (`objmodel-decomp-game-animal.c`) |
| cScrPlayer | `0x10a2e0` | `LAB_100ec9e0` RVA `0xec9e0` | body not decompiled. `UNVERIFIED` |
| cScr_ModHunter | `0x106ff0` | `FUN_100530c0` RVA `0x530c0` | writes profile attribute 9 |

Animal slot `+0x154` is `FUN_1001e8b0` RVA `0x1e8b0`, the death presentation (health `0`, props `| 0x480`, `+0x600 = 1`, brain vfunc `+0x94(1)`). Player and hunter slot `+0x154` are both `LAB_100ec9da`. Whether brain vfunc `+0x94` is `Die` was not checked by dumping that brain slot. `UNVERIFIED`.

`cObjHuman` size `600` is the decimal malloc in `CreatecObjHuman` (`objmodel-decomp-common-ent.c`).

## Struct drafts

C++ drafts with padding and `static_assert` live in `mod/src/game/types_draft.h`. This section is the evidence.

### cObjEntity (0x194)

| Offset | Field | Evidence |
|---|---|---|
| `0x00` | vptr | every ctor |
| `0x04` | int16 id | `GetObjId` and `GetObjIdx` both return `(int)*(short*)(this+4)`. `IsValid` is `id >= 0`. `AddEntity` writes this short. Same slot is the index |
| `0x08` | uint16 rflags | `SetRFlags` ORs a ushort at `this+8`. Animal ctor ORs `0x10` into byte 8. CharacterEx ctor ORs `0x10` |
| `0x0A` | uint16 model index | `SetModel` writes the ushort from manager vfunc `+0x34`. On failure stores `0x7ff` |
| `0x0C`, `0x18` | state pos, state angle | ClearState (Engine RVA `0xb7510`) writes nullVect at state+8 and state+0x14. Role name `UNVERIFIED` |
| `0xA4` | int16 parent entity id | later entity methods compare `*(short*)(this+0xa4)` to `-1` (unattached) |
| `0xB4` | `cObjManager*` | `GetObjMgr`; `AddEntity` stores `this` |
| `0xBC` | uint32 flags | `SetFlags` / `GetFlags`. CharacterEx ctor ORs `0x10` |
| `0xC0` | uint16 props | `IsDead`: `(*(ushort*)(this+0xc0) & 0x400) == 0x400`. `IsVisible`: byte `+0xc0` bit 0 set means hidden. `SetModel` sets bits in byte `+0xC1` (the high byte) when the model kind is 9. `HaveValidTarget` treats byte `+0xC1` bit 2 as an invalid target |
| `0xC4` | tAABB, 6 floats | `SetModel` calls model vfunc `+0xcc` into `this+0xc4` |
| `0xE0` | `char*` model name | `SetModel` strdup |
| `0xE4` | `cModel*` | `SetModel`. `cObjEntity::Update` calls model vfunc `+0x8c(dt)` |
| `0xE8` | tVect3 world position | `GetEyePos` returns `this+0xe8/+0xec/+0xf0`. `SetModel` passes `this+0xe8` to model vfunc `+0x3c` |
| `0xF4` | tVect3 angle | `SetModel` passes `this+0xf4` to model vfunc `+0x44` |
| `0x100` | previous pos and angle | gap of `0x18` bytes before the phys pointer. Exact stores are inside `SetPos` / `SetAngle`. `UNVERIFIED` split in this pass |
| `0x118` | phys object | `DestroyPhysModel` frees it. `LinkWithPhysics` writes `phys+0x14 = entity` when `*(phys+8)==0`. Animal Update calls phys vfunc `+0x14` or `+0x10` |
| `0x18C` | light index array | `SetModel` malloc/free when model kind is 1 |

There is no velocity field on the entity. `ClearState` zeros `0x2c` dwords starting at `this+4` and does not name one. Motion lives on the phys object at `+0x118`. Which phys offset is linear velocity is `UNVERIFIED`.

`cObjEntity::Update` (Engine RVA `0xbc000`, thiscall, float dt): if `model != 0`, model vfunc `+0x8c(dt)`, then `UpdatePos`, then `UpdateAngle`. That is the animation clock. `Spawn` (`0xb7f50`) and `ClientSpawn` (`0xabc90`) are empty on this class.

`Destroy` (`0xaced0`): `FUN_10095ee0(this)` then `(*vptr)(1)`.

### cObjBaseEntity (0x19c)

Ctor calls `cObjEntity`. Bytes `+0x194` and `+0x198` are the two target **name strings** (`SetTarget` / `SetTargetName` / `GetTarget` / `GetTargetName`). They are `char*`, not entity pointers. Which slot is which was not re-read. The entity owner pointer is `cObjScriptedEntity::parent` at `+0x1C4`.

### cObjScriptedEntity (0x1d8)

| Offset | Evidence |
|---|---|
| `0x19C` float health | ctor stores `0x42c80000` (100.0f). `GetHealth` returns it |
| `0x1A4` `char*` script class | `SetScriptClassName` |
| `0x1B4` int script index | ctor `-1`. `Update` calls `g_pClassManager` vfunc `+0x7c(idx, dt)` when it is not `-1` |
| `0x1BC` int usable | `IsUsable` is nonzero |
| `0x1C4` parent entity | `GetParent` |
| `0x1CC` tVect3 | ctor nullVect. Role `UNVERIFIED` |

`IsPlayer` on this class returns 0. `GetActiveCamera` ignores `this` and returns the global `m_pActiveCamera` (`cSafePtr<cObjScriptedEntity>`, export RVA `0x1adfe0`).

`Update` (Common `0x3bce0`): `cObjEntity::Update(dt)`, then the script tick, then if byte `+0xC1` has bit `0x40` and a model is playing, `ActivatePhysObjectsInLeaf` for leaves over the AABB.

### cObjCharacter (0x240) and team

`GetBrain` is `*(cAIBrain**)(this+0x210)`. `SetTeam` / `GetTeam` are the int at `+0x214`.

`charFlags` at `+0x1F8`: bit 0 is `IsInputLocked`. `Update` calls brain vfunc `+0x1c(dt)` only when bit `0x10` is clear, and calls `cObjScriptedEntity::Update` only when bit `0x20` is clear. This class already has a per-entity brain freeze. CharacterEx does not.

### cObjCharacterEx (0x390)

`GetProfile` is `+0x1F8`. `GetBrain` is `*(cAIHuntBrainEx**)(this+0x200)`.

`Update` (Common `0x1ac90`) always calls brain vfunc `+0x1c(dt)` when the pointer is nonzero, then always calls `cObjScriptedEntity::Update`. There is no brain-frozen test here. `exFlags` at `+0x204` bit 0 gates a pathfinding blocker id at `+0x208`. `lastPos` at `+0x330` is what the animal bush update diffs against world pos. `+0x368 == 0` selects phys vfunc `+0x14` in the animal Update; any other value selects phys vfunc `+0x10`.

Ctor ORs props `0x840`, rflags `0x10`, flags `0x10`, and writes `1.0f` at decimal offset 500 (`+0x1F4`).

### cObjHumanEx (0x3c8)

`IsPlayer` is `(humanFlags >> 3) & 1` with `humanFlags` at `+0x3B4` (`SetHumanFlags` ORs). The ctor sets the flags to 0. `cScrPlayer`'s ctor does not set bit 3 either. Where the player bit gets set is `UNVERIFIED`.

`GetAttr` / `SetAttr` index `*(float**)(this+0x3BC)`. Energy is not an entity field. `GetBoneHead` is `+0x3B8`. `SetCurrentItem` is the int at `+0x3C0`. `GetHeightCrouched` is the float at `+0x390` (ctor writes `0.1f` at `+0x390`, `+0x394`, `+0x398`).

`Update` (`0x1d9a0`): if byte `+0xC1` bit 2 is set, it calls `cObjCharacterEx::Update` and returns. Otherwise it resizes the phys mover, then calls `cObjCharacterEx::Update`.

### cScrPlayer (0x4e8) and cScr_ModHunter (0x838)

`FUN_100655e0` calls `cObjHumanEx::cObjHumanEx`, sets vtable RVA `0x10a2e0`, script `cScrPlayer`, ORs `0x44` into the props byte, and writes `+0x4C8=1`, `+0x4CC=-1`, `+0x4D0=-1`, zeros `+0x4D4..+0x4E0`, byte `+0x3C8=0`.

`Update` `FUN_10066510` RVA `0x66510` calls `cObjHumanEx::Update`. `Spawn` `FUN_10065cb0` RVA `0x65cb0` calls `cObjHumanEx::Spawn`, then vfunc `+0x68` on the byte string at `+0x3C8`. If `model==0` it logs `Src\Game\Objects\Characters\MOD_ObjPlayer.cpp` "Could not set default model for player!".

`FUN_1004d8e0` calls `FUN_100655e0` first, then sets vtable RVA `0x106ff0` and script `cScr_ModHunter`. It constructs `cCFXDustChunk` at `+0x7A8` and `+0x7F0`. `Update` is `FUN_100522f0` RVA `0x522f0`. That function's `cBspTree::RayHit` is a downward ground probe (flags `0x400`), used for a footstep, not a bullet.

Hunter health is `cProfile` attribute 9, profile pointer at `+0x1F8`. `GetHUDHealth` and `SetAttr(..., 9, ...)`. `m_bGodMode` (Common export RVA `0x1ad98c`) gates the writes in `FUN_100530c0` and in `FUN_100522f0`. Attribute 5 is written next to attribute 9 in `FUN_10047400`. `cProfile::GetHUDStamina` exists as an export. That 5 is stamina is `UNVERIFIED`.

### cScrAnimal (0x71c)

Ctor `FUN_10018c80`: `cObjCharacterEx` ctor, vtable RVA `0xff470`, script `cScrAnimal`, props byte OR `0x40`, rflags byte OR `0x10`, `+0x4B0 = 0`.

`Update` `FUN_1001f3c0` RVA `0x1f3c0`, thiscall(float):

1. vfunc `+0x188`, then `cBspTree::UpdateBushes` using `lastPos` at `+0x330` versus `pos` at `+0xE8`.
2. If `+0x4B0 == 0` or `model == 0` or `phys == 0`, return. This return is before the phys step and before `cObjCharacterEx::Update`, so it skips animation and the phys step.
3. Otherwise step phys (`+0x14` if `+0x368==0`, else `+0x10`), then `cObjCharacterEx::Update(dt)` which ticks the brain and then the model.
4. Health float `+0x19C` drives the death timer at `+0x6F4` / `+0x450`.

`LinkWithPhysics` override `FUN_10012a30` calls the scripted version and sets `phys+0x84 = 0x200`.

### cAIHuntBrain_Animal (0x290)

| Offset | Evidence |
|---|---|
| `0x68` | `cAIBrain::Init` / `InitLoadEntity` store the `cObjCharacter*`. Animal Update reads the model through it |
| `0x71` | `Die` sets 1. Update, once past the freeze check, calls `ProcessStates` and returns when this is set and state is not `0x1000` |
| `0x79` | `FreezeBrain(int)` writes the low byte. `Update` returns immediately when it equals 1 |
| `0x84` | AI flags. Panic bit `0x40`. `Die` sets `0x4000` |
| `0xA0` | AI state. `Die` sets `0x2000` |
| `0x254` | target entity id, `-1` if none. `GetTargetIdx` / `SetTargetIdx` / `Die` |
| `0x25C` | `cObjEntity*` threat. Panic sense path calls `SetTarget` on it |
| `0x28C` | animal Update stores callback `FUN_1001e7b0` when the slot is 0. `0x28C+4 == 0x290`, so it is the last dword |

`GetTarget` calls `(*DAT_101ad8d0)(targetIdx)`. That function pointer is an entity lookup. `HaveValidTarget` (RVA `0x15ce0`) drops the id when the entity's byte `+0xC1` has bit 2.

### cAIBrain_Human (0x78)

`Init` (RVA `0x7c2c0`) calls `cAIBrain::Init(cObjCharacter*)`, then registers tasks `Task_Follow`, `Task_Strafe`, `Task_GoTo`, `Task_LookAt`, `Task_FireAt`, `Task_GoToFacing`, and a `Sense_Sight` created with `(0x42f00000, 0x42480000)` which is `(80.0f, 50.0f)`.

`Update` (RVA `0x7b830`) calls vfunc `+0xc(dt)` every time. It calls vfunc `+0x10(dt)` only when the accumulator at `+0x70` has reached the limit at `+0x74`, or the limit is negative. It does not call entity `Update`. The parameter type is `cObjCharacter*`, the older class. A `cObjCharacterEx` / `cScrPlayer` is a different layout (`brain` at `+0x200` versus `+0x210`).

### cAIHuntAnimalInterface

Getters in `objmodel-decomp-common-ai.c` around RVA `0x13be0`:

| Offset | Getter |
|---|---|
| `0x20` int | `GetGameDiff` |
| `0x24` float | `GetTimeMultiplier` / `SetTimeMultiplier` |
| `0x28` int | `GetCurrGameTime` |
| `0x2C` int | `GetSeason` |
| `0x30` float | `GetRainIntensity` |
| `0x34` byte | `GetMaxAttackersNo` |
| `0x38` float | `GetPostAttackTime` |
| `0x3C` uint | `GetAreaPointFlagsToIgnore` |
| `0x40` ptr | `LastFilteredAreaList` |

The global used by animal code is `DAT_101aea7c` in CommonDll7 (preferred base `0x10000000`, so RVA `0x1aea7c` if that base holds at the Ghidra load). Object size `UNVERIFIED`.

### cObjWorld (0x210) and cObjHuntAreaAIEntity (0x1b8)

World ctor RVA `0x67250` zeros `+0x1D8..+0x1F0`, sets `+0x1F4 = 0xff0000`, `+0x1FC = 50.0f` (`0x42480000`), `+0x20C = 1.0f`. Spawn RVA `0x674c0` writes the name `world` then `cObjScriptedEntity::Spawn`. Hunt-area fields past `0x19C` are ctor zeros. Names `UNVERIFIED`.

## Managers

### Engine cObjManager

Ctor RVA `0xbcd40`, thiscall. Calls `cClass::cClass`, then:

| Offset | Value |
|---|---|
| `0x04` | entity pointer array, `SafeMalloc(0x1000)` = `0x400 * 4` |
| `0x08` | count, 0 |
| `0x0C` | capacity `0x400` |
| `0x14` | temp array, same size |
| `0x18` | temp count |
| `0x1C` | temp capacity `0x400` |
| `0x20` | phys manager pointer, set to 0 |

`GetEntityCount` reads `+0x08`. `Get(i)` returns `entities[i]` when `0 <= i < count`, else null. `Get2` does not check. `AddEntity(cObjEntity*)` RVA `0xbd4b0`: writes `id = index` at entity `+4`, `mgr` at entity `+0xB4`, appends while `count < capacity`, otherwise reuses a null hole. `RemoveEntity(int, int)` RVA `0xbd570`: if the second arg is 0, sets the id short to `0xffff`, clears `mgr`, nulls the slot. If nonzero, calls entity vfunc `+0x24` (Destroy) and nulls the slot. Count is not compacted.

Iteration from a hook: `for (i = 0; i < GetEntityCount(); ++i) { e = Get(i); if (e) ... }`. Null slots are normal.

`CreateEntity(char const*, int)` RVA `0xbd370` cdecl: `GetClassIdByName` walks `m_nRepCount` and `m_pReplicators[i][2]` (class name string) and returns `m_pReplicators[i][3]` (class id). Unknown name logs `CreateObject: Unknown entity type '%s'` and still calls `CreateEntity(id, flags)` with a negative id, which logs `CreateObject: Unknown class id %d` and returns null. The int factory (RVA `0xbd020`) calls `FUN_100b73f0` for a valid id. That factory body was not re-read. The second integer is unused in the visible code.

`IsServer` returns 1 and `IsClient` returns 0 on the Engine base. There is no live dedicated-server split in these functions.

Vtable `??_7cObjManager@@6B@` RVA `0x4128a8`.

### Live manager is a Main7 subclass

`Main7.exe` `FUN_0041b390` RVA `0x1b390` calls `cObjManager::cObjManager` and then replaces the vptr with `PTR_FUN_0042ba58`. It constructs `cByteStream` objects at `+0x1E6C`, `+0x1EAC`, `+0x1EEC`, `+0x1F54`, `+0x1F94`, writes `+0x1FD4 = 4`, and assigns `+0x20 = 0` again after the base ctor. The Engine phys-manager slot is not a safe phys pointer on the live object until something later stores one. The ctor also zeros `0x6dd` dwords from `+0x30`.

No global symbol holding this object showed up in the dumps. The reliable path is `entity->mgr` at `+0xB4` once any entity is known. `GetActiveCamera()->mgr` is the same idea when a camera entity exists.

`FUN_0041deb0` RVA `0x1deb0` (thiscall, int) is a spawn helper on that subclass, not a get-by-id. When the arg is 0 it creates one entity through vfunc `+0x94`, `AddEntity`, `SetPos`, `SetAngle`, and `cBspTree::UpdateEntity`. It is not proven to be the player spawn.

`FUN_0041bcd0` RVA `0x1bcd0` is a frame tick (bsp `UseServer`, stream vfunc, dt accumulators, a state machine). It was not shown iterating the entity array.

### Camera

Two cameras:

- `cObjScriptedEntity::GetActiveCamera` RVA `0x7fb0` returns global `m_pActiveCamera` (export RVA `0x1adfe0`). A `cObjScriptedEntity*`.
- `cRender::GetCamera` Engine RVA `0xc470` returns a `cRenderCamera`. The render global is separate.

Nothing in these functions shows they are the same object.

### Local player

No exported "local player" pointer was found. `cScrAnimal::Update`, when health is 0 and `+0x404 != 0`, calls `FUN_10006c30()` and treats the result like an entity (position at `+0xe8`). That it is the local player is `UNVERIFIED`. `cObjHumanEx::IsPlayer` is flag bit 3 and is not set by the player ctor.

## Function table

Calling convention is `__thiscall` (this in ECX) unless the row says cdecl. Game ctors are printed `__fastcall` by Ghidra; the this pointer is still in ECX, which is the MSVC ctor convention.

| What | Symbol | RVA | Module | Notes |
|---|---|---|---|---|
| Spawn (base) | `cObjEntity::Spawn` | `0xb7f50` | Engine | empty virtual |
| ClientSpawn (base) | `cObjEntity::ClientSpawn` | `0xabc90` | Engine | empty virtual. GameDll imports it |
| Spawn human | `cObjHumanEx::Spawn` | `0x1cba0` | Common | calls three setup vfuncs, then `cObjCharacterEx::Spawn`, then vfunc `+0x19c`. GameDll imports it |
| Spawn character ex | `cObjCharacterEx::Spawn` | `0x1aa60` | Common | `cObjScriptedEntity::Spawn` then vfunc `+0x1ac` |
| Spawn player | `FUN_10065cb0` | `0x65cb0` | Game | `cObjHumanEx::Spawn` then set model from `+0x3C8` |
| Spawn animal | `FUN_1001e170` area calls `cObjCharacterEx::Spawn` | `0x1e170` calls into `0x1aa60` | Game | see animal dump line 565 |
| Create by name | `cObjManager::CreateEntity(char const*, int)` | `0xbd370` cdecl | Engine | replicator name lookup. GameDll and Main7 import it |
| Create by id | `cObjManager::CreateEntity(int, int)` | `0xbd020` cdecl | Engine | unknown id returns null |
| Class id | `cObjManager::GetClassIdByName` | `0xbd280` cdecl | Engine | `-1` if missing |
| Add | `cObjManager::AddEntity(cObjEntity*)` | `0xbd4b0` | Engine | assigns id and mgr |
| Destroy | `cObjEntity::Destroy` | `0xaced0` | Engine | then deleting dtor |
| Remove | `cObjManager::RemoveEntity(int, int)` | `0xbd570` | Engine | second arg 0 unlinks; nonzero calls vfunc `+0x24` |
| Damage (script) | `cScrEntity::Hurt(int, int, int)` | `0x3fbc0` | Common | see below. GameDll imports it |
| Damage (animal float) | `cAIHuntAnimalInterface::FuncInflictDamage(void*, float)` | `0x9a540` cdecl | Common | `entity->health -= amount` at `+0x19C`. No god-mode test |
| Damage (animal slot) | `FUN_100194b0` | `0x194b0` | Game | cScrAnimal vtable `+0x14c`. Empty `return;` |
| Damage (hunter slot) | `FUN_100530c0` | `0x530c0` | Game | cScr_ModHunter vtable `+0x14c`. Writes profile attr 9. God mode skips it |
| Damage (player slot) | `LAB_100ec9e0` | `0xec9e0` | Game | cScrPlayer vtable `+0x14c`. Body unread. `UNVERIFIED` |
| Animal death presentation | `FUN_1001e8b0` | `0x1e8b0` | Game | cScrAnimal vtable `+0x154`. Zeros health, sets dead props, calls brain vfunc `+0x94(1)` |
| NPC bullet raycast | `FUN_10056da0` | `0x56da0` | Game | only ref of string RVA `0x107ccc`. Not the player rifle |
| Animal hurt reaction | `cAIHuntBrain_Animal::OnHurt(cObjEntity*)` | `0x83ec0` | Common | retargets; does not subtract health in the body that was read |
| Death | `cAIHuntBrain_Animal::Die(int)` | `0x8c9b0` | Common | sets dead, clears target, state `0x2000`, restarts the current anim if body commands overlap `0x7000` |
| Hit test | `cBspTree::RayHit` two overloads | `0x2f9ab0`, `0x2fa300` | Engine | animal LOS uses flags `0xc0000` (`FUN_1001aa40`). Hunter ground probe uses `0x400` |
| Hit test | `cObjEntity::RayHit` two overloads | `0xb9750`, `0xb9940` | Engine | per-entity |
| AI tick animal | `cAIHuntBrain_Animal::Update(float)` | `0x95530` | Common | see key question |
| AI tick ex | `cAIHuntBrainEx::Update(float)` | `0x97790` | Common | vfunc `+0xc` then `+0x10` (senses, tasks) |
| Senses | `cAIHuntBrainEx::UpdateSenses` | `0x98700` | Common | skipped entirely when `g_bDisableThinking != 0` (export RVA `0x1aea54`) |
| Tasks | `cAIHuntBrainEx::UpdateTasks` | `0x98840` | Common | skipped when `g_bDisableActing != 0` (export RVA `0x1aea58`). Animal Update also checks that global before vfunc `+0x10` |
| Body / anim state | `cAIHuntBrain_Animal::UpdateBodyCommands` | `0x8a3a0` | Common | calls model vfunc `+0xc0` with a command struct. Does not advance the anim clock |
| Path | `cAIHuntBrain_Animal::UpdatePathFinding` | `0x89e70` | Common | talks to the bsp pathfinder. No write to entity `+0x118` in this function |
| Freeze | `cAIHuntBrain_Animal::FreezeBrain(int)` | `0x158a0` | Common | byte `+0x79` |
| Target | `SetTarget(cObjEntity*, float)` | `0x80ce0` | Common | writes id at `+0x254` |
| Target id | `SetTargetIdx` / `GetTargetIdx` | `0x15d90` / `0x15d80` | Common | |
| See | `cAISense_See::Update` | `0xd3880` | Common | does not call `SetTarget`. See below |
| Sense base | `cAIHuntSenseEx::Update` | `0x11960` | Common | empty virtual |
| Decide | `DecideAction` | `0x80e70` | Common | calls `SetTarget` on the chosen attack target |
| Process senses | `ProcessSensesData` | `0x899d0` | Common | walks sense records at brain `+0x130` (entity ptr at record `+0x0c`) and calls `DecideAction` |
| Entity tick | `cObjEntity::Update(float)` | `0xbc000` | Engine | model `+0x8c`, `UpdatePos`, `UpdateAngle` |
| CharacterEx tick | `cObjCharacterEx::Update(float)` | `0x1ac90` | Common | brain then scripted Update |
| HumanEx tick | `cObjHumanEx::Update(float)` | `0x1d9a0` | Common | phys height, then CharacterEx |
| Animal tick | `FUN_1001f3c0` | `0x1f3c0` | Game | phys step, then CharacterEx::Update |
| Player tick | `FUN_10066510` | `0x66510` | Game | HumanEx::Update |
| Hunter tick | `FUN_100522f0` | `0x522f0` | Game | bushes, god-mode gate, profile regen. Does not call HumanEx::Update in the body that was read |
| Time scale | `SetTimeMultiplier(float)` | `0x13bf0` | Common | writes `this+0x24` |
| Position | `cObjEntity::SetPos` | `0xbc130` | Engine | |
| Angle | `cObjEntity::SetAngle` | `0xbc460` | Engine | export; body not re-quoted |
| Model | `cObjEntity::SetModel` | `0xb83c0` | Engine | |

### cScrEntity::Hurt

```
void __thiscall cScrEntity::Hurt(cScrEntity* this, int attackerScrIdx, int amount, int kind);
```

RVA `0x3fbc0`. If `attackerScrIdx != -1`, it loads that class-manager record and, when `record+8` is nonzero, calls

`entity->vfunc[+0x14c]( attackerScrEntity+0x28, (float)amount, kind, nullVect, nullVect )`

where `entity` is `*(this+0x28)`. Otherwise it calls the same virtual with a null attacker. Hurt does not write `+0x19C` itself. `FuncInflictDamage` does. On a cScrAnimal the slot is the empty `FUN_100194b0`, so Hurt does nothing to an animal. On a cScr_ModHunter the slot is `FUN_100530c0`, which writes profile attribute 9. On a cScrPlayer the slot is `LAB_100ec9e0`; that body was not decompiled.

`SpawnEntity(char*, char*)` RVA `0x40000` calls entity vfunc `+0x16c` and returns that object's script index (`+0x1B4`), or `-1`.

### How an animal picks the player

1. `cAISense_See::Update` walks `cAIBrainCharacter::m_pArrBeSeen`. The owner entity is `*(brain+0x68)` via sense `+4`. For each candidate, owner vfunc `+0x1cc(candidate)` must return `0x66`. Distance squared must be `<= sense+0x18`. Dot with the view direction must be `>= sense+0x14`. Then `cBspTree::EntSeeEnt`. On success it calls brain vfunc `+0x5c(0, candidate, 0x143ab3)`. It does not call `SetTarget`.
2. `cAIHuntBrain_Animal::UpdateSenses` calls `cAIHuntBrainEx::UpdateSenses` unless AI flags bit `0x40` is set. The panic path forces task name `AIHuntTask_PanicR` and `SetTarget(threat at +0x25C, 1.0)`.
3. `ProcessSensesData` walks records at brain `+0x130`. `DecideAction` scores attack targets from `cAIHuntAnimalInterface::GetAttackTargetAt` and calls `SetTarget`.
4. `HaveValidTarget` rejects an entity whose byte `+0xC1` has bit 2, and clears the id.

`cAIHuntSenseEx::Update` is an empty override. The sight work is `cAISense_See`.

### Weapon fire

There is no exported `Fire` on the player. `cMOD_WeaponDef` (GameDll) is weapon data: damage, spread, impulse, ammo. `cAITask_FireAt` is the NPC task that `cAIBrain_Human::Init` registers. `cObjHuman::HearWeaponFire` returns 0.

The string `NPC bullet hit another entity %s \n` is GameDll RVA `0x107ccc`. The probe `str:` and `deccallers:` list one reference: `FUN_10056da0` RVA `0x56da0`, `__thiscall(void* this, float* param_1, float* param_2, undefined4 param_3, undefined4* param_4, float* param_5, undefined4* param_6)`. `param_1` and `param_2` are the segment ends (the delta is `param_2 - param_1`).

`*param_6` selects the case. Cases 1, 3, and 4 clear prop byte bit `0x40`, then `cBspTree::RayHit(g_bsp, origin, delta, *param_5, hitInfo, 0xc0000)`. If that hits and the hit type (`local_124`) is `0x40`, it logs the string with `local_e8[0x38]` (byte offset `0xE0`, the model name) and calls the hit entity's vfunc `+0x94`. A nonzero return sets prop bit `0x40` and returns 1. Case 2 normalizes the delta and calls model vfunc `+0x104` on `this+0xE4`; a hit writes `param_4[0] = 0x40`, `param_4[0xf] = this`, and the origin into `param_4[1..3]`, then returns 1. No hit returns 0.

That is the NPC bullet raycast. The player rifle fire function is still unnamed. `FUN_10046da0` and the RayHit inside `FUN_100522f0` are ground probes for dust and footsteps (flags `0x400`).

### Key question: can one animal's AI tick be skipped without breaking animation or physics?

Yes. Freeze that brain and leave the animal entity Update in place.

`cAIHuntBrain_Animal::Update` returns immediately when byte `+0x79 == 1`, before senses, tasks, pathfinding, stuck handling, and `UpdateBodyCommands` (the call that pushes a new anim command through model vfunc `+0xc0`). `cObjCharacterEx::Update` still calls that virtual, which returns, then calls `cObjScriptedEntity::Update` → `cObjEntity::Update`, which advances the current anim (model vfunc `+0x8c`) and runs `UpdatePos` / `UpdateAngle`.

The phys step is in `FUN_1001f3c0` **before** `cObjCharacterEx::Update`, and it does not look at `+0x79`. Freezing the brain leaves that step running.

Do not skip `FUN_1001f3c0`. Do not clear `+0x4B0`. Either one returns before the phys vfunc and before `cObjCharacterEx::Update`, which stops both the anim clock and the phys step. The game itself uses `+0x4B0 == 0` as that full cutoff (the ctor leaves it 0 until something activates the animal).

`g_bDisableThinking` and `g_bDisableActing` are process-wide. Thinking skips only the sense loop. Acting skips only `UpdateTasks`. Pathfinding and `UpdateBodyCommands` still run. They are the wrong tool for one remote animal.

`UpdatePathFinding` does not write the phys object. Whether some other brain function writes a desired velocity every tick was not found. What is verified: freezing the brain stops new body commands, and the entity Update keeps playing the anim that is already set, and the animal Update keeps stepping phys.

`Die` can restart the current anim at the moment of death by calling the model directly. Later frames still depend on entity Update.

## Hook candidates ranked for co-op

### 1. AI suppression (best: FreezeBrain)

Per animal, call `cAIHuntBrain_Animal::FreezeBrain(1)` or store `1` at brain `+0x79`. The host keeps the brain running. A remote animal stays in the world, the current anim keeps advancing, and the phys step in `FUN_1001f3c0` still runs. New locomotion and new target picks stop because those are inside the brain Update.

Nulling `cObjCharacterEx::brain` at `+0x200` also skips the brain call, and it also skips `Die` and `OnHurt` for that animal. FreezeBrain leaves the pointer valid.

### 2. Puppets (best: cScrPlayer or cObjHumanEx, brain left null)

`cObjEntity::ClientSpawn` and `cObjEntity::Spawn` are empty. The spawn that builds a human is `cObjHumanEx::Spawn` / `FUN_10065cb0`.

Recommended sequence, still `UNVERIFIED` against a live replicator table:

1. `cObjManager::CreateEntity("cScrPlayer", 0)` or `"cScrHumanEx"`. Both names are `SetScriptClassName` strings. Whether GameDll registers `cScrPlayer` into `m_pReplicators` was not shown. `cScr_ModHunter` is a poor puppet: size `0x838`, profile, weapons, dust, and `FUN_100522f0` assumes a profile at `+0x1F8`.
2. `AddEntity` so it has an id and a manager pointer.
3. `SetModel` to a human skin. `cScrPlayer::Spawn` logs and raises if the model stays null.
4. Leave `brain` at `+0x200` as null so `cObjCharacterEx::Update` does not run AI.
5. Drive `SetPos` / `SetAngle` each frame. `SetModel` already shows that `pos` and `angle` are what the model receives.

`cAIBrain_Human` is the wrong brain for this. `Init` requires a `cObjCharacter*` (brain pointer at `+0x210` on that class) and it installs `Task_FireAt` and `Sense_Sight`. Its `Update` does not tick the model. A hunt human is a `cObjHumanEx`, whose brain slot is `+0x200` and whose type is `cAIHuntBrainEx*`.

Who calls entity `Update` each frame was not traced through `FUN_0041bcd0`. If the manager does not tick a newly added entity, the mod has to call `cObjEntity::Update(dt)` itself or the anim clock will not move. `SetPos` still moves the model immediately.

### 3. Damage routing

Two health stores:

- Animals: float at entity `+0x19C`. `FuncInflictDamage(entity, amount)` subtracts it. `cScrEntity::Hurt` forwards to vfunc `+0x14c`, and on cScrAnimal that is the empty `FUN_100194b0`. Death presentation is animal vtable `+0x154`, `FUN_1001e8b0`: if `+0x600 != 0` or byte `+0xC1` mask `0x4` is set it returns; otherwise it sets props `| 0x480`, `+0x600 = 1`, health `0`, `+0x4F0 = 1`, calls brain vfunc `+0x94(1)` when `+0x200` is set, and `AddDeadAnimal`. `cAIHuntBrain_Animal::Die` is the brain-side state change (dead byte, target `-1`, state `0x2000`). That `+0x94` is `Die` is `UNVERIFIED`.
- Hunter: `cProfile` attribute 9 through the pointer at `+0x1F8`. Hunter vtable `+0x14c` is `FUN_100530c0`. It writes attribute 9 and bails out when `m_bGodMode` is set. Writing `+0x19C` on the hunter does not move the HUD.
- cScrPlayer slot `+0x14c` is `LAB_100ec9e0`. Do not assume it is `FUN_100530c0`. Body unread.

Route an animal hit through `FuncInflictDamage`, and through `FUN_1001e8b0` when it should die. Route a hunter hit through `FUN_100530c0`. `OnHurt` is a reaction (retarget), not the health write. Do not write both `+0x19C` and attribute 9 for the same pawn.

### 4. Target selection

`SetTarget` / `SetTargetIdx` write brain `+0x254`. `HaveValidTarget` clears the id when the target's byte `+0xC1` has bit 2. The next sense tick overwrites the id: `cAISense_See` reports through brain vfunc `+0x5c`, and `ProcessSensesData` / `DecideAction` call `SetTarget`. FreezeBrain stops that. The panic path uses the threat pointer at `+0x25C` instead of the id.

`cAIHuntAnimalInterface::GetAttackTargetAt` is the list DecideAction scores. Forcing one id without freezing the brain loses to the next sense pass.

### 5. Time scale / slow motion

`SetTimeMultiplier` writes float `+0x24` on the animal interface. Animal brain code multiplies that float into a few timers (`Update` accumulator at brain `+0xD0`, `ProcessPanicYellowState` at `+0x268`, and one more site near dump line 9651). It does not replace the dt passed into `FUN_1001f3c0` or `cObjEntity::Update`.

Slot B `callers:cAIHuntAnimalInterface::SetTimeMultiplier` found no function callers in CommonDll (two refs with `<no func>`). GameDll and Main7 do not import `SetTimeMultiplier` (pefile import walk). The HUD slow-motion path (`com_slowmotion`, `cHUDSlowMotion`, tracked in the loop notes) is a different system. Forcing `+0x24` to `1.0f` neutralizes the AI reads. Disabling the HUD cvar is separate work, and the co-op plan already turns slow-mo off. A direct call from the HUD into `SetTimeMultiplier` is not in this evidence.

## Open questions

- cScrPlayer vtable slot `+0x14c` is `LAB_100ec9e0` RVA `0xec9e0`. The body was not decompiled.
- The player rifle fire function is still unnamed. The NPC bullet path is `FUN_10056da0`.
- Is brain vfunc `+0x94` the export `cAIHuntBrain_Animal::Die`? `FUN_1001e8b0` calls it with `(1)`, which matches `Die(int)`, and the brain vtable slot was not dumped.
- Does GameDll register `cScrPlayer` / `cScrAnimal` / `cScr_ModHunter` in `m_pReplicators`? `CreateEntity("cScrPlayer")` depends on it.
- Where is the `cObjManager` instance stored, and does `FUN_0041bcd0` call entity `Update`?
- What does `FUN_10006c30` return? It is the only "nearby actor" pointer seen from animal death.
- Who sets `cObjHumanEx` flag bit 3 (`IsPlayer`)?
- Which offset on the phys object (`entity+0x118`) is linear velocity? `UpdatePathFinding` does not write it.
- Default value of interface `+0x24`, and the two non-function refs to `SetTimeMultiplier`.
- `cScrEntity` size and the rest of its fields past `+0x28`.
- `cAIBrain_Human` task list at `+0x18` is embedded. Its element type was not decoded.

## Proposed MEMORY facts

- `cObjEntity` is `0x194` bytes. `CreatecObjEntity` Engine RVA `0xacda0` mallocs `0x194`. Id and index are the same int16 at `+0x04` (`GetObjId` RVA `0xabcb0`). Manager pointer is `+0xB4` (`GetObjMgr` RVA `0xabca0`). World position is the tVect3 at `+0xE8` (`GetEyePos` RVA `0xb8380`). Model pointer is `+0xE4`. Phys pointer is `+0x118`.
- `cObjScriptedEntity` health is the float at `+0x19C`, ctor value `100.0f` (`GetHealth` RVA `0x1b30`). `IsDead` is props bit `0x400` at `+0xC0` (RVA `0x1af0`).
- `cObjCharacterEx::GetBrain` RVA `0x14920` reads `+0x200`. `cObjCharacter::GetBrain` RVA `0x3ae0` reads `+0x210`. They are different classes. Team is only on `cObjCharacter` at `+0x214` (`SetTeam` RVA `0x3bc0`).
- `cObjHumanEx::IsPlayer` RVA `0x15120` is bit 3 of the uint at `+0x3B4`. `GetAttr` RVA `0x15160` indexes `float*` at `+0x3BC`.
- Game animal entity is malloc `0x71c` (`FUN_1001dec0`), script name `cScrAnimal`, vtable RVA `0xff470`, parent ctor `cObjCharacterEx`. `+0x4B0 == 0` skips phys and `cObjCharacterEx::Update` inside `FUN_1001f3c0` RVA `0x1f3c0`.
- Game player is malloc `0x4e8` (`FUN_10067270`), script `cScrPlayer`, vtable RVA `0x10a2e0`. The playable hunter is malloc `0x838` (`FUN_100521a0`), script `cScr_ModHunter`, vtable RVA `0x106ff0`, and it constructs its base by calling the cScrPlayer ctor.
- `cAIHuntBrain_Animal` is malloc `0x290` (`CreateModel` RVA `0x912a0`). `FreezeBrain` RVA `0x158a0` writes byte `+0x79`. `Update` RVA `0x95530` returns immediately when that byte is 1, before pathfinding and `UpdateBodyCommands`. Target id is the int at `+0x254` (`SetTargetIdx` RVA `0x15d90`).
- `cObjEntity::Update` RVA `0xbc000` advances the model (vfunc `+0x8c`) even when the brain Update returned early. Skipping `FUN_1001f3c0` also skips the phys step.
- `cScrEntity::Hurt` RVA `0x3fbc0` forwards to entity vfunc `+0x14c` and does not write health. `FuncInflictDamage` RVA `0x9a540` does `*(float*)(entity+0x19c) -= amount`.
- cScrAnimal vtable RVA `0xff470` slot `+0x14c` is `FUN_100194b0` RVA `0x194b0`, an empty `return;` (`re/notes/objmodel-probe-gap-game.txt`, `objmodel-decomp-game-animal.c`). Slot `+0x154` is `FUN_1001e8b0` RVA `0x1e8b0`: early-out if `+0x600 != 0` or byte `+0xC1` mask `0x4`; else props `| 0x480`, `+0x600 = 1`, `*(uint32*)(this+0x19c) = 0`, brain vfunc `+0x94(1)`.
- cScr_ModHunter vtable RVA `0x106ff0` slot `+0x14c` is `FUN_100530c0` RVA `0x530c0`. cScrPlayer vtable RVA `0x10a2e0` slot `+0x14c` is `LAB_100ec9e0` RVA `0xec9e0` (body unread).
- GameDll string RVA `0x107ccc` (`NPC bullet hit another entity %s \n`) has one reference: `FUN_10056da0` RVA `0x56da0`. Cases 1/3/4 call `cBspTree::RayHit` with flags `0xc0000` and, on hit type `0x40`, the hit entity's vfunc `+0x94`.
- `cAIBrain_Human::CreateModel` mallocs `0x78` (RVA `0x7b6f0`). `Init` RVA `0x7c2c0` takes `cObjCharacter*` and registers `Task_FireAt`.
- `SetTimeMultiplier` RVA `0x13bf0` writes float `+0x24`. No function in CommonDll calls it, and GameDll/Main7 do not import it (`re/notes/objmodel-probe-gap-common.txt` callers; pefile import walk).
- `cObjManager::Get` RVA `0xaae30` indexes the pointer array at `+0x04`, count at `+0x08`, capacity `0x400`. `AddEntity` RVA `0xbd4b0`. The live manager ctor is Main7 `FUN_0041b390` RVA `0x1b390`; it overwrites vptr `PTR_FUN_0042ba58` and zeros `+0x20` after the Engine ctor.
- `GetActiveCamera` RVA `0x7fb0` returns export `m_pActiveCamera` (RVA `0x1adfe0`). `cRender::GetCamera` is Engine RVA `0xc470`.
- Vtable export RVAs (pefile, image base `0x10000000`): `??_7cObjEntity@@6B@` `0x412938`, `??_7cObjManager@@6B@` `0x4128a8`, `??_7cObjBaseEntity@@6B@` `0x11fba0`, `??_7cObjScriptedEntity@@6B@` `0x1206c0`, `??_7cScrEntity@@6B@` `0x11fd08`, `??_7cObjCharacter@@6B@` `0x120c20`, `??_7cObjCharacterEx@@6B@` `0x123628`, `??_7cObjHuman@@6B@` `0x120df0`, `??_7cObjHumanEx@@6B@` `0x1242a8`, `??_7cObjHuntAreaAIEntity@@6B@` `0x122028`, `??_7cObjWorld@@6B@` `0x12d428`, `??_7cAIHuntBrain_Animal@@6B@` `0x123888`, `??_7cAIHuntBrainEx@@6B@` `0x1233e0`, `??_7cAIBrain_Human@@6B@` `0x12e738`, `??_7cAIHuntAnimalInterface@@6B@` `0x123618`.
