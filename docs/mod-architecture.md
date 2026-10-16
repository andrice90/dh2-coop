# Mod architecture

**Progress:** skeleton written. Code and runtime proofs still landing. See `re/notes/p2a-runtime.md` for log evidence.

This is the Phase 2 scaffolding inside `dh2coop.asi` (Route E). It owns modules, the D3D8 frame hook, a basic console wrapper, and local two-process launch. It does not register console commands, disable slow motion, track the player, or speak the network protocol. Those are extension points for other owners.

## Module API

Modules derive from `dh2::Module` (`mod/src/core/module.h`). Every hook has a no-op default.

| Hook | When |
|---|---|
| `OnInit()` | ASI init thread, after logging is up and before engine hooks |
| `OnEngineReady(HMODULE engine)` | ASI init thread, once `EngineDll7.dll` is loaded |
| `OnGameDllLoaded(HMODULE game)` | ASI init thread, once `GameDll7.dll` is loaded |
| `OnMapLoaded(const char* bspName)` | Caller of `cBspTree::Load`, after the original returns 1 |
| `OnFrame(float dtSeconds)` | Game thread, inside `IDirect3DDevice8::Present`, before the original |
| `OnShutdown()` | Best-effort on the ASI init thread when the process detaches |

`Name()` is a stable id. `Priority()` defaults to 100. Lower priority runs first. Equal priorities sort by `Name()` with `strcmp`.

A module that throws (`std::exception` or anything else) is logged and disabled. Later hooks skip it. The original `Present` and `cBspTree::Load` still run.

`DH2_MODULE(ClassName)` registers the class during static initialization. Modules never edit `dllmain.cpp`.

`bspName` is the `cBspTree::Load` path (observed form `Bsp\HUNTMAP4.bsp`). It is valid only for the duration of `OnMapLoaded`.

`dtSeconds` is wall time between `Present` calls from `QueryPerformanceCounter`, not the engine's `frameTime`. The first `Present` passes 0.

## Event flow

```
DllMain PROCESS_ATTACH (loader lock)
  set ASI path
  hook kernel32!CreateMutexA          // must be sync; the game creates the mutex
                                      // just after InitEngineDll returns
  spawn ASI init thread

ASI init thread
  read dh2coop.ini, open dh2coop.log
  install unhandled-exception filter + vectored logger
  construct modules (priority, then name)
  OnInit()
  register LdrRegisterDllNotification
  poll:
    d3d8.dll appeared        -> hook Direct3DCreate8
    EngineDll7 already loaded or Ldr says so
                             -> resolve console, hook cBspTree::Load, OnEngineReady
    GameDll7 Ldr or GetModuleHandle
                             -> OnGameDllLoaded
    cRenderDX::m_d3dd set    -> late-attach Present/Reset if CreateDevice was missed

Game thread
  Direct3DCreate8 -> IDirect3D8::CreateDevice -> hook vtable Present + Reset
  Present:
    dt = QPC
    OnFrame(dt) for each live module
    console deferred checks
    PresentCallbacks
    original Present
  cBspTree::Load returns 1:
    OnMapLoaded(name)
    optional [Console] ExecAfterMap once
```

`EngineDll7.dll` is already mapped when the ASI attaches (the loader is `dinput8.dll`, imported by the engine). `LdrRegisterDllNotification` still catches `GameDll7.dll`, which loads later. The init thread also calls `GetModuleHandleA` so a notification that arrived before the callback was registered is not lost.

The loader-notification callback only stores the `HMODULE`. `OnEngineReady` / `OnGameDllLoaded` run on the ASI init thread, off the loader lock, because those hooks allocate and patch code.

`OnMapLoaded` and `OnFrame` run on the game thread that called `Load` or `Present`. Do not `Sleep` or wait on the init thread from those hooks.

