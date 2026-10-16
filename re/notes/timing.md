# Timing, main loop, slow motion, map load, hook points

**Progress:** done. Frame order, dt, `sv_fps`, slow motion, map load, and the hook points are filled from the `loop-decomp-*.c` dumps plus slot D probe `re\notes\loop-probe-d3.txt` (cGameApp vtable, `cRenderDX::Flip`, `cClassManager::ExportClasses`, `cGuiEvent` exports). Button-to-console dispatch is still not decompiled.

RVAs are file offsets from the preferred image base (Main7 `0x400000`, EngineDll7 and GameDll7 `0x10000000`). EngineDll7 relocates at runtime. Hook it by export or RVA, not by a hardcoded base.

## Frame order

Outer loop is `cApplication::Run` (Engine RVA `0x328700`). It logs `Entering main loop`, then pumps `PeekMessageA` / `GetMessageA` / `TranslateMessage` / `DispatchMessageA`. When the queue is empty it calls vfunc `+0x30` (MainLoop). The loop stops when that call returns 0, or when `GetMessage` returns 0. If `this+0x1c == 0` and `cApplication::m_bAllowBackground == 0`, MainLoop is not called (inactive window, background not allowed).

The base `cApplication` vtable (Engine RVA `0x411000`) slot `+0x30` is the stub `cApplication::MainLoop` (RVA `0x9e0b0`), which returns 1. The game object installs Main7 vtable RVA `0x29748` in `FUN_0040aaf0`. Slot `+0x30` of that table is `FUN_0040f170`. Slot `+0x0c` is a `jmp dword ptr` thunk to the import `cApplication::Run` (IAT RVA `0x271ac`). Slot `+0x08` is `FUN_0040cca0` (shutdown). WinMain `FUN_00401dd0` calls slot `+0x0c` unless `-testmodels` is set.

`FUN_0040f170` (Main7 RVA `0xf170`), one call per accepted message-pump turn:

1. If `DAT_0043d418 != 0`, `Sleep(1)`. What sets that dword was not traced.
2. `ms = cClock::TimeMs(app+0x204)`. Store `ms` at `app+0x1f0`. Raw dt `ms - app+0x1ec` goes to `app+0x1f4` (decimal offset 500 in the decompiler).
3. `FUN_0040ec40(app, (float)raw_dt)`.
4. `app+0x1ec = app+0x1f0`.
5. If dedicated (`DAT_0043d2bc != 0`), `Sleep(1)`.
6. Return 1 while `app+0x25c == 0` and `app+0x40 == 0` and `input+700 == 0`. Otherwise set `app+0x25c = 1` and return 0, which ends `Run`.

`FUN_0040ec40` (Main7 RVA `0xec40`) is the frame body. `DAT_0043d40c` is the per-frame object created by `FUN_00416220` and stored at the end of `FUN_0040aaf0`. Its vtable is the table at Main7 RVA `0x2af58` (`re\notes\loop-vtables.txt`: slot `+0x18` is `FUN_004167d0`).

Order inside `FUN_0040ec40`:

1. `FUN_0040b280` scales the raw milliseconds into a game dt (seconds) at `app+0x1fc` and adds it into `app+0x1f8`.
2. Server vfunc `+0x20` (`FUN_00416ad0`). That function only calls `FUN_0041c060`, which returns whether `DAT_0043d5a4+0x1fd4 == 3`, and ignores the result.
3. Server vfunc `+0x04` (`FUN_00416370`, returns 1). If non-zero, `DAT_0043d414 = 1` (the `rFromServer` linked dword).
4. `t0 = cClock::Time(app+0x204)`.
5. Server vfunc `+0x18` (`FUN_004167d0`) with `(app+0x1fc, app)`. This is the tick gate. On 0, the rest of this list is skipped and `app+0x200` is left intact.
6. On accept: `FUN_00408a40` (`-batchmode` driver). The clock delta from `t0` through this call and the tick is added to `_DAT_0043d438`, the log's `game` time. The tick itself is inside that window, so `game` includes input, the sim step, and the batch driver.
7. If not dedicated (`DAT_0043d2bc == 0`):
   - Call `DAT_0043d268()` and `DAT_0043d280()`. If the first is non-zero, call `DAT_0043d25c()`. If the second is non-zero, call `DAT_0043d274()`. Names of these pointers are UNVERIFIED.
   - If both predicates returned 0 and `input` vfunc `+0x18` is not 1, call `input` vfunc `+0x14`.
   - If `input+0x694 == 0`, call `FUN_0040e850` (render). Meaning of `+0x694` is UNVERIFIED. It is past the DIK array.
   - Sound, timed into `_DAT_0043d43c`: if `sound` is non-NULL and `FUN_004073b0` returns 0, two calls to sound vfunc `+0x6c` and one to sound vfunc `+0x08`.
