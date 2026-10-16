# Console, cvars, and command-line switches

**Progress:** done. Open sequence, switches, `console.cfg`, the registration API, and the cvar list are filled from the `loop-decomp-conregs-*.c` dumps, `loop-decomp-console.c`, and slot D probe `re\notes\loop-probe-d3.txt`. A few command handlers are missing because the decompiler rotated `Add*` arguments and dropped the function pointer. Those rows say so.

Names and flag immediates below are the string and integer literals in each `Add*` call. Where the decompiler also printed a `FUN_` in that same call, it is listed as the handler. Several GameDll `AddStaticFuncf` calls recovered the name and the flag only.

Type numbers used by `AddVar`: 4 int, 6 float, 7 string. `cam_fov` is type 4 even though its default text is `"90.0"`.

Slot flags, from `cConsole::IsSlotVisible` (Engine RVA `0x15d040`), `cConsole::RealExec` (RVA `0x166110`), and `cConsole::Save` (RVA `0x15eca0`):

| bits | effect |
| --- | --- |
| `0x01` | present on `menumap`, `sv_varchaged`, and the menu commands. `IsSlotVisible` does not test it. Meaning UNVERIFIED |
| `0x02` | a user line (the one `Exec` prefixed with `%`) that assigns this var logs `variable %s is readonly` and does not write. Internal `RealExec` still writes |
| `0x08` | on assign, call slot vfunc `+0x3c` and return before the normal scan |
| `0x20` | hidden from a `%` line unless console `+0x18` bit 2 is set (`-developper`) |
| `0x40` | hidden from a `%` line unless console `+0x18` bit 4 is set (`-cheats`) |
| `0x60` | both `0x20` and `0x40`. A typed use needs both switches |
| `0x80` | `Save` writes the var. The test is `(char)flags < 0`, so any flag whose low byte is negative qualifies. `0x80` is the one these registrations use |
| `0x100` | used on `cfServerName`, `cfMaxClients`, `cfNeedPassword`. Not tested by `IsSlotVisible`. Network meaning UNVERIFIED |
| `0x102` | `0x100` plus readonly. Used on `cfVersion`, `cfMapName`, `cfNumClients`, `cfLobby` |
| `0x200` / `0x400` / `0x600` | range printing only (`greater or equal`, `lower or equal`, or both). Not a registration argument in these dumps. The min/max strings are separate `AddVar` arguments |
| `0x800` | only on the built-in `addvar` command. Meaning UNVERIFIED |

`IsSlotVisible` runs only when the line starts with `%`. `cConsole::Exec(console, line, 1)` stores `%<line>`. `Exec(..., 0)` stores the line unchanged. `cConsole::Load` calls `RealExec` directly. So `console.cfg`, `-spmap`, `-map`, `-join`, and the dedicated stdin (`FUN_0040b400` uses `Exec(..., 0)`) bypass the developer and cheat gates.

`cConsole::StrCmp` (RVA `0x15d180`) uses `_stricmp` unless console flag bit 1 is set. The ctor sets `+0x18` to 0, and `-developper` sets bit 2, not bit 1. Names match regardless of case. `frametime` is `frameTime`. The dedicated string `snd_usesound` matches `snd_useSound`.

A missing name logs `symbol '%s' not found!` when `console+0x1024 != 0`. The decompiler's `FindSlot` failure path is that log.

## How to open the console

Opening the in-game view does not require `-developper` or `-cheats`. Those switches only set the visibility bits above.

`cConsoleView::Update` (Engine RVA `0x16daf0`) is the key handler. Two dwords in the Engine image, file bytes `01 00 00 00 01 00 00 00` at RVA `0x56a530`:

- `DAT_1056a530` starts at 1. While it is 1, the grave key does not toggle the view.
- `iRam1056a534` starts at 1. While it is 1, the first submitted line is compared and not executed.

Unlock, while the view is still locked. Either chord clears `DAT_1056a530`. The input object stores one DWORD per scan code at `input+0x160`. That base is fixed by two labeled hotkeys in `FUN_0040b400` (Main7 RVA `0xb400`): `input+0x270` calls `DumpMemoryState("MemDump_on_F10_")`, and DIK_F10 is `0x44` (`0x160 + 0x44*4 = 0x270`). `input+0x280` calls `DumpMemoryState("MemDump_on_NUM8_")`, and DIK_NUMPAD8 is `0x48`.

| chord | input offsets | DIK |
| --- | --- | --- |
| A + V + F12 | `+0x1d8`, `+0x21c`, `+0x2c0` | `0x1E`, `0x2F`, `0x58` |
| Q + I + F12 | `+0x1a0`, `+0x1bc`, `+0x2c0` | `0x10`, `0x17`, `0x58` |

Then grave (DIK `0x29`, `input+0x204`) toggles. On open it saves `input` vfunc `+0x18` and calls `input` vfunc `+0x14(1)`. On close it calls vfunc `+0x0c` and restores that saved value. While open, scan code `0x29` is ignored as text.

The first Enter is a password. It compares 9 bytes, including the NUL, with Engine RVA `0x41ff78`, whose bytes are `3.141692`. On match it clears `0x56a534` and does not `Exec` the line. A mismatch leaves the gate shut. Later lines go through `cConsole::Exec(console, line, 1)`.

The per-frame call `(**console)+0x14` is `cConsole::Update` (Engine RVA `0x167180`). The `cConsole` vtable at Engine RVA `0x411834` has that function at `+0x14` (`re\notes\loop-probe-d3.txt`). `Update` drains the `Exec` queue via `RealExec` and calls vfunc `+0x10` on the object at `console+0x1024`. That object is what runs `cConsoleView::Update` only if `+0x1024` is the view. The view pointer itself was not dumped. UNVERIFIED which field holds it. The grave toggle is inside `cConsoleView::Update`, and `Update` on the console is what the frame calls, so the toggle runs only if that chain reaches the view. The decompile of `cConsole::Update` does call `*(console+0x1024)` vfunc `+0x10` before draining the queue.

While the line editor is open, scan codes handled as editing rather than text: `0x0e` backspace, `0x1c` return, `0x29` grave (ignored), `0x39` space, `0xc7`/`0xc8`/`0xc9`/`0xcb`/`0xcd`/`0xcf`/`0xd0`/`0xd1`/`0xd3` (home, history, arrows). Printable bytes `0x20..0x7f` insert.

`-developper` and `-cheats` are applied later, in `FUN_0040f310`, by setting console `+0x18` bit 2 and bit 4. They do not clear `0x56a530` or `0x56a534`.