`OnShutdown` is best-effort. `TerminateProcess` (the launch script's force-kill path) skips it.

## File layout

| Path | Role |
|---|---|
| `mod/src/dllmain.cpp` | Attach, mutex hook, init thread. No feature code |
| `mod/src/core/module.*` | Registration and dispatch |
| `mod/src/core/lifecycle.*` | DLL watch, map hook, crash dump |
| `mod/src/core/heartbeat_module.cpp` | Example `OnFrame` logger |
| `mod/src/core/config.*` | `dh2coop.ini` |
| `mod/src/core/hook.*` | Leaked safetyhook helper |
| `mod/src/engine/d3d8_hooks.*` | D3D8 create/device/`Present`/`Reset` |
| `mod/src/engine/console.*` | `Ready` / `Exec` / `RealExec` |
| `mod/src/probe/*` | Logging module, ini `[Probe] Enabled` (default 0) |
| `mod/src/engine/console_cmds.*` | Lead. Command registration. Not in this job |
| `mod/src/coop/**` | Lead. Player tracker and coop requests |
| `mod/src/net/**`, `mod/net-test/**` | Other job. CMake globs them when they exist |
| `mod/dh2coop.ini` | Defaults copied into `game\Bin` |
| `run-coop-local.ps1` | Two windowed instances |
| `tools-src/console.windowed.cfg` | Loose `Data\Cfg\console.cfg` template |

`mod/CMakeLists.txt` globs `mod/src/**/*.cpp`. A missing `net/` or `coop/` folder still configures. `build.ps1` always re-runs CMake, so a file added after the last configure is picked up on the next build.

## How to add a module

1. Add `mod/src/<area>/<name>_module.cpp` (anywhere under `mod/src`).
2. Derive from `dh2::Module`, implement `Name()`, and override the hooks you need.
3. Put `DH2_MODULE(YourClass)` in that cpp file.
4. Rebuild with `.\mod\build.ps1`. Do not edit `dllmain.cpp`.

```cpp
class ExampleModule : public dh2::Module {
public:
    const char* Name() const override { return "example"; }
    int Priority() const override { return 200; }
    void OnFrame(float dt) override { /* game thread */ }
};
DH2_MODULE(ExampleModule)
```

`core/heartbeat_module.cpp` is the working example. It logs every 10 seconds of accumulated `Present` dt: total frame count, frames in the window, and average dt.

## Console wrapper

Header: `mod/src/engine/console.h`.

Resolved from `EngineDll7.dll` by export name (the DLL relocates):

| Export | Meaning |
|---|---|
| `?console@@3PAVcConsole@@A` | Address of the global `cConsole*`. `Ready()` dereferences it |
| `?Exec@cConsole@@QAEHPADH@Z` | `int __thiscall Exec(char* line, int flag)` |
| `?RealExec@cConsole@@QAEHPAD@Z` | `int __thiscall RealExec(char* line)` |

`dh2::console::Bind(HMODULE)` runs from `OnEngineReady`. The global stays null until `InitConsole`, which is after the engine DLL loads, so `Ready()` reads the pointer on each call.

`dh2::console::Exec(const char*)` calls engine `Exec` with **flag 0** through an `int(__thiscall*)(void*, char*, int)`. Flag 0 copies the line into the console queue unchanged. Flag non-zero stores `%<line>`, which is the user-typed path and is subject to the developer/cheat gates. The copy inside `Exec` is byte-by-byte into a 0x80-byte slot (decompile of `cConsole::Exec`, EngineDll7 RVA `0x15cd90`). The wrapper refuses lines of 127 characters or more so that copy stays inside the slot. Return value 1 means queued, 0 means the 32-deep queue overflowed, -1 means not ready.

`dh2::console::RealExec(const char*)` runs the line immediately and does not go through the `Exec` hook.

This job does not register commands. `AddStaticFuncf` is intentionally not called here.

Ini `[Console] ExecAfterMap` (default empty) is executed once, via `Exec`, after the first successful `cBspTree::Load`. The runtime proof sets it to `cam_fov 70`.

## D3D8 hooks

`dh2::d3d8::Device()` is the `IDirect3DDevice8*` from the first successful `CreateDevice`, or from the exported `cRenderDX::m_d3dd` if the device already exists.

`dh2::d3d8::AddPresentCallback` is the overlay hook point. Callbacks run on the game thread after `OnFrame` and before the original `Present`. Signature: `void(void* device, const RECT* src, const RECT* dst, HWND window, const void* dirty)`.

`dh2::d3d8::AddResetCallback` is the `Reset` stub. `phase` 0 is before the original `IDirect3DDevice8::Reset`, `phase` 1 is after. The overlay releases D3D resources on 0 and recreates them on 1. `present_params` is the D3D8 present-parameters pointer (52 bytes; the Windows SDK has no `d3d8.h`).

Vtable slots, carried from the working probe: `IDirect3D8::CreateDevice` 15, `IDirect3DDevice8::Reset` 14, `Present` 15.

## Crash dumps

The unhandled-exception filter writes `dh2coop-crash.dmp` next to the ASI (`game\Bin` for the primary copy, `game2\Bin` for the second) via `MiniDumpWriteDump`, then logs the exception code and `module+RVA`. A vectored handler still logs the first few severe first-chance exceptions and continues the search.

## Launch scripts

`run-game.ps1` starts one sandbox copy. `run-coop-local.ps1` mounts the disc the same way, mirrors `game\` to `game2\` with robocopy (skipping `*.log` and `console.cfg`), copies `mod\build\dh2coop.asi` and `mod\dh2coop.ini` into both `Bin` folders, writes `tools-src\console.windowed.cfg` to both `Data\Cfg\console.cfg`, and starts:

`Main7.exe +mod data -nointro -developper -cheats -spmap HUNTMAP4`

from each copy's `Bin` directory. `-Seconds N` closes both. The script prints both PIDs and the `dh2coop.log` / `debug.log` paths.

`r_windowed = 1` in the loose cfg is what makes the device windowed. The game rewrites that file on a clean shutdown.

`[Coop] AllowMultiInstance` defaults to 1. The mutex hook renames `"GI UNIQUE MUTEX"` to `"GI UNIQUE MUTEX <pid>"`.

## Extension points left for other owners

- `OnFrame` / `OnMapLoaded` / `OnGameDllLoaded` for the player tracker and slow-motion disable.
- `dh2::console::Exec` / `RealExec` for `coop_host` / `coop_join` once commands exist. Do that in `engine/console_cmds.*`, not here.
- `AddPresentCallback` and `AddResetCallback` for the D3D8 overlay.
- `dh2::d3d8::Device()` for anything that needs the device pointer.

## Proposed MEMORY facts

Code-level facts are listed here. Runtime confirmation is in `re/notes/p2a-runtime.md` and is filled in after the launches.

- `cConsole::Exec` is `int __thiscall(char*, int)`. Flag 0 copies the line into a 32-slot queue of 0x80-byte records and returns 1, or returns 0 and logs `command buffer overflow` when `*(console+0x1c) >= 0x20`. Evidence: export `?Exec@cConsole@@QAEHPADH@Z`; decompile at EngineDll7 RVA `0x15cd90` in `re/notes/loop-decomp-console.c`.
- `cConsole::RealExec` is `int __thiscall(char*)`. Evidence: export `?RealExec@cConsole@@QAEHPAD@Z`.
- `?console@@3PAVcConsole@@A` is the address of a `cConsole*` global, not the object. `InitConsole` (called from Main7 `FUN_00401dd0` after `CreateMutexA`) is what stores the object. Evidence: export name; `re/notes/loop-decomp-main7.c` lines 201-226.
- `CreateMutexA(..., "GI UNIQUE MUTEX")` runs after `InitEngineDll()` returns, so an ASI loaded by `EngineDll7`'s `dinput8` import can hook it from `DllMain` before the call. Evidence: `re/notes/loop-decomp-main7.c` `FUN_00401dd0` (InitEngineDll at the call before `CreateMutexA`).
- `cBspTree::Load` returning 1 means the map loaded. Evidence: `re/notes/runtime-spmap-probe.txt` line `cBspTree::Load -> 1 in 93ms` for `HUNTMAP1`.
