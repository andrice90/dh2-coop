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
- **Hunter `+0xE8` is the feet position** (despite `cObjEntity::GetEyePos` returning it): a puppet placed at a remote hunter's `+0xE8` stands on the ground (pixel check at 5.2 m, terrain +0.7 m). Hunter world AABB is a generic pos ±2 m box (no height info).

## Runtime probe (Track E, `re/notes/renderer.md`; reviewed 2026-10-16)
- Renderer: D3D8 only. `Direct3DCreate8` ×3 (callers EngineDll7 `+0x352a3e`, `+0x35211c`, `+0x358f9e`), one HAL device (behavior `0x84` = mixed VP + multithreaded). `Present` called from EngineDll7 `+0x354cfd`, `EndScene` from `+0x354b63` (vtable slots 15 / 35). `Direct3DCreate9` (`+0x35a43b`) creates no device — `runtime-baseline-probe.txt`.
- Default device: 640×480 `A8R8G8B8`, fullscreen, 240 Hz, `D3DPRESENT_INTERVAL_IMMEDIATE`. A loose `game\Data\Cfg\console.cfg` makes it **windowed** 640×480 (`r_windowed` default 1); the game rewrites that file on shutdown as an `addvar` dump. `game\Data\console.cfg` (wrong path) is ignored.
- `cCmdLine::Search` returns the argv index or -1.
- **`-spmap <MAP>` loads a map directly** (e.g. `+mod data -developper -cheats -spmap HUNTMAP1` → `cBspTree::Load('Bsp\HUNTMAP1.bsp')`, `[APP] map 'HUNTMAP1' loaded succesfully`) — `runtime-spmap-*.txt`. `-map <name>` does not work (execs missing symbol `map`).
- `-join <host>` → `Exec("cl_join <host>")` at Main7 `+0xf55e` → `[CONSOLE ] symbol 'cl_join' not found!`. `-dedicated` → no D3D device, no menu map, no sockets.
- `cConsole::Load` feeds cfg lines to `cConsole::RealExec` (not `Exec`); RealExec doesn't log unknown symbols.

## Disc image
- CloneCD set `Dngh2.ccd/.img/.sub/.cue`: track 1 MODE1/2352 data, track 2 audio — `.cue`. Mounts with Virtual CloneDrive.
