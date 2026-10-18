# Verified facts — engine and decompilation learnings

Format: fact — evidence. Add only verified items; hypotheses go in `re/notes/`. Mod/build/tooling learnings go in `docs/mod-learnings.md`; current progress in `STATUS.md`.

## Build identity
- Version: `DNGH2 (build 434 - Nov  1 2005)`, game/interface version 1.0 — `Bin\debug.log` lines 1, 19-20.
- Engine self-name: "Chameleon Engine" (c) 2000-2005 — `debug.log` line 2.
- Original PDB paths: `c:\2002\SRC\Game\Mod\DNGH2\Main\Out\Release_VC7\Main7.pdb`, `...\Game\EngineDll\Out\Release_VC7\EngineDll7.pdb`, `...\Game\CommonDll\Out\Release_VC7\CommonDll7.pdb` — PE debug directory. Compiler: MSVC 7.0 (msvcr70/msvcp70).

## SHA-256
| File | SHA-256 |
|---|---|
| Bin\Main7.exe | F9C1331EFCCCED45F196EC969966F2662D489EAD900AF5A64A5B40FAA7DCD71F |
| Bin\EngineDll7.dll | C36D18D74993383CA3FC7D842A9E464050C732D5AE9EB05BE898185966D8D562 |
| Bin\CommonDll7.dll | 723C29D1D69AFD2DC258093A2ACF9D06147E806F9B4241E7663AB815710DBAFE |
| Data\GameDll7.dll | 0DE1D8AFC3025F1E76186E3C5178316C5B69680E6C57FE4672D1654B82F80874 |
| Bin\binkw32.dll | DE7820F6B557C60F250146E863579C9D1A0673DC92E2110829031A5783F4245C |