Separate from the in-game view, `InitConsole` builds a `cConsoleWnd` when `-console` or `-dedicated` is present, and also when `-console98` is present. The `-console98` path also calls `cDebugSystem::WaitAfterTrace(1)`. The window class was not decompiled. It is not required for the grave toggle.

Dedicated stdin is a third path. `FUN_0040b400` reads `_kbhit` / `_getch` into `DAT_0043d470` and `Exec`s the line with flag 0 when `-dedicated` is set. That path does not use the grave key or the password.

## Command-line switches

`InitCmd` (Engine RVA `0xa9450`) feeds the WinMain command line to `cCmdLine::ReadFromString` (vfunc `+0x08`). `cCmdLine::Search` (RVA `0xa9af0`, vfunc `+0x10`) is case-insensitive and returns the token index, or -1. `Get` (vfunc `+0x18`) returns the token at an index. Callers pass `index+1` to read the next argument.

| switch | where | effect |
| --- | --- | --- |
| `+mod <folder>` | `FUN_00401470` Main7 RVA `0x1470` | copies the next token over the mod folder (`DAT_0043d138`). Home directory becomes registry-or-`.\..` plus `\` plus that name plus `\`, then `cFileManager::AddHomeDirectory`. The probe launch `+mod data` selects the `data` folder |
| `+path <dir>` | same function | `AddHomeDirectory` on the next token and logs `+path '%s'` |
| `-dedicated` | `FUN_00401dd0` sets `DAT_0043d2bc = 1` | skips render and sound in the frame, `Sleep(1)` each MainLoop, allows a second instance past mutex `GI UNIQUE MUTEX`, `RealExec("r_api 0; snd_usesound 0")`, sets `m_bAllowBackground = 1`, and `InitConsole` creates a `cConsoleWnd`. Stdin accepts console lines |
| `-join <addr>` | `FUN_0040f310` | `Exec(sprintf("cl_join %s", next), 0)`. Also lets a second instance past the mutex. No `Add*` of a command named `cl_join` is in the conregs dumps. If nothing else registers it, `RealExec` logs `symbol 'cl_join' not found!`. UNVERIFIED whether another site registers it |
| `-developper` | `FUN_0040f310`, string Main7 RVA `0x29ce8` | console `+0x18 \|= 2`, else the bit is cleared. This is `cConsole::SetDeveloper`'s bit (`SetDeveloper` RVA `0x9dea0` does the same `|= 2` / `&= ~2`). Makes flag `0x20` commands visible to typed lines. Does not unlock grave or the password |
| `-cheats` | `FUN_0040f310`, string RVA `0x29ce0` | console `+0x18 \|= 4`, else cleared. Same bit as `cConsole::SetCheats` (RVA `0x9dec0`). Makes flag `0x40` commands visible to typed lines. `godMode` and `noClip` need this |
| `-spmap <name>` | `FUN_0040f310` | `Exec(sprintf("spmap %s", next), 0)`, which is the real map command. GameDll `FUN_1000cc60` also searches `-spmap` and passes the next token to `FUN_1000a800`. That second consumer was not decompiled |
| `-map <name>` | `FUN_0040f310`, string Main7 RVA `0x29cc4` (bytes `-map`) | `Exec(sprintf("map %s", next), 0)`. No cstring `map` exists in Main7, EngineDll7, or GameDll7, and no `Add*` registers `map`. Expected log: `symbol 'map' not found!`. UNVERIFIED only if some runtime builder registers it later |
| `-batchmode` | `FUN_00408840` | registers command `batchmode` with flag `0x20`, then scans later tokens (not dashed switches) for `once`, `prf`, `nomenu`, `texdump`, `memdump`, `quit` (Main7 RVAs `0x2923c`, `0x29238`, literals `nomenu` / `texdump` / `memdump`, RVA `0x29218`). `FUN_00408a40` cycles maps and `Exec`s `sprintf("%s %s", command, map)`. `GAME` / `MENU` at RVAs `0x292c0` / `0x292c8` are dump labels in that driver, not switches |
| `-nointro` | `FUN_00413920` RVA `0x13920` | skips `EngIoToolPlayIntroMovie` |
| `-testmodels` | `FUN_00401dd0` | calls `FUN_00407d90` instead of vfunc `+0x0c` (`Run`). The game loop does not start |
| `-buildai <dll>` | `FUN_00401720` | loads that DLL, calls `InitGameDll`, writes `ai.lua`, and returns. Tool path, not the game |
| `-res` | `FUN_00401a00`, string RVA `0x281ac` | compiles `.res` resources (`cResourceManager::Load` / `Save`). The adjacent filter string at RVA `0x28138` is `.RC`. Tool path |
| `-shaderdup` | `FUN_00401dd0` | tool path `FUN_0040a4f0(".script")` and a `.snd` pass, instead of `InitConsole` and the game |
| `-logfiles` | `FUN_00401dd0` | `PakEnableLog(1)` |
| `-console` | `InitConsole`, Engine string RVA `0x43de88` | create `cConsoleWnd` |
| `-console98` | `InitConsole`, Engine RVA `0x43de70` | create `cConsoleWnd` and `cDebugSystem::WaitAfterTrace(1)` |
| `-exp` | `cClassManager::ExportClasses`, Engine RVA `0x2553b0` | the only code pointer to the `-exp` cstring (Engine RVA `0x42f3fc`) is the `cCmdLine` vfunc `+0x10` call at RVA `0x2553ec`. If `Search` returns -1 the function does not write a file and calls vfunc `+0x2c`. If it hits, it logs `Scripter.exp WILL BE exported !\n` and `PakFopen`s the path argument with `"wb"`, then writes a binary class dump (`ExportClass`, `ExportVarKey`, `ExportFncKey`). File bytes at RVA `0x42f3e0` are `.exp WILL BE exported !\n`, immediately before the `-exp` cstring. The decompiler's log literal is the `Scripter.exp` line. Callers of `ExportClasses` were not listed. If nothing calls it, the switch is never tested |
| `-debug` | string Engine RVA `0x4391e4` | the only code xref is `cBspTree::ShaderReport` (Engine RVA `0x2b2c20`). What that function changes when the switch is present was not decompiled. UNVERIFIED beyond the xref |

`cApplication::ProcesParameters` only logs the exe path. It does not parse these switches.

## `console.cfg`

`InitConsole` calls `cConsole::Load(console, "Cfg\\console.cfg")` after the `cConsole` ctor. `Load` (Engine RVA `0x1672c0`) does `cFileManager::Open(fileManager, path, 0x101)`. The path is resolved through the file manager, so the `+mod` home directory applies. Each non-empty line is `RealExec`'d. A line is one or more console commands, separated by `;`, with the same grammar as a typed line: `name`, `name = value`, or `name arg`. Quotes (`'`) group arguments. Empty lines are skipped. Lines do not get a `%` prefix, so developer and cheat commands in the file run even when the switches are absent.

