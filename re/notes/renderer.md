# Renderer and multiplayer surface (Track E)

Runtime logs copied to `re/notes/runtime-<exp>-debug.txt` and `re/notes/runtime-<exp>-probe.txt`. UDP samples are in `runtime-<exp>-udp.txt`. Launches used `run-game.ps1` (disc image mounted, `Main7.exe +mod data` from `game\Bin`). Dates are 2026-10-16.

## Build

`mod\build.ps1 -Clean` completed with exit 0 and installed `game\Bin\dh2coop.asi` (811 KB). One third-party warning: `safetyhook` `os.windows.cpp` C4005 `NOMINMAX` redefinition. No project-source errors.

| Tool | Version |
|---|---|
| CMake | 4.1.2, Ninja 1.13.2, generator Ninja, `CMAKE_BUILD_TYPE=Release` |
| Compiler | MSVC 19.44.35226.0, toolset 14.44.35207, `Hostx86\x86\cl.exe` (VS 2022 Build Tools), Win32, static CRT `/MT` |
| Language | C++20 on `dh2coop`; safetyhook v0.7.0 requests C++23 and raises the target to `/std:c++latest` |
| safetyhook | v0.7.0, git `1922366` |
| Zydis | v4.1.0, git `569320a`; Zycore submodule `74620eef` (fetch log: v1.5.0) |
| spdlog | v1.17.0, git `79524dd` |
| Dear ImGui | v1.93.0, git `c1aaac6` (compiled, not wired to a device) |
| ENet | v1.3.18, git `2662c0d` |

## Renderer and hooks

The retail session renders with **D3D8**. `Direct3DCreate9` runs once and never creates a D3D9 device. `dgVoodooD3D8.dll` was not loaded. System `d3d8.dll` / `d3d9.dll` are the ones hooked.

Baseline (`+mod data`, 40s, pid 13400), probe `runtime-baseline-probe.txt`:

- `Direct3DCreate8(sdk=0xdc)` three times. Callers: `EngineDll7.dll+0x352a3e`, `+0x35211c` (same object as the first), `+0x358f9e` (the object passed to `CreateDevice`). SDK `0xDC` is 220.
- One `IDirect3D8::CreateDevice`: adapter 0, devtype 1 (HAL), behavior `0x84` (D3DCREATE_MIXED_VERTEXPROCESSING `0x80` plus D3DCREATE_MULTITHREADED `0x04`), hr 0.
- Params with no loose `Cfg\console.cfg`: `640x480 fmt=21 bb=1 ms=0 swap=3 windowed=0 autoDS=1 dsfmt=80 flags=0 hz=240 pi=0x80000000`. fmt 21 is `D3DFMT_A8R8G8B8`, swap 3 is `D3DSWAPEFFECT_COPY`, dsfmt 80 is `D3DFMT_D16`, `pi` `0x80000000` is `D3DPRESENT_INTERVAL_IMMEDIATE`.
- `Direct3DCreate9(sdk=0x20)` once, caller `EngineDll7.dll+0x35a43b`. `cdev9` stayed 0.
- `D3D8 Device::EndScene` caller `EngineDll7.dll+0x354b63`. `D3D8 Device::Present` caller `EngineDll7.dll+0x354cfd`. Both counters tracked each other (about 30 Hz before the menu map, about 100 Hz after). At t=20s: `present8=524 endscene8=524`.
- `cRenderDX::Init` returned 1. `cRenderDX::m_d3dd` matched the created device. `cObjManager::m_nDriverType` stayed 0 for the whole run.
- `cApplication::m_bWindowed` stayed 0 even on later runs whose CreateDevice `windowed` flag was 1. That static is not the D3D windowed flag.

ImGui hook point: `IDirect3DDevice8::Present` (vtable slot 15 in the probe), called from `EngineDll7.dll+0x354cfd`. `EndScene` is slot 35, caller `+0x354b63`. The device vtable itself sits in the device allocation, not in `d3d8.dll`. The method bodies are `d3d8.dll+0x2a1e0` (Present) and `d3d8.dll+0x4d7e0` (EndScene) on this Windows build.

