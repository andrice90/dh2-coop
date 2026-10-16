# Verified facts — engine and decompilation learnings

Format: fact — evidence. Add only verified items; hypotheses go in `re/notes/`. Mod/build/tooling learnings go in `docs/mod-learnings.md`; current progress in `STATUS.md`.

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
- Exports: EngineDll7 8587 entries / 8192 distinct names (395 entries have no name; 571 classes), CommonDll7 3174 (199), GameDll7 133 (12) — PE export directory, `re/exports/*.txt`; demangled in `re/exports/*.demangled.txt`.
- Multiplayer surface exists in retail: Main7.exe strings `sv_multiplayer`, `sv_port`, `sv_password`, `sv_fps`, `sv_changeMap`, `cl_join %s`, `cl_playerName`, `cl_password`, `cl_disconnect`, `cfLobby`, switch `-join`; EngineDll7 `-dedicated` and debug strings for `cLocalDriver::*` and `cRdpDriver::*` (Listen, Connect, RequestServerList, GetServerInfo, RxTx); GameDll7 `-spmap` — `re/notes/strings/*.txt`. Whether they work end-to-end: unverified.
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

## Net layer (Track A, `re/notes/net-layer.md`; reviewed 2026-10-16)
- Single player is NOT client/server over a socket or loopback. The "server" is `cSpServer` inside Main7.exe (ctor RVA `0x1b390`, `StartGame` `0x1ed80`, `StartListen` `0x1ba40`, `ClJoin` `0x1e410`, `ClientJoinGame` `0x1e0c0`): one in-process object that loads the map, spawns the hunter via GameDll (`MOD_SvCreatePlayerEntity`/`MOD_SvSpawnPlayerEntity`) and passes bytes between its own `cByteStream`s. `[SERVER] loading time ... client: N` measures `ClJoin`, not a network client.
- `cSpServer` state at `+0x28`: 0 NotStarted, 1 Listening, 2 GameStarting, 3 GameStarted (table Main7 RVA `0x3bae0`). Client-info state `+0x1fd4`: 0 Joining … 3 GameStarted, ctor writes 4 (table `0x3bb04`). `Update` (`0x1bcd0`) ticks the world only in state 3.
- `cNetServer`/`cNetClient`/`cObjManager::SetDriverType` (`0xaad50`) are exported by EngineDll7 but imported by no other binary and called nowhere; `m_nDriverType` (RVA `0x5826e0`) stays 0 — lead re-checked imports with pefile; probe shows `driverType=0`.
- No binary imports `WSAStartup` (lead re-checked). Any use of engine sockets needs our ASI to call it first.
- `cRdpDriver` (vtable `0x43e844`, unnamed class) is a working reliable-UDP transport: opcodes `0x01` connect … `0x0b` server info, reliable/unreliable payloads `0x04`/`0x09` with seq + u16 length. Payload is an opaque byte pipe; no entity replication exists anywhere (`cObjEntity::ClientSpawn` `0xabc90` and `ClientCommand` `0xb9480` are empty).
- `sv_multiplayer` (default 0) is never branched on; `StartListen` writes `"0"` into it each map load. `sv_port` default 27318 (`0x6ab6`), read only by `GetListenPort` (`0x21610`, no callers). `cl_join` has no console handler. `-join`/`-dedicated` let a second process pass the `"GI UNIQUE MUTEX"` single-instance check (`FUN_00401dd0`, Main7 `0x1dd0`); `-dedicated` also sets `r_api 0; snd_usesound 0` (no rendering).
- Runtime (Track E, 2026-10-16): no UDP endpoint is bound in baseline, `-dedicated`, `-join 127.0.0.1`, `sv_multiplayer 1` via `Cfg\console.cfg`, or `-map` runs — `re/notes/runtime-*-udp.txt`. `-map` execs a nonexistent console symbol (`[CONSOLE ] symbol 'map' not found!`).
- GameDll `MOD_SvParseMessage` (GameDll7 `0x3140`) handles only in-process message ids `0x87` and `0xff`.