8. Server vfunc `+0x1c` (`FUN_004169b0`): if `DAT_0043d5a4+0x1e1c != 0`, call `FUN_0041f080`.
9. If not dedicated: if server vfunc `+0x08` is non-zero, `FUN_0040ce80(that)`. If server vfunc `+0x24` returns 0, `FUN_0040ce80(0)`.
10. Bump frame counter `DAT_0043d430`. Add `app+0x200` into `_DAT_0043d420` and `_DAT_0043d424`. While `_DAT_0043d420 < 1.0` (`_DAT_00429c70`), increment `DAT_0043d428`. Otherwise log and zero the accumulators:

```
fn=%d, fps=%d; tris=%8d, rend(%1.3f) game(%1.3f) wait(%1.3f) sound(%1.3f) input(%1.3f) time=%f
```

Fields: `DAT_0043d430` fn, `DAT_0043d428` fps, `DAT_0043d42c` tris, `_DAT_0043d434` rend, `_DAT_0043d438` game, `_DAT_0043d444` wait, `_DAT_0043d43c` sound, `_DAT_0043d440` input, `_DAT_0043d424` time.

11. Zero `app+0x200`.

`FUN_004167d0` (the accept path), after the cap test below:

1. `FUN_0040b400` (input poll, dedicated stdin, `cConsole::Update`, debug hotkeys). Its duration is added to `_DAT_0043d440` (`input`).
2. `FUN_00414000` (fps meter). About once per 1000 ms (`_DAT_0042982c` is 1000), `app+0x1e8 = frameCount * 1000 / elapsed_ms`.
3. If `DAT_0043d5a4+0x1fd4 == 3`, `FUN_0041d400(server, app+0x200)`.
4. `state = *(DAT_0043d5a4+0x28)` via `FUN_0041e600`. If state is not 1 and not 3, return 1 (frame accepted, sim not stepped).
5. Call server vfunc `+0x50` with `app+0x200` (the sim step). Return 1.

`FUN_0040e850` (render), when it does not return early:

1. `cShaderManager::Animate` with `app+0x200` unless a server predicate skips it.
2. `cRender` vfunc `+0x80` (clear / begin). The clock around that call, and again after `cRender::EndFrame`, is added to `_DAT_0043d444` (`wait`).
3. World draw. That span is `_DAT_0043d434` (`rend`).
4. `cRender::EndFrame(render, 1)`, which reaches `cRenderDX::EndFrame` (EndScene). See hook points.
5. `input+0x43c` edge calls `FUN_0040a8a0`. That offset is not a DIK index (the DIK array base is `input+0x160`; `0x43c` is past it). Do not treat it as a key.

Dedicated (`DAT_0043d2bc`, set by `-dedicated` in `FUN_00401dd0`): the render and sound block is skipped, `Sleep(1)` runs every MainLoop, `cApplication::m_bAllowBackground` is set to 1 in `FUN_0040f310`, and `InitConsole`'s caller `RealExec`s `r_api 0; snd_usesound 0`. The tick still runs, including the stdin console in `FUN_0040b400`. A second process that finds mutex `GI UNIQUE MUTEX` (Win32 error `0xB7`) continues only when `-dedicated` or `-join` is present. Otherwise it shuts down (`FUN_00401dd0`).

