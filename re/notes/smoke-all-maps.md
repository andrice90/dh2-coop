# Smoke baseline: every story map, current main build (2026-10-18)

Job smoke-all-maps (wave playthrough). Worktree `D:\scary hunt\dh2coop-smoke`, branch `smoke-pass`, moved to `main` (`a3322d1`, "MAP4A truck sync and scaling merge"). Port 27690. Build: `.\mod\build.ps1` once (Release, `dh2coop.asi` build id `7fc4953c`, protocol 4).

## Method

- One two-window run per map: `.\run-coop-local.ps1 -Map <MAP> -Port 27690 -Seconds 120 -Capture` (host `game`, client `game2`, no keyboard or mouse input), driven by `tools-src\smoke-maps.ps1 -Phase base`. The countdown starts when the client is welcomed.
- A watchdog in the script polls both processes every 3 s (still running, window responding, no `WerFault` for that pid): `alive.txt` of each run. The run script itself aborts with `exited before the run finished (code N)` when a process dies.
- Logs of both trees are copied after each run (`re\notes\coop-local\smoke\<MAP>\<phase>\`: `game-dh2coop.log`, `game2-dh2coop.log`, `*-debug.log`, `run-output.txt`, `alive.txt`, `status.txt`, `row.md`). Rows come from `tools-src\smoke-row.sh`; `tools-src\smoke-summary.sh <dir>` is the long digest.
- Screenshots: `re\notes\coop-local\smoke-<MAP>-<phase>-<stamp>-game|game2.png` (4 of the 12 the run script takes are kept, spread over the run).
- Columns: **welcome** = seconds from client process start to `coop: welcomed`; **sync** = `events: synced with host` relative to that line. **animals H/C** = host `animals: host snapshot N` / client `animals: client received N`, in brackets the client's distinct `mirror freeze ... ok=true` ids (animals + NPCs; `bad` = `ok=false`). **trig** = host armed/fired (unique `cScrTaskNN` names: armed = `trigger event activate`, fired = `trigger FIRE`/`ran`), client armed/applied-fire/own-fire-sent, `defer` = `story: defer host fire` lines on the client. **first-chance** = `first-chance exception` lines that are not `nvd3dum.dll` (the logger stops after 16 per process). **lines/s** = `dh2coop.log` lines per second after the first 10 s of the process (avg, peak second); the steady diagnostic baseline is about 10-13 lines/s (`animals: sync`, `tracker: animal` every 1-2 s). **-1** = `SetBodyState ... -> -1` lines; the last column counts `refused`/`mismatch` lines of both logs.

## Limits of this smoke (read before trusting a green row)

- **No input is sent**, so the hunters stand where the map puts them. Maps that open with a modal message (MAP1: the tutorial text, "RETURN continue") sit on it for the 2 minutes. Trigger volumes are not walked; only what fires on its own in the first 2 minutes shows up. A clean row means "loads, syncs, idles without a flood or crash", not "the mission is playable".
- Nothing moves a mission forward, so late-map crash classes (vehicles, bosses, cutscenes, exit zones) are only reached when the map starts them by itself.
- Run-to-run noise: first-chance `AUDIOSES.DLL`/`nvd3dum.dll` access violations happen inside Windows audio / the NVIDIA driver and do not change anything the mod does.

## Baseline table (2 windows, 120 s each)

| map / run | alive H/C | welcome, sync | animals H/C (mirrored) | trig host arm/fire, client arm/fire/sent, defer | first-chance (not nvd3dum) | crash | fwd H/C | lines/s H / C avg(peak) | -1 | refused/mismatch | build | clients 3-4 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| MAP1/base-a3322d1 | ok/ok <ok> | 1.7s, sync +0.0s | 8/8 (7+1 npc, 0 cobra, 0 bad) | 2/1, 2/1/1, defer 0 | none | no | 0/0 | 9.7(18) / 9.7(19) | 0 | 0 | main a3322d1 (asi 7fc4953c) | - |
| MAP2A/base-a3322d1 | ok/ok <ok> | 1.5s, sync +0.1s | 11/11 (10+1 npc, 0 cobra, 0 bad) | 5/1, 5/1/0, defer 0 | none [at shutdown: game:0xc0000005@AUDIOSES.DLL+0x275ba x1; game2:0xc0000005@AUDIOSES.DLL+0x275ba x1] | no | 0/0 | 12.6(36) / 12.8(36) | 0 | 0 | main a3322d1 (asi 7fc4953c) | - |
| MAP2B/base-a3322d1 | ok/ok <ok> | 1.7s, sync +0.0s | 13/13 (12+1 npc, 0 cobra, 0 bad) | 2/1, 2/1/0, defer 0 | none [at shutdown: game:0xc0000005@dh2coop.asi+0x1062c7 x2; game2:0xc0000005@dh2coop.asi+0x1062c7 x2; game:0xc0000005@AUDIOSES.DLL+0x275ba x1; game2:0xc0000005@AUDIOSES.DLL+0x275ba x1] | no | 0/0 | 15.8(31) / 16.8(36) | 0 | 0 | main a3322d1 (asi 7fc4953c) | - |
| MAP2C/base-a3322d1 | ok/ok <ok> | 1.7s, sync +0.0s | 11/11 (8+3 npc, 0 cobra, 0 bad) | 2/1, 2/1/0, defer 0 | none [at shutdown: game:0xc0000005@AUDIOSES.DLL+0x275ba x1; game2:0xc0000005@AUDIOSES.DLL+0x275ba x1] | no | 0/0 | 12.5(26) / 12.3(24) | 0 | 0 | main a3322d1 (asi 7fc4953c) | - |
| MAP3A/base-a3322d1 | ok/ok <ok> | 1.7s, sync +0.0s | 13/13 (12+1 npc, 0 cobra, 0 bad) | 11/1, 11/1/0, defer 0 | none [at shutdown: game2:0xc0000005@dh2coop.asi+0x1062c7 x2; game:0xc0000005@AUDIOSES.DLL+0x275ba x1; game2:0xc0000005@AUDIOSES.DLL+0x275ba x1] | no | 0/0 | 14.8(38) / 14.8(38) | 0 | 0 | main a3322d1 (asi 7fc4953c) | - |
| MAP3B/base-a3322d1 | ok/ok <ok> | 2s, sync +0.1s | 8/8 (5+0 npc, 3 cobra, 0 bad) | 4/1, 4/1/0, defer 0 | none [at shutdown: game:0xc0000005@dh2coop.asi+0x1062c7 x2; game:0xc0000005@AUDIOSES.DLL+0x275ba x1; game2:0xc0000005@AUDIOSES.DLL+0x275ba x1] | no | 0/0 | 10.2(20) / 10.3(20) | 0 | 0 | main a3322d1 (asi 7fc4953c) | - |
| MAP3C/base-a3322d1 | ok/ok <ok> | 1.8s, sync +0.1s | 11/11 (6+1 npc, 4 cobra, 0 bad) | 2/1, 2/1/0, defer 0 | none [at shutdown: game:0xc0000005@AUDIOSES.DLL+0x275ba x1; game2:0xc0000005@AUDIOSES.DLL+0x275ba x1] | no | 0/0 | 12.7(25) / 12.8(25) | 0 | 0 | main a3322d1 (asi 7fc4953c) | - |
| MAP4A/base-a3322d1 | ok/ok <ok> | 2.3s, sync +0.0s | 13/13 (12+1 npc, 0 cobra, 0 bad) | 5/3, 5/3/0, defer 0 | none | no | 0/18 | 16.2(50) / 16.3(46) | 6 | 0 | main a3322d1 (asi 7fc4953c) | - |
| MAP4B/base-a3322d1 | ok/ok <ok> | 2.2s, sync +0.1s | 17/17 (16+1 npc, 0 cobra, 0 bad) | 17/1, 17/1/0, defer 0 | none | no | 0/0 | 18.1(37) / 18.4(37) | 0 | 0 | main a3322d1 (asi 7fc4953c) | - |
| MAP5/base-a3322d1 | ok/ok <ok> | 2.5s, sync +0.0s | 13/13 (11+2 npc, 0 cobra, 0 bad) | 18/1, 18/1/0, defer 1 | none | no | 0/0 | 14.2(51) / 14.6(51) | 0 | 0 | main a3322d1 (asi 7fc4953c) | - |
| MAP6/base-a3322d1 | ok/ok <ok> | 2s, sync +0.0s | 12/12 (9+3 npc, 0 cobra, 0 bad) | 5/1, 5/1/1, defer 0 | none [at shutdown: game:0xc0000005@dh2coop.asi+0x1062c7 x2; game2:0xc0000005@dh2coop.asi+0x1062c7 x2] | no | 0/0 | 12.4(25) / 12.5(24) | 0 | 0 | main a3322d1 (asi 7fc4953c) | - |
| MAP6/base | ok/ok <ok> | 2.2s, sync +0.0s | 12/12 (9+3 npc, 0 cobra, 0 bad) | 5/1, 5/1/1, defer 0 | none | no | 0/0 | 12.5(25) / 12.6(25) | 0 | 0 | main 3e33695 (asi 16064da7) | - |
| MAP7/base-a3322d1 | DEAD/DEAD <DIED game pid=1932 exited before the run finished (code )> | 1.5s, sync +0.0s | 12/12 (9+2 npc, 1 cobra, 0 bad) | 11/1, 11/1/1, defer 0 | none | no | 0/0 | 18.5(27) / 18.7(35) | 0 | 0 | main a3322d1 (asi 7fc4953c) | - |
| MAP7/base | ok/ok <ok> | 1.7s, sync +0.1s | 12/12 (9+2 npc, 1 cobra, 0 bad) | 11/1, 11/1/1, defer 0 | none | no | 0/0 | 13.9(29) / 14.2(31) | 0 | 0 | main 3e33695 (asi 16064da7) | - |
| MAP8/base | ok/ok <ok> | 1.9s, sync +0.1s | 14/14 (11+3 npc, 0 cobra, 0 bad) | 23/1, 23/1/0, defer 0 | none | no | 0/0 | 14.8(77) / 15.1(75) | 0 | 0 | main 3e33695 (asi 16064da7) | - |
| MAP9/base | ok/ok <ok> | 1.7s, sync +0.1s | 8/8 (7+1 npc, 0 cobra, 0 bad) | 13/1, 13/1/0, defer 0 | none | no | 0/0 | 9.7(21) / 9.8(20) | 0 | 0 | main 3e33695 (asi 16064da7) | - |
| MAP10/base | ok/ok <ok> | 1.9s, sync +0.0s | 19/19 (17+2 npc, 0 cobra, 0 bad) | 21/1, 21/1/1, defer 0 | none | no | 0/0 | 19.8(43) / 20.1(47) | 0 | 0 | main 3e33695 (asi 16064da7) | - |
| MAP11A/base | ok/ok <ok> | 1.9s, sync +0.1s | 13/13 (9+4 npc, 0 cobra, 0 bad) | 4/2, 4/2/0, defer 1 | none | no | 0/0 | 16.5(48) / 16.2(39) | 0 | 0 | main 3e33695 (asi 16064da7) | - |

## Level flow and 3-player runs

| map / run | alive H/C | welcome, sync | animals H/C (mirrored) | trig host arm/fire, client arm/fire/sent, defer | first-chance (not nvd3dum) | crash | fwd H/C | lines/s H / C avg(peak) | -1 | refused/mismatch | build | clients 3-4 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

## Findings

(in progress)
