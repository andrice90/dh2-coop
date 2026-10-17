# Job: coop4-launcher — 2 to 4 local instances, plus drop-in of one more client (YOU OWN GAME LAUNCHES in the main checkout; no Ghidra)

## Goal
The user wants to test 4 players and mid-game join on one PC. Today `run-coop-local.ps1` starts exactly two instances (`game\` host, `game2\` client).

## Deliverables (edit only these)
- `run-coop-local.ps1`
- `.gitignore` (add `game3/` and `game4/`)
- `re/notes/coop4-launch.md` (test record)

Do not touch `mod/src`, `STATUS.md`, `MEMORY.md`, `PLAN.md`, `CLAUDE.md`. The lead is changing `mod/src` at the same time. Do not run `mod\build.ps1`; use the `mod\build\dh2coop.asi` that is there.

## Changes to `run-coop-local.ps1`
1. **`-Players <2..4>`** (default 2). Trees: `game` (host), `game2`, `game3`, `game4`. Mirror every client tree from `game` with the same robocopy line used for `game2` today. Every client joins `127.0.0.1`.
2. **Per-instance name.** After copying `mod\dh2coop.ini` into each `Bin`, set `PlayerName=` to `Host`, `P2`, `P3`, `P4`. Only that line changes. Write the file as ASCII (the game reads it with `GetPrivateProfileString`).
3. **Arrange.** 2 players: side by side, as today. 3 or 4 players: a 2x2 grid with the host top-left, `game2` top-right, `game3` bottom-left, `game4` bottom-right. Keep `SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE`. If the grid does not fit the screen, let the rows overlap vertically rather than going off-screen. Generalise `Test-Placed`, drift re-placement, `Assert-InstancesAlive`, `Stop-LaunchedInstances` and `-Capture` to N instances. Screenshot names: `<stamp>-game.png`, `<stamp>-game2.png`, ...
4. **Start order.** The host first. Wait for `coop: host start` as today. Then start each client in turn and wait for its own `dh2coop.log` to contain `coop: welcomed` (timeout 60 s) before starting the next one. Log the time between each start and its welcome.
5. **`-LateJoin <seconds>`** (default 0). With 3+ players (or 2), the *last* client starts this many seconds after the previous one was welcomed, not straight away. This is the drop-in test.
6. **`-AddClient <2..4>`**: a separate mode for a session that is already running. It does not stop, mirror over or rearrange running instances.
   - Find running `Main7.exe` processes and the tree each one belongs to (process `Path`).
   - Refuse if no instance from `game\Bin` is running (no host), or if the target tree already runs.
   - Refresh only the target tree (robocopy from `game`), copy the asi/ini/cfg into it, set its `PlayerName`, start it with `-coopjoin 127.0.0.1` and the same `-spmap` as `-Map` (default `HUNTMAP4`; the client follows the host's map anyway).
   - Wait for `coop: welcomed` in its log and print the line.
7. Keep every existing behaviour for the default call (`.\run-coop-local.ps1` and `-Map X`). Update the header comment with the new switches and one example per switch.

## Test (record everything in `re/notes/coop4-launch.md`)
Before each launch check `Get-Process Main7`. If any runs, wait (poll every 30 s, up to 20 min) — another agent may be testing. Never close a Main7 you did not start.
1. `.\run-coop-local.ps1 -Players 4 -Seconds 60 -Capture -Map HUNTMAP4`:
   - Host log: three `coop: player N '...' joined` lines.
   - Each client log: `coop: welcomed as player N`, and `coop: remote <id>` lines for each of the other three players within the run.
   - Any `exception` / `Access violation` lines in the four `dh2coop.log` and the four `debug.log`.
   - The fps value the overlay shows in each screenshot, if readable.
2. `.\run-coop-local.ps1 -Players 2 -Map HUNTMAP4` (no `-Seconds`), wait 20 s, then `.\run-coop-local.ps1 -AddClient 3 -Map HUNTMAP4`, wait 30 s, read all three logs, then close the three games you started (`CloseMainWindow`, then `Stop-Process` after 5 s).
3. `.\run-coop-local.ps1 -Players 3 -LateJoin 20 -Seconds 60 -Map MAP3B`: the third client joins 20 s late into a story map. Quote every `events:` and `story:` line from that client's log and the host's `joined` line.

The note: the commands, a pass/fail table per check, the decisive log lines (short quotes), screenshot names, and anything odd. Mark guesses UNVERIFIED.

## Commit
When the tests pass, commit only your files:
`git commit -m "<one line>" -- run-coop-local.ps1 .gitignore re/notes/coop4-launch.md` (after `git add` of those three). Author `andri <andrice90@gmail.com>`, trailer `Co-Authored-By: Grok 4.7 <noreply@x.ai>`. Do not stage anything else.