### Module bases (baseline pid 13400)

Bases move between processes. RVAs below are stable across the runs in this note.

| Module | Base this run | Notes |
|---|---|---|
| Main7.exe | `0x00400000` | size `0x3F000` |
| EngineDll7.dll | `0x00DB0000` | preferred image base is `0x10000000`; the dedicated run did load there |
| CommonDll7.dll | `0x10000000` | |
| GameDll7.dll | `0x0DC70000` | loaded at +30986 ms from `game\data\GAMEDLL7.DLL` |
| dh2coop.asi | `0x67C90000` | |
| d3d8.dll | `0x69DD0000` | `C:\WINDOWS\SYSTEM32\d3d8.dll` |
| d3d9.dll | `0x69C30000` | loaded before any `Direct3DCreate9` |
| dinput8.dll (ASI loader) | `0x69780000` | `game\Bin\DINPUT8.dll` |
| binkw32.dll | `0x18000000` | |

### Hooks installed

Every probe hook the ini enables installed on the baseline. None logged a safetyhook error.

EngineDll7 RVAs: `cNetServer::Create` `0xb20d0`, `CheckNewClients` `0xb3660`, `DisconnectClient` `0xb2fb0`, `ShutdownServer` `0xb17d0`, `IgnoreRemotes` `0xb1910`, `ClientDisconnected` `0xb26d0`, `RxTx` `0xb1680`, `DispatchMessages` `0xb2260`, `cNetClient::Init` `0xad9b0`, `TryConnect` `0xadc00`, `ConnectionSucceeded` `0xadd60`, `ResetLanDrv` `0xad800`, `StopListening` `0xadae0`, `RxTx` `0xadfc0`, `SendMessages` `0xae2b0`, `ReceiveServerMessages` `0xae0e0`, `cBspTree::Load` `0x2d0860`, `cLevelMap::Load` `0x301ae0`, `cConsole::Exec` `0x15cd90`, `SetCheats` `0x9dec0`, `SetDeveloper` `0x9dea0`, `cCmdLine::Search` `0xa9af0`, `cApplication::Init` `0x329610`, `ProcesParameters` `0x3285d0`, `MainLoop` `0x9e0b0`, `cRenderDX::Init` `0x358e10`, `CreateDevice` `0x356110`.

Statics: `m_nDriverType` `0x5826e0`, `m_nCurClassId` `0x5826dc`, `cRenderDX::m_d3d` `0x69ae0c`, `m_d3dd` `0x69ae10`, `m_hWnd` `0x698aa0`, `m_bWindowed` `0x698aa8`.