On shutdown, `FUN_0040cca0` (cGameApp vtable slot `+0x08`) does this when not dedicated:

```
path = cFileManager::GetFullPath(fileManager, "Cfg\\console.cfg")
cConsole::Save(console, path)
```

`Save` (RVA `0x15eca0`) opens with `PakFopen(path, "wt")` and writes one line per var slot (`slot+0x14 == 1`) whose low flag byte is negative (the `0x80` archive bit):

```
addvar <name> <typeName>; <name> = <value>
```

`addvar` is the built-in command (flag `0x800`) that calls `cConsole::AddVarByTypeName`. Commands are not archived. Cvars without `0x80` are not archived. A hand-written `console.cfg` can still set them, because `Load` executes every line, not only `addvar` lines.

The engine probes this file at startup. If `Open` fails, `Load` returns and the built-in defaults stand.

## How a mod registers commands

Exports on EngineDll7, from the decompile headers:

| export | RVA |
| --- | --- |
| `?console@@3PAVcConsole@@A` | the global `cConsole*` |
| `cConsole::AddStaticFunc(char*, void(*)(void), ...)` | `0x166a40` |
| `cConsole::AddStaticFuncf(unsigned, char*, void(*)(void), ...)` | `0x166bd0` |
| `cConsole::AddMemberFunc` / `AddMemberFuncf` | `0x166730` / `0x1668c0` |
| `cConsole::AddVar` without flags | `0x166d50` |
| `cConsole::AddVar` with flags | `0x166ef0` |
| `InitConsole` | `0x329ec0` |

`InitConsole` mallocs the console, runs the ctor (which registers `help`, `?`, `addvar`, `delvar`), then loads `Cfg\console.cfg`. After it returns, `console` is non-NULL. Register there, or on the first `FUN_0040f170` once the pointer is set. Registering before `InitConsole` writes a null pointer.

Vararg type tags end with `0xffffffff`. The ctor's `addvar` is `AddMemberFuncf(..., 0x800, "addvar", ..., AddVarByTypeName, 7, 7, 0xffffffff)` (two strings). `delvar` is one string (`7`). `batchmode` is `AddStaticFuncf(console, 0x20, "batchmode", FUN_00408700, 0xffffffff)` (no args). `AppResolution` takes two ints (`4, 4`).

For commands that should work from the keyboard without `-developper`, pass flags 0. Flag `0x20` hides them until `-developper`. Flag `0x40` hides them until `-cheats`.

```
// one string argument, visible without -developper
cConsole::AddStaticFuncf(console, 0, "coop_host", coop_host, 7, 0xffffffff);
cConsole::AddStaticFuncf(console, 0, "coop_join", coop_join, 7, 0xffffffff);
```

The handler is `void __cdecl (*)(void)` in the export's type. The console passes the scanned argument string through `cConsoleFunc::CallFunction`. Copy the pattern of an existing one-string command (`spmap`, handler `FUN_004209b0`) when binding the argument. Calling `cConsole::Exec(console, "spmap menumap", 0)` from the handler reuses the map path and skips the visibility gate.

`AddVar(console, flags, "name", type, "default", &storage, "min", "max")` links a cvar. Pass NULL storage to keep the value only on the cvar object. `sv_fps` does that.

## Registered commands and cvars

Source files: `re\notes\loop-decomp-conregs-EngineDll7.c`, `loop-decomp-conregs-CommonDll7.c`, `loop-decomp-conregs-Main7.c`, `loop-decomp-conregs-GameDll7.c`, and the ctor in `loop-decomp-console.c`. "flags" blank means the no-flag `AddVar` overload. "min"/"max" blank means the call passed NULL.

### Built into `cConsole` (ctor RVA `0x167460`)

| name | kind | flags | args | notes |
| --- | --- | --- | --- | --- |
| `help` | cmd | | none | string Engine RVA `0x41fa30`. Handler `cConsole::Help`. The ctor also `RealExec("?")` |
| `?` | cmd | | none | string RVA `0x40e688`. Same `Help` |
| `addvar` | cmd | `0x800` | string, string | `AddVarByTypeName` |
| `delvar` | cmd | | string | `RemoveVar` |

### EngineDll7 render (`cRenderInterface::RegisterConsole`, RVA `0xa6240`)

All of these cvars are flag `0x80`, type 4, and linked at the listed offset of the render interface. `r_apply` and `r_default` are commands with no flags.

| name | default | min | max | link |
| --- | --- | --- | --- | --- |
| `r_api` | the local `local_68` (not a literal) | `0` | not a literal (`pcVar1`) | `this+500` |
| `r_adapter` | NULL | `0` | `1` | `this+0x1f8` |
| `r_width` | `640` | `320` | none | `this+0x1fc` |
| `r_height` | `480` | `200` | none | `this+0x200` |
| `r_maxRefreshRate` | `0` | `0` | `200` | `this+0x204` |
| `r_windowed` | `1` | `0` | `1` | `this+0x208` |
| `r_displaybpp` | `16` | `16` | `32` | `this+0x20c` |
| `r_zbufferbpp` | `16` | `16` | `32` | `this+0x210` |
| `r_texturebpp` | `16` | `16` | `32` | `this+0x214` |
| `r_triplebuffer` | `0` | `0` | `1` | `this+0x218` |
| `r_texfilter` | `2` | `0` | `4` | `this+0x21c` |
| `r_texturescale` | `100` | `10` | `100` | `this+0x220` |
| `r_gamma` | `50` | `0` | `100` | `this+0x224` |
| `r_brightness` | `50` | `-100` | `100` | `this+0x228` |
| `r_contrast` | `50` | `0` | `100` | `this+0x22c` |
| `r_shadow` | `1` | `0` | `3` | `this+0x230` |
| `r_shadowmapres` | `64` | `32` | `256` | `this+0x234` |
| `r_antialias` | `0` | `0` | `4` | `this+0x238` |
| `r_postprocess` | `1` | `0` | `1` | `this+0x23c` |
| `r_dithering` | `0` | `0` | `1` | `this+0x244` |
| `r_specular` | `1` | `0` | `1` | `this+0x248` |

Commands: `r_apply` (handler `ConsoleApply`), `r_default` (handler `ConsoleDefault`).

### EngineDll7 input

