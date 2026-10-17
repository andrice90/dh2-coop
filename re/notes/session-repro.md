# Session repro — two local windows

**Progress:** All five tasks recorded. Games from this job closed at the end.

Working directory: `D:\scary hunt\dh2coop`. Game launches owned by this job. No `mod/src` edits. Original install is read-only.

Reference (user's two-PC host log, not modified): `D:\scary hunt\Activision Value\Magic Wand\DNGH2\Bin\dh2coop.log`. That log's `cScrTask04` fires are on a session whose animal id 157 starts at the hidden position `(-10000,-10000,-10000)` (lines around 11:39 and 11:53). Local runs below are separate processes under `game\` and `game2\`.

## Task 1 — Mountain lion on MAP2A

### Run 1

Commands (pids host 14796 `game\Bin\Main7.exe`, client 23996 `game2\Bin\Main7.exe`, launched 15:40:58):

```
.\run-coop-local.ps1 -Map MAP2A
[IO.File]::WriteAllLines('game\Bin\dh2coop.cmd', @('coop_trigger cScrTask04'))   # arm only
[IO.File]::WriteAllLines('game\Bin\dh2coop.cmd', @('coop_trigger cScrTask04'))   # fire
[IO.File]::WriteAllLines('game2\Bin\dh2coop.cmd', @('coop_tp -46:41.7:30'))
[IO.File]::WriteAllLines('game\Bin\dh2coop.cmd', @('coop_tp -139:36.6:109.7'))
```

The launcher returned exit code 1 after arranging the windows; both processes were still running. Watched 90 s after the teleports (15:41:53 through ~15:43:36). Interval screenshots during that window failed: `capture-window.ps1` rejects an already-absolute `-Out` (`The given path's format is not supported`). Start and end shots used relative paths and saved: `re/notes/coop-local/repro-t1r1-host-0.png`, `repro-t1r1-client-0.png` (forest, Hugh talking), `repro-t1r1-host-end.png`, `repro-t1r1-client-end.png` (both on MISSION FAILED).

What happened: first `coop_trigger` only armed. Second fired. About 12 s later the lion (id 157) switched to the client and walked up to the client's teleport. It then left. By the end shot both windows were on the results screen (host timer 0:02:04, hits taken 0, companion hits 45; client timer 0:02:02, hits taken 1, companion hits 21). Neither process exited. No `kMissionEnd` / `MISSION` line in either `dh2coop.log`. Client `debug.log` tail is ordinary `[SYSTEM] fn=... fps=100` lines, no exception.

Host quotes (`game\Bin\dh2coop.log`):

```
[2026-10-17 15:41:32.644] story: coop_trigger 'cScrTask04' -> id 120 vt=GAMEDLL7.DLL+0x10afa0 armed=false fired=false -> arm
[2026-10-17 15:41:41.685] story: [host] trigger FIRE id=120 name='cScrTask04' vt=GAMEDLL7.DLL+0x10afa0 activator=id=172 name='hunter'
[2026-10-17 15:41:41.691] story: [host] trigger event activate id=153 name='cScrTask05' ...
[2026-10-17 15:41:41.691] story: [host] trigger event activate id=152 name='cScrTask14' ...
[2026-10-17 15:41:41.693] animals: id=157 state='IdleSurprise' pos=(-36.9,42.5,14.5) hp=80
[2026-10-17 15:41:53.257] coop_tp: hunter to (-139.0,36.6,109.7)
[2026-10-17 15:41:53.909] combat: animal id 157 switches to the nearer player 1 (18.2 m, was 139.7 m) ok=true
[2026-10-17 15:41:55.997] animals: id=157 state='Stalk' pos=(-42.5,39.6,24.2) hp=80
[2026-10-17 15:42:01.789] animals: id=157 state='IdleAttack' pos=(-46.2,39.6,30.3) hp=80
[2026-10-17 15:42:12.832] animals: id=157 state='LookL45' pos=(-42.0,39.1,36.0) hp=80
```

After 15:42:12 the host's repeating `animals: sync role=host id=157` lines (71 of them) use `pos=(-10000.00,-10000.00,-10000.00) state='LookL45'` and later the client mirror says `state='KeepAnim'` at that same hidden position. The only host `combat:` line in the session, besides the startup `knock on shot=true`, is the switch above. Puppet lines around the contact are walk/strafe clips on entity `0x10e4ad38`, not a knock.

Client quotes (`game2\Bin\dh2coop.log`):

```
[2026-10-17 15:41:00.250] animals: mirror freeze id=157 ok=true npc=false state='IdleAttack'
[2026-10-17 15:41:41.760] animals: id=157 state='IdleSurprise' pos=(-8369.7,-8356.7,-8361.3) hp=80
[2026-10-17 15:41:42.340] animals: sync role=client id=157 pos=(-36.97,42.47,14.44) state='IdleSurprise' hp=80.0 yaw=44.9 target=(-36.97,42.47,14.44) drift=0.000
[2026-10-17 15:41:53.362] coop_tp: hunter to (-46.0,41.7,30.0)
[2026-10-17 15:42:01.894] animals: id=157 state='IdleAttack' pos=(-46.2,39.6,30.2) hp=80
[2026-10-17 15:42:02.375] animals: sync role=client id=157 pos=(-46.19,39.56,30.28) state='Run' hp=80.0 yaw=346.9 ... drift=0.000
```

Client last 80 lines (15:43:42.509–15:43:54.546) are the same cycle repeated: `coop: remote 0 'Host' pos=(-139.00,36.19,109.70)`, seven `animals: sync role=client` rows with ids 154–159 and 157 at `(-10000,...)` (`id=157 state='KeepAnim'`), `id=171` at `(-87.34,20.72,17.13)`, `id=75` Hugh `hp=0.0` at `(-91.90,-7.50,14.16)`, `tracker: hunter pos=(-44.88,39.44,31.42)`, and `puppets: physics drift ... dy=0.90`. No fault text. Client `debug.log` tail (same moment):

```
[SYSTEM  ] fn=17315, fps=99; tris= 9745200, rend(0.002) game(0.000) wait(0.030) sound(0.008) input(0.000) time=174.308685
[SYSTEM  ] fn=17415, fps=99; tris= 9745200, rend(0.004) game(0.000) wait(0.022) sound(0.005) input(0.000) time=175.313171
```

Client process had not exited at 15:43:54 (`HasExited=False`).

### Hypothesis

UNVERIFIED. Firing `cScrTask04` places lion 157 and, once the client is closer than the host, combat retargets it onto player 1. The mirror follows that path (drift 0) until the host sample collapses to the hidden `(-10000)` position while the state name sticks. The results screen can come up on both windows with no `events:` mission-end line and without the client process exiting. The one-frame client position `(-8369,-8356,-8361)` is a bad sample, not a second animal. This run did not reproduce the hard crash.

### Run 2

Commands (host pid 25472, client pid 21628, host start 15:45:21):

```
.\run-coop-local.ps1 -Map MAP2A
coop_trigger cScrTask04   # arm, then fire
coop_tp -46:41.7:30       # client
coop_tp -139:36.6:109.7   # host
```

Launcher exit code 1 again after a successful arrange. Watched ~90 s. Screenshots every ~5 s: `re/notes/coop-local/repro-t1r2-host-1.png` … `repro-t1r2-host-18.png` and the matching `repro-t1r2-client-*.png`. Both processes stayed up (`HasExited=False` at the end). Client `debug.log` tail is `[SYSTEM] fn=13568` … `fn=14272` fps lines, no exception.

The client did not stay at the lion. `coop_tp` landed, the lion switched onto player 1, then the client's opening cutscene ended and the gather fix moved the client to the host. From shot 2 onward both overlays show the hunter at about (-139, 36.2, 109.7) and `Host 0m` on the client. Shots show the bridge and Hugh's face on the host; no lion in frame; no results screen this time.

Host quotes:

```
[2026-10-17 15:45:35.507] story: [host] trigger FIRE id=120 name='cScrTask04' ... activator=id=172 name='hunter'
[2026-10-17 15:45:37.525] coop_tp: hunter to (-139.0,36.6,109.7)
[2026-10-17 15:45:37.878] combat: animal id 157 switches to the nearer player 1 (18.1 m, was 139.7 m) ok=true
[2026-10-17 15:45:35.512] animals: id=157 state='IdleSurprise' pos=(-36.9,42.5,14.5) hp=80
[2026-10-17 15:45:40.108] animals: id=157 state='Stalk' pos=(-43.3,39.6,25.4) hp=80
[2026-10-17 15:45:45.981] animals: id=157 state='Run' pos=(-46.7,39.6,29.4) hp=80
[2026-10-17 15:45:47.413] animals: id=157 state='IdleAttack' pos=(-46.2,39.5,34.3) hp=80
```

Client quotes:

```
[2026-10-17 15:45:37.479] coop_tp: hunter to (-46.0,41.7,30.0)
[2026-10-17 15:45:35.617] animals: id=157 state='IdleSurprise' pos=(-9446.5,-9442.1,-9443.6) hp=80
[2026-10-17 15:45:45.367] story: defer host fire 'cScrTask01' (id 117) until the local cutscene camera ends
[2026-10-17 15:45:46.099] animals: id=157 state='Run' pos=(-46.7,39.6,29.4) hp=80
[2026-10-17 15:45:47.512] animals: id=157 state='IdleAttack' pos=(-1837.9,-1767.6,-1772.0) hp=80
[2026-10-17 15:45:48.863] story: cutscene over, host 119.5 m away: gathering
[2026-10-17 15:45:48.863] coop: teleport to host (after cutscene) from (-49.4,39.8,30.6) to (-139.0,36.4,109.7)
```

Last client sync sample (15:47:49): `animals: sync role=client id=157 pos=(-10000.00,-10000.00,-10000.00) state='IdleAttack'`. The last 80 client lines are that hidden-position sync cycle plus `coop: remote 0` at the host spot. No fault text. Only host `combat:` line besides startup is the switch above. No knock line.

### Hypothesis

UNVERIFIED. Same retarget as run 1. The hard crash did not recur in either run (client process alive, `debug.log` still printing frame stats). Run 2 shows the gather fix undoing a test teleport that happens during the client's own cutscene: once that camera ends, a client more than 20 m from the host is moved to the host, so the lion is left at the old spot and the client is no longer the body being attacked. The huge one-frame positions (`-8369` in run 1, `-9446` and `-1837` in run 2) are on the client state-change line only; the next sync sample is a normal coordinate. Run 1, where gather did not fire, ended on a MISSION FAILED screen in both windows with no `events:` mission-end line and with the lion's later snapshots stuck at `(-10000)`.

### Hypothesis

UNVERIFIED.

## Task 2 — Mission end and continue

### RETURN

```
.\tools-src\event-test.ps1 -Keep
.\tools-src\game-input.ps1 -ProcessId 20472 -Keys RETURN
```

Host pid 20472, client pid 7624, HUNTMAP4. `event-test` closed CHOOSE ITEM on both, host `coop_kill 120:1000`, client `coop_claim 120`. Both logs show the success screen before any key:

```
[2026-10-17 15:49:49.688] events: host mission message code=5 text='MISSION SUCCESSFUL!' caller=GAMEDLL7.DLL+0x94f13
[2026-10-17 15:49:49.688] events: commit kind=2 subject=0 actor=0 i0=5 key='MISSION SUCCESSFUL!' seq=4 sent=true journal=4
[2026-10-17 15:49:49.693] events: applied host mission end code=5 text='MISSION SUCCESSFUL!' ok=true
```

`game-input` reported `foreground ok=True` for the host. Client screen stayed on MISSION SUCCESSFUL for the whole minute (`re/notes/coop-local/repro-t2ret-client-1.png` through `repro-t2ret-client-6.png`, timer frozen at 0:00:12, prompt `RETURN continue` / `ESC replay mission`). Host left that screen for `PLEASE SELECT A SAVE LOCATION` (`repro-t2ret-host-1.png` through `repro-t2ret-host-6.png`, three EMPTY slots, `RETURN save`). No new `kind=4` / `following host` line in either log during those 60 s. The client did not follow.

A second RETURN on the host (after the 60 s, still foregrounded on pid 20472) left the save screen. `repro-t2ret-host-save.png` shows the host overlay `role offline`, `no remotes`, hunter `(-3.4,-2.9,-68.3)`, knife in hand, another hunter ahead. `repro-t2ret-client-save.png` is still MISSION SUCCESSFUL. Still no new map-start line in either `dh2coop.log`.

`event-154905-host-2claim3.png` (taken by `event-test` before the RETURN) is the same offline cave. `event-154905-client-2claim3.png` is already MISSION SUCCESSFUL. So the host window was not sitting on the results screen at the last claim capture, even though its log had just committed the mission message.

### ESCAPE

```
.\tools-src\event-test.ps1 -Keep
.\tools-src\game-input.ps1 -ProcessId 21320 -Keys ESCAPE
```

Host pid 21320, client pid 3720. Before the key, both windows were on MISSION SUCCESSFUL (`event-155229-host-2claim3.png`, `event-155229-client-2claim3.png`). `foreground ok=True` for the host. Over the next 60 s both windows showed the same dialog (`repro-t2esc-host-1.png` … `host-5.png` and the client pair): "Are you sure you want to restart this mission? Your current progress will be lost!" with NO highlighted. Neither process exited. No new `kind=4` and no `following host` line after the mission message. The client did not load another map.

The scan-code input was aimed at the host, and both windows reacted. That matches the job note that local keyboard input can reach both games. This run does not show an ESCAPE that only the host saw.

### Hypothesis

UNVERIFIED. Mission success is shared (both logs, both screens). The host's RETURN then walks a local save menu the client does not enter; a further RETURN left the host `role offline` in a different scene while the client stayed on the results screen, with no `kMapStart`. ESCAPE opens a local restart confirm. In this setup that key was visible to both processes, so it does not prove the client follows a host-only cancel. Nothing in these two runs forwarded the continue or the replay into `events: following host to`.

### Hypothesis

UNVERIFIED.

## Task 3 — Host pause

```
.\run-coop-local.ps1 -Map MAP3B
# welcome 15:55:26; about 24 s later:
.\tools-src\game-input.ps1 -ProcessId 8236 -Keys ESCAPE
# shots 2 s later, then ESCAPE again
```

Host pid 8236, client pid 23036. `foreground ok=True`.

Two seconds after the first ESCAPE both windows showed GAME PAUSED with RESUME / RESTART / MISSIONS / OPTIONS / MAIN MENU / QUIT (`repro-t3-host-pause.png`, `repro-t3-client-pause.png`). Two seconds after the second ESCAPE both were back in the cave with the knife (`repro-t3-host-resume.png`, `repro-t3-client-resume.png`).

The client log does not mention pause. Through the pause it keeps printing positions:

```
[2026-10-17 15:55:48.325] coop: remote 0 'Host' pos=(-3.42,-2.91,-68.28) yaw=180.228 seq=456 rx=426
[2026-10-17 15:55:50.335] coop: remote 0 'Host' pos=(-3.42,-2.91,-68.28) yaw=180.228 seq=495 rx=465
```

Those seq numbers kept climbing while the menu was up. Same pattern on the host for remote 1. No pause, menu, or freeze line on either side.

### Hypothesis

UNVERIFIED. The pause menu appeared on the client because the ESCAPE scan code reached both games, the same way the restart confirm did in task 2. The client log has no pause event, and position packets did not stop, so this run does not show the mod pausing the client when only the host pauses.

### Hypothesis

UNVERIFIED.

## Task 4 — Late joiner damage

```
.\run-coop-local.ps1 -Map MAP2A -Players 2
# host pid 11696, client pid 23156, welcome 15:56:55
coop_trigger cScrTask05   # arm then fire (id 153)
coop_trigger cScrTask14   # arm then fire (id 152)
coop_trigger cScrTask06   # arm then fire (id 121) — this is the one that moved the wolves
.\run-coop-local.ps1 -AddClient 3 -Map MAP2A
# game3 pid 12956, welcomed as player 2 at 15:57:55
coop_kill 154:30          # written to game3\Bin\dh2coop.cmd
```

`cScrTask05` / `cScrTask14` are task 3 in `story-objectives.md`. Firing them left every animal at `(-10000)`. `TRIGGERMISSION02A.SCR` `cScrTask06` is the call that `MoveToPos`s wolf1/wolf2 and starts `AIHuntTask_AttackDogs`. After that fire the host showed wolves off the hidden point:

```
[2026-10-17 15:57:41.258] animals: sync role=host id=154 pos=(-87.66,24.71,37.65) state='IdleSurprise' hp=50.0 yaw=44.6
[2026-10-17 15:57:41.258] animals: sync role=host id=155 pos=(-119.58,19.69,3.28) state='IdleAttack' hp=50.0 yaw=44.6
```

id 156 stayed at `(-10000)`.

game3:

```
[2026-10-17 15:57:55.538] coop: welcomed as player 2 (max 4)
[2026-10-17 15:57:55.604] events: synced with host on 'MAP2A' (14 commits replayed)
[2026-10-17 15:57:55.605] animals: mirror freeze id=154 ok=true npc=false state='IdleSurprise'
[2026-10-17 15:57:55.605] animals: mirror first place id=154 spawn=(-87.7,24.7,37.6) target=(-87.7,24.7,37.7) yaw=44.6 (repeat 3 frames)
[2026-10-17 15:58:13.972] animals: local hit id=154 dmg=30.00 localHp=20.00 hostHp=50.00 sent=true
```

host:

```
[2026-10-17 15:58:13.984] animals: AnimalHit id=154 shooter=2 dmg=30.00 hp 50.0->20.0 ok=true
```

The next host sync is `hp=20.0` at the same spot. All three processes were still up.

### Hypothesis

UNVERIFIED. A late joiner receives the wolf's current pose (`mirror freeze` / `mirror first place`) and a `coop_kill` on that mirror is an `AnimalHit` the host applies (shooter = the joiner's player id). `cScrTask05`/`14` start the mission task and do not themselves move the wolves.

