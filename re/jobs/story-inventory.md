# Job: story-inventory — catalog the story-mode scripts and script natives (NO game launches; Ghidra slot W only)

## Why
The project's primary goal is now the **story mode (campaign) in co-op** (PLAN.md, 2026-10-16 evening). The lead needs a factual map of what the story levels' scripts do before designing co-op for them. This is an inventory job: collect and count, quote file:line evidence, do not design.

## Inputs
- Level scripts (plain text): `re\extracted\SCRIPTS\` — `LEVELS\<MAP>\MAIN.SPJ` (list of script files per map) and `WORLD.SCR`, `MISSIONS\`, `TRIGGERS\`, `CUTSCENE\`, `MODELS\`, `GAME\`.
- Story maps: `MAP1`, `MAP2A`–`MAP2C`, `MAP3A`–`MAP3C`, `MAP4A`, `MAP4B`, `MAP5`–`MAP10`, `MAP11A`–`MAP11C`, `MAP12A`–`MAP12C`. Bonus/other: `MAPARENA`, `MAPISLAND`, `MAPTSAVO`, `MAPMENU`. Hunt maps (`HUNTMAP1`–`4`, `HUNTMAPBASE.SCR`) are already analyzed (`re/notes/lead-mission.md`); include them only in the native-usage counts.
- Native registration (already decompiled): `re\notes\lead-natives.c` = GameDll7 `FUN_1000ba30`, which registers global script natives as `cExportKey::ExportFnc("<Name>", …)` followed by `cBaseClass::s_ExportStaticFunction(<fn>, …)`.
- Engine notes: `re/notes/lead-mission.md` (mission flow, claim), `MEMORY.md` section "Animals and mission flow".

## Deliverable: `re\notes\story-inventory.md`
Create it first with these headings and a `## Progress` line, then fill in order. Use tables. Every claim needs a file:line (scripts) or RVA (natives).

1. **Maps.** One row per story/bonus map:
   - the scripts listed in `MAIN.SPJ`: mission script, trigger script, number of cutscene scripts, model scripts;
   - the class names of the `<instance>` members in the map's globals (`cScrAnimal`, `cScrNPC`, `cScrTriggerEvent`, `cScrTrap`, `cScrWayPoint`, …) with counts;
   - the size of each WORLD/Mission/Trigger script in lines.
2. **Native catalog.** Build the full list of script-callable natives:
   a. Global natives from `re\notes\lead-natives.c`: name → function address (RVA = address − 0x10000000) → argument type codes as written.
   b. Class-method natives (`cScrAnimal::…`, `cScrHunter::…`, `cScrNPC::…`, `cScrTriggerEvent::…`, …) are registered by other functions that also call `cExportKey::ExportFnc`. Find them with Ghidra slot **W**: `.\re\scripts\ghrun.ps1 -Slot W -Binary GameDll7.dll -Script CallersDecomp.java -ScriptArgs 're\notes\story-natives-gamedll.c','ExportFnc'`. Repeat for `CommonDll7.dll` into `re\notes\story-natives-commondll.c`. Run them one after the other, never in parallel.
   c. From those dumps, extract (name, owning registrar function RVA, implementation function RVA, type codes) into a table. A helper script under `re\scripts\` is fine (Python is `py`).
3. **Native usage in story scripts.** For every identifier called in the story scripts that is not defined in the scripts themselves, list:
   - the call count per map;
   - the first file:line example;
   - whether it is in the native catalog (2).
   Group into these categories by name:
   - mission/task: `CompleteTask`, `UpdateMissionStatus`, `GlobalsCheckMissionStatus`, `SetObjective`, …
   - HUD/hint/text: `PlayHint`, `DisableHudItem`, …
   - cutscene/camera/movie
   - player control: freeze, teleport, flags, health, inventory, weapons
   - AI tasks and animals: `StartTask`, `SetTaskParam`, `MoveToPos`, `ForceBrainActivate`, …
   - NPC/companion
   - spawn/destroy/show/hide entities
   - triggers/activation
   - sound/music
   - timing/latent: `Pause`, `Wait`, `WaitForTask`
   - other
4. **Player references.** Every place a story script refers to the player:
   - the literal `"hunter"`;
   - `player.` / `::globals.player` method calls;
   - `GetDistBetween2Ais` with `"hunter"`;
   - triggers that test the player.
   Give a table of file:line → call → what it decides (distance check, task target, teleport, freeze, damage, camera). Then count by kind. This is the most important section: in co-op every one of these must pick a player.
5. **Cutscenes and QTE/melee.**
   - How a cutscene starts and ends (the calls used in `CUTSCENE\*`, and who calls them).
   - What cutscenes do to the player (input lock, camera, teleport).
   - Search for QTE / struggle / melee / button-mash mechanics (names containing QTE, Struggle, Melee, Button, Fight, CloseCombat, Knife, …) and list where they are used.
6. **Mission/task flow per map.**
   - How tasks are defined: the mission script classes, task ids, objectives.
   - What completes them and what fails them (calls and conditions).
   - How the map ends: success call, failure call, the next map or menu.
7. **Map transitions, checkpoints, saves, death.**
   - Any script call that loads another map, restarts, saves or loads a checkpoint, or handles player death/respawn. Quote it.
   - If none is in the scripts, say so and list the candidate C++ exports by name from `re\exports\*.demangled.txt` (search `Checkpoint`, `Save`, `Load`, `Restart`, `Respawn`, `Death`, `Die`, `NextMap`, `Level`), marked UNVERIFIED.
8. **Companions / NPCs.** Which maps have a guide or other NPCs, their script classes, and what they do (follow the player, talk, fight). Same rule: file:line.
9. **Summary for the lead.** At most 30 lines:
   - the biggest co-op problem areas by count (player references, cutscenes, QTE, NPC follow);
   - the maps that look simplest to make co-op first.
   Facts only, no design.

## Rules
- Read-only on everything except your deliverable, raw dumps in `re\notes\story-natives-*.c|txt`, and helper scripts in `re\scripts\`.
- Never launch the game. Ghidra slot **W** only, one process at a time.
- Do not commit. Do not edit `MEMORY.md`, `STATUS.md`, `PLAN.md`, `CLAUDE.md`.
- Mark anything inferred as UNVERIFIED. Prefer counts and quotes over prose.
- Add `## Proposed MEMORY facts` at the end (only facts with evidence).
