# P3 — side-by-side two-instance windows and stall measurement

## Progress

Done. `run-coop-local.ps1` places the windows and can capture them. One 120 s run finished (exit 0). Tables and screenshot notes are below.

## Usage

```powershell
.\run-coop-local.ps1 [-Seconds N] [-NoArrange] [-Capture]
```

- No `-Seconds`: start both instances, wait until both main windows exist, place them, then return and leave the games running.
- `-Seconds N`: same launch as before, then close both after N seconds (graceful, then force). The N seconds start when the second process is started.
- `-NoArrange`: do not move the windows.
- `-Capture`: every 10 s during the `-Seconds` wait, save both windows under `re\notes\coop-local\<yyyyMMdd-HHmmss>-game.png` and `-game2.png`. With `-Seconds 0` the script does not stay up, so it captures nothing and prints a warning.

`-Capture` runs `tools-src\capture-window.ps1` in a hidden child PowerShell with an absolute `-Out`. That script does `Join-Path (Get-Location) $Out` before `GetFullPath`. Windows PowerShell 5.1 concatenates a rooted child path, so a direct absolute `-Out` throws. The child defines a `Join-Path` that returns a rooted path unchanged, then runs the helper. The helper also calls `SetForegroundWindow` and sleeps 400 ms. Captures run `game` first, then `game2`, so a successful focus change ends on `game2`.

Placement uses `SetWindowPos` with `SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE`. `game` goes to `(0, 0)`. `game2` goes to `(game.Right + 10, 0)` from `GetWindowRect`. The script waits until both outer rects are at least 640×480, or until 90 s, then places the windows it has. The process is marked DPI-aware first so the coordinates are physical pixels. Rects are printed. If a window later leaves that placement by more than 40 px, it is placed again.

`re/notes/coop-local/` is in `.gitignore`.

## What changed

- `run-coop-local.ps1`: `-NoArrange`, `-Capture`, window poll, `SetWindowPos` placement, foreground samples in `re\notes\coop-local\run-log.txt` when arranging or capturing.
- `.gitignore`: `re/notes/coop-local/`.

Launch arguments are unchanged: `+mod data -nointro -developper -cheats -spmap HUNTMAP4`, then `-coophost` and, 3 s later, `-coopjoin 127.0.0.1`.

## Run

`.\run-coop-local.ps1 -Seconds 120 -Capture` once, 2026-10-16 11:58:59–12:01. Exit 0. `closed after 120s (graceful, then force)`. No manual clicks. `Main7` was not left running.

| Role | Pid | Args | Log |
|---|---|---|---|
| game (host) | 19356 | `-coophost` | `game\Bin\dh2coop.log` |
| game2 (client) | 10644 | `-coopjoin 127.0.0.1` | `game2\Bin\dh2coop.log` |

Both set `cApplication::m_bAllowBackground 0 -> 1 at engine ready`. Map `Bsp\HUNTMAP4.bsp` loaded on both. Host: `coop: hosting on UDP 27420 -> true`, `coop: player 1 'Hunter' joined`, `puppets: spawned puppet (remote 1)`. Client: `coop: joining 127.0.0.1:27420 -> true`, `coop: welcomed as player 1`, `puppets: spawned puppet (remote 0)`.

## Window rects

After `SetProcessDPIAware`, the primary screen was 1920×1200 (virtual origin 0,0). A DPI-unaware query of the same display earlier in the session reported 1536×960, which is 1920×1200 at 125%.

Printed after placement (`run-log.txt`):

```
place game=(0,0) game2=(650,0) flags=SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE
game hwnd=133064 rect=(0,0)-(640,480) size=640x480
game2 hwnd=329662 rect=(650,0)-(1450,600) size=800x600
gap=10px overlap=no both_fully_inside=yes
```

The outer rects do not intersect. Both sit inside the 1920×1200 screen. No later "windows drifted" line; the rects stayed put for the run. The two outer sizes were already different when measured. `SWP_NOSIZE` does not resize. `game` is 640×480. `game2` is 800×600, which is 640×480 × 1.25. Cause of the difference: UNVERIFIED (same `console.cfg` `r_width`/`r_height`/`r_windowed` was copied into both trees; the exes live in different directories). The DPI-unaware capture helper reported the same windows as 512×384 and 640×480.

`game2`'s PNGs include a narrow strip of `game`'s menu along the left edge. That strip is inside `game2`'s captured window rect. It matches a non-client border that reaches across the 10 px gap. The two client views are otherwise separate.

## Foreground

`GetForegroundWindow` once a second, plus a sample around every capture (`re\notes\coop-local\run-log.txt`):

- t=0 s, before `game2`'s window existed: `game` (pid 19356).
- After both windows existed, and before every capture: `game2` (pid 10644). 105 of 106 one-second samples are `game2`. The other sample is t=0.
- Each capture called `SetForegroundWindow`. After the `game` capture, foreground was `game`. After the `game2` capture, foreground was `game2` again. That happened for all 11 capture rounds (11:59:12 through 12:00:52).

