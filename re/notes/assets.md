# Assets and scripts (Track C)

Inventory of retail `game\Data\data.arc` after extract to `re\extracted\` (gitignored). Counts and quotes below are from that tree plus `re\scripts\arc_extract.py`, `re\notes\arc-decomp*.c`, `re\notes\script-load-decomp.c`, and `re\notes\loop-decomp-console.c`. Helper: `re\scripts\asset_inventory.py`. Ghidra slot C was not required.

## Archive format

See the docstring in `re\scripts\arc_extract.py`. Short form, already confirmed against `EngineDll7.dll` `cArchive` / `cArcFile` and the retail file:

- 2048-byte sectors. Each entry is one zlib stream (`78 9C`) or stored raw, and starts on a sector boundary.
- 24-byte footer on the last data sector: `ARC\0`, version `0x100`, directory sector, uncompressed size, compressed size, entry count.
- Directory is a zlib stream of `entry_count` records: magic `ARCH`, first sector, sector count, sizes, compression flag (`-1` zlib, `0` stored), NUL-terminated upper-case name with `\` separators.
- `.mtp` blobs are a second container, magic `MULA` ("MltiFile"). `shaders.mtp` is the one in this archive (98 inner files). The startup log reads it: `[SYSTEM] === MltiFile 'shaders.mtp' ===` and `read 407789 bytes ... in 98 files` (`game\Bin\debug.log` lines 17-18).

Mount path in the engine: `cFileManager::AddHomeDirectory` (RVA `0x33cd50`) searches the home directory for the literal `.ARC` (`EngineDll7.dll` RVA `0x43f1e0`) and opens each hit with the archive reader at RVA `0x335640`, which checks footer magic `0x00435241` (`ARC\0`) and per-entry magic `0x48435241` (`ARCH`). Names are upper-cased and `/` is turned into `\` before lookup (`FUN_103380a0`, RVA `0x3380a0`).

## Verify result

Command: `py re\scripts\arc_extract.py --verify` on `game\Data\data.arc`.

```
data.arc: 294207488 bytes, version 0x100, 9612 entries, dir sector 143604 (103867->724278 bytes)
verify: 9612 entries, 0 problems, data end sector 143604 (dir at 143604)
```

Zero gaps, overlaps, duplicate names, sector-count mismatches, or zlib/size failures. The extractor is consistent with this archive.

On disk under `re\extracted\`: **9710 files** = 9612 archive entries (including `shaders.mtp`) plus the 98 inner MTP files. Extension totals: `.DDS` 5325, `.SKIN` 1019, `.MODEL` 996, `.AP` 606, `.WAV` 574, `.SCR` 344, `.FX` 270, `.OUT` 98, `.SCRIPT` 98, `.XSC` 85, `.SPJ` 85, `.SND` 39, `.MAT` 38, `.CFG` 31, `.BSP` 29, `.TGA` 20, `.GRASS` 17, `.SPL` 13, `.INC` 10, `.FNT` 5, `.RES` 2, `.TXT` 2, `.MTP` 1, `.LUA` 1, `.EXP` 1, `.DAT` 1.

Top folders: `LEVELS` 3597, `OBJECTS` 2898, `TEXTURES` 1437, `SOUND` 604, `SCRIPTS` 451, `FX` 380, `INTERFACE` 120, `_mtp` 98, `TYPEMAP` 38, `SHADERS` 35, `BSP` 29, `GRASS` 17, `FONTS` 3, `CFG` 1, `RESOURCES` 1.

## File types

Two script systems sit side by side. Lua 4.0 bytecode is only the `.OUT` files (header `1B 4C 75 61`, version byte `0x40` on all 98). The custom VM is `cVMachine` in `ScrVMCore.cpp` (source path string in EngineDll7). `cVMachine::LoadInfo` (RVA `0x287950`) rejects any script whose version byte is not 3 (`Script version incorrect: %i (required: %i)`, string at Engine RVA `0x432914`). `cVMachine::SmartLoadPrj` (RVA `0x2870d0`) appends `.xsc` (RVA `0x43164c`) and falls back to `.spj` (RVA `0x432858`), and opens `scripts/scripter.exp` when it has to compile.

| Ext | Count | What it is |
|---|---:|---|
| `.SCR` | 344 | Custom VM **source**. Text. Class syntax (`class cGBear extends cAnimalBrain`). |
| `.SPJ` | 85 | Custom VM **project**. Text list of `.SCR` paths, one per line. |
| `.XSC` | 85 | Custom VM **bytecode**. Not Lua. All 85 start `46 53 01 01` and byte offset 9 is `03`. Banner: `Script Compiler (build 402 - Sep 30 2005)`. |
| `.SPL` | 13 | Same kind of text file list as `.SPJ` (weapon packs and console save-UI variants). |
| `.EXP` | 1 | `SCRIPTS\SCRIPTER.EXP`. Binary export table. First class string is `cBaseClass`. Header `2F 00 00 00`. |
| `.SCRIPT` | 98 | **Lua source** for shaders (comment style `--`, `function`, `BeginShader`). |
| `.OUT` | 98 | **Lua 4.0 bytecode** compiled from those scripts. Names like `DNGH2_FX.SCRIPT.OUT`, all under `_mtp\`. |
| `.LUA` | 1 | **Lua source**: `SCRIPTS\MATERIALS.LUA` (`DefineMaterial` / `Set`). |
| `.FX` | 270 | **Text FX scripts** in a Lua-like dialect (`function beez()`, `FXInfo`, `FXLife`). Not bytecode. |
| `.INC` | 10 | **Text** GUI/string includes (`language = "En"`, `--` comments). |
| `.CFG` | 31 | **Text**. One console cvar file; thirty Lua-like FX color files. See config section. |
| `.SND` | 39 | **Text** sound scripts (`LoadScript`, `function MenuHighlight_a()`). |
| `.AP` | 606 | **Not a script.** Magic `46 42 41 46` (`FBAF`) plus `Make Model (build 402 - Sep 30 2005)`. Per-map animation pak. `AddAnimPakForAI("bear", "map1.ap")` in `SCRIPTS\LEVELS\MAP1\WORLD.SCR`. |
| `.MAT` | 38 | **Binary** typemaps under `TYPEMAP\`. First bytes `00 00 01 00`. Terrain mats are `MAP*.MAT` / `HUNTMAP*.MAT`. Eight files are `EVENT_*.MAT`. Record layout **UNVERIFIED**. |
| `.BSP` | 29 | **Binary** level. Every file starts `0C 00 00 00` (chunk id 12, `CHUNK_INFO`, matching `debug.log`). Entity text is embedded as ASCII `entity("...")` / `property("...")`. |
| `.DAT` | 1 | `TEXTURES\TEXTURE.DAT`, 40 bytes, header `0A 00 00 00`. Purpose **UNVERIFIED**. |

Stem pairing: 56 paths have `.SCR`+`.SPJ`+`.XSC` together (animals, NPCs, both GUI menus). Every level folder under `SCRIPTS\LEVELS\` has `MAIN.SPJ` + `MAIN.XSC` + `WORLD.SCR`. The 98 `.SCRIPT` sources and 98 `.OUT` bytecodes are a pair; the bytecode lives in the MTP pack.

`.SCR` sample, `SCRIPTS\MODELS\ANIMALS\ANIMAL.SCR`:

```
class cAnimalBrain extends cScrAnimal
{
	void	InitModel()
	{
	}
	void SetProps()
	{
	}
	void	Spawn()
	{
		InitModel();
		AddHitBones();
		SetProps();
	}
```

`.SPJ` sample, `SCRIPTS\LEVELS\HUNTMAP1\MAIN.SPJ` (first lines):

```
scripts/levels/mapworldbase.scr
scripts/levels/huntmapbase.scr
scripts/levels/huntmap1/world.scr
scripts/levels/LensFlare.scr
scripts/models/Characters/humanlegs.scr
scripts/models/Characters/human.scr
```

`.XSC` header, `INTERFACE\SCRIPTS\GUIGAMEMENU\GAMEMENU.XSC` (same prefix on all 85):

```
46 53 01 01 34 00 00 00 00 03 53 63 72 69 70 74
FS..4.....Script Compiler (build 402 - Sep 30 2005)
```

`.OUT` header, `_mtp\SHADERS\FX\SHADERS\DNGH2_FX.SCRIPT.OUT`: `1B 4C 75 61 40` then Lua 4.0 bytecode fields. The matching source is `FX\SHADERS\DNGH2_FX.SCRIPT` (`function fx_zlensflare()` / `BeginShader()`).

`.FX` sample, `FX\SCRIPTS\BEEZ.FX`:

```
function beez()
 FXInfo( 502281643, "beez", CLASSID_CFXPS );
 FXAbout( "animated explosion" );
 FXLife( 36000, 36000, 0, 1, 1, 0 );
```

`.LUA` sample, `SCRIPTS\MATERIALS.LUA`: `DefineMaterial("Material0");` / `Set( "ray_hit", "0" );`. GameDll7 string `Scripts/Materials.lua` matches this path. The `MaterialManager` Lua state in `debug.log` is the likely consumer. Binding **UNVERIFIED** beyond the name match.

`.AP` header, `OBJECTS\ANIMALS\ASIATICBEAR\MAP6.AP`: `FBAF` + `Make Model (build 402 - Sep 30 2005)`.

`.MAT` header, `TYPEMAP\HUNTMAP1.MAT` and `TYPEMAP\EVENT_MAP1.MAT`: `00 00 01 00 ...`. `SCRIPTS\LEVELS\MAPTSAVO\WORLD.SCR` calls `LoadMat("Typemap/maptsavo.mat")` and `LoadEventMat("Typemap/event_maptsavo.mat")`. The terrain file exists. The event file does not. The same `LoadEventMat` call is present for 21 maps, and only these event files are in the archive: `EVENT_MAP1`, `EVENT_MAP2A`, `EVENT_MAP2C`, `EVENT_MAP3B`, `EVENT_MAP7`, `EVENT_MAP8`, `EVENT_MAP9`, `EVENT_MAP12A`. Whether a missing event mat is fatal is **UNVERIFIED**.

## Map inventory and MVP map recommendation

29 BSPs in `BSP\`. Each `world_properties` entity points at `scripts/levels/<name>/main.spj` and class `cMapTest` (menu map uses `scripts/levels/mapmenu/main.spj`). UI strings are `{id, text}` rows in `INTERFACE\TEXT\TEXTTABLE.INC`.

Region labels that actually appear as map text:

| BSP | UI text | Kind |
|---|---|---|
| `HUNTMAP1` | Hunt Area Africa | free-hunt (`SetMission(13)`) |
| `HUNTMAP2` | Hunt Area Australia | free-hunt (`SetMission(14)`) |
| `HUNTMAP3` | Hunt Area Alaska | free-hunt (`SetMission(15)`) |
| `HUNTMAP4` | Hunt Area Argentina | free-hunt (`SetMission(16)`) |
| `MAPARENA` | Bonus: Aztec Temple | bonus (`SetMission(17)`) |
| `MAPISLAND` | Bonus: Island | bonus (`SetMission(18)`) |
| `MAPTSAVO` | Bonus: Savannah | bonus (`SetMission(19)`) |
| `MENUMAP` | (no row; log loads `menumap`) | menu |

A text search of `.SCR` / `.SCRIPT` / `.INC` / `.TXT` / `.LUA` / `.FX` found no map label "Tanzania" or "India". "Siberia" appears as the NPC class `cScrSiberian1` in `SCRIPTS\MODELS\NPC\GUIDE\GUIDE.SCR` and as instances in `MAP11A` / `MAP11C` world scripts, plus a comment in `SHADERS\GLOBE.SCRIPT`. It is not a hunt-area name. Africa / Australia / Alaska / Siberia / India also appear as comments in `SHADERS\GLOBE.SCRIPT`. Treating story chapters as those countries is **UNVERIFIED**.

Story BSP names follow the mission files. `SCRIPTS\MISSIONS\MISSION01.SCR` calls `SetMission(1)` and the chapter UI id is `map1` = "Training Day". The same pattern holds for missions 2-12 (`MISSION02A` is `SetMission(2)`, UI `map2a` = "Ole One Eye", through `MISSION12A` / `SetMission(12)` / "Showdown!"). Letter suffixes (`MAP2B`, `MAP11C`, `MAP12C`, ...) are extra BSPs in the same chapter: `MISSION02B.SCR` calls `SetMissionId(2)` and does not call `SetMission`. Each story chapter that has cutscenes has `SCRIPTS\CUTSCENE\MAP*\CUTSCENE*.SCR`. `MAP2D.BSP` does not exist. `TYPEMAP\MAP2D.MAT` and `FX\HFXCFG\HFXMAP2D.CFG` do. `MAP2C\WORLD.SCR` asks for `Typemap/event_map2d.mat`, which is also absent.

Entity counts are ASCII `entity("...")` records inside the BSP. "Animals" is the uncommented `AddAnimal` list in that map's `WORLD.SCR`.

| BSP | Bytes | Spawn | Hunt pts | Triggers | Waypoints | Animals placed by script |
|---|---:|---:|---:|---:|---:|---|
| HUNTMAP1 | 1219352 | 2 | 20 | 18 | 72 | 4 impala, 1 leopard |
| HUNTMAP2 | 1054168 | 1 | 25 | 20 | 73 | 4 axis deer, 1 dingo |
| HUNTMAP3 | 1399000 | 1 | 24 | 19 | 72 | 4 whitetail, 1 grizzly |
| HUNTMAP4 | 872952 | 2 | 22 | 20 | 72 | 4 red deer, 1 puma |
| MAP1 | 387048 | 6 | 2 | 12 | 49 | black bear, mountain lion, 4 whitetail |
| MAP2A | 608064 | 19 | 5 | 37 | 13 | 3 grey wolf, 2 mountain lion, 1 grizzly |
| MAP2B | 743248 | 44 | 7 | 30 | 58 | 6 grey wolf, 1 mountain lion, 1 grizzly |
| MAP2C | 639616 | 13 | 5 | 10 | 48 | 2 grey wolf, 1 grizzly, 1 wild boar |
| MAP3A | 488856 | 27 | 0 | 18 | 6 | 4 hyena, 2 leopard, 5 water croc |
| MAP3B | 395544 | 13 | 0 | 8 | 1 | 2 leopard, 3 cobra |
| MAP3C | 475640 | 12 | 0 | 6 | 6 | 2 lioness, 1 lion, 4 cobra |
| MAP4A | 1362520 | 12 | 1 | 11 | 50 | 3 lioness, 3 lion |
| MAP4B | 1162232 | 22 | 3 | 20 | 2 | lions, 1 hippo, 5 water croc, 4 pantelope |
| MAP5 | 1323784 | 35 | 3 | 20 | 78 | 2 hyena, 2 elephant |
| MAP6 | 1226680 | 12 | 1 | 12 | 79 | 4 Bengal tiger, 1 Asiatic bear |
| MAP7 | 1576440 | 14 | 3 | 20 | 91 | 4 Bengal tiger, 1 rhino, 1 cobra |
| MAP8 | 1189240 | 30 | 10 | 31 | 23 | 3 croc, 3 water croc, 5 scrub bull |
| MAP9 | 626552 | 20 | 2 | 20 | 10 | 3 dingo (camel line is commented out) |
| MAP10 | 705360 | 46 | 1 | 22 | 85 | 4 puma, 6 wild boar |
| MAP11A | 1335528 | 14 | 0 | 12 | 1 | 6 wild boar |
| MAP11B | 292280 | 13 | 2 | 12 | 33 | 6 wild boar, 1 hogzilla |
| MAP11C | 1220976 | 13 | 0 | 12 | 1 | 4 wild boar |
| MAP12A | 501104 | 33 | 3 | 18 | 74 | 6 grey wolf, 1 polar bear, 2 wild boar |
| MAP12B | 335680 | 7 | 0 | 4 | 1 | 1 biggrim |
| MAP12C | 108536 | 2 | 0 | 2 | 1 | 1 yeti |
| MAPARENA | 377472 | 8 | 0 | 8 | 3 | 2 leopard, 2 scrub bull, 3 wolf, 2 lion, 1 grizzly, 1 rhino |
| MAPISLAND | 702712 | 8 | 0 | 10 | 3 | 2 Bengal tiger |
| MAPTSAVO | 450352 | 8 | 0 | 12 | 3 | 1 lion, 1 lioness |
| MENUMAP | 313384 | 1 | 0 | 0 | 0 | none |

Hunt-area worlds are long (about 730 lines) and share `cScrAnimalController`, `cScrAnimalEventManager`, and eight `cScrScenarioManager*` classes. Each hunt BSP also has about 20 `hunt_area_point` entities named `predator01` and up, with `property("Animal names", "")` empty. The script then places one predator under the entity name `bear01`. So the hunt areas are scripted predator-attack maps, and each one only places a single predator species.

Files that define a map, using huntmap1 as the pattern:

- `BSP\HUNTMAP1.BSP` — geometry, entity list, link to the project.
- `SCRIPTS\LEVELS\HUNTMAP1\MAIN.SPJ` and `MAIN.XSC` — project and compiled VM image.
- `SCRIPTS\LEVELS\HUNTMAP1\WORLD.SCR` — entity binds and `AddAnimal`.
- `SCRIPTS\LEVELS\HUNTMAPBASE.SCR` — shared hunt globals (`szPredatorSpawnPoint`, `BindAnimal`, `cScrAnimal the_predator`).
- `SCRIPTS\TRIGGERS\TRIGGERHUNTMAP1.SCR` — trigger classes.
- `SCRIPTS\MISSIONS\MISSIONHUNTMAP1.SCR` — objective (`SetTaskAnimalToShoot(ANIMAL_LEOPARD, ANIMAL_KILL, 1)`).
- `TYPEMAP\HUNTMAP1.MAT` — terrain typemap. Event mat is referenced and missing.
- `FX\HFXCFG\HFXHUNTMAP1.CFG` — per-map color grades.
- `LEVELS\COLORMAP\HUNTMAP1\` and `LEVELS\BILLBOARDS\HUNTMAP1_BILLBOARDS.SCRIPT` — terrain and billboard shaders.

Bonus and story maps use the same shape with `MAPWORLDBASE.SCR` instead of `HUNTMAPBASE.SCR`, plus `SCRIPTS\CUTSCENE\<MAP>\` on story maps. NPC model projects (`scripts/models/npc/guide/guide.spj`) are extra BSP script properties on maps that spawn a guide.

**MVP recommendation.** Use `MAPARENA` first, then `HUNTMAP4` if the test has to be a map the UI calls a hunt area.

`MAPARENA` is a bonus level (`SetMission(17)`, "Bonus: Aztec Temple"), 377 KB, 27 entities, no `hunt_area_point`, 8 `info_spawn_point`, 8 triggers, and no guide NPC project on the BSP. Its `AddAnimal` block is the only small map that already places several predator types together (leopard, wolf, lion, grizzly, rhino, plus scrub bulls). It still has task triggers: `SCRIPTS\TRIGGERS\TRIGGERMAPARENA.SCR` class `cScrTask01` calls `::globals.player.StartTask("1")` on activate, and `MAIN.SPJ` includes `MissionMapArena.scr`. It is the smallest multi-predator sandbox in the archive, and it is not empty of scripts.

`HUNTMAP4` is the smallest official hunt area (873 KB, Argentina, UI "Hunt Area Argentina"): one puma, four red deer, 22 hunt points, and the full scenario-manager set. `HUNTMAP2` (Australia, one dingo, four axis deer, 1.05 MB) is the next smallest. Neither hunt area places several predator species. `MAP12C` is smaller (109 KB) and is the yeti story ending, so it is a poor co-op sandbox.

## Multiplayer remnants

Searched extracted `.SCR`, `.SCRIPT`, `.SPJ`, `.CFG`, `.FX`, `.INC`, `.TXT`, `.LUA`, `.XML`, `.BSP`, `.XSC`, `.OUT`, and `.AP` for `sv_multiplayer`, `cl_join`, `cfLobby`, `maxplayers`, `deathmatch`, `info_player`, `spawnpoint`, `player2`, `multiplayer`, `coop`, `co-op`, `dedicated`, `sv_port`, `sv_password`, `cl_password`, `cl_disconnect`, `cl_playername`, `lobby`, `server`, `client`.

Asset hits:

- `spawnpoint` only as the script field `szPredatorSpawnPoint` in `SCRIPTS\LEVELS\HUNTMAPBASE.SCR` and the four hunt `WORLD.SCR` files (and the same identifier inside the four hunt `MAIN.XSC` blobs). That is the predator attack spawn, not a deathmatch spawn.
- `network` once, in `INTERFACE\TEXT\TEXTTABLE.INC`, inside the credits string `network administration`.
- `team` only inside the word "steam" in FX descriptions and the credits string `engine team`.

No asset file contains `sv_multiplayer`, `cl_join`, `cfLobby`, `lobby`, `player2`, `maxplayers`, `deathmatch`, `coop`, `server`, or `client`.

Player placement in BSP data is `entity("info_spawn_point")` with `property("target_name", "human")`. There is no `info_player` or `info_player_start` string in the BSPs.

GUI projects are `INTERFACE\SCRIPTS\GUIMAINMENU\` (`cGuiMainMenu`, `cGuiRoundList`) and `INTERFACE\SCRIPTS\GUIGAMEMENU\` (`cGuiGameMenu`, `cGuiFailMenu`, `cGuiSuccesMenu`, `cGuiTasks`, `cGuiWeaponMenu`). Option pages in `GUICOMMON\GUIRENDEROPTIONS.SCR` are video, new game, profile, mission list, animal list, arena animal list, controls, gameplay. There is no class whose name contains lobby, join, server, multi, or coop. `cGuiCommonPlayerOptions` is a single-player options page.

Console-port leftovers, not PC netplay: `INTERFACE\SCRIPTS\GUICOMMON\SAVE\` has parallel `GCN`, `PC`, `PS2`, and `XBOX` trees, and `.SPL` files list those `.SCR` paths. `GUICOMMON\COMMONTEXT.INC` still has DualShock and Xbox controller error strings.

The multiplayer cvars named in `MEMORY.md` (`sv_multiplayer`, `cl_join`, `cfLobby`, `-join`) stay where that note put them: string literals in `Main7.exe` / `EngineDll7.dll`, not in `data.arc`.

One code remnant outside the archive: `Main7.exe` `FUN_00401dd0` creates mutex `GI UNIQUE MUTEX` and, on `ERROR_ALREADY_EXISTS` (`0xB7`), keeps going only when `-dedicated` or `-join` is on the command line. Otherwise it jumps to `LAB_00402115` (the early-out path in `loop-decomp-main7.c`). What that early-out does to a second process is **UNVERIFIED** here.

## Config, console files, and override rules

`CFG\CONSOLE.CFG` (2640 bytes, 59 lines) is the only console file in the archive. Syntax is one statement per line:

```
addvar r_width int; r_width = 640
addvar cam_fov int; cam_fov = 60
addvar input_mouse_speed float; input_mouse_speed = 1
```

`cConsole::Load` (RVA `0x1672c0`) opens the path through `cFileManager::Open` and runs each non-empty line through `RealExec`. `InitConsole` (RVA `0x329ec0`) always calls `cConsole::Load(console, "Cfg\\console.cfg")`. There is no `sv_*` or `cl_*` assignment in this file. Defaults that matter for later work: `cam_fov = 60`, `r_width = 640`, `r_height = 480`, `r_windowed = 0`, `r_api = 1`, `snd_useSound = 1`. The rest are render, input, and BSP draw switches (`bspDrawDetail`, `bspUseLightmap`, `bspDrawSky`, `bspDrawGrass = 0`, `bspDrawRain`, water reflect/refract, shadow LOD).

The other 30 `.CFG` files are `FX\HFXCFG\HFX<MAP>.CFG` plus `HFXUTIL.CFG`. They are Lua-like color tables (`SNOW_COLOR = COLORMULTI(...)`), one per BSP including `HFXMAP2D.CFG`, not console cvars.

Load order, from code:

1. `Main7.exe` `FUN_00401dd0` calls `InitPakFileManager(".zip", 2)` (Engine RVA `0x340b70`). That stores the extension `.ZIP` on the file manager (upper-cased in the constructor).
2. The same function then calls `FUN_00401470` (RVA `0x1470`), which builds `<install-or-.\..>\<mod>\` and calls `cFileManager::AddHomeDirectory(path, 0)`. The sandbox log records `[APP] Home Path = '.\..\data\'` (`debug.log` line 7). An optional `+path` argument calls `AddHomeDirectory` again with flag 0, which appends that tree after the first one.
3. With flag 0, `AddHomeDirectory` appends providers in this order: the directory provider (object type 2, `FUN_10339a10`), then one archive provider per filename containing `.ARC` (type 4, `FUN_10335450`, reader RVA `0x335640`), then one provider per filename containing `.ZIP` (type 3, `FUN_10341d40`).
4. `cFileManager::Open` (RVA `0x33e230`) walks that list from index 0 and returns the first provider that opens the path. The list object is the one at `cFileManager+0x1c` (`FUN_1033d660` / `FUN_1033dd80` / `FUN_1033d7a0`). Upper-casing happens before the walk.
5. Still in `FUN_00401dd0`, after the home path is registered: `-dedicated` sets `DAT_0043d2bc = 1`. `InitConsole` runs next, unless `-shaderdup` was passed. If `DAT_0043d2bc != 0`, the next call is `cConsole::RealExec("r_api 0; snd_usesound 0")`, which turns rendering and sound off for that process. Shutdown saves `console.cfg` only when that flag is still 0 (`FUN_0040cca0`).

So a loose file under the home directory is registered before `data.arc`. `Cfg\console.cfg` resolves to `game\Data\Cfg\console.cfg` when the process is started with home `.\..\data\`. `cFileManager::GetFullPath` (RVA `0x33d330`) concatenates the first home path with the relative path. Main7 shutdown (`FUN_0040cca0`, RVA `0xcca0`) calls `cConsole::Save` on that full path when `DAT_0043d2bc == 0`.

At the start of this job, `game\Data\Cfg` was absent, so the archive `CFG\CONSOLE.CFG` was the only copy. Later the same day `game\Data\Cfg\console.cfg` existed (2639 bytes, not byte-identical to the archive; its first line is `addvar cam_fov int; cam_fov = 90`). This track did not write that file and did not launch the game. The path and the size difference match a disk save beside the archive. A controlled read-priority test was not run. The directory provider's own `Open` body (vtable of `PTR_FUN_1043ef14`) was not decompiled here; the registration order and the first-hit loop are what the override claim rests on.

Other loose trees already under `game\Data\`: `Interface\`, `Movies\` (`.bik`), `Sound\Stream\`, plus `GameDll7.dll` and `data.arc`. Those disk files sit in front of same-named archive entries for the same reason.

`.mtp` is special-cased inside `cFileManager::Open`: extension `MTP` and flag bit `0x200` clear selects `cMultiFile` before the provider walk.

## Animal spawn data

Pipeline, verified from BSP strings and the level scripts:

1. BSP `world_properties` names the `.spj` and the class `cMapTest`.
2. BSP `info_spawn_point` / `target_name` `human` is the hunter spawn. `info_way_point` records are the path network (`target` / `target_name` pairs such as `s1_1wp1`). `hunt_area_point` records exist on hunt maps and a few story maps; every `Animal names` property found is an empty string. Species are not stored in that BSP field.
3. The `.spj` pulls in `WORLD.SCR`, the mission script, the trigger script, and the animal/character `.scr` files.
4. `WORLD.SCR` declares `cScrAnimal <name> <instance>`, binds it with `FindEntScrObject("<entity>")` or `BindAnimal`, then `AddAnimal(ANIMAL_<SPECIES>, TRUE, "<entity>")`. Some maps also call `AddAnimPakForAI("<entity>", "<map>.ap")` and `AddEvent(EVENT_ANIMAL, ANIMAL_..., "animal")`.
5. The species script sets the body and the brain. `SCRIPTS\MODELS\ANIMALS\GBEAR\GBEAR.SCR` class `cGBear extends cAnimalBrain`:

```
SetModel("Objects/Animals/GBear/gbear.skin");
SetHealth(350.0);
SetPhysDimensionsXZ(1.8, 0.7);
SetRealFxMass(520.0);
Phys_SetMass(100.0);
SetBrain("AIHuntBrain_Bear");
```

`SetProps` then calls `SetBrainParam` for `sigh`, `hear`, `smel`, `agre`, `carn`, `soci`, `sens`, `lure`, `routine`, `rspeed`, `wspeed`, `runhealth`, `maximpspd`, `bulletmul`, `speedmul`, starts `AIHuntTask_AttackBears`, and sets task params `dmgint`, `lratspd`, `cratspd`, `rtspd`, idle times, and distances. `Make_Scared` sets health 10000. `Make_Aggressive` sets health 500. The base `cAnimalBrain::Spawn` in `ANIMAL.SCR` calls `InitModel`, `AddHitBones`, `SetProps`. Hit bones use `AddHitBone("BipHead", 1000, 0)` style damage weights.

6. Hunt attack entry, `HUNTMAP1\WORLD.SCR`: `setidx(the_predator, FindEntScrObject("bear01"))`, then `MoveToPos(szPredatorSpawnPoint)`, `StartTask("AIHuntTask_AttackCats")`, `SetTaskParam("target","hunter")`. Alaska's hunt map uses `AIHuntTask_AttackBears`. Australia uses `AIHuntTask_AttackDogs`. The task class names match exports already listed for CommonDll7 (`cAIHuntTask_AttackCats` and the bear tasks in `class-catalog.md`).
7. Mission scripts do not spawn the animals. They score them: `SetTaskAnimalToShoot(ANIMAL_LEOPARD, ANIMAL_KILL, 1)` in `MISSIONHUNTMAP1.SCR`, and the same call with other `ANIMAL_*` tokens in the story mission files.
8. `ANIMAL_LEOPARD`, `ANIMAL_GBEAR`, and the rest are used and never defined in the text scripts. They are VM symbols. `SCRIPTS\SCRIPTER.EXP` is the export file `SmartLoadPrj` opens. Numeric values are **UNVERIFIED**.
9. Model mesh is `OBJECTS\ANIMALS\<NAME>\*.SKIN` / `*.MODEL`. The brain name string `AIHuntBrain_Bear` matches the CommonDll class `cAIHuntBrain_Animal` family. Per-species brain strings were not all tabulated here; grizzly is the one read in full.

Species scripts present under `SCRIPTS\MODELS\ANIMALS\`: abear, axis, bbear, biggrim, btiger, cobra, crocodile, dingo, elephant, gbear, gwolf, hippo, hogzilla, hyena, impala, leopard, lion, lionesse, mountainlion, pantelope, pbear, puma, reddeer, rhino, scrubbull, snake, watercrocodile, whitetail, wildboar, wildboar3, yeti. `OBJECTS\ANIMALS\` also has `CAMEL`, `BIGCOBRA`, `CROCODILE_W`, `TREX`, `WILDDOG` meshes. Camel's `AddAnimal` line in `MAP9` is commented out. Trex has no `AddAnimal` in any `WORLD.SCR` read here.

`TYPEMAP\*.MAT` is loaded from `AddAI`'s neighbor `LoadMat` / `LoadEventMat` in the world script (seen in `MAPTSAVO\WORLD.SCR`). That is map material/event data, separate from the animal list.

## Open questions

- Runtime check that a loose `game\Data\Cfg\console.cfg` is the file `cConsole::Load` actually executes. Code order says yes. This track did not launch the game. A loose file showed up mid-session with `cam_fov = 90`; who wrote it, and whether the next boot reads it, was not tested here.
- Directory-provider `Open` (type 2 vtable) was not decompiled. The append order is.
- `ANIMAL_*` integer values, and how `cScrAnimalController` fills the empty `hunt_area_point` "Animal names" slots at runtime.
- `.MAT` / `.DAT` binary layouts.
- Whether a missing `LoadEventMat` file aborts the map or is ignored.
- Which story chapter, if any, the retail game presents as India, Tanzania, or Siberia. The UI hunt labels are Africa, Australia, Alaska, Argentina.
- `MAP2D` typemap and HFX config with no BSP.

## Proposed MEMORY facts

- `game\Data\data.arc` is 294207488 bytes, footer version `0x100`, 9612 entries, directory at sector 143604. `py re\scripts\arc_extract.py --verify` reported 0 problems. Evidence: extractor stdout, this run.
- Extracted tree is 9710 files: 9612 archive entries plus 98 files from `shaders.mtp` (`_mtp\`). Evidence: `asset_inventory.py` extension total and `debug.log` lines 17-18 (`MltiFile 'shaders.mtp'`, 98 files, 407789 bytes).
- Archive auto-mount glob is the four characters `.ARC` at EngineDll7 RVA `0x43f1e0`. Evidence: bytes at that RVA (`2E 41 52 43 00`), consumed by `cFileManager::AddHomeDirectory` RVA `0x33cd50`.
- `cFileManager::Open` RVA `0x33e230` returns the first provider that opens the path. `AddHomeDirectory(..., 0)` appends the home directory, then each `.ARC`, then each `.ZIP`. First home in the sandbox is `.\..\data\` (`debug.log` line 7). A loose file in that tree is ahead of `data.arc`.
- `InitConsole` RVA `0x329ec0` loads `Cfg\console.cfg` via `cConsole::Load` RVA `0x1672c0`. Archive file `CFG\CONSOLE.CFG` uses `addvar <name> <type>; <name> = <value>` and sets `cam_fov` to 60.
- `Main7.exe` `FUN_00401dd0` sets `DAT_0043d2bc = 1` when `-dedicated` is present, then after `InitConsole` runs `cConsole::RealExec("r_api 0; snd_usesound 0")` if that flag is set. Evidence: `re\notes\loop-decomp-main7.c` around the `InitPakFileManager(".zip", 2)` call.
- Custom script VM files: `.SCR` text source, `.SPJ` text project, `.XSC` bytecode magic `46 53 01 01` with version byte `03` at offset 9 on all 85 files. `cVMachine::LoadInfo` RVA `0x287950` requires version 3. `SmartLoadPrj` RVA `0x2870d0` appends `.xsc` (string RVA `0x43164c`) and `.spj` (string RVA `0x432858`).
- Lua bytecode in the archive is only `.OUT` (98 files, header `1B 4C 75 61 40`). `.SCRIPT`, `.FX`, `.LUA`, `.INC`, `.SND` are text. `.AP` is `FBAF` animation data, not a script.
- Free-hunt UI maps are `HUNTMAP1` Africa, `HUNTMAP2` Australia, `HUNTMAP3` Alaska, `HUNTMAP4` Argentina (`INTERFACE\TEXT\TEXTTABLE.INC` ids `huntmap1`..`huntmap4`). Mission ids 13-16 are `MISSIONHUNTMAP1.SCR`..`MISSIONHUNTMAP4.SCR` (`SetMission(13)` through `SetMission(16)`).
- Bonus maps: `MAPARENA` mission 17, `MAPISLAND` mission 18, `MAPTSAVO` mission 19.
- Each BSP `world_properties` entity sets `script file name` to `scripts/levels/<map>/main.spj` and `script class name` to `cMapTest`. Hunter spawn entity is `info_spawn_point` / `human`. No `info_player` string in the BSPs.
- `data.arc` contains no `sv_multiplayer`, `cl_join`, `cfLobby`, `lobby`, `player2`, or `deathmatch` string in the script, config, GUI, or BSP text scanned above.
