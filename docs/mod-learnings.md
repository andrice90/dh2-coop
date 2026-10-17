# Mod implementation learnings

Practical knowledge for building, injecting, hooking, testing and orchestrating. Engine facts live in `MEMORY.md`; this file is about *how to work*. Add an entry whenever something cost time to figure out. Format: learning — evidence/date.

## Toolchain
- VS 2022 Build Tools (MSVC 14.44.35207, Windows SDK 10.0.26100) at `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`. `vcvars32.bat` fails silently unless `${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer` (vswhere) is on PATH first — 2026-10-15.
- 32-bit build confirmed (`sizeof(void*)==4`) — 2026-10-15.
- winget's VS BuildTools install "fails" with exit 1 when it is already installed; check `dd_installer_*.log` for "already installed" before assuming breakage — 2026-10-15.
- Mod deps (CMake FetchContent, `mod/CMakeLists.txt`): safetyhook v0.7.0, spdlog v1.17.0, Dear ImGui v1.93.0, ENet v1.3.18. Static CRT (`/MT`), output `dh2coop.asi`. Build: `mod\build.ps1` (copies ASI + ini to `game\Bin`).
- Python: use `py` (3.14); the `python` alias is the Microsoft Store stub. `pefile` installed for user.
- Ghidra 12.1.4 + Temurin JDK 21 (`JAVA_HOME` set at user level). Headless projects: master `re/ghidra/DH2`, per-worker copies `re/ghidra-slots/<Letter>` (a Ghidra project allows one process at a time). `re/scripts/decomp.ps1` ≈ 7 s per call.

- Net layer test: `mod\net-test\build.ps1 [-Clean]` builds and runs `dh2net_test.exe` (932 checks; loopback host + 3 clients on UDP 27420). Build scripts must run native tools as `cmd /c "<bat> 2>&1"`: under PS 5.1 + `ErrorActionPreference=Stop`, git's "Cloning into..." on stderr aborts the script when the host captures stderr — 2026-10-16.

## Injection and launch
- Ultimate ASI Loader v9.7.4 works as `game\Bin\dinput8.dll` (EngineDll7 imports DINPUT8, so the proxy loads early). Our `dh2coop.asi` and the AlphaYellow FOV fix coexist — 2026-10-15.
- Launch must be `Main7.exe +mod data` with working dir `Bin`; otherwise the home path is wrong and nothing loads. Use `run-game.ps1` (auto-mounts the disc image via `VCDMount.exe`) — 2026-10-15.
- All engine DLLs relocate (EngineDll7 seen at 0x00CA0000; GameDll7 at 0x0E570000). Resolve by export name or pattern, log as `Module+RVA` — 2026-10-15.
- `GameDll7.dll` is loaded late (≈30 s after process start in a `-developper -cheats` run, after intro/menu setup) and from `game\data\`. The probe catches it with `LdrRegisterDllNotification`; hooks into GameDll7 must be installed from that callback, not at ASI init — dh2coop.log 2026-10-15.

## Testing loop
- Add `-nointro` to skip intro movies (switch confirmed in Main7 by Track D; effect on load time not yet measured) — 2026-10-16.
- Console from our code: `cConsole::Exec`/`RealExec` via the exported `console` pointer; in-game console needs A+V+F12 (or Q+I+F12), then `` ` ``, then password `3.141692` as the first line — 2026-10-16.
- For hands-on play sessions give the user `run-game.ps1` **without** `-Seconds` (that flag auto-closes the game; a user walk test was cut off at 60 s) — 2026-10-16.
- **Fast dev loop:** `.\run-game.ps1 -Seconds 60 -ExtraArgs '-developper -cheats -spmap HUNTMAP4'` loads straight into a hunt map (~30 s, mostly intro/menu init before GameDll7 loads). `-map` does NOT work — 2026-10-16.
- **Windowed mode:** put any loose `game\Data\Cfg\console.cfg` in place → 640×480 windowed (needed to run two instances side by side). The game rewrites the file on exit, so keep a template copy elsewhere and restore it before each run. Without the file: fullscreen 640×480 @ 240 Hz — 2026-10-16.
- **Scripted input works:** `SendInput` with `KEYEVENTF_SCANCODE` (DIK scan codes, arrows with the extended flag) to the **foreground** game window reaches DirectInput — `tools-src/game-input.ps1 -ProcessId <pid> -Keys 'RETURN','W:2000',...` (2026-10-16 13:43; the confirm dialog opened). The INPUT struct must include the full union (x64 PowerShell size 40). The older note "injected keys don't reach the game" was about window messages / unfocused targets. DirectInput is foreground-only: capture other windows with `capture-window.ps1 -NoFocus` or the walking instance stops reading keys.
- Free-hunt CHOOSE ITEM menu closes with **Enter, Left, Enter** (pick item → move confirm to YES → accept; user 2026-10-16).
- `game-input.ps1` also does relative mouse moves: `MOUSE:dx,dy` sent through `SendInput` reaches DirectInput (400 units ≈ 34.7° yaw; negative dy looks up) — 2026-10-16.
- **Finding a runtime value:**
  1. Start with `[Tracker] DiffHunter=1`. It logs every changed float in the hunter object, plus every entity whose `+0xF4` changed, once per second. Drive it with scripted input.
  2. If the value isn't there, look for the engine API that consumes it (here the render camera) before scanning the heap.
  3. A whole-heap float scan is a last resort and a trap: `fabs(x - t) > tol` lets NaN through (`0xFFFFFFFF` filler is NaN), so compare with `!(fabs(x - t) <= tol)`. Exclude your own buffers, or they fill the candidate list with copies.
  4. Matches found that way were per-bone copies with no stable owner.
  — 2026-10-16.
