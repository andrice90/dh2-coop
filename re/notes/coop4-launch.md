# coop4 launcher — 2 to 4 local instances and a drop-in client

## Progress
All three launches finished. Launcher checks passed except the 4-player remote-line check: game2 faulted and stopped sending snapshots.

## Goal
`run-coop-local.ps1` can start 2–4 local instances, delay the last client, or attach one more client to a host that is already running.

## Commands
1. `.\run-coop-local.ps1 -Players 4 -Seconds 60 -Capture -Map HUNTMAP4` — script printed `closed after 60s`. Pids: host 10380, game2 10412, game3 12568, game4 24252. No Main7 was running beforehand. Shell exit 1 is robocopy's "files copied" code; the script still completed.
2. Waited while `D:\scary hunt\dh2coop-markers\game\Bin\Main7.exe` pid 14376 was up (about 7 minutes, polled every 30s). That process was not closed. Then `.\run-coop-local.ps1 -Players 2 -Map HUNTMAP4` (no `-Seconds`). Host 15624, game2 4332, side by side `gap=10px overlap=no both_fully_inside=yes`. A wrapper treated robocopy exit 1 as failure after arrange; the two games were left running. After 20s, `.\run-coop-local.ps1 -AddClient 3 -Map HUNTMAP4` started pid 3548 only. No arrange line. After 30s, `CloseMainWindow` on 15624, 4332, and 3548. All three exited without `Stop-Process`. `remaining=0`.
3. `.\run-coop-local.ps1 -Players 3 -LateJoin 20 -Seconds 60 -Map MAP3B` — `closed after 60s`, `remaining=0`. Pids: host 14788, game2 6164, game3 18704.

## Pass/fail

| Check | Result |
|---|---|
| Test 1 host: three `coop: player N '...' joined` | PASS. P2, P3, P4. |
| Test 1 each client `coop: welcomed as player N` | PASS. game2 player 1, game3 player 2, game4 player 3, each `max 4`. |
| Test 1 each client `coop: remote` for the other three | FAIL. game3 has Host, P2, and P4. game2 has Host and P3 only. game4 has Host and P3 only. |
| Test 1 exception / Access violation lines | Present. See quotes. game and game4 `debug.log`: none. game2 and game3 `debug.log`: one access violation each. |
| Test 1 overlay fps | game 100, game3 100, game4 100 on `20261017-095552-*.png`. game2 is a loading screen; fps not readable. |
| Test 1 2x2 grid | PASS. Host (0,0), game2 (650,0), game3 (0,490), game4 (650,490). `row_overlap=no all_fully_inside=yes` on 3440x1440. |
| Test 2 drop-in of game3 | PASS. Host gained `player 2 'P3' joined` and did not log a new `host start`. game3 welcomed as player 2 after 1.5s. No exception or access-violation line in the three `dh2coop.log` or `debug.log` files. |
| Test 3 last client 20s late on MAP3B | PASS. Script logged `late join: waiting 20s after game2 welcomed`. game2 welcomed 10:06:43.215; game3 welcomed 10:07:04.653 (1.5s after its start). Host joined line is quoted below. Grid: game (0,0), game2 (650,0), game3 (0,490), `row_overlap=no all_fully_inside=yes`. |

## Log lines

Test 1 welcomes:

- `host ready pid=10380 engineTime=1.093 (listening) | [2026-10-17 09:55:15.536] [info] [t1532] coop: host start port=27420`
- `game2 welcomed pid=10412 after 1.9s | [2026-10-17 09:55:17.506] [info] [t22544] coop: welcomed as player 1 (max 4)`
- `game3 welcomed pid=12568 after 2.3s | [2026-10-17 09:55:19.878] [info] [t24240] coop: welcomed as player 2 (max 4)`
- `game4 welcomed pid=24252 after 2.7s | [2026-10-17 09:55:22.489] [info] [t24476] coop: welcomed as player 3 (max 4)`

Test 1 host joins (`game\Bin\dh2coop.log`):

- `[2026-10-17 09:55:17.497] [info] [t1532] coop: player 1 'P2' joined`
- `[2026-10-17 09:55:19.867] [info] [t1532] coop: player 2 'P3' joined`
- `[2026-10-17 09:55:22.483] [info] [t1532] coop: player 3 'P4' joined`

`PlayerName=` after that run: Host, P2, P3, P4. game2 also logged `name='P2'`.

Test 1 remote counts (`coop: remote <id> '<name>'`):

- host: `1 'P2'` x11, `2 'P3'` x63, `3 'P4'` x60
- game2: `0 'Host'` x5, `2 'P3'` x3. Last remote line 09:55:22.075. No id 3.
- game3: `0 'Host'` x63, `1 'P2'` x8, `3 'P4'` x60
- game4: `0 'Host'` x60, `2 'P3'` x60. No id 1. The welcome batch does include `coop: player 1 'P2' joined`.

Sample: `[2026-10-17 09:55:20.419] [info] [t24240] coop: remote 0 'Host' pos=(0.00,-1.69,0.00) yaw=120.226 seq=95 rx=11`

Test 1 exceptions:

- host `dh2coop.log` 09:56:22.700: `first-chance exception 0xc0000005 at dh2coop.asi+0xc5527 eip=0x68705527` (four copies). `debug.log`: none.
- game2 `dh2coop.log` 09:55:22.515: `first-chance exception 0xc0000005 at EngineDll7.dll+0x3882e7 eip=0x10282e7`. `debug.log`: `Access violation while 'writting to' address 0x10 at address 0x10282e7`. The log ends on that line.
- game3 `dh2coop.log` 09:56:22.693: same `EngineDll7.dll+0x3882e7` first-chance. `debug.log`: the same `writting to` line.
- game4 `dh2coop.log` 09:56:22.702: `first-chance exception 0xc0000005 at dh2coop.asi+0xc5527 eip=0x61485527` (two copies). `debug.log`: none.

Test 2:

- `[2026-10-17 10:04:43.365] [info] [t22332] coop: host start port=27420`
- `[2026-10-17 10:04:44.891] [info] [t22332] coop: player 1 'P2' joined`
- `[2026-10-17 10:05:35.498] [info] [t22332] coop: player 2 'P3' joined`
- game3 stdout: `game3 welcomed pid=3548 after 1.5s | [2026-10-17 10:05:35.499] [info] [t10936] coop: welcomed as player 2 (max 4)`
- The same welcome line was printed again on its own, as the AddClient result.

The 51s between P2 and P3 is the 20s pause plus the game3 robocopy, then the 1.5s welcome.

Test 3 host joined line for the late client:

- `[2026-10-17 10:07:04.647] [info] [t14120] coop: player 2 'P3' joined`
- Earlier: `[2026-10-17 10:06:43.208] [info] [t14120] coop: player 1 'P2' joined`

Every `events:` and `story:` line in `game3\Bin\dh2coop.log` (the late client):

- `[2026-10-17 10:07:03.420] [info] [t5572] events: registry AnimalDied=1 MissionEnd=2 TrophyClaim=3 MapStart=4`
- `[2026-10-17 10:07:04.291] [info] [t5572] story: skip path scr=0x10001aa0 record=0xd72fe0 mgr=0x1343b5c`
- `[2026-10-17 10:07:04.369] [info] [t12412] events: journal cleared for map 'MAP3B' (serial 1)`
- `[2026-10-17 10:07:04.394] [info] [t12412] story: [solo] trigger event activate id=39 name='cScrTask01' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7`
- `[2026-10-17 10:07:04.518] [info] [t12412] story: [solo] trigger FIRE id=39 name='cScrTask01' vt=GAMEDLL7.DLL+0x10afa0 activator=id=60 name='hunter' vt=GAMEDLL7.DLL+0x106ff0`
- `[2026-10-17 10:07:04.518] [info] [t12412] story: [solo] trigger event activate id=44 name='cScrTask02' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7`
- `[2026-10-17 10:07:04.518] [info] [t12412] story: [solo] trigger event activate id=46 name='cScrTask06' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7`
- `[2026-10-17 10:07:04.518] [info] [t12412] story: [solo] trigger event activate id=45 name='cScrTask07' vt=GAMEDLL7.DLL+0x10afa0 caller=GAMEDLL7.DLL+0x6aac7`
- `[2026-10-17 10:07:04.653] [info] [t12412] events: request kind=9 subject=1 key='MAP3B' sent=true`
- `[2026-10-17 10:07:04.684] [info] [t12412] events: apply commit kind=4 subject=1 actor=0 i0=0 key='MAP3B'`
- `[2026-10-17 10:07:04.684] [info] [t12412] events: host map 'MAP3B' (serial 1) is already loaded here`
- `[2026-10-17 10:07:04.684] [info] [t12412] events: apply commit kind=10 subject=1 actor=0 i0=0 key='MAP3B'`
- `[2026-10-17 10:07:04.684] [info] [t12412] events: synced with host on 'MAP3B' (0 commits replayed)`
- `[2026-10-17 10:07:05.683] [info] [t12412] events: catch-up finished after 1000 ms; host 0.0 m away`

Test 3 shutdown: game3 `dh2coop.log` 10:08:04.758 `first-chance exception 0xc0000005 at dh2coop.asi+0xcc3e7 eip=0x61b4c3e7` (two copies). Host and game2 `dh2coop.log`: no exception line. All three `debug.log` files: no access-violation line.

## Screenshots
`re\notes\coop-local\`, stamps `20261017-095532`, `095542`, `095552`, `095602`, `095612`, each with `-game.png`, `-game2.png`, `-game3.png`, `-game4.png`.

Read `20261017-095552-game.png`, `-game3.png`, `-game4.png`: overlay `fps 100`. Host lists P3 and P4. game3 lists Host and P4. game4 lists Host and P3.

`20261017-095552-game2.png`: loading screen, no overlay.

## Oddities
game2's test-1 access violation is 30 ms after `player 3 'P4' joined` (09:55:22.485, then 09:55:22.515). Later game2 captures stay on the loading screen. The process did not exit, so the 60s alive check did not fire. P2's snapshots stop, which is why game4 never logs `coop: remote 1`.

On MAP3B the late client's story lines are tagged `[solo]`, and the events log says `0 commits replayed`. That is what the log printed. Whether those triggers had already fired on the host is UNVERIFIED; this note did not diff the host story log.

`run-coop-local.ps1` leaves `LASTEXITCODE` at robocopy's 0–7. Exit 1 here meant files were copied.

## Proposed MEMORY facts
None. The lines above are from `mod\build\dh2coop.asi` (`dh2coop 0.2.0-framework` in the game2 load banner). The faulting engine instruction was not identified.