## Assets (Track C, `re/notes/assets.md`; reviewed 2026-10-16)
- `data.arc`: 2048-byte sectors, 24-byte footer `ARC\0` v`0x100`, zlib directory of `ARCH` records; 9,612 entries, `arc_extract.py --verify` 0 problems. Extract = 9,710 files (+98 from `shaders.mtp`, magic `MULA`). Format spec in `re/scripts/arc_extract.py` docstring.
- File lookup order (`cFileManager::Open` EngineDll7 `0x33e230`): home dir `.\..\data\` first, then `*.ARC`, then `*.ZIP`. **Loose files under `game\Data\` override the archive** — confirmed at runtime: a loose `Data\Cfg\console.cfg` was read and rewritten by the game (`re/notes/runtime-cfg-file-after.txt`).
- `Cfg\console.cfg` syntax: `addvar <name> <type>; <name> = <value>` (loaded by `InitConsole` `0x329ec0` → `cConsole::Load` `0x1672c0`).
- Two script systems: custom VM (`ScrVMCore.cpp`; `.SCR` text source, `.SPJ` project list, `.XSC` bytecode magic `46 53 01 01`, version 3 required by `cVMachine::LoadInfo` `0x287950`) and Lua 4.0 (`.OUT` bytecode `1B 4C 75 61 40`, 98 files). `.SCRIPT/.FX/.LUA/.INC/.SND` are text.
- Maps: 29 BSPs. Free-hunt: `HUNTMAP1` Africa (missions id 13), `HUNTMAP2` Australia (14), `HUNTMAP3` Alaska (15), `HUNTMAP4` Argentina (16). Bonus: `MAPARENA` (17), `MAPISLAND` (18), `MAPTSAVO` (19). Story: `MAP1`…`MAP12C`. Each BSP's `world_properties` points to `scripts/levels/<map>/main.spj`, class `cMapTest`; hunter spawn entity `info_spawn_point`/`human`.
- Animals are placed by level script: `WORLD.SCR` → `AddAnimal(ANIMAL_<SPECIES>, TRUE, "<bsp entity name>")`; species brains/health in `SCRIPTS\MODELS\ANIMALS\`. Free-hunt maps place 4 prey + 1 predator (HUNTMAP4: 4 red deer + 1 puma; 2 spawn points).
- No multiplayer/lobby content exists in the archive (no `sv_multiplayer`, `cl_join`, `lobby`, `player2` in scripts/GUI/BSP text).

## Runtime probe (Track E, `re/notes/renderer.md`; reviewed 2026-10-16)
- Renderer: D3D8 only. `Direct3DCreate8` ×3 (callers EngineDll7 `+0x352a3e`, `+0x35211c`, `+0x358f9e`), one HAL device (behavior `0x84` = mixed VP + multithreaded). `Present` called from EngineDll7 `+0x354cfd`, `EndScene` from `+0x354b63` (vtable slots 15 / 35). `Direct3DCreate9` (`+0x35a43b`) creates no device — `runtime-baseline-probe.txt`.
- Default device: 640×480 `A8R8G8B8`, fullscreen, 240 Hz, `D3DPRESENT_INTERVAL_IMMEDIATE`. A loose `game\Data\Cfg\console.cfg` makes it **windowed** 640×480 (`r_windowed` default 1); the game rewrites that file on shutdown as an `addvar` dump. `game\Data\console.cfg` (wrong path) is ignored.
- `cCmdLine::Search` returns the argv index or -1.
- **`-spmap <MAP>` loads a map directly** (e.g. `+mod data -developper -cheats -spmap HUNTMAP1` → `cBspTree::Load('Bsp\HUNTMAP1.bsp')`, `[APP] map 'HUNTMAP1' loaded succesfully`) — `runtime-spmap-*.txt`. `-map <name>` does not work (execs missing symbol `map`).
- `-join <host>` → `Exec("cl_join <host>")` at Main7 `+0xf55e` → `[CONSOLE ] symbol 'cl_join' not found!`. `-dedicated` → no D3D device, no menu map, no sockets.
- `cConsole::Load` feeds cfg lines to `cConsole::RealExec` (not `Exec`); RealExec doesn't log unknown symbols.

## Disc image
- CloneCD set `Dngh2.ccd/.img/.sub/.cue`: track 1 MODE1/2352 data, track 2 audio — `.cue`. Mounts with Virtual CloneDrive.
