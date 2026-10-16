# Mod implementation learnings

Practical knowledge for building, injecting, hooking, testing and orchestrating. Engine facts live in `MEMORY.md`; this file is about *how to work*. Add an entry whenever something cost time to figure out. Format: learning — evidence/date.

## Toolchain
- VS 2022 Build Tools (MSVC 14.44.35207, Windows SDK 10.0.26100) at `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`. `vcvars32.bat` fails silently unless `${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer` (vswhere) is on PATH first — 2026-10-15.
- 32-bit build confirmed (`sizeof(void*)==4`) — 2026-10-15.
- winget's VS BuildTools install "fails" with exit 1 when it is already installed; check `dd_installer_*.log` for "already installed" before assuming breakage — 2026-10-15.
- Mod deps (CMake FetchContent, `mod/CMakeLists.txt`): safetyhook v0.7.0, spdlog v1.17.0, Dear ImGui v1.93.0, ENet v1.3.18. Static CRT (`/MT`), output `dh2coop.asi`. Build: `mod\build.ps1` (copies ASI + ini to `game\Bin`).
- Python: use `py` (3.14); the `python` alias is the Microsoft Store stub. `pefile` installed for user.
- Ghidra 12.1.4 + Temurin JDK 21 (`JAVA_HOME` set at user level). Headless projects: master `re/ghidra/DH2`, per-worker copies `re/ghidra-slots/<Letter>` (a Ghidra project allows one process at a time). `re/scripts/decomp.ps1` ≈ 7 s per call.

## Injection and launch
- Ultimate ASI Loader v9.7.4 works as `game\Bin\dinput8.dll` (EngineDll7 imports DINPUT8, so the proxy loads early). Our `dh2coop.asi` and the AlphaYellow FOV fix coexist — 2026-10-15.
- Launch must be `Main7.exe +mod data` with working dir `Bin`; otherwise the home path is wrong and nothing loads. Use `run-game.ps1` (auto-mounts the disc image via `VCDMount.exe`) — 2026-10-15.
- All engine DLLs relocate (EngineDll7 seen at 0x00CA0000; GameDll7 at 0x0E570000). Resolve by export name or pattern, log as `Module+RVA` — 2026-10-15.
- `GameDll7.dll` is loaded late (≈30 s after process start in a `-developper -cheats` run, after intro/menu setup) and from `game\data\`. The probe catches it with `LdrRegisterDllNotification`; hooks into GameDll7 must be installed from that callback, not at ASI init — dh2coop.log 2026-10-15.

## Testing loop
- Add `-nointro` to skip intro movies (switch confirmed in Main7 by Track D; effect on load time not yet measured) — 2026-10-16.
- Console from our code: `cConsole::Exec`/`RealExec` via the exported `console` pointer; in-game console needs A+V+F12 (or Q+I+F12), then `` ` ``, then password `3.141692` as the first line — 2026-10-16.
- **Fast dev loop:** `.\run-game.ps1 -Seconds 60 -ExtraArgs '-developper -cheats -spmap HUNTMAP4'` loads straight into a hunt map (~30 s, mostly intro/menu init before GameDll7 loads). `-map` does NOT work — 2026-10-16.
- **Windowed mode:** put any loose `game\Data\Cfg\console.cfg` in place → 640×480 windowed (needed to run two instances side by side). The game rewrites the file on exit, so keep a template copy elsewhere and restore it before each run. Without the file: fullscreen 640×480 @ 240 Hz — 2026-10-16.
- Injected keys (`keybd_event` with scan codes) don't reach the game (DirectInput). In-game console key unconfirmed; test by hand (`` ` `` then F1–F12 with `-developper`). Prefer driving the game through our ASI (`cConsole::Exec`) over synthetic input.
- Second process on one PC: `"GI UNIQUE MUTEX"` blocks it unless `-join`/`-dedicated` is on the command line (decompile; not yet tested). `-join x` harmlessly execs a missing `cl_join` command, so `-join 0` is a workable "allow second instance" flag until our ASI handles the mutex.
- UDP check while the game runs: `Get-NetUDPEndpoint -OwningProcess <pid>` plus `netstat -ano -p udp`.
- `re/scripts/runtime-exp.ps1` (Track E) wraps launch + log capture + UDP sampling per experiment.

## Hooking
- safetyhook inline hooks on mangled exports (resolved with `GetProcAddress`) work: `cCmdLine::Search`, `cConsole::Exec`, `cBspTree::Load`, `cHunterProfile::{GetMapToBeStarted,GetLastmap,SetLastMap,SetCurrentMission,SetMapIsComplete}` — dh2coop.log 2026-10-15.
- `cApplication::MainLoop` (EngineDll7 RVA `0x9e0b0`) was hooked and never entered — the real frame loop is elsewhere (Track D). Use the D3D8 `Present` hook as the per-frame callback until a game-tick hook is found — 2026-10-16.
- To see console commands from cfg files, hook `cConsole::RealExec` as well as `cConsole::Exec` — 2026-10-16.
- Build warning to ignore: safetyhook `os.windows.cpp` C4005 `NOMINMAX` redefinition; safetyhook v0.7.0 bumps the target to `/std:c++latest` — 2026-10-16.
- Renderer is D3D8: one `IDirect3DDevice8`; `Present` called from `EngineDll7+0x354cfd`, `EndScene` from `EngineDll7+0x354b63`. `Direct3DCreate8` is called 3×, `Direct3DCreate9` 1× (probe/enumeration only; no D3D9 device) — dh2coop.log. Overlay (ImGui) must use a DX8 path (ImGui has no DX8 backend: use d3d8to9 or a custom DX8 renderer).

## Orchestration
- Workers: Grok Build CLI `grok` pinned to `-m grok-4.7`, headless via `tools-src/grokjob.ps1` (prompt `re/jobs/<name>.md` + `_rules.md`). Permission mode `auto` works headless (runs shell, writes files). `bypassPermissions` is blocked by Claude Code's auto-mode classifier — don't use it — 2026-10-16.
- `--output-format plain` prints only at the end; a running job's log shows just the start line.
- Windows PowerShell 5.1 + `$ErrorActionPreference='Stop'` + `& native 2>&1` turns any stderr line into a terminating error. Grok writes "Auto-compacting conversation (81% full)..." to stderr, which killed Track B at ~25 min. Runner now uses `Start-Process -RedirectStandardOutput/-RedirectStandardError` (stderr → `re/jobs/<name>.err.log`); touch `$p.Handle` right after start or `ExitCode` comes back empty — 2026-10-16.
- `cursor-agent` also works (`grok-4.7-*` models listed) but was ~4× slower than `grok` on the same prompt (150 s vs 37 s) — 2026-10-16.
- Claude subagents can hit session usage limits mid-task and die without writing summaries. Mitigation: workers write deliverables incrementally; lead keeps `STATUS.md` current.
- Windows PowerShell 5.1 `Get-Content -Raw` reads BOM-less UTF-8 as ANSI; writing it back mangles `—`/`→` into `â€”`. Edit docs with Write/Edit tools, or pass `-Encoding UTF8` on both read and write — 2026-10-16.
- Only one worker per wave launches the game (shared sandbox, single disc mount, DirectInput exclusive mode).