### Hypothesis

UNVERIFIED.

## Task 5 — Fix checks

```
.\run-coop-local.ps1 -Map MAP1
# host pid 11676, client pid 21424, welcome 15:58:53
# client command file:
coop_trigger cScrTask03
coop_trigger cScrTask03
# later, for the gather attempt:
coop_tp -14.9:60.1:40          # client
coop_trigger cScrTask07        # host, arm then fire
```

### (a) Client-requested fire

The client asked the host to run `cScrTask03`. The host log does **not** contain `ran here: true` or `[host] trigger FIRE ... (host commit)` for that fire. What it contains:

```
[2026-10-17 15:59:24.979] story: trigger 'cScrTask03' (id 94) requested by player 1 -> active
[2026-10-17 15:59:27.000] story: trigger 'cScrTask03' (id 94) fired by player 1 (ran here: false)
[2026-10-17 15:59:27.000] events: commit kind=6 subject=94 actor=1 i0=0 key='cScrTask03' seq=5 sent=true journal=5
```

The client still applied it: `story: applied host fire 'cScrTask03' (id 94)`. The same `ran here: false` line was logged earlier for the map's own `cScrTask01` (`fired by player 1`). A `(host commit)` suffix does appear, on an activate rather than the requested fire:

```
[2026-10-17 15:58:55.000] story: [host] trigger event activate id=93 name='cScrTask02' ... caller=GAMEDLL7.DLL+0x6aac7 (host commit)
```

