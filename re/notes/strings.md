# Strings (Track F)

Source: sandbox copies in `game\` (`Bin\Main7.exe`, `Bin\EngineDll7.dll`, `Bin\CommonDll7.dll`, `Data\GameDll7.dll`). SHA-256 prefixes match the MEMORY.md table for all four.

Full dumps: `re\notes\strings\<file>.txt`. Format per line: `file-offset  rva  section  string`. Regenerate with `py re\scripts\dump_strings.py`.

| File | Strings (ASCII, len >= 5) | Section split |
|---|---:|---|
| Main7.exe | 2293 | .rdata 1318, .text 924 |
| EngineDll7.dll | 55315 | .text 24784, .rdata 16996, .reloc 13064 |
| CommonDll7.dll | 12289 | .rdata 7092, .reloc 5018 |
| GameDll7.dll | 9547 | .rdata 4846, .reloc 4498 |

Caveats:
- `.text` hits are mostly code bytes. Read `.rdata` first.
- The min-length-5 filter drops short tokens. `+mod` (4 chars) is missing from the dumps. A byte search finds it once in Main7.exe, matching the MEMORY.md launch line.
- UTF-16 strings are not extracted.
- Every string below is a literal in the binary. Behaviour is not verified. Hypotheses are marked.

## Checks against MEMORY.md

- Hashes: all four sandbox binaries match.
- `+mod`: present in Main7.exe (byte search). Consistent with `Bin\Main7.exe +mod data`.
- `Software\2002EngineTools`: present in Main7.exe at `0x2836c`. Consistent.
- `Registry error: unable to open registry !`: present in EngineDll7.dll (file offset `0x43f9b4`). Consistent.
- `[SERVER] loading time : ...` line: the format text `loading time : %dms (map : %dms, client: %dms)` is in EngineDll7.dll `.rdata`. The bracket part is not a literal (see log tags).
- **Mismatch:** MEMORY.md gives the launcher key as `SOFTWARE\Activision Value\Magic Wand\Dngh2`. That string is not in any of the four binaries. The closest is `ProgramFiles\ActivisionValue\FUNLabs\Dngh2` in Main7.exe at `0x279e0`. The MEMORY.md source may be Launcher.exe, which is not in `game\Bin`. Needs review by whoever owns MEMORY.md.
- `-cheats -developper` (MEMORY.md cites Launcher.exe): both strings are in Main7.exe. Launcher.exe is not in `game\Bin`.

## Command-line switches

Main7.exe (`.rdata`):
- `+path`, `+mod` (byte search only)
- `-join`, `-dedicated`, `-batchmode`, `-spmap`, `-cheats`, `-developper`
- `-buildai`, `-testmodels`, `-logfiles`, `-shaderdup`, `-nointro`, `-debug`, `-console`, `-AL16`, `-np1A`

Also present: `-dedicated` in EngineDll7.dll `.rdata` (`0x43de7c`), `-console98` in EngineDll7.dll (`0x43de70`), and `-spmap` in GameDll7.dll.

Effect of each switch is unverified. `-join` and `-dedicated` are the most relevant to co-op.

## Console commands and cvars

Network and session (mostly Main7.exe `.rdata`, the launcher side; `sv_restart` and `sv_stop` also in GameDll7.dll; `sv_varchaged` also in EngineDll7.dll):
- `sv_multiplayer`, `sv_port`, `sv_password`, `sv_fps`, `sv_save`, `sv_load`, `sv_dump`, `sv_restart`, `sv_changeMap`, `sv_stop`, `sv_options`, `sv_varchaged` (sic)
- `cl_playerName`, `cl_password`, `cl_disconnect`
- `r_from_server` (Main7.exe)

Save and game (not Main7):
- `g_try_save`, `g_do_autosave` (CommonDll7.dll)
- `cam_fov` (GameDll7.dll and Main7.exe)
- `mod_slowmotionFrameTime`, `mod_slowmotionFadeTime`, `mod_slowmotionBlur`, `com_slowmotion`, `slowMotion`, `Game_Pause`, `TASK_PAUSED`, `Restart`, `Reload` (GameDll7.dll)

Rendering, sound, FX (EngineDll7.dll unless noted):
- `r_*` (display, gamma, shadow, AA), `snd_*` (EAX, gain classes, debug), `fx_*` (light, sun, breath, bullet trail), `tex_mat` (all EngineDll7.dll); `in_mirror` (CommonDll7.dll); `r_eye` (GameDll7.dll)
- Camera: `cam_thirdPerson` and `cam_fov` (GameDll7.dll and Main7.exe). Other `cam_*` names (`cam_distance`, `cam_angle`, `cam_angleUp`, `cam_x_angle`) not located per binary.

Lua C API names (EngineDll7.dll): `lua_newtag`, `lua_settagmethod`, `lua_dofile`, `lua_dobuffer`, `lua_dostring`, `lua_call`, and many more. The naming (tags and tag methods) looks pre-5.0 Lua. Version not confirmed.

## File paths and extensions

- Config and logs: `Cfg\console.cfg` (Main7, Engine), `console.txt` (Main7), `debug.log` (Main7, Engine), `default_%d.log` (Engine), `ScreenShoot%d.tga` (Engine)
- Assets: `shaders.mtp` (Main7), `zmtp/%s.mtp` (CommonDll7), `Bsp\%s.bsp` (Main7), `%s%s%s.prt` (Engine), `Textures\sun\%s_smask.tga` (GameDll7), `textures/DefaultTexture.tga`, `textures/DefaultSkyTexture.tga` (Engine)
- Scripts: `ai.lua` (Main7), `Scripts/Materials.lua` (GameDll7), `scripts\game\decals.txt` (GameDll7), `fx\scripts\fx.ini`, `fx\scripts\%s.fx`, `FxClasses.ini` (Engine), `fx\hfxcfg\hfx%s.cfg` (GameDll7), `out.xml`, `%s_out.xml`, `<xml id=ancillary src="ancillary.xml">` (Main7)
- Movies (`.bik`): `Movies\Legal.bik`, `Movies\Intro.bik`, `Movies\MagicWand.bik`, `Movies\Atvi.bik` (Main7); `Movies\Demo3...` (GameDll7)
- Loaded DLLs: `EngineDll7.dll`, `CommonDll7.dll`, `GameDll7.dll`, `InterfaceDll7.dll` (Main7). Engine references `d3d8.dll`, `DINPUT8.dll`, `WS2_32.dll`, `binkw32.dll`.

Not present as strings in any of the four: `.arc`, `.wav`, `.dds`, `.mp3`, `.ogg`, `data.arc`, `data\`. Asset file names are likely built at run time (unverified).

## Log prefixes and tags

- `[PHYS] ERROR: objects out`, `[PHYS] ERROR: joints out`, `[PHYS] ERROR reading object`, `[PHYS] There was an error loading save file`, `[PHYS] Error: End loading PhysManager` (Engine)
- `[WARNING] objIndex = %d`, `[WARNING] countPforce = %d`, `[WARNING ] inertia must be positive definite`, `[WARNING ] mass must be greater than zero` (Engine)
- `[Warning]: BSP compiled with FAST VIS !` (Engine)
- `[SHADERDUPP ]` (Main7)
- `LuaState[%d] '%s' uses %d blocks (GC = %dk)` (Engine)

**`[NET]`, `[SERVER]`, `[CLIENT]` are not literal strings.** The bare words `SERVER` (Main7 x1, Engine x3) and `CLIENT` (Engine x1) are present. The surrounding brackets are probably added in code. Hypothesis, not verified.

## Network-related text

Engine, `.rdata` unless noted:
- Socket errors: `RequestServerInfo: Unable to create socket`, `RequestServerInfo: sendto failed error=%d!`, `RequestServerInfo: failed to create search socket (err=%d)!`, `Listen: bind on port %d failed` (`0x43e8a8`), `Receiver: Invalid packet type %d`, `Timeout`
- Drivers (string-only class names, no exports): `cLocalDriver::Connect`, `cRdpDriver::Connect`, `cRdpDataChannel::PingClient`
- Ping: `Pinging`, `cPingManager::PollPing`, `cPingManager::RemoveAllPings`
- Server and client: `CLIENT`, `SERVER`, `client`, `server`, `invalid client`, `Unable to create server`, `SERVER ERROR: '%s'` (Main7 `0x2b0d0`), `'Server v%d.%d'` (Main7 `0x2c500`)
- Disconnect dumps: `************** AUTO DISCONNECT %3d ******` (Main7 `0x29404`), `************** AUTO DUMP DISCONNECT %d ************`
- Lobby: `cfLobby` (Main7 `0x2c2dc`). Only lobby string found.
- Player: `player only` and `PLAYER` (CommonDll7); `Trigger which will kill the player` and `Could not set default model for player!` (GameDll7)

Batch and headless hook (Main7):
- `batch mode option '%s' %s`, `Batch mode paused on demand.`, `Batch mode resumed on demand.`, `Batch mode finished`
- `Batch mode parameters not available - implement GetBatchModeParameters in game mod!` (`0x29198`). This names a function the game mod is expected to provide. `GetBatchModeParameters` is not in any of the three export lists, so it is not an export of the stock DLLs.

## Debug, assert and source-path messages

Generic:
- `Assertion Failed` (`0x4107a8`), `ASSERTION FAILED` (`0x4107bc`) (Engine)
- `*** IF THIS HAPPENS PLEASE SEND DEBUG.LOG FILE TO DEVELOPPER ***` (Engine)
- `Invalid %s file version %x (expected %x) in file %s`, `SAVE problem: script class ID valid, but invalid definition`, `UNABLE to restore Garbage Collector on Loading !`, `[PHYS] There was an error loading save file` (Engine)
- `8All .bsp-s MUST have pathfinding !!!!!` (CommonDll7): a level-data rule
- `Collision Error: COLLMAN_NOTNUMBER` (Engine)

Source paths (`__FILE__`-style, relative and absolute). The tree is an internal `Src` / `Core` / `Render` layout. Absolute form: `c:\2002\src\game\...`.

Co-op-relevant:
- Local single-player server: `Src\Local\SpServer\SpSvEntity.cpp`, `Src\Local\SpServer\SpSvConfig.cpp`
- Script VM (custom, separate from Lua): `Core\Src\Scripter\ScrVMCore.cpp`, `ScrTableEntries.cpp`, `ScrTreeNod.cpp`
- Model replication: `Core\Src\Model\ModelReplicator.cpp`, `c:\2002\src\game\enginedll\core\inc\ModelReplicator.h`. This matches the "Replicator" name pattern as model instancing, not networking.
- Console, command line, debug: `Core\Src\Console\ConsoleVar.cpp`, `Core\Src\Io\CommandLine.cpp`, `Debug\Src\DebugSystem.cpp`, `Core\Src\Windows\Application.cpp`, `Src\Tools.cpp`, `Src\FrameWork.cpp`
- Physics: `Core\Src\Physics\PhysManager.cpp`, `PhysMan.cpp`, `CollManager.cpp`, `PhysJointCarWheel.cpp`, `PhysConstraint.cpp`
- AI (CommonDll): `Src\AI\AIHuntBrainAnimal.cpp`, `AIHuntTaskAttack.cpp`, `AIHuntSenses.cpp`, `AIHuntBrainManager.cpp`, `AIHuntGroup.cpp`, `AIHuntFeed.cpp`, `AIHuntDrink.cpp`, `AIHuntBed.cpp`, `Inc\AIHuntBrainAnimal.h`
- Objects (CommonDll): `Src\Objects\ObjCharacter.cpp`, `ObjCharacterEx.cpp`, `ObjHuntAreaAIs.cpp`, `ObjScriptedEntity.cpp`, `ObjTrain.cpp`
- Game DLL (GameDll7): `Src\Game\Objects\Characters\MOD_ObjPlayer.cpp`, `MOD_ObjHunter.cpp`, `MOD_ObjNPC.cpp`, `MOD_ObjPilot.cpp`, `Src\Game\Objects\MOD_ObjAnimal.cpp`, `Src\Game\Core\MOD_GameRules.cpp`, `Src\Game\Model\MOD_AnimalPart.cpp`. `MOD_ObjPlayer` is not in the export list. Only the source path exists as text. The class name is confirmed by the path, not by an export.
- Shared headers: `..\..\..\..\..\Src\Game\Common\tArray.h`, `.\..\Common\StaticArray.h`, `..\EngineDll\Core\Inc\ModelReplicator.h`
- Rendering backend: `Platform\Win32\Render\DX\RSDDX\RSDRigidDX.cpp`, `Platform\Win32\Render\DX\PipelineDX\PertRRDX\PertRRDX.cpp`, `Platform\Win32\Input\InputDX.cpp`

## Registry keys and paths

- `Software\2002EngineTools` (Main7, `0x2836c`). Matches MEMORY.md.
- `ProgramFiles\ActivisionValue\FUNLabs\Dngh2` (Main7, `0x279e0`). Differs from MEMORY.md (see above).
- `Software\Microsoft\Direct3D`, `SOFTWARE\Microsoft\Direct3D` (Engine)
- `Software\Microsoft\Windows\CurrentVersion\Policies\Explorer` (Engine). Not game data. Read as an Explorer policy check (unverified).
- Registry helper exports: `GetRegVal`, `GetRegValA`, `SetRegVal`, `DelRegVal` (EngineDll7 exports, `__cdecl`).
- Messages: `Registry error: unable to open registry !`, `Registry error: unable create key !`, `Registry error #%i : %s` (Engine)

