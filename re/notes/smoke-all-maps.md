# Smoke baseline: every story map, main build (2026-10-18)

Job smoke-all-maps (wave playthrough). Worktree `D:\scary hunt\dh2coop-smoke`, branch `smoke-pass`, port 27690, Release build of the mod from the checkout (`.\mod\build.ps1`). The pass ran while main moved, so each row names the build it ran on (column **build**: main hash + the `dh2coop.asi` build id the host logged):

| main | asi build id | rows |
|---|---|---|
| `a3322d1` | `7fc4953c` | MAP1 - MAP5 (`base-a3322d1`), the first MAP6/MAP7 runs (`base-a3322d1`) |
| `3e33695` | `16064da7` | MAP6 - MAP11A (`base`) |
| `13b5efa` (has `eccc1ef`: test sessions bind 127.0.0.1) | `43bad5c4` | MAP11B - MAP12C (`base`), the level-flow and 3-player runs |

MAP1 - MAP5 were not repeated on the newer builds (about 12 minutes if the lead wants one uniform hash).

## Method

- One two-window run per map: `.\run-coop-local.ps1 -Map <MAP> -Port 27690 -Seconds 120 -Capture` (host `game`, client `game2`, no keyboard or mouse input), driven by `tools-src\smoke-maps.ps1 -Phase base`. The countdown starts when the client is welcomed.
- A watchdog in the script polls the processes every 3 s (still running, window responding, no `WerFault` for that pid): `alive.txt` of each run. The run script itself aborts with `exited before the run finished (code N)` when a process dies.
- Logs of every tree are copied after each run (`re\notes\coop-local\smoke\<MAP>\<phase>\`: `game-dh2coop.log`, `game2-dh2coop.log`, `*-debug.log`, `run-output.txt`, `alive.txt`, `status.txt`, `build.txt`, `row.md`). Rows come from `tools-src\smoke-row.sh`; `tools-src\smoke-summary.sh <dir>` is the long digest (map loads, log families, first-chance sites).
- Screenshots: `re\notes\coop-local\smoke-<MAP>-<phase>-<stamp>-game|game2.png` (4 of the 12 the run script takes are kept, spread over the run; the older builds' ones carry `base-a3322d1`).
- Columns: **alive H/C** = last watchdog poll of the host / client before the run script closes the windows (`ok` = running and responding). **welcome** = seconds from the client process start to `coop: welcomed`; **sync** = `events: synced with host` relative to that line. **animals H/C** = host `animals: host snapshot N` / client `animals: client received N`, in brackets the client's distinct `mirror freeze ... ok=true` ids (animals + NPCs), cobras (`ok=false` is expected for them: they are not `cAIHuntBrain_Animal`) and real failures (`bad`). **trig** = host armed/fired (unique `cScrTaskNN`, armed = `trigger event activate`, fired = `trigger FIRE`/`ran`), client armed/fired/own-fire-sent (map-load arms and fires run locally in both processes before the join, role `solo`), `defer` = `story: defer host fire` lines on the client. **first-chance** = `first-chance exception` lines that are not `nvd3dum.dll` (the logger stops after 16 per process); a site within 3 s of the end of the log is listed apart as "at shutdown". **crash** = `unhandled exception` lines or a dump. **fwd** = `Could not forward animation` in `debug.log`, host/client. **lines/s** = `dh2coop.log` lines per second after the first 10 s of the process (average, peak second); the steady diagnostic baseline is about 10-20 lines/s (`animals: sync`, `tracker: animal`). **-1** = `SetBodyState ... -> -1` lines, **refused/mismatch** = lines with either word (all logs). **host mission msg** = the first `events: host mission message` (code 6 = "Mission failed") and seconds since the host log began (the run script's countdown starts about 5 s later). **clients 3-4** = the extra windows of a 3-4 player run.

## Limits of this smoke (read before trusting a green row)

- **No input is sent**, so the hunters stand where the map puts them. Maps that open with a modal message (MAP1: the tutorial text, "RETURN continue") sit on it for the 2 minutes. Trigger volumes are not walked; only what fires on its own in the first 2 minutes shows up. A clean row means "loads, syncs, idles without a flood or crash", not "the mission is playable". Mission-failed rows below are the unattended host being killed (design note 7 of the wave brief), not a sync fault.
- Nothing moves a mission forward, so late-map crash classes (vehicles after the start, bosses past their opening, exit zones) are only reached when the map starts them by itself. The trigger walk and exit phases of the script were not run in this pass.
- Run-to-run noise: first-chance `AUDIOSES.DLL`/`nvd3dum.dll` access violations happen inside Windows audio / the NVIDIA driver and do not change anything the mod does.
- `coop_level restart|next` arms and confirms in one write (`cmd_level` calls the handler twice), so the brief's "twice within 3 s" became two writes 1.2 s apart: harmless for restart (the map is already the menu), but `next` then jumped two levels (`MAP3C` -> `MAP4A` -> `MAP4B`, kept as a stress result). A single write was run too (`next-once`).

## Baseline table (2 windows, 120 s each)

| map / run | alive H/C | welcome, sync | animals H/C (mirrored) | trig host arm/fire, client arm/fire/sent, defer | first-chance (not nvd3dum) | crash | fwd H/C | lines/s H / C avg(peak) | -1 | refused/mismatch | host mission msg | build | clients 3-4 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| MAP1/base-a3322d1 | ok/ok <ok> | 1.7s, sync +0.0s | 8/8 (7+1 npc, 0 cobra, 0 bad) | 2/1, 2/1/1, defer 0 | none | no | 0/0 | 9.7(18) / 9.7(19) | 0 | 0 | - | main a3322d1 (asi 7fc4953c) | - |
| MAP2A/base-a3322d1 | ok/ok <ok> | 1.5s, sync +0.1s | 11/11 (10+1 npc, 0 cobra, 0 bad) | 5/1, 5/1/0, defer 0 | none [at shutdown: game:0xc0000005@AUDIOSES.DLL+0x275ba x1; game2:0xc0000005@AUDIOSES.DLL+0x275ba x1] | no | 0/0 | 12.6(36) / 12.8(36) | 0 | 0 | - | main a3322d1 (asi 7fc4953c) | - |
| MAP2B/base-a3322d1 | ok/ok <ok> | 1.7s, sync +0.0s | 13/13 (12+1 npc, 0 cobra, 0 bad) | 2/1, 2/1/0, defer 0 | none [at shutdown: game:0xc0000005@dh2coop.asi+0x1062c7 x2; game2:0xc0000005@dh2coop.asi+0x1062c7 x2; game:0xc0000005@AUDIOSES.DLL+0x275ba x1; game2:0xc0000005@AUDIOSES.DLL+0x275ba x1] | no | 0/0 | 15.8(31) / 16.8(36) | 0 | 0 | code 6 'Mission failed' +86s | main a3322d1 (asi 7fc4953c) | - |
| MAP2C/base-a3322d1 | ok/ok <ok> | 1.7s, sync +0.0s | 11/11 (8+3 npc, 0 cobra, 0 bad) | 2/1, 2/1/0, defer 0 | none [at shutdown: game:0xc0000005@AUDIOSES.DLL+0x275ba x1; game2:0xc0000005@AUDIOSES.DLL+0x275ba x1] | no | 0/0 | 12.5(26) / 12.3(24) | 0 | 0 | - | main a3322d1 (asi 7fc4953c) | - |
| MAP3A/base-a3322d1 | ok/ok <ok> | 1.7s, sync +0.0s | 13/13 (12+1 npc, 0 cobra, 0 bad) | 11/1, 11/1/0, defer 0 | none [at shutdown: game2:0xc0000005@dh2coop.asi+0x1062c7 x2; game:0xc0000005@AUDIOSES.DLL+0x275ba x1; game2:0xc0000005@AUDIOSES.DLL+0x275ba x1] | no | 0/0 | 14.8(38) / 14.8(38) | 0 | 0 | - | main a3322d1 (asi 7fc4953c) | - |
| MAP3B/base-a3322d1 | ok/ok <ok> | 2s, sync +0.1s | 8/8 (5+0 npc, 3 cobra, 0 bad) | 4/1, 4/1/0, defer 0 | none [at shutdown: game:0xc0000005@dh2coop.asi+0x1062c7 x2; game:0xc0000005@AUDIOSES.DLL+0x275ba x1; game2:0xc0000005@AUDIOSES.DLL+0x275ba x1] | no | 0/0 | 10.2(20) / 10.3(20) | 0 | 0 | - | main a3322d1 (asi 7fc4953c) | - |
| MAP3C/base-a3322d1 | ok/ok <ok> | 1.8s, sync +0.1s | 11/11 (6+1 npc, 4 cobra, 0 bad) | 2/1, 2/1/0, defer 0 | none [at shutdown: game:0xc0000005@AUDIOSES.DLL+0x275ba x1; game2:0xc0000005@AUDIOSES.DLL+0x275ba x1] | no | 0/0 | 12.7(25) / 12.8(25) | 0 | 0 | - | main a3322d1 (asi 7fc4953c) | - |
| MAP4A/base-a3322d1 | ok/ok <ok> | 2.3s, sync +0.0s | 13/13 (12+1 npc, 0 cobra, 0 bad) | 5/3, 5/3/0, defer 0 | none | no | 0/18 | 16.2(50) / 16.3(46) | 6 | 0 | code 6 'Mission failed' +66s | main a3322d1 (asi 7fc4953c) | - |
| MAP4B/base-a3322d1 | ok/ok <ok> | 2.2s, sync +0.1s | 17/17 (16+1 npc, 0 cobra, 0 bad) | 17/1, 17/1/0, defer 0 | none | no | 0/0 | 18.1(37) / 18.4(37) | 0 | 0 | - | main a3322d1 (asi 7fc4953c) | - |
| MAP5/base-a3322d1 | ok/ok <ok> | 2.5s, sync +0.0s | 13/13 (11+2 npc, 0 cobra, 0 bad) | 18/1, 18/1/0, defer 1 | none | no | 0/0 | 14.2(51) / 14.6(51) | 0 | 0 | - | main a3322d1 (asi 7fc4953c) | - |
| MAP6/base-a3322d1 | ok/ok <ok> | 2s, sync +0.0s | 12/12 (9+3 npc, 0 cobra, 0 bad) | 5/1, 5/1/1, defer 0 | none [at shutdown: game:0xc0000005@dh2coop.asi+0x1062c7 x2; game2:0xc0000005@dh2coop.asi+0x1062c7 x2] | no | 0/0 | 12.4(25) / 12.5(24) | 0 | 0 | - | main a3322d1 (asi 7fc4953c) | - |
| MAP6/base | ok/ok <ok> | 2.2s, sync +0.0s | 12/12 (9+3 npc, 0 cobra, 0 bad) | 5/1, 5/1/1, defer 0 | none | no | 0/0 | 12.5(25) / 12.6(25) | 0 | 0 | - | main 3e33695 (asi 16064da7) | - |
| MAP7/base-a3322d1 | DEAD/DEAD <DIED game pid=1932 exited before the run finished (code )> | 1.5s, sync +0.0s | 12/12 (9+2 npc, 1 cobra, 0 bad) | 11/1, 11/1/1, defer 0 | none | no | 0/0 | 18.5(27) / 18.7(35) | 0 | 0 | - | main a3322d1 (asi 7fc4953c) | - |
| MAP7/base | ok/ok <ok> | 1.7s, sync +0.1s | 12/12 (9+2 npc, 1 cobra, 0 bad) | 11/1, 11/1/1, defer 0 | none | no | 0/0 | 13.9(29) / 14.2(31) | 0 | 0 | code 6 'Mission failed' +69s | main 3e33695 (asi 16064da7) | - |
| MAP8/base | ok/ok <ok> | 1.9s, sync +0.1s | 14/14 (11+3 npc, 0 cobra, 0 bad) | 23/1, 23/1/0, defer 0 | none | no | 0/0 | 14.8(77) / 15.1(75) | 0 | 0 | - | main 3e33695 (asi 16064da7) | - |
| MAP9/base | ok/ok <ok> | 1.7s, sync +0.1s | 8/8 (7+1 npc, 0 cobra, 0 bad) | 13/1, 13/1/0, defer 0 | none | no | 0/0 | 9.7(21) / 9.8(20) | 0 | 0 | - | main 3e33695 (asi 16064da7) | - |
| MAP10/base | ok/ok <ok> | 1.9s, sync +0.0s | 19/19 (17+2 npc, 0 cobra, 0 bad) | 21/1, 21/1/1, defer 0 | none | no | 0/0 | 19.8(43) / 20.1(47) | 0 | 0 | - | main 3e33695 (asi 16064da7) | - |
| MAP11A/base | ok/ok <ok> | 1.9s, sync +0.1s | 13/13 (9+4 npc, 0 cobra, 0 bad) | 4/2, 4/2/0, defer 1 | none | no | 0/0 | 16.5(48) / 16.2(39) | 0 | 0 | - | main 3e33695 (asi 16064da7) | - |
| MAP11B/base | ok/ok <ok> | 1.8s, sync +0.0s | 12/12 (11+1 npc, 0 cobra, 0 bad) | 2/1, 2/1/1, defer 0 | none | no | 0/0 | 14.5(49) / 14.2(30) | 0 | 0 | - | main 13b5efa (asi 43bad5c4) | - |
| MAP11C/base | ok/ok <ok> | 2s, sync +0.0s | 11/11 (7+4 npc, 0 cobra, 0 bad) | 3/2, 3/2/2, defer 0 | none | no | 0/0 | 15.5(36) / 14.6(31) | 0 | 0 | - | main 13b5efa (asi 43bad5c4) | - |
| MAP12A/base | ok/ok <ok> | 1.9s, sync +0.1s | 15/15 (13+2 npc, 0 cobra, 0 bad) | 6/1, 6/1/1, defer 0 | none | no | 0/0 | 15.9(32) / 16.1(32) | 0 | 0 | - | main 13b5efa (asi 43bad5c4) | - |
| MAP12B/base | ok/ok <ok> | 1.9s, sync +0.1s | 1/1 (1+0 npc, 0 cobra, 0 bad) | 2/1, 2/1/0, defer 1 | none | no | 0/0 | 4.2(14) / 4.2(15) | 0 | 0 | code 6 'Mission failed' +23s | main 13b5efa (asi 43bad5c4) | - |
| MAP12C/base | ok/ok <ok> | 1.9s, sync +0.1s | 2/2 (1+1 npc, 0 cobra, 0 bad) | 2/1, 2/1/0, defer 1 | none | no | 0/18 | 4.9(16) / 4.7(13) | 6 | 0 | code 6 'Mission failed' +64s | main 13b5efa (asi 43bad5c4) | - |

## Level flow and 3-player runs

| map / run | alive H/C | welcome, sync | animals H/C (mirrored) | trig host arm/fire, client arm/fire/sent, defer | first-chance (not nvd3dum) | crash | fwd H/C | lines/s H / C avg(peak) | -1 | refused/mismatch | host mission msg | build | clients 3-4 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| MAP1/players3 | ok/ok <ok> | 1.7s, sync +0.1s | 8/8 (7+1 npc, 0 cobra, 0 bad) | 2/1, 2/1/1, defer 0 | none | no | 0/0 | 11.0(21) / 11.2(20) | 0 | 0 | - | main 13b5efa (asi 43bad5c4) | game3: ok, welcome 3.9s, synced x1, received 8, rate 11.3(22) |
| MAP2B/restart | ok/ok <ok> | 2.2s, sync +0.0s | 13/13 (12+1 npc, 0 cobra, 0 bad) | 2/1, 2/1/1, defer 0 | none | no | 0/0 | 18.6(116) / 18.8(142) | 0 | 0 | - | main 13b5efa (asi 43bad5c4) | - |
| MAP3C/next | ok/ok <ok> | 1.9s, sync +0.0s | 17/17 (34+3 npc, 4 cobra, 0 bad) | 17/1, 17/1/1, defer 0 | game:0xc0000005@dh2coop.asi+0x121777 x6;  | no | 0/0 | 22.9(122) / 23.3(189) | 0 | 0 | - | main 13b5efa (asi 43bad5c4) | - |
| MAP3C/next-once | ok/ok <ok> | 1.9s, sync +0.0s | 13/13 (18+2 npc, 4 cobra, 0 bad) | 5/3, 5/3/0, defer 0 | none | no | 0/18 | 19.3(106) / 19.7(156) | 6 | 0 | code 6 'Mission failed' +91s | main 13b5efa (asi 43bad5c4) | - |
| MAP4A/players3 | ok/ok <ok> | 2s, sync +0.0s | 13/13 (12+1 npc, 0 cobra, 0 bad) | 5/3, 5/3/0, defer 0 | none | no | 0/18 | 17.6(72) / 17.8(60) | 12 | 0 | code 6 'Mission failed' +64s | main 13b5efa (asi 43bad5c4) | game3: ok, welcome 1.9s, synced x1, received 13, rate 17.9(59) |
| MAP5/restart | ok/ok <ok> | 1.9s, sync +0.1s | 13/13 (11+2 npc, 0 cobra, 0 bad) | 19/2, 19/2/1, defer 2 | none | no | 0/0 | 18.4(88) / 22.1(128) | 0 | 0 | - | main 13b5efa (asi 43bad5c4) | - |
| MAP8/players3 | ok/ok <ok> | 1.9s, sync +0.1s | 14/14 (11+3 npc, 0 cobra, 0 bad) | 23/1, 23/1/0, defer 0 | none | no | 0/0 | 16.2(79) / 16.2(77) | 0 | 0 | - | main 13b5efa (asi 43bad5c4) | game3: ok, welcome 2.1s, synced x1, received 14, rate 16.6(77) |

## Verdict

All 21 story maps load in two windows, the client is welcomed in 1.5-2.5 s and is synced 0.0-0.1 s later, host and client log the same animal count on every map, and **no process died on its own** in any of the 21 baseline runs, the 2 restart runs, the 2 level-jump runs or the 3 three-player runs (no `unhandled exception`, no dump, no `WerFault`, every watchdog poll `ok/ok` except the two external closes below). No log flood: 4-23 lines/s steady, the highest single second after the first 10 s in a plain run is 77-79 on MAP8 (a periodic snapshot), and 106-189 only in the second after a map load during level jumps. No `refused`/`mismatch` line anywhere. VERIFIED by the rows above. UNVERIFIED: anything that needs input (see Limits).

Two outlier runs, both on `a3322d1` and both with host and client gone in the same watchdog poll: `MAP7/base-a3322d1` ended at +62 s with both processes shutting down cleanly (`puppets: unloading remote ...`, `Total allocated memory 0` in both logs, no exception), and the first MAP8 run (`base-a3322d1-partial`, `alive.txt` only) lost both at +46 s about a minute later. Both windows were closed from outside (another agent's cleanup or the user), not a crash. MAP7 and MAP8 on `3e33695` ran the full 2 minutes.

## Maps with something worth a look (decisive line each)

| map | what | decisive line (file) |
|---|---|---|
| MAP4A (`base-a3322d1`, `players3`, `MAP3C/next*`) | the lioness copy on the client still cannot play `idleattack1`; the 3-tries limit keeps it from flooding (18 engine errors, 6 `-> -1` lines per client, no crash, 12 with two clients) | client `debug.log`: `Could not forward animation idleattack1 from Legs to Legs model objects/animals/lionesse/lionesse.skin` x18; client `dh2coop.log`: `animals: SetBodyState id=N 'Attack' -> -1 (copy is in 'CutScene'); not tried again on this animal` |
| MAP12C | same class on the yeti copy, same 18 / 6 | client `debug.log`: `Could not forward animation idleattack1 from Legs to Legs model objects/animals/yeti/yeti.skin` x18; `animals: SetBodyState id=24 'Attack' -> -1 (copy is in 'CutScene'); not tried again on this animal` |
| MAP12B | unattended host dies 23 s after load while the BIGGRIM is in `Attack` (hp 1500 -> 1875 for 2 players) | host `dh2coop.log`: `animals: sync role=host id=55 ... state='Attack'` then `events: host mission message code=6 text='Mission failed` (+23 s); client `events: applied host mission end code=6` 9 ms later |
| MAP12C | idle host dies at +64 s with the yeti on the field (hp 2500 -> 3125 for 2 players) | `events: host mission message code=6 text='Mission failed` (+64 s), then `combat: animal id 24 hurt player 1 for 60.0 (type 1)` |
| MAP4A | the idle host dies at +66 s (`a3322d1`), +64 s (`players3`); the killer is not in the log | `events: host mission message code=6 text='Mission failed` |
| MAP7 | idle host killed at +69 s | same line (`3e33695`) |
| MAP2B | idle host killed at +86 s (`a3322d1`; the killer is not in the log) | same line |
| MAP3A, MAP4B, MAP8 (crocodiles), MAP3B, MAP3C, MAP7 (cobras) | scaling cannot add reserve animals of these species: identical on host and client, harmless at 2 players but it means no pack scaling there | `scale: AddAnimal(WATERCROCODILE, 'crocodile1') entity not found` (MAP3A x5, MAP4B x5, MAP8 x3 as `croco`), `scale: AddAnimal(COBRA, 'cobra1') entity not found` (MAP3B x3, MAP3C x4, MAP7 x1) |
| MAP3C `next` (two writes, `MAP3C` -> `MAP4A` -> `MAP4B` in 1.1 s) | 6 first-chance AVs on the host 11 ms after `MAP4B` loaded; caught by `safe_copy`, caller is the HUD layout reading `cGuiManager` while the GUI manager is being replaced | host `dh2coop.log` 14:54:10.772 `first-chance exception 0xc0000005 at dh2coop.asi+0x121777`; symbolized (build `13b5efa`): `safe_copy` (`safe_mem.cpp:14`) <- `menu_is_open`/`gameplay_icon_scale` (`display.cpp:128`) <- `hud_place` (`display.cpp:407,417`) <- `remap_hud_rect` (`display.cpp:440`). Harmless, display module |

Not problems, for the record: the `ok=false` mirror freezes on MAP3B (ids 49-51), MAP3C (52-55) and MAP7 (id 239) are the cobras (`tracker: cobras at map load ids=...`, handled); a client `defer host fire 'cScrTask01'` appears once on MAP5 (twice in the restart run, once per load), MAP11A, MAP12B and MAP12C (the host's opening cutscene) and the fire is applied afterwards, nothing keeps growing; MAP12B has a single animal (the bear) and ~4 lines/s because the game then sits on the mission-failed screen; MAP1 sits on its tutorial text.

## First-chance exceptions

- `AUDIOSES.DLL+0x275ba`: one per process, in the last second of every run (the windows are closing). Windows audio, no mod frame.
- `dh2coop.asi+0x1062c7` x2 per process at shutdown (MAP2B, 3A, 3B, 6, ...): symbolized against the `a3322d1` build (`mod\build` of that build, PDB matched): CRT `memcpy+0x507`, i.e. the guarded copy inside `dh2::core::safe_copy`. It is `puppets.cpp` `OnMapUnloading` running a second time over the same puppet (`puppets: unloading remote 1 puppet 0xdfe70e0 ref 2->1` then again `ref -99->-99 weapon 0x0`): the two `safe_read`s of `entity+0xb8` hit the freed entity and are caught. Harmless, but the second pass walks stale pointers (`unregister_target` too).
- `dh2coop.asi+0x121777` x6, host, only in the double level jump (table above).
- No other site; no `nvd3dum.dll` AV count above the known map-load noise.

## Level flow (F5 / F6, `coop_level`)

| run | result |
|---|---|
| `MAP2B/restart` | host `coop: host restarts 'MAP2B' through the main menu -> 1` at +27 s; both windows `MAP2B` -> `MENUMAP` -> `MAP2B` (host 14:49:32 / 14:49:34, client 14:49:32 / 14:49:34), client `events: following host to 'MENUMAP'` then `'MAP2B' (serial 3)`, `synced with host (3 commits replayed)`, `animals: client received 13 animals seq=521`; both alive and responding to the end, no warning/error line |
| `MAP5/restart` | same: `MAP5` -> `MENUMAP` -> `MAP5` in 2 s on both, both alive to the end, no exception |
| `MAP3C/next-once` (one write) | `coop: host skips 'MAP3C' -> 'MAP4A'`, both windows on `MAP4A` 1 s apart, alive to the end (then the MAP4A lioness lines above and an idle-host `Mission failed` at +91 s) |
| `MAP3C/next` (two writes) | `MAP3C` -> `MAP4A` -> `MAP4B` within 1.1 s, both windows followed, alive to the end; only the harmless first-chance burst above |

## 3-player runs (`-Players 3`, no late join)

MAP1, MAP4A, MAP8: all three windows welcomed (game3 after 1.9-3.9 s), `synced x1`, game3 received the same animal count as game2 (8 / 13 / 14), all alive to the end, no first-chance site, no flood (game3 11-18 lines/s). MAP4A shows the lioness lines twice (12 `-> -1`) and the idle-host mission failure at +64 s.

## Screenshots worth knowing about

- `smoke-MAP1-base-a3322d1-*`: the MAP1 tutorial modal ("FOLLOW HUGH TO THE TARGET RANGE ... RETURN continue") covers the host window for the whole run.
- `smoke-MAP2B-base-a3322d1-20261018-095905-game2.png`: a Windows Defender Firewall prompt ("Allow public and private networks ... Main7.exe") sits over the client window. It is from before `eccc1ef` (new game copies binding all interfaces); nothing in the pass clicked it. The `13b5efa` runs bind 127.0.0.1 (`coop: sockets bind to 127.0.0.1 only`).

## Proposed MEMORY facts

- Idle two-window smoke (no input) kills the unattended host on MAP2B (~85 s), MAP4A (~65 s), MAP7 (~70 s), MAP12B (~23 s) and MAP12C (~64 s): `events: host mission message code=6` then the client applies it. Not a sync fault.
- `coop_level restart|next` arms and confirms in one console write; a second write inside 3 s jumps another level.
- `dh2coop.asi+0x1062c7` and `+0x121777` first-chance AVs are the guarded `memcpy` in `safe_copy` (stale puppet entity at shutdown; `cGuiManager` read during a level switch). Symbolize with `.\tools-src\symbolize.ps1 -Module <asi> -Rva <rva>`, one RVA per process (a second call in the same PowerShell session fails with `SymLoadModuleEx failed 0`; `$PSScriptRoot` is empty in the parameter default when it is started with `-File` from another process, so pass `-Module`).
- Both `Could not forward animation idleattack1` classes (MAP4A lioness, MAP12C yeti) survive a 120 s idle run with the 3-tries limit: 18 engine errors per client, 6 `SetBodyState -> -1` lines.
- `scale: AddAnimal(... 'crocodileN'|'cobraN') entity not found` on MAP3A/3B/3C/4B/7/8: pack scaling skips those species.
- A smoke row costs ~2.3 min (2 windows, 120 s); `tools-src\smoke-maps.ps1 -Maps ... -Phase base|restart|next|players3` writes `re\notes\coop-local\smoke\rows.md`.

## Open

- A uniform build for MAP1-MAP11A (rows run on `a3322d1` / `3e33695`).
- Trigger-walk and exit-zone phases per map (the script has them, untested in this pass); input-driven checks (tutorial modals, picking up the hunters' weapons) need a `coop_*` command or a real session.