## Fixed tick or variable dt

Variable dt. One sim step per accepted frame. There is no loop that spends a fixed quantum.

`FUN_0040b280` (Main7 RVA `0xb280`) clamps the raw millisecond delta, then scales it. File floats, read from `game\Bin\Main7.exe`:

| RVA | value | use |
| --- | --- | --- |
| `0x29080` | 0.0 | negative-dt test |
| `0x29828` | 0.001 | milliseconds to seconds |
| `0x2982c` | 1000.0 | max raw dt, and the fps-meter divisor |
| `0x29c70` | 1.0 | `sv_fps` compare, and the log interval |
| `0x3b5d0` | 1.0 | `frameTime` linked float, initial image value |

Clamp: if raw ms `> 1000`, use 1000. If raw ms `< 0`, use 10. Otherwise keep the raw value (0 is kept). On either clamp it also rewrites `app+0x1f0` from `cClock::TimeMs`.

Then:

```
scaled = GetNumber(cvar at app+0x24c) * clamped_ms * 0.001
app+0x1fc = scaled
app+0x1f8 += scaled
```

`app+0x24c` is the `frameTime` cvar pointer stored by `FUN_004128b0`. Slot `+0x20` on the exported `cConsoleVar` vtable (Engine RVA `0x410f40`) is the base `cConsoleSlot::GetNumber` (RVA `0x9d6e0`), whose bytes are `fld dword ptr [0x104046a8]; ret` and that float is 0.0. `AddVar` does not construct that exact class. It calls `FUN_10165d50(name, type)` and the returned object's vtable is what `+0x20` dispatches. The type-specific `GetNumber` was not decompiled. The call site is still that multiply. Whether it returns the linked float at Main7 RVA `0x3b5d0` is UNVERIFIED. The `sv_fps` test reads `0x3b5d0` by name, not through `GetNumber`.

The cap accumulator is `app+0x200`. `FUN_004167d0` adds the scaled dt into it before the test. `FUN_0040ec40` zeros it only when the tick returns non-zero. A rejected frame keeps the remainder. An accepted frame discards whatever is left. It does not subtract `1/sv_fps`.

## Where time, dt and the time scale live

| What | Where | Evidence |
| --- | --- | --- |
| Real-time clock | `cClock` at `app+0x204`, built in `FUN_0040aaf0` | ctor |
| `cClock::Time` | Engine RVA `0x9a380`. `(timeGetTime() - start) * float at Engine RVA 0x407604` | probe `loop-probe-d-engine.txt`. That float is 0.001 |
| `cClock::TimeMs` | Engine RVA `0x9a3c0`. `timeGetTime() - start`, milliseconds | same probe |
| Last real ms | `app+0x1ec`, seeded from `TimeMs` in the ctor | `FUN_0040aaf0` |
| This raw ms | `app+0x1f0`. Raw delta `app+0x1f4` | `FUN_0040f170` |
| Integrated game seconds | `app+0x1f8`, starts at 0 | `FUN_0040b280` |
| This frame's game dt (seconds) | `app+0x1fc` | `FUN_0040b280` |
| Cap accumulator (scaled seconds) | `app+0x200`, starts at 0, cleared on an accepted frame | `FUN_004167d0`, `FUN_0040ec40` |
| Time scale cvar | `frameTime`, type 6, default `"1.0"`, min `"0.001"`, max `"100"`, no slot flags, linked to Main7 RVA `0x3b5d0`, cvar pointer at `app+0x24c` | `FUN_004128b0` |
| Console name match | case-insensitive while console flag bit 1 is clear. The ctor sets flags to 0, so `frametime` matches `frameTime` | `cConsole::StrCmp` RVA `0x15d180` uses `_stricmp` |

There is no exported static whose name is a time scale. `cClock` is real time. The scale the sim is built from is the `frameTime` cvar.

## `sv_fps`

Registered in `FUN_00421750` (Main7 RVA `0x21750`) on the server object:

```
AddVar(console, "sv_fps", 4, "100", NULL, "2", NULL)
```