`cInputSystem::Init` (RVA `0xa7670`) adds these only after a `FindSlot` miss-or-hit preamble. All flag `0x80`. Default argument is NULL, so the image value of the linked field stands unless `console.cfg` already set the slot. `InputInit` (RVA `0x34ae90`) adds `input_use`.

| name | type | min | max | link |
| --- | --- | --- | --- | --- |
| `input_mouse_swapbuttons` | 4 | `0` | `1` | `this+0x20ac` |
| `input_mouse_swapyaxis` | 4 | `0` | `1` | `this+0x20b0` |
| `input_mouse_speed` | 6 | `0.5` | `4` | `this+0x20b4` |
| `input_mouse_acc` | 6 | `0.5` | `2` | `this+0x20b8` |
| `input_joy_tresh` | 6 | `0` | `0.8` | `this+0x20bc` |
| `input_mouse_doubleclick` | 6 | `0.1` | `0.8` | `this+0x20c0` |
| `input_kb_wait` | 6 | `0` | `1` | `this+0x20c4` |
| `input_kb_repeat` | 6 | `0` | `1` | `this+0x20c8` |
| `input_use` | 4 | `0` | `1` | none. Default literal `"1"`, flag `0x80` |

### EngineDll7 sound

`FUN_1015c1c0` (RVA `0x15c1c0`) and `FUN_10387e40` (RVA `0x387e40`).

| name | flags | type | default | min | max | notes |
| --- | --- | --- | --- | --- | --- | --- |
| `snd_useSound` | `0x80` | 4 | `1` | `0` | `1` | not linked |
| `snd_useEAX` | `0x80` | 4 | `0` | `0` | `1` | not linked |
| `snd_useEnvMorphing` | `0x80` | 4 | `0` | `0` | `1` | not linked |
| `snd_gain` | `0x80` | 4 | NULL | `0` | `100` | `param+0xbc` |
| `snd_issound` | `0x02` | 4 | NULL | `0` | `1` | readonly, `param+0x84` |
| `snd_iseax` | `0x02` | 4 | NULL | `0` | `1` | readonly, `param+0x88` |
| `snd_gainClass0` .. `snd_gainClass7` | `0x80` | 6 | NULL | `0` | `100` | built by `sprintf("%s%d", "snd_gainClass", i)` for `i` in `0..7` |
| `snd_memory` | `0x02` | 4 | NULL | `-100000000` | `100000000` | `DAT_1069cfe4` |
| `snd_playingchannels` | `0x02` | 4 | NULL | `0` | `65536` | `DAT_1069cfe8` |
| `snd_debug_playing` | `0x02` | 7 | NULL | | | not linked |

Commands: `snd_apply` (`FUN_10155160`), `snd_Report` (`FUN_10159a40`), `snd_apiReport` (`FUN_103874c0`), `snd_debug_playing_refresh` (`FUN_103877c0`). No flag literals on those four.

### CommonDll7 (`InitCommonDll`, RVA `0x4af0`)

All three are flag `0x40` (cheat), type 4, default `"0"`, range `0..1`.

| name | storage |
| --- | --- |
| `noClip` | exported `m_bNoClip` |
| `godMode` | exported `m_bGodMode` |
| `debugCamera` | `DAT_101ad990` |

### Main7 camera, time, and debug view (`FUN_004128b0`, RVA `0x128b0`)

| name | flags | type | default | min | max | notes |
| --- | --- | --- | --- | --- | --- | --- |
| `gun_fov` | `0x20` | 6 | `.1` | `0.0f` | `179.0f` | cvar* at `app+0x218` |
| `cam_fov` | `0x80` | 4 | `90.0` | `1.0f` | `179.0f` | `app+0x21c`. Type is int |
| `cam_thirdPerson` | | 4 | `0` | `0` | `1` | `DAT_0043d58c`, `app+0x220` |
| `cam_distance` | | 6 | `1.5` | | | `app+0x224` |
| `cam_angle` | | 6 | `0.0` | | | `app+0x228` |
| `cam_angleUp` | | 6 | `10.0` | | | `app+0x22c` |
| `frameTime` | | 6 | `1.0` | `0.001` | `100` | `DAT_0043b5d0`, cvar* `app+0x24c` |
| `showStats` | `0x20` | 4 | `0` | | | `app+0x230` |
| `showGameStats` | `0x20` | 4 | `0` | | | `app+0x234` |
| `showRenderStats` | `0x20` | 4 | `0` | | | `app+0x238` |
| `showPosition` | `0x20` | 4 | `0` | | | `app+0x23c` |
| `showMemoryStats` | `0x20` | 4 | `0` | | | `app+0x240` |
| `showNetStats` | `0x20` | 4 | `0` | | | `app+0x244` |
| `showNetGraph` | `0x20` | 4 | `0` | | | `app+0x248` |
| `showSound` | `0x20` | 4 | `0` | | | `app+0x250` |
| `showBorder` | `0x20` | 4 | `0` | | | `app+0x254` |
| `trace_script` | `0x20` | 4 | `0` | `0` | `1` | `m_traceScript` |
| `clearCache` | | 4 | `0` | | | not linked |
| `rViewOffsetX` | `0x80` | 4 | `0` | `-100` | `100` | `m_nViewOffsetX` |
| `rViewOffsetY` | `0x80` | 4 | `0` | `-50` | `50` | `m_nViewOffsetY` |
| `rViewScaleX` | `0x80` | 6 | `1` | `0.5` | `4` | `m_fViewScaleX` |
| `rViewScaleY` | `0x80` | 6 | `1` | `0.5` | `4` | `m_fViewScaleY` |
| `rDrawNormals` | `0x60` | 4 | `0` | | | `m_bShowNormals` |
| `rDrawTris` | `0x60` | 4 | `0` | | | `m_bShowTris` |
| `rDrawTotalAABB` | `0x60` | 4 | `0` | | | `m_bDrawTotalAABB` |
| `rDrawSkel` | `0x60` | 4 | `0` | | | `m_bDrawSkeleton` |
| `rDrawModel` | `0x60` | 4 | `1` | | | `m_bDrawMeshes` |
| `rDrawModelStatic` | `0x60` | 4 | `1` | | | `drawModels` |
| `rDrawBound` | `0x60` | 4 | `0` | | | `m_bDrawBoundingBoxes` |
| `rDrawAxis` | `0x60` | 4 | `0` | | | `m_bDrawAxis` |
| `rDrawPivot` | `0x60` | 4 | `0` | | | `m_bDrawPivot` |
| `rDrawPhysModel` | `0x60` | 4 | `0` | | | `m_bDrawPhysModel` |
| `rDrawAABB` | `0x60` | 4 | `0` | | | `m_bDrawAABB` |
| `rDrawLights` | `0x60` | 4 | `0` | | | `m_bDrawVertexLights` |
| `rUseLights` | `0x60` | 4 | `1` | | | `m_bUseVertexLight` |
| `rCastLights` | `0x60` | 4 | `0` | | | `m_bCastLights` |
| `rDrawBSPBound` | `0x60` | 4 | `0` | | | `drawBound` |
| `rDrawPlanes` | `0x60` | 4 | `0` | | | `drawPlanes` |
| `rDrawPath` | `0x60` | 4 | `0` | | | `DAT_0043d410` |
| `rFromServer` | `0x60` | 4 | `0` | | | `DAT_0043d414`. The frame also stores 1 here |
| `rDrawBillboard` | `0x60` | 4 | `1` | | | `drawBillboards` |
| `rDrawLevel` | `0x02` | 4 | `1` | | | `DAT_0043b4fc`. Readonly to typed input |
| `rLodLevel` | `0x80` | 6 | `0.0` | `0.0` | `1.0` | `g_fMinLod` |
| `rShadowLod` | `0x80` | 6 | `1.0` | `0.0` | `1.0` | `g_fShadowLod` |
| `rShadowDist` | `0x80` | 6 | `10.0` | `0.0` | none | `m_fShadowDist` |
| `rWaterReflect` | `0x80` | 4 | NULL | | | `m_bUseReflection` |
| `rWaterRefract` | `0x80` | 4 | NULL | | | `m_bUseRefraction` |
| `fxDraw` | `0x60` | 4 | `1` | | | `m_bDraw` |
| `fxUpdate` | `0x60` | 4 | `1` | | | `m_bUpdate` |
| `lastError` | `0x02` | 7 | `""` | | | not linked |
| `bspDrawDetail` | `0x80` | 4 | `1` | `0` | `1` | `drawDetail` |
| `bspUseLightmap` | `0x80` | 4 | `1` | `0` | `1` | `useLightmap` |
| `bspUseTexture` | `0x60` | 4 | `1` | `0` | `1` | `useTextures` |
| `bspUseVis` | `0x60` | 4 | `1` | `0` | `1` | `useVis` |
| `dirmapDir` | `0x60` | 6 | NULL | `0` | `1` | `m_fDirmapDirectional` |
| `dirmapAmb` | `0x60` | 6 | NULL | `0` | `1` | `m_fDirmapAmbiental` |
| `bspDrawSky` | `0x80` | 4 | `1` | `0` | `1` | `showSky` |
| `bspDrawGrass` | `0x80` | 4 | `1` | `0` | `1` | `drawGrass` |
| `bspDrawRain` | `0x80` | 4 | `1` | `0` | `1` | `drawRain` |
| `bspUnderWaterReflect` | `0x80` | 4 | `0` | `0` | `1` | `drawUnderWaterReflection` |