There is no `[host] trigger FIRE` line for `cScrTask03` at all.

### (b) Game-skipped cutscene

The phrase `skipped by the game itself; clients keep theirs` does not appear in either MAP1 log. Twenty seconds after `cScrTask01` was requested, the host treated the skip as its own and the client followed:

```
[2026-10-17 15:59:15.689] story: host skipped the cutscene of 'cScrTask01' (id 92)
[2026-10-17 15:59:15.689] events: commit kind=11 subject=92 actor=0 i0=0 key='cScrTask01' seq=3 sent=true journal=3
[2026-10-17 15:59:15.701] story: will skip the cutscene of trigger id 92 (host skipped it)
[2026-10-17 15:59:15.701] story: skipped cutscene of 'cScrTask01' (id 92) ok=true
```

No key was sent to this pair by this job. `repro-t5-host-1.png` and `repro-t5-client-1.png` are both normal gameplay at the cabin (no cutscene camera). Host `cScrTask07` then fired with no skip line. Both windows showed the same "PRESS MOUSE 2 TO STRIKE AN ANIMAL..." page (`repro-t5-host-cs.png`, `repro-t5-client-cs.png`). The client did apply that fire (`applied host fire 'cScrTask07'`).

### (c) Gather after the cutscene

MAP1 produced no `cutscene over` / `gathering` / `teleport to host` line. `coop_tp` on the client logged `hunter to (-14.9,60.1,40.0)` at 16:00:24, and the next tracker samples (16:00:2x) are still `pos=(-14.91,60.14,-7.14)`. The teleport did not stay.

