# Job: P2a — mod core (Phase 2). YOU OWN GAME LAUNCHES. Ghidra slot E if needed.

Route E is decided: we are building our own co-op layer inside `dh2coop.asi` (see `PLAN.md` → Decisions). This job turns the probe ASI into a real mod framework. Read these first:
- `MEMORY.md`, especially "Loop and console", "Object model" and "Runtime probe"
- `docs/mod-learnings.md`
- `mod/CMakeLists.txt`, `mod/build.ps1`, all of `mod/src/` (existing core + probe)
- `mod/src/game/types_draft.h`
- `re/notes/console.md` and `re/notes/object-model.md` as needed

**File ownership:**
- You own `mod/CMakeLists.txt`, `mod/build.ps1`, `mod/dh2coop.ini`, `mod/src/dllmain.cpp`, `mod/src/core/**`, `mod/src/engine/**` (new), `mod/src/coop/**` (new), `mod/src/probe/**`, and the new launch scripts at repo root.
- You also own `docs/mod-architecture.md` (new).
- **Do not create or edit anything under `mod/src/net/` or `mod/net-test/`.** Another job owns them in parallel. Your CMake must glob `mod/src/**/*.cpp`, so their files get picked up automatically when they appear. If `mod/src/net/` does not exist yet, the build must still succeed.

## Tasks
1. **Module system** (`core/module.h`). Use a static-registration macro, for example `DH2_MODULE(name)`, with optional `OnInit()`, `OnEngineReady()`, `OnGameDllLoaded(HMODULE)`, `OnMapLoaded(const char* name)`, `OnFrame(float dtSeconds)` and `OnShutdown()`.
   - Modules never edit `dllmain.cpp`.
   - Port the probe into a module, gated by ini `[Probe] Enabled`, default **0** from now on.
   - Keep the probe's useful hooks available as an engine-event source: device creation/Present, `cBspTree::Load` → `OnMapLoaded`, and the loader notification → `OnGameDllLoaded`.
2. **Frame callback.** Dispatch `OnFrame` on the game thread from the `IDirect3DDevice8::Present` hook, with dt from QueryPerformanceCounter. Put the Present hook in `engine/d3d8_hooks.*` and expose the device pointer for the future overlay job.
3. **Console API** (`engine/console.*`):
   - Resolve the exported global `?console@@3PAVcConsole@@A` (it is a `cConsole*` variable), plus `cConsole::Exec`, `cConsole::RealExec` and `cConsole::AddStaticFuncf`, all by mangled export name (see `re/exports/EngineDll7.dll.exports.txt`).
   - Provide `Console::Exec(const char*)` and `Console::AddCommand(name, fn, argTypes...)`. Register commands once the console pointer is non-null and `InitConsole` has run; hook `InitConsole` if it is exported, otherwise detect lazily on the first frame.
   - Register these commands with flag 0 (visible without `-developper`):
     - `coop_status`: logs state.
     - `coop_host [port]` and `coop_join <ip> [port]`: forward to the functions declared in `coop/coop.h`, `coop::RequestHost(uint16_t)` and `coop::RequestJoin(const char*, uint16_t)`. Implement them for now as stubs that log, plus a `coop::SetTransport(...)` hook point. The net job's API will be wired in later.
   - Figure out how a static console func receives its arguments: read `re/notes/console.md` and the decompile of `AddStaticFuncf` and the dispatch code. Document it in `docs/mod-architecture.md`.
   - **Verify at runtime** by calling `Console::Exec("coop_status")` from our own code after registration, and checking `dh2coop.log` for the handler's log line.
4. **Two instances on one PC.**
   - Hook `CreateMutexA` (kernel32) so that the name `"GI UNIQUE MUTEX"` becomes `"GI UNIQUE MUTEX <pid>"` when ini `[Coop] AllowMultiInstance=1` (default 1). This stops the second process from exiting.
   - Add `run-coop-local.ps1` at repo root. It should:
     - create/refresh `game2\` from `game\` with robocopy, excluding logs and `Data\Cfg\console.cfg`
     - write a windowed `Data\Cfg\console.cfg` into both copies from a template in `tools-src\console.windowed.cfg`. Derive the template from `re/notes/runtime-cfg-file-after.txt`; `r_windowed = 1` is the key.
     - launch two instances with `+mod data -nointro -developper -cheats -spmap HUNTMAP4`
     - give each instance its own working dir and its own log files
     - support `-Seconds N` to auto-close both
   - Prove both processes reach the map. Two `[APP] map 'HUNTMAP4' loaded succesfully` lines, one per copy's `debug.log`.
   - Add `game2/` to `.gitignore`.
5. **Slow motion off.** With ini `[Coop] DisableSlowMo=1` (default 1):
   - after the map loads, exec `mod_slowmotionFrameTime 1`
   - also hook `cHUDSlowMotion::Enable` (GameDll7 RVA `0xca690`, not exported) by pattern scan to return immediately; confirm the RVA and pattern in Ghidra slot E
   - log both actions
6. **Player tracker** (`coop/player_tracker.*`):
   - Hook `cObjManager::AddEntity` (EngineDll7 export, RVA `0xbd4b0`) to capture the manager pointer and entities as they are added.
   - Find the local player: the entity whose vtable equals GameDll7 `cScr_ModHunter`'s vtable (GameDll7 base + RVA `0x106ff0`), or failing that, `cObjHumanEx::IsPlayer`.
   - Once per second in `OnFrame`, log the player's world position (`+0xE8` tVect3), health (`+0x19C`) and entity id (`+0x04`), and also the yaw/angle if you can find its offset. Check `cObjEntity::SetAngle`/`GetAngle` exports.
   - Prove it with a single-instance `-spmap HUNTMAP4` run: position values should be plausible and stable.
   - Also count entities by vtable, so animals show up (cScrAnimal vtable GameDll7 RVA `0xff470`), and log each animal's id and position once.
7. **Build and run** with `mod\build.ps1` (builds and copies to `game\Bin`; `run-coop-local.ps1` must copy the ASI and ini to `game2\Bin` too). Keep the FOV fix installed. If the game crashes, write a minidump or log the exception, and fix it.

## Deliverables
- The code above.
- `docs/mod-architecture.md`: module system, event flow, console API usage including the argument-passing finding, file layout, and how to add a module.
- `re/notes/p2a-runtime.md`: each runtime proof with log excerpts of 30 lines or fewer, problems found, open questions, and `## Proposed MEMORY facts` (for example the angle offset, the manager pointer, the console arg convention, and the mutex result).