Commands from the same function. Arg types are the vararg tags before `0xffffffff`.

| name | flags | args | handler as printed |
| --- | --- | --- | --- |
| `Exit` | | none | string Main7 RVA `0x2a3e0`. Handler `LAB_00410a40` |
| `AppVersion` | | none | `FUN_00411010` |
| `AppReport` | | none | `FUN_00414090` |
| `AppResolution` | | int, int | `FUN_00414280` |
| `AppWindowed` | | int | `FUN_00414450` |
| `AppReportShaders` | `0x20` | none | `FUN_00410a70` |
| `AppReportBspShaders` | `0x20` | none | `FUN_00410b90` |
| `AppDumpTextures` | `0x20` | none | `FUN_00410cb0` |
| `AppDumpTextureFiles` | `0x20` | string | `FUN_004120e0` |
| `AppDumpModels` | `0x20` | none | `FUN_00410dd0` |
| `AppDumpAlloc` | `0x20` | none | `FUN_00410ef0` |
| `DumpConsole` | `0x20` | none | `FUN_00411150` |
| `AppTakeCubeMap` | `0x20` | string | `FUN_00412210` |
| `rDrawDebug` | `0x60` | int | `LAB_00412870` |
| `bspDrawReset` | `0x60` | none | `LAB_00411940` |
| `bspDrawTexture` | `0x60` | none | `LAB_00411990` |
| `bspDrawWire` | `0x60` | none | `LAB_004119c0` |
| `bspDrawLeafs` | `0x60` | none | `LAB_004119f0` |
| `bspDrawZones` | `0x60` | none | `LAB_00411a20` |
| `bspDrawNormals` | `0x60` | none | `LAB_00411a50` |
| `bspDrawInvisible` | `0x60` | int | `LAB_00411c60` |
| `bspDrawTerrainLod` | `0x60` | int | `FUN_00411b40` |
| `bspBlendDistance` | `0x20` | float | `LAB_00411b10` |
| `bspCTB` | `0x20` | string, int | `FUN_00411ce0` |
| `bspProfile` | `0x20` | float, float | `FUN_00411f80` |
| `FogStart` | `0x60` | float | `FUN_00411500` |
| `FogEnd` | `0x60` | float | `FUN_00411650` |
| `FogColor` | `0x60` | int, int, int | `FUN_004117d0` |
| `FogInfo` | `0x60` | none | `LAB_00411920` |
| `batchmode` | `0x20` | none | `FUN_00408700`, registered in `FUN_00408840` |

### Main7 server and client (`FUN_00421750` RVA `0x21750`, `FUN_0041d560` RVA `0x1d560`)