`game\Bin\debug.log` and `game2\Bin\debug.log` each contain `Application loses focus` 12 times and `WM_ACTIVATEAPP: 1` 12 times. Those lines have no timestamps. The count matches the capture flips plus startup.

Between captures, the host (`game`) was the background window for about 8–9 s at a time. `game2` was foreground for the rest of the run. Neither window was clicked.

## Measurement tables

Heartbeat is one line per 10 s of accumulated `Present` dt (`heartbeat frames=… avg_dt=…`). Wall gap is the clock time since the previous line. A stall shows up as a wall gap well above 10 s, or as `avg_dt` rising (the earlier report was ~0.113 s).

### game (host, pid 19356) heartbeat

| Time | Wall gap | frames | window_frames | avg_dt |
|---|---:|---:|---:|---:|
| 11:59:10.213 |  | 968 | 968 | 0.01034 s |
| 11:59:20.220 | 10.01 s | 1966 | 998 | 0.01003 s |
| 11:59:30.228 | 10.01 s | 2965 | 999 | 0.01002 s |
| 11:59:40.233 | 10.00 s | 3963 | 998 | 0.01003 s |
| 11:59:50.234 | 10.00 s | 4961 | 998 | 0.01002 s |
| 12:00:00.237 | 10.00 s | 5959 | 998 | 0.01002 s |
| 12:00:10.243 | 10.01 s | 6958 | 999 | 0.01002 s |
| 12:00:20.249 | 10.01 s | 7955 | 997 | 0.01004 s |
| 12:00:30.251 | 10.00 s | 8953 | 998 | 0.01002 s |
| 12:00:40.254 | 10.00 s | 9951 | 998 | 0.01002 s |
| 12:00:50.254 | 10.00 s | 10949 | 998 | 0.01002 s |
| 12:01:00.256 | 10.00 s | 11948 | 999 | 0.01001 s |

### game2 (client, pid 10644) heartbeat

| Time | Wall gap | frames | window_frames | avg_dt |
|---|---:|---:|---:|---:|
| 11:59:13.166 |  | 967 | 967 | 0.01034 s |
| 11:59:23.173 | 10.01 s | 1966 | 999 | 0.01002 s |
| 11:59:33.174 | 10.00 s | 2964 | 998 | 0.01002 s |
| 11:59:43.180 | 10.01 s | 3963 | 999 | 0.01002 s |
| 11:59:53.186 | 10.01 s | 4962 | 999 | 0.01002 s |
| 12:00:03.196 | 10.01 s | 5962 | 1000 | 0.01001 s |
| 12:00:13.203 | 10.01 s | 6961 | 999 | 0.01002 s |
| 12:00:23.206 | 10.00 s | 7960 | 999 | 0.01001 s |
| 12:00:33.211 | 10.01 s | 8959 | 999 | 0.01002 s |
| 12:00:43.220 | 10.01 s | 9958 | 999 | 0.01002 s |
| 12:00:53.222 | 10.00 s | 10957 | 999 | 0.01001 s |

The client has one fewer line because it started about 3 s later (`cmdline` 11:59:02.536 vs host 11:58:59.515). Its next 10 s window would have closed around 12:01:03, after the 120 s timer (client start + 120 s ≈ 12:01:02) began closing the processes. That missing line is the shutdown cutoff, not a gap in the series above.

### coop: remote rx per 10 s

`LogRemotes` writes about once per second of frame dt. Each row is the last line in that wall-clock bucket. `+rx` is the increase since the previous row's `rx` (about 10 s). The first row's `+rx` is the increase from that side's first `coop: remote` line to the end of the bucket. Position on every line was `(0.00,-1.69,0.00)`, yaw `120.000`.

Host receives remote 1. First line 11:59:04.212 `seq=14 rx=14`. Last line 12:01:01.499 `seq=2286 rx=2246`.

| Bucket (last sample) | rx | +rx | seq | lines in bucket |
|---|---:|---:|---:|---:|
| 11:59:13 | 150 | 136 | 189 | 10 |
| 11:59:23 | 345 | 195 | 384 | 10 |
| 11:59:33 | 539 | 194 | 578 | 10 |
| 11:59:43 | 734 | 195 | 773 | 10 |
| 11:59:53 | 926 | 192 | 966 | 10 |
| 12:00:03 | 1120 | 194 | 1160 | 10 |
| 12:00:13 | 1313 | 193 | 1353 | 10 |
| 12:00:23 | 1506 | 193 | 1546 | 10 |
| 12:00:33 | 1700 | 194 | 1740 | 10 |
| 12:00:43 | 1894 | 194 | 1934 | 10 |
| 12:00:53 | 2089 | 195 | 2129 | 10 |
| 12:01:01 | 2246 | 157 | 2286 | 8 |

Client receives remote 0. First line 11:59:04.170 `seq=71 rx=12`. Last line 12:01:01.486 `seq=2359 rx=2300`.

