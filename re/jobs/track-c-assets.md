# Job: Track C — assets and scripts inventory (Ghidra slot C if needed)

A previous agent already reversed `data.arc` and wrote `re\scripts\arc_extract.py`; read its docstring for the format. All entries are extracted to `re\extracted\`, about 9,710 files. Top extensions: .DDS 5325, .SKIN 1019, .MODEL 996, .AP 606, .WAV 574, .SCR 344, .FX 270, .OUT 98, .SCRIPT 98, .XSC 85, .SPJ 85, .CFG 31, .BSP 29. Top-level folders: BSP, CFG, FONTS, FX, GRASS, INTERFACE, LEVELS, OBJECTS, RESOURCES, SCRIPTS, SHADERS, SOUND, TEXTURES, TYPEMAP, _mtp. The extracted data is gitignored; never copy it into tracked files beyond short quotes (at most 10 lines per file).

Tasks:
1. Run `py re\scripts\arc_extract.py --verify` and confirm the extractor is consistent. Record the result.
2. Identify every script and data format: .SCR, .SCRIPT, .XSC, .SPJ, .OUT, .AP, .CFG, .FX, .INC, .MAT. For each, say whether it is text, Lua source, Lua bytecode (header `1B 4C 75 61` plus a version byte) or a custom VM. Note that strings mention a custom script VM, `ScrVMCore.cpp`, which runs alongside Lua. Show a representative snippet of each type.
3. Build a map and level inventory: every BSP and level with its region (Alaska, Tanzania, India, Australia, Siberia), mission vs free-hunt status, and the files that define each mission (scripts, spawn tables, triggers). Recommend 1–2 free-hunt maps as the co-op MVP test map, preferring small maps with few scripted events and several predator types.
4. Search everything for multiplayer remnants: sv_multiplayer, cl_join, lobby, cfLobby, server, client, player2, maxplayers, coop, deathmatch, team, spawnpoint/info_player, "network". Inspect `INTERFACE` GUI definitions for any hidden multiplayer or lobby screens, and the CFG files for cvar defaults.
5. Document the console config: what the CFG files contain (cvar syntax and defaults) and whether a loose file in `game\Data\Cfg\` overrides the archive. Check the loader order in `cFileManager`/`cArcFile` via `re\notes\arc-decomp*.c`, or use Ghidra slot C.
6. Document how levels reference animal spawns and AI parameters. These feed the later "shared animals" phase.

Deliverable: `re\notes\assets.md`, with sections Archive format (short, point to the script), Verify result, File types, Map inventory and MVP map recommendation, Multiplayer remnants, Config/console files and override rules, Animal spawn data, Open questions, and Proposed MEMORY facts.