Type 4 (int). Default 100. Min 2. No max. No slot flags, so a typed console can read and write it without `-developper` or `-cheats`. The cvar pointer is stored at `server+0x1e44` (`0x791 * 4`). It is not linked to a raw global. The tick reads it with vfunc `+0x20`.

`FUN_004167d0`:

```
app+0x200 += scaled_dt
fps = GetNumber(cvar at DAT_0043d5a4+0x1e44)
if (fps >= 1.0 && (app+0x200 / frameTime_float) < (1.0 / fps)) return 0
```

`frameTime_float` is `_DAT_0043b5d0`. `_DAT_00429c70` is 1.0, not 1000.

So `sv_fps` is a maximum accepted frame rate against real time only when `GetNumber(frameTime)` equals the linked float (the division cancels the scale). That equality is UNVERIFIED. With both at the default 1.0, a frame is skipped until the accumulator reaches `1/sv_fps` seconds. Default 100 means about 10 ms. While a frame is skipped, input, sim, render, and sound in the accept branch do not run. If the cvar were below 1 the test is skipped. The registered min is 2, so a normal console write cannot do that.

It is not a separate server timestep. The sim receives the whole accumulated scaled dt in one call, then the accumulator is zeroed.

## Slow motion

The HUD object is `cHUDSlowMotion`. Catch-handler strings in `re\notes\loop-decomp-slowmo-GameDll7.c` name the methods. Bodies:

| Method | GameDll RVA | What it does |
| --- | --- | --- |
| `SetFrameTime` | `0xca400` | `RealExec(sprintf("frametime %.5f", value))`, stores `this+0xd0`, calls sound vfunc `+0x68(0x14, value)`, writes `DAT_101608a4` |
| `KeyStatus` | `0xca550` | stores the key at `this+0xe8`. On press, calls vfunc `+0x68` (`Enable`) |
| `Enable` | `0xca690` | if `DAT_101657ec` vfunc `+0x60(0x20000)` is 0 and `this+0x78 == 0` and `this+0xe4 == 0`: `SetFrameTime(DAT_10164d38)` and copy fade time `DAT_10164d3c` into `this+0xcc`. Then `this+0x78 = 1`, `this+0xd8 = 1`. Also copies blur state from `render+0x47c` |
| `Disable` | `0xca7f0` | `this+0x78 = 0`, `SetFrameTime(1.0)`, clears `this+0xd8` and `this+0xd4`, sets `this+0xe4 = 1` |
| `Update` | `0xcaa20` | if `DAT_10186cf4 != 0`, `SetFrameTime(1.0)` once. Meaning of that global is UNVERIFIED. Otherwise it lerps. A stored frame time of 0 is replaced with bit pattern `0x3a83126f` (0.001). A fade time of 0 is replaced with 0.001 |

`DAT_10164d38` / `DAT_10164d3c` / `DAT_10164d48` are the linked floats for:

| cvar | type | default | min | max | storage |
| --- | --- | --- | --- | --- | --- |
| `mod_slowmotionFrameTime` | 6 | `0.35` | `0.01` | `1` | GameDll `DAT_10164d38` |
| `mod_slowmotionFadeTime` | 6 | `5` | `0.5` | `20` | `DAT_10164d3c` |
| `mod_slowmotionBlur` | 4 | `160` | `0` | `192` | `DAT_10164d48` |

No flags on those three, so they are visible without `-developper`. There is no cvar named `mod_slowmotion`.

`com_slowmotion` and `com_slomo` are both type 4, default `"-1"`, range `-1..1024`, and both link `DAT_10189e60`. They are key-binding slots (same pattern as `com_fire`). Default `-1` is unbound. The HUD path does not read the `slowMotion` cvar. That cvar is separate: type 4, default `"0"`, range `0..1`, flag `0x40` (cheat), storage `DAT_10185ddc`.

`Enable` passes `mod_slowmotionFrameTime` straight to `SetFrameTime`. Setting that cvar to `1.0` keeps the time scale at 1, and the blur setup in `Enable` still runs.

