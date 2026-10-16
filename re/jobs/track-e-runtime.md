# Job: Track E — runtime probe write-up + multiplayer surface experiments (Ghidra slot E; YOU OWN GAME LAUNCHES)

A previous agent built the mod skeleton and runtime probe before it was cut off. Read `mod\CMakeLists.txt`, `mod\build.ps1`, `mod\src\**` (especially `mod\src\probe\probe.cpp`) and the latest `game\Bin\dh2coop.log`. The probe works: D3D8 is the renderer (Direct3DCreate8 called 3 times, one D3D8 device, Present at EngineDll7.dll+0x354cfd). It also hooks cCmdLine::Search, cConsole::Exec, cBspTree::Load, cHunterProfile map functions, net counters and m_nDriverType.

You are the only job allowed to launch the game. Use `.\run-game.ps1 -Seconds <N> [-ExtraArgs '...']`; it mounts the disc image and runs `Main7.exe +mod data` in `game\Bin`. The user has approved the game window popping up. Kill `Main7.exe` if it hangs. Build with `mod\build.ps1`.

Tasks:
1. Confirm the mod builds cleanly from scratch with `mod\build.ps1`, and fix build issues if any. Record the toolchain and dependency versions.
2. Run a baseline 40-second launch. Document in `re\notes\renderer.md`:
   - renderer facts
   - module bases
   - hook points that installed
   - which probe hooks fire
   - the switch list
   - every console Exec seen
3. Multiplayer surface experiments. Run each one separately, saving `debug.log` and `dh2coop.log` copies as `re\notes\runtime-<exp>-debug.txt` and `re\notes\runtime-<exp>-probe.txt`. While the game runs, check bound UDP ports with `Get-NetUDPEndpoint -OwningProcess <pid>`.
   - a) `-ExtraArgs '-dedicated'`
   - b) `-ExtraArgs '-join 127.0.0.1'`
   - c) Create `game\Data\Cfg\console.cfg` containing `sv_multiplayer 1`, then launch normally. Check `debug.log` for whether the file was read. If the loose file is ignored, try `game\Data\console.cfg` as well. Delete the file afterwards.
   - d) `-ExtraArgs '-developper -cheats -map <a free-hunt map name>'`. Find a map name under `re\extracted\BSP` or `LEVELS`, and check whether it loads straight into a map.
   - e) If (a) or (c) bound a UDP port: start a second instance in a separate copy, `game2\` (robocopy `game\` to `game2\`, gitignored), with `-join 127.0.0.1`. Record whether a connection is attempted. Check the net counters in the probe log and the `debug.log` lines tagged `[NET]` or `[SERVER]`. Note that two instances may conflict over the mounted disc or input devices; record what happens.
4. If the console can be opened in-game (try the ` and ~ keys and F1–F12 while `-developper` is set), record how. If an experiment needs interactive keyboard input that you cannot send, write down the exact manual steps for the user instead.

Deliverable: `re\notes\renderer.md`, with sections Build, Renderer and hooks, Baseline run, MP experiments (one subsection each, with conclusions), Manual steps for the user (if any), Open questions, and Proposed MEMORY facts. Add `game2/` to `.gitignore` if you create it.
