# Verified facts

Format: fact — evidence. Add only verified items; hypotheses go in `re/notes/`.

## Build identity
- Version: `DNGH2 (build 434 - Nov  1 2005)`, game/interface version 1.0 — `Bin\debug.log` lines 1, 19-20.
- Engine self-name: "Chameleon Engine" (c) 2000-2005 — `debug.log` line 2.
- Original PDB paths: `c:\2002\SRC\Game\Mod\DNGH2\Main\Out\Release_VC7\Main7.pdb`, `...\Game\EngineDll\Out\Release_VC7\EngineDll7.pdb`, `...\Game\CommonDll\Out\Release_VC7\CommonDll7.pdb` — PE debug directory. Compiler: MSVC 7.0 (msvcr70/msvcp70).

## SHA-256
| File | SHA-256 |
|---|---|
| Bin\Main7.exe | F9C1331EFCCCED45F196EC969966F2662D489EAD900AF5A64A5B40FAA7DCD71F |
| Bin\EngineDll7.dll | C36D18D74993383CA3FC7D842A9E464050C732D5AE9EB05BE898185966D8D562 |
| Bin\CommonDll7.dll | 723C29D1D69AFD2DC258093A2ACF9D06147E806F9B4241E7663AB815710DBAFE |
| Data\GameDll7.dll | 0DE1D8AFC3025F1E76186E3C5178316C5B69680E6C57FE4672D1654B82F80874 |
| Bin\binkw32.dll | DE7820F6B557C60F250146E863579C9D1A0673DC92E2110829031A5783F4245C |

## Module layout
- Load order: `InitEngineDll`, `InitCommonDll`, then `.\..\data\GAMEDLL7.DLL` — `debug.log` 4-5, 19.
- Home path `.\..\data\`; assets in `Data\data.arc` (294 MB, entries begin with zlib header `78 9C`), `shaders.mtp` multi-file (98 files) — `debug.log` 7, 17; hex dump.
- Exports: EngineDll7 8587 (571 classes), CommonDll7 3174 (199), GameDll7 133 (12) — `re/exports/*.txt`.
- EngineDll7 imports: `d3d8!Direct3DCreate8`, `d3d9!Direct3DCreate9`, `DINPUT8!DirectInput8Create`, `WS2_32` (socket/bind/sendto/recvfrom/select), `binkw32`, `MSACM32`, `WINMM` — pefile.
- No useful RTTI (only exception classes); names come from exports instead.

## Engine subsystems seen
- Net: `cNetServer`, `cNetClient`, `cNetGameState`, `cNetClientInfo` exported from EngineDll7.
- Internal client/server: `[SERVER] loading time : 156ms (map : 78ms, client: 78ms)`; Lua state `SpServer` — `debug.log` 68, 74.
- Lua: `EngLuaExecuteScript`, `EngLuaExecuteBuffer` exported; Lua states MaterialManager, resource manager, FxManager-Define, SpServer, DecalManager.
- Maps are BSP with chunked format (`CHUNK_ENTITIES`, `CHUNK_NEWPATHGRID`, ...). Menu map: `menumap`.
- AI (CommonDll7): `cAIBrain`, `cAIBrain_Human`, `cAIHuntBrain_Animal`, `cAIHuntBrainEx`, `cAIHuntSenseEx`, `cAISense_See`. World: `cObjWorld`, `cObjBaseEntity`, `cObjScriptedEntity`, `cObjHuntAreaAIEntity`.
- GameDll7: `cHunterProfile`, `cMOD_WeaponDef`, `cMOD_AmmoDef`, `cMOD_ItemDef`, `cMOD_MedDef`, `cMOD_RadioDef`.

## Launch and runtime
- Correct command line: `Bin\Main7.exe +mod data` with working dir `Bin`. Without `+mod data`, home path falls back to `.\..\DNGH2\` and assets fail to load — `Launcher.exe` string, sandbox `debug.log` 2026-10-15.
- `Launcher.exe` also contains `-cheats -developper` (sic) flags; console config `Cfg\console.cfg` is probed at startup. Effect of the flags: unverified.
- Registry key used by launcher: `SOFTWARE\Activision Value\Magic Wand\Dngh2`; Main7 also reads `Software\2002EngineTools`. Missing registry is non-fatal ("Registry error: unable to open registry !").
- `EngineDll7.dll` relocates at runtime (seen at 0x00CA0000, not 0x10000000) — FOV fix log. Use RVAs/patterns only.
- ASI route works: Ultimate ASI Loader v9.7.4 as `Bin\dinput8.dll` loads `.asi` files from `Bin\` — FOV fix v1.1 logged "loaded" and resolved `EngineDll7.dll+2fe07` (FOV) and `+2fe1d` (aspect).
- Disc image must be mounted (Virtual CloneDrive, `VCDMount.exe <ccd>`) before launch.

## Disc image
- CloneCD set `Dngh2.ccd/.img/.sub/.cue`: track 1 MODE1/2352 data, track 2 audio — `.cue`. Mounts with Virtual CloneDrive.
