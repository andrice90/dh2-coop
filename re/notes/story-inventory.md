# Story-mode script inventory

## Progress
Done: maps, native catalog (GameDll + CommonDll ExportFnc dumps), usage, player refs, cutscenes/QTE, mission flow, transitions, NPCs, summary. Game was not launched.

Scope: story maps MAP1, MAP2A, MAP2B, MAP2C, MAP3A, MAP3B, MAP3C, MAP4A, MAP4B, MAP5, MAP6, MAP7, MAP8, MAP9, MAP10, MAP11A, MAP11B, MAP11C, MAP12A, MAP12B, MAP12C. Bonus: MAPARENA, MAPISLAND, MAPTSAVO, MAPMENU. Hunt maps appear only in native-usage counts (and in the transition search). Script paths are under `re/extracted/SCRIPTS`. Line numbers are 1-based in the file on disk. A call is counted only when it sits outside a `//` comment and outside a string. An identifier is omitted from section 3 when a script file defines a function of that name.

## 1. Maps

Globals class is the class constructed with `new <Class>(globals)` in the map `WORLD.SCR`. `<instance>` counts are members of that class only.

| Map | Mission script (lines) | Trigger script (lines) | Cutscene scripts | Model scripts | Globals class | `<instance>` counts | WORLD.SCR lines |
|---|---|---|---:|---:|---|---|---:|
| MAP1 | MISSION01.SCR (74) | TRIGGERMISSION01.SCR (429) | 5 | 10 | cScrMapGlobals | cScrAnimal×6, cScrCustomCharacterEx×1, cScrHunter×1, cScrNPC×1, cScrTrap×1, cScrTriggerEvent×12, cScrWayPoint×1, cScrWorldItem×1 | 252 |
| MAP2A | MISSION02A.SCR (75) | TRIGGERMISSION02A.SCR (463) | 6 | 6 | cScrMapGlobals | cScrAnimal×6, cScrCustomCharacterEx×1, cScrHunter×1, cScrNPC×1, cScrTrap×10, cScrTriggerEvent×14 | 168 |
| MAP2B | MISSION02B.SCR (79) | TRIGGERMISSION02B.SCR (1340) | 3 | 6 | cScrMapGlobals | cScrAnimal×8, cScrFx×7, cScrHunter×1, cScrNPC×1, cScrTrap×9, cScrTriggerEvent×25 | 241 |
| MAP2C | MISSION02C.SCR (72) | TRIGGERMISSION02C.SCR (278) | 3 | 7 | cScrMapGlobals | cScrAnimal×4, cScrHunter×1, cScrNPC×3, cScrTrap×5, cScrTriggerEvent×10, cScrWorldItem×1 | 241 |
| MAP3A | MISSION03A.SCR (78) | TRIGGERMISSION03A.SCR (473) | 4 | 8 | cScrMapGlobals | cScrAnimal×11, cScrHunter×1, cScrNPC×1, cScrTriggerEvent×16 | 169 |
| MAP3B | MISSION03B.SCR (31) | TRIGGERMISSION03B.SCR (164) | 0 | 7 | cScrMapGlobals | cScrAnimal×5, cScrHunter×1, cScrTriggerEvent×10 | 135 |
| MAP3C | MISSION03C.SCR (40) | TRIGGERMISSION03C.SCR (173) | 3 | 9 | cScrMapGlobals | cScrAnimal×7, cScrHelicopter×1, cScrHunter×1, cScrNPC×1, cScrTriggerEvent×10 | 148 |
| MAP4A | MISSION04A.SCR (38) | TRIGGERMISSION04A.SCR (275) | 2 | 9 | cScrMapGlobals | cMap4Truck×1, cScrAnimal×6, cScrHunter×1, cScrNPC×1, cScrTriggerEvent×10 | 146 |
| MAP4B | MISSION04B.SCR (45) | TRIGGERMISSION04B.SCR (676) | 5 | 6 | cScrMapGlobals | cScrAnimal×15, cScrHunter×1, cScrNPC×1, cScrTriggerEvent×20 | 324 |
| MAP5 | MISSION05.SCR (40) | TRIGGERMISSION05.SCR (616) | 3 | 7 | cScrMapGlobals | cScrAnimal×6, cScrHunter×1, cScrNPC×2, cScrTriggerEvent×20 | 146 |
| MAP6 | MISSION06.SCR (79) | TRIGGERMISSION06.SCR (443) | 5 | 7 | cScrMapGlobals | cScrAnimal×5, cScrHunter×1, cScrNPC×4, cScrTriggerEvent×12 | 148 |
| MAP7 | MISSION07.SCR (85) | TRIGGERMISSION07.SCR (687) | 3 | 8 | cScrMapGlobals | cScrAnimal×6, cScrHunter×1, cScrNPC×2, cScrTrap×1, cScrTriggerEvent×19 | 157 |
| MAP8 | MISSION08.SCR (86) | TRIGGERMISSION08.SCR (1130) | 5 | 7 | cScrMapGlobals | cScrAnimal×11, cScrEntity×3, cScrHunter×1, cScrNPC×3, cScrTriggerEvent×27 | 208 |
| MAP9 | MISSION09.SCR (67) | TRIGGERMISSION09.SCR (546) | 4 | 10 | cScrMapGlobals | cScrAnimal×3, cScrHelicopter×1, cScrHunter×1, cScrNPC×1, cScrTriggerEvent×16 | 141 |
| MAP10 | MISSION10.SCR (88) | TRIGGERMISSION10.SCR (828) | 6 | 6 | cScrMapGlobals | cScrAnimal×10, cScrHunter×1, cScrMine×10, cScrNPC×2, cScrTrap×10, cScrTriggerEvent×22 | 232 |
| MAP11A | MISSION11A.SCR (52) | TRIGGERMISSION11A.SCR (244) | 3 | 10 | cScrMapGlobals | cScrAnimal×6, cScrEntity×1, cScrHelicopter×2, cScrHunter×1, cScrNPC×4, cScrTriggerEvent×11 | 148 |
| MAP11B | MISSION11B.SCR (72) | TRIGGERMISSION11B.SCR (320) | 2 | 7 | cScrMapGlobals | cScrAnimal×7, cScrHunter×1, cScrNPC×1, cScrTriggerEvent×11 | 134 |
| MAP11C | MISSION11C.SCR (61) | TRIGGERMISSION11C.SCR (262) | 3 | 10 | cScrMapGlobals | cScrAnimal×4, cScrHelicopter×2, cScrHunter×1, cScrNPC×4, cScrTriggerEvent×11 | 127 |
| MAP12A | MISSION12A.SCR (72) | TRIGGERMISSION12A.SCR (645) | 7 | 9 | cScrMapGlobals | cScrAnimal×9, cScrHunter×1, cScrNPC×2, cScrTrap×10, cScrTriggerEvent×15 | 281 |
| MAP12B | MISSION12B.SCR (28) | TRIGGERMISSION12B.SCR (52) | 1 | 11 | cScrMapGlobals | cScrAnimal×1, cScrHunter×1, cScrTrap×5, cScrTriggerEvent×10 | 117 |
| MAP12C | MISSION12C.SCR (26) | TRIGGERMISSION12C.SCR (46) | 1 | 10 | cScrMapGlobals | cScrAnimal×1, cScrHunter×1, cScrNPC×1, cScrTriggerEvent×10 | 105 |
| MAPARENA | MISSIONMAPARENA.SCR (23) | TRIGGERMAPARENA.SCR (107) | 0 | 10 | cScrMapGlobals | cScrAnimal×11, cScrHunter×1, cScrLavaThrower×1, cScrTriggerArena1×1, cScrTriggerArena2×1, cScrTriggerArena3×1, cScrTriggerArena4×1, cScrTriggerArena5×1, cScrTriggerArena6×1, cScrTriggerArena7×1, cScrTriggerEvent×1, cScrWorldItem×3 | 800 |
| MAPISLAND | MISSIONMAPISLAND.SCR (24) | TRIGGERMAPISLAND.SCR (155) | 0 | 6 | cScrMapGlobals | cScrAnimal×2, cScrHunter×1, cScrTriggerArena1×1, cScrTriggerArena2×1, cScrTriggerArena3×1, cScrTriggerArena4×1, cScrTriggerArena5×1, cScrTriggerArena6×1, cScrTriggerArena7×1, cScrTriggerEvent×2, cScrWorldItem×2 | 347 |
| MAPTSAVO | MISSIONMAPTSAVO.SCR (24) | TRIGGERMAPTSAVO.SCR (106) | 0 | 10 | cScrMapGlobals | cScrAnimal×2, cScrHunter×1, cScrTriggerArena1×1, cScrTriggerArena2×1, cScrTriggerArena3×1, cScrTriggerArena4×1, cScrTriggerArena5×1, cScrTriggerArena6×1, cScrTriggerArena7×1, cScrTriggerEvent×1, cScrWorldItem×3 | 376 |
| MAPMENU | — | — | 0 | 0 | no globals constructor; instances are on cMapTest | cScrCamera×1 | 22 |

Model and cutscene file names, and other MAIN.SPJ entries that are not mission/trigger/model/cutscene/this map's WORLD.SCR:

- **MAP1** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE3.SCR, CUTSCENE4.SCR, CUTSCENE5.SCR
  - models (10): HUMANLEGS.SCR, HUMAN.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR, WEAPON.SCR, RIFLE.SPL, HGUN.SPL, MBRANCH.SPL
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP2A** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE3.SCR, CUTSCENE4.SCR, CUTSCENE5.SCR, CUTSCENE6.SCR
  - models (6): HUMANLEGS.SCR, HUMAN.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR
- **MAP2B** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE3.SCR
  - models (6): HUMANLEGS.SCR, HUMAN.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR
- **MAP2C** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE4.SCR
  - models (7): HUMANLEGS.SCR, HUMAN.SCR, GIRLLEGS.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR
- **MAP3A** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE3.SCR, CUTSCENE4.SCR
  - models (8): HUMANLEGS.SCR, HUMAN.SCR, GUIDE.SCR, GUIDELEGS.SCR, REDSHIRT.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP3B** cutscenes: —
  - models (7): HUMANLEGS.SCR, HUMAN.SCR, GUIDE.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP3C** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE3.SCR
  - models (9): HUMANLEGS.SCR, HUMAN.SCR, GUIDE.SCR, GUIDELEGS.SCR, REDSHIRT.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR, BINOCULAR.SPL
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP4A** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR
  - models (9): HUMANLEGS.SCR, HUMAN.SCR, DRIVER.SCR, GUIDE.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR, TRUCK.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP4B** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE5.SCR, CUTSCENE6.SCR, CUTSCENE7.SCR
  - models (6): HUMANLEGS.SCR, HUMAN.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP5** cutscenes: CUTSCENE1.SCR, CUTSCENE3.SCR, CUTSCENE4.SCR
  - models (7): HUMANLEGS.SCR, HUMAN.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR, TRUCK.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP6** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE3.SCR, CUTSCENE4.SCR, CUTSCENE5.SCR
  - models (7): HUMANLEGS.SCR, HUMAN.SCR, GUIDELEGS.SCR, GIRLLEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR
- **MAP7** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE6.SCR
  - models (8): HUMANLEGS.SCR, HUMAN.SCR, GUIDE.SCR, GUIDELEGS.SCR, GIRLLEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR
- **MAP8** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE3.SCR, CUTSCENE4.SCR, CUTSCENE5.SCR
  - models (7): HUMANLEGS.SCR, HUMAN.SCR, GUIDELEGS.SCR, GUIDE.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP9** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE3.SCR, CUTSCENE4.SCR
  - models (10): HUMANLEGS.SCR, HUMAN.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR, WEAPON.SCR, KNIFE.SCR, HGUN.SCR, MBRANCH.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP10** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE3.SCR, CUTSCENE4.SCR, CUTSCENE5.SCR, CUTSCENE6.SCR
  - models (6): HUMANLEGS.SCR, HUMAN.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP11A** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE3.SCR
  - models (10): HUMANLEGS.SCR, HUMAN.SCR, PILOT.SCR, HELICOPTER.SCR, HELICOPTERENT.SCR, GUIDE.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP11B** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR
  - models (7): HUMANLEGS.SCR, HUMAN.SCR, GUIDE.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP11C** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE3.SCR
  - models (10): HUMANLEGS.SCR, HUMAN.SCR, PILOT.SCR, HELICOPTER.SCR, HELICOPTERENT.SCR, GUIDE.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP12A** cutscenes: CUTSCENE1.SCR, CUTSCENE2.SCR, CUTSCENE3.SCR, CUTSCENE4.SCR, CUTSCENE5.SCR, CUTSCENE6.SCR, CUTSCENE7.SCR
  - models (9): HUMANLEGS.SCR, HUMAN.SCR, GUIDE.SCR, GUIDELEGS.SCR, REDSHIRT.SCR, POACHER.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP12B** cutscenes: CUTSCENE1.SCR
  - models (11): HUMANLEGS.SCR, HUMAN.SCR, WEAPON.SCR, KNIFE.SCR, HGUN.SCR, MBRANCH.SCR, RIFLE.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAP12C** cutscenes: CUTSCENE1.SCR
  - models (10): HUMANLEGS.SCR, HUMAN.SCR, WEAPON.SCR, KNIFE.SCR, MBRANCH.SCR, HELICOPTER.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, TRAP.SCR
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAPARENA** cutscenes: —
  - models (10): HUMANLEGS.SCR, HUMAN.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, VOLCANO.SCR, WEAPON.SCR, RIFLE.SPL, HGUN.SPL, MBRANCH.SPL
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAPISLAND** cutscenes: —
  - models (6): HUMANLEGS.SCR, HUMAN.SCR, WEAPON.SCR, RIFLE.SPL, HGUN.SPL, MBRANCH.SPL
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAPTSAVO** cutscenes: —
  - models (10): HUMANLEGS.SCR, HUMAN.SCR, GUIDELEGS.SCR, SNAKE.SCR, BIRDS.SCR, VOLCANO.SCR, WEAPON.SCR, RIFLE.SPL, HGUN.SPL, MBRANCH.SPL
  - other: MAPWORLDBASE.SCR, LENSFLARE.SCR
- **MAPMENU** cutscenes: —
  - models (0): —

## 2. Native catalog

### 2a. Global natives (`re/notes/lead-natives.c`, `FUN_1000ba30`, RVA `0xba30`)

Each row is one `cExportKey::ExportFnc` in that function. Codes are the numeric arguments after the name, as written (second slot is `0` or `1`, then return type, `-1`, argument types, `0xffffffff` terminator). Implementation RVA = decompiler address − `0x10000000`.

| Name | Impl RVA | Impl symbol | Codes |
|---|---|---|---|
| `LoadMat` | `0x5e30` | `FUN_10005e30` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `LoadEventMat` | `0x5fe0` | `FUN_10005fe0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `LoadRiverMat` | `0x6170` | `FUN_10006170` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `AddHUDItem` | `0x6300` | `FUN_10006300` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetPoisonColor` | `0x6720` | `LAB_10006720` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetPoisonIntensity` | `0x6700` | `LAB_10006700` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetEyeHurtColor` | `0x66e0` | `LAB_100066e0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetEyeHurtIntensity` | `0x6710` | `LAB_10006710` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetDamageDuration` | `0x6740` | `LAB_10006740` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetFadeOutSndValue` | `0x6750` | `LAB_10006750` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `AddFX` | `0x6770` | `LAB_10006770` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `AddManager` | `0x6790` | `LAB_10006790` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `AutoPlayMusic` | `0x6460` | `LAB_10006460` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `SetAutoPlayMusicDelays` | `0x6480` | `LAB_10006480` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `AddAnimal` | `0x8d970` | `FUN_1008d970` | `0, 0x7fffffea, -1, 0x7fffffea, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `AddEvent` | `0x76b0` | `FUN_100076b0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `AddDanger` | `0x64a0` | `LAB_100064a0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `PrecacheAnimal` | `0x8d220` | `FUN_1008d220` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetTime` | `0x7040` | `FUN_10007040` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `EquipAIWith` | `0x7180` | `FUN_10007180` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `AiCanFindItem` | `0x7340` | `FUN_10007340` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `AddAIToBeProtected` | `0x77c0` | `FUN_100077c0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `RemoveAIToBeProtected` | `0x78f0` | `FUN_100078f0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `FindEntScrObject` | `0x6f00` | `FUN_10006f00` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `SetMaxAttackerNo` | `0x7a20` | `LAB_10007a20` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetDiffProps` | `0xa530` | `FUN_1000a530` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetDamageBlurParams` | `0x7e40` | `LAB_10007e40` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetWaterFog` | `0x80b0` | `FUN_100080b0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `UpdateMissionStatus` | `0x8390` | `FUN_10008390` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `ActivateTrigger` | `0x84a0` | `FUN_100084a0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `DeactivateTrigger` | `0x85c0` | `FUN_100085c0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ActivateAIHuntAreaPoint` | `0x86e0` | `FUN_100086e0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `DeactivateAIHuntAreaPoint` | `0x8810` | `FUN_10008810` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `AddAnimPakForAI` | `0x8940` | `FUN_10008940` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `GameIsInCutScene` | `0x9040` | `FUN_10009040` | `0, 0x7fffffdd, -1, -1` |
| `SetCharacterModel` | `0x9190` | `FUN_10009190` | `0, 0x7fffffdd, -1, 0x7fffffea, 0xffffffff` |
| `SetBreathFxVisible` | `0x91a0` | `LAB_100091a0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `ActivateBulletWarning` | `0x93a0` | `LAB_100093a0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `SetColorLightDirDown` | `0x9500` | `FUN_10009500` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetColorLightDirUp` | `0x9610` | `FUN_10009610` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetAmbientalColor` | `0x9810` | `FUN_10009810` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetModifiedDirLight` | `0x9730` | `FUN_10009730` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetCoeficientLightUp` | `0x98d0` | `LAB_100098d0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetCoeficientLightDown` | `0x98b0` | `LAB_100098b0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `GlobalsCheckMissionStatus` | `0x98f0` | `FUN_100098f0` | `0, 0x7fffffd8, -1, -1` |
| `GetDistFromClosestStimulusTo` | `0x8cc0` | `FUN_10008cc0` | `0, 0x7fffffe9, -1, 0x7fffffe7, 0xffffffff` |
| `GetClosestMatchingEntity` | `0x8e20` | `FUN_10008e20` | `0, 0x7fffffe7, -1, 0x7fffffe7, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `SetDamageBloomFadeInOut` | `0x7e20` | `LAB_10007e20` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetDamageBloom` | `0xaf90` | `FUN_1000af90` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetDeathBloom` | `0xb130` | `FUN_1000b130` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetPoisonBloom` | `0xb470` | `FUN_1000b470` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetLightBloom` | `0xb2d0` | `FUN_1000b2d0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetShadowBloom` | `0xb610` | `FUN_1000b610` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `EnableHudItem` | `0x64c0` | `FUN_100064c0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `DisableHudItem` | `0x65d0` | `FUN_100065d0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetShootingAccuracy` | `0x9010` | `LAB_10009010` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetMinSoftlock` | `0x9060` | `LAB_10009060` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `CompleteTask` | `0x9080` | `FUN_10009080` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `GetDistBetween2Ais` | `0x9200` | `FUN_10009200` | `0, 0x7fffffe9, -1, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `AddPlayerStepSoundForMaterial` | `0xb920` | `FUN_1000b920` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `GetTaskStatus` | `0x93d0` | `FUN_100093d0` | `0, 0x7fffffea, -1, 0x7fffffea, 0xffffffff` |

Count: 61 `ExportFnc` entries in `re/notes/lead-natives.c` `FUN_1000ba30`. The same function in `re/notes/story-natives-gamedll.c` has 61 entries; name lists match: True.

### 2b. Class-method dumps

Commands run one after another on slot W (no game launch):

`.\re\scripts\ghrun.ps1 -Slot W -Binary GameDll7.dll -Script CallersDecomp.java -ScriptArgs 're\notes\story-natives-gamedll.c','ExportFnc'`

Result: 25 callers, written to `re/notes/story-natives-gamedll.c`.

`.\re\scripts\ghrun.ps1 -Slot W -Binary CommonDll7.dll -Script CallersDecomp.java -ScriptArgs 're\notes\story-natives-commondll.c','ExportFnc'`

Result: 17 callers, written to `re/notes/story-natives-commondll.c`.

Registrar list (signature class when Ghidra named the function; otherwise the function is `FUN_`/`LAB_`). `parent string` is the `s_<Name>_` symbol copied into the export block. On CommonDll, named `ScrExport` functions store the **parent** class string, not the signature class (example: `cScrHuman::ScrExport` copies `cScrCharacter`, `AllocMem(0xe)` = 14 bytes = `cScrCharacter` plus NUL). For unnamed GameDll registrars the owning class is the parent string only when that is all the dump shows; treat a subclass relationship as UNVERIFIED.

| Binary | Registrar | RVA | Signature class | Parent string in export block | ExportFnc count | static/method |
|---|---|---|---|---|---:|---|
| GameDll7 | `FUN_1000ba30` | `0xba30` | `—` | `cBaseClass` | 61 | static |
| GameDll7 | `FUN_1001ec10` | `0x1ec10` | `—` | `cScrCharacterEx` | 32 | method |
| GameDll7 | `FUN_10022a30` | `0x22a30` | `—` | `cScrEntity` | 3 | method |
| GameDll7 | `FUN_100244f0` | `0x244f0` | `—` | `cScrItemEx` | 6 | method |
| GameDll7 | `FUN_10029eb0` | `0x29eb0` | `—` | `cMOD_ScrWeapon` | 3 | method |
| GameDll7 | `FUN_10039830` | `0x39830` | `—` | `cScrItemEx` | 18 | method |
| GameDll7 | `FUN_10043ef0` | `0x43ef0` | `—` | `cScrEntity` | 1 | method |
| GameDll7 | `FUN_10044aa0` | `0x44aa0` | `—` | `cScrCharacterEx` | 1 | method |
| GameDll7 | `FUN_10044c20` | `0x44c20` | `—` | `—` | 3 | method |
| GameDll7 | `FUN_10053e80` | `0x53e80` | `—` | `cScrPlayer` | 46 | method |
| GameDll7 | `FUN_10063510` | `0x63510` | `—` | `cScrHunter` | 37 | method |
| GameDll7 | `FUN_10067670` | `0x67670` | `—` | `cScrHumanEx` | 9 | method |
| GameDll7 | `FUN_1006ba10` | `0x6ba10` | `—` | `cScrEntity` | 23 | method |
| GameDll7 | `FUN_100746b0` | `0x746b0` | `—` | `cScrEntity` | 3 | method |
| GameDll7 | `FUN_100749e0` | `0x749e0` | `—` | `cScrBirds` | 2 | method |
| GameDll7 | `FUN_1007a9d0` | `0x7a9d0` | `—` | `cScrEntity` | 2 | method |
| GameDll7 | `FUN_1007d670` | `0x7d670` | `—` | `cScrItemEx` | 1 | method |
| GameDll7 | `FUN_10081810` | `0x81810` | `—` | `cScrEntity` | 5 | method |
| GameDll7 | `FUN_10089510` | `0x89510` | `—` | `cScrItemEx` | 3 | method |
| GameDll7 | `FUN_1008ee40` | `0x8ee40` | `—` | `cBaseClass` | 5 | method |
| GameDll7 | `FUN_10095bd0` | `0x95bd0` | `—` | `cScrEntity` | 23 | method |
| GameDll7 | `FUN_100ad9e0` | `0xad9e0` | `—` | `cBaseClass` | 8 | method |
| GameDll7 | `FUN_100c2d40` | `0xc2d40` | `—` | `cBaseClass` | 39 | static |
| GameDll7 | `FUN_100e37c0` | `0xe37c0` | `—` | `cHUDItem` | 8 | static |
| GameDll7 | `FUN_100e9fe0` | `0xe9fe0` | `—` | `cBaseClass` | 10 | static |
| CommonDll7 | `cScrHuman::ScrExport` | `0xeae0` | `cScrHuman` | `cScrCharacter` | 3 | method |
| CommonDll7 | `cScrCharacter::ScrExport` | `0x10700` | `cScrCharacter` | `cScrEntity` | 26 | method |
| CommonDll7 | `cScrCharacterEx::ScrExport` | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | 23 | method |
| CommonDll7 | `cScrHumanEx::ScrExport` | `0x298a0` | `cScrHumanEx` | `cScrCharacterEx` | 6 | method |
| CommonDll7 | `cScrItem::ScrExport` | `0x31570` | `cScrItem` | `cScrEntity` | 1 | method |
| CommonDll7 | `cScrItemEx::ScrExport` | `0x33e20` | `cScrItemEx` | `cScrEntity` | 1 | method |
| CommonDll7 | `cScrEntity::ScrExport` | `0x41690` | `cScrEntity` | `cBaseClass` | 61 | method,static |
| CommonDll7 | `cScrTrain::ScrExport` | `0x4c3b0` | `cScrTrain` | `cScrEntity` | 8 | method |
| CommonDll7 | `cScrRotating::ScrExport` | `0x4c7b0` | `cScrRotating` | `cScrEntity` | 4 | method |
| CommonDll7 | `cScrWayPoint::ScrExport` | `0x60880` | `cScrWayPoint` | `cScrEntity` | 2 | method |
| CommonDll7 | `FUN_100663a0` | `0x663a0` | `—` | `cScrItemEx` | 7 | method |
| CommonDll7 | `cScrAIHunt::ScrExport` | `0xa1080` | `cScrAIHunt` | `cScrEntity` | 4 | static |
| CommonDll7 | `FUN_100dfd20` | `0xdfd20` | `—` | `cScrContainer` | 39 | method,static |
| CommonDll7 | `cGuiEvent::ScrExport` | `0xe7610` | `cGuiEvent` | `cBaseClass` | 8 | method,static |
| CommonDll7 | `cGuiSkinBase::ScrExport` | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | 47 | method |
| CommonDll7 | `FUN_100f74c0` | `0xf74c0` | `—` | `cBaseClass` | 4 | static |
| CommonDll7 | `cGuiSaveMan::ScrExport` | `0x102270` | `cGuiSaveMan` | `cScrContainer` | 31 | method |

### 2c. Method and extra-registrar natives

`FUN_1000ba30` rows are in 2a and are not repeated here. `kind` is `static` when the next call is `s_ExportStaticFunction`, `method` when it is `s_ExportFunction`.

| Name | Binary | Registrar RVA | Signature class | Parent string | Kind | Impl RVA | Impl symbol | Codes |
|---|---|---|---|---|---|---|---|---|
| `AddHitBone` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x18b10` | `LAB_10018b10` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `AddHurtProbab` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x18b20` | `LAB_10018b20` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetHitSphereRadius` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16160` | `LAB_10016160` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetTrackSize` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x18b30` | `LAB_10018b30` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetHitImpulse` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x18b40` | `LAB_10018b40` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `AddSound` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x1dff0` | `FUN_1001dff0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0xffffffff` |
| `SetModelScale` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x15ec0` | `FUN_10015ec0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetHealth` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x15ff0` | `FUN_10015ff0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetRealFxMass` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16140` | `LAB_10016140` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `IsProtected` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16180` | `LAB_10016180` | `0, 0x7fffffd8, -1, -1` |
| `MoveToPos` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x161a0` | `FUN_100161a0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `CustomProps` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16300` | `FUN_10016300` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `AddStep` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x1d600` | `FUN_1001d600` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetTranqHurt` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16450` | `LAB_10016450` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetTranqTime` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16470` | `LAB_10016470` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetTimeBeforeLimpOnTranq` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16490` | `LAB_10016490` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetHitSound` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x165c0` | `FUN_100165c0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `CanBeRespawned` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x164b0` | `FUN_100164b0` | `0, 0x7fffffdd, -1, -1` |
| `SetPhysDimensionsXZ` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x166e0` | `FUN_100166e0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `PlaySoundEffect` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16800` | `FUN_10016800` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetName` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x15c70` | `FUN_10015c70` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetScrClaimCallback` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16930` | `FUN_10016930` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `SetCanBeClaimed` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16910` | `LAB_10016910` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `Kill` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16a40` | `FUN_10016a40` | `0, 0x7fffffd8, -1, -1` |
| `DontPlayHurtOrImpulseOnHurt` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16b50` | `LAB_10016b50` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetTimeBeforeGetUp` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16bf0` | `LAB_10016bf0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetTimeBeforeFade` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16bd0` | `LAB_10016bd0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetKickMultiplier` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16c00` | `LAB_10016c00` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `LoadSoundOnJumpSurprise` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16c10` | `FUN_10016c10` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `GetDistFromClosestStimulus` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16e30` | `FUN_10016e30` | `0, 0x7fffffe9, -1, -1` |
| `MakeBoss` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x16d20` | `FUN_10016d20` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `SetMinMaxDelaysBetweenHurts` | GameDll7 | `0x1ec10` | `—` | `cScrCharacterEx` | method | `0x17000` | `LAB_10017000` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetCountDownTime` | GameDll7 | `0x22a30` | `—` | `cScrEntity` | method | `0x22410` | `LAB_10022410` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `StartCountDown` | GameDll7 | `0x22a30` | `—` | `cScrEntity` | method | `0x22430` | `LAB_10022430` | `0, 0x7fffffd8, -1, -1` |
| `RestoreLight` | GameDll7 | `0x22a30` | `—` | `cScrEntity` | method | `0x22450` | `LAB_10022450` | `0, 0x7fffffd8, -1, -1` |
| `IsTaskCompleted` | GameDll7 | `0x244f0` | `—` | `cScrItemEx` | method | `0x22db0` | `LAB_10022db0` | `0, 0x7fffffdd, -1, -1` |
| `CompleteTask` | GameDll7 | `0x244f0` | `—` | `cScrItemEx` | method | `0x22dc0` | `LAB_10022dc0` | `0, 0x7fffffd8, -1, -1` |
| `UncompleteTask` | GameDll7 | `0x244f0` | `—` | `cScrItemEx` | method | `0x22dd0` | `LAB_10022dd0` | `0, 0x7fffffd8, -1, -1` |
| `BSMGoToState` | GameDll7 | `0x244f0` | `—` | `cScrItemEx` | method | `0x240f0` | `FUN_100240f0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetStormFXOffset` | GameDll7 | `0x244f0` | `—` | `cScrItemEx` | method | `0x24200` | `LAB_10024200` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `MakeVisible` | GameDll7 | `0x244f0` | `—` | `cScrItemEx` | method | `0x24250` | `LAB_10024250` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `GetArmStatus` | GameDll7 | `0x29eb0` | `—` | `cMOD_ScrWeapon` | method | `0x29c10` | `LAB_10029c10` | `0, 0x7fffffe9, -1, -1` |
| `ForceFeedBack` | GameDll7 | `0x29eb0` | `—` | `cMOD_ScrWeapon` | method | `0x65ec0` | `LAB_10065ec0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `ResetForceFeedBack` | GameDll7 | `0x29eb0` | `—` | `cMOD_ScrWeapon` | method | — | `—` | `0, 0x7fffffd8, -1, -1` |
| `Fire` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x36470` | `FUN_10036470` | `0, 0x7fffffd8, -1, -1` |
| `SecFire` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x365b0` | `FUN_100365b0` | `0, 0x7fffffd8, -1, -1` |
| `Reload` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x366b0` | `FUN_100366b0` | `0, 0x7fffffd8, -1, -1` |
| `Destroy` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x36860` | `FUN_10036860` | `0, 0x7fffffd8, -1, -1` |
| `IsTranq` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x36970` | `FUN_10036970` | `0, 0x7fffffd8, -1, -1` |
| `GetZoomPowerMin` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x36ba0` | `FUN_10036ba0` | `0, 0x7fffffe9, -1, -1` |
| `GetZoomPowerMax` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x36a80` | `FUN_10036a80` | `0, 0x7fffffe9, -1, -1` |
| `GetCurrentZoom` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x36cc0` | `FUN_10036cc0` | `0, 0x7fffffe9, -1, -1` |
| `SetCurrentZoom` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x36de0` | `FUN_10036de0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `CanZoom` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x36ef0` | `FUN_10036ef0` | `0, 0x7fffffdd, -1, -1` |
| `HasFixedZoom` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x37010` | `FUN_10037010` | `0, 0x7fffffdd, -1, -1` |
| `Zoom` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x37130` | `FUN_10037130` | `0, 0x7fffffd8, -1, -1` |
| `SetBkpZoom` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x374d0` | `FUN_100374d0` | `0, 0x7fffffd8, -1, -1` |
| `GetZoomFromBkp` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x374f0` | `LAB_100374f0` | `0, 0x7fffffe9, -1, -1` |
| `BeginAim` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x37240` | `FUN_10037240` | `0, 0x7fffffd8, -1, -1` |
| `ExitAim` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x37360` | `FUN_10037360` | `0, 0x7fffffd8, -1, -1` |
| `SetCamOffsetsInAim` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x37480` | `LAB_10037480` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `CanFire` | GameDll7 | `0x39830` | `—` | `cScrItemEx` | method | `0x36460` | `LAB_10036460` | `0, 0x7fffffdd, -1, -1` |
| `MoveToPos` | GameDll7 | `0x43ef0` | `—` | `cScrEntity` | method | `0x3e8f0` | `FUN_1003e8f0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `MoveToPos` | GameDll7 | `0x44aa0` | `—` | `cScrCharacterEx` | method | `0x18b10` | `LAB_10018b10` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `CanBeRespawned` | GameDll7 | `0x44c20` | `—` | `—` | method | `0x41590` | `LAB_10041590` | `0, 0x7fffffdd, -1, -1` |
| `Activate` | GameDll7 | `0x44c20` | `—` | `—` | method | `0x416c0` | `LAB_100416c0` | `0, 0x7fffffd8, -1, -1` |
| `Deactivate` | GameDll7 | `0x44c20` | `—` | `—` | method | `0x416e0` | `LAB_100416e0` | `0, 0x7fffffd8, -1, -1` |
| `ActivateCutSceneMode` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a6b0` | `FUN_1004a6b0` | `0, 0x7fffffd8, -1, -1` |
| `DeactivateCutSceneMode` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a7e0` | `FUN_1004a7e0` | `0, 0x7fffffd8, -1, -1` |
| `SetHealthDamageFromPoison` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a900` | `LAB_1004a900` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetRecoverSpeed` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a920` | `LAB_1004a920` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetMaxSpringTime` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a930` | `LAB_1004a930` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetMinStaminaForRunning` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a940` | `LAB_1004a940` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetChanceToGetDamaged` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a960` | `LAB_1004a960` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetTimeSelfRecoverDisabled` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a950` | `LAB_1004a950` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetMinHurtForDamage` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a970` | `LAB_1004a970` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetStaminaRecoverSpeedOnIdle` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a980` | `LAB_1004a980` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetStaminaRecoverSpeedOnWalk` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a990` | `LAB_1004a990` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetBreathSoundFadeOutSpeedOnIdle` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a9a0` | `LAB_1004a9a0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetBreathSoundFadeOutSpeedOnWalk` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a9b0` | `LAB_1004a9b0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetBreathSoundFadeOutSpeedOnRunning` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a9c0` | `LAB_1004a9c0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `PlaySound` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4d790` | `LAB_1004d790` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `PlayMessage` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4a9d0` | `FUN_1004a9d0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `PlayDialog` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x65ec0` | `LAB_10065ec0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `TakeItem` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4ad10` | `FUN_1004ad10` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ActivateItem` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x52090` | `FUN_10052090` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `DeactivateItem` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4ae20` | `FUN_1004ae20` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetItemCommand` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4af30` | `FUN_1004af30` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `MoveToPos` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4d7a0` | `LAB_1004d7a0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetYawLimit` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b1a0` | `FUN_1004b1a0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `GetInCar` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b2c0` | `FUN_1004b2c0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `DriveTo` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b3d0` | `FUN_1004b3d0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `StopDriving` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b4e0` | `FUN_1004b4e0` | `0, 0x7fffffd8, -1, -1` |
| `PlayHint` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b630` | `FUN_1004b630` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `PlayHint` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b780` | `FUN_1004b780` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0xffffffff` |
| `SetSandFallSpeed` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b8d0` | `LAB_1004b8d0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetSandGetoutSpeed` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b8f0` | `LAB_1004b8f0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetWaterCurrentInfluence` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b910` | `LAB_1004b910` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetControllerInfluence` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b930` | `LAB_1004b930` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetControllerInertia` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4ba30` | `LAB_1004ba30` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetIceCrackTime` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b950` | `LAB_1004b950` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetIceFallTime` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b970` | `LAB_1004b970` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetIceSpeedFallForPlayer` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b990` | `LAB_1004b990` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetIceModelsOffsets` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4b9b0` | `LAB_1004b9b0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetIceWalkRunMultiplier` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4ba10` | `LAB_1004ba10` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetStealthLevels` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4bb70` | `LAB_1004bb70` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `GetStealthLevel` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4bbb0` | `LAB_1004bbb0` | `0, 0x7fffffe9, -1, -1` |
| `ShowInfo` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4bbe0` | `FUN_1004bbe0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `DisableStimulus` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4bd00` | `LAB_1004bd00` | `0, 0x7fffffd8, -1, -1` |
| `AddItemAmmo` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4be50` | `FUN_1004be50` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0xffffffff` |
| `SetDenySATime` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4bf60` | `LAB_1004bf60` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetCanBeAttacked` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4bf80` | `FUN_1004bf80` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetMaxKnifeDistance` | GameDll7 | `0x53e80` | `—` | `cScrPlayer` | method | `0x4c090` | `LAB_1004c090` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetSoundShader` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x60ca0` | `FUN_10060ca0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `SetAttribute` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59160` | `FUN_10059160` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `ConfigTaskPriority` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59280` | `FUN_10059280` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `ConfigTaskCmd` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x593b0` | `FUN_100593b0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0xffffffff` |
| `ConfigTaskExtraCmd` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x594d0` | `FUN_100594d0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffe7, 0x7fffffea, 0xffffffff` |
| `SetHelloDistance` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59600` | `LAB_10059600` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetFollowDistance` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59620` | `LAB_10059620` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetSafeDistance` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59640` | `LAB_10059640` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetCoverPointSearchDist` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59c40` | `LAB_10059c40` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `AllowTask` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59770` | `FUN_10059770` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `DenyTask` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59890` | `FUN_10059890` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `ActivateWeaponIn` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x599f0` | `FUN_100599f0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `DeactivateWeaponIn` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59b20` | `FUN_10059b20` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetAIType` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x60db0` | `FUN_10060db0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `SetFavoriteWeapon` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x599d0` | `LAB_100599d0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetDangerTime` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x599b0` | `LAB_100599b0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `AddToCoverPointList` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59c60` | `FUN_10059c60` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `RemoveFromCoverPointList` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59d80` | `FUN_10059d80` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetFreezePhysics` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59ec0` | `FUN_10059ec0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetRightEyeRelPos` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59ff0` | `FUN_10059ff0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetLeftEyeRelPos` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5a120` | `FUN_1005a120` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetRightEyeRelAngles` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5a250` | `LAB_1005a250` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetLeftEyeRelAngles` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5a280` | `LAB_1005a280` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `StopAllTask` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59660` | `FUN_10059660` | `0, 0x7fffffd8, -1, -1` |
| `SetInstantActivate` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5a2b0` | `LAB_1005a2b0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `GetInstantActivate` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5a2d0` | `LAB_1005a2d0` | `0, 0x7fffffea, -1, -1` |
| `SetIconCoords` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59000` | `LAB_10059000` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetNPCName` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x59050` | `FUN_10059050` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `EnableStimulus` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5a400` | `LAB_1005a400` | `0, 0x7fffffd8, -1, -1` |
| `DisableStimulus` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x4bd00` | `LAB_1004bd00` | `0, 0x7fffffd8, -1, -1` |
| `SetPhysNPC` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5a730` | `LAB_1005a730` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetCanBeHited` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5a760` | `LAB_1005a760` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `AddIgnoredStimulus` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5a790` | `FUN_1005a790` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0xffffffff` |
| `RemoveIgnoredStimulus` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5a8b0` | `FUN_1005a8b0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `EnableBlocker` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5a9d0` | `FUN_1005a9d0` | `0, 0x7fffffd8, -1, -1` |
| `DisableBlocker` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5aad0` | `FUN_1005aad0` | `0, 0x7fffffd8, -1, -1` |
| `PlaySoundOnTime` | GameDll7 | `0x63510` | `—` | `cScrHunter` | method | `0x5a610` | `FUN_1005a610` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetHumanSize` | GameDll7 | `0x67670` | `—` | `cScrHumanEx` | method | `0x18b10` | `LAB_10018b10` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `AddStep` | GameDll7 | `0x67670` | `—` | `cScrHumanEx` | method | `0x66e60` | `FUN_10066e60` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetPFlags` | GameDll7 | `0x67670` | `—` | `cScrHumanEx` | method | `0x67260` | `LAB_10067260` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `ClearPFlags` | GameDll7 | `0x67670` | `—` | `cScrHumanEx` | method | `0x67250` | `LAB_10067250` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `IsCrouch` | GameDll7 | `0x67670` | `—` | `cScrHumanEx` | method | `0x66fe0` | `LAB_10066fe0` | `0, 0x7fffffdd, -1, -1` |
| `IsLegHurt` | GameDll7 | `0x67670` | `—` | `cScrHumanEx` | method | `0x67000` | `LAB_10067000` | `0, 0x7fffffdd, -1, -1` |
| `Jump` | GameDll7 | `0x67670` | `—` | `cScrHumanEx` | method | `0x67040` | `FUN_10067040` | `0, 0x7fffffd8, -1, -1` |
| `IsInVehicle` | GameDll7 | `0x67670` | `—` | `cScrHumanEx` | method | `0x67020` | `LAB_10067020` | `0, 0x7fffffdd, -1, -1` |
| `GetRotYawSpeed` | GameDll7 | `0x67670` | `—` | `cScrHumanEx` | method | `0x67150` | `FUN_10067150` | `0, 0x7fffffe9, -1, -1` |
| `SetName` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x695d0` | `FUN_100695d0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ActivateBy` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x69e40` | `FUN_10069e40` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetObjective` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x696e0` | `FUN_100696e0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetDescription` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x693b0` | `FUN_100693b0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ActivateAfter` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x694c0` | `FUN_100694c0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `ExecuteScript` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x697f0` | `FUN_100697f0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ActivateHUD` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6b8b0` | `FUN_1006b8b0` | `0, 0x7fffffd8, -1, -1` |
| `DeactivateHUD` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x69970` | `FUN_10069970` | `0, 0x7fffffd8, -1, -1` |
| `ActivateSkipHUD` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x69ac0` | `FUN_10069ac0` | `0, 0x7fffffd8, -1, -1` |
| `DeactivateSkipHUD` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x69bf0` | `FUN_10069bf0` | `0, 0x7fffffd8, -1, -1` |
| `NextTrig` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x69f90` | `FUN_10069f90` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ForceEndCutScene` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6a0a0` | `FUN_1006a0a0` | `0, 0x7fffffd8, -1, -1` |
| `CheckMissionStatus` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6a1a0` | `FUN_1006a1a0` | `0, 0x7fffffd8, -1, -1` |
| `PlaySoundEffect` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6a2b0` | `FUN_1006a2b0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `ActivateTriggerLevel` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6a4d0` | `FUN_1006a4d0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `LatentActivateTriggerLevel` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6a610` | `FUN_1006a610` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `Wait` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6a7e0` | `LAB_1006a7e0` | `1, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `WaitForTask` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6b4b0` | `FUN_1006b4b0` | `1, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `IgnoreMissionFlow` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6aa70` | `LAB_1006aa70` | `0, 0x7fffffd8, -1, -1` |
| `Activate` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6aa90` | `FUN_1006aa90` | `0, 0x7fffffd8, -1, -1` |
| `Deactivate` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6ab90` | `FUN_1006ab90` | `0, 0x7fffffd8, -1, -1` |
| `WaitTask` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6ac90` | `FUN_1006ac90` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `UpdateTaskSoundParam` | GameDll7 | `0x6ba10` | `—` | `cScrEntity` | method | `0x6ada0` | `FUN_1006ada0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `Destroy` | GameDll7 | `0x746b0` | `—` | `cScrEntity` | method | `0x70f40` | `FUN_10070f40` | `0, 0x7fffffd8, -1, -1` |
| `SetScale` | GameDll7 | `0x746b0` | `—` | `cScrEntity` | method | `0x71050` | `FUN_10071050` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `PlaySound` | GameDll7 | `0x746b0` | `—` | `cScrEntity` | method | `0x71180` | `FUN_10071180` | `0, 0x7fffffd8, -1, -1` |
| `LookAt` | GameDll7 | `0x749e0` | `—` | `cScrBirds` | method | `0x72890` | `FUN_10072890` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `Fly` | GameDll7 | `0x749e0` | `—` | `cScrBirds` | method | `0x72790` | `FUN_10072790` | `0, 0x7fffffd8, -1, -1` |
| `SetThrowTime` | GameDll7 | `0x7a9d0` | `—` | `cScrEntity` | method | `0x78430` | `LAB_10078430` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetThrowHorizSpeed` | GameDll7 | `0x7a9d0` | `—` | `cScrEntity` | method | `0x78450` | `LAB_10078450` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `MoveToPos` | GameDll7 | `0x7d670` | `—` | `cScrItemEx` | method | `0x7cdd0` | `LAB_1007cdd0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `HurtHuman` | GameDll7 | `0x81810` | `—` | `cScrEntity` | method | `0x7ed00` | `FUN_1007ed00` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `Destroy` | GameDll7 | `0x81810` | `—` | `cScrEntity` | method | `0x7eec0` | `FUN_1007eec0` | `0, 0x7fffffd8, -1, -1` |
| `UnlinkWithPhysics` | GameDll7 | `0x81810` | `—` | `cScrEntity` | method | `0x7efc0` | `LAB_1007efc0` | `0, 0x7fffffd8, -1, -1` |
| `SetScale` | GameDll7 | `0x81810` | `—` | `cScrEntity` | method | `0x7efe0` | `LAB_1007efe0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `PlayByteSound` | GameDll7 | `0x81810` | `—` | `cScrEntity` | method | `0x814c0` | `FUN_100814c0` | `0, 0x7fffffd8, -1, -1` |
| `HurtHuman` | GameDll7 | `0x89510` | `—` | `cScrItemEx` | method | `0x86b00` | `FUN_10086b00` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `DropObject` | GameDll7 | `0x89510` | `—` | `cScrItemEx` | method | `0x86c20` | `FUN_10086c20` | `0, 0x7fffffd8, -1, -1` |
| `MoveToPos` | GameDll7 | `0x89510` | `—` | `cScrItemEx` | method | `0x86d30` | `LAB_10086d30` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `WaitForTask` | GameDll7 | `0x8ee40` | `—` | `cBaseClass` | method | `0x8ebc0` | `FUN_1008ebc0` | `1, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `Wait` | GameDll7 | `0x8ee40` | `—` | `cBaseClass` | method | `0x8e660` | `LAB_1008e660` | `1, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `WaitForStreamSound` | GameDll7 | `0x8ee40` | `—` | `cBaseClass` | method | `0x8e8e0` | `FUN_1008e8e0` | `1, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `PlaySoundEffect` | GameDll7 | `0x8ee40` | `—` | `cBaseClass` | method | `0x8e680` | `FUN_1008e680` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `StopSoundEffect` | GameDll7 | `0x8ee40` | `—` | `cBaseClass` | method | `0x8e7d0` | `FUN_1008e7d0` | `0, 0x7fffffd8, -1, -1` |
| `SetMissionId` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x926d0` | `LAB_100926d0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `CreateTask` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92500` | `FUN_10092500` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `EndCreateTask` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92f80` | `FUN_10092f80` | `0, 0x7fffffd8, -1, -1` |
| `SetTaskId` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92700` | `LAB_10092700` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetTaskPrimary` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92720` | `LAB_10092720` | `0, 0x7fffffd8, -1, -1` |
| `SetTaskTimeLimit` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92760` | `LAB_10092760` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetTaskAnimalToShoot` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92780` | `LAB_10092780` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetTaskObjToFind` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x927c0` | `LAB_100927c0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `InsertSecondaryTask` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | — | `—` | `0, 0x7fffffd8, -1, -1` |
| `ActivateTriggerOnEnd` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x927f0` | `FUN_100927f0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ActivateTriggerOnFail` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92910` | `FUN_10092910` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ForceActivateTriggerOnEnd` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92a30` | `FUN_10092a30` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `BeginTaskMsg` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92c60` | `FUN_10092c60` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `EndTaskMsg` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92dd0` | `FUN_10092dd0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetSoundToBePlayed` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92b50` | `FUN_10092b50` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetBeginMessageForTask` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92f40` | `LAB_10092f40` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetEndMessageForTask` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x92f60` | `LAB_10092f60` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetTargetLimit` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x93090` | `FUN_10093090` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `MissionHasTargetLimit` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x931a0` | `LAB_100931a0` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `SetMissionOnFleePointReachedAction` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x931c0` | `LAB_100931c0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetTaskAvailableTarget` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x931e0` | `LAB_100931e0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `IsBonusTask` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x93200` | `LAB_10093200` | `0, 0x7fffffd8, -1, -1` |
| `SetCompleteCondition` | GameDll7 | `0x95bd0` | `—` | `cScrEntity` | method | `0x93210` | `LAB_10093210` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `Add_Task` | GameDll7 | `0xad9e0` | `—` | `cBaseClass` | method | `0xac630` | `FUN_100ac630` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0xffffffff` |
| `Add_Senses` | GameDll7 | `0xad9e0` | `—` | `cBaseClass` | method | `0xaa7e0` | `FUN_100aa7e0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `ConfigTaskPriority` | GameDll7 | `0xad9e0` | `—` | `cBaseClass` | method | `0xa4d00` | `FUN_100a4d00` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `ConfigTaskCmd` | GameDll7 | `0xad9e0` | `—` | `cBaseClass` | method | `0xa4e20` | `FUN_100a4e20` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0xffffffff` |
| `SetHelloDistance` | GameDll7 | `0xad9e0` | `—` | `cBaseClass` | method | `0xa4f40` | `LAB_100a4f40` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetFollowDistance` | GameDll7 | `0xad9e0` | `—` | `cBaseClass` | method | `0xa4f50` | `LAB_100a4f50` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `AllowTask` | GameDll7 | `0xad9e0` | `—` | `cBaseClass` | method | `0xa4f60` | `FUN_100a4f60` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `DenyTask` | GameDll7 | `0xad9e0` | `—` | `cBaseClass` | method | `0xa5070` | `FUN_100a5070` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `Resume` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbe7e0` | `FUN_100be7e0` | `0, 0x7fffffd8, -1, -1` |
| `CloseErrorPage` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbe910` | `FUN_100be910` | `0, 0x7fffffd8, -1, -1` |
| `Restart` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc0770` | `FUN_100c0770` | `0, 0x7fffffd8, -1, -1` |
| `GameLoadLevel` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbea20` | `FUN_100bea20` | `0, 0x7fffffd8, -1, -1` |
| `GameLoadHunt` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbeb60` | `FUN_100beb60` | `0, 0x7fffffd8, -1, -1` |
| `GameLoadMission` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbeca0` | `FUN_100beca0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `Quit` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbef40` | `FUN_100bef40` | `0, 0x7fffffd8, -1, -1` |
| `IsMainPageOn` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf060` | `LAB_100bf060` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `SaveProfile` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc0990` | `FUN_100c0990` | `0, 0x7fffffd8, -1, -1` |
| `SaveMapProfile` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc0fa0` | `FUN_100c0fa0` | `0, 0x7fffffd8, -1, -1` |
| `SetMission` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf070` | `LAB_100bf070` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `GetGameStats` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc12f0` | `FUN_100c12f0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `GetMissionStats` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc17f0` | `FUN_100c17f0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `GetNumAnimalsKilled` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc1f90` | `FUN_100c1f90` | `0, 0x7fffffea, -1, 0x7fffffea, 0xffffffff` |
| `RestartMission` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbe6a0` | `FUN_100be6a0` | `0, 0x7fffffd8, -1, -1` |
| `GetMissionType` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc20a0` | `FUN_100c20a0` | `0, 0x7fffffea, -1, -1` |
| `OpenMenu` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf0a0` | `FUN_100bf0a0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `EquipPlayerWith` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc21d0` | `FUN_100c21d0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `GetPlayerWeapons` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc2300` | `FUN_100c2300` | `0, 0x7fffffe7, -1, 0x7fffffea, 0xffffffff` |
| `GetMaxItems` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc2580` | `FUN_100c2580` | `0, 0x7fffffea, -1, -1` |
| `GetSkin` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc2440` | `FUN_100c2440` | `0, 0x7fffffe7, -1, 0x7fffffea, 0xffffffff` |
| `OpenMenuPage` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf1b0` | `FUN_100bf1b0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `PlayMenuSound` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf2c0` | `FUN_100bf2c0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `StopMenuSound` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf3e0` | `FUN_100bf3e0` | `0, 0x7fffffd8, -1, -1` |
| `SetMapCompleted` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc26c0` | `FUN_100c26c0` | `0, 0x7fffffd8, -1, -1` |
| `GetNumStumps` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf4f0` | `LAB_100bf4f0` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `GetUnlockStumps` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc27d0` | `FUN_100c27d0` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `SkipHunting` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc2940` | `FUN_100c2940` | `0, 0x7fffffd8, -1, -1` |
| `GameLoadBonus` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbedf0` | `FUN_100bedf0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `GetBonusStats` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc1bd0` | `FUN_100c1bd0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `IsMissionCompleted` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc2a50` | `FUN_100c2a50` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `IsMapCompleted` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xc2bd0` | `FUN_100c2bd0` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `GetDeviceName` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf500` | `FUN_100bf500` | `0, 0x7fffffe7, -1, -1` |
| `GetResolution` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf610` | `FUN_100bf610` | `0, 0x7fffffe7, -1, -1` |
| `SetResolution` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf740` | `FUN_100bf740` | `0, 0x7fffffd8, -1, -1` |
| `IsKeyAvail` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf8a0` | `LAB_100bf8a0` | `0, 0x7fffffdd, -1, 0x7fffffea, 0xffffffff` |
| `GetKeyName` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf8d0` | `FUN_100bf8d0` | `0, 0x7fffffe7, -1, 0x7fffffea, 0xffffffff` |
| `SetCTBMultiplier` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbf9f0` | `FUN_100bf9f0` | `0, 0x7fffffd8, -1, -1` |
| `SetViewDistance` | GameDll7 | `0xc2d40` | `—` | `cBaseClass` | static | `0xbfb10` | `FUN_100bfb10` | `0, 0x7fffffd8, -1, -1` |
| `AddLensFlare` | GameDll7 | `0xe37c0` | `—` | `cHUDItem` | static | `0xdd7f0` | `FUN_100dd7f0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetMap` | GameDll7 | `0xe37c0` | `—` | `cHUDItem` | static | `0xdd960` | `LAB_100dd960` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetSize` | GameDll7 | `0xe37c0` | `—` | `cHUDItem` | static | `0xcb020` | `LAB_100cb020` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetColor` | GameDll7 | `0xe37c0` | `—` | `cHUDItem` | static | `0xcaf90` | `LAB_100caf90` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `SetShader` | GameDll7 | `0xe37c0` | `—` | `cHUDItem` | static | `0xcb050` | `FUN_100cb050` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetFlags` | GameDll7 | `0xe37c0` | `—` | `cHUDItem` | static | `0xcb160` | `LAB_100cb160` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetDotFactor` | GameDll7 | `0xe37c0` | `—` | `cHUDItem` | static | `0xcb170` | `LAB_100cb170` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetSunFlare` | GameDll7 | `0xe37c0` | `—` | `cHUDItem` | static | `0xcb180` | `LAB_100cb180` | `0, 0x7fffffd8, -1, -1` |
| `LoadLevel` | GameDll7 | `0xe9fe0` | `—` | `cBaseClass` | static | `0xe7b30` | `FUN_100e7b30` | `0, 0x7fffffd8, -1, -1` |
| `SetMapIsComplete` | GameDll7 | `0xe9fe0` | `—` | `cBaseClass` | static | `0xe9070` | `FUN_100e9070` | `0, 0x7fffffd8, -1, -1` |
| `InvertYAxis` | GameDll7 | `0xe9fe0` | `—` | `cBaseClass` | static | `0xe7c70` | `FUN_100e7c70` | `0, 0x7fffffd8, -1, -1` |
| `ResetProfile` | GameDll7 | `0xe9fe0` | `—` | `cBaseClass` | static | `0xe9180` | `FUN_100e9180` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `GetMapName` | GameDll7 | `0xe9fe0` | `—` | `cBaseClass` | static | `0xe9290` | `FUN_100e9290` | `0, 0x7fffffe7, -1, 0x7fffffea, 0xffffffff` |
| `PlayVibration` | GameDll7 | `0xe9fe0` | `—` | `cBaseClass` | static | `0xe7d80` | `FUN_100e7d80` | `0, 0x7fffffd8, -1, -1` |
| `UpdateSave` | GameDll7 | `0xe9fe0` | `—` | `cBaseClass` | static | `0xe9510` | `FUN_100e9510` | `0, 0x7fffffd8, -1, -1` |
| `GetBonusName` | GameDll7 | `0xe9fe0` | `—` | `cBaseClass` | static | `0xe93d0` | `FUN_100e93d0` | `0, 0x7fffffe7, -1, 0x7fffffea, 0xffffffff` |
| `LoadGame` | GameDll7 | `0xe9fe0` | `—` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffd8, -1, -1` |
| `IsDiscInserted` | GameDll7 | `0xe9fe0` | `—` | `cBaseClass` | static | `0xe7e90` | `FUN_100e7e90` | `0, 0x7fffffdd, -1, -1` |
| `SetHumanSize` | CommonDll7 | `0xeae0` | `cScrHuman` | `cScrCharacter` | method | `0xe040` | `LAB_1000e040` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `IsCrouched` | CommonDll7 | `0xeae0` | `cScrHuman` | `cScrCharacter` | method | `0xe070` | `LAB_1000e070` | `0, 0x7fffffdd, -1, -1` |
| `SetCrouched` | CommonDll7 | `0xeae0` | `cScrHuman` | `cScrCharacter` | method | `0xe0a0` | `LAB_1000e0a0` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `SetLOD` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | — | `SetLOD` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `Activate` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | — | `Activate` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `Deactivate` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | — | `Deactivate` | `0, 0x7fffffd8, -1, -1` |
| `StartTask` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe0d0` | `LAB_1000e0d0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `StopTask` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe100` | `LAB_1000e100` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `IsTaskCompleted` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe160` | `LAB_1000e160` | `0, 0x7fffffdd, -1, 0x7fffffe7, 0xffffffff` |
| `SetTaskParam` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe130` | `LAB_1000e130` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `WaitForTask` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe000` | `LAB_1000e000` | `1, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetTeam` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe030` | `LAB_1000e030` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetBrain` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe060` | `LAB_1000e060` | `0, 0x7fffffdd, -1, 0x7fffffe7, 0xffffffff` |
| `SetBrainParam` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe0c0` | `LAB_1000e0c0` | `0, 0x7fffffdd, -1, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `SetBrainParamI` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe0f0` | `LAB_1000e0f0` | `0, 0x7fffffdd, -1, 0x7fffffe7, 0x7fffffea, 0xffffffff` |
| `EnablePathFinding` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe120` | `LAB_1000e120` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `EnablePathFindingBlocker` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe150` | `LAB_1000e150` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `ForceBrainActivate` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xdff0` | `LAB_1000dff0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `ItemPick` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe020` | `LAB_1000e020` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `ItemDrop` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe090` | `LAB_1000e090` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `ItemPick` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe020` | `LAB_1000e020` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `ItemGive` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe050` | `LAB_1000e050` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `ItemActivate` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe0b0` | `LAB_1000e0b0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `LockInput` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe0e0` | `LAB_1000e0e0` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `SetStaticBound` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe080` | `LAB_1000e080` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `SetCharacterFlags` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe110` | `LAB_1000e110` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `ResetCharacterFlags` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe140` | `LAB_1000e140` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `TestCharacterFlags` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xdfe0` | `LAB_1000dfe0` | `0, 0x7fffffdd, -1, 0x7fffffea, 0xffffffff` |
| `BrainFreeze` | CommonDll7 | `0x10700` | `cScrCharacter` | `cScrEntity` | method | `0xe010` | `LAB_1000e010` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `ResetAngles` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe0d0` | `LAB_1000e0d0` | `0, 0x7fffffd8, -1, -1` |
| `ApplyOffset` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe100` | `LAB_1000e100` | `0, 0x7fffffd8, -1, -1` |
| `AddAnimPak` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe160` | `LAB_1000e160` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ForceBodyState` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe130` | `LAB_1000e130` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `StartTask` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe000` | `LAB_1000e000` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ForceStartTask` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe030` | `LAB_1000e030` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `StopTask` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe060` | `LAB_1000e060` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `StopAllTasks` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe080` | `LAB_1000e080` | `0, 0x7fffffd8, -1, -1` |
| `IsTaskCompleted` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe0f0` | `LAB_1000e0f0` | `0, 0x7fffffdd, -1, 0x7fffffe7, 0xffffffff` |
| `SetTaskParam` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe0c0` | `LAB_1000e0c0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `WaitForTask` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe120` | `LAB_1000e120` | `1, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetBrain` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe150` | `LAB_1000e150` | `0, 0x7fffffdd, -1, 0x7fffffe7, 0xffffffff` |
| `SetBrainParam` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xdff0` | `LAB_1000dff0` | `0, 0x7fffffdd, -1, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `SetBrainParamI` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe020` | `LAB_1000e020` | `0, 0x7fffffdd, -1, 0x7fffffe7, 0x7fffffea, 0xffffffff` |
| `EnablePathFinding` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe050` | `LAB_1000e050` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `ForceBrainActivate` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe090` | `LAB_1000e090` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `SetCharacterFlags` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe0b0` | `LAB_1000e0b0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `ResetCharacterFlags` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe0e0` | `LAB_1000e0e0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `TestCharacterFlags` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe110` | `LAB_1000e110` | `0, 0x7fffffdd, -1, 0x7fffffea, 0xffffffff` |
| `BrainFreeze` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe140` | `LAB_1000e140` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `GetBrainParam` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xdfe0` | `LAB_1000dfe0` | `0, 0x7fffffe7, -1, 0x7fffffe7, 0xffffffff` |
| `AddSA` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe010` | `LAB_1000e010` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `PlaySA` | CommonDll7 | `0x293c0` | `cScrCharacterEx` | `cScrEntity` | method | `0xe040` | `LAB_1000e040` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `SetHumanSize` | CommonDll7 | `0x298a0` | `cScrHumanEx` | `cScrCharacterEx` | method | `0xe070` | `LAB_1000e070` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `IsCrouched` | CommonDll7 | `0x298a0` | `cScrHumanEx` | `cScrCharacterEx` | method | `0xe0a0` | `LAB_1000e0a0` | `0, 0x7fffffdd, -1, -1` |
| `SetCrouched` | CommonDll7 | `0x298a0` | `cScrHumanEx` | `cScrCharacterEx` | method | `0x25820` | `LAB_10025820` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `ResetAngles` | CommonDll7 | `0x298a0` | `cScrHumanEx` | `cScrCharacterEx` | method | `0xe0d0` | `LAB_1000e0d0` | `0, 0x7fffffd8, -1, -1` |
| `ResetTorso` | CommonDll7 | `0x298a0` | `cScrHumanEx` | `cScrCharacterEx` | method | `0x25830` | `LAB_10025830` | `0, 0x7fffffd8, -1, -1` |
| `ApplyOffset` | CommonDll7 | `0x298a0` | `cScrHumanEx` | `cScrCharacterEx` | method | `0xe100` | `LAB_1000e100` | `0, 0x7fffffd8, -1, -1` |
| `GetAttachTag` | CommonDll7 | `0x31570` | `cScrItem` | `cScrEntity` | method | `0xe0d0` | `LAB_1000e0d0` | `0, 0x7fffffe7, -1, -1` |
| `GetAttachTag` | CommonDll7 | `0x33e20` | `cScrItemEx` | `cScrEntity` | method | `0xe0d0` | `LAB_1000e0d0` | `0, 0x7fffffe7, -1, -1` |
| `GetHealth` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40750` | `LAB_10040750` | `0, 0x7fffffe9, -1, -1` |
| `IsDead` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x407c0` | `LAB_100407c0` | `0, 0x7fffffdd, -1, -1` |
| `IsPlayer` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40490` | `LAB_10040490` | `0, 0x7fffffdd, -1, -1` |
| `SetModel` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40670` | `LAB_10040670` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `PrecacheModel` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x406e0` | `LAB_100406e0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `Attach` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40740` | `LAB_10040740` | `0, 0x7fffffdd, -1, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `Detach` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x407b0` | `LAB_100407b0` | `0, 0x7fffffd8, -1, -1` |
| `LinkPart` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40480` | `LAB_10040480` | `0, 0x7fffffdd, -1, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `UnlinkPart` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40510` | `LAB_10040510` | `0, 0x7fffffdd, -1, -1` |
| `GetScrObjIdx` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x406d0` | `LAB_100406d0` | `0, 0x7fffffea, -1, -1` |
| `GetScrObjIdxParent` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40730` | `LAB_10040730` | `0, 0x7fffffea, -1, -1` |
| `SetVisible` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40580` | `LAB_10040580` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `SetRayHittable` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40600` | `LAB_10040600` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `SetStaticCollFlag` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40660` | `LAB_10040660` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `FindEntity` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x407a0` | `LAB_100407a0` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `SpawnEntity` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x405f0` | `LAB_100405f0` | `0, 0x7fffffea, -1, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `Phys_CreateMover` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40470` | `LAB_10040470` | `0, 0x7fffffd8, -1, -1` |
| `Phys_CreateBox` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40500` | `LAB_10040500` | `0, 0x7fffffd8, -1, -1` |
| `Phys_CreateJoint` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40570` | `LAB_10040570` | `0, 0x7fffffd8, -1, -1` |
| `Phys_CreateHinge` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x405e0` | `LAB_100405e0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `Phys_SetMass` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40790` | `LAB_10040790` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `Phys_TestColision` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40650` | `LAB_10040650` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `Phys_TestColisionObj` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x406c0` | `LAB_100406c0` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `Phys_SetPlayer` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40720` | `LAB_10040720` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `Phys_SetDimension` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40460` | `LAB_10040460` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `Phys_SetOffset` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x404f0` | `LAB_100404f0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `Phys_ResetDimension` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x405d0` | `LAB_100405d0` | `0, 0x7fffffd8, -1, -1` |
| `Phys_ResetOffset` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40560` | `LAB_10040560` | `0, 0x7fffffd8, -1, -1` |
| `Phys_SetSpeedLimit` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40640` | `LAB_10040640` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `Phys_SetFly` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x406b0` | `LAB_100406b0` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `Phys_SetCanSwim` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40710` | `LAB_10040710` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `Phys_SignalCollision` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40780` | `LAB_10040780` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0x7fffffdd, 0xffffffff` |
| `Phys_UseAngles` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40450` | `LAB_10040450` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `Phys_GetSpeed` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x404e0` | `LAB_100404e0` | `0, 0x7fffffe9, -1, -1` |
| `BeginCamera` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40550` | `LAB_10040550` | `0, 0x7fffffd8, -1, -1` |
| `EndCamera` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x405c0` | `LAB_100405c0` | `0, 0x7fffffd8, -1, -1` |
| `GetPosX` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40630` | `LAB_10040630` | `0, 0x7fffffe9, -1, -1` |
| `GetPosY` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x406a0` | `LAB_100406a0` | `0, 0x7fffffe9, -1, -1` |
| `GetPosZ` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40700` | `LAB_10040700` | `0, 0x7fffffe9, -1, -1` |
| `GetAngleX` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40770` | `LAB_10040770` | `0, 0x7fffffe9, -1, -1` |
| `GetAngleY` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40440` | `LAB_10040440` | `0, 0x7fffffe9, -1, -1` |
| `GetAngleZ` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x404d0` | `LAB_100404d0` | `0, 0x7fffffe9, -1, -1` |
| `SetPos` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40540` | `LAB_10040540` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `Rotate` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x405b0` | `LAB_100405b0` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `GetDistance` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40620` | `LAB_10040620` | `0, 0x7fffffe9, -1, 0x7fffffea, 0xffffffff` |
| `Use` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40610` | `LAB_10040610` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `GetTarget` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40690` | `LAB_10040690` | `0, 0x7fffffe7, -1, -1` |
| `SetTarget` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x404c0` | `LAB_100404c0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `GetTargetNext` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40760` | `LAB_10040760` | `0, 0x7fffffe7, -1, 0x7fffffea, 0xffffffff` |
| `GetTargetCount` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40430` | `LAB_10040430` | `0, 0x7fffffea, -1, -1` |
| `GetTargetName` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x406f0` | `LAB_100406f0` | `0, 0x7fffffe7, -1, -1` |
| `SetTargetName` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40530` | `LAB_10040530` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SetState` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x404b0` | `LAB_100404b0` | `0, 0x7fffffdd, -1, 0x7fffffe7, 0xffffffff` |
| `SetAnim` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40520` | `LAB_10040520` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `WaitForAnim` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x405a0` | `LAB_100405a0` | `1, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `Hurt` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40680` | `LAB_10040680` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `LantentKill` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x404a0` | `LAB_100404a0` | `0, 0x7fffffd8, -1, -1` |
| `SendGameMessage` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | method | `0x40590` | `LAB_10040590` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ConsoleExec` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ConsoleRealExec` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `DebugMessage` | CommonDll7 | `0x41690` | `cScrEntity` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `Stop` | CommonDll7 | `0x4c3b0` | `cScrTrain` | `cScrEntity` | method | `0xe0d0` | `LAB_1000e0d0` | `0, 0x7fffffd8, -1, -1` |
| `Translate` | CommonDll7 | `0x4c3b0` | `cScrTrain` | `cScrEntity` | method | `0xe130` | `LAB_1000e130` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `Teleport` | CommonDll7 | `0x4c3b0` | `cScrTrain` | `cScrEntity` | method | `0xe000` | `LAB_1000e000` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `Move` | CommonDll7 | `0x4c3b0` | `cScrTrain` | `cScrEntity` | method | `0xe160` | `LAB_1000e160` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `MoveToEntity` | CommonDll7 | `0x4c3b0` | `cScrTrain` | `cScrEntity` | method | `0xe030` | `LAB_1000e030` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `TargetReached` | CommonDll7 | `0x4c3b0` | `cScrTrain` | `cScrEntity` | method | `0xe100` | `LAB_1000e100` | `0, 0x7fffffdd, -1, -1` |
| `PlayerIn` | CommonDll7 | `0x4c3b0` | `cScrTrain` | `cScrEntity` | method | — | `PlayerIn` | `0, 0x7fffffdd, -1, 0x7fffffea, 0xffffffff` |
| `WaitForStop` | CommonDll7 | `0x4c3b0` | `cScrTrain` | `cScrEntity` | method | `0xe060` | `LAB_1000e060` | `1, 0x7fffffd8, -1, -1` |
| `Stop` | CommonDll7 | `0x4c7b0` | `cScrRotating` | `cScrEntity` | method | `0xe0d0` | `LAB_1000e0d0` | `0, 0x7fffffd8, -1, -1` |
| `Move` | CommonDll7 | `0x4c7b0` | `cScrRotating` | `cScrEntity` | method | `0xe130` | `LAB_1000e130` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `TargetReached` | CommonDll7 | `0x4c7b0` | `cScrRotating` | `cScrEntity` | method | `0xe100` | `LAB_1000e100` | `0, 0x7fffffdd, -1, -1` |
| `WaitForStop` | CommonDll7 | `0x4c7b0` | `cScrRotating` | `cScrEntity` | method | `0xe160` | `LAB_1000e160` | `1, 0x7fffffd8, -1, -1` |
| `GetSpeed` | CommonDll7 | `0x60880` | `cScrWayPoint` | `cScrEntity` | method | `0xe0d0` | `LAB_1000e0d0` | `0, 0x7fffffe9, -1, -1` |
| `GetWait` | CommonDll7 | `0x60880` | `cScrWayPoint` | `cScrEntity` | method | `0xe100` | `LAB_1000e100` | `0, 0x7fffffe9, -1, -1` |
| `Fire` | CommonDll7 | `0x663a0` | `—` | `cScrItemEx` | method | `0xe100` | `LAB_1000e100` | `0, 0x7fffffd8, -1, -1` |
| `Reload` | CommonDll7 | `0x663a0` | `—` | `cScrItemEx` | method | `0xe130` | `LAB_1000e130` | `0, 0x7fffffd8, -1, -1` |
| `UpdateStats` | CommonDll7 | `0x663a0` | `—` | `cScrItemEx` | method | `0xe030` | `LAB_1000e030` | `0, 0x7fffffd8, -1, -1` |
| `SetAmmoCount` | CommonDll7 | `0x663a0` | `—` | `cScrItemEx` | method | `0xe160` | `LAB_1000e160` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `GetClipSize` | CommonDll7 | `0x663a0` | `—` | `cScrItemEx` | method | `0xe000` | `LAB_1000e000` | `0, 0x7fffffea, -1, -1` |
| `SetAim` | CommonDll7 | `0x663a0` | `—` | `cScrItemEx` | method | `0x656e0` | `FUN_100656e0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetWeaponTunningParam` | CommonDll7 | `0x663a0` | `—` | `cScrItemEx` | method | `0x65800` | `FUN_10065800` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `AIHuntAddMemberToGroup` | CommonDll7 | `0xa1080` | `cScrAIHunt` | `cScrEntity` | static | `0xa0a00` | `FUN_100a0a00` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `AIHuntGroupAlreadyCreated` | CommonDll7 | `0xa1080` | `cScrAIHunt` | `cScrEntity` | static | `0xa0b20` | `FUN_100a0b20` | `0, 0x7fffffea, -1, 0x7fffffea, 0xffffffff` |
| `AIHuntCreateDailyRoutine` | CommonDll7 | `0xa1080` | `cScrAIHunt` | `cScrEntity` | static | — | `pcVar3` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `AIHuntAddTaskInfo` | CommonDll7 | `0xa1080` | `cScrAIHunt` | `cScrEntity` | static | — | `pcVar3` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `TestFlags` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdc130` | `FUN_100dc130` | `0, 0x7fffffdd, -1, 0x7fffffea, 0xffffffff` |
| `SetFlags` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdc160` | `FUN_100dc160` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `ClearFlags` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdc190` | `FUN_100dc190` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `GetChildCount` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xde0b0` | `FUN_100de0b0` | `0, 0x7fffffea, -1, -1` |
| `GetChild` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xde1d0` | `FUN_100de1d0` | `0, 0x7fffffd5, -1, 0x7fffffea, 0xffffffff` |
| `GetChildIndex` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xde310` | `FUN_100de310` | `0, 0x7fffffea, -1, 0x7fffffd5, 0xffffffff` |
| `GetChildByName` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xde450` | `FUN_100de450` | `0, 0x7fffffd5, -1, 0x7fffffe7, 0xffffffff` |
| `BringToFront` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xde5f0` | `FUN_100de5f0` | `0, 0x7fffffd8, -1, 0x7fffffd5, 0xffffffff` |
| `SendToBack` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xde740` | `FUN_100de740` | `0, 0x7fffffd8, -1, 0x7fffffd5, 0xffffffff` |
| `LocalPointInView` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdc0b0` | `FUN_100dc0b0` | `0, 0x7fffffdd, -1, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `GlobalPointInView` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xde9f0` | `FUN_100de9f0` | `0, 0x7fffffdd, -1, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `WaitForObject` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdc600` | `LAB_100dc600` | `1, 0x7fffffd8, -1, 0x7fffffd5, 0xffffffff` |
| `WaitEvent` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdc5d0` | `LAB_100dc5d0` | `1, 0x7fffffd8, -1, -1` |
| `PostEvent` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdc7a0` | `FUN_100dc7a0` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `ToLocalX` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdc1c0` | `FUN_100dc1c0` | `0, 0x7fffffe9, -1, 0x7fffffe9, 0xffffffff` |
| `ToLocalY` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdc330` | `FUN_100dc330` | `0, 0x7fffffe9, -1, 0x7fffffe9, 0xffffffff` |
| `FindSelectable` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdf860` | `FUN_100df860` | `0, 0x7fffffd5, -1, 0x7fffffd5, 0x7fffffea, 0xffffffff` |
| `GetThis` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdd5f0` | `LAB_100dd5f0` | `0, 0x7fffffd5, -1, -1` |
| `GetName` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | — | `GetName_exref` | `0, 0x7fffffe7, -1, -1` |
| `AddChild` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdd4d0` | `FUN_100dd4d0` | `0, 0x7fffffd8, -1, 0x7fffffd5, 0xffffffff` |
| `CreateChild` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdd390` | `FUN_100dd390` | `0, 0x7fffffd5, -1, 0x7fffffe7, 0x7fffffe7, 0xffffffff` |
| `RemoveChild` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdeee0` | `FUN_100deee0` | `0, 0x7fffffd8, -1, 0x7fffffd5, 0xffffffff` |
| `DestroyDynamicChildrens` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdf030` | `FUN_100df030` | `0, 0x7fffffd8, -1, -1` |
| `SetSkinClass` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdc8d0` | `FUN_100dc8d0` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `MgrPushModal` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | static | `0xdca90` | `FUN_100dca90` | `0, 0x7fffffd8, -1, 0x7fffffd5, 0xffffffff` |
| `MgrPopModal` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | static | `0xdcbb0` | `FUN_100dcbb0` | `0, 0x7fffffd8, -1, -1` |
| `MgrGetActionTip` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | static | `0xdccd0` | `FUN_100dccd0` | `0, 0x7fffffea, -1, -1` |
| `MgrGetMessageTip` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | static | `0xdcdf0` | `FUN_100dcdf0` | `0, 0x7fffffe7, -1, -1` |
| `MgrUpdateObject` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | static | `0xdcf10` | `FUN_100dcf10` | `0, 0x7fffffd8, -1, 0x7fffffd5, 0xffffffff` |
| `MgrSuspendObject` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | static | `0xdd030` | `FUN_100dd030` | `0, 0x7fffffd8, -1, 0x7fffffd5, 0xffffffff` |
| `MgrGetInitType` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | static | `0xdd150` | `FUN_100dd150` | `0, 0x7fffffea, -1, -1` |
| `MgrGetIsMainMenu` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | static | `0xdd270` | `FUN_100dd270` | `0, 0x7fffffdd, -1, -1` |
| `ConsoleGetFloat` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdd610` | `FUN_100dd610` | `0, 0x7fffffe9, -1, 0x7fffffe7, 0xffffffff` |
| `ConsoleGetInt` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdd750` | `FUN_100dd750` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `ConsoleSet` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdd890` | `FUN_100dd890` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `ConsoleExec` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | method | `0xdd9e0` | `FUN_100dd9e0` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `StrDeleteChar` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | static | `0xddb10` | `LAB_100ddb10` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0xffffffff` |
| `StrInsertChar` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | static | `0xddb80` | `LAB_100ddb80` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `GetASCII` | CommonDll7 | `0xdfd20` | `—` | `cScrContainer` | static | `0xddbe0` | `LAB_100ddbe0` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `Clear` | CommonDll7 | `0xe7610` | `cGuiEvent` | `cBaseClass` | method | — | `Clear` | `0, 0x7fffffd8, -1, -1` |
| `Get` | CommonDll7 | `0xe7610` | `cGuiEvent` | `cBaseClass` | method | — | `Get` | `0, 0x7fffffd8, -1, -1` |
| `Print` | CommonDll7 | `0xe7610` | `cGuiEvent` | `cBaseClass` | method | — | `Print` | `0, 0x7fffffd8, -1, -1` |
| `UsingMoreControllers` | CommonDll7 | `0xe7610` | `cGuiEvent` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffdd, -1, -1` |
| `GetLastControllerUsed` | CommonDll7 | `0xe7610` | `cGuiEvent` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffea, -1, -1` |
| `SetActiveControllers` | CommonDll7 | `0xe7610` | `cGuiEvent` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `SetEmptyBuffer` | CommonDll7 | `0xe7610` | `cGuiEvent` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `CheckControllerError` | CommonDll7 | `0xe7610` | `cGuiEvent` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `GetSound` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `GetSound` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `PlaySound` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `PlaySoundA` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `GetBrush` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `GetBrush` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `DrawBrush` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `DrawBrush` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `DrawBrushColor` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `DrawBrushColor` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `EnableBrushClipping` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `EnableBrushClipping` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `DisableBrushClipping` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `DisableBrushClipping` | `0, 0x7fffffd8, -1, -1` |
| `DrawHLine` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `DrawHLine` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `DrawVLine` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `DrawVLine` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `DrawLine` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `DrawLine` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `DrawRect` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `DrawRect` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `GetFont` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `GetFont` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `SetFont` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `SetFont` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `DrawText` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `DrawTextA` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `DrawTextScroll` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `DrawTextScroll` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `DrawTextClip` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `DrawTextClip` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `DrawTextClip2` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `DrawTextClip2` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `GetFirstVisible` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `GetFirstVisible` | `0, 0x7fffffea, -1, 0x7fffffe7, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `TextGetTotalWidth` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `scrTextGetTotalWidth` | `0, 0x7fffffe9, -1, 0x7fffffe7, 0xffffffff` |
| `TextGetTotalHeight` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `scrTextGetTotalHeight` | `0, 0x7fffffe9, -1, 0x7fffffe7, 0xffffffff` |
| `AMGet` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMGet` | `0, 0x7fffffea, -1, -1` |
| `AMGetChild` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMGetChild` | `0, 0x7fffffea, -1, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `AMDelete` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMDelete` | `0, 0x7fffffd8, -1, -1` |
| `AMDeleteChild` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMDeleteChild` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `AMSetModel` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMSetModel` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffe7, 0xffffffff` |
| `AMDraw` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMDraw` | `0, 0x7fffffd8, -1, -1` |
| `AMTouch` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMTouch` | `0, 0x7fffffd8, -1, -1` |
| `AMIsLoading` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMIsLoading` | `0, 0x7fffffdd, -1, -1` |
| `AMSetWorldSpace` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMSetWorldSpace` | `0, 0x7fffffd8, -1, 0x7fffffdd, 0xffffffff` |
| `AMSetAnim` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMSetAnim` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `AMSetFov` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMSetFov` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `AMSetZoom` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMSetZoom` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `AMSetCamXAngle` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMSetCamXAngle` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `AMSetYRotSpeed` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMSetYRotSpeed` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `AMSetYRot` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMSetYRot` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `AMSetRefHeight` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMSetRefHeight` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `AMSetAmbLight` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMSetAmbLight` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `AMSetDirLight` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMSetDirLight` | `0, 0x7fffffd8, -1, 0x7fffffea, 0xffffffff` |
| `AMGetBonePosX` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMGetBonePosX` | `0, 0x7fffffe9, -1, 0x7fffffe7, 0xffffffff` |
| `AMGetBonePosY` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMGetBonePosY` | `0, 0x7fffffe9, -1, 0x7fffffe7, 0xffffffff` |
| `AMGetBonePosZ` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMGetBonePosZ` | `0, 0x7fffffe9, -1, 0x7fffffe7, 0xffffffff` |
| `AMGetBoneRotX` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMGetBoneRotX` | `0, 0x7fffffe9, -1, 0x7fffffe7, 0xffffffff` |
| `AMGetBoneRotY` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMGetBoneRotY` | `0, 0x7fffffe9, -1, 0x7fffffe7, 0xffffffff` |
| `AMGetBoneRotZ` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `AMGetBoneRotZ` | `0, 0x7fffffe9, -1, 0x7fffffe7, 0xffffffff` |
| `CameraSetPos` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `CameraSetPos` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `CameraSetRot` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `CameraSetRot` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0x7fffffe9, 0x7fffffe9, 0xffffffff` |
| `CameraSetFov` | CommonDll7 | `0xf56e0` | `cGuiSkinBase` | `cScrContainer` | method | — | `CameraSetFov` | `0, 0x7fffffd8, -1, 0x7fffffe9, 0xffffffff` |
| `GetRenderDescNum` | CommonDll7 | `0xf74c0` | `—` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffea, -1, 0x7fffffe7, 0x7fffffdd, 0xffffffff` |
| `GetRenderDesc` | CommonDll7 | `0xf74c0` | `—` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffe7, -1, 0x7fffffe7, 0x7fffffea, 0x7fffffdd, 0xffffffff` |
| `GetCap` | CommonDll7 | `0xf74c0` | `—` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `ScanInt` | CommonDll7 | `0xf74c0` | `—` | `cBaseClass` | static | — | `pcVar3` | `0, 0x7fffffea, -1, 0x7fffffe7, 0x7fffffe7, 0x7fffffea, 0xffffffff` |
| `SaveGame` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `SaveGame` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `LoadGame` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `LoadGame` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `Autosave` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `Autosave` | `0, 0x7fffffea, -1, -1` |
| `DeleteGame` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `DeleteGame` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `Format` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `Format` | `0, 0x7fffffea, -1, -1` |
| `DeleteCorrupt` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `DeleteCorrupt` | `0, 0x7fffffea, -1, -1` |
| `CheckBoot` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `CheckBoot` | `0, 0x7fffffea, -1, -1` |
| `CheckCard` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `CheckCard` | `0, 0x7fffffea, -1, -1` |
| `CheckSave` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `CheckSave` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `CheckLoad` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `CheckLoad` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `CheckDelete` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `CheckDelete` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `CheckFormat` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `CheckFormat` | `0, 0x7fffffea, -1, -1` |
| `CheckAutosave` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `CheckAutosave` | `0, 0x7fffffea, -1, -1` |
| `GetStatus` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `GetStatus` | `0, 0x7fffffdd, -1, -1` |
| `CreateDevice` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `CreateDevice` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `GetMaxSaves` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `GetMaxSaves` | `0, 0x7fffffea, -1, -1` |
| `PostEvent` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `PostEvent` | `0, 0x7fffffd8, -1, 0x7fffffea, 0x7fffffea, 0x7fffffea, 0xffffffff` |
| `GetFiles` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `GetFiles` | `0, 0x7fffffea, -1, -1` |
| `GetFile` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `GetFile` | `0, 0x7fffffe7, -1, 0x7fffffea, 0xffffffff` |
| `FileExists` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `FileExists` | `0, 0x7fffffdd, -1, 0x7fffffe7, 0xffffffff` |
| `BeginSave` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `BeginSave` | `0, 0x7fffffd8, -1, -1` |
| `EndSave` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `EndSave` | `0, 0x7fffffd8, -1, -1` |
| `Reboot` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `Reboot` | `0, 0x7fffffd8, -1, -1` |
| `HasSaves` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `HasSaves` | `0, 0x7fffffdd, -1, -1` |
| `SetLastFile` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `SetLastFile` | `0, 0x7fffffd8, -1, 0x7fffffe7, 0xffffffff` |
| `SaveTemp` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `SaveTemp` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `LoadTemp` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `LoadTemp` | `0, 0x7fffffea, -1, 0x7fffffe7, 0xffffffff` |
| `MakeFileList` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `MakeFileList` | `0, 0x7fffffd8, -1, -1` |
| `GetFileInfo` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `GetFileInfo` | `0, 0x7fffffe7, -1, 0x7fffffe7, 0xffffffff` |
| `InitMenu` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `InitMenu` | `0, 0x7fffffd8, -1, -1` |
| `GetFileName` | CommonDll7 | `0x102270` | `cGuiSaveMan` | `cScrContainer` | method | — | `GetFileName` | `0, 0x7fffffe7, -1, 0x7fffffe7, 0xffffffff` |

Rows in 2c: 566. Distinct ExportFnc names in both dumps: 562.

## 3. Native usage in story scripts

Counts are call sites in each map's `MAIN.SPJ` closure (a shared script is counted on every map that lists it). The numeric column is that sum, not a unique file:line count. `in catalog` is yes when section 2 has that name. Category is assigned from the identifier, in this order: timing (`Pause`/`Wait`/`WaitForTask`/prefix `Wait`); AI (`StartTask`, `StopTask`, `StopAllTask`, `SetTaskParam`, `ForceBrainActivate`, `MoveToPos`, or name contains `Brain`/`Animal`/`AnimPak`); mission (`Task`/`Mission`/`Objective`); HUD (`Hud`/`Hint`/`PlayMessage`/`OpenMenu`/`MenuPage`); cutscene (`CutScene`/`Camera`/`Movie`/`Cinematic`/`ExecuteScript`); sound (`Sound`/`Music`); triggers (`Trigger`, `Activate`, `Deactivate`, `ActivateBy`, `ActivateAfter`, HUD activate); NPC (`NPC`/`Companion`/`Guide`/`Follow`/`Talk`/`SetCanBeAttacked`); spawn (`Spawn`/`Destroy`/`MakeVisible`/`Show`/`Hide`/`AddAnimal`/`AddEvent`/`Precache`/`AddAI`); player control (`PFlags`/`Health`/`Weapon`/`Ammo`/`Item`/`Inventory`/`Freeze`/`Teleport`/`Hurt`/`Damage`/`Crouch`/`Equip`/`LockInput`/`Knife`); else other.

### mission/task (28)

| Name | In catalog | Sum of per-map counts | Per-map counts (zeros omitted) | First example |
|---|---|---:|---|---|
| `Add_Task` | yes | 3888 | MAP3A:420 MAP3B:372 MAP3C:420 MAP4A:372 MAP7:372 MAP8:372 MAP11A:372 MAP11B:372 MAP11C:372 MAP12A:444 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:98` `brain.Add_Task("AITaskNPC_MoveToPos", 0.0);` |
| `ConfigTaskCmd` | yes | 352 | MAP1:2 MAP2A:1 MAP2B:3 MAP3A:35 MAP3B:31 MAP3C:35 MAP4A:31 MAP4B:7 MAP7:33 MAP8:37 MAP11A:31 MAP11B:31 MAP11C:31 MAP12A:44 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE2.SCR:82` `hugsm1.ConfigTaskCmd("AITaskNPC_MoveToPos", TASK_ACTION_NORMAL);` |
| `AllowTask` | yes | 348 | MAP1:5 MAP2A:1 MAP2B:6 MAP3A:35 MAP3B:31 MAP3C:35 MAP4A:31 MAP7:33 MAP8:37 MAP10:3 MAP11A:31 MAP11B:31 MAP11C:31 MAP12A:38 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:247` `::globals.hugsm1.AllowTask(AINPC_TASK_ATTACK);` |
| `ConfigTaskPriority` | yes | 324 | MAP3A:35 MAP3B:31 MAP3C:35 MAP4A:31 MAP7:31 MAP8:31 MAP11A:31 MAP11B:31 MAP11C:31 MAP12A:37 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:111` `brain.ConfigTaskPriority("AITaskNPC_MoveToPos", 1, 100);` |
| `DenyTask` | yes | 195 | MAP1:3 MAP2A:2 MAP2B:6 MAP2C:2 MAP3A:16 MAP3B:12 MAP3C:13 MAP4A:12 MAP5:3 MAP6:4 MAP7:20 MAP8:39 MAP10:4 MAP11A:12 MAP11B:12 MAP11C:12 MAP12A:23 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:202` `::globals.hugsm1.DenyTask(AINPC_TASK_ATTACK);` |
| `StopAllTasks` | yes | 160 | MAP1:5 MAP2A:9 MAP2B:20 MAP2C:6 MAP3A:4 MAP3C:8 MAP4B:16 MAP5:17 MAP6:6 MAP7:4 MAP8:5 MAP9:2 MAP10:9 MAP11B:2 MAP12A:4 MAP12B:2 MAP12C:1 HUNTMAP1:10 HUNTMAP2:10 HUNTMAP3:10 HUNTMAP4:10 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE4.SCR:33` `puma_t8_01.StopAllTasks();` |
| `CreateTask` | yes | 132 | MAP1:6 MAP2A:6 MAP2B:8 MAP2C:7 MAP3A:9 MAP3B:2 MAP3C:3 MAP4A:3 MAP4B:4 MAP5:3 MAP6:8 MAP7:9 MAP8:10 MAP9:6 MAP10:11 MAP11A:4 MAP11B:7 MAP11C:5 MAP12A:8 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:18` `m_pMission.CreateTask(TASK_RACE);` |
| `EndCreateTask` | yes | 132 | MAP1:6 MAP2A:6 MAP2B:8 MAP2C:7 MAP3A:9 MAP3B:2 MAP3C:3 MAP4A:3 MAP4B:4 MAP5:3 MAP6:8 MAP7:9 MAP8:10 MAP9:6 MAP10:11 MAP11A:4 MAP11B:7 MAP11C:5 MAP12A:8 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:23` `m_pMission.EndCreateTask();` |
| `SetTaskId` | yes | 132 | MAP1:6 MAP2A:6 MAP2B:8 MAP2C:7 MAP3A:9 MAP3B:2 MAP3C:3 MAP4A:3 MAP4B:4 MAP5:3 MAP6:8 MAP7:9 MAP8:10 MAP9:6 MAP10:11 MAP11A:4 MAP11B:7 MAP11C:5 MAP12A:8 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:19` `m_pMission.SetTaskId(1);` |
| `CompleteTask` | yes | 87 | MAP2C:8 MAP4B:4 MAP5:4 MAP6:13 MAP7:7 MAP8:13 MAP9:5 MAP10:20 MAP11A:2 MAP11C:4 MAP12A:7 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:105` `CompleteTask(2);` |
| `BeginTaskMsg` | yes | 55 | MAP1:5 MAP2A:4 MAP2B:3 MAP2C:5 MAP3A:4 MAP3C:1 MAP4A:2 MAP5:2 MAP6:3 MAP7:3 MAP8:2 MAP9:1 MAP10:2 MAP11A:1 MAP11B:3 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:20` `m_pMission.BeginTaskMsg("mis1task1_1_0");` |
| `UpdateMissionStatus` | yes | 54 | MAP1:2 MAP2A:6 MAP2B:3 MAP2C:3 MAP3A:7 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:2 MAP6:2 MAP7:2 MAP8:4 MAP9:1 MAP10:2 MAP11A:1 MAP11B:2 MAP11C:1 MAP12A:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:81` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);` |
| `EndTaskMsg` | yes | 53 | MAP1:5 MAP2A:3 MAP2B:3 MAP2C:4 MAP3A:3 MAP3C:2 MAP4A:1 MAP4B:1 MAP5:2 MAP6:3 MAP7:3 MAP8:2 MAP9:1 MAP10:2 MAP11B:3 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:21` `m_pMission.EndTaskMsg("mis1task1_1_1");` |
| `SetObjective` | yes | 45 | MAP1:1 MAP2A:1 MAP2B:3 MAP2C:7 MAP3A:5 MAP3B:1 MAP3C:2 MAP4A:1 MAP4B:1 MAP5:1 MAP6:2 MAP7:1 MAP8:2 MAP9:4 MAP10:2 MAP11B:4 MAP12A:1 MAP12B:1 MAP12C:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:9` `SetObjective("hugsm1");` |
| `SetTaskTimeLimit` | yes | 45 | MAP1:2 MAP2A:5 MAP2B:4 MAP2C:4 MAP3A:4 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:2 MAP6:2 MAP7:3 MAP8:3 MAP9:1 MAP10:2 MAP11A:1 MAP11B:2 MAP11C:1 MAP12A:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:22` `m_pMission.SetTaskTimeLimit(0);` |
| `SetMissionId` | yes | 29 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:16` `m_pMission.SetMissionId(1); // indica pozita in tabela de misiuni` |
| `UncompleteTask` | yes | 21 | MAP11A:7 MAP11C:7 MAP12C:7 | `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:25` `m_pHelicopter.UncompleteTask();` |
| `CheckMissionStatus` | yes | 19 | MAP1:1 MAP2C:1 MAP3C:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11C:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:400` `CheckMissionStatus();` |
| `ConfigTaskExtraCmd` | yes | 19 | MAP2B:2 MAP3A:2 MAP3B:1 MAP3C:1 MAP4A:1 MAP7:1 MAP8:4 MAP9:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:3 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:290` `::globals.hugs2b.ConfigTaskExtraCmd("AITaskNPC_PlayAnim", "idle", 0);` |
| `SetMission` | yes | 19 | MAP1:1 MAP2A:1 MAP3A:1 MAP4A:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP12A:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:15` `SetMission(1);` |
| `ForceStartTask` | yes | 13 | MAP1:2 MAP2C:3 MAP4B:6 MAP12A:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:214` `::globals.wtail1.ForceStartTask("AIHuntTask_IdleScare");` |
| `GlobalsCheckMissionStatus` | yes | 12 | HUNTMAP1:3 HUNTMAP2:3 HUNTMAP3:3 HUNTMAP4:3 | `re/extracted/SCRIPTS/LEVELS/HUNTMAPBASE.SCR:71` `GlobalsCheckMissionStatus();//end the mission` |
| `SetMissionOnFleePointReachedAction` | yes | 12 | MAP1:1 MAP2A:1 MAP2B:4 MAP2C:1 MAP4B:1 MAP7:1 MAP12A:2 MAP12B:1 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:49` `m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_LOSE_TARGET);` |
| `IsBonusTask` | yes | 9 | MAP1:1 MAP4B:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:47` `m_pMission.IsBonusTask();` |
| `IsTaskCompleted` | yes | 9 | MAP11A:3 MAP11C:3 MAP12C:3 | `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:49` `if (m_pHelicopter.IsTaskCompleted())` |
| `SetTaskObjToFind` | yes | 4 | MAP1:1 MAP3A:1 MAP7:1 MAP8:1 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:27` `m_pMission.SetTaskObjToFind(AI_GUIDE_ID,1);` |
| `GetTaskStatus` | yes | 1 | MAP1:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:238` `if(TASK_FAILED == GetTaskStatus(4))` |
| `SetTaskAvailableTarget` | yes | 1 | MAP1:1 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:51` `m_pMission.SetTaskAvailableTarget(2);` |

### HUD/hint/text (9)

| Name | In catalog | Sum of per-map counts | Per-map counts (zeros omitted) | First example |
|---|---|---:|---|---|
| `DeactivateHUD` | yes | 148 | MAP1:10 MAP2A:12 MAP2B:6 MAP2C:6 MAP3A:8 MAP3C:6 MAP4A:4 MAP4B:10 MAP5:6 MAP6:10 MAP7:6 MAP8:10 MAP9:8 MAP10:12 MAP11A:6 MAP11B:4 MAP11C:6 MAP12A:14 MAP12B:2 MAP12C:2 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:59` `trigger.DeactivateHUD();` |
| `PlayMessage` | yes | 125 | MAP1:12 MAP2A:10 MAP2B:5 MAP2C:8 MAP3A:6 MAP3C:3 MAP4A:5 MAP5:13 MAP6:9 MAP7:8 MAP8:10 MAP9:6 MAP10:8 MAP11A:4 MAP11B:4 MAP11C:1 MAP12A:13 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:44` `hugsm1.PlayMessage("Well, c'mon, youngster. Think you can show <key_time:2> a thing or two to Old Hugh, do ya? <key_time:2> Well, we'll just see about that. <key_time:3> ");` |
| `ActivateHUD` | yes | 74 | MAP1:5 MAP2A:6 MAP2B:3 MAP2C:3 MAP3A:4 MAP3C:3 MAP4A:2 MAP4B:5 MAP5:3 MAP6:5 MAP7:3 MAP8:5 MAP9:4 MAP10:6 MAP11A:3 MAP11B:2 MAP11C:3 MAP12A:7 MAP12B:1 MAP12C:1 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:37` `trigger.ActivateHUD();` |
| `PlayHint` | yes | 39 | MAP1:2 MAP2B:2 MAP2C:1 MAP3A:4 MAP5:1 MAP11A:1 MAP11C:1 MAP12A:2 MAPARENA:3 MAPISLAND:5 MAPTSAVO:5 HUNTMAP1:3 HUNTMAP2:3 HUNTMAP3:3 HUNTMAP4:3 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:239` `::globals.player.PlayHint("Hunt over. Follow Hugh.");` |
| `AddHUDItem` | yes | 32 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:126` `AddHUDItem(HUD_HUNTER\|HUD_TRIGGEREVENTDESC\|HUD_LENSFLARE);` |
| `OpenMenuPage` | yes | 13 | MAP1:8 MAP3A:1 MAP3B:2 MAP6:1 MAPISLAND:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:16` `OpenMenuPage(1);` |
| `DisableHudItem` | yes | 12 | MAP1:2 MAP2C:2 MAP4B:2 MAP12A:2 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:22` `DisableHudItem(HUD_STEALTH);` |
| `EnableHudItem` | yes | 8 | MAP1:1 MAP2C:1 MAP4B:1 MAP12A:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:188` `EnableHudItem(HUD_STEALTH);` |
| `OpenMenu` | yes | 4 | HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERHUNTMAP1.SCR:92` `OpenMenu("WeaponMenu");` |

### cutscene/camera/movie (5)

| Name | In catalog | Sum of per-map counts | Per-map counts (zeros omitted) | First example |
|---|---|---:|---|---|
| `DeactivateCutSceneMode` | yes | 148 | MAP1:10 MAP2A:12 MAP2B:6 MAP2C:6 MAP3A:8 MAP3C:6 MAP4A:4 MAP4B:10 MAP5:6 MAP6:10 MAP7:6 MAP8:10 MAP9:8 MAP10:12 MAP11A:6 MAP11B:4 MAP11C:6 MAP12A:14 MAP12B:2 MAP12C:2 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:61` `player.DeactivateCutSceneMode();` |
| `EndCamera` | yes | 136 | MAP1:11 MAP2A:6 MAP2B:8 MAP2C:6 MAP3A:8 MAP3C:6 MAP4B:10 MAP5:4 MAP6:10 MAP7:9 MAP8:4 MAP9:8 MAP10:10 MAP11A:6 MAP11B:4 MAP11C:6 MAP12A:16 MAP12B:2 MAP12C:2 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:48` `c1cam1.EndCamera();` |
| `ActivateCutSceneMode` | yes | 74 | MAP1:5 MAP2A:6 MAP2B:3 MAP2C:3 MAP3A:4 MAP3C:3 MAP4A:2 MAP4B:5 MAP5:3 MAP6:5 MAP7:3 MAP8:5 MAP9:4 MAP10:6 MAP11A:3 MAP11B:2 MAP11C:3 MAP12A:7 MAP12B:1 MAP12C:1 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:36` `player.ActivateCutSceneMode();` |
| `ExecuteScript` | yes | 71 | MAP1:5 MAP2A:3 MAP2B:3 MAP2C:3 MAP3A:4 MAP3B:1 MAP3C:3 MAP4A:2 MAP4B:5 MAP5:3 MAP6:5 MAP7:3 MAP8:5 MAP9:4 MAP10:5 MAP11A:3 MAP11B:2 MAP11C:3 MAP12A:7 MAP12B:1 MAP12C:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:39` `ExecuteScript("cScrCutScene1");` |
| `BeginCamera` | yes | 68 | MAP1:5 MAP2A:3 MAP2B:4 MAP2C:3 MAP3A:4 MAP3C:3 MAP4B:5 MAP5:2 MAP6:5 MAP7:5 MAP8:2 MAP9:4 MAP10:5 MAP11A:3 MAP11B:2 MAP11C:3 MAP12A:7 MAP12B:1 MAP12C:1 MAPMENU:1 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:30` `c1cam1.BeginCamera();` |

### player control (24)

| Name | In catalog | Sum of per-map counts | Per-map counts (zeros omitted) | First example |
|---|---|---:|---|---|
| `ClearPFlags` | yes | 1885 | MAP1:82 MAP2A:78 MAP2B:72 MAP2C:109 MAP3A:74 MAP3B:66 MAP3C:72 MAP4A:70 MAP4B:78 MAP5:72 MAP6:111 MAP7:107 MAP8:76 MAP9:80 MAP10:78 MAP11A:72 MAP11B:70 MAP11C:72 MAP12A:82 MAP12B:78 MAP12C:72 MAPARENA:70 MAPISLAND:14 MAPTSAVO:70 HUNTMAP1:15 HUNTMAP2:15 HUNTMAP3:15 HUNTMAP4:15 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:23` `player.ClearPFlags(PLAYER_CAN_CROUCH);` |
| `SetPFlags` | yes | 1576 | MAP1:67 MAP2A:63 MAP2B:60 MAP2C:88 MAP3A:61 MAP3B:57 MAP3C:60 MAP4A:59 MAP4B:63 MAP5:60 MAP6:89 MAP7:87 MAP8:62 MAP9:69 MAP10:63 MAP11A:60 MAP11B:59 MAP11C:60 MAP12A:65 MAP12B:70 MAP12C:63 MAPARENA:61 MAPISLAND:13 MAPTSAVO:61 HUNTMAP1:14 HUNTMAP2:14 HUNTMAP3:14 HUNTMAP4:14 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:189` `::globals.player.SetPFlags(PLAYER_CAN_CROUCH);` |
| `IsCrouch` | yes | 295 | MAP1:11 MAP2A:10 MAP2B:10 MAP2C:10 MAP3A:10 MAP3B:10 MAP3C:10 MAP4A:10 MAP4B:10 MAP5:10 MAP6:10 MAP7:10 MAP8:10 MAP9:12 MAP10:10 MAP11A:10 MAP11B:10 MAP11C:10 MAP12A:10 MAP12B:14 MAP12C:11 MAPARENA:11 MAPISLAND:11 MAPTSAVO:11 HUNTMAP1:11 HUNTMAP2:11 HUNTMAP3:11 HUNTMAP4:11 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:22` `if(!player.IsCrouch() && player.IsLegHurt())` |
| `IsLegHurt` | yes | 252 | MAP1:9 MAP2A:9 MAP2B:9 MAP2C:9 MAP3A:9 MAP3B:9 MAP3C:9 MAP4A:9 MAP4B:9 MAP5:9 MAP6:9 MAP7:9 MAP8:9 MAP9:9 MAP10:9 MAP11A:9 MAP11B:9 MAP11C:9 MAP12A:9 MAP12B:9 MAP12C:9 MAPARENA:9 MAPISLAND:9 MAPTSAVO:9 HUNTMAP1:9 HUNTMAP2:9 HUNTMAP3:9 HUNTMAP4:9 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:22` `if(!player.IsCrouch() && player.IsLegHurt())` |
| `HurtHuman` | yes | 65 | MAP1:3 MAP2A:3 MAP2B:3 MAP2C:3 MAP3A:3 MAP3B:3 MAP3C:3 MAP4A:3 MAP4B:3 MAP5:3 MAP6:3 MAP7:3 MAP8:3 MAP9:3 MAP10:3 MAP11A:3 MAP11B:3 MAP11C:3 MAP12A:3 MAP12B:3 MAP12C:3 MAPARENA:1 MAPTSAVO:1 | `re/extracted/SCRIPTS/MODELS/ANIMALS/SNAKE/SNAKE.SCR:46` `pSnake.HurtHuman(0.0);` |
| `EquipAIWith` | yes | 64 | MAP1:1 MAP2A:3 MAP2B:3 MAP2C:4 MAP3A:2 MAP3B:3 MAP3C:3 MAP4A:2 MAP4B:3 MAP5:3 MAP6:2 MAP7:1 MAP8:1 MAP9:1 MAP10:3 MAP11A:1 MAP11B:3 MAP11C:1 MAP12A:4 MAP12C:1 MAPARENA:3 MAPISLAND:1 MAPTSAVO:3 HUNTMAP1:3 HUNTMAP2:3 HUNTMAP3:3 HUNTMAP4:3 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:148` `EquipAIWith("hugsm1", "shotgun1,map1");` |
| `SetChanceToGetDamaged` | yes | 31 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:60` `SetChanceToGetDamaged(30.0);//chance in percents to get a special damage(leg, eye, arm, ear)` |
| `SetDamageDuration` | yes | 31 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:57` `SetDamageDuration(0.3);//duration for damage screen in seconds` |
| `SetEyeHurtColor` | yes | 31 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:42` `SetEyeHurtColor(218, 51, 0);//color in r,g,b components` |
| `SetEyeHurtIntensity` | yes | 31 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:43` `SetEyeHurtIntensity(30);//max alpha for fadein/out PC differs from PS2 - much higher level of alpha` |
| `SetHealthDamageFromPoison` | yes | 31 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:38` `SetHealthDamageFromPoison(0.04, 15.0);//Damage per second; max. second poison lasts` |
| `SetMinHurtForDamage` | yes | 31 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:61` `SetMinHurtForDamage(10.0);//min damage that triggers chance to get a special damage` |
| `SetDamageBloom` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:46` `SetDamageBloom(0.0, 0.0, 0.0, 0.0, 0.4, 0.0, 0.0, 0.0, 3.0);` |
| `SetDamageBloomFadeInOut` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:52` `SetDamageBloomFadeInOut(0.1, 0.4, 0.1);` |
| `SetDamageBlurParams` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:51` `SetDamageBlurParams(0.0, 120.0, 0.15, 6.0, 4.0);` |
| `SetHealth` | yes | 26 | MAP2A:1 MAP2B:12 MAP3C:3 MAP4A:6 MAP5:4 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:460` `::globals.wolf3.SetHealth(600);` |
| `DeactivateWeaponIn` | yes | 17 | MAP2A:2 MAP2B:4 MAP2C:3 MAP5:2 MAP6:2 MAP10:2 MAP11B:1 MAP12A:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:56` `::globals.hugs2a.DeactivateWeaponIn(-1);` |
| `ActivateWeaponIn` | yes | 13 | MAP1:2 MAP2A:1 MAP2B:1 MAP2C:3 MAP4B:1 MAP5:2 MAP6:1 MAP10:1 MAP12A:1 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:27` `hugsm1.ActivateWeaponIn(0,-1);` |
| `AiCanFindItem` | yes | 13 | MAP1:2 MAP2A:1 MAP2B:1 MAP2C:1 MAP3C:1 MAP5:1 MAP9:2 MAP12B:3 MAP12C:1 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:149` `AiCanFindItem("hunter", "handgun5_1st");` |
| `GetHealth` | yes | 13 | MAP2B:6 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:95` `fWolfHealth = ::globals.wolf1.GetHealth();` |
| `TakeItem` | yes | 8 | MAP2A:1 MAP2B:3 MAP2C:2 MAP5:2 | `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:112` `player.TakeItem("rifle1_1st");` |
| `SetMaxKnifeDistance` | yes | 7 | MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP10:1 MAPARENA:1 MAPTSAVO:1 | `re/extracted/SCRIPTS/LEVELS/MAP3A/WORLD.SCR:88` `player.SetMaxKnifeDistance(2.0);` |
| `AddItemAmmo` | yes | 4 | MAP2A:1 MAP11B:2 MAP12A:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:23` `::globals.player.AddItemAmmo("ammo_handgun5_1st", -5);` |
| `Hurt` | yes | 1 | MAPISLAND:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMAPISLAND.SCR:63` `::globals.player.Hurt(-1, 2, 0);` |

### AI tasks and animals (10)

| Name | In catalog | Sum of per-map counts | Per-map counts (zeros omitted) | First example |
|---|---|---:|---|---|
| `SetTaskParam` | yes | 860 | MAP1:21 MAP2A:87 MAP2B:103 MAP2C:15 MAP3A:32 MAP3B:8 MAP3C:18 MAP4A:16 MAP4B:63 MAP5:43 MAP6:47 MAP7:41 MAP8:70 MAP9:34 MAP10:65 MAP11A:7 MAP11B:29 MAP11C:12 MAP12A:48 MAP12B:5 MAP12C:13 MAPARENA:11 MAPISLAND:2 MAPTSAVO:2 HUNTMAP1:17 HUNTMAP2:17 HUNTMAP3:17 HUNTMAP4:17 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:24` `::globals.hugsm1.SetTaskParam("target", "start1");` |
| `StartTask` | yes | 689 | MAP1:28 MAP2A:61 MAP2B:58 MAP2C:20 MAP3A:35 MAP3B:10 MAP3C:15 MAP4A:13 MAP4B:48 MAP5:21 MAP6:33 MAP7:38 MAP8:45 MAP9:29 MAP10:55 MAP11A:11 MAP11B:25 MAP11C:17 MAP12A:42 MAP12B:3 MAP12C:4 MAPARENA:12 MAPISLAND:3 MAPTSAVO:3 HUNTMAP1:15 HUNTMAP2:15 HUNTMAP3:15 HUNTMAP4:15 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:23` `::globals.hugsm1.StartTask("AITaskNPC_MoveToPos");` |
| `MoveToPos` | yes | 554 | MAP1:24 MAP2A:25 MAP2B:50 MAP2C:25 MAP3A:27 MAP3B:8 MAP3C:9 MAP4A:10 MAP4B:50 MAP5:23 MAP6:25 MAP7:23 MAP8:23 MAP9:21 MAP10:64 MAP11A:10 MAP11B:15 MAP11C:15 MAP12A:62 MAP12B:7 MAP12C:3 MAPARENA:14 MAPISLAND:4 MAPTSAVO:5 HUNTMAP1:3 HUNTMAP2:3 HUNTMAP3:3 HUNTMAP4:3 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:21` `::globals.faketrap1.MoveToPos("capcana1");` |
| `SetBrain` | yes | 324 | MAP3A:35 MAP3B:31 MAP3C:35 MAP4A:31 MAP7:31 MAP8:31 MAP11A:31 MAP11B:31 MAP11C:31 MAP12A:37 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:89` `SetBrain("brain_npc");` |
| `StopTask` | yes | 224 | MAP1:16 MAP2A:37 MAP2B:19 MAP2C:5 MAP3A:4 MAP3C:7 MAP4B:17 MAP5:4 MAP6:10 MAP7:10 MAP8:13 MAP9:9 MAP10:15 MAP11B:3 MAP12A:11 MAP12B:1 MAP12C:3 HUNTMAP1:10 HUNTMAP2:10 HUNTMAP3:10 HUNTMAP4:10 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:98` `::globals.hugsm1.StopTask("AITaskNPC_MoveToPos");` |
| `AddAnimal` | yes | 170 | MAP1:6 MAP2A:6 MAP2B:8 MAP2C:4 MAP3A:11 MAP3B:5 MAP3C:7 MAP4A:6 MAP4B:15 MAP5:4 MAP6:5 MAP7:6 MAP8:11 MAP9:3 MAP10:10 MAP11A:6 MAP11B:7 MAP11C:4 MAP12A:9 MAP12B:1 MAP12C:1 MAPARENA:11 MAPISLAND:2 MAPTSAVO:2 HUNTMAP1:5 HUNTMAP2:5 HUNTMAP3:5 HUNTMAP4:5 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:163` `AddAnimal(ANIMAL_BBEAR, TRUE, "bear");` |
| `ForceBrainActivate` | yes | 146 | MAP1:9 MAP2A:8 MAP2B:15 MAP2C:4 MAP3A:6 MAP3C:11 MAP4B:24 MAP5:10 MAP6:9 MAP7:4 MAP8:6 MAP9:3 MAP10:13 MAP11B:3 MAP12A:7 MAP12B:3 MAP12C:3 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:276` `::globals.mlion.ForceBrainActivate(0);` |
| `SetTaskAnimalToShoot` | yes | 83 | MAP1:4 MAP2A:1 MAP2B:4 MAP2C:3 MAP3A:5 MAP3B:1 MAP3C:2 MAP4A:1 MAP4B:3 MAP5:1 MAP6:6 MAP7:6 MAP8:7 MAP9:5 MAP10:9 MAP11A:3 MAP11B:5 MAP11C:4 MAP12A:7 MAP12B:1 MAP12C:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:39` `m_pMission.SetTaskAnimalToShoot(TARGET_CAN, ANIMAL_KILL,4);` |
| `StopAllTask` | yes | 62 | MAP1:5 MAP2A:2 MAP2B:5 MAP2C:5 MAP3A:2 MAP3C:2 MAP4B:6 MAP5:3 MAP6:5 MAP7:8 MAP9:3 MAP10:6 MAP11B:2 MAP12A:7 MAP12C:1 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:72` `hugsm1.StopAllTask();` |
| `AddAnimPakForAI` | yes | 14 | MAP1:2 MAP2B:1 MAP2C:1 MAP4B:2 MAP6:1 MAP7:1 MAP10:4 MAP12A:2 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:175` `AddAnimPakForAI("bear", "map1.ap");` |

### NPC/companion (4)

| Name | In catalog | Sum of per-map counts | Per-map counts (zeros omitted) | First example |
|---|---|---:|---|---|
| `SetFollowDistance` | yes | 324 | MAP3A:35 MAP3B:31 MAP3C:35 MAP4A:31 MAP7:31 MAP8:31 MAP11A:31 MAP11B:31 MAP11C:31 MAP12A:37 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:114` `brain.SetFollowDistance(15.0);` |
| `SetNPCName` | yes | 312 | MAP3A:34 MAP3B:30 MAP3C:34 MAP4A:30 MAP7:30 MAP8:30 MAP11A:30 MAP11B:30 MAP11C:30 MAP12A:34 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:140` `SetNPCName("Hugh Andrews");` |
| `SetCanBeAttacked` | yes | 26 | MAP1:1 MAP2B:4 MAP3A:1 MAP3C:1 MAP4B:1 MAP5:1 MAP6:2 MAP7:2 MAP8:6 MAP9:1 MAP10:2 MAP12A:4 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:35` `::globals.hugsm1.SetCanBeAttacked(0);` |
| `SetPhysNPC` | yes | 3 | MAP2B:1 MAP12A:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:304` `::globals.hugs2b.SetPhysNPC(0);` |

### spawn/destroy/show/hide (5)

| Name | In catalog | Sum of per-map counts | Per-map counts (zeros omitted) | First example |
|---|---|---:|---|---|
| `CanBeRespawned` | yes | 66 | MAP2A:1 MAP2B:6 MAP2C:1 MAP5:3 MAP8:8 MAPARENA:30 MAPISLAND:6 MAPTSAVO:7 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:397` `if (TRUE==::globals.wolf3.CanBeRespawned())` |
| `Destroy` | yes | 57 | MAP1:3 MAP2A:2 MAP2B:2 MAP2C:2 MAP3A:2 MAP3B:2 MAP3C:2 MAP4A:2 MAP4B:2 MAP5:2 MAP6:2 MAP7:2 MAP8:2 MAP9:3 MAP10:2 MAP11A:2 MAP11B:2 MAP11C:2 MAP12A:2 MAP12B:3 MAP12C:3 MAPARENA:3 MAPISLAND:1 MAPTSAVO:3 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/ANIMALS/SNAKE/SNAKE.SCR:99` `pSnake.Destroy();` |
| `AddEvent` | yes | 47 | MAP1:2 MAP2A:3 MAP2B:3 MAP2C:2 MAP3A:4 MAP3B:4 MAP3C:4 MAP4B:3 MAP5:1 MAP6:4 MAP7:6 MAP8:1 MAP9:4 MAP10:2 MAP11B:1 MAP12A:2 MAP12B:1 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:142` `AddEvent(EVENT_ANIMAL, ANIMAL_MOUNTAINLION, "animal");` |
| `MakeVisible` | yes | 12 | MAP11A:6 MAP11C:6 | `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE2.SCR:29` `helicopter.MakeVisible(0);` |
| `AddAIToBeProtected` | yes | 11 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP5:1 MAP6:1 MAP7:1 MAP9:1 MAP10:1 MAP11B:1 MAP12A:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:288` `AddAIToBeProtected("hugsm1");` |

### triggers/activation (8)

| Name | In catalog | Sum of per-map counts | Per-map counts (zeros omitted) | First example |
|---|---|---:|---|---|
| `ActivateAfter` | yes | 289 | MAP1:12 MAP2A:14 MAP2B:19 MAP2C:7 MAP3A:16 MAP3B:7 MAP3C:5 MAP4A:10 MAP4B:20 MAP5:20 MAP6:12 MAP7:19 MAP8:29 MAP9:16 MAP10:22 MAP11A:9 MAP11B:11 MAP11C:10 MAP12A:15 MAP12B:2 MAP12C:2 MAPARENA:1 MAPISLAND:2 MAPTSAVO:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:8` `ActivateAfter(0);` |
| `ActivateBy` | yes | 289 | MAP1:12 MAP2A:14 MAP2B:19 MAP2C:7 MAP3A:16 MAP3B:7 MAP3C:5 MAP4A:10 MAP4B:20 MAP5:20 MAP6:12 MAP7:19 MAP8:29 MAP9:16 MAP10:22 MAP11A:9 MAP11B:11 MAP11C:10 MAP12A:15 MAP12B:2 MAP12C:2 MAPARENA:1 MAPISLAND:2 MAPTSAVO:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:6` `ActivateBy("hunter");` |
| `ActivateTriggerOnEnd` | yes | 30 | MAP1:3 MAP2A:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:2 MAP4B:2 MAP7:2 MAP10:1 MAP11A:3 MAP11B:1 MAP11C:4 MAP12A:2 MAP12B:1 MAP12C:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:41` `m_pMission.ActivateTriggerOnEnd("cScrTask05");` |
| `Deactivate` | yes | 23 | MAP2A:2 MAP2B:1 MAP2C:1 MAP3A:6 MAP3C:1 MAP4B:12 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:214` `::globals.trigger_14.Deactivate();` |
| `ActivateTriggerLevel` | yes | 8 | MAP2A:1 MAP2B:1 MAP3B:1 MAP4A:1 MAP11A:1 MAP11B:1 MAP12A:1 MAP12B:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:380` `ActivateTriggerLevel("exit_1");` |
| `ActivateTriggerOnFail` | yes | 5 | MAP1:1 MAP11A:4 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:53` `m_pMission.ActivateTriggerOnFail("cScrTask10");` |
| `ForceActivateTriggerOnEnd` | yes | 3 | MAP2B:2 MAP2C:1 | `re/extracted/SCRIPTS/MISSIONS/MISSION02B.SCR:35` `m_pMission.ForceActivateTriggerOnEnd("cScrTask04");` |
| `LatentActivateTriggerLevel` | yes | 2 | MAP3A:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:463` `LatentActivateTriggerLevel("exit_1");` |

### sound/music (9)

| Name | In catalog | Sum of per-map counts | Per-map counts (zeros omitted) | First example |
|---|---|---:|---|---|
| `SetSoundShader` | yes | 5020 | MAP3A:502 MAP3B:502 MAP3C:502 MAP4A:502 MAP7:502 MAP8:502 MAP11A:502 MAP11B:502 MAP11C:502 MAP12A:502 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:56` `SetSoundShader(SOUND_LOST1_ID, "GMRD05");` |
| `AddPlayerStepSoundForMaterial` | yes | 124 | MAP1:5 MAP2A:6 MAP2B:6 MAP2C:6 MAP3A:6 MAP3B:6 MAP3C:6 MAP4B:6 MAP5:4 MAP6:5 MAP7:5 MAP8:5 MAP9:5 MAP10:5 MAP11B:3 MAP12A:5 MAP12B:3 MAP12C:5 MAPARENA:2 MAPISLAND:5 MAPTSAVO:5 HUNTMAP1:5 HUNTMAP2:5 HUNTMAP3:5 HUNTMAP4:5 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:134` `AddPlayerStepSoundForMaterial(MATERIAL_GRASS);` |
| `PlaySound` | yes | 118 | MAP1:12 MAP2A:12 MAP2B:4 MAP2C:9 MAP3A:3 MAP3B:1 MAP3C:4 MAP4A:6 MAP4B:1 MAP5:6 MAP6:11 MAP7:5 MAP8:11 MAP9:6 MAP10:9 MAP11A:1 MAP11B:3 MAP11C:1 MAP12A:9 MAP12B:1 MAP12C:1 MAPARENA:1 MAPTSAVO:1 | `re/extracted/SCRIPTS/MODELS/WORLD/BIRDS/BIRDS.SCR:17` `pBirds.PlaySound();` |
| `PlaySoundEffect` | yes | 112 | MAP1:8 MAP2A:6 MAP2B:9 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:2 MAP4A:3 MAP4B:9 MAP5:4 MAP6:4 MAP7:11 MAP8:14 MAP9:14 MAP10:6 MAP11A:5 MAP11B:5 MAP11C:3 MAP12A:6 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:56` `PlaySoundEffect("M1OH02", "hugsm1");` |
| `SetSoundToBePlayed` | yes | 75 | MAP1:2 MAP2A:1 MAP2B:4 MAP2C:2 MAP3A:5 MAP3B:1 MAP3C:2 MAP4A:1 MAP4B:2 MAP5:2 MAP6:5 MAP7:5 MAP8:8 MAP9:5 MAP10:9 MAP11A:3 MAP11B:5 MAP11C:4 MAP12A:7 MAP12B:1 MAP12C:1 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:61` `m_pMission.SetSoundToBePlayed("attack2");` |
| `SetBreathSoundFadeOutSpeedOnIdle` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:70` `SetBreathSoundFadeOutSpeedOnIdle(5.0);//1.0 is one second, greater the fade out time raises - for IDLE` |
| `SetBreathSoundFadeOutSpeedOnRunning` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:72` `SetBreathSoundFadeOutSpeedOnRunning(15.0);//1.0 is one second, greater the fade out time raises - for RUNNING` |
| `SetBreathSoundFadeOutSpeedOnWalk` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:71` `SetBreathSoundFadeOutSpeedOnWalk(10.0);//1.0 is one second, greater the fade out time raises - for WALKING` |
| `PlayByteSound` | yes | 23 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPTSAVO:1 | `re/extracted/SCRIPTS/MODELS/ANIMALS/SNAKE/SNAKE.SCR:40` `pSnake.PlayByteSound();` |

### timing/latent (6)

| Name | In catalog | Sum of per-map counts | Per-map counts (zeros omitted) | First example |
|---|---|---:|---|---|
| `WaitForAnim` | yes | 981 | MAP1:40 MAP2A:40 MAP2B:40 MAP2C:57 MAP3A:40 MAP3B:40 MAP3C:40 MAP4A:40 MAP4B:40 MAP5:40 MAP6:57 MAP7:57 MAP8:40 MAP9:40 MAP10:40 MAP11A:40 MAP11B:40 MAP11C:40 MAP12A:40 MAP12B:40 MAP12C:40 MAPARENA:40 MAPISLAND:2 MAPTSAVO:40 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:713` `WaitForAnim(1);` |
| `Wait` | yes | 440 | MAP1:10 MAP2A:11 MAP2B:10 MAP2C:8 MAP3A:8 MAP3C:9 MAP4A:7 MAP4B:2 MAP5:16 MAP6:7 MAP7:10 MAP8:14 MAP9:7 MAP10:7 MAP11A:6 MAP11B:3 MAP11C:4 MAP12A:12 MAP12B:1 HUNTMAP1:68 HUNTMAP2:76 HUNTMAP3:68 HUNTMAP4:76 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:92` `Wait(1);` |
| `WaitForAnimFix` | no | 398 | MAP1:18 MAP2A:13 MAP2B:13 MAP2C:13 MAP3A:13 MAP3B:13 MAP3C:13 MAP4A:13 MAP4B:13 MAP5:13 MAP6:13 MAP7:13 MAP8:13 MAP9:20 MAP10:13 MAP11A:17 MAP11B:13 MAP11C:17 MAP12A:13 MAP12B:21 MAP12C:23 MAPARENA:16 MAPISLAND:11 MAPTSAVO:16 HUNTMAP1:11 HUNTMAP2:11 HUNTMAP3:11 HUNTMAP4:11 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:340` `WaitForAnimFix(990);` |
| `WaitForTask` | yes | 169 | MAP1:12 MAP2A:31 MAP2B:10 MAP2C:4 MAP3A:2 MAP3C:2 MAP4B:9 MAP5:2 MAP6:8 MAP7:9 MAP8:13 MAP9:8 MAP10:10 MAP11B:2 MAP12A:6 MAP12C:1 HUNTMAP1:10 HUNTMAP2:10 HUNTMAP3:10 HUNTMAP4:10 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:97` `WaitForTask(::globals.hugsm1.GetScrObjIdx(), "AITaskNPC_MoveToPos");` |
| `Pause` | no | 21 | MAPARENA:1 MAPISLAND:2 MAPTSAVO:2 HUNTMAP1:4 HUNTMAP2:4 HUNTMAP3:4 HUNTMAP4:4 | `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:695` `case 2: {::globals.player.PlayHint("PERFECT!"); UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);Pause(1.0);EndArena(); break;}` |
| `WaitTask` | yes | 19 | MAP1:1 MAP2B:2 MAP3A:1 MAP6:2 MAP8:2 MAP9:1 MAP11A:3 MAP11B:1 MAP11C:4 MAP12B:1 MAP12C:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:333` `WaitTask(5);` |

### other (133)

| Name | In catalog | Sum of per-map counts | Per-map counts (zeros omitted) | First example |
|---|---|---:|---|---|
| `setstate` | no | 4972 | MAP1:223 MAP2A:157 MAP2B:172 MAP2C:198 MAP3A:151 MAP3B:139 MAP3C:151 MAP4A:147 MAP4B:162 MAP5:150 MAP6:200 MAP7:202 MAP8:156 MAP9:284 MAP10:157 MAP11A:155 MAP11B:145 MAP11C:155 MAP12A:174 MAP12B:390 MAP12C:220 MAPARENA:192 MAPISLAND:127 MAPTSAVO:192 MAPMENU:1 HUNTMAP1:143 HUNTMAP2:143 HUNTMAP3:143 HUNTMAP4:143 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:197` `setstate(checkforenter);` |
| `TestBodyCmd` | no | 3454 | MAP1:140 MAP2A:105 MAP2B:105 MAP2C:132 MAP3A:105 MAP3B:105 MAP3C:105 MAP4A:105 MAP4B:105 MAP5:105 MAP6:132 MAP7:132 MAP8:105 MAP9:188 MAP10:105 MAP11A:105 MAP11B:105 MAP11C:105 MAP12A:105 MAP12B:268 MAP12C:147 MAPARENA:140 MAPISLAND:113 MAPTSAVO:140 HUNTMAP1:113 HUNTMAP2:113 HUNTMAP3:113 HUNTMAP4:113 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:20` `if(TestBodyCmd(BODYCMD_PLAYER_CROUCH) )` |
| `SetAnim` | yes | 2876 | MAP1:118 MAP2A:102 MAP2B:102 MAP2C:129 MAP3A:102 MAP3B:98 MAP3C:103 MAP4A:99 MAP4B:103 MAP5:101 MAP6:131 MAP7:131 MAP8:106 MAP9:131 MAP10:104 MAP11A:130 MAP11B:100 MAP11C:128 MAP12A:105 MAP12B:145 MAP12C:131 MAPARENA:108 MAPISLAND:52 MAPTSAVO:108 MAPMENU:1 HUNTMAP1:52 HUNTMAP2:52 HUNTMAP3:52 HUNTMAP4:52 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:16` `SetAnim("idle");` |
| `SetAttribute` | yes | 2296 | MAP2A:2 MAP2B:6 MAP3A:245 MAP3B:217 MAP3C:247 MAP4A:217 MAP6:2 MAP7:217 MAP8:217 MAP9:4 MAP11A:217 MAP11B:217 MAP11C:217 MAP12A:271 | `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:22` `hugs2a.SetAttribute(ATTRIBUTE_HEALTH_CURRENT, 10000);` |
| `AddStep` | yes | 1678 | MAP1:26 MAP2A:26 MAP2B:26 MAP2C:26 MAP3A:212 MAP3B:72 MAP3C:212 MAP4A:72 MAP4B:26 MAP5:26 MAP6:26 MAP7:72 MAP8:72 MAP9:26 MAP10:26 MAP11A:72 MAP11B:72 MAP11C:72 MAP12A:282 MAP12B:26 MAP12C:26 MAPARENA:26 MAPISLAND:26 MAPTSAVO:26 HUNTMAP1:26 HUNTMAP2:26 HUNTMAP3:26 HUNTMAP4:26 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:8` `AddStep("crouchwalk/backward", 457, 914, 0, 0);` |
| `setidx` | no | 1610 | MAP1:52 MAP2A:69 MAP2B:73 MAP2C:51 MAP3A:88 MAP3B:55 MAP3C:78 MAP4A:64 MAP4B:65 MAP5:50 MAP6:63 MAP7:83 MAP8:96 MAP9:53 MAP10:100 MAP11A:79 MAP11B:60 MAP11C:78 MAP12A:105 MAP12B:45 MAP12C:30 MAPARENA:33 MAPISLAND:20 MAPTSAVO:23 MAPMENU:1 HUNTMAP1:24 HUNTMAP2:24 HUNTMAP3:24 HUNTMAP4:24 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:65` `setidx(obj, FindEntScrObject(name));` |
| `trace` | no | 1340 | MAP1:35 MAP2A:26 MAP2B:26 MAP2C:27 MAP3A:57 MAP3B:49 MAP3C:56 MAP4A:52 MAP4B:26 MAP5:19 MAP6:25 MAP7:59 MAP8:54 MAP9:52 MAP10:24 MAP11A:62 MAP11B:51 MAP11C:62 MAP12A:65 MAP12B:67 MAP12C:40 MAPARENA:25 MAPISLAND:18 MAPTSAVO:23 HUNTMAP1:86 HUNTMAP2:84 HUNTMAP3:86 HUNTMAP4:84 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:15` `trace("Enabled stealth control\n");` |
| `FindEntScrObject` | yes | 1081 | MAP1:44 MAP2A:63 MAP2B:67 MAP2C:45 MAP3A:47 MAP3B:18 MAP3C:37 MAP4A:27 MAP4B:59 MAP5:44 MAP6:56 MAP7:45 MAP8:59 MAP9:37 MAP10:94 MAP11A:41 MAP11B:23 MAP11C:40 MAP12A:62 MAP12B:22 MAP12C:20 MAPARENA:26 MAPISLAND:16 MAPTSAVO:16 MAPMENU:1 HUNTMAP1:18 HUNTMAP2:18 HUNTMAP3:18 HUNTMAP4:18 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:65` `setidx(obj, FindEntScrObject(name));` |
| `isvalid` | no | 699 | MAP1:15 MAP2A:11 MAP2B:11 MAP2C:13 MAP3A:47 MAP3B:43 MAP3C:47 MAP4A:43 MAP4B:11 MAP5:9 MAP6:13 MAP7:44 MAP8:51 MAP9:26 MAP10:12 MAP11A:43 MAP11B:42 MAP11C:43 MAP12A:48 MAP12B:36 MAP12C:18 MAPARENA:14 MAPISLAND:8 MAPTSAVO:14 MAPMENU:1 HUNTMAP1:9 HUNTMAP2:9 HUNTMAP3:9 HUNTMAP4:9 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:66` `if(!isvalid(obj))` |
| `Phys_SetMass` | yes | 355 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:36 MAP3B:32 MAP3C:36 MAP4A:33 MAP4B:1 MAP5:1 MAP6:1 MAP7:32 MAP8:32 MAP9:1 MAP10:1 MAP11A:33 MAP11B:32 MAP11C:33 MAP12A:38 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:6` `Phys_SetMass(80.0);` |
| `SetHumanSize` | yes | 355 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:36 MAP3B:32 MAP3C:36 MAP4A:33 MAP4B:1 MAP5:1 MAP6:1 MAP7:32 MAP8:32 MAP9:1 MAP10:1 MAP11A:33 MAP11B:32 MAP11C:33 MAP12A:38 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:5` `SetHumanSize(1.80, 1.4, 1.25);` |
| `AddLensFlare` | yes | 345 | MAP1:15 MAP3A:15 MAP3B:15 MAP3C:15 MAP4A:15 MAP4B:15 MAP5:15 MAP8:15 MAP9:15 MAP10:15 MAP11A:15 MAP11B:15 MAP11C:15 MAP12A:15 MAP12B:15 MAP12C:15 MAPARENA:15 MAPISLAND:15 MAPTSAVO:15 HUNTMAP1:15 HUNTMAP2:15 HUNTMAP3:15 HUNTMAP4:15 | `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:20` `AddLensFlare(0.0);` |
| `SetColor` | yes | 345 | MAP1:15 MAP3A:15 MAP3B:15 MAP3C:15 MAP4A:15 MAP4B:15 MAP5:15 MAP8:15 MAP9:15 MAP10:15 MAP11A:15 MAP11B:15 MAP11C:15 MAP12A:15 MAP12B:15 MAP12C:15 MAPARENA:15 MAPISLAND:15 MAPTSAVO:15 HUNTMAP1:15 HUNTMAP2:15 HUNTMAP3:15 HUNTMAP4:15 | `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:23` `SetColor(105, 105, 57);` |
| `SetFlags` | yes | 345 | MAP1:15 MAP3A:15 MAP3B:15 MAP3C:15 MAP4A:15 MAP4B:15 MAP5:15 MAP8:15 MAP9:15 MAP10:15 MAP11A:15 MAP11B:15 MAP11C:15 MAP12A:15 MAP12B:15 MAP12C:15 MAPARENA:15 MAPISLAND:15 MAPTSAVO:15 HUNTMAP1:15 HUNTMAP2:15 HUNTMAP3:15 HUNTMAP4:15 | `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:25` `SetFlags(FLARE_FADEBYPOWER \| FLARE_FADEBYOCCLUDER );` |
| `SetMap` | yes | 345 | MAP1:15 MAP3A:15 MAP3B:15 MAP3C:15 MAP4A:15 MAP4B:15 MAP5:15 MAP8:15 MAP9:15 MAP10:15 MAP11A:15 MAP11B:15 MAP11C:15 MAP12A:15 MAP12B:15 MAP12C:15 MAPARENA:15 MAPISLAND:15 MAPTSAVO:15 HUNTMAP1:15 HUNTMAP2:15 HUNTMAP3:15 HUNTMAP4:15 | `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:22` `SetMap(0, 0.5, 0.5, 1);` |
| `SetShader` | yes | 345 | MAP1:15 MAP3A:15 MAP3B:15 MAP3C:15 MAP4A:15 MAP4B:15 MAP5:15 MAP8:15 MAP9:15 MAP10:15 MAP11A:15 MAP11B:15 MAP11C:15 MAP12A:15 MAP12B:15 MAP12C:15 MAPARENA:15 MAPISLAND:15 MAPTSAVO:15 HUNTMAP1:15 HUNTMAP2:15 HUNTMAP3:15 HUNTMAP4:15 | `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:24` `SetShader("fx_lensflare");` |
| `SetSize` | yes | 345 | MAP1:15 MAP3A:15 MAP3B:15 MAP3C:15 MAP4A:15 MAP4B:15 MAP5:15 MAP8:15 MAP9:15 MAP10:15 MAP11A:15 MAP11B:15 MAP11C:15 MAP12A:15 MAP12B:15 MAP12C:15 MAPARENA:15 MAPISLAND:15 MAPTSAVO:15 HUNTMAP1:15 HUNTMAP2:15 HUNTMAP3:15 HUNTMAP4:15 | `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:21` `SetSize(500.0, 500.0);` |
| `Add_Senses` | yes | 324 | MAP3A:35 MAP3B:31 MAP3C:35 MAP4A:31 MAP7:31 MAP8:31 MAP11A:31 MAP11B:31 MAP11C:31 MAP12A:37 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:120` `brain.Add_Senses(1.0, 1.0, 1.0);` |
| `SetHelloDistance` | yes | 324 | MAP3A:35 MAP3B:31 MAP3C:35 MAP4A:31 MAP7:31 MAP8:31 MAP11A:31 MAP11B:31 MAP11C:31 MAP12A:37 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:113` `brain.SetHelloDistance(15.0);` |
| `SetLeftEyeRelAngles` | yes | 314 | MAP3A:34 MAP3B:30 MAP3C:34 MAP4A:30 MAP7:30 MAP8:30 MAP11A:30 MAP11B:30 MAP11C:30 MAP12A:36 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:189` `SetLeftEyeRelAngles(0.057940, -0.057938, 0.704728, -0.704730);` |
| `SetRightEyeRelAngles` | yes | 314 | MAP3A:34 MAP3B:30 MAP3C:34 MAP4A:30 MAP7:30 MAP8:30 MAP11A:30 MAP11B:30 MAP11C:30 MAP12A:36 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:187` `SetRightEyeRelAngles(0.057940, -0.057938, 0.704728, -0.704730);` |
| `SetIconCoords` | yes | 312 | MAP3A:34 MAP3B:30 MAP3C:34 MAP4A:30 MAP7:30 MAP8:30 MAP11A:30 MAP11B:30 MAP11C:30 MAP12A:34 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:139` `SetIconCoords(2.0, 98.0, 28.0, 124.0, 256.0);` |
| `SetName` | yes | 292 | MAP1:12 MAP2A:14 MAP2B:22 MAP2C:7 MAP3A:16 MAP3B:7 MAP3C:5 MAP4A:10 MAP4B:20 MAP5:20 MAP6:12 MAP7:19 MAP8:29 MAP9:16 MAP10:22 MAP11A:9 MAP11B:11 MAP11C:10 MAP12A:15 MAP12B:2 MAP12C:2 MAPARENA:1 MAPISLAND:2 MAPTSAVO:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:5` `SetName("cScrTask01");` |
| `GetScrObjIdx` | yes | 281 | MAP1:16 MAP2A:31 MAP2B:10 MAP2C:4 MAP3A:2 MAP3C:2 MAP4B:9 MAP5:2 MAP6:8 MAP7:9 MAP8:13 MAP9:20 MAP10:10 MAP11B:2 MAP12A:6 MAP12B:16 MAP12C:9 MAPARENA:4 MAPISLAND:4 MAPTSAVO:4 HUNTMAP1:25 HUNTMAP2:25 HUNTMAP3:25 HUNTMAP4:25 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:76` `if(m_pWeapon.Attach(m_pOwner.GetScrObjIdx(),"BipLHand"))` |
| `SetDotFactor` | yes | 253 | MAP1:11 MAP3A:11 MAP3B:11 MAP3C:11 MAP4A:11 MAP4B:11 MAP5:11 MAP8:11 MAP9:11 MAP10:11 MAP11A:11 MAP11B:11 MAP11C:11 MAP12A:11 MAP12B:11 MAP12C:11 MAPARENA:11 MAPISLAND:11 MAPTSAVO:11 HUNTMAP1:11 HUNTMAP2:11 HUNTMAP3:11 HUNTMAP4:11 | `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:42` `SetDotFactor(2.0); */` |
| `SetLeftEyeRelPos` | yes | 134 | MAP3A:14 MAP3B:13 MAP3C:14 MAP4A:13 MAP7:13 MAP8:13 MAP11A:13 MAP11B:13 MAP11C:13 MAP12A:15 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:188` `SetLeftEyeRelPos(0.105033, 0.028231, 0.056899);` |
| `SetRightEyeRelPos` | yes | 134 | MAP3A:14 MAP3B:13 MAP3C:14 MAP4A:13 MAP7:13 MAP8:13 MAP11A:13 MAP11B:13 MAP11C:13 MAP12A:15 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:186` `SetRightEyeRelPos(0.105033, -0.030247, 0.056899);` |
| `SetDiffProps` | yes | 116 | MAP1:4 MAP2A:4 MAP2B:4 MAP2C:4 MAP3A:4 MAP3B:4 MAP3C:4 MAP4A:8 MAP4B:4 MAP5:4 MAP6:4 MAP7:4 MAP8:4 MAP9:4 MAP10:4 MAP11A:4 MAP11B:4 MAP11C:4 MAP12A:4 MAP12B:4 MAP12C:4 MAPARENA:4 MAPISLAND:4 MAPTSAVO:4 HUNTMAP1:4 HUNTMAP2:4 HUNTMAP3:4 HUNTMAP4:4 | `re/extracted/SCRIPTS/LEVELS/MAPWORLDBASE.SCR:5` `SetDiffProps(DMG_RECEIVED_ANIMAL, 12, 10, 8); //easy, mediu, hard` |
| `DeactivateAIHuntAreaPoint` | yes | 113 | MAP2A:10 MAP2B:49 MAP2C:5 MAP4B:7 MAP5:3 MAP7:5 MAP8:26 MAP11B:3 MAP12A:5 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:13` `DeactivateAIHuntAreaPoint("escapemlion1");` |
| `SetAnimSmooth` | no | 112 | MAP1:4 MAP2A:4 MAP2B:4 MAP2C:4 MAP3A:4 MAP3B:4 MAP3C:4 MAP4A:4 MAP4B:4 MAP5:4 MAP6:4 MAP7:4 MAP8:4 MAP9:4 MAP10:4 MAP11A:4 MAP11B:4 MAP11C:4 MAP12A:4 MAP12B:4 MAP12C:4 MAPARENA:4 MAPISLAND:4 MAPTSAVO:4 HUNTMAP1:4 HUNTMAP2:4 HUNTMAP3:4 HUNTMAP4:4 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:228` `SetAnimSmooth(current_anim);` |
| `TestBodyCmdOr` | no | 109 | MAP1:6 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:13 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:23 MAP12C:8 MAPARENA:6 MAPISLAND:6 MAPTSAVO:6 HUNTMAP1:6 HUNTMAP2:6 HUNTMAP3:6 HUNTMAP4:6 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:691` `else if (TestBodyCmdOr(BODYCMD_PLAYER_ONGROUND \| BODYCMD_PLAYER_INWATER))` |
| `AddToCoverPointList` | yes | 97 | MAP3A:3 MAP5:2 MAP6:5 MAP8:9 MAP9:9 MAP10:69 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:30` `::globals.abby.AddToCoverPointList("avioncover");` |
| `SetAnimSpeedFactor` | no | 86 | MAP1:2 MAP2A:2 MAP2B:2 MAP2C:2 MAP3A:2 MAP3B:2 MAP3C:2 MAP4A:2 MAP4B:2 MAP5:2 MAP6:2 MAP7:2 MAP8:2 MAP9:2 MAP10:2 MAP11A:12 MAP11B:2 MAP11C:12 MAP12A:2 MAP12B:2 MAP12C:12 MAPARENA:2 MAPISLAND:2 MAPTSAVO:2 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:396` `SetAnimSpeedFactor(fAnimSpeedFactor);` |
| `GetAnimStatus` | no | 75 | MAP1:3 MAP9:12 MAP11A:3 MAP11C:3 MAP12B:21 MAP12C:12 MAPARENA:3 MAPISLAND:3 MAPTSAVO:3 HUNTMAP1:3 HUNTMAP2:3 HUNTMAP3:3 HUNTMAP4:3 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:365` `if(GetAnimStatus() > 0.9)` |
| `ConsoleRealExec` | yes | 71 | MAP1:2 MAP2A:2 MAP2B:2 MAP2C:2 MAP3A:2 MAP3B:2 MAP3C:2 MAP4A:2 MAP4B:2 MAP5:2 MAP6:2 MAP7:2 MAP8:2 MAP9:2 MAP10:2 MAP11A:2 MAP11B:2 MAP11C:2 MAP12A:2 MAP12B:2 MAP12C:2 MAPARENA:3 MAPISLAND:3 MAPTSAVO:3 HUNTMAP1:5 HUNTMAP2:5 HUNTMAP3:5 HUNTMAP4:5 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:122` `ConsoleRealExec("mod_player_ent = obj_hunter");` |
| `rand` | no | 69 | MAP2A:1 MAP2B:9 MAP2C:1 MAP5:5 MAP7:1 MAP8:3 MAP9:2 MAP12B:2 MAP12C:2 MAPARENA:2 MAPISLAND:2 MAPTSAVO:3 HUNTMAP1:9 HUNTMAP2:9 HUNTMAP3:9 HUNTMAP4:9 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:162` `if(rand()%2==1) // pentru random surprise` |
| `SetCurrentZoom` | yes | 65 | MAP1:3 MAP9:11 MAP12B:27 MAP12C:3 MAPARENA:3 MAPISLAND:3 MAPTSAVO:3 HUNTMAP1:3 HUNTMAP2:3 HUNTMAP3:3 HUNTMAP4:3 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:410` `m_pWeapon.SetCurrentZoom(m_pWeapon.GetZoomPowerMin());` |
| `GetRotYawSpeed` | yes | 56 | MAP1:2 MAP2A:2 MAP2B:2 MAP2C:2 MAP3A:2 MAP3B:2 MAP3C:2 MAP4A:2 MAP4B:2 MAP5:2 MAP6:2 MAP7:2 MAP8:2 MAP9:2 MAP10:2 MAP11A:2 MAP11B:2 MAP11C:2 MAP12A:2 MAP12B:2 MAP12C:2 MAPARENA:2 MAPISLAND:2 MAPTSAVO:2 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:395` `fAnimSpeedFactor = (-0.85/132)*player.GetRotYawSpeed()+1.0;` |
| `Kill` | yes | 56 | MAP2B:7 MAP2C:2 MAP4B:18 MAP5:3 MAP6:4 MAP10:10 MAP12A:12 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:222` `::globals.wolf1.Kill();` |
| `GetZoomPowerMin` | yes | 53 | MAP1:3 MAP9:8 MAP12B:18 MAP12C:3 MAPARENA:3 MAPISLAND:3 MAPTSAVO:3 HUNTMAP1:3 HUNTMAP2:3 HUNTMAP3:3 HUNTMAP4:3 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:410` `m_pWeapon.SetCurrentZoom(m_pWeapon.GetZoomPowerMin());` |
| `ActivateAIHuntAreaPoint` | yes | 47 | MAP2A:7 MAP2B:10 MAP2C:4 MAP4B:6 MAP5:1 MAP7:3 MAP8:10 MAP10:1 MAP11B:2 MAP12A:3 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:155` `ActivateAIHuntAreaPoint("escapemlion1");` |
| `SetBkpZoom` | yes | 47 | MAP1:3 MAP9:6 MAP12B:14 MAP12C:3 MAPARENA:3 MAPISLAND:3 MAPTSAVO:3 HUNTMAP1:3 HUNTMAP2:3 HUNTMAP3:3 HUNTMAP4:3 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:409` `m_pWeapon.SetBkpZoom();` |
| `Zoom` | yes | 45 | MAP1:3 MAP9:6 MAP12B:12 MAP12C:3 MAPARENA:3 MAPISLAND:3 MAPTSAVO:3 HUNTMAP1:3 HUNTMAP2:3 HUNTMAP3:3 HUNTMAP4:3 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:411` `m_pWeapon.Zoom();` |
| `ExitAim` | yes | 44 | MAP1:4 MAP9:4 MAP12B:4 MAP12C:4 MAPARENA:4 MAPISLAND:4 MAPTSAVO:4 HUNTMAP1:4 HUNTMAP2:4 HUNTMAP3:4 HUNTMAP4:4 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:189` `m_pWeapon.ExitAim();*/` |
| `SetCoverPointSearchDist` | yes | 41 | MAP3A:4 MAP3B:4 MAP3C:5 MAP4A:4 MAP7:4 MAP8:4 MAP11A:4 MAP11B:4 MAP11C:4 MAP12A:4 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:1956` `SetCoverPointSearchDist(15.0);` |
| `LantentKill` | yes | 40 | HUNTMAP1:10 HUNTMAP2:10 HUNTMAP3:10 HUNTMAP4:10 | `re/extracted/SCRIPTS/LEVELS/HUNTMAPBASE.SCR:328` `LantentKill();` |
| `RemoveFromCoverPointList` | yes | 38 | MAP3A:1 MAP5:1 MAP8:6 MAP9:5 MAP10:25 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:404` `::globals.abby.RemoveFromCoverPointList("all");` |
| `SetRecoverSpeed` | yes | 35 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:2 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:2 MAPISLAND:2 MAPTSAVO:2 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:39` `SetRecoverSpeed(0.04);//recover points per second` |
| `Attach` | yes | 34 | MAP1:2 MAP9:6 MAP12B:8 MAP12C:4 MAPARENA:2 MAPISLAND:2 MAPTSAVO:2 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:76` `if(m_pWeapon.Attach(m_pOwner.GetScrObjIdx(),"BipLHand"))` |
| `LinkPart` | yes | 34 | MAP1:2 MAP9:6 MAP12B:8 MAP12C:4 MAPARENA:2 MAPISLAND:2 MAPTSAVO:2 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:78` `if(!m_pWeapon.LinkPart(m_pOwner.GetScrObjIdx(),"BipLHand"))` |
| `SetTimeSelfRecoverDisabled` | yes | 34 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:2 MAPISLAND:2 MAPTSAVO:2 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:59` `SetTimeSelfRecoverDisabled(10.0);//time before self healing kicks in, in seconds` |
| `AddManager` | yes | 32 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:125` `AddManager(GAME_MANAGERS);` |
| `SetFadeOutSndValue` | yes | 31 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:62` `SetFadeOutSndValue(20.0);//value in percents for the level of sound after the initial ear damage (the player still has ear damage on)` |
| `SetMaxSpringTime` | yes | 31 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:67` `SetMaxSpringTime(30.0);//max time player can spring in seconds` |
| `SetMinStaminaForRunning` | yes | 31 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:66` `SetMinStaminaForRunning(0.2);//min level of stamina for running (between 0.0 ... 1.0)` |
| `SetPoisonColor` | yes | 31 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:40` `SetPoisonColor(71, 168, 0);//color in r,g,b components` |
| `SetPoisonIntensity` | yes | 31 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:41` `SetPoisonIntensity(20);//max alpha for fadein/out PC differs from PS2 - much higher level of alpha` |
| `SetStaminaRecoverSpeedOnIdle` | yes | 31 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:2 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:2 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:68` `SetStaminaRecoverSpeedOnIdle(20.0);//time (in seconds) elapsed before stamina is 100% recoverd when player is IDLE` |
| `SetCanBeHited` | yes | 30 | MAP3A:3 MAP3B:3 MAP3C:3 MAP4A:3 MAP7:3 MAP8:3 MAP11A:3 MAP11B:3 MAP11C:3 MAP12A:3 | `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDE.SCR:716` `SetCanBeHited(0);` |
| `SetTimeBeforeFade` | yes | 30 | MAP2B:26 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:45` `::globals.wolf3.SetTimeBeforeFade(1.5);` |
| `GetDistBetween2Ais` | yes | 28 | MAP1:2 MAP2C:2 MAP4B:2 MAP12A:2 HUNTMAP1:5 HUNTMAP2:5 HUNTMAP3:5 HUNTMAP4:5 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:193` `fDistance = GetDistBetween2Ais(::globals.szStealthCenter, "hunter");` |
| `IsInVehicle` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:648` `if(TRUE == player.IsInVehicle())` |
| `LoadEventMat` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:155` `LoadEventMat("Typemap/event_map1.mat");` |
| `LoadMat` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:154` `LoadMat("Typemap/map1.mat");` |
| `Random` | no | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:744` `/* fAct = Random();` |
| `SetAIType` | yes | 28 | MAP3A:4 MAP3C:3 MAP8:9 MAP9:3 MAP10:4 MAP12A:5 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:32` `::globals.abby.SetAIType(AITYPE_GUIDE\|AITYPE_COVERSTAY,""); //crouch_sau_lie` |
| `SetControllerInertia` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:81` `SetControllerInertia(0.3);` |
| `SetControllerInfluence` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:80` `SetControllerInfluence(3.7);` |
| `SetDeathBloom` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:45` `SetDeathBloom(0.0, 0.0, 0.0, 0.0, 0.4, 0.0, 0.0, 0.0, 3.0);` |
| `SetIceCrackTime` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:85` `SetIceCrackTime(75.0);` |
| `SetIceFallTime` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:86` `SetIceFallTime(40.0);` |
| `SetIceModelsOffsets` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:88` `SetIceModelsOffsets(-1.6, -2.7, 1.3, -1.12, 2.1, 2.47, -1.6, 0.9); //x1, z1, x2, z2, x3, z3, x4,z4 no sign required` |
| `SetIceSpeedFallForPlayer` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:87` `SetIceSpeedFallForPlayer(0.8);` |
| `SetIceWalkRunMultiplier` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:89` `SetIceWalkRunMultiplier(1.2);` |
| `SetSandFallSpeed` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:76` `SetSandFallSpeed(0.3);` |
| `SetSandGetoutSpeed` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:77` `SetSandGetoutSpeed(0.1);` |
| `SetSpeedFromAnim` | no | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:665` `SetSpeedFromAnim(current_anim);` |
| `SetStaminaRecoverSpeedOnWalk` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:69` `SetStaminaRecoverSpeedOnWalk(30.0);//time (in seconds) elapsed before stamina is 100% recoverd when player isn't IDLE` |
| `SetVisible` | yes | 28 | MAP1:2 MAP9:4 MAP12B:5 MAP12C:3 MAPARENA:2 MAPISLAND:2 MAPTSAVO:2 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:104` `m_pWeapon.SetVisible(true);` |
| `SetWaterCurrentInfluence` | yes | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMAN.SCR:79` `SetWaterCurrentInfluence(0.6);` |
| `cScrMapGlobals` | no | 28 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:111` `new cScrMapGlobals(globals)` |
| `GetCurrentZoom` | yes | 24 | MAP9:6 MAP12B:18 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/HGUN/HGUN.SCR:298` `fZoom = m_pWeapon.GetCurrentZoom() + 7.0*fDeltaTime * (0.5 + m_pWeapon.GetCurrentZoom() * (-0.4));` |
| `UnlinkWithPhysics` | yes | 23 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPTSAVO:1 | `re/extracted/SCRIPTS/MODELS/ANIMALS/SNAKE/SNAKE.SCR:95` `pSnake.UnlinkWithPhysics();` |
| `DropObject` | yes | 21 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 | `re/extracted/SCRIPTS/MODELS/WORLD/TRAP/TRAP.SCR:110` `pTrap.DropObject();` |
| `SecFire` | yes | 18 | MAP1:1 MAP9:3 MAP12B:5 MAP12C:2 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:377` `m_pWeapon.SecFire();` |
| `SetCamOffsetsInAim` | yes | 18 | MAP9:6 MAP12B:12 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/HGUN/HGUN.SCR:74` `m_pWeapon.SetCamOffsetsInAim(0.08, -0.05, 0.15);` |
| `Fire` | yes | 17 | MAP1:1 MAP9:3 MAP12B:3 MAP12C:3 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:326` `m_pWeapon.Fire();` |
| `GetScrObjIdxParent` | yes | 17 | MAP1:1 MAP9:3 MAP12B:4 MAP12C:2 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:67` `setidx(m_pOwner,m_pWeapon.GetScrObjIdxParent());` |
| `IsDead` | yes | 17 | MAP2B:9 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:89` `if (TRUE==(::globals.wolf1.IsDead()))` |
| `Reload` | yes | 17 | MAP1:1 MAP9:2 MAP12B:6 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:498` `m_pWeapon.Reload();` |
| `getidx` | no | 17 | MAP1:1 MAP9:3 MAP12B:4 MAP12C:2 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:73` `settarget(getidx(m_pWeapon));` |
| `settarget` | no | 17 | MAP1:1 MAP9:3 MAP12B:4 MAP12C:2 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:73` `settarget(getidx(m_pWeapon));` |
| `SetLightBloom` | yes | 15 | MAP1:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP8:1 MAP10:1 MAPARENA:1 MAPISLAND:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:128` `SetLightBloom(0.35, 0.35, 0.35, 1.0, 0.15, 0.15, 0.15, 1.0, 3);` |
| `SetShadowBloom` | yes | 15 | MAP1:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP5:1 MAP8:1 MAP10:1 MAPARENA:1 MAPISLAND:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:129` `SetShadowBloom(0.35, 0.35, 0.35, 1.0, 0.15, 0.15, 0.15, 1.0, 3);` |
| `BSMGoToState` | yes | 14 | MAP11A:7 MAP11C:7 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:17` `::globals.helicopter.BSMGoToState("Ani1");` |
| `SetCharacterModel` | yes | 14 | MAP1:1 MAP2A:1 MAP2B:1 MAP2C:1 MAP8:1 MAP9:1 MAP10:1 MAP11A:2 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 | `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:133` `SetCharacterModel(AUTUMN_CHARACTER_MODEL);` |
| `CustomProps` | yes | 13 | MAP2B:5 MAP4A:8 | `re/extracted/SCRIPTS/LEVELS/MAP2B/WORLD.SCR:234` `::globals.gbear.CustomProps("Make_Scared");` |
| `SetWaterFog` | yes | 13 | MAP2A:1 MAP2B:1 MAP2C:1 MAP3A:1 MAP3B:1 MAP3C:1 MAP4A:1 MAP4B:1 MAP6:1 MAP7:1 MAP8:1 MAP9:1 MAP12C:1 | `re/extracted/SCRIPTS/LEVELS/MAP2A/WORLD.SCR:106` `SetWaterFog(7,39,54,-10,10);` |
| `SetState` | yes | 12 | HUNTMAP1:3 HUNTMAP2:3 HUNTMAP3:3 HUNTMAP4:3 | `re/extracted/SCRIPTS/LEVELS/HUNTMAPBASE.SCR:313` `SetState(szLastState);` |
| `BeginAim` | yes | 11 | MAP1:1 MAP9:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:309` `m_pWeapon.BeginAim();` |
| `Detach` | yes | 11 | MAP1:1 MAP9:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:534` `m_pWeapon.Detach();` |
| `UnlinkPart` | yes | 11 | MAP1:1 MAP9:1 MAP12B:1 MAP12C:1 MAPARENA:1 MAPISLAND:1 MAPTSAVO:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/WEAPON.SCR:533` `m_pWeapon.UnlinkPart();` |
| `DisableStimulus` | yes | 9 | MAP1:2 MAP2C:2 MAP4B:2 MAP12A:3 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:189` `::globals.player.DisableStimulus();//disables player stimulus on animals` |
| `SetAnimStatus` | no | 9 | MAP11A:3 MAP11C:3 MAP12C:3 | `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:52` `SetAnimStatus(0.0);` |
| `SetCanBeClaimed` | yes | 9 | MAP2C:1 HUNTMAP1:2 HUNTMAP2:2 HUNTMAP3:2 HUNTMAP4:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:166` `::globals.wildboar1.SetCanBeClaimed(TRUE);` |
| `SetCompleteCondition` | yes | 9 | MAP1:2 MAP11A:3 MAP11C:4 | `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:40` `m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE);` |
| `SetStealthLevels` | yes | 9 | MAP1:1 MAP2C:1 MAP4B:2 MAP12A:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:198` `::globals.player.SetStealthLevels(0.2, 0.25, 0.30, 0.35);` |
| `CanZoom` | yes | 8 | MAP9:2 MAP12B:6 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/HGUN/HGUN.SCR:296` `if(!m_pWeapon.CanZoom())` |
| `GetZoomPowerMax` | yes | 8 | MAP9:2 MAP12B:6 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/HGUN/HGUN.SCR:300` `if(m_pWeapon.GetCurrentZoom() >= m_pWeapon.GetZoomPowerMax())` |
| `RestoreLight` | yes | 7 | MAP2B:7 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:939` `::globals.fire1.RestoreLight();` |
| `SetCountDownTime` | yes | 7 | MAP2B:7 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:418` `::globals.fire1.SetCountDownTime(400.0);` |
| `StartCountDown` | yes | 7 | MAP2B:7 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:425` `::globals.fire1.StartCountDown();` |
| `EnableBlocker` | yes | 6 | MAP1:2 MAP12A:4 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE5.SCR:119` `hugsm1.EnableBlocker();` |
| `GetInCar` | yes | 6 | MAP4A:2 MAP5:4 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:22` `::globals.tleader4a.GetInCar(1);` |
| `RemoveAIToBeProtected` | yes | 6 | MAP2A:1 MAP2B:1 MAP2C:1 MAP5:1 MAP7:1 MAP12A:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:131` `RemoveAIToBeProtected("hugs2a");` |
| `SetBreathFxVisible` | yes | 6 | MAP11A:1 MAP11B:1 MAP11C:1 MAP12A:1 MAP12B:1 MAP12C:1 | `re/extracted/SCRIPTS/LEVELS/MAP11A/WORLD.SCR:107` `SetBreathFxVisible(1);` |
| `AddIgnoredStimulus` | yes | 5 | MAP8:3 MAP12A:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:633` `::globals.tribesman1.AddIgnoredStimulus("cObjAnimal", 0);` |
| `SetMaxAttackerNo` | yes | 5 | MAP3C:1 MAP4A:1 MAP8:1 MAP11A:1 MAP11C:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:37` `SetMaxAttackerNo(2);` |
| `SetScrClaimCallback` | yes | 5 | MAP2C:1 HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:168` `::globals.wildboar1.SetScrClaimCallback(trigger_05IDx, "ClaimCallback");` |
| `GetClosestMatchingEntity` | yes | 4 | HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/LEVELS/HUNTMAPBASE.SCR:162` `szPredatorSpawnPoint = GetClosestMatchingEntity(szAnimalActorName, "predator", "hunt_area_point");` |
| `GetDistFromClosestStimulusTo` | yes | 4 | HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/LEVELS/HUNTMAPBASE.SCR:865` `bullet_dist = GetDistFromClosestStimulusTo(szActorName);` |
| `GetStealthLevel` | yes | 4 | HUNTMAP1:1 HUNTMAP2:1 HUNTMAP3:1 HUNTMAP4:1 | `re/extracted/SCRIPTS/LEVELS/HUNTMAPBASE.SCR:823` `fInstStealth = ::globals.player.GetStealthLevel();` |
| `GetZoomFromBkp` | yes | 4 | MAP9:1 MAP12B:3 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/HGUN/HGUN.SCR:268` `m_pWeapon.SetCurrentZoom(m_pWeapon.GetZoomFromBkp());` |
| `DriveTo` | yes | 3 | MAP4A:1 MAP5:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:25` `::globals.tleader4a.DriveTo("startcar1");` |
| `EnableStimulus` | yes | 3 | MAP1:1 MAP12A:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:234` `::globals.hugsm1.EnableStimulus();` |
| `MakeBoss` | yes | 3 | MAP11B:1 MAP12B:1 MAP12C:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:176` `::globals.hogzilla1.MakeBoss(TRUE, "HOGZILLA");` |
| `SetDenySATime` | yes | 3 | MAP2A:1 MAP8:2 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:19` `::globals.player.SetDenySATime(25.0);` |
| `IsTranq` | yes | 2 | MAP9:1 MAP12B:1 | `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/HGUN/HGUN.SCR:455` `m_pWeapon.IsTranq();` |
| `ActivateBulletWarning` | yes | 1 | MAP11A:1 | `re/extracted/SCRIPTS/LEVELS/MAP11A/WORLD.SCR:99` `ActivateBulletWarning(1, 5, "M11RD02");` |
| `Fly` | yes | 1 | MAP9:1 | `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:58` `bats1.Fly();` |
| `LoadRiverMat` | yes | 1 | MAP2A:1 | `re/extracted/SCRIPTS/LEVELS/MAP2A/WORLD.SCR:142` `LoadRiverMat("Typemap/map2a_rivermask.mat");` |
| `NextTrig` | yes | 1 | MAP4A:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:184` `NextTrig("cScrTask09");` |
| `SetDangerTime` | yes | 1 | MAP1:1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:383` `::globals.hugsm1.SetDangerTime(0.0);` |

A line in a shared script (for example `MAPWORLDBASE.SCR`) is counted once per map that includes it, so the sum is larger than the number of source lines.

Identifiers that are both script-defined and called are excluded above. Script-defined names collected: 94. None of the excluded names are listed here by construction.

Script-defined names that are also ExportFnc names (calls excluded from the table because the scripts define them): `Activate`, `GetDistFromClosestStimulus`, `SetBrainParam`.

## 4. Player references

Every non-comment line in a story or bonus `MAIN.SPJ` closure that contains the string `"hunter"`, a `player.` / `::globals.player` call. Hunt-map scripts are not in this table (`re/notes/lead-mission.md` already covers their `"hunter"` uses). Kind is taken from the call on the same line. For `GetDist*`, the following `if` is quoted when it is within the next three lines.

| Kind | Count |
|---|---:|
| player flags | 419 |
| task target | 341 |
| trigger tests player | 317 |
| camera/cutscene | 222 |
| bind player entity | 98 |
| player method | 98 |
| inventory/weapon | 61 |
| damage/health | 13 |
| distance check | 8 |
| teleport/move | 4 |
| **total lines** | 1581 |

| File:line | Call | What it decides | Maps that include this file |
|---|---|---|---|
| `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:23` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_CAN_CROUCH);` | MAP1 |
| `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:108` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP1 |
| `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:149` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "handgun5_1st");` | MAP1 |
| `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:150` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "rifle2_1st");` | MAP1 |
| `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:189` | `SetPFlags` | player flags — `::globals.player.SetPFlags(PLAYER_CAN_CROUCH);` | MAP1 |
| `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:193` | `GetDistBetween2Ais` | distance check next if: if (fDistance >::globals. fRadiusOff) — `fDistance = GetDistBetween2Ais(::globals.szStealthCenter, "hunter");` | MAP1 |
| `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:212` | `ClearPFlags` | player flags — `::globals.player.ClearPFlags(PLAYER_CAN_CROUCH);` | MAP1 |
| `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR:216` | `GetDistBetween2Ais` | distance check next if: if (fDistance < ::globals.fRadiusOn) — `fDistance = GetDistBetween2Ais(::globals.szStealthCenter, "hunter");` | MAP1 |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:22` | `IsCrouch, IsLegHurt` | damage/health — `if(!player.IsCrouch() && player.IsLegHurt())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:24` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_CROUCH);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:30` | `IsCrouch` | player method — `else if(player.IsCrouch())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:32` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_CROUCH);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:49` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_CROUCH);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:106` | `IsCrouch` | player method — `if(player.IsCrouch())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:108` | `IsLegHurt` | damage/health — `else if(!player.IsLegHurt())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:114` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_WALK);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:148` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_WALK);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:164` | `IsCrouch` | player method — `if(player.IsCrouch())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:166` | `IsLegHurt` | damage/health — `else if(!player.IsLegHurt())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:172` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_RUN);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:207` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_RUN);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:219` | `IsCrouch` | player method — `if(player.IsCrouch())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:222` | `IsLegHurt` | damage/health — `if(!player.IsLegHurt())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:230` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_WALK);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:264` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_WALK);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:276` | `IsCrouch` | player method — `if(player.IsCrouch())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:279` | `IsLegHurt` | damage/health — `if(!player.IsLegHurt())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:287` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_RUN);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:321` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_RUN);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:380` | `IsCrouch` | player method — `if(player.IsCrouch())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:383` | `IsLegHurt` | damage/health — `if(!player.IsLegHurt())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:391` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_WALK);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:395` | `GetRotYawSpeed` | player method — `fAnimSpeedFactor = (-0.85/132)*player.GetRotYawSpeed()+1.0;` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:430` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_WALK);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:442` | `IsCrouch` | player method — `if(player.IsCrouch())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:445` | `IsLegHurt` | damage/health — `if(!player.IsLegHurt())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:453` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_WALK);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:459` | `GetRotYawSpeed` | player method — `fAnimSpeedFactor = (-0.85/132)*player.GetRotYawSpeed()+1.0;` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:493` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_WALK);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:505` | `IsCrouch` | player method — `if(player.IsCrouch())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:508` | `IsLegHurt` | damage/health — `if(!player.IsLegHurt())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:516` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_RUN);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:552` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_RUN);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:564` | `IsCrouch` | player method — `if(player.IsCrouch())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:567` | `IsLegHurt` | damage/health — `if(!player.IsLegHurt())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:575` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_RUN);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:611` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_RUN);` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:648` | `IsInVehicle` | player method — `if(TRUE == player.IsInVehicle())` | shared by 28 maps (MAP1..HUNTMAP4) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:32` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:33` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:34` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_LIEDEAD);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:35` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_FIRE);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:36` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RELOAD);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:37` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_DRIVE);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:38` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:39` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:88` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:108` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:121` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:137` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:149` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:165` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:177` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:193` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:206` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:224` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:236` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:252` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:264` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:280` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:292` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:308` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:321` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:335` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:348` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:359` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:372` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:382` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:395` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:405` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:419` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:431` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:445` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:457` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:471` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:484` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:498` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:510` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:525` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:538` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:552` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:564` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:578` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:591` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:605` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:617` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:631` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:641` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:653` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:663` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:676` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:688` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:701` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:713` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:725` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:735` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:747` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:757` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:771` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:786` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:800` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:815` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:829` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:844` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:857` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:871` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:885` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:899` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:912` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_LIEDEAD);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:926` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_LIEDEAD);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:939` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_LIEDEAD);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:952` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_LIEDEAD);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:966` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:980` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:993` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1007` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1020` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1034` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1048` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1062` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1074` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1088` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1101` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1114` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1127` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1143` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1156` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1172` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1185` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1201` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1215` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1231` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1245` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_DRIVE);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1258` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_DRIVE);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1273` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1288` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1301` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1311` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1324` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1334` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1347` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/MODELS/NPC/GUIDE/GUIDELEGS.SCR:1357` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | shared by 23 maps (MAP1..MAPTSAVO) |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:31` | `StartTask` | task target — `::globals.player.StartTask("1"); //start task` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:47` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:68` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:91` | `StartTask` | task target — `::globals.player.StartTask("2"); //start task` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:120` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:141` | `StartTask` | task target — `::globals.player.StartTask("3"); //start task` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:164` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:184` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:189` | `DisableStimulus` | player method — `::globals.player.DisableStimulus();//disables player stimulus on animals` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:198` | `SetStealthLevels` | player method — `::globals.player.SetStealthLevels(0.2, 0.25, 0.30, 0.35);` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:206` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:229` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:239` | `PlayHint` | player method — `::globals.player.PlayHint("Hunt over. Follow Hugh.");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:241` | `PlayHint` | player method — `::globals.player.PlayHint("Hunt successful. Follow Hugh.");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:262` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:285` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:297` | `SetTaskParam` | task target — `::globals.mlion.SetTaskParam("target", "hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:312` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:332` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:342` | `StartTask` | task target — `::globals.player.StartTask("6");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:349` | `SetTaskParam` | task target — `::globals.bear.SetTaskParam("target", "hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:366` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:385` | `SetTaskParam` | task target — `::globals.hugsm1.SetTaskParam("target_cutscene", "hunter");` | MAP1 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:419` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:35` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:36` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:60` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:61` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:78` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:79` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE2.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE2.SCR:39` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE2.SCR:40` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE2.SCR:84` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE2.SCR:85` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE2.SCR:102` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE2.SCR:103` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE3.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE3.SCR:35` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE3.SCR:36` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE3.SCR:67` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE3.SCR:68` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE3.SCR:85` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE3.SCR:86` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE4.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE4.SCR:43` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE4.SCR:44` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE4.SCR:90` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE4.SCR:91` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE4.SCR:110` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE4.SCR:111` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE5.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE5.SCR:54` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE5.SCR:55` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE5.SCR:131` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE5.SCR:132` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE5.SCR:153` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP1 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE5.SCR:154` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP1 |
| `re/extracted/SCRIPTS/LEVELS/MAP2A/WORLD.SCR:96` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2A |
| `re/extracted/SCRIPTS/LEVELS/MAP2A/WORLD.SCR:135` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "handgun5_1st");` | MAP2A |
| `re/extracted/SCRIPTS/LEVELS/MAP2A/WORLD.SCR:136` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "rifle1_1st");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:19` | `SetDenySATime` | player method — `::globals.player.SetDenySATime(25.0);` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:23` | `AddItemAmmo` | inventory/weapon — `::globals.player.AddItemAmmo("ammo_handgun5_1st", -5);` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:30` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:47` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:66` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:82` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:108` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:119` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:123` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:127` | `SetTaskParam` | task target — `::globals.wolf2.SetTaskParam("target", "hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:148` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:179` | `SetTaskParam` | task target — `::globals.mlion1.SetTaskParam("target", "hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:188` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:203` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:216` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:218` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:229` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:242` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:244` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:256` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:273` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:284` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:305` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:311` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:322` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:343` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:357` | `StartTask` | task target — `::globals.player.StartTask("6");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:370` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:389` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:411` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:441` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:461` | `SetTaskParam` | task target — `::globals.wolf3.SetTaskParam("target", "hunter");` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:33` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:34` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:49` | `PlaySound` | player method — `player.PlaySound("wolf_howl_stream");` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:68` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:69` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:86` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:87` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:26` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:54` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:55` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:83` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:84` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:103` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:104` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE3.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE3.SCR:26` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE3.SCR:27` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE3.SCR:108` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE3.SCR:109` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE3.SCR:125` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE3.SCR:126` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:19` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:24` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:25` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:42` | `SetTaskParam` | task target — `hugs2a.SetTaskParam("target_cutscene", "hunter");` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:99` | `SetTaskParam` | task target — `hugs2a.SetTaskParam("target_cutscene", "hunter");` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:112` | `TakeItem` | inventory/weapon — `player.TakeItem("rifle1_1st");` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:128` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:129` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:144` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:145` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE5.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE5.SCR:30` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE5.SCR:31` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE5.SCR:157` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE5.SCR:158` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE5.SCR:174` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE5.SCR:175` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE6.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE6.SCR:40` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE6.SCR:41` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE6.SCR:70` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE6.SCR:71` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE6.SCR:89` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE6.SCR:90` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2A |
| `re/extracted/SCRIPTS/LEVELS/MAP2B/WORLD.SCR:155` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2B |
| `re/extracted/SCRIPTS/LEVELS/MAP2B/WORLD.SCR:199` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "handgun5_1st");` | MAP2B |
| `re/extracted/SCRIPTS/LEVELS/MAP2B/WORLD.SCR:200` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "rifle1_1st");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:7` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:44` | `SetTaskParam` | task target — `::globals.wolf3.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:52` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:69` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:105` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:130` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:150` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:168` | `SetTaskParam` | task target — `::globals.wolf2.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:171` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:180` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:209` | `SetTaskParam` | task target — `::globals.gbear.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:212` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:237` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:265` | `PlayHint` | player method — `::globals.player.PlayHint("You received Hugh's Rifle");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:266` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:334` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:354` | `SetTaskParam` | task target — `::globals.wolf2.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:361` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:368` | `SetTaskParam` | task target — `::globals.wolf3.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:378` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:387` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:396` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:413` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:461` | `SetTaskParam` | task target — `::globals.mlion1.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:465` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:512` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:631` | `SetCanBeAttacked` | player method — `::globals.player.SetCanBeAttacked(1);` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:642` | `PlayHint` | player method — `::globals.player.PlayHint("Use Hugh's rifle to stop the wolves from a distance.");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:644` | `StartTask` | task target — `::globals.player.StartTask("6");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:645` | `SetCanBeAttacked` | player method — `::globals.player.SetCanBeAttacked(0);` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:846` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:902` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:912` | `StartTask` | task target — `::globals.player.StartTask("7");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:923` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:951` | `SetTaskParam` | task target — `::globals.mlion1.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:967` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:976` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:985` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:997` | `StartTask` | task target — `::globals.player.StartTask("8");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1014` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1032` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1052` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1071` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1106` | `SetTaskParam` | task target — `::globals.wolf2.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1115` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1145` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1170` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1193` | `SetTaskParam` | task target — `::globals.wolf2.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1205` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1238` | `SetTaskParam` | task target — `::globals.wolf3.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1246` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1276` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1302` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1333` | `SetTaskParam` | task target — `::globals.mlion1.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1338` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE1.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE1.SCR:43` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE1.SCR:44` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE1.SCR:87` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE1.SCR:88` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE1.SCR:108` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE1.SCR:109` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:21` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:29` | `MoveToPos` | teleport/move — `player.MoveToPos("c4_2");` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:39` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:40` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:62` | `TakeItem` | inventory/weapon — `player.TakeItem("rifle1_1st");` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:81` | `TakeItem` | inventory/weapon — `player.TakeItem("rifle1_1st");` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:86` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:87` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:97` | `MoveToPos` | teleport/move — `player.MoveToPos("c4_2");` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:99` | `TakeItem` | inventory/weapon — `player.TakeItem("rifle1_1st");` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:108` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:109` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:123` | `SetTaskParam` | task target — `::globals.wolf2.SetTaskParam("target", "hunter");` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE3.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE3.SCR:57` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE3.SCR:58` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE3.SCR:104` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE3.SCR:105` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE3.SCR:127` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE3.SCR:128` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2B |
| `re/extracted/SCRIPTS/LEVELS/MAP2C/WORLD.SCR:23` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_CAN_CROUCH);` | MAP2C |
| `re/extracted/SCRIPTS/LEVELS/MAP2C/WORLD.SCR:103` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2C |
| `re/extracted/SCRIPTS/LEVELS/MAP2C/WORLD.SCR:140` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "handgun5_1st");` | MAP2C |
| `re/extracted/SCRIPTS/LEVELS/MAP2C/WORLD.SCR:141` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "rifle1_1st");` | MAP2C |
| `re/extracted/SCRIPTS/LEVELS/MAP2C/WORLD.SCR:142` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "shotgun1_1st");` | MAP2C |
| `re/extracted/SCRIPTS/LEVELS/MAP2C/WORLD.SCR:178` | `SetPFlags` | player flags — `::globals.player.SetPFlags(PLAYER_CAN_CROUCH);` | MAP2C |
| `re/extracted/SCRIPTS/LEVELS/MAP2C/WORLD.SCR:182` | `GetDistBetween2Ais` | distance check next if: if (fDistance >::globals. fRadiusOff) — `fDistance = GetDistBetween2Ais(::globals.szStealthCenter, "hunter");` | MAP2C |
| `re/extracted/SCRIPTS/LEVELS/MAP2C/WORLD.SCR:201` | `ClearPFlags` | player flags — `::globals.player.ClearPFlags(PLAYER_CAN_CROUCH);` | MAP2C |
| `re/extracted/SCRIPTS/LEVELS/MAP2C/WORLD.SCR:205` | `GetDistBetween2Ais` | distance check next if: if (fDistance < ::globals.fRadiusOn) — `fDistance = GetDistBetween2Ais(::globals.szStealthCenter, "hunter");` | MAP2C |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:29` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:30` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:31` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_LIEDEAD);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:32` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_FIRE);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:33` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RELOAD);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:34` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_DRIVE);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:35` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:36` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:81` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:101` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:114` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:131` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:144` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:161` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:174` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:191` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:204` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:222` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:236` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:253` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:266` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:283` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:296` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:313` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:326` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:340` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:353` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:363` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:376` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:386` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:399` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:409` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:422` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:432` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:445` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:456` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:469` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:479` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:494` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:507` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:521` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:533` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:547` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:560` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:574` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:586` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_TURN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:599` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:611` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:625` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:638` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:651` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:664` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:677` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:690` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_WALK);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:704` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:717` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_RUN);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:730` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:740` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:753` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:763` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:777` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/MODELS/NPC/GIRL/GIRLLEGS.SCR:787` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_NPC_OTHER);` | MAP2C,MAP6,MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:35` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:45` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:63` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:71` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:80` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:90` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:117` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:127` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:128` | `PlayHint` | player method — `::globals.player.PlayHint("You received Hugh's shotgun");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:147` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:154` | `DisableStimulus` | player method — `::globals.player.DisableStimulus();` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:158` | `SetStealthLevels` | player method — `::globals.player.SetStealthLevels(0.2, 0.3, 0.4, 0.45); //set stealth levels:crouch/crouch-walk, stand, walk, run - can be set in world.scr also` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:178` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:179` | `StartTask` | task target — `::globals.player.StartTask("6");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:229` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:240` | `SetTaskParam` | task target — `::globals.gbear.SetTaskParam("target", "hunter");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:244` | `StartTask` | task target — `::globals.player.StartTask("7");` | MAP2C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:259` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:24` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:32` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:33` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:114` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:115` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:137` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:138` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE2.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE2.SCR:33` | `MoveToPos` | teleport/move — `player.MoveToPos("c1_13");` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE2.SCR:47` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE2.SCR:48` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE2.SCR:78` | `TakeItem` | inventory/weapon — `player.TakeItem("shotgun1_1st");` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE2.SCR:94` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE2.SCR:95` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE2.SCR:105` | `TakeItem` | inventory/weapon — `player.TakeItem("shotgun1_1st");` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE2.SCR:116` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE2.SCR:117` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE4.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE4.SCR:43` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE4.SCR:44` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE4.SCR:70` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE4.SCR:71` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE4.SCR:89` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP2C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE4.SCR:90` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP2C |
| `re/extracted/SCRIPTS/LEVELS/MAP3A/WORLD.SCR:86` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP3A |
| `re/extracted/SCRIPTS/LEVELS/MAP3A/WORLD.SCR:88` | `SetMaxKnifeDistance` | player method — `player.SetMaxKnifeDistance(2.0);` | MAP3A |
| `re/extracted/SCRIPTS/LEVELS/MAP3A/WORLD.SCR:122` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "macheta_1st");` | MAP3A |
| `re/extracted/SCRIPTS/LEVELS/MAP3A/WORLD.SCR:123` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "handgun3_1st");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:8` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:16` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:49` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:58` | `SetTaskParam` | task target — `::globals.hyena4.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:73` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:93` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:106` | `SetTaskParam` | task target — `::globals.hyena1.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:110` | `SetTaskParam` | task target — `::globals.hyena2.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:121` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:131` | `SetTaskParam` | task target — `::globals.hyena3.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:135` | `SetTaskParam` | task target — `::globals.hyena4.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:137` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:147` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:156` | `PlayHint` | player method — `::globals.player.PlayHint("You found water.");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:159` | `SetTaskParam` | task target — `::globals.hyena1.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:163` | `SetTaskParam` | task target — `::globals.hyena2.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:178` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:187` | `PlayHint` | player method — `::globals.player.PlayHint("You found water.");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:190` | `SetTaskParam` | task target — `::globals.hyena1.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:194` | `SetTaskParam` | task target — `::globals.hyena2.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:209` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:219` | `PlayHint` | player method — `::globals.player.PlayHint("You found water.");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:222` | `SetTaskParam` | task target — `::globals.hyena1.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:226` | `SetTaskParam` | task target — `::globals.hyena2.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:241` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:251` | `PlayHint` | player method — `::globals.player.PlayHint("You found a camp area.");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:254` | `SetTaskParam` | task target — `::globals.hyena3.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:260` | `SetTaskParam` | task target — `::globals.hyena4.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:263` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:276` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:286` | `SetTaskParam` | task target — `::globals.hyena1.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:290` | `SetTaskParam` | task target — `::globals.hyena2.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:292` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:304` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:314` | `SetTaskParam` | task target — `::globals.crocodile1.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:318` | `SetTaskParam` | task target — `::globals.crocodile2.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:342` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:351` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:362` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:372` | `SetTaskParam` | task target — `::globals.hyena1.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:376` | `SetTaskParam` | task target — `::globals.hyena2.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:380` | `SetTaskParam` | task target — `::globals.hyena3.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:382` | `StartTask` | task target — `::globals.player.StartTask("6");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:393` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:408` | `SetAIType` | trigger tests player — `::globals.abby.SetAIType(AITYPE_COMPANION\|AITYPE_COVERSTAY ,"hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:411` | `StartTask` | task target — `::globals.player.StartTask("7");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:420` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:429` | `SetTaskParam` | task target — `::globals.leopard1.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:433` | `SetTaskParam` | task target — `::globals.leopard2.SetTaskParam("target", "hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:435` | `StartTask` | task target — `::globals.player.StartTask("8");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:451` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:462` | `StartTask` | task target — `::globals.player.StartTask("9");` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:38` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:39` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:79` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:80` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:97` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:98` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:45` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:46` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:88` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:89` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:109` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:110` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE3.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE3.SCR:40` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE3.SCR:41` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE3.SCR:70` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE3.SCR:71` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE3.SCR:89` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE3.SCR:90` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE4.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE4.SCR:40` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE4.SCR:41` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE4.SCR:70` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE4.SCR:71` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE4.SCR:89` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE4.SCR:90` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3A |
| `re/extracted/SCRIPTS/LEVELS/MAP3B/WORLD.SCR:58` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP3B |
| `re/extracted/SCRIPTS/LEVELS/MAP3B/WORLD.SCR:60` | `SetMaxKnifeDistance` | player method — `player.SetMaxKnifeDistance(2.0);` | MAP3B |
| `re/extracted/SCRIPTS/LEVELS/MAP3B/WORLD.SCR:99` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "macheta_1st");` | MAP3B |
| `re/extracted/SCRIPTS/LEVELS/MAP3B/WORLD.SCR:100` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "handgun3_1st");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:16` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:23` | `SetTaskParam` | task target — `::globals.cobra1.SetTaskParam("target", "hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:27` | `SetTaskParam` | task target — `::globals.cobra2.SetTaskParam("target", "hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:31` | `SetTaskParam` | task target — `::globals.cobra3.SetTaskParam("target", "hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:43` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:55` | `SetTaskParam` | task target — `::globals.leopard1.SetTaskParam("target", "hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:59` | `SetTaskParam` | task target — `::globals.leopard2.SetTaskParam("target", "hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:69` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:82` | `SetTaskParam` | task target — `::globals.leopard1.SetTaskParam("target", "hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:86` | `SetTaskParam` | task target — `::globals.leopard2.SetTaskParam("target", "hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:94` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:103` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:108` | `SetTaskParam` | task target — `::globals.leopard1.SetTaskParam("target", "hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:116` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:134` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:144` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP3B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:153` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3B |
| `re/extracted/SCRIPTS/LEVELS/MAP3C/WORLD.SCR:70` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP3C |
| `re/extracted/SCRIPTS/LEVELS/MAP3C/WORLD.SCR:72` | `SetMaxKnifeDistance` | player method — `player.SetMaxKnifeDistance(2.0);` | MAP3C |
| `re/extracted/SCRIPTS/LEVELS/MAP3C/WORLD.SCR:106` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "macheta_1st");` | MAP3C |
| `re/extracted/SCRIPTS/LEVELS/MAP3C/WORLD.SCR:107` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "handgun3_1st");` | MAP3C |
| `re/extracted/SCRIPTS/LEVELS/MAP3C/WORLD.SCR:108` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "bino");` | MAP3C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:16` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP3C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:28` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:36` | `SetCanBeAttacked` | player method — `::globals.player.SetCanBeAttacked(0);` | MAP3C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:67` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:90` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP3C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:111` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:121` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP3C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:124` | `SetTaskParam` | task target — `::globals.lion3.SetTaskParam("target", "hunter");` | MAP3C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:140` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE1.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE1.SCR:54` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE1.SCR:55` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE1.SCR:113` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE1.SCR:114` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE1.SCR:137` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE1.SCR:138` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:31` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:47` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:48` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:85` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:86` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:107` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:108` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE3.SCR:29` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE3.SCR:47` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE3.SCR:48` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE3.SCR:72` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE3.SCR:73` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE3.SCR:91` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP3C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE3.SCR:92` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP3C |
| `re/extracted/SCRIPTS/LEVELS/MAP4A/WORLD.SCR:66` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP4A |
| `re/extracted/SCRIPTS/LEVELS/MAP4A/WORLD.SCR:68` | `SetMaxKnifeDistance` | player method — `player.SetMaxKnifeDistance(1.3);` | MAP4A |
| `re/extracted/SCRIPTS/LEVELS/MAP4A/WORLD.SCR:108` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "bow_1st");` | MAP4A |
| `re/extracted/SCRIPTS/LEVELS/MAP4A/WORLD.SCR:109` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "knife1_1st");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:8` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:16` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:23` | `GetInCar` | player method — `::globals.player.GetInCar(2);` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:44` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("M4AF03", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:49` | `SetTaskParam` | task target — `::globals.lion1.SetTaskParam("target", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:55` | `SetTaskParam` | task target — `::globals.lion2.SetTaskParam("target", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:58` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:81` | `SetTaskParam` | task target — `::globals.lion3.SetTaskParam("target", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:87` | `SetTaskParam` | task target — `::globals.lion4.SetTaskParam("target", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:109` | `SetTaskParam` | task target — `::globals.lion5.SetTaskParam("target", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:131` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("M4AF04", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:152` | `SetTaskParam` | task target — `::globals.lion6.SetTaskParam("target", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:158` | `SetTaskParam` | task target — `::globals.lion1.SetTaskParam("target", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:175` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("M4AF05", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:195` | `SetTaskParam` | task target — `::globals.lion2.SetTaskParam("target", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:199` | `SetTaskParam` | task target — `::globals.lion2.SetTaskParam("target", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:203` | `SetTaskParam` | task target — `::globals.lion3.SetTaskParam("target", "hunter");` | MAP4A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:205` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE1.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE1.SCR:24` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE1.SCR:25` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE1.SCR:56` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE1.SCR:57` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE1.SCR:72` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE1.SCR:73` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:24` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:25` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:58` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:59` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:74` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:75` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4A |
| `re/extracted/SCRIPTS/LEVELS/MAP4B/WORLD.SCR:23` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_CAN_CROUCH);` | MAP4B |
| `re/extracted/SCRIPTS/LEVELS/MAP4B/WORLD.SCR:129` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP4B |
| `re/extracted/SCRIPTS/LEVELS/MAP4B/WORLD.SCR:166` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "bow_1st");` | MAP4B |
| `re/extracted/SCRIPTS/LEVELS/MAP4B/WORLD.SCR:167` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "knife1_1st");` | MAP4B |
| `re/extracted/SCRIPTS/LEVELS/MAP4B/WORLD.SCR:227` | `SetPFlags` | player flags — `::globals.player.SetPFlags(PLAYER_CAN_CROUCH);` | MAP4B |
| `re/extracted/SCRIPTS/LEVELS/MAP4B/WORLD.SCR:232` | `GetDistBetween2Ais` | distance check — `fDistance = GetDistBetween2Ais(::globals.szStealthCenter, "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/LEVELS/MAP4B/WORLD.SCR:267` | `ClearPFlags` | player flags — `::globals.player.ClearPFlags(PLAYER_CAN_CROUCH);` | MAP4B |
| `re/extracted/SCRIPTS/LEVELS/MAP4B/WORLD.SCR:272` | `GetDistBetween2Ais` | distance check — `fDistance = GetDistBetween2Ais(::globals.szStealthCenter, "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:26` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:51` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:62` | `SetTaskParam` | task target — `::globals.lion1.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:66` | `SetTaskParam` | task target — `::globals.lion2.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:79` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:88` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:99` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:110` | `SetTaskParam` | task target — `::globals.lion1.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:114` | `SetTaskParam` | task target — `::globals.lion2.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:126` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:136` | `SetTaskParam` | task target — `::globals.crocodile1.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:140` | `SetTaskParam` | task target — `::globals.crocodile2.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:144` | `SetTaskParam` | task target — `::globals.crocodile3.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:148` | `SetTaskParam` | task target — `::globals.crocodile4.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:152` | `SetTaskParam` | task target — `::globals.crocodile5.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:163` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:172` | `SetTaskParam` | task target — `::globals.hippo.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:188` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:197` | `SetTaskParam` | task target — `::globals.hippo.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:213` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:218` | `DisableStimulus` | player method — `::globals.player.DisableStimulus();` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:221` | `SetStealthLevels` | player method — `::globals.player.SetStealthLevels(0.2, 0.25, 0.30, 0.35);` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:241` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:250` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:267` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:272` | `DisableStimulus` | player method — `::globals.player.DisableStimulus();` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:277` | `SetStealthLevels` | player method — `::globals.player.SetStealthLevels(0.2, 0.25, 0.30, 0.35);` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:296` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:308` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:317` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:344` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:360` | `SetTaskParam` | task target — `::globals.lion5.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:364` | `SetTaskParam` | task target — `::globals.lion4.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:375` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:384` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:388` | `SetTaskParam` | task target — `::globals.lion1.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:409` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:417` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:427` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:435` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:445` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:468` | `SetTaskParam` | task target — `::globals.lion2.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:472` | `SetTaskParam` | task target — `::globals.lion3.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:480` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:509` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:532` | `SetTaskParam` | task target — `::globals.lion2.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:536` | `SetTaskParam` | task target — `::globals.lion3.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:544` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:571` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:594` | `SetTaskParam` | task target — `::globals.lion2.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:598` | `SetTaskParam` | task target — `::globals.lion3.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:606` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:635` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:646` | `SetTaskParam` | task target — `::globals.lion1.SetTaskParam("target", "hunter");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:652` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP4B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:662` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE1.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE1.SCR:40` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE1.SCR:41` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE1.SCR:66` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE1.SCR:67` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE1.SCR:85` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE1.SCR:86` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE2.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE2.SCR:40` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE2.SCR:41` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE2.SCR:66` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE2.SCR:67` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE2.SCR:85` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE2.SCR:86` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE5.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE5.SCR:61` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE5.SCR:62` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE5.SCR:97` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE5.SCR:98` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE5.SCR:119` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE5.SCR:120` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE6.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE6.SCR:62` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE6.SCR:63` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE6.SCR:98` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE6.SCR:99` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE6.SCR:120` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE6.SCR:121` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE7.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE7.SCR:62` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE7.SCR:63` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE7.SCR:98` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE7.SCR:99` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE7.SCR:120` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP4B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE7.SCR:121` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP4B |
| `re/extracted/SCRIPTS/LEVELS/MAP5/WORLD.SCR:74` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP5 |
| `re/extracted/SCRIPTS/LEVELS/MAP5/WORLD.SCR:112` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "blowdarts_1st");` | MAP5 |
| `re/extracted/SCRIPTS/LEVELS/MAP5/WORLD.SCR:116` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "tranqgun1_1st");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:8` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:19` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:57` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:75` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:101` | `SetTaskParam` | task target — `::globals.hyena2.SetTaskParam("target", "hunter");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:103` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:122` | `SetTaskParam` | task target — `::globals.hyena1.SetTaskParam("target", "hunter");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:134` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:186` | `SetTaskParam` | task target — `::globals.hyena1.SetTaskParam("target", "hunter");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:213` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:235` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:243` | `PlayHint` | player method — `::globals.player.PlayHint("You received a Tranquilizer Rifle");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:267` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:268` | `SetRecoverSpeed` | player method — `::globals.player.SetRecoverSpeed(0.03); //the stampede was too easy` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:307` | `SetTaskParam` | task target — `::globals.elephant1.SetTaskParam("target", "hunter");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:312` | `SetTaskParam` | task target — `::globals.elephant2.SetTaskParam("target", "hunter");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:351` | `SetTaskParam` | task target — `::globals.elephant1.SetTaskParam("target", "hunter");` | MAP5 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:365` | `SetTaskParam` | task target — `::globals.elephant2.SetTaskParam("target", "hunter");` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:24` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:40` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:41` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:79` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:80` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:99` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:100` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:26` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:55` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:56` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:112` | `GetInCar` | player method — `player.GetInCar(2);` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:129` | `TakeItem` | inventory/weapon — `player.TakeItem("tranqgun1_1st");` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:140` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:141` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:170` | `TakeItem` | inventory/weapon — `player.TakeItem("tranqgun1_1st");` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:172` | `GetInCar` | player method — `player.GetInCar(2);` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:181` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:182` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE4.SCR:21` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE4.SCR:23` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE4.SCR:24` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE4.SCR:55` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE4.SCR:56` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE4.SCR:71` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP5 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE4.SCR:72` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP5 |
| `re/extracted/SCRIPTS/LEVELS/MAP6/WORLD.SCR:74` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP6 |
| `re/extracted/SCRIPTS/LEVELS/MAP6/WORLD.SCR:116` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "tranqgun1_1st");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:30` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:50` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:67` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:76` | `SetTaskParam` | task target — `::globals.leopard1.SetTaskParam("target", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:78` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:96` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:112` | `SetTaskParam` | task target — `::globals.leopard3.SetTaskParam("target", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:121` | `SetTaskParam` | task target — `::globals.leopard2.SetTaskParam("target", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:124` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:133` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:144` | `SetTaskParam` | task target — `::globals.abear.SetTaskParam("target", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:146` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:164` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:180` | `SetTaskParam` | task target — `::globals.leopard1.SetTaskParam("target", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:185` | `SetTaskParam` | task target — `::globals.leopard2.SetTaskParam("target", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:187` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:196` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:211` | `StartTask` | task target — `::globals.player.StartTask("6");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:232` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:241` | `StartTask` | task target — `::globals.player.StartTask("7");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:248` | `SetTaskParam` | task target — `::globals.leopard4.SetTaskParam("target", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:263` | `SetTaskParam` | task target — `::globals.leopard1.SetTaskParam("target", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:281` | `SetTaskParam` | task target — `::globals.leopard2.SetTaskParam("target", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:287` | `SetTaskParam` | task target — `::globals.leopard3.SetTaskParam("target", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:301` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:320` | `StartTask` | task target — `::globals.player.StartTask("8");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:343` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:378` | `SetTaskParam` | task target — `::globals.reggie6.SetTaskParam("target_cutscene", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:412` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:422` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("map6_envStream1", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:431` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP6 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:441` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("map6_envStream2", "hunter");` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:38` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:39` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:71` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:72` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:89` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:90` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE2.SCR:27` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE2.SCR:60` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE2.SCR:61` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE2.SCR:108` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE2.SCR:109` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE2.SCR:127` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE2.SCR:128` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE3.SCR:31` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE3.SCR:94` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE3.SCR:95` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE3.SCR:147` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE3.SCR:148` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE3.SCR:171` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE3.SCR:172` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE4.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE4.SCR:39` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE4.SCR:40` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE4.SCR:67` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE4.SCR:68` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE4.SCR:86` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE4.SCR:87` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE5.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE5.SCR:40` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE5.SCR:41` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE5.SCR:66` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE5.SCR:67` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE5.SCR:85` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP6 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE5.SCR:86` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP6 |
| `re/extracted/SCRIPTS/LEVELS/MAP7/WORLD.SCR:89` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP7 |
| `re/extracted/SCRIPTS/LEVELS/MAP7/WORLD.SCR:126` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "tranqgun1_1st");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:29` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:58` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:69` | `SetTaskParam` | task target — `::globals.tiger1.SetTaskParam("target", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:71` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:80` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:92` | `SetTaskParam` | task target — `::globals.tiger2.SetTaskParam("target", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:96` | `SetTaskParam` | task target — `::globals.tiger3.SetTaskParam("target", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:98` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:107` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:145` | `SetTaskParam` | task target — `::globals.tiger4.SetTaskParam("target", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:168` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:207` | `SetTaskParam` | task target — `::globals.tiger1.SetTaskParam("target", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:210` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:230` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:246` | `SetTaskParam` | task target — `::globals.tiger2.SetTaskParam("target", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:251` | `SetTaskParam` | task target — `::globals.tiger3.SetTaskParam("target", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:254` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:265` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:297` | `SetTaskParam` | task target — `::globals.reggie7.SetTaskParam("target_cutscene", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:328` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:348` | `SetTaskParam` | task target — `::globals.cobra1.SetTaskParam("target", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:351` | `StartTask` | task target — `::globals.player.StartTask("6");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:376` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:397` | `StartTask` | task target — `::globals.player.StartTask("7");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:428` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:440` | `StartTask` | task target — `::globals.player.StartTask("8");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:441` | `StartTask` | task target — `::globals.player.StartTask("9");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:458` | `SetTaskParam` | task target — `::globals.rhino1.SetTaskParam("target", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:484` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:517` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:561` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:571` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("map7_envStream1", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:580` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:590` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("map7_envStream2", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:599` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:609` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("map7_envStream3", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:618` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:628` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("map7_envStream4", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:637` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:647` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("map7_envStream5", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:656` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:666` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("map7_envStream6", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:675` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP7 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:685` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("map7_envStream7", "hunter");` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE1.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE1.SCR:35` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE1.SCR:36` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE1.SCR:66` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE1.SCR:67` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE1.SCR:84` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE1.SCR:85` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE2.SCR:20` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE2.SCR:35` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE2.SCR:36` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE2.SCR:60` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE2.SCR:61` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE2.SCR:78` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE2.SCR:79` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE6.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE6.SCR:47` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE6.SCR:48` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE6.SCR:138` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE6.SCR:139` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE6.SCR:160` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP7 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP7/CUTSCENE6.SCR:161` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP7 |
| `re/extracted/SCRIPTS/LEVELS/MAP8/WORLD.SCR:132` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP8 |
| `re/extracted/SCRIPTS/LEVELS/MAP8/WORLD.SCR:167` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "dbrifle1_1st");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:42` | `SetTaskParam` | task target — `::globals.scrubbul1.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:58` | `SetTaskParam` | task target — `::globals.scrubbul2.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:73` | `SetTaskParam` | task target — `::globals.scrubbul3.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:88` | `SetTaskParam` | task target — `::globals.scrubbul4.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:103` | `SetTaskParam` | task target — `::globals.scrubbul5.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:147` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:157` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:185` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:232` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:242` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:246` | `SetTaskParam` | task target — `::globals.scrubbul1.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:260` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:268` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:272` | `SetTaskParam` | task target — `::globals.scrubbul2.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:281` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:290` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:294` | `SetTaskParam` | task target — `::globals.scrubbul3.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:306` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:314` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:318` | `SetTaskParam` | task target — `::globals.scrubbul1.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:329` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:339` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("stone_water", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:354` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:368` | `SetDenySATime` | player method — `::globals.player.SetDenySATime(0.1);` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:374` | `SetTaskParam` | task target — `::globals.crocodile1.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:380` | `SetTaskParam` | task target — `::globals.crocodile2.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:387` | `SetTaskParam` | task target — `::globals.crocodile3.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:389` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("M8RD14", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:407` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:433` | `SetTaskParam` | task target — `::globals.croco1.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:442` | `SetTaskParam` | task target — `::globals.croco2.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:450` | `SetTaskParam` | task target — `::globals.croco3.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:468` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("M8RD13", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:489` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:500` | `StartTask` | task target — `::globals.player.StartTask("6");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:511` | `SetAIType` | trigger tests player — `::globals.tribesman1.SetAIType(AITYPE_COMPANION,"hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:512` | `SetAIType` | trigger tests player — `::globals.tribesman2.SetAIType(AITYPE_COMPANION,"hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:513` | `SetAIType` | trigger tests player — `::globals.tribesman3.SetAIType(AITYPE_COMPANION,"hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:532` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:547` | `SetTaskParam` | task target — `::globals.scrubbul1.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:549` | `StartTask` | task target — `::globals.player.StartTask("7");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:559` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:581` | `SetTaskParam` | task target — `::globals.scrubbul2.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:583` | `StartTask` | task target — `::globals.player.StartTask("8");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:593` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:605` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("tree_fall", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:614` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:627` | `StartTask` | task target — `::globals.player.StartTask("10");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:629` | `SetDenySATime` | player method — `::globals.player.SetDenySATime(15.0);` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:637` | `SetAIType` | trigger tests player — `::globals.tribesman1.SetAIType(AITYPE_COMPANION \| AITYPE_IDLESCARE, "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:638` | `SetAIType` | trigger tests player — `::globals.tribesman2.SetAIType(AITYPE_COMPANION \| AITYPE_IDLESCARE, "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:639` | `SetAIType` | trigger tests player — `::globals.tribesman3.SetAIType(AITYPE_COMPANION \| AITYPE_IDLESCARE, "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:676` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:696` | `SetTaskParam` | task target — `::globals.scrubbul3.SetTaskParam("target", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:698` | `StartTask` | task target — `::globals.player.StartTask("9");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:712` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:751` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:791` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:830` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:869` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:906` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:950` | `SetAIType` | trigger tests player — `::globals.tribesman1.SetAIType(AITYPE_COMPANION , "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:951` | `SetAIType` | trigger tests player — `::globals.tribesman2.SetAIType(AITYPE_COMPANION , "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:952` | `SetAIType` | trigger tests player — `::globals.tribesman3.SetAIType(AITYPE_COMPANION , "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:964` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:972` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("M8RD05", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:981` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:999` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1016` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1025` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("M8RD15", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1035` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1043` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1056` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1065` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("M8RD15", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1074` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1085` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1097` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1107` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("map8_envStream1", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1116` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP8 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1125` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("map8_envStream2", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:34` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:35` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:44` | `PlaySound` | player method — `player.PlaySound("M8RD01");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:45` | `PlayMessage` | player method — `player.PlayMessage("This is Reginald Dowling.\|Do you read me, chap?<key_time:2>");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:49` | `PlaySound` | player method — `player.PlaySound("M8RD02");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:50` | `PlayMessage` | player method — `player.PlayMessage("Testing, testing...\|Is this thing on?<key_time:2>");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:54` | `PlaySound` | player method — `player.PlaySound("M8RD03");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:55` | `PlayMessage` | player method — `player.PlayMessage("Sorry you don't have much\|in the way of weapons, old chap.<key_time:3> But it's frightfully important<key_time:2>you bring Wirake's tribesmen medical supplie...` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:59` | `PlaySound` | player method — `player.PlaySound("M8RD04");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:60` | `PlayMessage` | player method — `player.PlayMessage("Pay careful attention to my directions,\|sport.<key_time:3> You take a wrong turn on this trail,<key_time:2> and it's curtains for you.<key_time:2>");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:77` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:78` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:96` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE1.SCR:97` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:38` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:39` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:56` | `SetTaskParam` | task target — `tribesman1.SetTaskParam("target_cutscene", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:71` | `SetTaskParam` | task target — `tribesman1.SetTaskParam("target_cutscene", "hunter");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:102` | `PlaySound` | player method — `player.PlaySound("M8RD16");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:103` | `PlayMessage` | player method — `player.PlayMessage("Are you there? <key_time:1> This is curious. <key_time:2> We're seeing a lot of smoke in your vicinity. <key_time:2> If there's a brush fire,\|you better get ...` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:107` | `PlaySound` | player method — `player.PlaySound("M8RD17");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:108` | `PlayMessage` | player method — `player.PlayMessage("You're going to need to guide \|the natives back with you,<key_time:3> and for heaven's sakes,\|keep an eye out for stampeding animals.<key_time:3> They don't ...` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:135` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:136` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:153` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:154` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:42` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:43` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:57` | `PlaySound` | player method — `player.PlaySound("M8RD18");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:58` | `PlayMessage` | player method — `player.PlayMessage("Whew! You made it!<key_time:1> I didn't have any doubts.<key_time:2>");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:61` | `PlaySound` | player method — `player.PlaySound("M8RD19");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:62` | `PlayMessage` | player method — `player.PlayMessage("And I also have no doubt that fire\| was set on purpose.<key_time:4> We best watch our step, henchforth.<key_time:2>");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:65` | `PlaySound` | player method — `player.PlaySound("M8RD20");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:66` | `PlayMessage` | player method — `player.PlayMessage("At least we'll have Wirake\|with us as our guide.<key_time:3> He agreed to accompany us\|as thanks for rescuing his friends! <key_time:3>");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:69` | `PlaySound` | player method — `player.PlaySound("M8RD21");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:70` | `PlayMessage` | player method — `player.PlayMessage("So what are we waiting for? <key_time:2> Let's get this show on the road!<key_time:1>");` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:95` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:96` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:111` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:112` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE4.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE4.SCR:40` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE4.SCR:41` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE4.SCR:68` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE4.SCR:69` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE4.SCR:87` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE4.SCR:88` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE5.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE5.SCR:47` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE5.SCR:48` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE5.SCR:75` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE5.SCR:76` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE5.SCR:94` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP8 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE5.SCR:95` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP8 |
| `re/extracted/SCRIPTS/LEVELS/MAP9/WORLD.SCR:76` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP9 |
| `re/extracted/SCRIPTS/LEVELS/MAP9/WORLD.SCR:113` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "knife1_1st");` | MAP9 |
| `re/extracted/SCRIPTS/LEVELS/MAP9/WORLD.SCR:115` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "handgun2_1st");` | MAP9 |
| `re/extracted/SCRIPTS/LEVELS/MAP9/WORLD.SCR:116` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "mbranch_1st");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:16` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:18` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:48` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:57` | `SetAIType` | trigger tests player — `::globals.reggie9.SetAIType(AITYPE_COMPANION,"hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:76` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:88` | `SetTaskParam` | task target — `::globals.dingo1.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:92` | `SetTaskParam` | task target — `::globals.dingo2.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:94` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:111` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:128` | `SetTaskParam` | task target — `::globals.dingo1.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:132` | `SetTaskParam` | task target — `::globals.dingo2.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:134` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:148` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:166` | `SetTaskParam` | task target — `::globals.dingo1.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:170` | `SetTaskParam` | task target — `::globals.dingo2.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:180` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:198` | `SetTaskParam` | task target — `::globals.dingo1.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:202` | `SetTaskParam` | task target — `::globals.dingo2.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:206` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:217` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:230` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:244` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:254` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:262` | `SetAIType` | trigger tests player — `::globals.reggie9.SetAIType(AITYPE_COMPANION,"hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:268` | `SetTaskParam` | task target — `::globals.dingo1.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:273` | `SetTaskParam` | task target — `::globals.dingo2.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:278` | `SetTaskParam` | task target — `::globals.dingo3.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:296` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:329` | `SetTaskParam` | task target — `::globals.reggie9.SetTaskParam("target_cutscene", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:352` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:362` | `StartTask` | task target — `::globals.player.StartTask("6");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:373` | `SetTaskParam` | task target — `::globals.dingo1.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:377` | `SetTaskParam` | task target — `::globals.dingo2.SetTaskParam("target", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:390` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:427` | `SetTaskParam` | task target — `::globals.reggie9.SetTaskParam("target_cutscene", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:459` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:468` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:478` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:487` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:497` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:506` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:516` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:525` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:534` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP9 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:544` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("map9_envStream1", "hunter");` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:37` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:38` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:74` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:75` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:94` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:95` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:42` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:43` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:78` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:79` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:97` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:98` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE3.SCR:24` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE3.SCR:38` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE3.SCR:39` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE3.SCR:72` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE3.SCR:73` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE3.SCR:91` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE3.SCR:92` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE4.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE4.SCR:40` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE4.SCR:41` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE4.SCR:66` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE4.SCR:67` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE4.SCR:85` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP9 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE4.SCR:86` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP9 |
| `re/extracted/SCRIPTS/LEVELS/MAP10/WORLD.SCR:82` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP10 |
| `re/extracted/SCRIPTS/LEVELS/MAP10/WORLD.SCR:84` | `SetMaxKnifeDistance` | player method — `player.SetMaxKnifeDistance(2.0);` | MAP10 |
| `re/extracted/SCRIPTS/LEVELS/MAP10/WORLD.SCR:185` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "dbrifle1_1st");` | MAP10 |
| `re/extracted/SCRIPTS/LEVELS/MAP10/WORLD.SCR:186` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "macheta_1st");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:31` | `SetAIType` | trigger tests player — `::globals.reggie10.SetAIType(AITYPE_COMPANION\|AITYPE_COVERSTAY,"hunter"); //crouch_sau_lie` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:34` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:71` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:89` | `SetTaskParam` | task target — `::globals.puma4.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:95` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:111` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:134` | `SetTaskParam` | task target — `::globals.puma1.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:144` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:165` | `SetTaskParam` | task target — `::globals.puma2.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:167` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:177` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:204` | `SetTaskParam` | task target — `::globals.wildboar5.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:208` | `SetTaskParam` | task target — `::globals.wildboar6.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:231` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:256` | `SetTaskParam` | task target — `::globals.wildboar1.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:260` | `SetTaskParam` | task target — `::globals.wildboar2.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:262` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:273` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:301` | `SetTaskParam` | task target — `::globals.wildboar3.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:305` | `SetTaskParam` | task target — `::globals.wildboar4.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:307` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:317` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:341` | `SetTaskParam` | task target — `::globals.wildboar1.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:345` | `SetTaskParam` | task target — `::globals.wildboar2.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:347` | `StartTask` | task target — `::globals.player.StartTask("6");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:356` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:365` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:371` | `SetTaskParam` | task target — `::globals.puma3.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:383` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:405` | `SetTaskParam` | task target — `::globals.puma1.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:411` | `StartTask` | task target — `::globals.player.StartTask("7");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:424` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:455` | `SetTaskParam` | task target — `::globals.wildboar1.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:459` | `SetTaskParam` | task target — `::globals.wildboar2.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:461` | `StartTask` | task target — `::globals.player.StartTask("8");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:470` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:497` | `SetAIType` | trigger tests player — `::globals.wirake10.SetAIType(AITYPE_COMPANION,"hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:501` | `StartTask` | task target — `::globals.player.StartTask("9");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:518` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:527` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:538` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:554` | `SetTaskParam` | task target — `::globals.wildboar1.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:558` | `SetTaskParam` | task target — `::globals.wildboar2.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:562` | `SetTaskParam` | task target — `::globals.wildboar3.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:564` | `StartTask` | task target — `::globals.player.StartTask("10");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:574` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:590` | `SetTaskParam` | task target — `::globals.puma4.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:603` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:619` | `SetTaskParam` | task target — `::globals.puma4.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:630` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:641` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:651` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:664` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:670` | `SetTaskParam` | task target — `::globals.wildboar1.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:689` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:710` | `SetTaskParam` | task target — `::globals.puma3.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:720` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:747` | `SetTaskParam` | task target — `::globals.puma1.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:751` | `SetTaskParam` | task target — `::globals.puma2.SetTaskParam("target", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:754` | `StartTask` | task target — `::globals.player.StartTask("11");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:777` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:800` | `SetAIType` | trigger tests player — `::globals.reggie10.SetAIType(AITYPE_COMPANION\|AITYPE_COVERSTAY,"hunter"); //crouch_sau_lie` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:817` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP10 |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:826` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("suspense", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:54` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:55` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:108` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:109` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:127` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:128` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE2.SCR:24` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE2.SCR:68` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE2.SCR:69` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE2.SCR:106` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE2.SCR:107` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE2.SCR:128` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE2.SCR:129` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE3.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE3.SCR:40` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE3.SCR:41` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE3.SCR:66` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE3.SCR:67` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE3.SCR:85` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE3.SCR:86` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE4.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE4.SCR:46` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE4.SCR:47` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE4.SCR:101` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE4.SCR:102` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE4.SCR:122` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE4.SCR:123` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE5.SCR:33` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE5.SCR:87` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE5.SCR:88` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE5.SCR:119` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE5.SCR:120` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE5.SCR:141` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE5.SCR:142` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE6.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE6.SCR:26` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE6.SCR:27` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE6.SCR:42` | `SetTaskParam` | task target — `reggie10.SetTaskParam("target_cutscene", "hunter");` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE6.SCR:72` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE6.SCR:73` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE6.SCR:88` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP10 |
| `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE6.SCR:89` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP10 |
| `re/extracted/SCRIPTS/LEVELS/MAP11A/WORLD.SCR:81` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP11A |
| `re/extracted/SCRIPTS/LEVELS/MAP11A/WORLD.SCR:117` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "rifle3_1st");` | MAP11A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:20` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP11A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:44` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("M11RD01", "hunter");` | MAP11A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:66` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP11A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:75` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:103` | `PlaySoundEffect` | trigger tests player — `PlaySoundEffect("M11RD05", "hunter");` | MAP11A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:121` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP11A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:130` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:196` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP11A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:206` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:230` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:238` | `PlayHint` | player method — `::globals.player.PlayHint("You don't have enough ammo to finish the mission.");` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE1.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE1.SCR:34` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE1.SCR:35` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE1.SCR:44` | `PlayMessage` | player method — `player.PlayMessage("Well, chap, it's not like you need the practice,<key_time:2>but a sharp shooting refresher couldn't hurt,\|now could it?<key_time:3>Not when you think about w...` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE1.SCR:60` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE1.SCR:61` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE1.SCR:78` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE1.SCR:79` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE2.SCR:24` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE2.SCR:38` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE2.SCR:39` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE2.SCR:64` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE2.SCR:65` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE2.SCR:85` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE2.SCR:86` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE3.SCR:32` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE3.SCR:45` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE3.SCR:46` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE3.SCR:59` | `PlayMessage` | player method — `player.PlayMessage("Oh dear. We're running dreadfully short on ammo.<key_time:3>It's entirely possible I brought\|the wrong caliber.<key_time:4>");` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE3.SCR:63` | `PlayMessage` | player method — `player.PlayMessage("Not to worry, chap.\|There's a supply depot nearby.<key_time:3>");` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE3.SCR:67` | `PlayMessage` | player method — `player.PlayMessage("Of course, the problem is getting there, isn't it?<key_time:3>");` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE3.SCR:84` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE3.SCR:85` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE3.SCR:106` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE3.SCR:107` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11A |
| `re/extracted/SCRIPTS/LEVELS/MAP11B/WORLD.SCR:66` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP11B |
| `re/extracted/SCRIPTS/LEVELS/MAP11B/WORLD.SCR:98` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "dbrifle1_1st");` | MAP11B |
| `re/extracted/SCRIPTS/LEVELS/MAP11B/WORLD.SCR:99` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "handgun3_1st");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:20` | `AddItemAmmo` | inventory/weapon — `::globals.player.AddItemAmmo("ammo_dbrifle1_1st", -35);` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:21` | `AddItemAmmo` | inventory/weapon — `::globals.player.AddItemAmmo("ammo_handgun3_1st", -10);` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:26` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:41` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:49` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:63` | `SetTaskParam` | task target — `::globals.wildboar2.SetTaskParam("target", "hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:77` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:96` | `SetTaskParam` | task target — `::globals.wildboar5.SetTaskParam("target", "hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:102` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:111` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:131` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:148` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:158` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:169` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:201` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:219` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:237` | `SetTaskParam` | task target — `::globals.wildboar2.SetTaskParam("target", "hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:245` | `StartTask` | task target — `::globals.player.StartTask("6");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:254` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:263` | `StartTask` | task target — `::globals.player.StartTask("7");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:280` | `SetTaskParam` | task target — `::globals.wildboar5.SetTaskParam("target", "hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:290` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:309` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:34` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:35` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:63` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:64` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:81` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:82` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE2.SCR:24` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE2.SCR:45` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE2.SCR:46` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE2.SCR:77` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE2.SCR:78` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE2.SCR:97` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE2.SCR:98` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11B |
| `re/extracted/SCRIPTS/LEVELS/MAP11C/WORLD.SCR:73` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP11C |
| `re/extracted/SCRIPTS/LEVELS/MAP11C/WORLD.SCR:98` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "rifle3_1st");` | MAP11C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:18` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP11C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:56` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP11C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:65` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:107` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP11C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:116` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:144` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP11C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:193` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:212` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP11C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:224` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:248` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP11C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:256` | `PlayHint` | player method — `::globals.player.PlayHint("You don't have enough ammo to finish the mission.");` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE1.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE1.SCR:34` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE1.SCR:35` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE1.SCR:58` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE1.SCR:59` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE1.SCR:76` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE1.SCR:77` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE2.SCR:26` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE2.SCR:42` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE2.SCR:43` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE2.SCR:69` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE2.SCR:70` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE2.SCR:90` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE2.SCR:91` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE3.SCR:31` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE3.SCR:45` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE3.SCR:46` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE3.SCR:58` | `PlayMessage` | player method — `player.PlayMessage("You're thinning 'em out, chap.\|Good show.<key_time:2>");` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE3.SCR:75` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE3.SCR:76` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE3.SCR:97` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP11C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE3.SCR:98` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP11C |
| `re/extracted/SCRIPTS/LEVELS/MAP12A/WORLD.SCR:23` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_CAN_CROUCH);` | MAP12A |
| `re/extracted/SCRIPTS/LEVELS/MAP12A/WORLD.SCR:132` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP12A |
| `re/extracted/SCRIPTS/LEVELS/MAP12A/WORLD.SCR:169` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "handgun3_1st");` | MAP12A |
| `re/extracted/SCRIPTS/LEVELS/MAP12A/WORLD.SCR:170` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "shotgun1_1st");` | MAP12A |
| `re/extracted/SCRIPTS/LEVELS/MAP12A/WORLD.SCR:171` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "mbranch_1st");` | MAP12A |
| `re/extracted/SCRIPTS/LEVELS/MAP12A/WORLD.SCR:218` | `SetPFlags` | player flags — `::globals.player.SetPFlags(PLAYER_CAN_CROUCH);` | MAP12A |
| `re/extracted/SCRIPTS/LEVELS/MAP12A/WORLD.SCR:222` | `GetDistBetween2Ais` | distance check next if: if (fDistance >::globals. fRadiusOff) — `fDistance = GetDistBetween2Ais(::globals.szStealthCenter, "hunter");` | MAP12A |
| `re/extracted/SCRIPTS/LEVELS/MAP12A/WORLD.SCR:241` | `ClearPFlags` | player flags — `::globals.player.ClearPFlags(PLAYER_CAN_CROUCH);` | MAP12A |
| `re/extracted/SCRIPTS/LEVELS/MAP12A/WORLD.SCR:245` | `GetDistBetween2Ais` | distance check next if: if (fDistance < ::globals.fRadiusOn) — `fDistance = GetDistBetween2Ais(::globals.szStealthCenter, "hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:9` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:24` | `AddItemAmmo` | inventory/weapon — `::globals.player.AddItemAmmo("ammo_handgun3_1st", 30);` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:36` | `SetAIType` | trigger tests player — `::globals.reggie12a.SetAIType(AITYPE_COMPANION,"hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:51` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:63` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:74` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:80` | `StartTask` | task target — `::globals.player.StartTask("2");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:88` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:104` | `SetTaskParam` | task target — `::globals.pbear.SetTaskParam("target", "hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:106` | `StartTask` | task target — `::globals.player.StartTask("3");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:119` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:132` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:147` | `StartTask` | task target — `::globals.player.StartTask("4");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:157` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:177` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:180` | `StartTask` | task target — `::globals.player.StartTask("5");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:231` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:251` | `PlayHint` | player method — `::globals.player.PlayHint("You must proceed without Reginald.");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:290` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:300` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:310` | `StartTask` | task target — `::globals.player.StartTask("6");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:325` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:344` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:350` | `DisableStimulus` | player method — `::globals.player.DisableStimulus();//disables player stimulus on animals` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:368` | `SetStealthLevels` | player method — `::globals.player.SetStealthLevels(0.2, 0.25, 0.30, 0.35);` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:389` | `SetTaskParam` | task target — `::globals.wolf5.SetTaskParam("target", "hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:396` | `SetTaskParam` | task target — `::globals.wolf6.SetTaskParam("target", "hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:407` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:427` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:440` | `SetTaskParam` | task target — `::globals.wolf3.SetTaskParam("target", "hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:444` | `SetTaskParam` | task target — `::globals.wolf4.SetTaskParam("target", "hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:446` | `StartTask` | task target — `::globals.player.StartTask("7");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:455` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:476` | `SetTaskParam` | task target — `::globals.pbear.SetTaskParam("target", "hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:478` | `StartTask` | task target — `::globals.player.StartTask("8");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:540` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:559` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:578` | `PlayHint` | player method — `::globals.player.PlayHint("You must proceed without Wirake.");` | MAP12A |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:610` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:39` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:40` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:87` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:88` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:106` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:107` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE2.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE2.SCR:41` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE2.SCR:42` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE2.SCR:83` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE2.SCR:84` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE2.SCR:114` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE2.SCR:115` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE3.SCR:21` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE3.SCR:37` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE3.SCR:38` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE3.SCR:79` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE3.SCR:80` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE3.SCR:97` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE3.SCR:98` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE4.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE4.SCR:44` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE4.SCR:45` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE4.SCR:85` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE4.SCR:86` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE4.SCR:115` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE4.SCR:116` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE5.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE5.SCR:42` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE5.SCR:43` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE5.SCR:82` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE5.SCR:83` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE5.SCR:103` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE5.SCR:104` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE6.SCR:22` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE6.SCR:41` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE6.SCR:42` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE6.SCR:62` | `MoveToPos` | teleport/move — `player.MoveToPos("c5_2");` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE6.SCR:77` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE6.SCR:78` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE6.SCR:99` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE6.SCR:100` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE7.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE7.SCR:40` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE7.SCR:41` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE7.SCR:66` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE7.SCR:67` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE7.SCR:85` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12A |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE7.SCR:86` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12A |
| `re/extracted/SCRIPTS/LEVELS/MAP12B/WORLD.SCR:59` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP12B |
| `re/extracted/SCRIPTS/LEVELS/MAP12B/WORLD.SCR:91` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "handgun3_1st");` | MAP12B |
| `re/extracted/SCRIPTS/LEVELS/MAP12B/WORLD.SCR:92` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "rifle2_1st");` | MAP12B |
| `re/extracted/SCRIPTS/LEVELS/MAP12B/WORLD.SCR:93` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "mbranch_1st");` | MAP12B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12B.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12B.SCR:24` | `SetTaskParam` | task target — `::globals.pbear.SetTaskParam("target", "hunter");` | MAP12B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12B.SCR:26` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP12B |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12B.SCR:41` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12B/CUTSCENE1.SCR:23` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP12B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12B/CUTSCENE1.SCR:40` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP12B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12B/CUTSCENE1.SCR:41` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP12B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12B/CUTSCENE1.SCR:66` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12B/CUTSCENE1.SCR:67` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12B/CUTSCENE1.SCR:85` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12B |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12B/CUTSCENE1.SCR:86` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12B |
| `re/extracted/SCRIPTS/LEVELS/MAP12C/WORLD.SCR:50` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP12C |
| `re/extracted/SCRIPTS/LEVELS/MAP12C/WORLD.SCR:81` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "shotgun1_1st");` | MAP12C |
| `re/extracted/SCRIPTS/LEVELS/MAP12C/WORLD.SCR:82` | `AiCanFindItem` | inventory/weapon — `AiCanFindItem("hunter", "mbranch_1st");` | MAP12C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12C.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12C.SCR:20` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAP12C |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12C.SCR:35` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAP12C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12C/CUTSCENE1.SCR:25` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAP12C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12C/CUTSCENE1.SCR:46` | `SetPFlags` | player flags — `player.SetPFlags(PLAYER_INCUTSCENE);` | MAP12C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12C/CUTSCENE1.SCR:47` | `ActivateCutSceneMode` | camera/cutscene — `player.ActivateCutSceneMode();` | MAP12C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12C/CUTSCENE1.SCR:86` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12C/CUTSCENE1.SCR:87` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12C/CUTSCENE1.SCR:107` | `ClearPFlags` | player flags — `player.ClearPFlags(PLAYER_INCUTSCENE);` | MAP12C |
| `re/extracted/SCRIPTS/CUTSCENE/MAP12C/CUTSCENE1.SCR:108` | `DeactivateCutSceneMode` | camera/cutscene — `player.DeactivateCutSceneMode();` | MAP12C |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:120` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:122` | `SetMaxKnifeDistance` | player method — `player.SetMaxKnifeDistance(2.0);` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:190` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "macheta_1st");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:191` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "handgun3_1st");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:192` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "dbrifle1_1st");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:314` | `SetTaskParam` | task target — `::globals.leopard1.SetTaskParam("target", "hunter");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:322` | `SetTaskParam` | task target — `::globals.leopard2.SetTaskParam("target", "hunter");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:359` | `SetTaskParam` | task target — `::globals.bull1.SetTaskParam("target", "hunter");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:367` | `SetTaskParam` | task target — `::globals.bull2.SetTaskParam("target", "hunter");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:404` | `SetTaskParam` | task target — `::globals.wolf1.SetTaskParam("target", "hunter");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:412` | `SetTaskParam` | task target — `::globals.wolf2.SetTaskParam("target", "hunter");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:420` | `SetTaskParam` | task target — `::globals.wolf3.SetTaskParam("target", "hunter");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:502` | `SetTaskParam` | task target — `::globals.lion1.SetTaskParam("target", "hunter");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:510` | `SetTaskParam` | task target — `::globals.lion2.SetTaskParam("target", "hunter");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:548` | `SetTaskParam` | task target — `::globals.grizzly1.SetTaskParam("target", "hunter");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:586` | `SetTaskParam` | task target — `::globals.rhino1.SetTaskParam("target", "hunter");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:623` | `GetHealth` | damage/health — `if (0.4>::globals.player.GetHealth())` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:693` | `PlayHint` | player method — `case 0: {::globals.player.PlayHint("WELL DONE!\nTwo more rounds to go."); break;}` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:694` | `PlayHint` | player method — `case 1: {::globals.player.PlayHint("MOST IMPRESSIVE!\nLast round."); break;}` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:695` | `UpdateMissionStatus, Pause, EndArena, PlayHint` | player method — `case 2: {::globals.player.PlayHint("PERFECT!"); UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);Pause(1.0);EndArena(); break;}` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:794` | `SetRecoverSpeed` | player method — `::globals.player.SetRecoverSpeed(fArenaRecoverSpeed);` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:795` | `SetTimeSelfRecoverDisabled` | player method — `::globals.player.SetTimeSelfRecoverDisabled(fArenaRDisabledTime);` | MAPARENA |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMAPARENA.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAPARENA |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMAPARENA.SCR:15` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAPARENA |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:64` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAPISLAND |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:127` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "tranqgun1_1st");` | MAPISLAND |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:206` | `SetTaskParam` | task target — `::globals.btiger1.SetTaskParam("target", "hunter");` | MAPISLAND |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:216` | `SetTaskParam` | task target — `::globals.btiger2.SetTaskParam("target", "hunter");` | MAPISLAND |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:236` | `GetHealth` | damage/health — `if (0.4>::globals.player.GetHealth())` | MAPISLAND |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:292` | `PlayHint` | player method — `case 0: {::globals.player.PlayHint("YOU'RE A SURVIVOR!\nFour more rounds to go."); nRoundCount=2; break;}` | MAPISLAND |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:293` | `PlayHint` | player method — `case 1: {::globals.player.PlayHint("GOOD JOB!\nThree more rounds to go."); nRoundCount=3; break;}` | MAPISLAND |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:294` | `PlayHint` | player method — `case 2: {::globals.player.PlayHint("ALMOST THERE!\nTwo more rounds to go."); nRoundCount=3; break;}` | MAPISLAND |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:295` | `PlayHint` | player method — `case 3: {::globals.player.PlayHint("EXCELLENT!\nLast round."); nRoundCount=6; break;}` | MAPISLAND |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:296` | `Pause, UpdateMissionStatus, Pause, EndArena, PlayHint` | player method — `case 4: {::globals.player.PlayHint("PERFECT!"); Pause(2.0);UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);Pause(1.0);EndArena(); break;}` | MAPISLAND |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:343` | `SetRecoverSpeed` | player method — `::globals.player.SetRecoverSpeed(fArenaRecoverSpeed);` | MAPISLAND |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:344` | `SetTimeSelfRecoverDisabled` | player method — `::globals.player.SetTimeSelfRecoverDisabled(fArenaRDisabledTime);` | MAPISLAND |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMAPISLAND.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAPISLAND |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMAPISLAND.SCR:15` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAPISLAND |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMAPISLAND.SCR:31` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAPISLAND |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMAPISLAND.SCR:63` | `Hurt` | damage/health — `::globals.player.Hurt(-1, 2, 0);` | MAPISLAND |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:71` | `setidx, FindEntScrObject` | bind player entity — `setidx(player, FindEntScrObject("hunter"));` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:73` | `SetMaxKnifeDistance` | player method — `player.SetMaxKnifeDistance(2.0);` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:135` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "macheta_1st");` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:136` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "handgun3_1st");` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:137` | `EquipAIWith` | inventory/weapon — `EquipAIWith("hunter", "dbrifle1_1st");` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:222` | `SetTaskParam` | task target — `::globals.lion1.SetTaskParam("target", "hunter");` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:235` | `SetTaskParam` | task target — `::globals.lioness1.SetTaskParam("target", "hunter");` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:246` | `GetHealth` | damage/health — `if (0.4>::globals.player.GetHealth())` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:312` | `PlayHint` | player method — `case 0: {::globals.player.PlayHint("YOU'RE A SURVIVOR!\nFour more rounds to go."); nRoundCount=2; break;}` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:313` | `PlayHint` | player method — `case 1: {::globals.player.PlayHint("GOOD JOB!\nThree more rounds to go."); nRoundCount=3; break;}` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:314` | `PlayHint` | player method — `case 2: {::globals.player.PlayHint("ALMOST THERE!\nTwo more rounds to go."); nRoundCount=3; break;}` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:315` | `PlayHint` | player method — `case 3: {::globals.player.PlayHint("EXCELLENT!\nLast round."); nRoundCount=6; break;}` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:316` | `Pause, UpdateMissionStatus, Pause, EndArena, PlayHint` | player method — `case 4: {::globals.player.PlayHint("PERFECT!"); Pause(2.0);UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);Pause(1.0);EndArena(); break;}` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:371` | `SetRecoverSpeed` | player method — `::globals.player.SetRecoverSpeed(fArenaRecoverSpeed);` | MAPTSAVO |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:372` | `SetTimeSelfRecoverDisabled` | player method — `::globals.player.SetTimeSelfRecoverDisabled(fArenaRDisabledTime);` | MAPTSAVO |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMAPTSAVO.SCR:6` | `ActivateBy` | trigger tests player — `ActivateBy("hunter");` | MAPTSAVO |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMAPTSAVO.SCR:15` | `StartTask` | task target — `::globals.player.StartTask("1");` | MAPTSAVO |

## 5. Cutscenes and QTE/melee

Cutscene `.scr` files on disk under `re/extracted/SCRIPTS/CUTSCENE`: 83. Files whose text contains `state Skip`: 83.

Calls inside those cutscene files (every identifier call, including ones scripts also define), top of the list by count, plus every player-method call:

| Call in CUTSCENE\* | Count |
|---|---:|
| `setidx` | 397 |
| `FindEntScrObject` | 397 |
| `SetTaskParam` | 264 |
| `setstate` | 252 |
| `StartTask` | 209 |
| `StopTask` | 197 |
| `Update` | 174 |
| `Begin` | 167 |
| `End` | 166 |
| `DeactivateHUD` | 166 |
| `ClearPFlags` | 166 |
| `DeactivateCutSceneMode` | 166 |
| `MoveToPos` | 160 |
| `Wait` | 153 |
| `WaitForTask` | 143 |
| `GetScrObjIdx` | 143 |
| `EndCamera` | 136 |
| `PlayMessage` | 135 |
| `ForceBrainActivate` | 125 |
| `PlaySound` | 102 |
| `StopAllTasks` | 96 |
| `SetAnim` | 92 |
| `Activate` | 83 |
| `SetPFlags` | 83 |
| `ActivateCutSceneMode` | 83 |
| `ActivateHUD` | 83 |
| `trace` | 83 |
| `BeginCamera` | 67 |
| `StopAllTask` | 54 |
| `PlaySoundEffect` | 32 |
| `DenyTask` | 24 |
| `Kill` | 22 |
| `SetAttribute` | 20 |
| `ConfigTaskCmd` | 19 |
| `DeactivateWeaponIn` | 13 |
| `MakeVisible` | 12 |
| `BSMGoToState` | 12 |
| `ActivateWeaponIn` | 11 |
| `SetAIType` | 9 |
| `isvalid` | 9 |

Player methods called as `player.<method>` inside cutscene files:

| Method | Count | Example |
|---|---:|---|
| `ClearPFlags` | 166 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:60` `player.ClearPFlags(PLAYER_INCUTSCENE);` |
| `DeactivateCutSceneMode` | 166 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:61` `player.DeactivateCutSceneMode();` |
| `SetPFlags` | 83 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:35` `player.SetPFlags(PLAYER_INCUTSCENE);` |
| `ActivateCutSceneMode` | 83 | `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:36` `player.ActivateCutSceneMode();` |
| `PlayMessage` | 15 | `re/extracted/SCRIPTS/CUTSCENE/MAP11A/CUTSCENE1.SCR:44` `player.PlayMessage("Well, chap, it's not like you need the practice,<key_time:2>but a sharp shooting refresher couldn't hurt,\|now could it?<key_time:3>Not when you think about w...` |
| `PlaySound` | 11 | `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:49` `player.PlaySound("wolf_howl_stream");` |
| `TakeItem` | 8 | `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE4.SCR:112` `player.TakeItem("rifle1_1st");` |
| `MoveToPos` | 4 | `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE6.SCR:62` `player.MoveToPos("c5_2");` |
| `GetInCar` | 2 | `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:112` `player.GetInCar(2);` |

How a cutscene is entered from outside `CUTSCENE` (calls to `ExecuteScript` in story/bonus closures):

| Map | File:line | Call |
|---|---|---|
| MAP1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:39` | `ExecuteScript("cScrCutScene1");` |
| MAP1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:82` | `ExecuteScript("cScrCutScene2");` |
| MAP1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:221` | `ExecuteScript("cScrCutScene3");` |
| MAP1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:277` | `ExecuteScript("cScrCutScene4");` |
| MAP1 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:358` | `ExecuteScript("cScrCutScene5");` |
| MAP2A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:57` | `ExecuteScript("cScrCutScene1");` |
| MAP2A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:100` | `ExecuteScript("cScrCutScene6");` |
| MAP2A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:136` | `ExecuteScript("cScrCutScene2");` |
| MAP2B | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:228` | `ExecuteScript("cScrCutScene1");` |
| MAP2B | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:322` | `ExecuteScript("cScrCutScene2");` |
| MAP2B | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:683` | `ExecuteScript("cScrCutScene3");` |
| MAP2C | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:107` | `ExecuteScript("cScrCutScene1");` |
| MAP2C | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:135` | `ExecuteScript("cScrCutScene2");` |
| MAP2C | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:250` | `ExecuteScript("cScrCutScene4");` |
| MAP3A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:39` | `ExecuteScript("cScrCutScene1");` |
| MAP3A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:63` | `ExecuteScript("cScrCutScene3");` |
| MAP3A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:441` | `ExecuteScript("cScrCutScene4");` |
| MAP3A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:469` | `ExecuteScript("cScrCutScene2");` |
| MAP3B | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:35` | `ExecuteScript("cScrCutScene1");` |
| MAP3C | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:103` | `ExecuteScript("cScrCutScene1");` |
| MAP3C | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:130` | `ExecuteScript("cScrCutScene3");` |
| MAP3C | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:168` | `ExecuteScript("cScrCutScene2");` |
| MAP4A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:28` | `ExecuteScript("cScrCutScene1");` |
| MAP4A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:269` | `ExecuteScript("cScrCutScene2");` |
| MAP4B | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:179` | `ExecuteScript("cScrCutScene1");` |
| MAP4B | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:204` | `ExecuteScript("cScrCutScene2");` |
| MAP4B | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:497` | `ExecuteScript("cScrCutScene5");` |
| MAP4B | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:561` | `ExecuteScript("cScrCutScene6");` |
| MAP4B | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:624` | `ExecuteScript("cScrCutScene7");` |
| MAP5 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:49` | `ExecuteScript("cScrCutScene1");` |
| MAP5 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:278` | `ExecuteScript("cScrCutScene3");` |
| MAP5 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:614` | `ExecuteScript("cScrCutScene4");//"that was the last of them..."` |
| MAP6 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:41` | `ExecuteScript("cScrCutScene1");` |
| MAP6 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:88` | `ExecuteScript("cScrCutScene4");` |
| MAP6 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:156` | `ExecuteScript("cScrCutScene5");` |
| MAP6 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:221` | `ExecuteScript("cScrCutScene2");` |
| MAP6 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:334` | `ExecuteScript("cScrCutScene3");` |
| MAP7 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:50` | `ExecuteScript("cScrCutScene1");` |
| MAP7 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:158` | `ExecuteScript("cScrCutScene2");` |
| MAP7 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:474` | `ExecuteScript("cScrCutScene6");` |
| MAP8 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:223` | `ExecuteScript("cScrCutScene1");` |
| MAP8 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:251` | `ExecuteScript("cScrCutScene4");` |
| MAP8 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:394` | `ExecuteScript("cScrCutScene5");` |
| MAP8 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:523` | `ExecuteScript("cScrCutScene2");` |
| MAP8 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:954` | `ExecuteScript("cScrCutScene3");` |
| MAP9 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:65` | `ExecuteScript("cScrCutScene1");` |
| MAP9 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:99` | `ExecuteScript("cScrCutScene4");` |
| MAP9 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:138` | `ExecuteScript("cScrCutScene2");` |
| MAP9 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:285` | `ExecuteScript("cScrCutScene3");` |
| MAP10 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:61` | `ExecuteScript("cScrCutScene1");` |
| MAP10 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:100` | `ExecuteScript("cScrCutScene3");` |
| MAP10 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:215` | `ExecuteScript("cScrCutScene2");` |
| MAP10 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:507` | `ExecuteScript("cScrCutScene4");` |
| MAP10 | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:769` | `ExecuteScript("cScrCutScene5");` |
| MAP11A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:27` | `ExecuteScript("cScrCutScene1");` |
| MAP11A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:165` | `ExecuteScript("cScrCutScene2");` |
| MAP11A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:220` | `ExecuteScript("cScrCutScene3");` |
| MAP11B | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:33` | `ExecuteScript("cScrCutScene1");` |
| MAP11B | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:190` | `ExecuteScript("cScrCutScene2");` |
| MAP11C | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:23` | `ExecuteScript("cScrCutScene1");` |
| MAP11C | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:156` | `ExecuteScript("cScrCutScene2");` |
| MAP11C | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:238` | `ExecuteScript("cScrCutScene3");` |
| MAP12A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:55` | `ExecuteScript("cScrCutScene1");` |
| MAP12A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:111` | `ExecuteScript("cScrCutScene7");` |
| MAP12A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:219` | `ExecuteScript("cScrCutScene2");` |
| MAP12A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:277` | `ExecuteScript("cScrCutScene5");` |
| MAP12A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:314` | `ExecuteScript("cScrCutScene3");` |
| MAP12A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:526` | `ExecuteScript("cScrCutScene4");` |
| MAP12A | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:598` | `ExecuteScript("cScrCutScene6");` |
| MAP12B | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12B.SCR:33` | `ExecuteScript("cScrCutScene1");` |
| MAP12C | `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12C.SCR:27` | `ExecuteScript("cScrCutScene1");` |

Start/end pattern counted across cutscene files (a file counts if the call text appears outside comments):

| Marker | Cutscene files containing it |
|---|---:|
| `ExecuteScript` | 0 |
| `BeginCamera` | 64 |
| `EndCamera` | 64 |
| `ActivateCutSceneMode` | 83 |
| `DeactivateCutSceneMode` | 83 |
| `PLAYER_INCUTSCENE` | 83 |
| `SetPFlags` | 83 |
| `ClearPFlags` | 83 |
| `ActivateHUD` | 83 |
| `DeactivateHUD` | 83 |
| `m_bSkip` | 83 |

Quoted start (`re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR`): `player.SetPFlags(PLAYER_INCUTSCENE)`, `player.ActivateCutSceneMode()`, `c1cam1.BeginCamera()`, `trigger.ActivateHUD()` in `state Activate` / `Begin`. Quoted end in `End`: `trigger.DeactivateHUD()`, `player.ClearPFlags(PLAYER_INCUTSCENE)`, `player.DeactivateCutSceneMode()`, and `c1cam1.EndCamera()` in `Update` before `setstate(Inactive)`. The `Skip` state calls `EndCamera` and `StopAllTask`, then the same flag clear in `End`.

### QTE / struggle / melee / button / fight / knife

Case-insensitive search of every `.scr`/`.spl`/`.lua` under `re/extracted/SCRIPTS` for `QTE`, `Struggle`, `Melee`, `Button`, `Fight`, `CloseCombat`, `Knife`, `Mash`, `QuickTime`. `where` is `comment`, `string` (the match sits inside quotes), or `code`.

Hits: 22. In code: 14. In strings: 6. In comments: 2.

| File:line | Where | Match | Line |
|---|---|---|---|
| `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:53` | string | `knife` | `reggie9.PlayMessage("Oh, this is bleak.<key_time:1> This is just the sort of cave\|where dingoes like to roam. <key_time:4>It'd be nice if we had some sort of...` |
| `re/extracted/SCRIPTS/LEVELS/MAP10/WORLD.SCR:84` | code | `Knife` | `player.SetMaxKnifeDistance(2.0);` |
| `re/extracted/SCRIPTS/LEVELS/MAP3A/WORLD.SCR:88` | code | `Knife` | `player.SetMaxKnifeDistance(2.0);` |
| `re/extracted/SCRIPTS/LEVELS/MAP3B/WORLD.SCR:60` | code | `Knife` | `player.SetMaxKnifeDistance(2.0);` |
| `re/extracted/SCRIPTS/LEVELS/MAP3C/WORLD.SCR:72` | code | `Knife` | `player.SetMaxKnifeDistance(2.0);` |
| `re/extracted/SCRIPTS/LEVELS/MAP4A/WORLD.SCR:68` | code | `Knife` | `player.SetMaxKnifeDistance(1.3);` |
| `re/extracted/SCRIPTS/LEVELS/MAP4A/WORLD.SCR:109` | string | `knife` | `EquipAIWith("hunter", "knife1_1st");` |
| `re/extracted/SCRIPTS/LEVELS/MAP4B/WORLD.SCR:167` | string | `knife` | `EquipAIWith("hunter", "knife1_1st");` |
| `re/extracted/SCRIPTS/LEVELS/MAP9/WORLD.SCR:113` | string | `knife` | `EquipAIWith("hunter", "knife1_1st");` |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:122` | code | `Knife` | `player.SetMaxKnifeDistance(2.0);` |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:73` | code | `Knife` | `player.SetMaxKnifeDistance(2.0);` |
| `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/KNIFE/KNIFE.SCR:1` | code | `Knife` | `class cBodyKnife extends cBodyWeapon` |
| `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/KNIFE/KNIFE.SPL:1` | code | `knife` | `Scripts\Models\Items\Weapons\knife\knife.scr` |
| `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/MBRANCH/MBRANCH.SCR:1` | code | `Knife` | `class cBodyMbranch extends cBodyKnife` |
| `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/MBRANCH/MBRANCH.SCR:63` | code | `Knife` | `class cBodyMacheta extends cBodyKnife` |
| `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/MBRANCH/MBRANCH.SPL:1` | code | `Knife` | `Scripts\Models\Items\Weapons\Knife\knife.scr` |
| `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/MBRANCH/MBRANCHNPC.SCR:1895` | comment | `fight` | `// Pumafight state //` |
| `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/MBRANCH/MBRANCHNPC.SCR:1898` | code | `fight` | `state Pumafight` |
| `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/MBRANCH/MBRANCHNPC.SCR:1903` | string | `fight` | `SetAnim("cs_pumafight");` |
| `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/RADIO/RADIONPC.SCR:1895` | comment | `fight` | `// Pumafight state //` |
| `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/RADIO/RADIONPC.SCR:1898` | code | `fight` | `state Pumafight` |
| `re/extracted/SCRIPTS/MODELS/ITEMS/WEAPONS/RADIO/RADIONPC.SCR:1903` | string | `fight` | `SetAnim("cs_pumafight");` |

ExportFnc names matching the same words (native exists even if scripts do not call it):

- `SetMaxKnifeDistance` registrar `FUN_10053e80` `0x53e80` impl `0x4c090` parent `cScrPlayer` sig `—`

## 6. Mission/task flow per map

Tasks are the `CreateTask` … `EndCreateTask` blocks in `MISSIONS\*.SCR` (calls on `m_pMission` kept; blank and comment lines dropped). Completion and failure calls are every non-comment `CompleteTask`, `UncompleteTask`, `UpdateMissionStatus`, `GlobalsCheckMissionStatus`, `ActivateTriggerOnEnd`, `ActivateTriggerOnFail`, `SetCompleteCondition` in that map's `MAIN.SPJ` closure.

### MAP1

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:15` `SetMission(1);`
- Task blocks: 6
  - task 1 (starts line 18): 18:m_pMission.CreateTask(TASK_RACE); | 19:m_pMission.SetTaskId(1); | 20:m_pMission.BeginTaskMsg("mis1task1_1_0"); | 21:m_pMission.EndTaskMsg("mis1task1_1_1"); | 22:m_pMission.SetTaskTimeLimit(0); | 23:m_pMission.EndCreateTask();
  - task 2 (starts line 25): 25:m_pMission.CreateTask(TASK_FIND); | 26:m_pMission.SetTaskId(2); | 27:m_pMission.SetTaskObjToFind(AI_GUIDE_ID,1); | 28:m_pMission.BeginTaskMsg("mis1task2_2_0"); | 29:m_pMission.EndTaskMsg("mis1task2_2_1"); | 31:m_pMission.SetTaskTimeLimit(0); | 32:m_pMission.EndCreateTask();
  - task 3 (starts line 35): 35:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 36:m_pMission.SetTaskId(3); | 37:m_pMission.BeginTaskMsg("mis1task3_3_0"); | 38:m_pMission.EndTaskMsg("mis1task3_3_1"); | 39:m_pMission.SetTaskAnimalToShoot(TARGET_CAN, ANIMAL_KILL,4); | 40:m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE); | 41:m_pMission.ActivateTriggerOnEnd("cScrTask05"); | 42:m_pMission.EndCreateTask();
  - task 4 (starts line 45): 45:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 46:m_pMission.SetTaskId(4); | 47:m_pMission.IsBonusTask(); | 48:m_pMission.SetTaskAnimalToShoot(ANIMAL_WHITETAIL,ANIMAL_KILL,2); | 49:m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_LOSE_TARGET); | 50:m_pMission.SetCompleteCondition(NEED_ANIMAL_FOR_COMPLETE); | 51:m_pMission.SetTaskAvailableTarget(2); | 52:m_pMission.ActivateTriggerOnEnd("cScrTask10"); | 53:m_pMission.ActivateTriggerOnFail("cScrTask10"); | 54:m_pMission.EndCreateTask();
  - task 5 (starts line 56): 56:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 57:m_pMission.SetTaskId(5); | 58:m_pMission.BeginTaskMsg("mis1task4_4_0"); | 59:m_pMission.EndTaskMsg("mis1task4_4_1"); | 60:m_pMission.SetTaskAnimalToShoot(ANIMAL_MOUNTAINLION,ANIMAL_KILL,1); | 61:m_pMission.SetSoundToBePlayed("attack2"); | 62:m_pMission.EndCreateTask();
  - task 6 (starts line 64): 64:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 65:m_pMission.SetTaskId(6); | 66:m_pMission.BeginTaskMsg("mis1task5_5_0"); | 67:m_pMission.EndTaskMsg("mis1task5_5_1"); | 68:m_pMission.SetTaskAnimalToShoot(ANIMAL_BBEAR,ANIMAL_KILL,1); | 69:m_pMission.ActivateTriggerOnEnd("cScrTask09"); | 70:m_pMission.SetSoundToBePlayed("attack2"); | 71:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:81` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:126` `UpdateMissionStatus(MISSION_ACTION_FIND_SOMETHING, AI_GUIDE_ID);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:40` `m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:41` `m_pMission.ActivateTriggerOnEnd("cScrTask05");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:50` `m_pMission.SetCompleteCondition(NEED_ANIMAL_FOR_COMPLETE);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:52` `m_pMission.ActivateTriggerOnEnd("cScrTask10");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:53` `m_pMission.ActivateTriggerOnFail("cScrTask10");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION01.SCR:69` `m_pMission.ActivateTriggerOnEnd("cScrTask09");`

### MAP2A

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSION02A.SCR:14` `SetMission(2);`
- Task blocks: 6
  - task 1 (starts line 17): 17:m_pMission.CreateTask(TASK_RACE); | 18:m_pMission.SetTaskId(1); | 19:m_pMission.BeginTaskMsg("mis2task1_1_0"); | 20:m_pMission.SetTaskTimeLimit(0); | 21:m_pMission.EndCreateTask();
  - task 2 (starts line 32): 32:m_pMission.CreateTask(TASK_RACE); | 33:m_pMission.SetTaskId(2); | 34:m_pMission.BeginTaskMsg("mis2task2_2_0"); | 35:m_pMission.EndTaskMsg("mis2task2_2_1"); | 36:m_pMission.SetTaskTimeLimit(0); | 37:m_pMission.EndCreateTask();
  - task 3 (starts line 41): 41:m_pMission.CreateTask(TASK_RACE); | 42:m_pMission.SetTaskId(3); | 43:m_pMission.BeginTaskMsg("mis2task3_3_0"); | 44:m_pMission.EndTaskMsg("mis2task3_3_1"); | 45:m_pMission.SetTaskTimeLimit(0); | 46:m_pMission.EndCreateTask();
  - task 4 (starts line 50): 50:m_pMission.CreateTask(TASK_RACE); | 51:m_pMission.SetTaskId(4); | 52:m_pMission.BeginTaskMsg("mis2task4_4_0"); | 53:m_pMission.EndTaskMsg("mis2task4_4_1"); | 54:m_pMission.SetTaskTimeLimit(0); | 55:m_pMission.EndCreateTask();
  - task 5 (starts line 58): 58:m_pMission.CreateTask(TASK_SHOOT); | 59:m_pMission.SetTaskId(5); | 60:m_pMission.SetTaskAnimalToShoot(ANIMAL_GWOLF,ANIMAL_KILL,3); | 61:m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL); | 62:m_pMission.SetSoundToBePlayed("attack2"); | 63:m_pMission.ActivateTriggerOnEnd("cScrTask07"); | 64:m_pMission.EndCreateTask();
  - task 6 (starts line 67): 67:m_pMission.CreateTask(TASK_RACE); | 68:m_pMission.SetTaskId(6); | 69:m_pMission.SetTaskTimeLimit(0); | 70:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:75` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:160` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:262` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:294` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:328` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:379` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 6);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION02A.SCR:63` `m_pMission.ActivateTriggerOnEnd("cScrTask07");`

### MAP2B

- No `SetMission` call in the mission script.
- Task blocks: 8
  - task 1 (starts line 18): 18:m_pMission.CreateTask(TASK_RACE); | 19:m_pMission.SetTaskId(1); | 20:m_pMission.SetTaskTimeLimit(0); | 21:m_pMission.EndCreateTask();
  - task 2 (starts line 23): 23:m_pMission.CreateTask(TASK_SHOOT); | 24:m_pMission.SetTaskId(2); | 25:m_pMission.SetTaskAnimalToShoot(ANIMAL_GWOLF,ANIMAL_KILL,1); | 26:m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL); | 27:m_pMission.SetSoundToBePlayed("attack2"); | 28:m_pMission.EndCreateTask();
  - task 3 (starts line 31): 31:m_pMission.CreateTask(TASK_SHOOT); | 32:m_pMission.SetTaskId(3); | 33:m_pMission.SetTaskAnimalToShoot(ANIMAL_GBEAR,ANIMAL_KILL,1); | 34:m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL); | 35:m_pMission.ForceActivateTriggerOnEnd("cScrTask04"); | 36:m_pMission.SetSoundToBePlayed("attack3"); | 37:m_pMission.EndCreateTask();
  - task 4 (starts line 39): 39:m_pMission.CreateTask(TASK_RACE); | 40:m_pMission.SetTaskId(4); | 41:m_pMission.BeginTaskMsg("mis2task5_5_0"); | 42:m_pMission.EndTaskMsg("mis2task5_5_1"); | 43:m_pMission.SetTaskTimeLimit(0); | 44:m_pMission.EndCreateTask();
  - task 5 (starts line 47): 47:m_pMission.CreateTask(TASK_SHOOT); | 48:m_pMission.SetTaskId(5); | 49:m_pMission.SetTaskAnimalToShoot(ANIMAL_MOUNTAINLION,ANIMAL_KILL,1); | 50:m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL); | 51:m_pMission.SetSoundToBePlayed("attack2"); | 52:m_pMission.EndCreateTask();
  - task 6 (starts line 54): 54:m_pMission.CreateTask(TASK_SHOOT); | 55:m_pMission.SetTaskId(6); | 56:m_pMission.SetTaskAnimalToShoot(ANIMAL_GWOLF,ANIMAL_KILL,10); | 57:m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL); | 58:m_pMission.BeginTaskMsg("mis2task6_6_0"); | 59:m_pMission.EndTaskMsg("mis2task6_6_1"); | 60:m_pMission.ForceActivateTriggerOnEnd("cScrTask13"); | 61:m_pMission.SetSoundToBePlayed("attack_Hugh"); | 62:m_pMission.EndCreateTask();
  - task 7 (starts line 65): 65:m_pMission.CreateTask(TASK_RACE); | 66:m_pMission.SetTaskId(7); | 67:m_pMission.BeginTaskMsg("mis2task7_7_0"); | 68:m_pMission.EndTaskMsg("mis2task7_7_1"); | 69:m_pMission.SetTaskTimeLimit(0); | 70:m_pMission.EndCreateTask();
  - task 8 (starts line 73): 73:m_pMission.CreateTask(TASK_RACE); | 74:m_pMission.SetTaskId(8); | 75:m_pMission.SetTaskTimeLimit(0); | 76:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:186` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:643` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:990` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 7);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION02B.SCR:35` `m_pMission.ForceActivateTriggerOnEnd("cScrTask04");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION02B.SCR:60` `m_pMission.ForceActivateTriggerOnEnd("cScrTask13");`

### MAP2C

- No `SetMission` call in the mission script.
- Task blocks: 7
  - task 1 (starts line 18): 18:m_pMission.CreateTask(TASK_RACE); | 19:m_pMission.SetTaskId(1); | 20:m_pMission.EndTaskMsg("mis2task1_1_1"); | 21:m_pMission.SetTaskTimeLimit(0); | 22:m_pMission.EndCreateTask();
  - task 2 (starts line 25): 25:m_pMission.CreateTask(TASK_SHOOT); | 26:m_pMission.SetTaskId(2); | 27:m_pMission.SetTaskAnimalToShoot(ANIMAL_GWOLF,ANIMAL_KILL,2); | 28:m_pMission.SetSoundToBePlayed("attack2"); | 29:m_pMission.EndCreateTask();
  - task 3 (starts line 31): 31:m_pMission.CreateTask(TASK_RACE); | 32:m_pMission.SetTaskId(3); | 33:m_pMission.BeginTaskMsg("mis2task8_8_0"); | 34:m_pMission.EndTaskMsg("mis2task8_8_1"); | 35:m_pMission.SetTaskTimeLimit(0); | 36:m_pMission.EndCreateTask();
  - task 4 (starts line 38): 38:m_pMission.CreateTask(TASK_SHOOT); | 39:m_pMission.SetTaskId(4); | 40:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR,ANIMAL_KILL,1); | 41:m_pMission.BeginTaskMsg("mis2task9_9_0"); | 42:m_pMission.ForceActivateTriggerOnEnd("cScrTask07"); | 44:m_pMission.EndCreateTask();
  - task 5 (starts line 46): 46:m_pMission.CreateTask(TASK_RACE); | 47:m_pMission.SetTaskId(5); | 48:m_pMission.BeginTaskMsg("mis2task9_9_1"); | 49:m_pMission.SetTaskTimeLimit(0); | 50:m_pMission.EndCreateTask();
  - task 6 (starts line 53): 53:m_pMission.CreateTask(TASK_RACE); | 54:m_pMission.SetTaskId(6); | 55:m_pMission.BeginTaskMsg("mis2task10_10_0"); | 56:m_pMission.EndTaskMsg("mis2task10_10_1"); | 57:m_pMission.SetTaskTimeLimit(0); | 58:m_pMission.EndCreateTask();
  - task 7 (starts line 61): 61:m_pMission.CreateTask(TASK_SHOOT); | 62:m_pMission.SetTaskId(7); | 63:m_pMission.SetTaskAnimalToShoot(ANIMAL_GBEAR,ANIMAL_KILL,1); | 64:m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL); | 65:m_pMission.BeginTaskMsg("mis2task11_11_0"); | 66:m_pMission.EndTaskMsg("mis2task11_11_1"); | 67:m_pMission.ActivateTriggerOnEnd("cScrTask09"); | 68:m_pMission.SetSoundToBePlayed("attack2"); | 69:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:86` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION ,1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:105` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:122` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION ,3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:233` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 6);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:268` `CompleteTask(1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:269` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:270` `CompleteTask(3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:271` `CompleteTask(4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:272` `CompleteTask(5);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:273` `CompleteTask(6);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:274` `CompleteTask(7);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION02C.SCR:42` `m_pMission.ForceActivateTriggerOnEnd("cScrTask07");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION02C.SCR:67` `m_pMission.ActivateTriggerOnEnd("cScrTask09");`

### MAP3A

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSION03A.SCR:15` `SetMission(3);`
- Task blocks: 9
  - task 1 (starts line 19): 19:m_pMission.CreateTask(TASK_FIND); | 20:m_pMission.SetTaskId(1); | 21:m_pMission.SetTaskObjToFind(AI_GUIDE_ID,3); | 22:m_pMission.BeginTaskMsg("mis3task1_1_0"); | 23:m_pMission.EndTaskMsg("mis3task1_1_1"); | 24:m_pMission.ActivateTriggerOnEnd("cScrTask08"); | 25:m_pMission.SetTaskTimeLimit(0); | 26:m_pMission.EndCreateTask();
  - task 2 (starts line 28): 28:m_pMission.CreateTask(TASK_SHOOT); | 29:m_pMission.SetTaskId(2); | 30:m_pMission.SetTaskAnimalToShoot(ANIMAL_HYENA,ANIMAL_KILL,2); | 31:m_pMission.SetSoundToBePlayed("attack1"); | 32:m_pMission.EndCreateTask();
  - task 3 (starts line 34): 34:m_pMission.CreateTask(TASK_SHOOT); | 35:m_pMission.SetTaskId(3); | 36:m_pMission.SetTaskAnimalToShoot(ANIMAL_HYENA,ANIMAL_KILL,2); | 37:m_pMission.SetSoundToBePlayed("attack2"); | 38:m_pMission.EndCreateTask();
  - task 4 (starts line 40): 40:m_pMission.CreateTask(TASK_SHOOT); | 41:m_pMission.SetTaskId(4); | 42:m_pMission.SetTaskAnimalToShoot(ANIMAL_HYENA,ANIMAL_KILL,2); | 43:m_pMission.SetSoundToBePlayed("attack3"); | 44:m_pMission.EndCreateTask();
  - task 5 (starts line 46): 46:m_pMission.CreateTask(TASK_RACE); | 47:m_pMission.SetTaskId(5); | 48:m_pMission.BeginTaskMsg("mis3task2_2_0"); | 49:m_pMission.EndTaskMsg("mis3task2_2_1"); | 50:m_pMission.SetTaskTimeLimit(0); | 51:m_pMission.EndCreateTask();
  - task 6 (starts line 53): 53:m_pMission.CreateTask(TASK_SHOOT); | 54:m_pMission.SetTaskId(6); | 55:m_pMission.SetTaskAnimalToShoot(ANIMAL_HYENA,ANIMAL_KILL,3); | 56:m_pMission.SetSoundToBePlayed("attack1"); | 57:m_pMission.EndCreateTask();
  - task 7 (starts line 59): 59:m_pMission.CreateTask(TASK_RACE); | 60:m_pMission.SetTaskId(7); | 61:m_pMission.BeginTaskMsg("mis3task3_3_0"); | 62:m_pMission.EndTaskMsg("mis3task3_3_1"); | 63:m_pMission.SetTaskTimeLimit(0); | 64:m_pMission.EndCreateTask();
  - task 8 (starts line 66): 66:m_pMission.CreateTask(TASK_SHOOT); | 67:m_pMission.SetTaskId(8); | 68:m_pMission.SetTaskAnimalToShoot(ANIMAL_LEOPARD,ANIMAL_KILL,2); | 69:m_pMission.SetSoundToBePlayed("attack3"); | 70:m_pMission.EndCreateTask();
  - task 9 (starts line 72): 72:m_pMission.CreateTask(TASK_RACE); | 73:m_pMission.SetTaskId(9); | 74:m_pMission.BeginTaskMsg("mis3task4_4_0"); | 75:m_pMission.SetTaskTimeLimit(0); | 76:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:98` `UpdateMissionStatus(MISSION_ACTION_FIND_SOMETHING, AI_GUIDE_ID);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:152` `UpdateMissionStatus(MISSION_ACTION_FIND_SOMETHING, AI_GUIDE_ID);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:183` `UpdateMissionStatus(MISSION_ACTION_FIND_SOMETHING, AI_GUIDE_ID);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:215` `UpdateMissionStatus(MISSION_ACTION_FIND_SOMETHING, AI_GUIDE_ID);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:246` `UpdateMissionStatus(MISSION_ACTION_FIND_SOMETHING, AI_GUIDE_ID);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:399` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 5);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03A.SCR:468` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 7);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION03A.SCR:24` `m_pMission.ActivateTriggerOnEnd("cScrTask08");`

### MAP3B

- No `SetMission` call in the mission script.
- Task blocks: 2
  - task 1 (starts line 18): 18:m_pMission.CreateTask(TASK_RACE); | 19:m_pMission.SetTaskId(1); | 20:m_pMission.SetTaskTimeLimit(0); | 21:m_pMission.ActivateTriggerOnEnd("cScrTask03"); | 22:m_pMission.EndCreateTask();
  - task 2 (starts line 24): 24:m_pMission.CreateTask(TASK_SHOOT); | 25:m_pMission.SetTaskId(2); | 26:m_pMission.SetTaskAnimalToShoot(ANIMAL_LEOPARD,ANIMAL_KILL,1); | 27:m_pMission.SetSoundToBePlayed("attack2"); | 28:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03B.SCR:51` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION03B.SCR:21` `m_pMission.ActivateTriggerOnEnd("cScrTask03");`

### MAP3C

- No `SetMission` call in the mission script.
- Task blocks: 3
  - task 1 (starts line 18): 18:m_pMission.CreateTask(TASK_RACE); | 19:m_pMission.SetTaskId(1); | 20:m_pMission.EndTaskMsg("mis3task4_4_1"); | 21:m_pMission.SetTaskTimeLimit(0); | 22:m_pMission.EndCreateTask();
  - task 2 (starts line 24): 24:m_pMission.CreateTask(TASK_SHOOT); | 25:m_pMission.SetTaskId(2); | 26:m_pMission.BeginTaskMsg("mis3task5_5_0"); | 27:m_pMission.SetTaskAnimalToShoot(ANIMAL_LIONESSE,ANIMAL_KILL,2); | 28:m_pMission.ActivateTriggerOnEnd("cScrTask04"); | 29:m_pMission.SetSoundToBePlayed("attack2"); | 30:m_pMission.EndCreateTask();
  - task 3 (starts line 32): 32:m_pMission.CreateTask(TASK_SHOOT); | 33:m_pMission.SetTaskId(3); | 34:m_pMission.EndTaskMsg("mis3task5_5_1"); | 35:m_pMission.SetTaskAnimalToShoot(ANIMAL_LION,ANIMAL_KILL,1); | 36:m_pMission.ActivateTriggerOnEnd("cScrTask05"); | 37:m_pMission.SetSoundToBePlayed("attack3"); | 38:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION03C.SCR:78` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION03C.SCR:28` `m_pMission.ActivateTriggerOnEnd("cScrTask04");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION03C.SCR:36` `m_pMission.ActivateTriggerOnEnd("cScrTask05");`

### MAP4A

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSION04A.SCR:15` `SetMission(4);`
- Task blocks: 3
  - task 1 (starts line 18): 18:m_pMission.CreateTask(TASK_RACE); | 19:m_pMission.SetTaskId(1); | 20:m_pMission.BeginTaskMsg("mis4task1_1_0"); | 21:m_pMission.SetTaskTimeLimit(0); | 22:m_pMission.SetSoundToBePlayed("track_map4"); | 23:m_pMission.EndCreateTask();
  - task 2 (starts line 25): 25:m_pMission.CreateTask(TASK_RACE); | 26:m_pMission.SetTaskId(2); | 27:m_pMission.BeginTaskMsg("mis4task2_2_0"); | 28:m_pMission.EndTaskMsg("mis4task2_2_1"); | 29:m_pMission.SetTaskTimeLimit(0); | 30:m_pMission.EndCreateTask();
  - task 3 (starts line 32): 32:m_pMission.CreateTask(TASK_SHOOT); | 33:m_pMission.SetTaskId(3); | 34:m_pMission.SetTaskAnimalToShoot(ANIMAL_LION,ANIMAL_KILL,1); | 36:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04A.SCR:268` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 2);`

### MAP4B

- No `SetMission` call in the mission script.
- Task blocks: 4
  - task 1 (starts line 18): 18:m_pMission.CreateTask(TASK_RACE); | 19:m_pMission.SetTaskId(1); | 20:m_pMission.SetTaskTimeLimit(0); | 21:m_pMission.EndCreateTask();
  - task 2 (starts line 23): 23:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 24:m_pMission.SetTaskId(2); | 25:m_pMission.SetTaskAnimalToShoot(ANIMAL_PANTELOPE,ANIMAL_KILL,4); | 26:m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL); | 27:m_pMission.IsBonusTask(); | 28:m_pMission.EndCreateTask();
  - task 3 (starts line 30): 30:m_pMission.CreateTask(TASK_SHOOT); | 31:m_pMission.SetTaskId(3); | 32:m_pMission.SetTaskAnimalToShoot(ANIMAL_LIONESSE,ANIMAL_KILL,2); | 33:m_pMission.ActivateTriggerOnEnd("cScrTask19"); | 34:m_pMission.SetSoundToBePlayed("attack2"); | 35:m_pMission.EndCreateTask();
  - task 4 (starts line 37): 37:m_pMission.CreateTask(TASK_SHOOT); | 38:m_pMission.SetTaskId(4); | 39:m_pMission.SetTaskAnimalToShoot(ANIMAL_LION,ANIMAL_KILL,1); | 40:m_pMission.EndTaskMsg("mis4task1_1_1"); | 41:m_pMission.ActivateTriggerOnEnd("cScrTask20"); | 42:m_pMission.SetSoundToBePlayed("attack2"); | 43:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:643` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:670` `CompleteTask(1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:671` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:672` `CompleteTask(3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:673` `CompleteTask(4);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION04B.SCR:33` `m_pMission.ActivateTriggerOnEnd("cScrTask19");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION04B.SCR:41` `m_pMission.ActivateTriggerOnEnd("cScrTask20");`

### MAP5

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSION05.SCR:15` `SetMission(5);`
- Task blocks: 3
  - task 1 (starts line 18): 18:m_pMission.CreateTask(TASK_RACE); | 19:m_pMission.SetTaskId(1); | 20:m_pMission.BeginTaskMsg("mis5task1_1_0"); | 21:m_pMission.EndTaskMsg("mis5task1_1_1"); | 22:m_pMission.SetTaskTimeLimit(0); | 23:m_pMission.EndCreateTask();
  - task 2 (starts line 26): 26:m_pMission.CreateTask(TASK_SHOOT); | 27:m_pMission.SetTaskId(2); | 28:m_pMission.SetTaskAnimalToShoot(ANIMAL_HYENA,ANIMAL_KILL,1); | 29:m_pMission.SetSoundToBePlayed("attack2"); | 30:m_pMission.EndCreateTask();
  - task 3 (starts line 32): 32:m_pMission.CreateTask(TASK_RACE); | 33:m_pMission.SetTaskId(3); | 34:m_pMission.BeginTaskMsg("mis5task2_2_0"); | 35:m_pMission.EndTaskMsg("mis5task2_2_1"); | 36:m_pMission.SetTaskTimeLimit(0); | 37:m_pMission.SetSoundToBePlayed("track_map4"); | 38:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:219` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:273` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:610` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:611` `CompleteTask(1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:612` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:613` `CompleteTask(3);`

### MAP6

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSION06.SCR:15` `SetMission(6);`
- Task blocks: 8
  - task 1 (starts line 18): 18:m_pMission.CreateTask(TASK_RACE); | 19:m_pMission.SetTaskId(1); | 20:m_pMission.BeginTaskMsg("mis6task1_1_0"); | 21:m_pMission.SetTaskTimeLimit(0); | 22:m_pMission.EndCreateTask();
  - task 2 (starts line 25): 25:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 26:m_pMission.SetTaskId(2); | 27:m_pMission.SetTaskAnimalToShoot(ANIMAL_BTIGER,ANIMAL_KILL,1); | 28:m_pMission.SetSoundToBePlayed("attack1"); | 29:m_pMission.EndCreateTask();
  - task 3 (starts line 32): 32:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 33:m_pMission.SetTaskId(3); | 34:m_pMission.SetTaskAnimalToShoot(ANIMAL_BTIGER,ANIMAL_KILL,2); | 35:m_pMission.SetSoundToBePlayed("attack1"); | 36:m_pMission.EndCreateTask();
  - task 4 (starts line 39): 39:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 40:m_pMission.SetTaskId(4); | 41:m_pMission.SetTaskAnimalToShoot(ANIMAL_ABEAR,ANIMAL_KILL,1); | 42:m_pMission.SetSoundToBePlayed("attack1"); | 43:m_pMission.EndCreateTask();
  - task 5 (starts line 46): 46:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 47:m_pMission.SetTaskId(5); | 48:m_pMission.SetTaskAnimalToShoot(ANIMAL_BTIGER,ANIMAL_KILL,2); | 49:m_pMission.SetSoundToBePlayed("attack1"); | 50:m_pMission.EndCreateTask();
  - task 6 (starts line 53): 53:m_pMission.CreateTask(TASK_RACE); | 54:m_pMission.SetTaskId(6); | 55:m_pMission.BeginTaskMsg("mis6task2_2_0"); | 56:m_pMission.EndTaskMsg("mis6task2_2_1"); | 57:m_pMission.SetTaskTimeLimit(0); | 58:m_pMission.EndCreateTask();
  - task 7 (starts line 61): 61:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 62:m_pMission.SetTaskId(7); | 63:m_pMission.SetTaskAnimalToShoot(ANIMAL_BTIGER,ANIMAL_KILL,4); | 64:m_pMission.EndTaskMsg("mis6task1_1_1"); | 67:m_pMission.EndCreateTask();
  - task 8 (starts line 70): 70:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 71:m_pMission.SetTaskId(8); | 72:m_pMission.SetTaskAnimalToShoot(ANIMAL_ABEAR,ANIMAL_KILL,1); | 73:m_pMission.BeginTaskMsg("mis6task3_3_0"); | 74:m_pMission.EndTaskMsg("mis6task3_3_1"); | 76:m_pMission.SetSoundToBePlayed("attack1"); | 77:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:105` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:142` `CompleteTask(3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:173` `CompleteTask(4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:205` `CompleteTask(5);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:236` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:328` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 6);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:366` `CompleteTask(1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:367` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:368` `CompleteTask(3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:369` `CompleteTask(4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:370` `CompleteTask(5);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:371` `CompleteTask(6);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:372` `CompleteTask(7);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:373` `CompleteTask(8);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:374` `CompleteTask(9);`

### MAP7

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSION07.SCR:14` `SetMission(7);`
- Task blocks: 9
  - task 1 (starts line 17): 17:m_pMission.CreateTask(TASK_FIND); | 18:m_pMission.SetTaskId(1); | 19:m_pMission.SetTaskObjToFind(AI_CHILD_ID,1); | 20:m_pMission.BeginTaskMsg("mis7task1_1_0"); | 21:m_pMission.EndTaskMsg("mis7task1_1_1"); | 22:m_pMission.SetTaskTimeLimit(0); | 23:m_pMission.EndCreateTask();
  - task 2 (starts line 26): 26:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 27:m_pMission.SetTaskId(2); | 28:m_pMission.SetTaskAnimalToShoot(ANIMAL_BTIGER,ANIMAL_KILL,1); | 29:m_pMission.SetSoundToBePlayed("attack1"); | 30:m_pMission.EndCreateTask();
  - task 3 (starts line 33): 33:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 34:m_pMission.SetTaskId(3); | 35:m_pMission.SetTaskAnimalToShoot(ANIMAL_BTIGER,ANIMAL_KILL,2); | 36:m_pMission.SetSoundToBePlayed("attack1"); | 37:m_pMission.EndCreateTask(); //insert task - ALWAYS last
  - task 4 (starts line 40): 40:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 41:m_pMission.SetTaskId(4); | 42:m_pMission.SetTaskAnimalToShoot(ANIMAL_BTIGER,ANIMAL_KILL,1); | 43:m_pMission.SetSoundToBePlayed("attack1"); | 44:m_pMission.EndCreateTask();
  - task 5 (starts line 47): 47:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 48:m_pMission.SetTaskId(5); | 49:m_pMission.SetTaskAnimalToShoot(ANIMAL_BTIGER,ANIMAL_KILL,2); | 50:m_pMission.SetSoundToBePlayed("attack1"); | 51:m_pMission.EndCreateTask();
  - task 6 (starts line 54): 54:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 55:m_pMission.SetTaskId(6); | 56:m_pMission.SetTaskAnimalToShoot(ANIMAL_COBRA,ANIMAL_KILL,1); | 57:m_pMission.ActivateTriggerOnEnd("cScrTask08"); | 58:m_pMission.EndCreateTask();
  - task 7 (starts line 61): 61:m_pMission.CreateTask(TASK_RACE); | 62:m_pMission.SetTaskId(7); | 63:m_pMission.BeginTaskMsg("mis7task2_2_0"); | 64:m_pMission.SetTaskTimeLimit(0); | 65:m_pMission.EndCreateTask();
  - task 8 (starts line 68): 68:m_pMission.CreateTask(TASK_SHOOT); //describe a task | 69:m_pMission.SetTaskId(8); | 70:m_pMission.BeginTaskMsg("mis7task3_3_0"); | 71:m_pMission.EndTaskMsg("mis7task3_3_1"); | 72:m_pMission.SetTaskAnimalToShoot(ANIMAL_RHINO,ANIMAL_KILL,1); | 73:m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL); | 74:m_pMission.ActivateTriggerOnEnd("cScrTask10"); | 75:m_pMission.SetSoundToBePlayed("attack1"); | 76:m_pMission.EndCreateTask();
  - task 9 (starts line 79): 79:m_pMission.CreateTask(TASK_RACE); | 80:m_pMission.SetTaskId(9); | 81:m_pMission.EndTaskMsg("mis7task2_2_1"); | 82:m_pMission.SetTaskTimeLimit(0); | 83:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:89` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:150` `CompleteTask(3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:288` `CompleteTask(4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:289` `CompleteTask(5);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:332` `UpdateMissionStatus(MISSION_ACTION_FIND_SOMETHING, AI_CHILD_ID);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:522` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION ,9);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:536` `CompleteTask(7);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:537` `CompleteTask(8);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:538` `CompleteTask(9);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION07.SCR:57` `m_pMission.ActivateTriggerOnEnd("cScrTask08");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION07.SCR:74` `m_pMission.ActivateTriggerOnEnd("cScrTask10");`

### MAP8

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSION08.SCR:15` `SetMission(8);`
- Task blocks: 10
  - task 1 (starts line 19): 19:m_pMission.CreateTask(TASK_FIND); | 20:m_pMission.SetTaskId(1); | 21:m_pMission.SetTaskObjToFind(AI_GUIDE_ID,1); | 22:m_pMission.BeginTaskMsg("mis8task1_1_0"); | 23:m_pMission.EndTaskMsg("mis8task1_1_1"); | 24:m_pMission.SetTaskTimeLimit(0); | 25:m_pMission.EndCreateTask();
  - task 2 (starts line 27): 27:m_pMission.CreateTask(TASK_SHOOT); | 28:m_pMission.SetTaskId(2); | 29:m_pMission.SetTaskAnimalToShoot(ANIMAL_SCRUBBULL,ANIMAL_KILL,1); | 30:m_pMission.SetSoundToBePlayed("attack2"); | 31:m_pMission.EndCreateTask();
  - task 3 (starts line 33): 33:m_pMission.CreateTask(TASK_SHOOT); | 34:m_pMission.SetTaskId(3); | 35:m_pMission.SetTaskAnimalToShoot(ANIMAL_SCRUBBULL,ANIMAL_KILL,1); | 36:m_pMission.SetSoundToBePlayed("attack1"); | 37:m_pMission.EndCreateTask();
  - task 4 (starts line 40): 40:m_pMission.CreateTask(TASK_SHOOT); | 41:m_pMission.SetTaskId(4); | 42:m_pMission.SetTaskAnimalToShoot(ANIMAL_SCRUBBULL,ANIMAL_KILL,1); | 43:m_pMission.SetSoundToBePlayed("attack3"); | 44:m_pMission.EndCreateTask();
  - task 5 (starts line 46): 46:m_pMission.CreateTask(TASK_SHOOT); | 47:m_pMission.SetTaskId(5); | 48:m_pMission.SetTaskAnimalToShoot(ANIMAL_SCRUBBULL,ANIMAL_KILL,1); | 49:m_pMission.SetSoundToBePlayed("attack3"); | 50:m_pMission.EndCreateTask();
  - task 6 (starts line 52): 52:m_pMission.CreateTask(TASK_RACE); | 53:m_pMission.SetTaskId(6); | 54:m_pMission.BeginTaskMsg("mis8task2_2_0"); | 55:m_pMission.EndTaskMsg("mis8task2_2_1"); | 56:m_pMission.SetTaskTimeLimit(0); | 57:m_pMission.EndCreateTask();
  - task 7 (starts line 59): 59:m_pMission.CreateTask(TASK_SHOOT); | 60:m_pMission.SetTaskId(7); | 61:m_pMission.SetTaskAnimalToShoot(ANIMAL_SCRUBBULL,ANIMAL_KILL,1); | 62:m_pMission.SetSoundToBePlayed("attack1"); | 63:m_pMission.EndCreateTask();
  - task 8 (starts line 66): 66:m_pMission.CreateTask(TASK_SHOOT); | 67:m_pMission.SetTaskId(8); | 68:m_pMission.SetTaskAnimalToShoot(ANIMAL_SCRUBBULL,ANIMAL_KILL,1); | 69:m_pMission.SetSoundToBePlayed("attack2"); | 70:m_pMission.EndCreateTask();
  - task 9 (starts line 72): 72:m_pMission.CreateTask(TASK_SHOOT); | 73:m_pMission.SetTaskId(9); | 74:m_pMission.SetTaskAnimalToShoot(ANIMAL_SCRUBBULL,ANIMAL_KILL,1); | 75:m_pMission.SetSoundToBePlayed("attack3"); | 76:m_pMission.EndCreateTask();
  - task 10 (starts line 78): 78:m_pMission.CreateTask(TASK_RACE); | 79:m_pMission.SetTaskId(10); | 80:m_pMission.SetTaskTimeLimit(0); | 81:m_pMission.SetSoundToBePlayed("attack2"); | 82:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:342` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:343` `CompleteTask(3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:344` `CompleteTask(4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:345` `CompleteTask(5);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:363` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:364` `CompleteTask(3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:365` `CompleteTask(4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:366` `CompleteTask(5);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:494` `UpdateMissionStatus(MISSION_ACTION_FIND_SOMETHING, AI_GUIDE_ID);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:910` `CompleteTask(6);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:911` `CompleteTask(7);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:912` `CompleteTask(8);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:913` `CompleteTask(9);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:914` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 6);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:942` `CompleteTask(10);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:943` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 10);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION08.SCR:1084` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 3);`

### MAP9

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSION09.SCR:15` `SetMission(9);`
- Task blocks: 6
  - task 1 (starts line 19): 19:m_pMission.CreateTask(TASK_RACE); | 20:m_pMission.SetTaskId(1); | 21:m_pMission.BeginTaskMsg("mis9task1_1_0"); | 22:m_pMission.EndTaskMsg("mis9task1_1_1"); | 23:m_pMission.SetTaskTimeLimit(0); | 24:m_pMission.EndCreateTask();
  - task 2 (starts line 27): 27:m_pMission.CreateTask(TASK_SHOOT); | 28:m_pMission.SetTaskId(2); | 29:m_pMission.SetTaskAnimalToShoot(ANIMAL_DINGO,ANIMAL_KILL,2); | 30:m_pMission.SetSoundToBePlayed("attack1"); | 31:m_pMission.EndCreateTask();
  - task 3 (starts line 34): 34:m_pMission.CreateTask(TASK_SHOOT); | 35:m_pMission.SetTaskId(3); | 36:m_pMission.SetTaskAnimalToShoot(ANIMAL_DINGO,ANIMAL_KILL,2); | 37:m_pMission.SetSoundToBePlayed("attack1"); | 38:m_pMission.EndCreateTask();
  - task 4 (starts line 48): 48:m_pMission.CreateTask(TASK_SHOOT); | 49:m_pMission.SetTaskId(4); | 50:m_pMission.SetTaskAnimalToShoot(ANIMAL_DINGO,ANIMAL_KILL,2); | 51:m_pMission.SetSoundToBePlayed("attack1"); | 52:m_pMission.EndCreateTask();
  - task 5 (starts line 55): 55:m_pMission.CreateTask(TASK_SHOOT); | 56:m_pMission.SetTaskId(5); | 57:m_pMission.SetTaskAnimalToShoot(ANIMAL_DINGO,ANIMAL_KILL,3); | 58:m_pMission.SetSoundToBePlayed("attack1"); | 59:m_pMission.EndCreateTask();
  - task 6 (starts line 61): 61:m_pMission.CreateTask(TASK_SHOOT); | 62:m_pMission.SetTaskId(6); | 63:m_pMission.SetTaskAnimalToShoot(ANIMAL_DINGO,ANIMAL_KILL,2); | 64:m_pMission.SetSoundToBePlayed("attack1"); | 65:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:397` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:420` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:421` `CompleteTask(3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:422` `CompleteTask(4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:423` `CompleteTask(5);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:424` `CompleteTask(6);`

### MAP10

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSION10.SCR:15` `SetMission(10);`
- Task blocks: 11
  - task 1 (starts line 18): 18:m_pMission.CreateTask(TASK_RACE); | 19:m_pMission.SetTaskId(1); | 20:m_pMission.BeginTaskMsg("mis10task1_1_0"); | 21:m_pMission.EndTaskMsg("mis10task1_1_1"); | 22:m_pMission.SetTaskTimeLimit(0); | 23:m_pMission.EndCreateTask();
  - task 2 (starts line 26): 26:m_pMission.CreateTask(TASK_SHOOT); | 27:m_pMission.SetTaskId(2); | 28:m_pMission.SetTaskAnimalToShoot(ANIMAL_PUMA,ANIMAL_KILL,1); | 29:m_pMission.SetSoundToBePlayed("attack2"); | 30:m_pMission.EndCreateTask();
  - task 3 (starts line 32): 32:m_pMission.CreateTask(TASK_SHOOT); | 33:m_pMission.SetTaskId(3); | 34:m_pMission.SetTaskAnimalToShoot(ANIMAL_PUMA,ANIMAL_KILL,1); | 35:m_pMission.SetSoundToBePlayed("attack3"); | 36:m_pMission.EndCreateTask();
  - task 4 (starts line 38): 38:m_pMission.CreateTask(TASK_SHOOT); | 39:m_pMission.SetTaskId(4); | 40:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR3,ANIMAL_KILL,2); | 41:m_pMission.SetSoundToBePlayed("attack1"); | 42:m_pMission.EndCreateTask();
  - task 5 (starts line 44): 44:m_pMission.CreateTask(TASK_SHOOT); | 45:m_pMission.SetTaskId(5); | 46:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR3,ANIMAL_KILL,2); | 47:m_pMission.SetSoundToBePlayed("attack2"); | 48:m_pMission.EndCreateTask();
  - task 6 (starts line 50): 50:m_pMission.CreateTask(TASK_SHOOT); | 51:m_pMission.SetTaskId(6); | 52:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR3,ANIMAL_KILL,2); | 53:m_pMission.SetSoundToBePlayed("attack1"); | 54:m_pMission.EndCreateTask();
  - task 7 (starts line 56): 56:m_pMission.CreateTask(TASK_SHOOT); | 57:m_pMission.SetTaskId(7); | 58:m_pMission.SetTaskAnimalToShoot(ANIMAL_PUMA,ANIMAL_KILL,2); | 59:m_pMission.SetSoundToBePlayed("attack3"); | 60:m_pMission.EndCreateTask();
  - task 8 (starts line 62): 62:m_pMission.CreateTask(TASK_SHOOT); | 63:m_pMission.SetTaskId(8); | 64:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR3,ANIMAL_KILL,2); | 65:m_pMission.SetSoundToBePlayed("attack1"); | 66:m_pMission.EndCreateTask();
  - task 9 (starts line 68): 68:m_pMission.CreateTask(TASK_RACE); | 69:m_pMission.SetTaskId(9); | 70:m_pMission.BeginTaskMsg("mis10task2_2_0"); | 71:m_pMission.EndTaskMsg("mis10task2_2_1"); | 72:m_pMission.SetTaskTimeLimit(0); | 73:m_pMission.EndCreateTask();
  - task 10 (starts line 75): 75:m_pMission.CreateTask(TASK_SHOOT); | 76:m_pMission.SetTaskId(10); | 77:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR3,ANIMAL_KILL,3); | 78:m_pMission.SetSoundToBePlayed("attack2"); | 79:m_pMission.EndCreateTask();
  - task 11 (starts line 81): 81:m_pMission.CreateTask(TASK_SHOOT); | 82:m_pMission.SetTaskId(11); | 83:m_pMission.SetTaskAnimalToShoot(ANIMAL_PUMA,ANIMAL_KILL,2); | 84:m_pMission.SetSoundToBePlayed("attack1"); | 85:m_pMission.ActivateTriggerOnEnd("cScrTask17"); | 86:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:119` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:152` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:186` `CompleteTask(3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:282` `CompleteTask(4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:326` `CompleteTask(5);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:391` `CompleteTask(6);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:433` `CompleteTask(7);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:482` `CompleteTask(8);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:639` `CompleteTask(9);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:759` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION , 9);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:782` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION , 1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:788` `CompleteTask(1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:789` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:790` `CompleteTask(3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:791` `CompleteTask(4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:792` `CompleteTask(5);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:793` `CompleteTask(6);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:794` `CompleteTask(7);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:795` `CompleteTask(8);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:796` `CompleteTask(9);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:797` `CompleteTask(10);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:798` `CompleteTask(11);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION10.SCR:85` `m_pMission.ActivateTriggerOnEnd("cScrTask17");`

### MAP11A

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSION11A.SCR:14` `SetMission(11);`
- Task blocks: 4
  - task 1 (starts line 18): 18:m_pMission.CreateTask(TASK_RACE); | 19:m_pMission.SetTaskId(1); | 20:m_pMission.BeginTaskMsg("mis11task1_1_0"); | 21:m_pMission.ActivateTriggerOnFail("cScrFailed"); | 22:m_pMission.SetTaskTimeLimit(0); | 23:m_pMission.EndCreateTask();
  - task 2 (starts line 25): 25:m_pMission.CreateTask(TASK_SHOOT); | 26:m_pMission.SetTaskId(2); | 27:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR,ANIMAL_KILL,1); | 28:m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE); | 29:m_pMission.ActivateTriggerOnEnd("cScrTask06"); | 30:m_pMission.ActivateTriggerOnFail("cScrFailed"); | 31:m_pMission.SetSoundToBePlayed("attack1"); | 32:m_pMission.EndCreateTask();
  - task 3 (starts line 34): 34:m_pMission.CreateTask(TASK_SHOOT); | 35:m_pMission.SetTaskId(3); | 36:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR,ANIMAL_KILL,3); | 37:m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE); | 38:m_pMission.ActivateTriggerOnEnd("cScrTask07"); | 39:m_pMission.ActivateTriggerOnFail("cScrFailed"); | 40:m_pMission.SetSoundToBePlayed("attack1"); | 41:m_pMission.EndCreateTask();
  - task 4 (starts line 43): 43:m_pMission.CreateTask(TASK_SHOOT); | 44:m_pMission.SetTaskId(4); | 45:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR,ANIMAL_KILL,3); | 46:m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE); | 47:m_pMission.ActivateTriggerOnEnd("cScrTask08"); | 48:m_pMission.ActivateTriggerOnFail("cScrFailed"); | 49:m_pMission.SetSoundToBePlayed("attack1"); | 50:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:25` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:59` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:93` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:110` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:125` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:157` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:172` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:43` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:86` `::globals.helicopter.CompleteTask();`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11A.SCR:140` `::globals.helicopter.CompleteTask();`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11A.SCR:21` `m_pMission.ActivateTriggerOnFail("cScrFailed");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11A.SCR:28` `m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11A.SCR:29` `m_pMission.ActivateTriggerOnEnd("cScrTask06");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11A.SCR:30` `m_pMission.ActivateTriggerOnFail("cScrFailed");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11A.SCR:37` `m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11A.SCR:38` `m_pMission.ActivateTriggerOnEnd("cScrTask07");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11A.SCR:39` `m_pMission.ActivateTriggerOnFail("cScrFailed");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11A.SCR:46` `m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11A.SCR:47` `m_pMission.ActivateTriggerOnEnd("cScrTask08");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11A.SCR:48` `m_pMission.ActivateTriggerOnFail("cScrFailed");`

### MAP11B

- No `SetMission` call in the mission script.
- Task blocks: 7
  - task 1 (starts line 19): 19:m_pMission.CreateTask(TASK_RACE); | 20:m_pMission.SetTaskId(1); | 21:m_pMission.BeginTaskMsg("mis11task2_2_0"); | 22:m_pMission.EndTaskMsg("mis11task2_2_1"); | 23:m_pMission.SetTaskTimeLimit(0); | 24:m_pMission.EndCreateTask();
  - task 2 (starts line 26): 26:m_pMission.CreateTask(TASK_SHOOT); | 27:m_pMission.SetTaskId(2); | 28:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR,ANIMAL_KILL,3); | 30:m_pMission.SetSoundToBePlayed("attack1"); | 31:m_pMission.EndCreateTask();
  - task 3 (starts line 33): 33:m_pMission.CreateTask(TASK_SHOOT); | 34:m_pMission.SetTaskId(3); | 35:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR,ANIMAL_KILL,3); | 36:m_pMission.SetSoundToBePlayed("attack1"); | 37:m_pMission.EndCreateTask();
  - task 4 (starts line 44): 44:m_pMission.CreateTask(TASK_RACE); | 45:m_pMission.SetTaskId(4); | 46:m_pMission.BeginTaskMsg("mis11task3_3_0"); | 47:m_pMission.EndTaskMsg("mis11task3_3_1"); | 48:m_pMission.SetTaskTimeLimit(0); | 49:m_pMission.EndCreateTask();
  - task 5 (starts line 51): 51:m_pMission.CreateTask(TASK_SHOOT); | 52:m_pMission.SetTaskId(5); | 53:m_pMission.SetTaskAnimalToShoot(ANIMAL_HOGZILLA,ANIMAL_KILL,1); | 54:m_pMission.BeginTaskMsg("mis11task4_4_0"); | 55:m_pMission.EndTaskMsg("mis11task4_4_1"); | 56:m_pMission.SetSoundToBePlayed("attack1"); | 57:m_pMission.ActivateTriggerOnEnd("cScrTask11"); | 58:m_pMission.EndCreateTask();
  - task 6 (starts line 60): 60:m_pMission.CreateTask(TASK_SHOOT); | 61:m_pMission.SetTaskId(6); | 62:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR,ANIMAL_KILL,3); | 63:m_pMission.SetSoundToBePlayed("attack1"); | 64:m_pMission.EndCreateTask();
  - task 7 (starts line 66): 66:m_pMission.CreateTask(TASK_SHOOT); | 67:m_pMission.SetTaskId(7); | 68:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR,ANIMAL_KILL,2); | 69:m_pMission.SetSoundToBePlayed("attack1"); | 70:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:137` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:296` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 4);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11B.SCR:57` `m_pMission.ActivateTriggerOnEnd("cScrTask11");`

### MAP11C

- No `SetMission` call in the mission script.
- Task blocks: 5
  - task 1 (starts line 19): 19:m_pMission.CreateTask(TASK_RACE); | 20:m_pMission.SetTaskId(1); | 21:m_pMission.SetTaskTimeLimit(0); | 22:m_pMission.EndCreateTask();
  - task 2 (starts line 24): 24:m_pMission.CreateTask(TASK_SHOOT); | 25:m_pMission.SetTaskId(2); | 26:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR,ANIMAL_KILL,3); | 27:m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE); | 28:m_pMission.ActivateTriggerOnEnd("cScrTask06"); | 30:m_pMission.SetSoundToBePlayed("attack1"); | 31:m_pMission.EndCreateTask();
  - task 3 (starts line 33): 33:m_pMission.CreateTask(TASK_SHOOT); | 34:m_pMission.SetTaskId(3); | 35:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR,ANIMAL_KILL,3); | 36:m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE); | 37:m_pMission.ActivateTriggerOnEnd("cScrTask07"); | 39:m_pMission.SetSoundToBePlayed("attack1"); | 40:m_pMission.EndCreateTask();
  - task 4 (starts line 42): 42:m_pMission.CreateTask(TASK_SHOOT); | 43:m_pMission.SetTaskId(4); | 44:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR,ANIMAL_KILL,4); | 45:m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE); | 46:m_pMission.ActivateTriggerOnEnd("cScrTask09"); | 48:m_pMission.SetSoundToBePlayed("attack1"); | 49:m_pMission.EndCreateTask();
  - task 5 (starts line 51): 51:m_pMission.CreateTask(TASK_SHOOT); | 52:m_pMission.SetTaskId(5); | 53:m_pMission.EndTaskMsg("mis11task1_1_1"); | 54:m_pMission.SetTaskAnimalToShoot(ANIMAL_WILDBOAR,ANIMAL_KILL,2); | 55:m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE); | 56:m_pMission.ActivateTriggerOnEnd("cScrTask08"); | 58:m_pMission.SetSoundToBePlayed("attack1"); | 59:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:25` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:59` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:93` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:110` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:125` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:157` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:172` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:38` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:76` `::globals.helicopter.CompleteTask();`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11C.SCR:125` `::globals.helicopter.CompleteTask();`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11C.SCR:27` `m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11C.SCR:28` `m_pMission.ActivateTriggerOnEnd("cScrTask06");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11C.SCR:36` `m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11C.SCR:37` `m_pMission.ActivateTriggerOnEnd("cScrTask07");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11C.SCR:45` `m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11C.SCR:46` `m_pMission.ActivateTriggerOnEnd("cScrTask09");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11C.SCR:55` `m_pMission.SetCompleteCondition(NEED_AMMO_FOR_COMPLETE);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION11C.SCR:56` `m_pMission.ActivateTriggerOnEnd("cScrTask08");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE3.SCR:65` `helicopter.CompleteTask();`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11C/CUTSCENE3.SCR:90` `helicopter.CompleteTask();`

### MAP12A

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSION12A.SCR:14` `SetMission(12);`
- Task blocks: 8
  - task 1 (starts line 17): 17:m_pMission.CreateTask(TASK_RACE); | 18:m_pMission.SetTaskId(1); | 19:m_pMission.BeginTaskMsg("mis12task1_1_0"); | 20:m_pMission.EndTaskMsg("mis12task1_1_1"); | 21:m_pMission.SetTaskTimeLimit(0); | 22:m_pMission.EndCreateTask();
  - task 2 (starts line 25): 25:m_pMission.CreateTask(TASK_SHOOT); | 26:m_pMission.SetTaskId(2); | 27:m_pMission.SetTaskAnimalToShoot(ANIMAL_GWOLF,ANIMAL_KILL,2); | 28:m_pMission.SetSoundToBePlayed("attack1"); | 29:m_pMission.EndCreateTask();
  - task 3 (starts line 31): 31:m_pMission.CreateTask(TASK_SHOOT); | 32:m_pMission.SetTaskId(3); | 33:m_pMission.SetTaskAnimalToShoot(ANIMAL_PBEAR,ANIMAL_KILL,1); | 34:m_pMission.SetSoundToBePlayed("attack1"); | 35:m_pMission.EndCreateTask();
  - task 4 (starts line 37): 37:m_pMission.CreateTask(TASK_SHOOT); | 38:m_pMission.SetTaskId(4); | 39:m_pMission.SetTaskAnimalToShoot(ANIMAL_GWOLF,ANIMAL_KILL,2); | 40:m_pMission.SetSoundToBePlayed("attack1"); | 41:m_pMission.EndCreateTask();
  - task 5 (starts line 43): 43:m_pMission.CreateTask(TASK_SHOOT); | 44:m_pMission.SetTaskId(5); | 45:m_pMission.SetTaskAnimalToShoot(ANIMAL_GWOLF,ANIMAL_KILL,1); | 46:m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL); | 47:m_pMission.SetSoundToBePlayed("attack1"); | 48:m_pMission.ActivateTriggerOnEnd("cScrTask10"); | 49:m_pMission.EndCreateTask();
  - task 6 (starts line 51): 51:m_pMission.CreateTask(TASK_SHOOT); | 52:m_pMission.SetTaskId(6); | 53:m_pMission.SetTaskAnimalToShoot(ANIMAL_GWOLF,ANIMAL_KILL,2); | 54:m_pMission.SetSoundToBePlayed("attack1"); | 55:m_pMission.EndCreateTask();
  - task 7 (starts line 58): 58:m_pMission.CreateTask(TASK_SHOOT); | 59:m_pMission.SetTaskId(7); | 60:m_pMission.SetTaskAnimalToShoot(ANIMAL_GWOLF,ANIMAL_KILL,2); | 61:m_pMission.SetSoundToBePlayed("attack1"); | 62:m_pMission.EndCreateTask();
  - task 8 (starts line 64): 64:m_pMission.CreateTask(TASK_SHOOT); | 65:m_pMission.SetTaskId(8); | 66:m_pMission.SetTaskAnimalToShoot(ANIMAL_PBEAR,ANIMAL_KILL,1); | 67:m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL); | 68:m_pMission.SetSoundToBePlayed("attack1"); | 69:m_pMission.ActivateTriggerOnEnd("cScrTask13"); | 70:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:96` `CompleteTask(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:127` `CompleteTask(3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:174` `CompleteTask(4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:252` `CompleteTask(5);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:366` `CompleteTask(6);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:473` `CompleteTask(7);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:580` `CompleteTask(8);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:628` `UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION12A.SCR:48` `m_pMission.ActivateTriggerOnEnd("cScrTask10");`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION12A.SCR:69` `m_pMission.ActivateTriggerOnEnd("cScrTask13");`

### MAP12B

- No `SetMission` call in the mission script.
- Task blocks: 1
  - task 1 (starts line 18): 18:m_pMission.CreateTask(TASK_SHOOT); | 19:m_pMission.SetTaskId(1); | 20:m_pMission.BeginTaskMsg("mis12task2_2_0"); | 21:m_pMission.EndTaskMsg("mis12task2_2_1"); | 22:m_pMission.SetTaskAnimalToShoot(ANIMAL_BIGGRIM,ANIMAL_KILL,1); | 23:m_pMission.ActivateTriggerOnEnd("cScrTask02"); | 24:m_pMission.SetSoundToBePlayed("attack1"); | 25:m_pMission.SetMissionOnFleePointReachedAction(MISSION_ACTION_KILL); | 26:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/MISSIONS/MISSION12B.SCR:23` `m_pMission.ActivateTriggerOnEnd("cScrTask02");`

### MAP12C

- No `SetMission` call in the mission script.
- Task blocks: 1
  - task 1 (starts line 17): 17:m_pMission.CreateTask(TASK_SHOOT); | 18:m_pMission.SetTaskId(1); | 19:m_pMission.BeginTaskMsg("mis12task3_3_0"); | 20:m_pMission.EndTaskMsg("mis12task3_3_1"); | 21:m_pMission.SetTaskAnimalToShoot(ANIMAL_YETI,ANIMAL_KILL,1); | 22:m_pMission.ActivateTriggerOnEnd("cScrTask02"); | 23:m_pMission.SetSoundToBePlayed("attack1"); | 24:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:25` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:59` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:93` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:110` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:125` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:157` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MODELS/WORLD/HELICOPTER/HELICOPTER.SCR:172` `m_pHelicopter.UncompleteTask();`
  - `re/extracted/SCRIPTS/MISSIONS/MISSION12C.SCR:22` `m_pMission.ActivateTriggerOnEnd("cScrTask02");`

### MAPARENA

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSIONMAPARENA.SCR:14` `SetMission(17);`
- Task blocks: 1
  - task 1 (starts line 15): 15:m_pMission.CreateTask(TASK_RACE); | 16:m_pMission.SetTaskId(1); | 17:m_pMission.IsBonusTask(); | 18:m_pMission.BeginTaskMsg("mis17task1_1_0"); | 19:m_pMission.EndTaskMsg("mis17task1_1_1"); | 20:m_pMission.SetTaskTimeLimit(0); | 21:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:695` `case 2: {::globals.player.PlayHint("PERFECT!"); UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);Pause(1.0);EndArena(); break;}`

### MAPISLAND

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSIONMAPISLAND.SCR:14` `SetMission(18);`
- Task blocks: 1
  - task 1 (starts line 15): 15:m_pMission.CreateTask(TASK_RACE); | 16:m_pMission.SetTaskId(1); | 17:m_pMission.IsBonusTask(); | 18:m_pMission.BeginTaskMsg("mis18task1_1_0"); | 19:m_pMission.EndTaskMsg("mis18task1_1_1"); | 20:m_pMission.SetTaskTimeLimit(0); | 21:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:296` `case 4: {::globals.player.PlayHint("PERFECT!"); Pause(2.0);UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);Pause(1.0);EndArena(); break;}`

### MAPTSAVO

- SetMission `re/extracted/SCRIPTS/MISSIONS/MISSIONMAPTSAVO.SCR:14` `SetMission(19);`
- Task blocks: 1
  - task 1 (starts line 15): 15:m_pMission.CreateTask(TASK_RACE); | 16:m_pMission.SetTaskId(1); | 17:m_pMission.IsBonusTask(); | 18:m_pMission.BeginTaskMsg("mis19task1_1_0"); | 19:m_pMission.EndTaskMsg("mis19task1_1_1"); | 20:m_pMission.SetTaskTimeLimit(0); | 21:m_pMission.EndCreateTask();
- Complete / fail / status calls:
  - `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:316` `case 4: {::globals.player.PlayHint("PERFECT!"); Pause(2.0);UpdateMissionStatus(MISSION_ACTION_REACHED_POSITION, 1);Pause(1.0);EndArena(); break;}`

### MAPMENU

- No `SetMission` call in the mission script.
- Task blocks: 0
- Complete / fail / status calls:
  - none in this map's script closure

## 7. Map transitions, checkpoints, saves, death

Searched every file in story, bonus, and hunt `MAIN.SPJ` closures (each file once). A call that loads another map, restarts the mission, saves, loads a checkpoint, or handles player death is quoted below. Keyword collisions are listed separately so they are not read as those calls.

### Restart(

No matches in the script closures.

### RestartMission(

No matches in the script closures.

### save/load/checkpoint

No matches in the script closures.

### map-load or end name

No matches in the script closures.

### player death name

No matches in the script closures.

### OpenMenu(

| File:line | Line |
|---|---|
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERHUNTMAP1.SCR:92` | `OpenMenu("WeaponMenu");` |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERHUNTMAP2.SCR:92` | `OpenMenu("WeaponMenu");` |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERHUNTMAP3.SCR:92` | `OpenMenu("WeaponMenu");` |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERHUNTMAP4.SCR:92` | `OpenMenu("WeaponMenu");` |

### quoted map token

No matches in the script closures.

### Player damage lines

| File:line | Line |
|---|---|
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:22` | `if(!player.IsCrouch() && player.IsLegHurt())` |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:108` | `else if(!player.IsLegHurt())` |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:166` | `else if(!player.IsLegHurt())` |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:222` | `if(!player.IsLegHurt())` |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:279` | `if(!player.IsLegHurt())` |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:383` | `if(!player.IsLegHurt())` |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:445` | `if(!player.IsLegHurt())` |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:508` | `if(!player.IsLegHurt())` |
| `re/extracted/SCRIPTS/MODELS/CHARACTERS/HUMANLEGS.SCR:567` | `if(!player.IsLegHurt())` |
| `re/extracted/SCRIPTS/LEVELS/MAPARENA/WORLD.SCR:623` | `if (0.4>::globals.player.GetHealth())` |
| `re/extracted/SCRIPTS/LEVELS/MAPISLAND/WORLD.SCR:236` | `if (0.4>::globals.player.GetHealth())` |
| `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMAPISLAND.SCR:63` | `::globals.player.Hurt(-1, 2, 0);` |
| `re/extracted/SCRIPTS/LEVELS/MAPTSAVO/WORLD.SCR:246` | `if (0.4>::globals.player.GetHealth())` |

`IsLegHurt` and `GetHealth` are queries. `Hurt(` on those lines is the damage call.

### Keyword collisions (not map load, save, checkpoint, or player respawn)

- `SetMap lens-flare rect (not a level load)`: 17 lines.
  - `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:17` `void SetMapLensFlare()`
  - `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:22` `SetMap(0, 0.5, 0.5, 1);`
  - `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:30` `SetMap(0.20, 0.2, 0.80, 0.8);`
  - `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:38` `SetMap(0.0, 0.0, 1, 1);`
  - `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:47` `SetMap(0, 0, 1, 1);`
  - `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:55` `SetMap(0.5, 0.5, 1, 1);`
  - `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:63` `SetMap(0.5, 0, 1, 0.5);`
  - `re/extracted/SCRIPTS/LEVELS/LENSFLARE.SCR:72` `SetMap(0, 0, 0.5, 0.5);`
  - … 9 more
- `animal CanBeRespawned / StampedeRespawn (not a player respawn)`: 71 lines.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:397` `if (TRUE==::globals.wolf3.CanBeRespawned())`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:541` `if(TRUE == ::globals.wolf1.CanBeRespawned())`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:566` `if(TRUE == ::globals.wolf2.CanBeRespawned())`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:595` `if(TRUE == ::globals.wolf3.CanBeRespawned())`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:659` `if ((TRUE == ::globals.wolf1.CanBeRespawned()) &&`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:660` `(TRUE == ::globals.wolf2.CanBeRespawned()) &&`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:661` `(TRUE == ::globals.wolf3.CanBeRespawned())) setstate(NoChange);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:193` `if (TRUE == ::globals.wildboar1.CanBeRespawned())`
  - … 63 more
- `OpenMenuPage (page index)`: 13 lines.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:16` `OpenMenuPage(1);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:76` `OpenMenuPage(2);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:130` `OpenMenuPage(3);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:194` `OpenMenuPage(4);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:270` `OpenMenuPage(5);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:320` `OpenMenuPage(6);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:341` `OpenMenuPage(7);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:427` `OpenMenuPage(12 );`
  - … 5 more
- `other keyword`: 2 lines.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:172` `setstate(wildboarrespawn);`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:185` `state wildboarrespawn`

There is no script call in these closures whose text is `LoadMap`, `NextMap`, `Checkpoint`, `SaveGame`, or `LoadGame`. `Restart` / `RestartMission` / `OpenMenu` / `SaveGame` exist as ExportFnc names (section 2c, GameDll registrar `0xc2d40` and CommonDll `cGuiSaveMan::ScrExport` `0x102270`). Whether a script calls `Restart` is the table above, not an inference from the export.

Candidate C++ exports from `re/exports/*.demangled.txt` whose demangled name matches `Checkpoint`, `Save`, `Load`, `Restart`, `Respawn`, `Death`, `Die`, `NextMap`, or `Level`. These are names only (UNVERIFIED: not tied to a script call).

Matching demangled lines: 836.

Stronger subset (`Checkpoint`/`Respawn`/`NextMap`/`SaveGame`/`LoadGame`/`Restart`/`Death`/`Die`): 21.

- `CommonDll7.dll.demangled.txt`: `public: virtual void __thiscall cAIHuntBrain_Animal::Die(int)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cAIHuntBrain_Animal::IsDeathEnded(void)const`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::LoadGame(char const *)`
- `CommonDll7.dll.demangled.txt`: `public: virtual void __thiscall cObjCharacter::OnDie(void)`
- `CommonDll7.dll.demangled.txt`: `public: virtual void __thiscall cObjConveyor::OnDie(void)`
- `CommonDll7.dll.demangled.txt`: `public: virtual void __thiscall cObjHuman::OnDie(void)`
- `CommonDll7.dll.demangled.txt`: `public: virtual void __thiscall cObjRotating::OnDie(void)`
- `CommonDll7.dll.demangled.txt`: `public: virtual void __thiscall cObjScriptedEntity::OnDie(void)`
- `CommonDll7.dll.demangled.txt`: `public: virtual void __thiscall cObjTrain::OnDie(void)`
- `CommonDll7.dll.demangled.txt`: `public: virtual void __thiscall cObjTriggerHit::OnDie(void)`
- `CommonDll7.dll.demangled.txt`: `public: static void __cdecl cAIHuntBrain_Animal::OnPhysDeathCollision(void *,void *)`
- `CommonDll7.dll.demangled.txt`: `protected: void __thiscall cAIHuntBrain_Animal::ProcessAfterDeathState(void)`
- `CommonDll7.dll.demangled.txt`: `protected: void __thiscall cAIHuntBrain_Animal::ProcessDie(void)`
- `CommonDll7.dll.demangled.txt`: `protected: void __thiscall cAIHuntBrain_Animal::ProcessDieStuck(void)`
- `CommonDll7.dll.demangled.txt`: `protected: void __thiscall cAIHuntBrain_Animal::ProcessPhysDeathState(void)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::SaveGame(char const *)`
- `CommonDll7.dll.demangled.txt`: `public: void __thiscall cAIHuntBrain_Animal::SetPhysDeathImpulses(struct tVect3,struct tVect3)`
- `EngineDll7.dll.demangled.txt`: `void __cdecl ConsoleRestart(void)`
- `EngineDll7.dll.demangled.txt`: `public: virtual int __thiscall cObjEntity::HasPhysDeath(void)`
- `EngineDll7.dll.demangled.txt`: `void __cdecl RestartScript(void)`
- `EngineDll7.dll.demangled.txt`: `void __cdecl ScripterRestart(void)`

Remaining matches (name contains `Save`, `Load`, or `Level` but not the stronger tokens): 815. First 80:

- `CommonDll7.dll.demangled.txt`: `public: __thiscall cGuiSaveMan::cGuiSaveMan(class cGuiSaveMan const &)`
- `CommonDll7.dll.demangled.txt`: `public: __thiscall cGuiSaveMan::cGuiSaveMan(void)`
- `CommonDll7.dll.demangled.txt`: `public: __thiscall cSaveBuffer::cSaveBuffer(class cSaveBuffer const &)`
- `CommonDll7.dll.demangled.txt`: `public: __thiscall cSaveBuffer::cSaveBuffer(void)`
- `CommonDll7.dll.demangled.txt`: `public: virtual __thiscall cGuiSaveMan::~cGuiSaveMan(void)`
- `CommonDll7.dll.demangled.txt`: `public: virtual __thiscall cSaveBuffer::~cSaveBuffer(void)`
- `CommonDll7.dll.demangled.txt`: `public: class cGuiSaveMan & __thiscall cGuiSaveMan::operator=(class cGuiSaveMan const &)`
- `CommonDll7.dll.demangled.txt`: `public: class cSaveBuffer & __thiscall cSaveBuffer::operator=(class cSaveBuffer const &)`
- `CommonDll7.dll.demangled.txt`: `const cGuiSaveMan::`vftable'`
- `CommonDll7.dll.demangled.txt`: `const cSaveBuffer::`vftable'`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSkinBase::AMIsLoading(void)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cSaveBuffer::AddChunk(unsigned int,unsigned char *,unsigned int)`
- `CommonDll7.dll.demangled.txt`: `public: void __thiscall cSaveBuffer::AddData(unsigned char *,unsigned int)`
- `CommonDll7.dll.demangled.txt`: `public: static void * __cdecl cGuiSaveMan::Alloc(unsigned int)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::Autosave(void)`
- `CommonDll7.dll.demangled.txt`: `public: void __thiscall cGuiSaveMan::BeginSave(void)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::CheckAutosave(void)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::CheckBoot(void)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::CheckCard(void)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::CheckDelete(char const *)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::CheckFormat(void)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::CheckLoad(char const *)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::CheckSave(char const *)`
- `CommonDll7.dll.demangled.txt`: `public: static int __cdecl cSaveBuffer::ChunkCallback(void *,unsigned int,unsigned int,unsigned char *)`
- `CommonDll7.dll.demangled.txt`: `public: unsigned char * __thiscall cSaveBuffer::ChunkValid(struct tSaveChunk const *)`
- `CommonDll7.dll.demangled.txt`: `public: struct tSaveChunk * __thiscall cSaveBuffer::ChunkWalk(int (__cdecl*)(void *,unsigned int,unsigned int,unsigned char *),void *)`
- `CommonDll7.dll.demangled.txt`: `public: static class cBaseClass * __cdecl cGuiSaveMan::Create(void)`
- `CommonDll7.dll.demangled.txt`: `public: void __thiscall cGuiSaveMan::CreateDevice(char const *)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::DeleteCorrupt(void)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::DeleteGame(char const *)`
- `CommonDll7.dll.demangled.txt`: `public: virtual void __thiscall cGuiSaveMan::DestroyClass(void)`
- `CommonDll7.dll.demangled.txt`: `public: void __thiscall cSaveBuffer::Empty(void)`
- `CommonDll7.dll.demangled.txt`: `public: void __thiscall cGuiSaveMan::EndSave(void)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::FileExists(char const *)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::Format(void)`
- `CommonDll7.dll.demangled.txt`: `public: static void __cdecl cGuiSaveMan::Free(void *)`
- `CommonDll7.dll.demangled.txt`: `public: static void __cdecl cSaveBuffer::Free(void *)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIBrain::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIBrainCharacter::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIBrain_Human::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIFlyPath::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIHuntBrainEx::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIHuntSenseEx::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIHuntTaskEx::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAISense::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_FlyLookAt::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_FlyModelCamera::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_FlyOnPath::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_FlyTo::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_Follow::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_Idle::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_LookAt::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_Strafe::GameLoad(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIBrain::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIBrainCharacter::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIBrain_Human::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIFlyPath::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIHuntBrainEx::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIHuntSenseEx::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAIHuntTaskEx::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAISense::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_FlyLookAt::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_FlyModelCamera::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_FlyOnPath::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_FlyTo::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_Follow::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_Idle::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_LookAt::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: virtual int __thiscall cAITask_Strafe::GameSave(class cByteStream &)`
- `CommonDll7.dll.demangled.txt`: `public: unsigned char * __thiscall cSaveBuffer::GetBuffer(void)`
- `CommonDll7.dll.demangled.txt`: `public: struct tSaveChunk const * __thiscall cSaveBuffer::GetChunk(unsigned int)`
- `CommonDll7.dll.demangled.txt`: `public: unsigned char * __thiscall cSaveBuffer::GetChunksBuf(void)`
- `CommonDll7.dll.demangled.txt`: `public: unsigned char const * __thiscall cSaveBuffer::GetData(unsigned int,unsigned int &)`
- `CommonDll7.dll.demangled.txt`: `public: unsigned int __thiscall cSaveBuffer::GetDataSize(void)`
- `CommonDll7.dll.demangled.txt`: `public: char const * __thiscall cGuiSaveMan::GetFile(int)`
- `CommonDll7.dll.demangled.txt`: `public: char const * __thiscall cGuiSaveMan::GetFileInfo(char const *)`
- `CommonDll7.dll.demangled.txt`: `public: char const * __thiscall cGuiSaveMan::GetFileName(char const *)`
- `CommonDll7.dll.demangled.txt`: `public: int __thiscall cGuiSaveMan::GetFiles(void)`
- … 735 more in `re/exports/*.demangled.txt`

## 8. Companions / NPCs

Character `<instance>` members of the globals class (`cScrNPC`, `cScrCustomCharacter`, `cScrCustomCharacterEx`, `cScrHuman*`). Quotes are non-comment lines in the map closure that name that instance and call `StartTask`, `SetTaskParam`, `PlayMessage`, `PlaySound`, `Follow`, `MoveToPos`, or `LookTo`. `cScrHunter player` is the player bind, not an NPC.

- **MAP1**: `cScrCustomCharacterEx faketrap1`, `cScrNPC hugsm1`. Behavior lines: 62.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:21` `::globals.faketrap1.MoveToPos("capcana1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:23` `::globals.hugsm1.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:24` `::globals.hugsm1.SetTaskParam("target", "start1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:95` `::globals.hugsm1.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:96` `::globals.hugsm1.SetTaskParam("target_cutscene", "c3_2");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:100` `::globals.hugsm1.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:101` `::globals.hugsm1.SetTaskParam("target_cutscene", "c3_look1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:175` `::globals.hugsm1.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:176` `::globals.hugsm1.SetTaskParam("target", "start2");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:251` `::globals.hugsm1.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:252` `::globals.hugsm1.SetTaskParam("target", "start3");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:293` `::globals.hugsm1.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:294` `::globals.hugsm1.SetTaskParam("target", "start6");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:355` `::globals.faketrap1.MoveToPos("c4_1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:384` `::globals.hugsm1.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:385` `::globals.hugsm1.SetTaskParam("target_cutscene", "hunter");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:389` `::globals.hugsm1.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:390` `::globals.hugsm1.SetTaskParam("anim", "conversation");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:391` `::globals.hugsm1.PlaySound("M1OH26");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:395` `::globals.hugsm1.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION01.SCR:396` `::globals.hugsm1.SetTaskParam("anim", "conversation");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:32` `hugsm1.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:33` `hugsm1.SetTaskParam("anim", "cs01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:43` `hugsm1.PlaySound("M1OH01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP1/CUTSCENE1.SCR:44` `hugsm1.PlayMessage("Well, c'mon, youngster. Think you can show <key_time:2> a thing or two to Old Hugh, do ya? <key_time:2> Well, we'll just see about that. ...`
  - … 37 more lines
- **MAP2A**: `cScrCustomCharacterEx fallbridge`, `cScrNPC hugs2a`. Behavior lines: 77.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:44` `::globals.hugs2a.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:45` `::globals.hugs2a.SetTaskParam("target", "start1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:90` `::globals.hugs2a.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:91` `::globals.hugs2a.SetTaskParam("target", "PathNode2");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:192` `::globals.hugs2a.MoveToPos("c2_3");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:354` `::globals.hugs2a.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02A.SCR:355` `::globals.hugs2a.SetTaskParam("target", "start2");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:30` `hugs2a.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:31` `hugs2a.SetTaskParam("anim", "cs01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:41` `hugs2a.PlaySound("M2OH03");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:42` `hugs2a.PlayMessage("C'mon, pal, and keep a sharp eye.<key_time:2> We've got some rough terrain to cover,\|so we need to pack light. <key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:45` `hugs2a.PlaySound("M2OH01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:46` `hugs2a.PlayMessage("C'mon, young fellar.<key_time:1>If anyone can get these kin of mine<key_time:2> home in one piece it's you. <key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:51` `hugs2a.PlaySound("M2OH02");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE1.SCR:52` `hugs2a.PlayMessage("You hear that? Wolves!<key_time:2> Running all over 'round here.<key_time:2> I told that fool niece an' nephew o' mine <key_time:2> not t...`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:41` `hugs2a.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:42` `hugs2a.SetTaskParam("play_sa", "fallbridge");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:43` `hugs2a.SetTaskParam("play_sa_order", "fallbridge");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:44` `hugs2a.SetTaskParam("anim", "cs02");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:71` `hugs2a.MoveToPos("c2_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE2.SCR:94` `hugs2a.MoveToPos("c2_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE3.SCR:43` `hugs2a.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE3.SCR:44` `hugs2a.SetTaskParam("target_cutscene", "c3_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE3.SCR:48` `hugs2a.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2A/CUTSCENE3.SCR:49` `hugs2a.SetTaskParam("target_cutscene", "c3tent_1");`
  - … 52 more lines
- **MAP2B**: `cScrNPC hugs2b`. Behavior lines: 26.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:66` `::globals.hugs2b.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:67` `::globals.hugs2b.SetTaskParam("target", "start1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:279` `::globals.hugs2b.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:280` `::globals.hugs2b.SetTaskParam("target", "fire_hugh");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:284` `::globals.hugs2b.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:285` `::globals.hugs2b.SetTaskParam("target_cutscene", "fire_wolf1_3");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:293` `::globals.hugs2b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:294` `::globals.hugs2b.SetTaskParam("anim", "hurtsit");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:299` `::globals.hugs2b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:300` `::globals.hugs2b.SetTaskParam("anim", "hurtdown");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:935` `::globals.hugs2b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:936` `::globals.hugs2b.SetTaskParam("anim", "syncguidegetup");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1002` `::globals.hugs2b.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02B.SCR:1003` `::globals.hugs2b.SetTaskParam("target", "start2");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE1.SCR:36` `hugs2b.MoveToPos("c3_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE1.SCR:64` `hugs2b.PlayMessage("I reckon this here's where they made camp.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE1.SCR:69` `hugs2b.PlayMessage("Looks like we got some company.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:28` `hugs2b.MoveToPos("c4_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:36` `hugs2b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:37` `hugs2b.SetTaskParam("anim", "cs04");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:47` `hugs2b.PlaySound("M2OH07");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:48` `hugs2b.PlayMessage("Thanks again, kid.<key_time:1>Looks like you saved my old hide once again.<key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:51` `hugs2b.PlaySound("M2OH08");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:52` `hugs2b.PlayMessage("That blasted critter sure \|did a job on the ol' wooden leg.<key_time:3> I'm not gonna be able to move\|too quick after this.<key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2B/CUTSCENE2.SCR:55` `hugs2b.PlaySound("M2OH09");`
  - … 1 more lines
- **MAP2C**: `cScrNPC hugs2c`, `cScrNPC erin`, `cScrNPC danny`. Behavior lines: 38.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:32` `::globals.hugs2c.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:33` `::globals.hugs2c.SetTaskParam("target", "start1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:93` `::globals.hugs2c.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:94` `::globals.hugs2c.SetTaskParam("target", "end1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:96` `::globals.erin.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:97` `::globals.erin.SetTaskParam("target", "end1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:99` `::globals.danny.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION02C.SCR:100` `::globals.danny.SetTaskParam("target", "end1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:43` `hugs2c.MoveToPos("c3_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:44` `danny.MoveToPos("c3_4");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:45` `erin.MoveToPos("c3_2");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:51` `danny.StartTask("AITaskNPC_PlayAnim"); //begin wirake`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:52` `danny.SetTaskParam("play_sa", "erin");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:53` `danny.SetTaskParam("play_sa_order", "erin");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:54` `danny.SetTaskParam("anim", "cs06");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:57` `hugs2c.StartTask("AITaskNPC_PlayAnim"); //begin hugs2c`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:58` `hugs2c.SetTaskParam("anim", "cs06");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:66` `hugs2c.PlaySound("M2OH12");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:67` `hugs2c.PlayMessage("Looks like this is the end of the line.\|Betcha they're in here.<key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:70` `hugs2c.PlaySound("M2OH13");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:71` `hugs2c.PlayMessage("ERIN? DANNY?!<key_time:1> It's your old Uncle Hugh\|and I brought some help!<key_time:3> ");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:75` `erin.PlaySound("UH_03");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:76` `erin.PlayMessage("Uncle Hugh!<key_time:1>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:80` `hugs2c.PlaySound("M2OH14");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP2C/CUTSCENE1.SCR:81` `hugs2c.PlayMessage("I was worried you kids were goners.<key_time:2>");`
  - … 13 more lines
- **MAP3A**: `cScrNPC abby`. Behavior lines: 13.
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:33` `abby.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:34` `abby.SetTaskParam("anim", "cs01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:49` `abby.PlaySound("M3AP01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:50` `abby.PlayMessage("That was a close call.\|Not a bad landing, all things considered.<key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:55` `abby.PlayMessage("You okay?\|I twisted up my ankle pretty bad.<key_time:3>I don't think I'm going\|to be able to keep up too well.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE1.SCR:60` `abby.PlayMessage("Say, maybe you should go on ahead,<key_time:2>find some food and water, and\|a safe area where we can rest.<key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:27` `abby.MoveToPos("c2_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:34` `abby.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:35` `abby.SetTaskParam("anim", "cs02");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:57` `abby.PlaySound("M3AP04");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:58` `abby.PlayMessage("Well, this is a pretty good spot.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:63` `abby.PlayMessage("Look! That looks like a radio tower.<key_time:2> Why don't you hike over there and\|see if you can send an S.O.S.<key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3A/CUTSCENE2.SCR:68` `abby.PlayMessage("Maybe someone will hear it\|and take us out of this mess.<key_time:2>");`
- **MAP3B**: no NPC/custom-character instances in globals.
- **MAP3C**: `cScrNPC abby3c`. Behavior lines: 11.
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE1.SCR:44` `abby3c.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE1.SCR:45` `abby3c.SetTaskParam("anim", "cs03");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:36` `abby3c.MoveToPos("c2_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:43` `abby3c.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:44` `abby3c.SetTaskParam("anim", "cs04");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:56` `abby3c.PlaySound("M3AP08");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:57` `abby3c.PlayMessage("Hey! A 'copter.<key_time:1>Looks like your S.O.S.\|went through after all.<key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:61` `abby3c.PlaySound("M3AP09");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:62` `abby3c.PlayMessage("You know,<key_time:1>I wouldn't have made it if you\|hadn't returned when you did.<key_time:2>That's not something I'll ever forget.<key_t...`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:66` `abby3c.PlaySound("M3AP10");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP3C/CUTSCENE2.SCR:67` `abby3c.PlayMessage("Don't be a stranger. <key_time:1>Maybe after you've seen whoever you need to see,<key_time:2>and caught whatever it is you're hunting,<ke...`
- **MAP4A**: `cScrNPC tleader4a`. Behavior lines: 10.
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE1.SCR:34` `tleader4a.PlaySound("M4AF01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE1.SCR:35` `tleader4a.PlayMessage("Hamisi Acuna has not been the same since <key_time:3> he returned from his last hunting expedition.<key_time:3> Something terrible hap...`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE1.SCR:41` `tleader4a.PlaySound("M4AF02");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE1.SCR:42` `tleader4a.PlayMessage("If you want to speak to Hamisi,\|we must make haste.<key_time:4>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:33` `tleader4a.PlaySound("M4AF06");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:34` `tleader4a.PlayMessage("This is as far as I can take you.<key_time:3> You must make the rest of the journey\|on your own.<key_time:4>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:38` `tleader4a.PlaySound("M4AF07");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:39` `tleader4a.PlayMessage("But be very careful.<key_time:2> Death hides in the bush,<key_time:2>and lurks around every bend.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:43` `tleader4a.PlaySound("M4AF08");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4A/CUTSCENE2.SCR:44` `tleader4a.PlayMessage("Good luck! May the gods protect you!<key_time:3>");`
- **MAP4B**: `cScrNPC fiancee4b`. Behavior lines: 14.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:21` `::globals.fiancee4b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:22` `::globals.fiancee4b.SetTaskParam("anim", "idle");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:459` `::globals.fiancee4b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:460` `::globals.fiancee4b.SetTaskParam("anim", "idle");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:523` `::globals.fiancee4b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:524` `::globals.fiancee4b.SetTaskParam("anim", "idle");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:585` `::globals.fiancee4b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION04B.SCR:586` `::globals.fiancee4b.SetTaskParam("anim", "idle");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE5.SCR:46` `fiancee4b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE5.SCR:47` `fiancee4b.SetTaskParam("anim", "cs05");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE6.SCR:47` `fiancee4b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE6.SCR:48` `fiancee4b.SetTaskParam("anim", "cs05");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE7.SCR:47` `fiancee4b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP4B/CUTSCENE7.SCR:48` `fiancee4b.SetTaskParam("anim", "cs05");`
- **MAP5**: `cScrNPC hamisi5`, `cScrNPC poachera5`. Behavior lines: 29.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:16` `::globals.hamisi5.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION05.SCR:17` `::globals.hamisi5.SetTaskParam("target", "start1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:35` `hamisi5.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:36` `hamisi5.SetTaskParam("anim", "cs01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:48` `hamisi5.PlaySound("M5HA01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:49` `hamisi5.PlayMessage("This is how we hunt in my village.\|Blow darts. But be careful!<key_time:3> These darts have a powerful\|tranquilizer on their tips.<key_t...`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:53` `hamisi5.PlaySound("M5HA02");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:54` `hamisi5.PlayMessage("There's enough sedative on them to put\|down the angriest elephant or rhino,<key_time:3> but if you even nick yourself,<key_time:2> it'll...`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:58` `hamisi5.PlaySound("M5HA03");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE1.SCR:59` `hamisi5.PlayMessage("Tread cautiously and quietly. It is best\|not to attract the attention of the elephants.<key_time:4> They often react violently to intrud...`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:44` `hamisi5.MoveToPos("c3_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:50` `poachera5.StartTask("AITaskNPC_PlayAnim"); //begin wirake`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:51` `poachera5.SetTaskParam("play_sa", "hamisi5");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:52` `poachera5.SetTaskParam("play_sa_order", "hamisi5");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:53` `poachera5.SetTaskParam("anim", "cs03");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:68` `poachera5.PlayMessage("What was that?<key_time:1>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:72` `poachera5.PlayMessage("Did you hear that?<key_time:1>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:76` `poachera5.PlayMessage("Aaaah...Just the wind!<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:80` `poachera5.PlayMessage("Oy! What's that noise?<key_time:2> You there!\|You're not supposed to be here!<key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:95` `hamisi5.PlayMessage("That idiot set off a stampede.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:98` `hamisi5.PlayMessage("And the elephants are heading toward my village!\|We need to stop them!<key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:101` `hamisi5.PlayMessage("Looks like somebody left a tranq rifle in there.\|Good thing for us.<key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:104` `hamisi5.PlayMessage("I'll drive. You shoot.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:115` `poachera5.MoveToPos("ci_3");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP5/CUTSCENE3.SCR:169` `poachera5.MoveToPos("ci_3");`
  - … 4 more lines
- **MAP6**: `cScrNPC reggie6`, `cScrNPC redshirt1`, `cScrNPC redshirt2`, `cScrNPC redshirt3`. Behavior lines: 46.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:18` `::globals.reggie6.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:19` `::globals.reggie6.SetTaskParam("target", "start1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:81` `::globals.reggie6.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:82` `::globals.reggie6.SetTaskParam("target", "PathNode37");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:149` `::globals.reggie6.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:150` `::globals.reggie6.SetTaskParam("target", "PathNode52");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:208` `::globals.reggie6.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:209` `::globals.reggie6.SetTaskParam("target", "start2");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:377` `::globals.reggie6.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:378` `::globals.reggie6.SetTaskParam("target_cutscene", "hunter");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:379` `::globals.reggie6.PlaySound("M6RD07");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:383` `::globals.reggie6.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:384` `::globals.reggie6.SetTaskParam("anim", "conversation");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:388` `::globals.reggie6.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION06.SCR:389` `::globals.reggie6.SetTaskParam("anim", "conversation");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:34` `reggie6.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:35` `reggie6.SetTaskParam("anim", "cs01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:48` `reggie6.PlaySound("M6RD01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:49` `reggie6.PlayMessage("Beautiful beasts, those bengals, beautiful...<key_time:2> and deadly.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:53` `reggie6.PlaySound("M6RD02");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE1.SCR:54` `reggie6.PlayMessage("Watch yourself.<key_time:1> These kitties are quick as lightning,<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE2.SCR:35` `reggie6.MoveToPos("c2_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE2.SCR:57` `reggie6.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE2.SCR:58` `reggie6.SetTaskParam("anim", "cs02");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP6/CUTSCENE2.SCR:70` `reggie6.PlaySound("M6SC01");`
  - … 21 more lines
- **MAP7**: `cScrNPC reggie7`, `cScrNPC indie_girl`. Behavior lines: 51.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:21` `::globals.reggie7.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:22` `::globals.reggie7.SetTaskParam("target", "start1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:118` `::globals.reggie7.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:119` `::globals.reggie7.SetTaskParam("target", "start72");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:189` `::globals.reggie7.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:190` `::globals.reggie7.SetTaskParam("target_cutscene", "c2_1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:195` `::globals.reggie7.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:196` `::globals.reggie7.SetTaskParam("target_cutscene", "capcanalook1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:202` `::globals.reggie7.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:203` `::globals.reggie7.SetTaskParam("target", "start152");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:291` `::globals.indie_girl.PlaySound("call_for_help2");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:296` `::globals.reggie7.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:297` `::globals.reggie7.SetTaskParam("target_cutscene", "hunter");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:306` `::globals.reggie7.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:307` `::globals.reggie7.SetTaskParam("target", "start35");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:354` `::globals.indie_girl.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:355` `::globals.indie_girl.SetTaskParam("target_cutscene", "cobras_t7_01");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:400` `::globals.indie_girl.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:401` `::globals.indie_girl.SetTaskParam("anim", "jump");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:406` `::globals.reggie7.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:407` `::globals.reggie7.SetTaskParam("target", "start42");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:409` `::globals.indie_girl.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:410` `::globals.indie_girl.SetTaskParam("target", "startgirl");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:460` `::globals.reggie7.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION07.SCR:461` `::globals.reggie7.SetTaskParam("target", "reggiepath1");`
  - … 26 more lines
- **MAP8**: `cScrNPC tribesman1`, `cScrNPC tribesman2`, `cScrNPC tribesman3`. Behavior lines: 31.
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:55` `tribesman1.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:56` `tribesman1.SetTaskParam("target_cutscene", "hunter");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:60` `tribesman1.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:61` `tribesman1.SetTaskParam("target_cutscene", "c2_4");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:65` `tribesman1.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:66` `tribesman1.SetTaskParam("anim", "call");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:70` `tribesman1.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:71` `tribesman1.SetTaskParam("target_cutscene", "hunter");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:75` `tribesman1.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:76` `tribesman1.SetTaskParam("anim", "followmeright");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:80` `tribesman1.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:81` `tribesman1.SetTaskParam("target_cutscene", "c2_4");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:86` `tribesman1.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:87` `tribesman1.SetTaskParam("target_cutscene", "c2_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:88` `tribesman2.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:89` `tribesman2.SetTaskParam("target_cutscene", "c2_2");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:91` `tribesman3.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:92` `tribesman3.SetTaskParam("target_cutscene", "c2_3");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:111` `tribesman2.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:112` `tribesman2.SetTaskParam("target_cutscene", "cover_t10_2");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:116` `tribesman2.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE2.SCR:117` `tribesman2.SetTaskParam("anim", "point");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:36` `tribesman1.MoveToPos("c3_4");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:37` `tribesman2.MoveToPos("c3_5");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP8/CUTSCENE3.SCR:38` `tribesman3.MoveToPos("c3_6");`
  - … 6 more lines
- **MAP9**: `cScrNPC reggie9`. Behavior lines: 33.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:232` `::globals.reggie9.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:233` `::globals.reggie9.SetTaskParam("target", "c3_1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:317` `::globals.reggie9.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:318` `::globals.reggie9.SetTaskParam("target_cutscene", "c5_1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:323` `::globals.reggie9.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:324` `::globals.reggie9.SetTaskParam("anim", "radiotalk");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:328` `::globals.reggie9.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:329` `::globals.reggie9.SetTaskParam("target_cutscene", "hunter");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:426` `::globals.reggie9.StartTask("AITaskNPC_LookToEntity");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:427` `::globals.reggie9.SetTaskParam("target_cutscene", "hunter");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:432` `::globals.reggie9.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION09.SCR:433` `::globals.reggie9.SetTaskParam("anim", "conversation");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:27` `reggie9.MoveToPos("c1_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:33` `reggie9.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:34` `reggie9.SetTaskParam("anim", "cs01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:47` `reggie9.PlaySound("M9RD01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:48` `reggie9.PlayMessage("Blast it all.<key_time:1> I'm afraid my ankle has \|a nasty sprain from the fall.<key_time:3>Don't worry, I'll keep up...\|as best I can, ...`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:52` `reggie9.PlaySound("M9RD02");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE1.SCR:53` `reggie9.PlayMessage("Oh, this is bleak.<key_time:1> This is just the sort of cave\|where dingoes like to roam. <key_time:4>It'd be nice if we had some sort of...`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:31` `reggie9.MoveToPos("c2_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:38` `reggie9.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:39` `reggie9.SetTaskParam("anim", "cs02");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:52` `reggie9.PlaySound("M9RD04");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:53` `reggie9.PlayMessage("This reminds me a bit of a hedge maze\|I used to explore as a boy in Wiltshire.<key_time:4> Without the rampaging dingoes, of course.<key...`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP9/CUTSCENE2.SCR:59` `reggie9.PlaySound("M9RD10");`
  - … 8 more lines
- **MAP10**: `cScrNPC wirake10`, `cScrNPC reggie10`. Behavior lines: 45.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:23` `::globals.wirake10.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:24` `::globals.wirake10.SetTaskParam("target", "PathNode71");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:92` `::globals.wirake10.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:93` `::globals.wirake10.SetTaskParam("target", "PathNode73");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:188` `::globals.wirake10.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:189` `::globals.wirake10.SetTaskParam("target", "PathNode81");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION10.SCR:485` `::globals.reggie10.MoveToPos("c5_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:43` `wirake10.StartTask("AITaskNPC_PlayAnim"); //begin wirake`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:44` `wirake10.SetTaskParam("play_sa", "reggie10");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:45` `wirake10.SetTaskParam("play_sa_order", "reggie10");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:46` `wirake10.SetTaskParam("anim", "cs01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:65` `reggie10.PlaySound("M10RD01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:66` `reggie10.PlayMessage("Cor! What a dreary, desolate place.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:68` `wirake10.PlaySound("M10W03");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:69` `wirake10.PlayMessage("Pumas up ahead. Lots of them.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:72` `wirake10.PlaySound("M10W04");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:73` `wirake10.PlayMessage("I can tell by the tracks.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:76` `reggie10.PlaySound("M10RD02");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:77` `reggie10.PlayMessage("Wirake, for heaven's sake,\|slow down.<key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:80` `wirake10.PlaySound("M10W05");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE1.SCR:81` `wirake10.PlayMessage("Slow down? <key_time:1> If I was going any slower,\|I wouldn't be moving at all.<key_time:3>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE4.SCR:37` `reggie10.MoveToPos("c4_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE4.SCR:38` `wirake10.MoveToPos("c4_2");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE4.SCR:68` `reggie10.MoveToPos("c5_1");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP10/CUTSCENE4.SCR:72` `wirake10.MoveToPos("c4_1");`
  - … 20 more lines
- **MAP11A**: `cScrNPC siberian1`, `cScrNPC siberian2`, `cScrNPC siberian3`, `cScrNPC siberian4`. Behavior lines: 0.
- **MAP11B**: `cScrNPC reggie11b`. Behavior lines: 17.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:22` `::globals.reggie11b.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:23` `::globals.reggie11b.SetTaskParam("target", "start1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:145` `::globals.reggie11b.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:146` `::globals.reggie11b.SetTaskParam("target", "start2");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:173` `::globals.reggie11b.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION11B.SCR:174` `::globals.reggie11b.SetTaskParam("target", "PathNode20");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:31` `reggie11b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:32` `reggie11b.SetTaskParam("anim", "cs03");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:42` `reggie11b.PlaySound("M11bRD04");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:43` `reggie11b.PlayMessage("We'll have to go on foot from here.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:46` `reggie11b.PlaySound("M11bRD05");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE1.SCR:47` `reggie11b.PlayMessage("I do confess,<key_time:1>I vastly preferred facing these boars<key_time:2>from the safety of our helicopter.<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE2.SCR:35` `reggie11b.MoveToPos("cs4_reggie");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE2.SCR:41` `reggie11b.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE2.SCR:42` `reggie11b.SetTaskParam("anim", "cs04");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE2.SCR:56` `reggie11b.PlayMessage("Look at that Hog! Why, it's a Hogzilla!<key_time:2>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP11B/CUTSCENE2.SCR:59` `reggie11b.PlayMessage("That's a real Hogzilla!<key_time:1>");`
- **MAP11C**: `cScrNPC siberian1`, `cScrNPC siberian2`, `cScrNPC siberian3`, `cScrNPC siberian4`. Behavior lines: 0.
- **MAP12A**: `cScrNPC reggie12a`, `cScrNPC wirake12a`. Behavior lines: 58.
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:37` `::globals.wirake12a.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:38` `::globals.wirake12a.SetTaskParam("target", "start1");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:98` `::globals.wirake12a.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:99` `::globals.wirake12a.SetTaskParam("target", "PathNode14");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:195` `::globals.reggie12a.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:196` `::globals.reggie12a.SetTaskParam("anim", "hurtdown");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:261` `::globals.reggie12a.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:262` `::globals.reggie12a.SetTaskParam("anim", "hurtdown");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:265` `::globals.wirake12a.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:266` `::globals.wirake12a.SetTaskParam("target", "PathNode30");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:307` `::globals.wirake12a.StartTask("AITaskNPC_MoveToPos");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:308` `::globals.wirake12a.SetTaskParam("target", "start2");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:491` `::globals.wirake12a.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:492` `::globals.wirake12a.SetTaskParam("anim", "hurtdown");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:583` `::globals.wirake12a.StartTask("AITaskNPC_PlayAnim");`
  - `re/extracted/SCRIPTS/TRIGGERS/TRIGGERMISSION12A.SCR:584` `::globals.wirake12a.SetTaskParam("anim", "hurtdown");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:33` `wirake12a.StartTask("AITaskNPC_PlayAnim"); //begin wirake`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:34` `wirake12a.SetTaskParam("play_sa", "reggie12a");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:35` `wirake12a.SetTaskParam("play_sa_order", "reggie12a");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:36` `wirake12a.SetTaskParam("anim", "cs01");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:51` `reggie12a.PlayMessage("C'mon, you lot!<key_time:1> We've got a lot of territory\|to cover, and not much time.<key_time:4>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:56` `reggie12a.PlayMessage("Hurry! We can't let ourselves be\|bested by some low - life poacher.<key_time:4>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:60` `reggie12a.PlayMessage("Watch for traps. <key_time:1>There's no telling what surprises that snake\|Benedek has left behind to trip us up.<key_time:4>");`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE1.SCR:64` `wirake12a.PlayMessage("This way. The poachers went this way.\|And the bear.<key_time:3> And... hmm, this is odd, something else.\|Never seen those kinds of tra...`
  - `re/extracted/SCRIPTS/CUTSCENE/MAP12A/CUTSCENE2.SCR:34` `reggie12a.MoveToPos("c2_1");`
  - … 33 more lines
- **MAP12B**: no NPC/custom-character instances in globals.
- **MAP12C**: `cScrNPC mountainguide12c`. Behavior lines: 0.
- **MAPARENA**: no NPC/custom-character instances in globals.
- **MAPISLAND**: no NPC/custom-character instances in globals.
- **MAPTSAVO**: no NPC/custom-character instances in globals.
- **MAPMENU**: no NPC/custom-character instances in globals.

## 9. Summary for the lead

- Player-reference lines in story+bonus closures: 1581. Kinds: player flags 419, task target 341, trigger tests player 317, camera/cutscene 222, bind player entity 98, player method 98, inventory/weapon 61, damage/health 13, distance check 8, teleport/move 4.
- Cutscene scripts named by story+bonus MAIN.SPJ: 74 on 20 maps. Cutscene files on disk: 83. Files containing `state Skip`: 83.
- QTE/Struggle/Melee/Button/CloseCombat/Mash/QuickTime: no code hits. Code hits for the search words: 14 (knife distance, knife equip, `Pumafight` anim state, knife class names). String/comment hits are dialogue or comments (section 5).
- Maps with a globals NPC or custom-character instance: 19 of 25.
- No `LoadMap` / `NextMap` / `Checkpoint` / `SaveGame` / `LoadGame` call in the script closures (section 7).
- Fewest cutscenes, then fewest NPC instances, then fewest map-local player lines: MAP3B (cutscenes 0, npc instances 0, map-local player lines 22, tasks 2); MAP12B (cutscenes 1, npc instances 0, map-local player lines 15, tasks 1); MAP12C (cutscenes 1, npc instances 1, map-local player lines 13, tasks 1); MAP4A (cutscenes 2, npc instances 1, map-local player lines 36, tasks 3); MAP11B (cutscenes 2, npc instances 1, map-local player lines 41, tasks 7).
- Other end of that order: MAP8 (cutscenes 5, npc instances 3, map-local player lines 139, tasks 10); MAP6 (cutscenes 5, npc instances 4, map-local player lines 132, tasks 8); MAP2A (cutscenes 6, npc instances 2, map-local player lines 82, tasks 6); MAP10 (cutscenes 6, npc instances 2, map-local player lines 112, tasks 11); MAP12A (cutscenes 7, npc instances 2, map-local player lines 98, tasks 8).
- Cutscenes start with `ExecuteScript("cScrCutScene…")` from trigger scripts. Inside the cutscene script the player calls are `SetPFlags(PLAYER_INCUTSCENE)`, `ActivateCutSceneMode`, `ClearPFlags`, `DeactivateCutSceneMode`, plus camera `BeginCamera` / `EndCamera` (section 5).
- Task definitions are `m_pMission.CreateTask` blocks in `MISSIONS\*.SCR`. Scripts advance them with `UpdateMissionStatus` and `ActivateTriggerOnEnd` / `ActivateTriggerOnFail` (section 6). `CompleteTask` and `GlobalsCheckMissionStatus` counts are in section 3.

## Proposed MEMORY facts

- GameDll7 `FUN_1000ba30` (RVA `0xba30`) is the global script-native registrar (`cExportKey::ExportFnc` + `cBaseClass::s_ExportStaticFunction`). Evidence: `re/notes/lead-natives.c` and the same function in `re/notes/story-natives-gamedll.c`. Already recorded in MEMORY.md; repeated here only as the index for section 2a.
- CommonDll7 class method natives are registered by `ScrExport` functions that call `cExportKey::ExportFnc` then `cBaseClass::s_ExportFunction`. Evidence: `re/notes/story-natives-commondll.c` headers, including `cScrCharacter::ScrExport` RVA `0x10700` (`StartTask`, `SetTaskParam`, `ForceBrainActivate`, `LockInput`, `BrainFreeze`) and `cScrHuman::ScrExport` RVA `0xeae0` (`SetHumanSize`, `IsCrouched`, `SetCrouched`).
- Those CommonDll `ScrExport` functions copy the parent class name into the export block, not the leaf class: `cScrHuman::ScrExport` copies `s_cScrCharacter` with `AllocMem(0xe)` (14 bytes). Evidence: `re/notes/story-natives-commondll.c` lines 24–29.
- GameDll7 has further `ExportFnc` registrars besides `0xba30` (25 callers in the slot W dump, listed in section 2b). Unnamed `FUN_` registrars store a parent-class string the same way; the leaf class is not named in that decompilation.
- Story map globals bind the player with `setidx(player, FindEntScrObject("hunter"))` on a `cScrHunter player <instance>` member. Evidence: `re/extracted/SCRIPTS/LEVELS/MAP1/WORLD.SCR` lines 59 and 106–108 (`InitPlayer`). Other maps use the same call where section 4 lists `FindEntScrObject`.