- Automated tests: `tools-src/focus-test.ps1` (background stall phases), `tools-src/move-test.ps1` (menu close + scripted host walk + captures of both windows + puppet log lines).
- Second process on one PC: `"GI UNIQUE MUTEX"` blocks it unless `-join`/`-dedicated` is on the command line (decompile; not yet tested). `-join x` harmlessly execs a missing `cl_join` command, so `-join 0` is a workable "allow second instance" flag until our ASI handles the mutex.
- UDP check while the game runs: `Get-NetUDPEndpoint -OwningProcess <pid>` plus `netstat -ano -p udp`.
- `re/scripts/runtime-exp.ps1` (Track E) wraps launch + log capture + UDP sampling per experiment.

## Hooking
- safetyhook inline hooks on mangled exports (resolved with `GetProcAddress`) work: `cCmdLine::Search`, `cConsole::Exec`, `cBspTree::Load`, `cHunterProfile::{GetMapToBeStarted,GetLastmap,SetLastMap,SetCurrentMission,SetMapIsComplete}` — dh2coop.log 2026-10-15.
- `cApplication::MainLoop` (EngineDll7 RVA `0x9e0b0`) was hooked and never entered — the real frame loop is elsewhere (Track D). Use the D3D8 `Present` hook as the per-frame callback until a game-tick hook is found — 2026-10-16.
- To see console commands from cfg files, hook `cConsole::RealExec` as well as `cConsole::Exec` — 2026-10-16.
- Build warning to ignore: safetyhook `os.windows.cpp` C4005 `NOMINMAX` redefinition; safetyhook v0.7.0 bumps the target to `/std:c++latest` — 2026-10-16.
- Renderer is D3D8: one `IDirect3DDevice8`; `Present` called from `EngineDll7+0x354cfd`, `EndScene` from `EngineDll7+0x354b63`. `Direct3DCreate8` is called 3×, `Direct3DCreate9` 1× (probe/enumeration only; no D3D9 device) — dh2coop.log. Overlay (ImGui) must use a DX8 path (ImGui has no DX8 backend: use d3d8to9 or a custom DX8 renderer).