## Co-op relevant findings (for the notes)

1. Launch surface: `+mod`, `-join`, `-dedicated` (Main7) and `-dedicated` (Engine). Effects not verified.
2. Netcode cvars live in Main7.exe: `sv_multiplayer`, `sv_port`, `sv_password`, `cl_playerName`, `cl_password`, `cl_disconnect`.
3. Transport classes `cLocalDriver` and `cRdpDriver` appear only as strings, with no exports. Name-based guess: LAN and RDP transports (unverified).
4. Slow motion is in GameDll7: cvars `mod_slowmotion*`, `com_slowmotion`, and a HUD class `cHUDSlowMotion` (`SetFrameTime`, `Enable`, `Disable`, `Update`). Strings only. Matters for the open slow-mo decision in PLAN.md.
5. Engine `cNetServer` and `cNetClient` debug text names `RequestServerInfo` and `Listen: bind on port` (search and listen sockets).
6. String-only class names (no export, see class-catalog.md last section): cSpServer, cGameManagerSp, cGameApp, cMODApp, cObjHunter, cObjNPC, cObjAnimal, cMOD_ObjWeapon, cGameRules, cMission, cHUDManager, cHUDSlowMotion.
7. Game player class source path `Src\Game\Objects\Characters\MOD_ObjPlayer.cpp` in GameDll7. No export by that name.
8. Local server source paths (`Src\Local\SpServer\SpSvEntity.cpp`, `SpSvConfig.cpp`) are embedded as text, so a single-player server class exists in the build (`cSpServer` string only).
9. Custom script VM (`ScrVMCore.cpp`, `cVMachine`) runs alongside Lua, not only Lua.
10. Batch mode expects a mod hook `GetBatchModeParameters` (Main7 message).
11. Save path texts: `g_try_save`, `g_do_autosave` (CommonDll7), `[PHYS] There was an error loading save file` (Engine).
12. `cfLobby` is the only lobby string.
13. MEMORY.md launcher registry key does not match the binaries (see above).