| Bucket (last sample) | rx | +rx | seq | lines in bucket |
|---|---:|---:|---:|---:|
| 11:59:13 | 189 | 177 | 248 | 10 |
| 11:59:23 | 387 | 198 | 446 | 10 |
| 11:59:33 | 583 | 196 | 642 | 10 |
| 11:59:43 | 779 | 196 | 838 | 10 |
| 11:59:53 | 973 | 194 | 1032 | 10 |
| 12:00:03 | 1167 | 194 | 1226 | 10 |
| 12:00:13 | 1361 | 194 | 1420 | 10 |
| 12:00:23 | 1557 | 196 | 1616 | 10 |
| 12:00:33 | 1753 | 196 | 1812 | 10 |
| 12:00:43 | 1949 | 196 | 2008 | 10 |
| 12:00:53 | 2144 | 195 | 2203 | 10 |
| 12:01:01 | 2300 | 156 | 2359 | 8 |

After the opening bucket, each ~10 s step gains about 192–198 receives (~19.5/s). The host's opening step is smaller: rx 14→150 (+136) by 11:59:13. The 12:01:01 row is short because the processes were closed then.

### Did the background instance throttle?

No. The background instance was the host. For the whole run its heartbeat wall gap stayed 10.00–10.01 s and `avg_dt` stayed 0.01001–0.01034 s (~100 fps, ~998 frames per window). `rx` on both sides kept climbing through 12:01:01. The client, which held the foreground, matched that rate (`avg_dt` 0.01001–0.01034 s, wall gap 10.00–10.01 s). Neither log stops early. Neither shows the ~0.113 s `avg_dt` from the earlier overlapped run.

Limit of this sample: `capture-window.ps1` foregrounded the host for part of every 10 s. The longest stretch the host stayed in the background was about 8–9 s. This run does not show what happens when a window stays unfocused, or covered, for longer than that.

## Screenshot observations

Eleven pairs in `re/notes/coop-local/`, from `20261016-115912` through `20261016-120052`. Each file is one window (`capture-window.ps1`), not a desktop shot.

Checked `115912`, `120002`, and `120052` for both instances:

- Both windows are on screen and showing the game. The host capture is the full host view. The client capture is the full client view plus a narrow left strip of the host (see Window rects).
- Both still show the CHOOSE ITEM menu at 12:00:52: handgun, double barrel shotgun, replica long range rifle, the yellow `navigate` prompt, and the hunt-end hint. The menu is up for the whole checked series.
- The F8 overlay is up on both. Host: `role host`, `local id 0`, `fps 100`, remote `Hunter` pkt rising (about 151 at 11:59:12, 2087 at 12:00:52). Client: `role client`, `local id 1`, `fps 100`, remote pkt rising (about 206, then 2157). Both hunters read pos `0.0 -1.7 0.0`, yaw `120.0`. `animals 5`.
- No separate mountain-guide body is visible. The logs did spawn one puppet on each side (`remote 1` on the host, `remote 0` on the client) at 11:59:03. Both reported positions stay on the same spawn point, and the weapon menu covers the center of the view, so a body at the hunter is not distinguishable in these shots.

## Proposed MEMORY facts

- Side-by-side run 2026-10-16 11:59–12:01 (`run-coop-local.ps1 -Seconds 120 -Capture`, HUNTMAP4): both instances kept `Present` at about 100 fps for the full run. Host `game\Bin\dh2coop.log` heartbeat `avg_dt` 0.01034 s then 0.01001–0.01004 s, wall gap 10.00–10.01 s, last line `[2026-10-16 12:01:00.256] heartbeat frames=11948 window_frames=999 avg_dt=0.01001s`. Client `game2\Bin\dh2coop.log` the same shape, last line `[2026-10-16 12:00:53.222] heartbeat frames=10957 window_frames=999 avg_dt=0.01001s`. The host was the background window except during captures.
- Same run, after the host's opening step (rx 14→150, +136, by 11:59:13), `coop: remote` rx increased by about 192–198 per 10 s on both sides until shutdown. Host last line `[2026-10-16 12:01:01.499] coop: remote 1 'Hunter' pos=(0.00,-1.69,0.00) yaw=120.000 seq=2286 rx=2246`. Client last line `[2026-10-16 12:01:01.486] coop: remote 0 'Hunter' pos=(0.00,-1.69,0.00) yaw=120.000 seq=2359 rx=2300`. Position never left the spawn.
- `tools-src\capture-window.ps1` calls `SetForegroundWindow`. In this run that succeeded: after each host capture, `GetForegroundWindow` was the host; after each client capture, it was the client (`re\notes\coop-local\run-log.txt`). Each `debug.log` has `Application loses focus` 12 times. The host was therefore not left unfocused for longer than the ~8–9 s between captures. Whether a longer unfocused or fully covered window still stalls is UNVERIFIED. This run did not reproduce the earlier ~9 fps stall.

## Open questions

- Does the unfocused instance still stall if nothing calls `SetForegroundWindow` for more than ~10 s, or only when its window is covered?
- Why was the host outer size 640×480 and the client outer size 800×600 on a 125% display, from the same windowed `console.cfg`?
- The spawned mountain-guide puppet was not visually separable while both hunters stood on one spawn under CHOOSE ITEM. A movement check still needs the menu closed and one hunter walked.