## Module layout
- Load order: `InitEngineDll`, `InitCommonDll`, then `.\..\data\GAMEDLL7.DLL` — `debug.log` 4-5, 19.
- Home path `.\..\data\`; assets in `Data\data.arc` (294 MB, entries begin with zlib header `78 9C`), `shaders.mtp` multi-file (98 files) — `debug.log` 7, 17; hex dump.
- Exports: EngineDll7 8587 entries / 8192 distinct names (395 entries have no name; 571 classes), CommonDll7 3174 (199), GameDll7 133 (12) — PE export directory, `re/exports/*.txt`; demangled in `re/exports/*.demangled.txt`.
- Multiplayer surface exists in retail: Main7.exe strings `sv_multiplayer`, `sv_port`, `sv_password`, `sv_fps`, `sv_changeMap`, `cl_join %s`, `cl_playerName`, `cl_password`, `cl_disconnect`, `cfLobby`, switch `-join`; EngineDll7 `-dedicated` and debug strings for `cLocalDriver::*` and `cRdpDriver::*` (Listen, Connect, RequestServerList, GetServerInfo, RxTx); GameDll7 `-spmap` — `re/notes/strings/*.txt`. Whether they work end-to-end: unverified.
- EngineDll7 imports: `d3d8!Direct3DCreate8`, `d3d9!Direct3DCreate9`, `DINPUT8!DirectInput8Create`, `WS2_32` (socket/bind/sendto/recvfrom/select), `binkw32`, `MSACM32`, `WINMM` — pefile.
- No useful RTTI (only exception classes); names come from exports instead.

## Engine subsystems seen
- Net: `cNetServer`, `cNetClient`, `cNetGameState`, `cNetClientInfo` exported from EngineDll7.
- Internal client/server: `[SERVER] loading time : 156ms (map : 78ms, client: 78ms)`; Lua state `SpServer` — `debug.log` 68, 74.
- Lua: `EngLuaExecuteScript`, `EngLuaExecuteBuffer` exported; Lua states MaterialManager, resource manager, FxManager-Define, SpServer, DecalManager.
- Maps are BSP with chunked format (`CHUNK_ENTITIES`, `CHUNK_NEWPATHGRID`, ...). Menu map: `menumap`.
- AI (CommonDll7): `cAIBrain`, `cAIBrain_Human`, `cAIHuntBrain_Animal`, `cAIHuntBrainEx`, `cAIHuntSenseEx`, `cAISense_See`. World: `cObjWorld`, `cObjBaseEntity`, `cObjScriptedEntity`, `cObjHuntAreaAIEntity`.
- GameDll7: `cHunterProfile`, `cMOD_WeaponDef`, `cMOD_AmmoDef`, `cMOD_ItemDef`, `cMOD_MedDef`, `cMOD_RadioDef`.

## Launch and runtime
- Correct command line: `Bin\Main7.exe +mod data` with working dir `Bin`. Without `+mod data`, home path falls back to `.\..\DNGH2\` and assets fail to load — `Launcher.exe` string, sandbox `debug.log` 2026-10-15.
- `Launcher.exe` also contains `-cheats -developper` (sic) flags; console config `Cfg\console.cfg` is probed at startup. Effect of the flags: unverified.
- Registry key used by launcher: `SOFTWARE\Activision Value\Magic Wand\Dngh2`; Main7 also reads `Software\2002EngineTools`. Missing registry is non-fatal ("Registry error: unable to open registry !").
- `EngineDll7.dll` relocates at runtime (seen at 0x00CA0000, not 0x10000000) — FOV fix log. Use RVAs/patterns only.
- ASI route works: Ultimate ASI Loader v9.7.4 as `Bin\dinput8.dll` loads `.asi` files from `Bin\` — FOV fix v1.1 logged "loaded" and resolved `EngineDll7.dll+2fe07` (FOV) and `+2fe1d` (aspect).
- Disc image must be mounted (Virtual CloneDrive, `VCDMount.exe <ccd>`) before launch.

## Net layer (Track A, `re/notes/net-layer.md`; reviewed 2026-10-16)
- Single player is NOT client/server over a socket or loopback. The "server" is `cSpServer` inside Main7.exe (ctor RVA `0x1b390`, `StartGame` `0x1ed80`, `StartListen` `0x1ba40`, `ClJoin` `0x1e410`, `ClientJoinGame` `0x1e0c0`): one in-process object that loads the map, spawns the hunter via GameDll (`MOD_SvCreatePlayerEntity`/`MOD_SvSpawnPlayerEntity`) and passes bytes between its own `cByteStream`s. `[SERVER] loading time ... client: N` measures `ClJoin`, not a network client.
- `cSpServer` state at `+0x28`: 0 NotStarted, 1 Listening, 2 GameStarting, 3 GameStarted (table Main7 RVA `0x3bae0`). Client-info state `+0x1fd4`: 0 Joining … 3 GameStarted, ctor writes 4 (table `0x3bb04`). `Update` (`0x1bcd0`) ticks the world only in state 3.
- `cNetServer`/`cNetClient`/`cObjManager::SetDriverType` (`0xaad50`) are exported by EngineDll7 but imported by no other binary and called nowhere; `m_nDriverType` (RVA `0x5826e0`) stays 0 — lead re-checked imports with pefile; probe shows `driverType=0`.
- No binary imports `WSAStartup` (lead re-checked). Any use of engine sockets needs our ASI to call it first.
- `cRdpDriver` (vtable `0x43e844`, unnamed class) is a working reliable-UDP transport: opcodes `0x01` connect … `0x0b` server info, reliable/unreliable payloads `0x04`/`0x09` with seq + u16 length. Payload is an opaque byte pipe; no entity replication exists anywhere (`cObjEntity::ClientSpawn` `0xabc90` and `ClientCommand` `0xb9480` are empty).
- `sv_multiplayer` (default 0) is never branched on; `StartListen` writes `"0"` into it each map load. `sv_port` default 27318 (`0x6ab6`), read only by `GetListenPort` (`0x21610`, no callers). `cl_join` has no console handler. `-join`/`-dedicated` let a second process pass the `"GI UNIQUE MUTEX"` single-instance check (`FUN_00401dd0`, Main7 `0x1dd0`); `-dedicated` also sets `r_api 0; snd_usesound 0` (no rendering).
- Runtime (Track E, 2026-10-16): no UDP endpoint is bound in baseline, `-dedicated`, `-join 127.0.0.1`, `sv_multiplayer 1` via `Cfg\console.cfg`, or `-map` runs — `re/notes/runtime-*-udp.txt`. `-map` execs a nonexistent console symbol (`[CONSOLE ] symbol 'map' not found!`).
- GameDll `MOD_SvParseMessage` (GameDll7 `0x3140`) handles only in-process message ids `0x87` and `0xff`.

## Assets (Track C, `re/notes/assets.md`; reviewed 2026-10-16)
- `data.arc`: 2048-byte sectors, 24-byte footer `ARC\0` v`0x100`, zlib directory of `ARCH` records; 9,612 entries, `arc_extract.py --verify` 0 problems. Extract = 9,710 files (+98 from `shaders.mtp`, magic `MULA`). Format spec in `re/scripts/arc_extract.py` docstring.
- File lookup order (`cFileManager::Open` EngineDll7 `0x33e230`): home dir `.\..\data\` first, then `*.ARC`, then `*.ZIP`. **Loose files under `game\Data\` override the archive** — confirmed at runtime: a loose `Data\Cfg\console.cfg` was read and rewritten by the game (`re/notes/runtime-cfg-file-after.txt`).
- `Cfg\console.cfg` syntax: `addvar <name> <type>; <name> = <value>` (loaded by `InitConsole` `0x329ec0` → `cConsole::Load` `0x1672c0`).
- Two script systems: custom VM (`ScrVMCore.cpp`; `.SCR` text source, `.SPJ` project list, `.XSC` bytecode magic `46 53 01 01`, version 3 required by `cVMachine::LoadInfo` `0x287950`) and Lua 4.0 (`.OUT` bytecode `1B 4C 75 61 40`, 98 files). `.SCRIPT/.FX/.LUA/.INC/.SND` are text.
- Maps: 29 BSPs. Free-hunt: `HUNTMAP1` Africa (missions id 13), `HUNTMAP2` Australia (14), `HUNTMAP3` Alaska (15), `HUNTMAP4` Argentina (16). Bonus: `MAPARENA` (17), `MAPISLAND` (18), `MAPTSAVO` (19). Story: `MAP1`…`MAP12C`. Each BSP's `world_properties` points to `scripts/levels/<map>/main.spj`, class `cMapTest`; hunter spawn entity `info_spawn_point`/`human`.
- Animals are placed by level script: `WORLD.SCR` → `AddAnimal(ANIMAL_<SPECIES>, TRUE, "<bsp entity name>")`; species brains/health in `SCRIPTS\MODELS\ANIMALS\`. Free-hunt maps place 4 prey + 1 predator (HUNTMAP4: 4 red deer + 1 puma; 2 spawn points).
- No multiplayer/lobby content exists in the archive (no `sv_multiplayer`, `cl_join`, `lobby`, `player2` in scripts/GUI/BSP text).
- **`.AP` animation packs** (Grok p3-anim-names, lead-reviewed 2026-10-16; `re/notes/anim-names.md`): magic `FBAF`, version `0x0411`; clip names = length-prefixed ASCII after each `ModelAnimBone` record in the `E8 EF` chunk (list stored N times, copies identical). Lister: `py re\scripts\ap_list.py <file.AP>`.
- **NPC locomotion clips** (17 NPC `GAME.AP`s share the same 332 names: ABBY, ABO1-3, AUSRANGER1/2, BENEDECK, MOUNTAINGUIDE, OLDHUGH, REGINALD(_W), SIBERIAN1/2, SURVIVOR, TRIBALLEADER, WIRAKE(_W)): standing idle **`idlenormalneutral`** (`idle` is a different clip; `GUIDELEGS.SCR` never plays it), `walk/forward`, `walk/backward`, `walk/left`, `walk/right`, `run/forward`; no run-back/strafe or crouch clip (scripts reuse walk clips); death `fall` then `liedead`. Armed variants live in `SHOTGUN1.AP`/`HANDGUN3.AP` as prefixed names (`shotgun1/walkforward`, `shotgun1/run`, …); NPC `RIFLE1.AP` has no locomotion. POACHER `GAME.AP` = `blink` only (unusable as a puppet body). Player `TEENM\BODY\GAME.AP` (44 clips) is the first-person pack.

## Object model (Track B, `re/notes/object-model.md`, structs in `mod/src/game/types_draft.h`; reviewed 2026-10-16)
RVAs: EngineDll7 unless noted (CommonDll7 = "C", GameDll7 = "G").
- `cObjEntity` size `0x194` (`CreatecObjEntity` `0xacda0`). Id/index int16 `+0x04`; manager ptr `+0xB4`; model ptr `+0xE4`; **world position tVect3 `+0xE8`** (lead re-checked: `cObjEntity::SetPos` `0xbc130` writes `+0xE8/+0xEC/+0xF0`, local pos `+0x0C`); phys ptr `+0x118`.
- `cObjScriptedEntity` (C): health float `+0x19C` (ctor 100.0, `GetHealth` C `0x1b30`); dead = props bit `0x400` at `+0xC0`.
- Hunt characters derive from `cObjCharacterEx` (C, brain ptr `+0x200`, `GetBrain` `0x14920`); legacy `cObjCharacter` (brain `+0x210`, team `+0x214`) is a separate tree. `cObjHumanEx::IsPlayer` = bit 3 of `+0x3B4`.
- Game classes (G): animal `cScrAnimal` size `0x71c` vtable G `0xff470`; player `cScrPlayer` `0x4e8` vtable G `0x10a2e0`; playable hunter `cScr_ModHunter` `0x838` vtable G `0x106ff0` (derives from cScrPlayer, profile ptr `+0x1F8`).
- **Per-animal AI freeze:** `cAIHuntBrain_Animal::FreezeBrain` (C `0x158a0`) writes brain byte `+0x79` (lead re-checked); brain `Update` (C `0x95530`) returns early when set, before senses/pathfinding/body commands; animation (`cObjEntity::Update` `0xbc000`) and physics (G `FUN_1001f3c0`) keep running. Separate entity-side flag: `cObjCharacterEx::IsBrainFrozen` (C `0x149b0`) = bit 4 of `+0x204` (lead found; semantics vs brain flag unverified).
- Animal brain `cAIHuntBrain_Animal` size `0x290`; target id int `+0x254` (`SetTargetIdx` C `0x15d90`), threat ptr `+0x25C`; sense passes overwrite the target unless frozen.
- Damage: `cScrEntity::Hurt` (C `0x3fbc0`) only forwards to entity vfunc `+0x14c` (empty on cScrAnimal); animal health changes via `FuncInflictDamage` (C `0x9a540`, subtracts from `+0x19C`); animal death presentation vfunc `+0x154` (G `0x1e8b0`). Hunter health is profile attribute 9 via hunter vfunc `+0x14c` (G `0x530c0`, god-mode gated), not `+0x19C`.
- `cObjEntity::ClientSpawn`/`Spawn` are empty; humans spawn via `cObjHumanEx::Spawn`. `cObjManager::Get` (`0xaae30`): entity ptr array `+0x04`, count `+0x08`, cap `0x400`; `AddEntity` `0xbd4b0`. Active camera export `m_pActiveCamera` (C RVA `0x1adfe0`).
- `cAIHuntAnimalInterface::SetTimeMultiplier` (C `0x13bf0`) has no callers — not the slow-mo path.

## Loop and console (Track D, `re/notes/timing.md`, `re/notes/console.md`; reviewed 2026-10-16)
- Frame: `cApplication::Run` pumps Win32 messages and calls cGameApp vtable slot `+0x30` (Main7 `FUN_0040f170`). **Variable dt**: raw ms clamped × `frameTime` (default 1.0, range 0.001–100, Main7 RVA `0x3b5d0`). `sv_fps` (default 100, min 2) gates one input/sim/render/sound step per ≈1/sv_fps s; leftover time is dropped.
- Present = `cRenderDX::Flip` (EngineDll7 `0x354c40`); EndScene = `cRenderDX::EndFrame` (`0x354b10`).
- Console global: export `?console@@3PAVcConsole@@A` (pointer). Created in `InitConsole` (`0x329ec0`), which then loads `Cfg\console.cfg`. Register commands with `cConsole::AddStaticFuncf(unsigned flags, char* name, void(*)(void), ...)` (`0x166bd0`; arg type tags 4 int, 6 float, 7 string, terminator `0xffffffff`) after `InitConsole`. Exec paths: `cConsole::Exec(line, n)`, `cConsole::RealExec(char*)` (lead re-checked exports).
- In-game console: key grave (DIK `0x29`), locked until A+V+F12 or Q+I+F12, first line must be password `3.141692` (string EngineDll7 RVA `0x41ff78`, lead re-checked). `-developper` reveals commands flagged `0x20`, `-cheats` flagged `0x40` (`godMode`, `noClip`, `debugCamera` — CommonDll statics `m_bGodMode`, `m_bNoClip`).
- Map loading: console `spmap <MAP>` / `menumap <MAP>` → `cBspTree::Load("Bsp\%s.bsp", 0x6ff, 1)`. Commands `map` and `cl_join` are not registered anywhere.
- Slow motion is client-local: `cHUDSlowMotion::Enable` (GameDll7 `0xca690`) sets `frameTime` from cvar `mod_slowmotionFrameTime` (default 0.35; string confirmed in GameDll7). Disable for co-op by forcing that cvar to 1 or patching `Enable`/`SetFrameTime` (`0xca400`).
- Switches: `+mod`, `+path`, `-dedicated`, `-join`, `-developper`, `-cheats`, `-spmap`, `-map`, `-batchmode`, `-nointro`, `-testmodels`, `-buildai`, `-res`, `-shaderdup`, `-logfiles`, `-console`, `-console98`, `-exp` (class export dump), `-debug`.

## Lead runtime findings (dh2coop.asi, two-instance runs on HUNTMAP4, 2026-10-16)
- **Threads:** the device window's thread (`GetWindowThreadProcessId(focusHwnd)` at CreateDevice) is the main game thread: it runs `Present`, `cBspTree::Load`, console commands. While a map loads, a **second thread presents the loading screen** — never run game logic from Present without checking the thread. — dh2coop.log `t12624` vs `t23860`.
- **Focus:** the game stops simulating/presenting when its window loses focus unless `cApplication::m_bAllowBackground` (EngineDll7 export `?m_bAllowBackground@cApplication@@2HA`, default 0) is 1. Our ASI sets it to 1 — debug.log `WM_ACTIVATEAPP: 0`, heartbeat frames=2 before the fix. With it set there is **no other inactive throttle**: decompile (`re/notes/lead-focus-*.c`) shows `cApplication::Run` only gates on `this+0x1c` (active flag, set by `OnActivateApp`/`OnSetFocus`, cleared by `OnKillFocus` "Application loses focus"), and Main7's overrides (vtable `0x29748` slots `+0x20/+0x24/+0x28` → `0x13c30/0x13d70/0x13ea0`) only acquire/unacquire DirectInput (input vt `+0x0c`/`+0x10`; "Input unacquire" EngineDll7 `0x3488b0`). Runtime-verified 2026-10-16 13:37–13:40 (`tools-src/focus-test.ps1`, `re/notes/coop-local/focus-test.txt`): both instances held avg_dt 0.010 s and rx ≈ 19/s while focused, unfocused, fully covered, minimized and restored. The earlier ~9 fps stall is NOT focus-related; most likely cause is a user window drag (Win32 modal move loop blocks the game's message pump → no frames) — UNVERIFIED.
- **Console commands verified at runtime:** `AddStaticFuncf` called as `__cdecl(self, flags, name, fn, tags..., -1)` registers commands; `console::Exec("coop_status")` (queued, returns 1) reaches our `__cdecl` handler on the main thread.
- **Live entity manager:** captured from the `this` of `cObjManager::AddEntity(cObjEntity*)` (EngineDll7 export, called by Main7's manager override for every spawn). `SetLoadSaveObjManager`'s global (EngineDll7 RVA `0x5826ec`) is still null during normal play (only set around save/load). `cObjManager::Get` export is not called by game code.
- HUNTMAP4 at load: 127 entities, 13 vtables; hunter = 1× GameDll7 `+0x106ff0` (`cScr_ModHunter`, id 125, parent -1); animals = 5× GameDll7 `+0xff470` (`cScrAnimal`, ids 120–124). Other common vtables: CommonDll7 `+0x12b320` ×72, `+0x122028` ×22 (`cObjHuntAreaAIEntity`), GameDll7 `+0x10afa0` ×10.
- **Animal transforms:** `+0xE8` is world position, **y is up** (deer at y≈0.4–8, x/z within ±110); a walking deer moved >100 units in 100 s. Animal `+0xF4` angle is mostly (0,0,0) even while moving, but one deer (id 122) later showed y = -18.45 → it is written only in some states; whether it is the animal's true facing is UNVERIFIED (the model's heading may live elsewhere). Deer health `+0x19C` = 45. Puma (id 124) sits at (-10000,-10000,-10000) with hp 100 → parked/inactive until triggered (UNVERIFIED).
- **Hunter transform (verified by user walking, 2026-10-16 10:24):** `+0xE8` is the hunter's world position (HUNTMAP4 spawn is at the origin, eye/feet offset y = -1.69). Walking speed ≈ 5 units/s. `+0xF4`.y = heading in **degrees, unwrapped** (420.27 observed), convention **yaw = atan2(dx, dz)** (0° faces +z, 90° faces +x): step (dx 4.71, dz 1.69) ↔ logged yaw 70.0° (atan2 = 70.3°). `+0xF4`.x (pitch) and .z stay 0 while the user looks around → view pitch lives on the camera, not the entity.
- Slow-mo hook signature check passed at runtime (`GAMEDLL7.DLL+0xca690`); blocking itself not yet exercised. **Free-hunt maps have no slow motion** (user, 2026-10-16); HUNTMAP scripts only carry the constants `SLOW_MOTION`, `DOUBLE_HEALTH`, `MISSION_TYPE_ACTION`, `MISSION_TYPE_HUNT` (`SCRIPTS\LEVELS\HUNTMAP4\MAIN.XSC`) → slow-mo is an action-mission/cheat feature. Verify the block in Phase 7 (missions).

## Entity spawning (lead, runtime-verified 2026-10-16; details `re/notes/lead-puppets.md`)
- BSP entity lump is a Lua chunk: `entity("<class>")` (Main7 `FUN_00422410`) creates via `cObjManager::CreateEntity(name, 0)` and stores via `cObjManager::Set`; `property(k, v)` (Main7 `FUN_00422770`) calls entity vt `+0x14` `HandleKey`. BSP positions are cm (×0.01 in `HandleKey`).
- Runtime spawn recipe (as `cObjScriptedEntity::SpawnEntity`, CommonDll7 `0x3e590`): `CreateEntity(class, 0)` (EngineDll7 static cdecl `?CreateEntity@cObjManager@@SAPAVcObjEntity@@PBDH@Z`) → manager vt `+0x10` `AddEntity(ent)` → entity vt `+0x15C` `SetScriptClassName` (scripted only) → vt `+0x54` `Precache` → vt `+0x58` `Spawn`. Our `obj_humanEx` puppet spawned this way gets index 127, model and phys objects, has collision (user-felt), and renders with an NPC body (see below).
- Entity vtable slots: `+0x14` HandleKey, `+0x28` Draw, `+0x54` Precache, `+0x58` Spawn, `+0x68` SetModel, `+0xA8` SetPos, `+0xB0` SetAngle, `+0xEC` SetTargetName, `+0x15C` SetScriptClassName, `+0x174` SetAnim (scripted), `+0x194` AddAnimPak (character_ex). Manager: `+0x10` AddEntity, `+0x20` RemoveEntity(ent, int).
- Entity fields: `+0x08` byte flags (bit 0x08 = draw lights/shadows gate; player spawn clears 0x10), `+0xA7` entity kind byte (4 player, 0x42 animal, 2 ladder, 3/5/6 vehicle, 7 train, 8 world, 9 scope; bare obj_humanEx 0), `+0xC0` bit 0 = hidden, `+0xC4..+0xD8` world AABB, `+0x124` BSP link (-1 = unlinked; set by `cBspTree::UpdateEntity`), `+0xE0` model name ptr.
- Class registry dumped by `cObjManager::DumpClassesKeys(file)` (thiscall on the live manager) → `game\Bin\dh2coop-classes.txt` (212 lines, ~110 classes: base_entity → obj_scripted_entity → obj_character_ex → obj_humanEx → obj_player → obj_hunter → obj_npc → obj_guide …).
- Hunter model: `Objects/Characters/Teenm/Body/teenmm.skin,game.ap,HUNTMAP4.ap`. **The skin only contains first-person arms** (`teenmm1st.skin`, "fpshands1") — the game has no third-person player body. Puppets must use an NPC body: `objects/npc/<name>/<name>.skin,game.ap` (verified visible with mountainguide, user-confirmed 2026-10-16); NPC models load on demand on free-hunt maps.
- A runtime-spawned `obj_humanEx` is drawn every frame once it is AddEntity'd + Spawned + BSP-relinked (`cBspTree::UpdateEntity` with a world AABB at `+0xC4`) — verified with a `cObjEntity::Draw` hook (200 draws/2 s).
- `cBspTree::UseServer(int)` (static cdecl export) sets EngineDll7 global RVA `0x6963dc`; value was 1 when read from the Present hook.
- **Puppets must drop their physics body** (lead, runtime-verified 2026-10-16 13:48, `tools-src/move-test.ps1`): `cObjEntity::SetPos` (`0xbc130`) only writes `+0x0C`/`+0xE8`/`+0x100` and the model (model vt `+0x3c`); it never moves the phys body `+0x118`, and the physics step writes the body position back onto the entity → a spawned character stays drawn at its spawn point and its capsule pushes the hunter (two capsules at one spawn = puppet "stacked" on the hunter's head). Fix: `cObjScriptedEntity::UnlinkWithPhysics(1)` (CommonDll7 export `?UnlinkWithPhysics@cObjScriptedEntity@@UAEXH@Z`, `0x3d820`; humanEx does not override it) = phys manager vt `+0xb8` remove + entity vt `+0xc` DestroyPhysModel → `+0x118 = 0`. `cObjHumanEx::Update` (C `0x1d9a0`) skips a null body. After that SetPos/SetAngle move the drawn puppet (screenshots `re/notes/coop-local/move-134802-*`); no crash, no collision.
- **View direction = render camera, not the hunter** (lead, runtime-verified 2026-10-16 14:16):
  - The hunter's `+0xF4.y` is the movement heading. It snaps to the view yaw only while moving: standing mouse turns leave it unchanged, and nothing in the hunter's 0x838 bytes tracks the view.
  - `cObjScriptedEntity::m_pActiveCamera` (CommonDll7 `0x1adfe0`) is null during free hunt.
  - The weapon/arms child entity (GameDll7 vtable `+0x102a70`, parent = hunter) has a `+0xF4` that follows the view, but with a ~95° mount offset, idle sway and pitch coupling. Not usable.
  - **Source used:** hook `cRender::SetCamera(const cRenderCamera&)` (EngineDll7 `0x242b0`, virtual). The first camera set each frame is the world view; the second is a mirrored reflection pass (eye y negated).
  - `cRenderCamera` layout: X axis `+0x84`, Z axis/forward `+0xA4`, position `+0xB4`, projection type `+0xC4` (0 = perspective), FOV `+0xD0` (90). `cRender` keeps a copy at `+0x3c8`, so `GetCameraZAxis` = `+0x46c`.
  - yaw = atan2(z.x, z.z), which matches the heading convention exactly while walking (154.68 = 154.68). pitch = asin(z.y), positive = up (mouse up → +16°). The eye sits 1.71 m above hunter `+0xE8`.
  - Mouse: 400 DirectInput units ≈ 34.7° yaw.
- **Hunter actions** (lead, runtime-verified 2026-10-16 15:06–15:21):
  - The active weapon is a child entity of the hunter (parent id `+0xA4` = hunter id; idx 126, GameDll7 vtable `+0x102a70`). Its item activation state is `+0x1f0` (1/2 = active), and its model is the first-person skin (`objects/items/game/weapons/handgun31st/handgun3.skin`).
  - `cObjItemEx::GetCurrentState` (CommonDll7 `0x32c00`) returns the weapon's body-state name: `Idle`/`Walk`/`Run`/`Fire`/`Reload`. That is a clean fire/reload signal.
  - **Crouch:** the `cObjCharacterEx +0x204` bit 1 (`IsCrouched`) stays 0 for the hunter. Use eye height instead (view camera y − feet y): standing 1.71, crouched ~1.20.
  - Current item index is `+0x3c0` (`SetCurrentItem`), and the items list is at `+0x1d8`. The free-hunt CHOOSE ITEM menu gives one weapon.
  - Scripted LMB (`SendInput` mouse button) fires.
- **Weapon in an NPC's hand** (lead, user/screenshot-verified 2026-10-16 15:21):
  - Third-person models: `objects/items/game/weapons/<handgun3|shotgun1|rifle1>/<name>.skin`. The `*1ST` dirs are first-person. Each skin header references its `.model`, which holds the bones (`bolt`, `flame` = muzzle flash, `magazine`), vis keys, sounds, and clips named like the armed body clips without the prefix (`fire`, `reload`, `idlealertneutral`, `walkforwardalert`…).
  - Recipe, as the NPC weapon scripts (`HGUNNPC/SGUNNPC/RIFLENPC.SCR`) do it:
    1. `cObjEntity::Attach(parentId, GetTagIndex(hand), 1, {0,0,0})` (EngineDll7 `0xbc740`/`0xb9020`). This sets `+0xA4` parent, `+0xA6` tag, `+0xA8` flags, and the parent's child slots `+0x168`.
    2. `LinkPart(owner, hand)`: owner model `GetTag(name)` (vt `+0xe4`) → `bone+0x138` controller → vt `+0x38`(child model part 0). This is the same as `cScrEntity::LinkPart` (CommonDll7 `0x3f590`).
    3. **Play the matching clip on the weapon.** Without a clip the gun sits sideways in bind pose with the flame always visible.
  - Hands: handgun `BipRHand` (tag 37 on mountainguide); shotgun and rifle `BipLHand`.
- **Hunter `+0xE8` is the feet position** (despite `cObjEntity::GetEyePos` returning it): a puppet placed at a remote hunter's `+0xE8` stands on the ground (pixel check at 5.2 m, terrain +0.7 m). Hunter world AABB is a generic pos ±2 m box (no height info).

## Animals and mission flow (lead + Grok P4b, runtime-verified 2026-10-16; details `re/notes/lead-animals.md`, `re/notes/lead-mission.md`)
- **Driving an animal on a client:**
  - `cAIHuntBrain_Animal::FreezeBrain` (brain `+0x200`). Keep physics; an unlinked animal is not drawn.
  - In the pre-render callback: `HandleKey("position", cm)` + `HandleKey("angle", "0 yaw 0")` + world AABB + `BspUpdate`. Position error ≤ 0.01 m (P4b run 2).
  - `FreezeBrain` does not pin the body state: re-apply `SetBodyState` when the live name differs.
  - Host yaw = movement `atan2(dx,dz)`; `+0xF4` is unreliable on animals.
- `SetBodyState("Dead")` + health 0 does not play the fall (the user saw the client deer standing, 16:13). Death presentation is vfunc `+0x154` (`0x1e8b0`). It also calls `UpdateMissionStatus`.
- HUNTMAP4 animal entity ids 120–124 match on every instance; spawn positions do not.
- **Mission (GameDll7):**
  - `g_mission` = `*(GameDll7+0x1869e8)`, status `+0x254` (1 running / 2 success / 3 failed), hunter `+0x258`, the hunter's `cHunterProfile` at `+0x1f8`.
  - `CheckMissionStatus` `0x94e50` (thiscall): all tasks done → status 2, `SetMapIsComplete`, `0x50760(hunter,0)`, `0x91c20(5,"MISSION SUCCESSFUL!")`.
  - **`0x91c20` = thiscall(mission, char code, const char* text)**. It sends in-process server message `0x87`; code 5/6 opens the results screen. Every mission end on every map calls it (callers `0x94e50`, `0x965e0` fail, `0x91e80` protected kill, `0x990b0` task fail).
  - Verified 20:04: calling it on a client with code 5 + status 2 + `SetMapIsComplete` shows the MISSION SUCCESSFUL results screen.
- **Trophy claim:**
  - `cObjAnimal::ClaimCallback` `0x1cd40` (cdecl, animal) → `0x15550`: `animal+0x404` = 0 (claimable flag), then `g_pClassManager` vt `+0x28(scriptIdx +0x408, funcName +0x40c)`, which runs the level script's `ClaimCallback`.
  - Calling it on the host for a dead animal ran the hunt script's end (`UpdateMissionStatus`, `gbCanEndHunt`) → MISSION SUCCESSFUL 20 ms later (20:04).
- Script natives are registered in GameDll7 `0x1000ba30` (`cExportKey::ExportFnc(name, …)` + `cBaseClass::s_ExportStaticFunction(fn, …)`). Examples: `GlobalsCheckMissionStatus` `0x98f0`, `UpdateMissionStatus` `0x8390`, `CompleteTask` `0x9080`, `ActivateTrigger` `0x84a0`, `GetDistBetween2Ais` `0x9200`.
- Hunt map scripts (`SCRIPTS\LEVELS\HUNTMAPBASE.SCR`) are plain text. Every `"hunter"` in them is the local player: spook distance, the 15 m trophy distance, predator spawn within 70 m, and the 25 m safe distance.

## Story triggers and map flow (lead, runtime-verified 2026-10-16 evening; `re/notes/lead-story-coop.md`)
- **Script trigger events (`cScrTriggerEvent trigger_N`, GameDll class `cObjTriggerEvent`, vtable GameDll7 `+0x10afa0`) have two steps.**
  - **Arm:** `0x68eb0` (fastcall trigger). Sets `+0x1e4 = 1` and registers with the mission. Scripts call it with `trigger_N.Activate()`.
  - **Fire:** `0x68130` (thiscall(trigger, activator)). Runs the script's `OnActivate` (function index `+0x20c`, object `+0x1b4`), sets `+0x1fc = 1`, and notifies the mission. An armed trigger fires when its `ActivateBy("hunter")` activator enters. This is the story's main branch point: it moves animals in (`MoveToPos`, `StartTask`), starts tasks and cutscenes, and arms the next triggers.
  - Entity ids and target names (`+0x198`) of these triggers match across instances (MAP3B `cScrTask01`=39 … `cScrTask07`=45).
  - Calling arm/fire on a client by id replays the host's branch. Verified 21:28: leopards aggroed for both players.
- `-spmap MAP3B` (console `spmap`) starts a story map directly. Both instances ran the same map-start arming (`cScrTask01/02/06/07`).
- `cSpServer::StartGame` (Main7 `0x1ed80`, thiscall(server, map, int)) is the common map start. Starting the map that is already loaded resets the BSP (no `cBspTree::Load`). `cSpServer::StopServer` (`0x1b060`) runs before every teardown. Main7 is not relocated (base `0x400000`); the current map name is `char*` at `0x43d590`.
- `cModelPart::SetSyncWith(part)` (EngineDll7 `0x1e7050`; bone `+0x138` part vt `+0x38`) links two parts in a ring of `cModelPartLink`s. Each link `InternalAddRef`s the other part's owner entity (refcount `+0xb8`). `SetSyncWith(nullptr)` is the engine's `UnlinkPart`.
- **Animals attacking an entity with no physics body crash the host** in `cAIHuntBrain_Animal::ProcessAttackStuck` (null `+0x7c`: the target's body radius). Puppets now keep their body (section below); `combat.cpp` also skips that function for a body-less target.
- Damage applied with `FuncInflictDamage` does not run the death (stays hp 0 standing); the bullet path `cObjScriptedEntity::OnHurt` does (vt `+0x154` at 0). Present death explicitly after lethal remote damage.

## Phase 5 combat, drop-in, knockdown (lead, runtime-verified 2026-10-17; `re/notes/lead-combat.md`, `lead-dropin.md`, `knockdown.md`)
- **Physics body** (`cPhysicsObject`, entity `+0x118`; EngineDll7 exports): `+4` = body struct (`+0x28` position, `+0xBC` flags: `4` frozen, `0x20` player), `+0x74/+0x78/+0x7C` = half extents (guide puppet 0.625 / 0.900 / 0.625), `+0xC4` move type, `+0xC8` collision priority.
  - The physics step writes the body position back onto the entity every tick, and the model is drawn from the body. The `"position"` key moves the body's **centre**, so a body placed at the feet draws the model half a body low.
  - Collision words `+0x20/+0x24/+0x28`: a character's default is `7, 7, 0xfffffff8`. `cObjCharacterEx::PlaySA` (CommonDll `0x27f10`) writes `0xfffffff8, 0xfffffff8, 7`. With those values the body still stands on the world but does not push characters. A puppet with the default words lifted both players without end (each window's puppet pushed its local hunter up).
  - `cObjCharacterEx::CharacterFreeze` does not stop physics: it sets character `+0x204 |= 0x30` and calls `USER_STOP` on the body (input only).
- **Host animal targeting.** Story attack tasks aim at `"hunter"` (the host). `cAIHuntBrain_Animal` exports `GetTarget`, `GetClosestAttackTarget` (nearest entry of the attack list), `SetTarget(ent, float)`, `OnHurt(attacker)`. The engine's own switch (in `ProcessAttackStuck`) is task vt `+0xf8(old, new)` (when task `+4 == 1`), then `SetTarget(new, 0.1)`. Running that on the host every 0.5 s made MAP3B leopards chase and hurt the nearer player, client included (user-verified 11:14).
- **Damage to a puppet** arrives at `cObjScriptedEntity::OnHurt` (vt `+0x14C` on `obj_humanEx`), not `cObjCharacter::OnHurt`. The hunter's own vt `+0x14C` is GameDll `0x530c0`: profile attribute 9, `OnDie` (`0x50350`: `+0xC1 |= 4`, parts `Death`, `+0x6CC = 1`, mission failed) at ≤ 0.
- **Knockdown** = sync attack: hunter vt `+0x210` `PlaySA(char* clip, m4x4&)` (GameDll `0x50e20`; basis rows 0-2 rotation, row 3 = victim position), vt `+0x214` `OnEndSA` (`0x53cd0`) to stand up. Each map's teen pak `TEENM\BODY\<MAP>.AP` holds that map's `pl_*_syncattack*` clips (table in `combat.cpp`). Calling the pair on the local hunter knocks it down and up again (verified 10:25, both directions).
- **`cScrEntity::SetAnim` is also called on cutscene camera objects** (`csNNcamNN`, e.g. MAP1 `cs02cam01` ~15 s after `cScrTask02` fires) that do not have the entity layout: `+0x04` is not an entity id (2237, 23205, 32689…) and the `+0x198` name read faults. Check `EntityById(id) == self` before treating a caller as a map entity (log sweep 2026-10-17, fixed `ad8b0b4`).
- **Engine `debug.log`** (next to `Main7.exe`, no timestamps) is truncated early in startup, before our init thread runs; copy it from `DllMain` to keep the previous run (verified 2026-10-17: 34612-byte copy).
- **Cutscene skip:** `0x6b350` fastcall(trigger) → `cScrTriggerEvent::SkipCutScene` `0x6a940` → `cScrCutScene::Skip` `0x8ed10` (`m_bSkip` `+0x4c`, script state `"Skip"`). `cScrCutScene` vtable GameDll7 `+0x110998`. Used to skip old cutscenes for a late joiner (verified MAP12C 10:14).

## Runtime probe (Track E, `re/notes/renderer.md`; reviewed 2026-10-16)
- Renderer: D3D8 only. `Direct3DCreate8` ×3 (callers EngineDll7 `+0x352a3e`, `+0x35211c`, `+0x358f9e`), one HAL device (behavior `0x84` = mixed VP + multithreaded). `Present` called from EngineDll7 `+0x354cfd`, `EndScene` from `+0x354b63` (vtable slots 15 / 35). `Direct3DCreate9` (`+0x35a43b`) creates no device — `runtime-baseline-probe.txt`.
- Default device: 640×480 `A8R8G8B8`, fullscreen, 240 Hz, `D3DPRESENT_INTERVAL_IMMEDIATE`. A loose `game\Data\Cfg\console.cfg` makes it **windowed** 640×480 (`r_windowed` default 1); the game rewrites that file on shutdown as an `addvar` dump. `game\Data\console.cfg` (wrong path) is ignored.
- `cCmdLine::Search` returns the argv index or -1.
- **`-spmap <MAP>` loads a map directly** (e.g. `+mod data -developper -cheats -spmap HUNTMAP1` → `cBspTree::Load('Bsp\HUNTMAP1.bsp')`, `[APP] map 'HUNTMAP1' loaded succesfully`) — `runtime-spmap-*.txt`. `-map <name>` does not work (execs missing symbol `map`).
- `-join <host>` → `Exec("cl_join <host>")` at Main7 `+0xf55e` → `[CONSOLE ] symbol 'cl_join' not found!`. `-dedicated` → no D3D device, no menu map, no sockets.
- `cConsole::Load` feeds cfg lines to `cConsole::RealExec` (not `Exec`); RealExec doesn't log unknown symbols.

## Disc image
- CloneCD set `Dngh2.ccd/.img/.sub/.cue`: track 1 MODE1/2352 data, track 2 audio — `.cue`. Mounts with Virtual CloneDrive.

## Main menu, HUD dial, addresses (verified 2026-10-17 late)
- Without `-spmap` the game boots into `Bsp\menumap.bsp` (`cSpServer::StartGame('menumap')`); `OnMapLoaded` fires there, so `-coophost` / `-coopjoin` start networking at the main menu.
- **Console `spmap` from the main menu crashes** (`cBoneStatic::UnlinkFromParent` AV under `MOD_CloseMainMenu`): its handler (Main7 `0x209b0`) queues `closemainmenu` with `Exec(...,0)`, then `StopServer` + `StartGame`; the deferred close unloads the menu GUI models after their world is gone. **`mod_spmap <MAP>`** is the game's own loader (GameDll `MOD_LoadLevel` `0x36b0`): `RealExec("closemainmenu")` and `closegamemenu` at once, sets the profile map (`0xa800(DAT_101657f0, map)`), then queues `spmap`. The menu's New Game is `cModMainMenu::LoadLevel` (`0xe7b30`: closemainmenu, closegamemenu, `mod_startlevel`). Other menu console commands: `mainmenu`, `gamemenu`, `mod_newlevel`, `mod_newhunt`, `mod_restartmission`, `mod_gameloadmission`, `mod_gameloadbonus`, `mod_playendfmv`; save load is `sv_load` (Main7 `0x213c0`).
- **Compass dial:** `cHUDCompass::Draw` (GameDll `0xdfe90`) calls the HUD drawer at `*(GameDll+0x1657ec)` (vtable GameDll `0x117a00`) slot `+0x28` = `0xc6be0` (stdcall(rect, uv, tex, color, angle, centered, virtual)): corners go pixel → camera space (`cRender::Transform2dTo3dCameraSpace`), a Z rotation (`0xc5ac0`), and back. Those read `cRender +0x4a4` / `+0x4a8` (tan of the half FOVs; ortho `+0x4ac` / `+0x4b0`, mode `+0x48c`); `DrawTriangle2D` reads them too. Slot `+0x24` = `0xc6530` is the partial-fill quad (not the dial). The cRender GameDll draws through is EngineDll export `?render@@3PAVcRender@@A`; CommonDll `+0x11f2d4` is a different object (fields read 0). At 2560x1080 the camera holds tanX 1.778 / tanY 1.344 (ratio 1.32, not 2.37).
- **Rifle scope** = `cHUDZoom::Draw` (GameDll `0xdb810`, fastcall(this); profiler label "cHUDZoom::Draw"). It lays itself out in back-buffer pixels from the cRender viewport (`+0x1a8..+0x1b4`): side bars `(w - h) / 2` wide, the lens as four quarters of the centered `h x h` square (one `cRender::DrawSprite`, three HUD drawer slots `+0x18/+0x1c/+0x20` = `0xc7a70/0xc7c70/0xc7e70`, two `DrawTriangle2D` each). Any 4:3 HUD remap must leave it alone; with that, the scope is round and whole at 1696x1360 (user-verified MAP11C, 2026-10-18). Weapon swap keys C / F, zoom E.
- The user hosts real games from the retail install on UDP 27420; local test sessions use another `-Port` (a second bind fails: `coop: hosting on UDP 27420 -> false`).
- Windows PowerShell: `rd` is an alias of `Remove-Item`; never name a helper function `rd`.

## Lobby, input, puppet bodies (verified 2026-10-17 late)
- Game DirectInput devices are `NONEXCLUSIVE|BACKGROUND` (keyboard and mouse): the Windows cursor moves normally and the window still gets `WM_CHAR` / `WM_KEYDOWN`. The lobby uses `imgui_impl_win32` through a `GWLP_WNDPROC` subclass and blanks the game's DirectInput reads while open.
- `console_cmds::AddCommandStr` handlers get one word: `coop_lobby join 1.2.3.4` is dropped silently (Exec still returns 1). Use one-token arguments (`coop_tp x:y:z:yaw` style).
- ENet keeps retrying an unanswered connect for well over 10 s; the lobby gives up after 8 s.
- Puppet bodies that animate with `game.ap,shotgun1.ap,handgun3.ap`: mountainguide, oldhugh (Hugh), reginald, hamisi, ausranger1, siberian1, wirake, triballeader, benedeck, survivor. `poacher.skin` makes `HandleKey("model_name")` fault and the spawn fail.
- Melee item skins: `objects/items/game/weapons/knife1st/knife.skin`, `macheta1st/macheta.skin` (the MAP3B player blade, folder name "macheta"), `mbranch/mbranch.skin` (third-person). Attached to `BipRHand` they show but lie flat to the side.
- Tailscale adapter while logged out: Up, IPv4 169.254.x only. Logged in: 100.64/10 address on the adapter named "Tailscale".
- `capture-window.ps1` copies the screen rectangle; another window on top ends up in the shot. `-PrintWindow` asks the window to draw itself.
- No checkpoints inside a level: no level script saves; code 7 "Checkpoint reached" is the level exit. Story order (text table `INTERFACE/TEXT/TEXTTABLE.INC` chapters, BSP exits): MAP1, MAP2A-C, MAP3A-C, MAP4A-B, MAP5 ... MAP10, MAP11A-C, MAP12A-C.
- Restarting the loaded level safely: `menumap menumap`, then `mod_spmap <map>` once the menu is up (verified MAP2C, clients follow both). `mod_spmap <next>` from inside a level also works (MAP2B → MAP2C).
- NVIDIA `nvd3dum.dll+0x2757548` first-chance AVs at every map load are driver noise (both windows, no effect).
- **Same-map replay crash** (unmodded game too, replay-crash agent): Main7's map loader (RVA `0x101d0`, cdecl(const char* map)) runs the map-change cleanup (`0xf8f0`, cdecl(int): BSP free, sound `SetBSP(0)`, `cModelManager::ClearCache`) only for a different map; for the loaded map (name at Main7 `0x3d590`) it resets the BSP, so cached models keep `cBoneSound` prototypes whose waves the first `EndPrecache` already released; the next teardown's `cSndDrvDS::WaveRelease` (EngineDll `0x3882a0`) writes through the empty slot. Calling `0xf8f0(0)` before the loader for a same-map start makes it a full load — verified 2026-10-18, `replay-coop-test.ps1` MAP1 ×3 → MAP2A → MAP1, two windows, no crash.

## Mission flow (flow agents, verified 2026-10-18)
- Level exits (`cObjTriggerLevel` `0x6c420` walk-in, `0x6c610` immediate) take the next map from the profile, not the BSP token: `SetMapIsComplete` (`0xec160`) sets the last map and `profile +0x23d8 += 1`; `mod_newlevel` (`0xc0080`) loads `GetMapToBeStarted`. A `-spmap` start leaves the index at map1; `0xa800(rules, "map3b")` (lower case) sets it. The mission table interleaves the hunt maps: MAP2C → HUNTMAP3 → MAP3A, MAP5 → HUNTMAP1 → MAP6, MAP9 → HUNTMAP2 → MAP10 → HUNTMAP4 → MAP11A.
- The game's in-place `Restart` (`0xc0770`) with the fail menu open, plus the same-map cleanup, faults next frame in `cGuiManager::Update` (from `MOD_UpdateGameMenu`); retry through the main menu (`menumap`, then `mod_spmap <lower-case map>`) is clean.
- Game end: `mod_playendfmv` (`0xbe480`, `Movies\omap12c`), then `Quit` → menu; no credits (commented out in `GAMEMENU.SCR`).

## Kill credit, lives, corpses (life agents, verified 2026-10-18)
- **Kill credit:** `+0x420` (the mission action a death reports) is written only by `cScrAnimal::OnHit` `0x1a680` (1) and read then cleared by present-death `0x1e8b0` via `0x91e80(mission, +0x420, +0x434)`. A host-applied client hit (`FuncInflictDamage` + `PresentDeath`) reported action 0, so `cTaskShoot::UpdateTaskStatus` `0x999a0` (counts action 1 only) ignored client kills. Fixed by reporting action 1 for client kills; verified MAP2A task 5 and MAP4B (lionesses + lion → MISSION SUCCESSFUL).
- `cMission` holds one pending report (`+0x1dc/+0x1e0`); its Update `0x94ba0` (vt `+0x44`) hands it to the active tasks and resets it, so two reports before one Update lose one kill (the mod queues them). Task list `+0x1f8`: count `*(+4)+1`, array `*(+0x10)`; task id `+0x64`, state `+0x68`; `cTaskShoot` species `+0xf0`, left `+0xf4`.
- Second lives: script `MoveToPos` revives a dead animal; host commits `kAnimalRevive` (kind 20) with a life serial, clients reset and re-freeze the mirror; verified MAP2A, MAP2B siege (lives 2-3), MAP3A hyenas, MAP11B boars. `[Scale] ReuseKilled=1` verified (MAP2B reserve life 2).
- Corpse fade runs per process (`cScrAnimal::Update` `0x1f3c0`: hp 0 and `+0x404 == 0` → countdowns `+0x6f4`, `+0x450`, then park vt `+0x234`); a client copy can park ~15 s before the host's (MAP7 tiger); the mirror puts it back until the host parks.
- Two modules hooking one function: safetyhook chains on an `e9` prologue; a strict `55 8b ec 6a ff` check in the second module must accept this mod's own jump (animals / beasts on `0x1a680`).

## Vehicles and riders (verified MAP4A, 2026-10-18)
- `cObjVehicle` (CommonDll exports): `Update` `0x5a9c0` (calls `PhysDrive` while `+0x33c & 8`), `Freeze` `0x53d20` (zeroes chassis body velocity `+0x34`/`+0x40`, gravity `+0x110`), `+0x118` = `cPhysVehicle` (a `cPhysicsObject`): `+4` rigid body with orientation m3x3 rows at `+0x04`, position `+0x28`; wheels: object array `+0xcc`, count `+0xe8`, each `+4` = its own body (4 on truck1, offsets about +/-1.9 / -1 / +/-1 m). `cPhysicsObject::SetPosition` (EngineDll `0xf9c40`) / `SetOrientation(m3x3)` (`0xfa0f0`) write the body; `cPhysVehicle::SetPosition` (`0x131360`) also adds the argument to every wheel (spawn-time use only). Bodies keep two more states at `*(+0xc0)` / `*(+0xc4)` (+0x28 position); moving only the current one makes the drawn model trail.
- MAP4A truck `car1` = entity id 68 (`truck1` model, seats `place_0` driver, `place_1` player). Seated hunter `+0xA4` = 68; its `+0xE8` follows the seat (1.69 m from the chassis). `LocalPlayer().pos` lags 2.5-5 m behind at truck speed: use `+0xE8` for anything placed in a moving vehicle.
- A puppet's physics body with collision words zeroed still falls (gravity at body `+0x110`); a sync attack (PlaySA) writes the collision words back.
- Script `MoveToPos` on an animal (GameDll `0x14610`, standard `55 8b ec 6a ff` prologue, thiscall(animal, mode, arg)) clears brain `+0x79`; hooking it and calling FreezeBrain after the original keeps client copies frozen.
- On a MAP4A client lioness copy, forced `SetBodyState("IdleAttack")` (and `Attack`, `Limp`) failed with "Could not forward animation"; hundreds per second ended in a script VM fault (`EngineDll7+0x29a8bf`, `cVMachine::SysMain`). **Correction (research-B, static, 2026-10-18):** the lioness does have `idleattack1` (her skin uses `lion.model`); that message comes only from `cModelPart::SetAnim` (EngineDll `0x1e73b0`) forwarding a clip to a sync-linked partner. Likely chain: host sync attack → state `CutScene` → `ApplyMirror` unfreezes the copy → it tackles the client's own hunter (`syncattacktruck`, attached and part-linked) → re-frozen mid-attack, still linked. Runtime UNVERIFIED; see `re/notes/playthrough-b.md`.
- A cleared `kFlagRiding` puppet in a truck: every player reports the same seat of their own truck copy, so view-ray player hits are 100% there.