| name | kind | flags | type | default | min | max | notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `spmap` | cmd | | | | | | string arg. Handler `FUN_004209b0`. Loads `Bsp\<name>.bsp` |
| `menumap` | cmd | `0x01` | | | | | string arg. Handler `FUN_00420b60` |
| `sv_changeMap` | cmd | | | | | | string. `FUN_00420f40` |
| `sv_restart` | cmd | | | | | | `FUN_00420df0` |
| `sv_stop` | cmd | | | | | | `FUN_00420ce0` |
| `sv_options` | cmd | | | | | | `FUN_004210d0` |
| `sv_save` | cmd | | | | | | string. `FUN_00421200` |
| `sv_load` | cmd | | | | | | string. `FUN_004213c0` |
| `sv_varchaged` | cmd | `0x01` | | | | | spelled that way in the binary. `FUN_00420870` |
| `sv_port` | cvar | `0x02` | 4 | `sprint("%d", 0x6ab6)` = 27318 | `1000` | `32768` | cvar* at `server+0x1e40`. Readonly |
| `sv_password` | cvar | | 7 | `""` | | | `server+0x1e4c` |
| `sv_multiplayer` | cvar | `0x02` | 4 | `0` | `0` | `1` | `server+0x1e48`. Readonly |
| `sv_fps` | cvar | | 4 | `100` | `2` | none | not linked. cvar* `server+0x1e44`. See `timing.md` |
| `cfVersion` | cvar | `0x102` | 7 | `sprint("'Server v%d.%d'", 2, 5)` so the default text includes the quotes: `'Server v2.5'` | | | `server+0x1e50` |
| `cfServerName` | cvar | `0x100` | 7 | `DefaultServer` | | | `server+0x1e54` |
| `cfMapName` | cvar | `0x102` | 7 | `""` | | | `server+0x1e60` |
| `cfMaxClients` | cvar | `0x100` | 4 | `1` | `1` | `32` | `server+0x1e58` |
| `cfNumClients` | cvar | `0x102` | 4 | NULL | | | linked to `server+0x1d18` |
| `cfNeedPassword` | cvar | `0x100` | 4 | `0` | | | `server+0x1e5c` |
| `cfLobby` | cvar | `0x102` | 4 | `0` | `0` | `1` | linked to `DAT_0043d6b4`. `FUN_0041ed80` sets that dword to 0 at the start of a load |
| `cl_disconnect` | cmd | | | | | | `FUN_0041d2d0` |
| `cl_playerName` | cvar | | 7 | `Default` | | | not linked |
| `cl_password` | cvar | | 7 | `""` | | | not linked |

### GameDll7 bindings and mods (`FUN_1000d9f0` RVA `0xd9f0`)

Key slots. All type 4, default `"-1"`, range `-1..1024`, no flags. `-1` means unbound. `com_slowmotion` and `com_slomo` share one DWORD.

| name | storage | name | storage |
| --- | --- | --- | --- |
| `com_forward` | `DAT_10189e18` | `com_previtem` | `DAT_10189e48` |
| `com_backward` | `DAT_10189e1c` | `com_slowmotion` | `DAT_10189e60` |
| `com_strafeleft` | `DAT_10189e20` | `com_slomo` | `DAT_10189e60` |
| `com_straferight` | `DAT_10189e24` | `com_defend` | `DAT_10189e50` |
| `com_run` | `DAT_10189e28` | `com_skipcutscene` | `DAT_10189e54` |
| `com_aim` | `DAT_10189e30` | `com_zoomin` | `DAT_10189e58` |
| `com_togglecrouch` | `DAT_10189e34` | `com_zoomout` | `DAT_10189e5c` |
| `com_use` | `DAT_10189e38` | `com_tasks` | `DAT_10189e64` |
| `com_fire` | `DAT_10189e3c` | `com_secfire` | `DAT_10189e40` |
| `com_reload` | `DAT_10189e44` | `com_nextitem` | `DAT_10189e4c` |

The function writes `DAT_10189e58 = 0x10c`, `DAT_10189e5c = 0x10d`, and `DAT_10189e64 = 0xf` before `AddVar`. `AddVar` then scans the default `"-1"`. Whether the link keeps those three integers is UNVERIFIED. The registered default is `-1`.

Other cvars from the same function. Flags blank unless noted. `mod_force3rd` is the registered name. A later `RemoveSlot` list spells `mode_force3rd`. Trust the `AddVar` string.