- **Game-thread rule:** module `OnFrame` runs only when `Present` is called on the device-window thread (`engine/d3d8_hooks.cpp` gate); the loading screen presents from another thread. `OnEngineReady`/`OnGameDllLoaded`/`OnShutdown` run on the lifecycle poll thread — install hooks there, never touch game state. `OnMapLoaded` runs on the main thread inside the `cBspTree::Load` detour — 2026-10-16.
- Two-instance test: `.\run-coop-local.ps1 -Seconds 100` (instance 1 `-coophost`, instance 2 `-coopjoin 127.0.0.1`, both `-spmap HUNTMAP4`, windowed). Grep each `Bin\dh2coop.log` for `coop: remote` — 2026-10-16.
- `run-coop-local.ps1` starts each `Main7.exe` with `Win32_Process.Create`. An agent shell (Grok Build's, 2026-10-16 16:36/16:53/17:00) runs commands in a job object that kills `Start-Process` children when the command returns; that was the "both crashed" report, not the mod. The host `debug.log` stopping at `time=3.01` was only where its buffered log ended. The script also waits for host engine time ≥ 8 s (host in the map, listening) before starting the client — 2026-10-16.
- **Automation channel:** write console lines to `<tree>\Bin\dh2coop.cmd`; the coop module reads and deletes it every 0.5 s and queues each line with `cConsole::Exec`. Test commands: `coop_kill <animal id> [dmg]` (host/solo, `FuncInflictDamage`), `coop_claim <animal id>` (runs the trophy-claim hook as if the claim key fired). `tools-src\event-test.ps1` uses both — 2026-10-16.
- **Worker CLI:** defined jobs go to Grok 4.7 through the Cursor CLI: `tools-src\cursorjob.ps1 -Name <job>` (`cursor-agent -p --model grok-4.7-medium-fast`, prompt file `re\jobs\<job>.prompt.md`). Grok Build (`grokjob.ps1`) is retired (user, 2026-10-16) — 2026-10-16.
- Hooking an exported engine function only catches calls that go through that body: `cObjManager::Get` is never called by game code (inlined/unused), `AddEntity(cObjEntity*)` is (Main7's override calls the engine body). Check the importers of an export (pefile import walk) before choosing a hook — 2026-10-16.
- Lead-owned modules live in `mod/src/coop/` (`coop.cpp` session + console commands + background flag, `player_tracker.cpp`, `slowmo.cpp`) and `mod/src/engine/console_cmds.*` — 2026-10-16.

- **Overlay (P2b):** Dear ImGui with our own DX8 backend `mod/src/ui/imgui_impl_dx8.*` (ported from imgui_impl_dx9: `SetVertexShader(FVF)`, `SetIndices(ib, base)`, per-frame `CreateStateBlock(D3DSBT_ALL)`/`ApplyStateBlock`, own `BeginScene`/`EndScene` inside the Present callback). F8 toggles (foreground only). `[Overlay] Enabled=1` in `dh2coop.ini`. Screenshots: `re/notes/p2b-overlay*.png` — 2026-10-16.
- **License trap:** mingw-w64's `d3d8.h` is a Wine import (LGPL, `__WINE_D3D8_H`). We ship an original minimal COM declaration in `mod/third_party/d3d8/` instead. Check provenance of any vendored header — 2026-10-16.
- Free-hunt start shows a "CHOOSE ITEM" weapon menu over the map before play; overlay draws above it — 2026-10-16.

## Orchestration
- Workers: Grok Build CLI `grok` pinned to `-m grok-4.7`, headless via `tools-src/grokjob.ps1` (prompt `re/jobs/<name>.md` + `_rules.md`). Permission mode `auto` works headless (runs shell, writes files). `bypassPermissions` is blocked by Claude Code's auto-mode classifier — don't use it — 2026-10-16.
- `--output-format plain` prints only at the end; a running job's log shows just the start line.
- Windows PowerShell 5.1 + `$ErrorActionPreference='Stop'` + `& native 2>&1` turns any stderr line into a terminating error. Grok writes "Auto-compacting conversation (81% full)..." to stderr, which killed Track B at ~25 min. Runner now uses `Start-Process -RedirectStandardOutput/-RedirectStandardError` (stderr → `re/jobs/<name>.err.log`); touch `$p.Handle` right after start or `ExitCode` comes back empty — 2026-10-16.
- `cursor-agent` also works (`grok-4.7-*` models listed) but was ~4× slower than `grok` on the same prompt (150 s vs 37 s) — 2026-10-16.
- Claude subagents can hit session usage limits mid-task and die without writing summaries. Mitigation: workers write deliverables incrementally; lead keeps `STATUS.md` current.
- Windows PowerShell 5.1 `Get-Content -Raw` reads BOM-less UTF-8 as ANSI; writing it back mangles `—`/`→` into `â€”`. Edit docs with Write/Edit tools, or pass `-Encoding UTF8` on both read and write — 2026-10-16.
- Only one worker per wave launches the game (shared sandbox, single disc mount, DirectInput exclusive mode).
- **Parallel workers + lead in one repo (2026-10-17):** a C++ worker goes in a git worktree (`git worktree add ..\dh2coop-<job> -b <job>`, its own `mod\build`, robocopy `game\` into it, launch from that tree); merge with `git cherry-pick`. Script-only and research workers stay in the main checkout and commit with `git commit -- <their paths>`. Every job spec says "check `Get-Process Main7`; wait, never close a game you did not start" — two workers and the lead shared the screen this way with no collisions.
- **Profiler label strings lead to methods:** retail GameDll has no symbols for most methods, but each one has a `"cClass::Method"` string referenced only from its `Catch@` funclet. `XrefStrings.java` finds the funclet, `FuncBefore.java` (nearest preceding non-funclet function) gives the method. That is how `cScrTriggerEvent::SkipCutScene` (`0x6a940`) and `cScrCutScene::Skip` (`0x8ed10`) were found — 2026-10-17.
- **Commit message files:** PowerShell 5.1 `Set-Content -Encoding utf8` writes a BOM, which lands in the commit subject. Use `[IO.File]::WriteAllText` (no BOM) for `git commit -F` files — 2026-10-17.
- **cursor-agent worker timing:** the story-inventory job (read ~350 script files, two Ghidra dumps, a 4000-line deliverable) took 15 min on `grok-4.7-medium-fast` with `--auto-review`, no permission stalls. A one-line smoke prompt answers in seconds - 2026-10-16.