GameDll7 RVAs (installed when the DLL loaded, ~31s, after `menumap` was already Exec'd): `GetMapToBeStarted` `0xeacd0`, `GetLastmap` `0x11c0`, `SetLastMap` `0xeaa20`, `SetCurrentMission` `0x11e0`, `SetMapIsComplete` `0xec160`.

`EngLuaExecuteScript` export: NOT FOUND in the probe log.

### Hooks that fired on the baseline

Fired: `Direct3DCreate8/9`, `IDirect3D8::CreateDevice`, device `Reset` hook installed but Reset was not called, `Present`, `EndScene`, `cCmdLine::Search`, `cConsole::Exec`, `cApplication::Init` / `ProcesParameters`, `cRenderDX::Init` / `CreateDevice`, `cBspTree::Load`.

Installed and not called in any run in this note: `cApplication::MainLoop`, `cLevelMap::Load`, `cNetServer::*` (including `Create`), `cNetClient::*`, `cConsole::SetCheats`, `cConsole::SetDeveloper`. `debug.log` still prints `[APP] Entering main loop`, so that line is not the `cApplication::MainLoop` export.

Net counters stayed `srv(rxtx=0 dispatch=0) cli(rxtx=0 send=0 recv=0)` on every run, including `-dedicated`, `-join`, `sv_multiplayer 1`, and `-spmap HUNTMAP1`.

## Baseline run

Command: `Main7.exe +mod data`. 40 seconds. Window title `DNGH2`. Graceful shutdown (`Main loop finished` in `runtime-baseline-debug.txt`).

UDP: `Get-NetUDPEndpoint -OwningProcess` reported no `MSFT_NetUDPEndpoint` rows. `netstat -ano -p udp` had no line for pid 13400. Same empty result on every later run (the sampler treats the CIM "no objects" error as zero sockets).

`debug.log`: home path `.\..\data\`, GameDll 1.0, `[APP] map 'menumap' loaded succesfully`, `[SERVER] loading time : 156ms (map : 78ms, client: 78ms)`. No `[NET]` line and no `[CLIENT]` line in any copied `debug.log`.

`cBspTree::Load(name='Bsp\menumap.bsp', flags=0x6ff, 1)` returned 1 in 46 ms, caller `Main7.exe+0x10372`.

### Switch list (each distinct `cCmdLine::Search` once)

`Search` returns the token index, or -1 when the switch is absent. Baseline command tokens are `+mod`, `data`, so every switch below is -1.

| Switch | Result | Caller |
|---|---|---|
| `+path` | -1 | `Main7.exe+0x15c6` |
| `-shaderdup` | -1 | `Main7.exe+0x1fae` |
| `-console` | -1 | `EngineDll7.dll+0x329f05` |
| `-dedicated` | -1 | `EngineDll7.dll+0x329f1b` |
| `-console98` | -1 | `EngineDll7.dll+0x329f31` |
| `-buildai` | -1 | `Main7.exe+0x2007` |
| `-res` | -1 | `Main7.exe+0x2029` |
| `-logfiles` | -1 | `Main7.exe+0x204b` |
| `-nointro` | -1 | `Main7.exe+0x13969` |
| `-exp` | -1 | `EngineDll7.dll+0x2553fd` |
| `-spmap` | -1 | `GAMEDLL7.DLL+0xd06c` |
| `-batchmode` | -1 | `Main7.exe+0x88a9` |
| `-developper` | -1 | `Main7.exe+0xf3e4` |
| `-cheats` | -1 | `Main7.exe+0xf41d` |
| `-map` | -1 | `Main7.exe+0xf4ce` |
| `-join` | -1 | `Main7.exe+0xf520` |
| `-testmodels` | -1 | `Main7.exe+0x20cb` |
| `-debug` | -1 | `EngineDll7.dll+0x2b2c6a` |

On `+mod data -developper -cheats -map HUNTMAP1` the same calls returned `-developper` = 2, `-cheats` = 3, `-map` = 4. That matches token order `[0]+mod [1]data [2]-developper [3]-cheats [4]-map [5]HUNTMAP1`.

### Console Exec lines (baseline)

Only two, both flag 0:

1. `gun_fov 60` caller `GAMEDLL7.DLL+0x5d33`
2. `menumap menumap` caller `GAMEDLL7.DLL+0xe737c`

`SetCheats` and `SetDeveloper` did not run. The existing decompile `re/notes/loop-decomp-main7b.c` `FUN_0040f310` ORs console field `+0x18` with 2 for `-developper` and with 4 for `-cheats` instead of calling those exports. That decompile was not re-run in Ghidra this session.

## MP experiments

UDP was sampled about every 4 seconds with `Get-NetUDPEndpoint -OwningProcess` and `netstat -ano -p udp` for the whole lifetime of each `Main7.exe`. No run owned a UDP socket.

### a) `-dedicated`

50s. `cCmdLine::Search('-dedicated', 0) -> 2` at `EngineDll7.dll+0x329f1b` (+about 40 ms). The following `-console98` search from the baseline did not run (the dedicated branch skips it; same shape as `InitConsole` in `loop-decomp-console.c`).

GameDll loaded at +453 ms. No `menumap` Exec, no `cBspTree::Load`. `Direct3DCreate8` once (`EngineDll7.dll+0x352a3e`). No `CreateDevice`, `m_d3d` stayed 0, `m_hWnd` stayed 0, `present8=0`. Window title stayed the exe path, not `DNGH2`. `debug.log`: `[DS] Could not set the cooperative level`, then `[APP] Entering main loop` and ~96 fps with `tris=0` until the process was closed at 50s. No `[SERVER]` loading line. `cNetServer::Create` was not called.

The existing decompile of Main7 (`loop-decomp-main7.c`) RealExec's `r_api 0; snd_usesound 0` when `-dedicated` is set, through `cConsole::RealExec`, which the probe does not hook. The render-less loop matches that. The RealExec string itself was not seen in `dh2coop.log`.

Conclusion: `-dedicated` is live and skips the menu map and the D3D device. It does not open a listen socket or call `cNetServer` in 50 seconds.

### b) `-join 127.0.0.1`

50s. After GameDll load (~31s): `Search('-join') -> 2`, then `cConsole::Exec('cl_join 127.0.0.1', 0)` caller `Main7.exe+0xf55e`. `debug.log` line: `[CONSOLE ] symbol 'cl_join' not found!`. The run then Exec'd `menumap menumap`, loaded `Bsp\menumap.bsp`, and printed `[SERVER] loading time : 125ms (map : 78ms, client: 47ms)`. `cNetClient::Init` / `TryConnect` were not called. No UDP socket before or after the Exec (samples at 08:47:30 through 08:47:46, Exec at 08:47:27).

Conclusion: the switch is turned into `cl_join <host>` and the console rejects the name. No connection attempt at the `cNetClient` layer.

### c) `Cfg\console.cfg` with `sv_multiplayer 1`

File created at `game\Data\Cfg\console.cfg` (home path `.\..\data\` + `Cfg\console.cfg`). Contents `sv_multiplayer 1` plus CRLF, 18 bytes, SHA-256 `A5DF744F…B2087`. 50s normal launch.

`debug.log` does not name the file, `sv_multiplayer`, or a missing symbol. The Exec hook did not see `sv_multiplayer`. `cConsole::Load` in the existing decompile (`loop-decomp-console.c`, RVA `0x1672c0`) feeds each line to `RealExec`, not `Exec`, so the probe would miss a successful load. `cNetServer` still did not run. No UDP socket.

On shutdown the file was replaced (2639 bytes, SHA-256 `9B287CA0…F4AD`). The new text is an `addvar` dump. It contains `r_windowed = 1`, `r_api = 1`, `r_width = 640`, `r_height = 480`. It does not contain `sv_multiplayer`. Copy: `re/notes/runtime-cfg-file-after.txt`. The dump was deleted afterwards, and `game\Data\Cfg` was removed.

CreateDevice on this run was `windowed=1 hz=0 pi=0`, unlike the baseline `windowed=0 hz=240`. A follow-up 15s launch with the only line `dh2coop_cfg_marker` (`runtime-cfgmarker-*`) did the same: CreateDevice `windowed=1`, and `debug.log` has no `symbol` line and no marker text. `r_windowed` is registered with default `"1"` (`loop-decomp-conregs-EngineDll7.c`, `cConsole::AddVar`). The missing-file path used fullscreen. The reason a present loose file selects the cvar default is **UNVERIFIED** (which branch skips the fullscreen setup).

`game\Data\console.cfg` with the same marker (`runtime-cfgdata-*`, `Cfg\console.cfg` absent): CreateDevice went back to `windowed=0 hz=240 pi=0x80000000`. No marker and no `symbol` line in `debug.log`. That path is ignored.

Conclusion: the loose file the engine reads and rewrites is `game\Data\Cfg\console.cfg`. `sv_multiplayer 1` there did not bind a port and did not call `cNetServer`. `debug.log` does not record the read. The file's presence does change the presentation mode.

### d) `-developper -cheats -map HUNTMAP1`

55s. Map name from `re\extracted\BSP\HUNTMAP1.BSP`. `Search('-map') -> 4`, then `cConsole::Exec('map HUNTMAP1', 0)` caller `Main7.exe+0xf50c`. `debug.log`: `[CONSOLE ] symbol 'map' not found!`. Next Exec was `menumap menumap`, and `cBspTree::Load` loaded `Bsp\menumap.bsp`. `[APP] map 'menumap' loaded succesfully`. The hunt map was not loaded.

This run still had the shutdown dump of `Cfg\console.cfg` on disk, so CreateDevice was windowed (`windowed=1`). Key injection is in Manual steps.

Follow-up, after both cfg files were deleted: `-developper -cheats -spmap HUNTMAP1` for 45s (`runtime-spmap-*`). CreateDevice was fullscreen again. `Search('-spmap') -> 4` at `GAMEDLL7.DLL+0xd06c`. `cConsole::Exec('spmap HUNTMAP1', 0)` caller `Main7.exe+0xf4ac` (no "symbol not found" line). Then `closemainmenu`, `closegamemenu`, and `cBspTree::Load(name='Bsp\HUNTMAP1.bsp', 0x6ff, 1) -> 1` in 93 ms. `debug.log`: `[APP] map 'HUNTMAP1' loaded succesfully` and `[SERVER] loading time : 219ms (map : 172ms, client: 47ms)`. Also `cConsole::Exec('mod_startTimer 1')`. `cHunterProfile::GetMapToBeStarted -> 'map1'` at `GAMEDLL7.DLL+0x4ecde` while the BSP name was HUNTMAP1. What `map1` refers to is **UNVERIFIED**. Net counters stayed 0. No UDP socket. Present kept running after the load (present8 passed 2000).

Conclusion: `-map` is parsed and Exec'd as `map <name>`, and that symbol is missing, so the menu map still loads. `-spmap HUNTMAP1` loads `Bsp\HUNTMAP1.bsp` directly.

### e) Second instance

Not run. Neither `-dedicated` nor `sv_multiplayer 1` bound a UDP port, which was the condition for copying `game\` to `game2\` and joining. `game2\` was not created. `.gitignore` was not changed.

The existing decompile (`loop-decomp-main7.c`) creates mutex `GI UNIQUE MUTEX` and, on `ERROR_ALREADY_EXISTS`, continues only when `-dedicated` or `-join` is set. That two-process behavior is **UNVERIFIED** here.

## Manual steps for the user

Injected keys did not open a console in a way the logs can see. During the `-map` run, at ~36s (menu map already up, window title `DNGH2`, `SetForegroundWindow` returned true), the sampler sent scan code `0x29` (DIK_GRAVE) twice and `0x3B`–`0x46` (F1–F12) with `keybd_event` `KEYEVENTF_SCANCODE`. No later `cConsole::Exec` and no new `[CONSOLE]` line. DirectInput often ignores injected scan codes. The on-screen result was not visible from this session.

To check the drop-down console by hand:

1. Mount the disc image. From `game\Bin` run `Main7.exe +mod data -developper -cheats`.
2. Wait until the menu is on screen (GameDll and `menumap` land about 30s in).
3. Click the game window. Press the `` ` `` key (same physical key as `~`) once. Then try F1 through F12 one at a time.
4. If a console line appears, type `cam_fov 90` and press Enter. `game\Bin\dh2coop.log` should gain a `cConsole::Exec('cam_fov 90', …)` line while the probe ASI is installed.
5. `-console` and `-console98` are searched at startup (`EngineDll7.dll+0x329f05` and `+0x329f31`). This session did not pass them. A separate check is `Main7.exe +mod data -console` and look for a second console window beside `DNGH2`.

## Open questions

- `-dedicated` never calls `cNetServer::Create` and never binds UDP. Whether a map plus `sv_multiplayer` / `sv_port`, passed after the console symbols exist, would listen is **UNVERIFIED**.
- `cl_join` and `map` are executed at GameDll-init time and the console says the symbol is missing. `spmap` exists at that same moment. Where those symbols would be registered, if anywhere, was not located in this run.
- A loose `game\Data\Cfg\console.cfg` selects windowed 640x480 (`r_windowed` default 1). No file selects 640x480 fullscreen at 240 Hz. The branch that writes the fullscreen value was not identified.
- `cConsole::Load` uses `RealExec`. The probe cannot see those lines. A marker command in the cfg file produced no `[CONSOLE] symbol` line, so `RealExec` may not log missing names the way `Exec` does. **UNVERIFIED** whether `sv_multiplayer 1` ran and was ignored, or was never applied.
- `cApplication::MainLoop` (`RVA 0x9e0b0`) was hooked and never entered. The function that prints `Entering main loop` is still unnamed.
- `m_nDriverType` stayed 0. The enum value was not mapped in this track.
- In-game console key is unconfirmed on screen. Steps are above.
- Two processes on one PC were not tried. Mutex behavior is decompile-only.

## Proposed MEMORY facts

- Retail render path is D3D8: `Direct3DCreate8` three times (callers `EngineDll7.dll+0x352a3e`, `+0x35211c`, `+0x358f9e`), one HAL device, `Present` caller `EngineDll7.dll+0x354cfd`, `EndScene` caller `+0x354b63`. `Direct3DCreate9` (`+0x35a43b`) creates no device. Evidence: `re/notes/runtime-baseline-probe.txt`.
- With no loose `game\Data\Cfg\console.cfg`, CreateDevice params are 640x480, format 21, windowed 0, refresh 240, presentation interval `0x80000000`, behavior `0x84`. Evidence: same log, `params in` line.
- `cCmdLine::Search` returns the argv index or -1. Evidence: `-developper` = 2, `-cheats` = 3, `-map` = 4 on `+mod data -developper -cheats -map HUNTMAP1` in `runtime-map-probe.txt`.
- `-join <host>` becomes `cConsole::Exec("cl_join <host>")` at `Main7.exe+0xf55e`, and `debug.log` prints `[CONSOLE ] symbol 'cl_join' not found!`. `cNetClient::Init` does not run. Evidence: `runtime-join-probe.txt`, `runtime-join-debug.txt`.
- `-map <name>` becomes `Exec("map <name>")` at `Main7.exe+0xf50c` and `debug.log` prints `[CONSOLE ] symbol 'map' not found!`, then `menumap` loads. Evidence: `runtime-map-probe.txt`, `runtime-map-debug.txt`.
- `-spmap HUNTMAP1` loads `Bsp\HUNTMAP1.bsp` (`cBspTree::Load` -> 1) and `debug.log` prints `map 'HUNTMAP1' loaded succesfully`. Evidence: `runtime-spmap-probe.txt`, `runtime-spmap-debug.txt`.
- `-dedicated` is recognized (`Search` -> 2 at `EngineDll7.dll+0x329f1b`), skips the D3D device and the menu BSP, and does not call `cNetServer::Create` or bind UDP over 50s. Evidence: `runtime-dedicated-probe.txt`, `runtime-dedicated-debug.txt`, `runtime-dedicated-udp.txt`.
- The engine reads and, on a normal shutdown, rewrites `game\Data\Cfg\console.cfg`. `game\Data\console.cfg` does not change CreateDevice. Evidence: cfg vs cfgdata CreateDevice lines; file grew from 18 to 2639 bytes on the cfg run.
- Single-player `[SERVER] loading time` lines are printed with `cNetServer` / `cNetClient` call counts still zero. Evidence: baseline and spmap debug logs plus probe state lines `srv(rxtx=0 dispatch=0)`.
- No experiment bound a UDP port (`Get-NetUDPEndpoint` empty and `netstat -ano -p udp` empty for the game pid). Evidence: `re/notes/runtime-*-udp.txt`.