| name | flags | type | default | min | max | storage |
| --- | --- | --- | --- | --- | --- | --- |
| `mod_viewDistance` | | 4 | `50` | `0` | `100` | `DAT_10165224` |
| `mod_vegetationDistance` | | 4 | `50` | `0` | `100` | `DAT_10165220` |
| `mod_noHUD` | | 4 | `0` | `0` | `1` | `DAT_10185f20` |
| `mod_resolution` | | 4 | `4` | `0` | not a C string (pointer GameDll RVA `0x185f18`) | `DAT_10160730` |
| `mod_rayhittime` | | 6 | `0.15` | `0.0` | `100.0` | `DAT_1016240c` |
| `mod_softlockType` | | 4 | `2` | `0` | `3` | `DAT_10163be0` |
| `mod_lockTargetOnHitTime` | | 6 | `1.5` | `0.0` | `100.0` | `DAT_10163bdc` |
| `mod_rotationDisableLock` | | 6 | `80.0` | `0.0` | `100.0` | `DAT_10164560` |
| `mod_lockSize` | | 6 | `70.0` | `0.0` | `640.0` | `DAT_10163bd8` |
| `mod_lockValueX` | | 6 | `10.0` | `0.0` | `720.0` | `DAT_10164558` |
| `mod_lockValueY` | | 6 | `10.0` | `0.0` | `720.0` | `DAT_1016455c` |
| `mod_unlockArena` | | 4 | `0` | `0` | `1` | `DAT_10185de4` |
| `thoughSkin` | | 4 | `0` | `0` | `1` | `DAT_10185de0` |
| `infiniteAmmo` | | 4 | `0` | `0` | `1` | `DAT_10185dc8` |
| `zeroGravity` | | 4 | `0` | `0` | `1` | `DAT_10185dcc` |
| `quickRegen` | | 4 | `0` | `0` | `1` | `DAT_10185dd0` |
| `doubleDamage` | | 4 | `0` | `0` | `1` | `DAT_10185dd4` |
| `doubleHealth` | | 4 | `0` | `0` | `1` | `DAT_10185dd8` |
| `slowMotion` | | 4 | `0` | `0` | `1` | `DAT_10185ddc`. Not the HUD path. See `timing.md` |
| `bloomtype` | | 4 | `-1` | `0` | `1` | `DAT_10185ef0` |
| `bloomindex` | | 4 | `-1` | `0` | `6` | `DAT_10185eec` |
| `bloomtresh` | | 4 | `0` | `0` | `1` | `DAT_10185ef4` |
| `bloommodulate` | | 4 | `0` | `0` | `1` | `DAT_10185ef8` |
| `bloomalpha` | | 6 | `0` | `0.0` | `1.0` | `DAT_10185efc` |
| `bloomred` | | 6 | `0` | `0.0` | `1.0` | `DAT_10185f00` |
| `bloomgreen` | | 6 | `0` | `0.0` | `1.0` | `DAT_10185f04` |
| `bloomblue` | | 6 | `0` | `0.0` | `1.0` | `DAT_10185f08` |
| `mod_player_ent` | | 7 | NULL | | | not linked |
| `mod_player_cls` | | 7 | NULL | | | not linked |
| `fastmode` | | 4 | `0` | `0` | `1` | `DAT_10186640` |
| `eyehurt` | | 4 | `0` | `0` | `1` | `DAT_10186644` |
| `earhurt` | | 4 | `0` | `0` | `1` | `DAT_10186648` |
| `leghurt` | | 4 | `0` | `0` | `1` | `DAT_1018664c` |
| `armhurt` | | 4 | `0` | `0` | `1` | `DAT_10186650` |
| `randomhurt` | | 4 | `0` | `0` | `100` | `DAT_10186654` |
| `poison` | | 4 | `0` | `0` | `1` | `DAT_10186658` |
| `revive` | | 4 | `0` | `0` | `1` | `DAT_1018665c` |
| `mod_deactivatetriggers` | | 4 | `0` | `0` | `1` | `DAT_101869f8` |
| `mod_freezanimals` | | 4 | `0` | `0` | `1` | `DAT_10186058` |
| `mod_showTimer` | | 4 | `0` | `0` | `1` | `DAT_10186660` |
| `mod_startTimer` | | 4 | `0` | `0` | `1` | `DAT_10186664` |
| `mod_stopTimer` | | 4 | `0` | `0` | `1` | `DAT_10186668` |
| `mod_resetTimer` | | 4 | `0` | `0` | `1` | `DAT_1018666c` |
| `timer` | | 6 | `0` | `0` | `10000` | `DAT_10185f14` |
| `mod_yawAccelVal1` | | 6 | `0.0001` | `0` | `1` | `DAT_1016087c` |
| `mod_yawAccelVal2` | | 6 | `1.1` | `0` | `5` | `DAT_10160880` |
| `mod_yawAccelTime` | | 6 | `0.5` | `0` | `10` | `DAT_10160884` |
| `mod_noInputTime` | | 6 | `0.03` | `0` | `10` | `DAT_10160888` |
| `mod_pitchFactor` | | 6 | `0.5` | `0` | `10` | `DAT_1016088c` |
| `mod_yawInertia` | | 6 | `0.6` | `0` | `1` | `DAT_10160890` |
| `mod_inputType` | | 4 | `0` | `0` | `1` | `DAT_1018600c` |
| `mod_input2_yawAccel` | | 6 | `360` | `30` | `7200` | `DAT_10160894` |
| `mod_input2_yawDecel` | | 6 | `720` | `30` | `7200` | `DAT_10160898` |
| `mod_input2_pitchAccel` | | 6 | `360` | `30` | `7200` | `DAT_1016089c` |
| `mod_input2_pitchDecel` | | 6 | `360` | `30` | `7200` | `DAT_101608a0` |
| `mod_weaponAimSens` | | 6 | `0.25` | `0.01` | `1` | `DAT_10161dac` |
| `mod_weaponAimFov` | | 6 | `20` | `10` | `60` | `DAT_10161da8` |
| `mod_weaponAimTime` | | 6 | `0.5` | `0.01` | `5` | `DAT_10161db0` |
| `mod_dodgeTurnTime` | | 6 | `1.0` | `0.01` | `3` | `DAT_10162404` |
| `mod_dodgeTurnSpeed` | | 6 | `360` | `0` | `720` | `DAT_10162408` |
| `mod_slowmotionFrameTime` | | 6 | `0.35` | `0.01` | `1` | `DAT_10164d38` |
| `mod_slowmotionFadeTime` | | 6 | `5` | `0.5` | `20` | `DAT_10164d3c` |
| `mod_slowmotionBlur` | | 4 | `160` | `0` | `192` | `DAT_10164d48` |
| `mod_ignoreGR` | | 4 | `0` | `0` | `1` | `DAT_10185f10` |
| `mod_force3rd` | `0x20` | 4 | `0` | | | not linked |

Commands, flag `0x20` unless noted. Handlers were not recovered (the decompiler kept the name and dropped the function pointer).

From `FUN_1000d9f0`: `mod_loadlevel`, `mod_movetopos`, `mod_batchmode`, `mod_failmission`, `mod_completemission`, `mod_minimap`, `mod_godmode`, `mod_hurt`, `mod_showmissionstatus`, `mod_camoffset`, `mod_setdiff`, `mod_setdirlightdown`, `mod_setinitialdirlight`, `mod_setamblight`, `mod_shootstumps`.

From `FUN_10010b80` (RVA `0x10b80`), flag `0x20`, member functions: `mod_dbgai_type`, `mod_dbgai_target`.

From `FUN_100bd6d0` (RVA `0xbd6d0`):

| name | flags |
| --- | --- |
| `mainmenu`, `gamemenu`, `closemainmenu`, `closegamemenu` | `0x01` |
| `mod_spmap` | `0x20` |
| `mod_newlevel`, `mod_newhunt`, `mod_startlevel`, `mod_restartmission`, `mod_gameloadmission`, `mod_gameloadbonus`, `mod_playendfmv` | `0x01` |

`mod_godmode` (command, flag `0x20`) and `godMode` (cvar, flag `0x40`) are different symbols.

## Map, spawn, god, noclip, AI, network

Map. `spmap <name>` and `menumap <name>` are the commands that call `cBspTree::Load`. `-spmap <name>` queues `spmap`. `mod_spmap` is a second command, flag `0x20`. Its handler was not decompiled, so whether it also loads a bsp is UNVERIFIED. `sv_changeMap` restarts into a new bsp only from server state 3. `sv_load` / `sv_save` take a string. They were not decompiled. UNVERIFIED whether they load a map or a save slot. There is no registered command `map`. `-map` still tries to run one.

Spawn. No command whose name contains `spawn` is in these registration dumps. `mod_player_ent` and `mod_player_cls` are string cvars with a NULL default and no linked storage. What reads them was not decompiled. `mod_loadlevel` and `mod_newlevel` are the nearest level commands. Their bodies were not decompiled.

God. `godMode 1` is the CommonDll cheat cvar (`m_bGodMode`), flag `0x40`, so a typed line needs `-cheats`. `mod_godmode` is a GameDll command, flag `0x20`, so a typed line needs `-developper`. An internal `Exec(..., 0)` runs either one without the switches.

Noclip. `noClip 1`, same cheat gate as `godMode`, exported `m_bNoClip`. `debugCamera` is the third cheat cvar.

AI. `mod_freezanimals` is a 0/1 cvar with no flag (typed use does not need `-cheats`). `mod_dbgai_type` and `mod_dbgai_target` are developer commands. `-buildai <dll>` is a startup tool that writes `ai.lua` and does not enter the game. `mod_deactivatetriggers` is a 0/1 cvar with no flag.

