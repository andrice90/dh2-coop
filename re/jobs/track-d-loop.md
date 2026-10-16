# Job: Track D — main loop, timing, console, switches (Ghidra slot D)

A previous agent was cut off before writing its summary. Its raw work is in `re\notes\loop-*.c` and `re\notes\loop-*.txt` (cApplication, frame, Main7 WinMain chain, console command registrations per DLL, command xrefs, map load, manager, slow-motion in GameDll7, vtables). Read those first, then fill gaps with `re\scripts\decomp.ps1 -Slot D`, `probe.ps1 -Slot D` or `ghrun.ps1 -Slot D`.

Known from the runtime probe (launched with `+mod data -developper -cheats`):
- The game checks command-line switches through `cCmdLine::Search` for `-exp`, `-spmap`, `-batchmode`, `-developper` (found), `-cheats` (found), `-map`, `-join`, `-testmodels` and `-debug`.
- GameDll7 runs `cConsole::Exec('gun_fov 60')` and `cConsole::Exec('menumap menumap')`.
- The map loads through `cBspTree::Load('Bsp\menumap.bsp')`, called from Main7.exe+0x10372.
- The engine probes `Cfg\console.cfg` at startup.

Tasks (give evidence for each):
1. Main loop order per frame (input, server tick, client, render, sound, wait). Answer whether it uses a fixed tick or a variable dt, where the global time, dt and time scale live (exported statics), and the `sv_fps` meaning.
2. Every command-line switch and what it enables, especially `-developper`, `-cheats`, `-map <name>`, `-join`, `-dedicated`, `-spmap` and `-exp`.
3. Console:
   - how to open it in-game (key binding), and whether it requires `-developper`
   - the full list of registered commands and cvars with defaults, from the conregs dumps
   - the `console.cfg` format and load path
   - cvars or commands that load a map, spawn entities, toggle god mode, noclip or AI, or relate to the network
4. Slow motion: how to force it off for everyone. Name the cvar, `mod_slowmotion*` settings or the function to patch. Describe the HUD element `cHUDSlowMotion`.
5. Map load entry point by name. What the EV_COMMAND / CM_* event system is and how GUI buttons dispatch commands.
6. Best mod hook points:
   - per-frame pre/post game tick
   - after map load
   - D3D8 present (the probe saw `EngineDll7.dll+0x354cfd` calling Present and `+0x354b63` calling EndScene)
   - the console command registration API, so our mod can register its own commands such as `coop_host` and `coop_join`

Deliverables:
- `re\notes\timing.md`
- `re\notes\console.md`: commands/cvars table, switches table, how to open the console, how a mod registers commands.
- Both files end with `## Runtime experiments requested`, giving exact steps for the runtime job, and `## Proposed MEMORY facts`.
