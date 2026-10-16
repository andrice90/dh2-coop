# Job: P2a — mod framework (Phase 2). YOU OWN GAME LAUNCHES. No Ghidra needed.

Route E is decided: we are building our own co-op layer inside `dh2coop.asi`. This job is **scaffolding only**. The lead (Claude) does the expert hooking work in parallel: console command registration, the slow-motion hook and the player tracker. Do not implement those; leave clean extension points.

Read first: `MEMORY.md` ("Loop and console", "Runtime probe"), `docs/mod-learnings.md`, `mod/CMakeLists.txt`, `mod/build.ps1`, and all of `mod/src/` (existing core + probe, which already works).

## File ownership
- **You own:**
  - `mod/CMakeLists.txt`, `mod/build.ps1`, `mod/dh2coop.ini`
  - `mod/src/dllmain.cpp`, `mod/src/core/**`, `mod/src/probe/**`
  - `mod/src/engine/d3d8_hooks.*`, `mod/src/engine/console.*`
  - `run-coop-local.ps1`, `tools-src/console.windowed.cfg`
  - `docs/mod-architecture.md`, `re/notes/p2a-runtime.md`
- **Do not create or edit:**
  - `mod/src/net/**` and `mod/net-test/**` (another job owns them)
  - `mod/src/coop/**` and `mod/src/engine/console_cmds.*` (the lead owns them)
- CMake must glob `mod/src/**/*.cpp` recursively, so the other owners' files compile in automatically. The build must succeed whether or not those folders exist yet.

## Tasks
1. **Module system** (`core/module.h/.cpp`).
   - A static-registration macro such as `DH2_MODULE(ClassName)`. Modules derive from `dh2::Module`, which has virtual no-op hooks:
     - `OnInit()`, called from the ASI init thread
     - `OnEngineReady(HMODULE engine)`, once EngineDll7 is loaded
     - `OnGameDllLoaded(HMODULE game)`
     - `OnMapLoaded(const char* bspName)`
     - `OnFrame(float dtSeconds)`, on the game thread
     - `OnShutdown()`
   - Modules never edit `dllmain.cpp`. Dispatch order is registration priority, then name. Catch and log exceptions per module, and disable a module that throws.
   - Include a tiny example module, `core/heartbeat_module.cpp`, that logs once every 10 s from `OnFrame` with the frame count and average dt. This proves `OnFrame` runs.
2. **Port the probe** into a module gated by ini `[Probe] Enabled`, default **0** from now on. The event sources stay always-on, independent of the probe:
   - EngineDll7/GameDll7 load detection via the existing `LdrRegisterDllNotification` code → `OnEngineReady` / `OnGameDllLoaded`
   - the `cBspTree::Load` hook → `OnMapLoaded` (after the original returns success)
   - the D3D8 hooks below → `OnFrame`
3. **D3D8 frame hook** (`engine/d3d8_hooks.*`).
   - Move the working Direct3DCreate8 → CreateDevice → `IDirect3DDevice8::Present` hook chain out of the probe.
   - Dispatch `OnFrame` from Present, before calling the original, with dt from QueryPerformanceCounter.
   - Expose `dh2::d3d8::Device()` and a `PresentCallbacks` hook point for the future overlay, plus a `Reset` hook stub that the overlay will need.
4. **Console wrapper** (`engine/console.*`), basic only.
   - Resolve, by mangled export name from `re/exports/EngineDll7.dll.exports.txt`: the global `?console@@3PAVcConsole@@A` (a `cConsole*` variable, so dereference it), `cConsole::Exec`, and `cConsole::RealExec`.
   - Provide `dh2::console::Ready()` (pointer non-null) and `dh2::console::Exec(const char*)` (thiscall through a typed function pointer; use the exact mangled signature from the exports to get the argument list right).
   - Prove it: after a map loads, exec `cam_fov 70` and confirm it took effect. The probe's Exec hook, or `debug.log`, should show the line. Do **not** implement command registration.
5. **Two instances on one PC.**
   - Hook kernel32 `CreateMutexA`. When the name is `"GI UNIQUE MUTEX"` and ini `[Coop] AllowMultiInstance=1` (default 1), replace it with `"GI UNIQUE MUTEX <pid>"`.
   - Add `run-coop-local.ps1`. It should:
     - mount the disc if needed (reuse logic from `run-game.ps1`)
     - robocopy `game\` → `game2\`, excluding `*.log`, `debug.log` and `Data\Cfg\console.cfg`
     - copy the freshly built ASI and ini into both copies
     - write `tools-src\console.windowed.cfg` (derive it from `re/notes/runtime-cfg-file-after.txt`; `r_windowed = 1` is what matters) into both `Data\Cfg\console.cfg`
     - launch both with `+mod data -nointro -developper -cheats -spmap HUNTMAP4`, each from its own `Bin` dir
     - support `-Seconds N` to auto-close both
     - print both PIDs and log paths
   - Prove it: both `debug.log`s contain `map 'HUNTMAP4' loaded succesfully`, and both `dh2coop.log`s show heartbeat lines.
   - Add `game2/` to `.gitignore` if not already there.
6. **Build hygiene.**
   - `mod\build.ps1 -Clean` from scratch must pass.
   - Warnings from our own code at /W4 must be fixed.
   - Keep the FOV fix installed.
   - On a crash, install an unhandled-exception filter that writes a minidump to `game\Bin\dh2coop-crash.dmp` and logs the faulting module and RVA.

## Deliverables
- The code above.
- `docs/mod-architecture.md`: module API, event flow (which thread, when), file layout, how to add a module, the console wrapper usage, and launch scripts.
- `re/notes/p2a-runtime.md`: each runtime proof with log excerpts of 30 lines or fewer, problems, open questions, and `## Proposed MEMORY facts`.