`frameTime` is not one of the `cf*` cvars. Nothing in these dumps writes it into a replicated field. Forcing the local scale back to 1 on every client is what turns slow motion off for everyone. Whether some other packet copies `frameTime` is UNVERIFIED.

Force it off:

- Make `cHUDSlowMotion::Enable` (GameDll RVA `0xca690`) return immediately. `KeyStatus` then cannot start it.
- Or make `SetFrameTime` (RVA `0xca400`) ignore any value other than 1, since both `Enable` and `Update` go through that `RealExec`.
- A per-frame internal `RealExec("frameTime 1.0")` matches what `Disable` already does. Writing only Main7 RVA `0x3b5d0` is weaker: the `sv_fps` test reads that float, and the dt multiply reads `GetNumber`. Those are the same only if the type-specific getter returns the linked float (UNVERIFIED).

`FUN_100e6490` ends with `RealExec("frameTime 1.0")`. That is a parent init, not a slow-motion toggle. It also loads shader `hud_threat`. Do not treat `hud_threat` as the slow-motion HUD.

## Map load entry

The named export is `cBspTree::Load`. The game-side chain:

1. Console command `spmap` is `FUN_004209b0` (Main7 RVA `0x209b0`). If `Bsp\<name>.bsp` exists (`cFileManager::FileExists`), it `Exec`s `closemainmenu` and `closegamemenu` with the internal flag 0, then `FUN_0041ed80(this, name, 0)`.
2. `menumap` is `FUN_00420b60` (RVA `0x20b60`). Same existence test and the same `FUN_0041ed80(..., 0)`. It does not close the menus. Registered with flag `0x01`.
3. `sv_changeMap` is `FUN_00420f40`. It runs only when server state `+0x28 == 3` and the bsp exists, then `Exec`s `sv_restart`.
4. `FUN_0041ed80` (RVA `0x1ed80`) calls `FUN_004101d0(name)`. On success it precaches (`cModelManager::BeginPrecache` / `EndPrecache`), calls server vfuncs `+0x5c`, `+100` (`0x64`), and `+0xb8`, and when the third argument is 0 calls `FUN_0041e5d0(this, 3)` and `FUN_0041e410`. It logs `loading time : %dms (map : %dms, client: %dms)` (the format string's extra arguments are not all passed in the decompiler output).
5. `FUN_004101d0` (RVA `0x101d0`) does `sprintf("Bsp\\%s.bsp", name)` and `cBspTree::Load(bsp, path, 0x6ff, 1)`. Success log: `map '%s' loaded succesfully` (Main7 string RVA `0x29ebc`). Failure log: `failed to load map '%s'`. If `DAT_0043d590` already holds the same name it resets instead of loading. The runtime probe's caller `Main7.exe+0x10372` is `0x1A2` bytes after this function's entry, and the next decompiled function in this set is `FUN_0041ed80` at `0x1ed80`. The `Load` call is the body of `FUN_004101d0`.

Startup `cConsole::Exec("menumap menumap")` from GameDll is the menu map. That string is the command plus the map name.

There is no cstring `"map"` in Main7, EngineDll7, or GameDll7 (pefile scan this session). `-map` still queues `map %s`. See `console.md`.

`FUN_0041fae0` is `cSpServer::PrecacheModel` (catch string `cSpServer::PrecacheModel`), not the map command. It is slot `+0x30` of the vtable at Main7 RVA `0x2ba58` (`re\notes\loop-vtables2.txt`).

## EV_COMMAND / CM_*

`cGuiEvent::ScrExport` (CommonDll RVA `0xe7610`) exports the constants. `cGuiEvent::Print` (RVA `0xe5570`) only copies those names into a buffer for display. It is not the dispatcher.

Event type (`m_nType`, object `+0x28`):

| name | value |
| --- | --- |
| `EV_MOUSE` | 1 |
| `EV_KEYBOARD` | 2 |
| `EV_MESSAGE` | 3 |
| `EV_COMMAND` | 4 |

`EV_NONE` is a CommonDll string. It is not in this `ScrExport` list.

Command id (`m_nCommand`, `+0x2c`), the values this export assigns:

| name | value | name | value |
| --- | --- | --- | --- |
| `CM_NONE` | 0 | `CM_OK` (`VALIDCOMMAND` is the same 0x100) | `0x100` |
| `CM_KEYDOWN` | `0x10` | `CM_CANCEL` | `0x101` |
| `CM_KEYUP` | `0x11` | `CM_YES` | `0x102` |
| `CM_MOUSEMOVE` | `0x20` | `CM_NO` | `0x103` |
| `CM_MOUSEWHEEL` | `0x21` | `CM_CONT` | `0x104` |
| `CM_LBUTTONDOWN` | `0x22` | `CM_SELECT_NEXT` | `0x110` |
| `CM_LBUTTONUP` | `0x23` | `CM_SELECT_PREVIOUS` | `0x111` |
| `CM_LBUTTONDBLCLK` | `0x24` | `CM_SELECT_UP` | `0x112` |
| `CM_RBUTTONDOWN` | `0x25` | `CM_SELECT_DOWN` | `0x113` |
| `CM_RBUTTONUP` | `0x26` | `CM_SELECT_LEFT` | `0x114` |
| `CM_RBUTTONDBLCLK` | `0x27` | `CM_SELECT_RIGHT` | `0x115` |
| `CM_MBUTTONDOWN` | `0x28` | `CM_CONTROLLER_ERROR` | `0x80` |
| `CM_MBUTTONUP` | `0x29` | `CM_CONTROLLER_ERROR_CLOSE` | `0x81` |
| `CM_MBUTTONDBLCLK` | `0x2a` | `CM_LOOSEFOCUS` | `0x30` |
| `CM_VALUE_CHANGED` | `0x32` | `CM_RECEIVEFOCUS` | `0x31` |

Other fields: `m_nParam1` `+0x30`, `m_nParam2` `+0x34`, `m_object` `+0x38`, `m_nData` `+0x3c`.

`cGuiManager::m_mapVars` is an exported static. The mangled name is `std::map<std::string, void (*)(char*)>` (CommonDll string RVA `0x1a410c` in `re\notes\loop-xrefs-cmds-CommonDll7.txt`). How a GUI button string is looked up in that map, and whether the result calls `cConsole::Exec`, was not decompiled. UNVERIFIED.

GameDll has its own `CM_DIK_*` strings. Those are a second set, not the CommonDll `CM_*` values above. Their numeric values were not dumped.

## Mod hook points

All Main7 RVAs below are stable for this v1.0 binary. Engine RVAs are stable. The Engine base is not.

Per-frame, before the game tick. `FUN_0040f170` (Main7 RVA `0xf170`), which is cGameApp vtable `0x29748` slot `+0x30`. A hook at entry sees every message-pump turn, including turns whose tick is then rejected by `sv_fps`. The raw millisecond delta is in hand after the `TimeMs` subtract, before `FUN_0040ec40`.

Per-frame, the accepted tick, before the sim. Inside `FUN_004167d0` (Main7 RVA `0x167d0`), after the early `return 0`, the call to `FUN_0040b400` is the input and console update. The sim is the later `(*DAT_0043d5a4 + 0x50)(app+0x200)` in that same function. Hook that call for pre-sim. The argument is the accumulated scaled dt. Rejected frames never reach it.

Per-frame, after the sim and before the 3D draw. `FUN_0040e850` (Main7 RVA `0xe850`). It runs only when the tick returned non-zero, the process is not dedicated, and `input+0x694 == 0`. `cRender::EndFrame` is near the end of this function.

After the BSP is loaded. `cBspTree::Load` inside `FUN_004101d0` (Main7 RVA `0x101d0`), or the success log `map '%s' loaded succesfully`. Entities and precache are not done yet.

After the map is ready for play. The success return of `FUN_0041ed80` (Main7 RVA `0x1ed80`), at the `loading time` log, after vfuncs `+0x5c`, `+0x64`, and `+0xb8`. That is the hook for "map and client finished."

D3D8 EndScene. `cRenderDX::EndFrame` (Engine RVA `0x354b10`, export `?EndFrame@cRenderDX@@UAEXXZ`). The device call is vfunc `+0x8c`, which is `IDirect3DDevice8::EndScene` (method index 35). The runtime probe's `EngineDll7.dll+0x354b63` is this call. The function returns without presenting.

D3D8 Present. `cRenderDX::Flip` (Engine RVA `0x354c40`, export `?Flip@cRenderDX@@UAEHXZ`). The probe's `EngineDll7.dll+0x354cfd` is inside this function (`0x354cfd - 0x354c40 = 0xBD`). `Flip` calls device vfunc `+0x0c` first (`TestCooperativeLevel`, index 3). If that returns 0 and `this+0x1c0 == 0`, it calls device vfunc `+0x3c` (`Present`, index 15) as `Present(NULL, NULL, hwnd at this+0x130, NULL)`. If `this+0x1c0 == 1` it instead calls some other object's vfunc `+0x0c`. What that object is is UNVERIFIED. Hook `Flip` rather than a raw Present detour if you want both branches.

Console commands. Global export `?console@@3PAVcConsole@@A`. `InitConsole` (Engine RVA `0x329ec0`) allocates it, then `cConsole::Load(console, "Cfg\\console.cfg")`. Register at the return of `InitConsole`, or on the first `FUN_0040f170` once `console` is non-NULL. API and the `coop_host` / `coop_join` shape are in `console.md`.

## Runtime experiments requested

This job does not launch the game. Track E owns launches. Working directory `game\Bin`, disc mounted. Do not write the original install.

1. Launch `Main7.exe +mod data`. Confirm `debug.log` contains `Entering main loop` and, about once a second, the `fn=%d, fps=%d; tris=... rend(...) game(...) wait(...) sound(...) input(...) time=%f` line. Record one line at the menu and one in a level. The `fps` field should sit near `sv_fps` (default 100) or below it, not above it for long.
2. Open the console with the chord in `console.md` (no `-developper` needed to open it). Run `sv_fps` and confirm it prints 100. Set `sv_fps 20` and confirm the log's `fps` field drops and the game keeps simulating. Set `sv_fps 2` (the min) only long enough to see the cap, then set it back. Do not leave it there.
3. Run `frameTime` and confirm the print is 1 (or the `console.cfg` value). During a slow-motion press, run `frameTime` again and record the value. It should move toward `0.35` (`mod_slowmotionFrameTime`) and return to 1 on release. Also print `mod_slowmotionFrameTime`, `mod_slowmotionFadeTime`, and `mod_slowmotionBlur`. The binding cvars `com_slowmotion` and `com_slomo` default to -1, so the live key is whatever `Cfg\console.cfg` or the options menu stored. Record that key. Do not patch `Enable` in this experiment.
4. From the console, `spmap` of a known map (the menu used `menumap`). Confirm `map '<name>' loaded succesfully` and a `loading time` line. Then quit and relaunch with `-spmap <same>` and confirm the same logs without typing the command.
5. Relaunch with `-map huntmap1` (or another real bsp name). Capture the log line. The expected miss is `symbol 'map' not found!` because no `map` command was registered in the dumps. If a map loads instead, record the command that ran. That result closes the UNVERIFIED gap.
6. Present / EndScene do not need another launch. `cRenderDX::Flip` and `cRenderDX::EndFrame` name the two probe sites. A render hook test belongs to the renderer job.

## Proposed MEMORY facts

- `cApplication::Run` (Engine RVA `0x328700`) pumps Win32 messages, then calls vfunc `+0x30`. Evidence: decompile in `re\notes\loop-decomp-app.c`. Base slot `+0x30` is `cApplication::MainLoop` (RVA `0x9e0b0`), vtable Engine RVA `0x411000`. Evidence: `re\notes\loop-probe-d3.txt`.
- The game vtable is Main7 RVA `0x29748`, installed by `FUN_0040aaf0`. Slot `+0x30` is `FUN_0040f170` (the frame). Slot `+0x0c` is an import thunk to `cApplication::Run` (IAT RVA `0x271ac`). Slot `+0x08` is `FUN_0040cca0`. Evidence: `re\notes\loop-probe-d3.txt` and the IAT resolve against `game\Bin\Main7.exe`.
- Frame body is `FUN_0040ec40` (Main7 RVA `0xec40`). Accepted order: scale dt, server `+0x20`, `rFromServer`, tick `+0x18` (input, then sim vfunc `+0x50` when server state is 1 or 3), batch mode, then if not dedicated the render and sound, then server `+0x1c`. Evidence: `re\notes\loop-decomp-frame.c`, tick in `re\notes\loop-decomp-gameobj.c`.
- Dt is variable. `scaled = GetNumber(frameTime cvar) * clamp(raw_ms, negatives become 10, max 1000) * 0.001`. Stored at `app+0x1fc`, integrated at `app+0x1f8`. Cap accumulator `app+0x200` is zeroed only when the tick returns non-zero. Evidence: `FUN_0040b280` RVA `0xb280`, floats at Main7 RVA `0x29080` (0), `0x29828` (0.001), `0x2982c` (1000).
- `frameTime` cvar: type 6, default `1.0`, range `0.001..100`, no flags, linked float Main7 RVA `0x3b5d0` (image value 1.0), pointer at `app+0x24c`. Evidence: `FUN_004128b0`. The exported `cConsoleVar::GetNumber` slot returns the constant 0 at Engine RVA `0x4046a8`. Live cvars are built by `FUN_10165d50`. Whether that override reads `0x3b5d0` is UNVERIFIED.
- `cClock::Time` Engine RVA `0x9a380` and `TimeMs` RVA `0x9a3c0` use `timeGetTime`. `Time` multiplies by Engine float RVA `0x407604` (0.001). The clock object is `app+0x204`.
- `sv_fps`: type 4, default 100, min 2, no max, cvar pointer at `server+0x1e44`. The tick skips the frame while `accumulator / frameTime_float < 1/sv_fps`. One variable step per accepted frame. Evidence: `FUN_00421750`, `FUN_004167d0`, float `0x29c70` = 1.0.
- Slow motion is `cHUDSlowMotion`. `Enable` GameDll RVA `0xca690` calls `SetFrameTime` (`0xca400`) with `mod_slowmotionFrameTime` (default 0.35, `DAT_10164d38`). `SetFrameTime` does `RealExec("frametime %.5f")`. `Disable` (`0xca7f0`) sets `frameTime` back to 1. It is local. Forcing `Enable` to return, or ignoring non-1 values in `SetFrameTime`, turns it off on that client.
- Map load: `spmap` / `menumap` to `FUN_0041ed80` (Main7 RVA `0x1ed80`) to `FUN_004101d0` (RVA `0x101d0`) to `cBspTree::Load("Bsp\\%s.bsp", 0x6ff, 1)`. Probe caller `Main7+0x10372` falls inside `FUN_004101d0`.
- `cGuiEvent` script constants: `EV_COMMAND` = 4, `EV_KEYBOARD` = 2, `EV_MOUSE` = 1, `EV_MESSAGE` = 3. `CM_OK` = `0x100`. Fields `m_nType` `+0x28`, `m_nCommand` `+0x2c`. Evidence: `cGuiEvent::ScrExport` CommonDll RVA `0xe7610` in `re\notes\loop-probe-d3.txt`. The button-to-`Exec` path is UNVERIFIED. `cGuiManager::m_mapVars` is `map<string, void(*)(char*)>`.
- D3D8: `cRenderDX::EndFrame` (Engine RVA `0x354b10`) calls device `+0x8c` (EndScene). `cRenderDX::Flip` (Engine RVA `0x354c40`) calls device `+0x3c` (Present) when `TestCooperativeLevel` returns 0 and `this+0x1c0 == 0`. The probe sites `+0x354b63` and `+0x354cfd` are those two functions.