The gather lines did show up earlier, on MAP2A run 2, when the client's own opening cutscene ended while the test teleport had left it 119.5 m from the host:

```
[2026-10-17 15:45:48.863] story: cutscene over, host 119.5 m away: gathering
[2026-10-17 15:45:48.863] coop: teleport to host (after cutscene) from (-49.4,39.8,30.6) to (-139.0,36.4,109.7)
```

Shots after that (`repro-t1r2-client-2.png` onward) show the client hunter on the host's spot (`Host 0m`).

### Hypothesis

UNVERIFIED. The client fire request is committed and applied, but this build logged `ran here: false` and did not print a host-commit FIRE line, so check (a) as worded did not pass. The one cutscene skip that happened was forwarded (`host skipped` / client `skipped cutscene ok=true`), which is the manual-skip path, not the "game itself" path. Gather does run when a client cutscene camera ends far from the host; it also pulled the MAP2A client off the lion in task 1 run 2.

## Proposed MEMORY facts

- MAP2A `cScrTask04` is entity id 120. Firing it places animal id 157 (hp 80) at about (-36.9, 42.5, 14.5) in `IdleSurprise`. Evidence: `game\Bin\dh2coop.log` 15:41:41.693 and 15:45:35.512.
- The host retarget log for that lion is `combat: animal id 157 switches to the nearer player 1 (18.2 m, was 139.7 m) ok=true` (15:41:53.909) and the same sentence with 18.1 m at 15:45:37.878.
- After the lion's last real sample, later `animals: sync` lines for id 157 use `pos=(-10000.00,-10000.00,-10000.00)` while the state name stays `LookL45` or `KeepAnim`. Evidence: host log after 15:42:12 (71 hidden sync lines) and client sync at 15:43:42.
- A client cutscene end far from the host logs `story: cutscene over, host 119.5 m away: gathering` and `coop: teleport to host (after cutscene) from (-49.4,39.8,30.6) to (-139.0,36.4,109.7)`. Evidence: `game2\Bin\dh2coop.log` 15:45:48.863.
- MAP2A wolves that left the hidden position after `cScrTask06` (id 121) fired: id 154 at (-87.66, 24.71, 37.65) `IdleSurprise` hp 50, id 155 at (-119.58, 19.69, 3.28) `IdleAttack` hp 50. Evidence: host log 15:57:41.258. `cScrTask05` (id 153) and `cScrTask14` (id 152) fired first and did not move them.
- Late joiner (player 2, `game3`) `coop_kill 154:30` logged `animals: local hit id=154 dmg=30.00 localHp=20.00 hostHp=50.00 sent=true` and the host logged `animals: AnimalHit id=154 shooter=2 dmg=30.00 hp 50.0->20.0 ok=true`. Evidence: `game3\Bin\dh2coop.log` and `game\Bin\dh2coop.log` 15:58:13.
- MAP1 client `coop_trigger cScrTask03` produced host `story: trigger 'cScrTask03' (id 94) fired by player 1 (ran here: false)` and no `[host] trigger FIRE` line for id 94. Evidence: `game\Bin\dh2coop.log` 15:59:27.000.
- The MAP1 `cScrTask01` skip was `story: host skipped the cutscene of 'cScrTask01' (id 92)` plus client `story: skipped cutscene of 'cScrTask01' (id 92) ok=true`. The line `skipped by the game itself; clients keep theirs` was not logged. Evidence: both MAP1 logs at 15:59:15.