Network. `sv_port` default 27318, readonly to typed input, range 1000..32768. `sv_multiplayer` default 0, readonly, range 0..1. `sv_password`, `cfServerName` default `DefaultServer`, `cfMaxClients` default 1 max 32, `cfNeedPassword` default 0, `cfNumClients` linked to the live count, `cfMapName`, `cfLobby`, `cfVersion` default `'Server v2.5'`. Client: `cl_playerName` default `Default`, `cl_password`, `cl_disconnect`. `-join <addr>` queues `cl_join`, which is not in the registration dumps. `-dedicated` is the listen/server process switch, not a cvar. `showNetStats` and `showNetGraph` are developer cvars. The `0x100` bit on the `cf*` vars is not given a meaning by `IsSlotVisible` or `RealExec`. Treating it as "replicated" is UNVERIFIED.

## Runtime experiments requested

Track E owns the launch. Working directory `game\Bin`, disc mounted. Do not patch binaries for these. Do not write the original install.

1. `Main7.exe +mod data` with no `-developper` and no `-cheats`. Hold Q+I+F12 (or A+V+F12), press the grave/tilde key, type `3.141692`, press Enter. The password line must not run as a command. Then type `frameTime` and confirm a value prints (default 1). Type `showStats 1` and confirm it does not take effect (developer bit is off). Type `godMode 1` and `noClip 1` and confirm they do not take effect. Copy the `symbol '...' not found!` lines from `debug.log` if that is what appears. A hidden slot and a missing name both fail `FindSlot` visibility. Record which message you get.
2. Quit. Relaunch `Main7.exe +mod data -developper`. Same chord, grave, password. `showStats 1` should now be accepted. `godMode 1` should still be rejected. Relaunch once more with `-developper -cheats` and confirm `godMode 1` and `noClip 1` are accepted. Toggle them back to 0 before quitting so `console.cfg` does not keep a cheat. `godMode` has no `0x80` bit, so `Save` should not write it. Confirm by reading `Cfg\console.cfg` afterward: it should contain `addvar <name> <type>; <name> = <value>` lines, and it should not contain `godMode` unless something else archived it.
3. With the console open, run `help` (or `?`) and `DumpConsole` (needs `-developper`). Save that print next to this table and mark any name this file missed.
4. `spmap` of a real map, then a fresh launch with `-spmap <same>`. Both should log `map '<name>' loaded succesfully`. A fresh launch with `-map <same>` should log `symbol 'map' not found!`. If the map loads, record the command that actually ran.
5. `-join 127.0.0.1` on a second copy, while one copy is already running. The mutex allows it only because `-join` is present. Capture the log around `cl_join`. Expected, until a registration shows up: `symbol 'cl_join' not found!`. A second copy with neither `-join` nor `-dedicated` should exit.
6. One `-dedicated` launch. Confirm no rendered view, that `r_api` and `snd_useSound` read back as 0, and that typing a line on stdin (for example `sv_fps`) echoes and runs without the grave chord. This is the dedicated console path.
7. `-nointro` should skip the intro movies. `-exp` should log `Scripter.exp WILL BE exported !` if `ExportClasses` runs. `-debug` should show whatever `cBspTree::ShaderReport` adds. Capture those log lines. If `-exp` prints nothing, `ExportClasses` was not called and the switch is inert at startup.

## Proposed MEMORY facts

- In-game console toggle is DIK grave (`0x29`) at `input+0x204`. The DIK DWORD array starts at `input+0x160`. Evidence: `cConsoleView::Update` Engine RVA `0x16daf0`, and `MemDump_on_F10_` / `MemDump_on_NUM8_` in `FUN_0040b400`.
- The toggle stays locked until A+V+F12 or Q+I+F12. Engine RVA `0x56a530` is 1 in the file. The first line must be `3.141692` (Engine RVA `0x41ff78`, 9-byte compare) which clears RVA `0x56a534` and is not executed. `-developper` does not clear either lock.
- `-developper` sets console `+0x18` bit 2. `-cheats` sets bit 4. `IsSlotVisible` (Engine RVA `0x15d040`) hides slot flag `0x20` without bit 2 and slot flag `0x40` without bit 4, and only on lines that start with `%`. `Exec(console, line, 1)` adds the `%`. `Exec(..., 0)`, `Load`, and dedicated stdin do not.
- `godMode` and `noClip` are CommonDll cvars, flag `0x40`, default 0, range 0..1, exported statics `m_bGodMode` and `m_bNoClip`. `debugCamera` is the third. `mod_godmode` is a different GameDll command, flag `0x20`.
- `InitConsole` (Engine RVA `0x329ec0`) constructs `console`, then `Load("Cfg\\console.cfg")`. `Save` writes `addvar <name> <type>; <name> = <value>` for var slots with flag `0x80`. Shutdown save is `FUN_0040cca0` when not dedicated, via `cFileManager::GetFullPath`.
- Registration: `cConsole::AddStaticFuncf` Engine RVA `0x166bd0`. Tags 4 int, 6 float, 7 string, terminator `0xffffffff`. `coop_host` / `coop_join` with flag 0 and one string: `AddStaticFuncf(console, 0, "coop_host", fn, 7, 0xffffffff)` after `InitConsole`.
- `frameTime` default 1.0, range 0.001..100, no flags, linked Main7 RVA `0x3b5d0`. `sv_fps` default 100, min 2, cvar* at `server+0x1e44`. `sv_port` default 27318 (`0x6ab6`), flag `0x02`. `cfMaxClients` default 1, max 32.
- Switches that change boot: `+mod`, `+path`, `-dedicated` (`DAT_0043d2bc`), `-join` queues `cl_join` (no registration in the dumps), `-developper`, `-cheats`, `-spmap` queues `spmap`, `-map` queues `map` (no such command in the dumps), `-batchmode`, `-nointro`, `-testmodels`, `-buildai`, `-res`, `-shaderdup`, `-logfiles`, `-console`, `-console98`, `-exp` (`cClassManager::ExportClasses`, Engine RVA `0x2553b0`), `-debug` (only xref `cBspTree::ShaderReport`).
- Map command is `spmap` (`FUN_004209b0`) or `menumap` (`FUN_00420b60`), both ending in `cBspTree::Load("Bsp\\%s.bsp", 0x6ff, 1)` from `FUN_004101d0`.
- `cGuiEvent::ScrExport` assigns `EV_COMMAND = 4` and `CM_OK = 0x100`. `cGuiManager::m_mapVars` is `map<string, void(*)(char*)>`. The button dispatch into `Exec` is UNVERIFIED.
