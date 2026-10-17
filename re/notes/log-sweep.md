# Log sweep (job `log-sweep`)

Research note, read-only sweep. Lead: Opus main thread. Date of logs: 2026-10-16 21:35 to 2026-10-17 16:30 (local time, as in the logs). Nothing in the game tree was changed. No game process was touched.

## What was done

- Read `re\jobs\_claude-rules.md`.
- Listed every in-scope log: 14 mod logs (`dh2coop.log`), 34 engine logs (`debug*.log` in game `Bin` folders, including the original install), 4 coop-local `map12c-*.log` files (2 timestamped, 2 engine), and 4 coop-local `.txt` notes. That is 56 files, 7.7 MB. `%LOCALAPPDATA%\dh2coop` holds only `lasthost.txt` and `lastmap.txt` (no logs). No dh2coop logs under `VirtualStore`.
- Copied all 56 files to `%TEMP%\dh2-logsweep\raw\` with a normal copy (no lock errors) and analysed the copies only. The original install was read, never written.
- Roles: `coop: host start` = host; `coop: welcomed` or `coop: join start` = client; neither = solo. Engine `debug*.log` files take the role of the `dh2coop.log` in the same `Bin` folder. The `map12c-*-debug.log` files take it from their file name.
- Maps: from `cBspTree::Load('Bsp\X.bsp')` and `[APP] map 'X' loaded`, upper-cased. A line is "boot" if it comes before the first map load in its file, which is before `-spmap` takes effect.
- Selection: level warning, error or critical in a mod or note line; any line containing the brief's keywords (exception, first-chance, Could not forward, `-> -1`, failed, no such, not a, refused, timeout, crash, fatal, assert). Engine `debug.log` lines have no level, so they also match couldn't, could not, not found, unable, invalid, missing, undefined, nil, attempt to, error, warning.
- Normalisation for the key: timestamps and thread ids removed; quoted text to `'~'`; hex to `0x#`; numbers to `#`; Windows paths to `<path>`; `*.skin` names to `<skin>`. The map is kept as its own column.
- Severity: `critical` / `error` / `warn` from the log level. For engine and note lines, derived from the keyword: `error` if it says exception, first-chance, fatal, assert or error; otherwise `warn`. `kw` = info-level mod line matched only by a keyword (not an alarm the mod raised itself).
- Flags: CLIENT-ONLY / HOST-ONLY / SOLO-ONLY (roles seen), ONE-MAP, POST-SPMAP (never before the first map load), BOOT-ONLY (only before the first load), ONE-FILE.
- Result: 2115 matching lines in 37 groups. Scripts: `%TEMP%\dh2-logsweep\scripts\sweep.py` and `report.py`; output in `%TEMP%\dh2-logsweep\out\`.

Caveats on counts: the story notes (`story-s1-smoke.txt`, `story-s1-digest.txt`) cover runs from 10-16 21:35 to 21:55 that have no raw log in the set. They are separate runs, not copies of the `map12c-*` logs (which are from 22:07). Same-signature AVs recur across them, with different module bases, so the counts are independent occurrences. The "Files" column shows where each line sits. The digest has `HH:MM:SS` times only, so its span uses the file mtime.

## 1. Inventory of logs

Path is relative to `D:\scary hunt\`. Role is the role of the run in that file. "header only" means a 5-line stub with no content.

| # | Log (path under `D:\scary hunt\`) | Kind | Role | Maps (load order) | Span | Lines | Launches | warning / error |
|---|---|---|---|---|---|---|---|---|
| 1 | `Activision Value/Magic Wand/DNGH2/Bin/debug.log` | eng | host | MAP1, MAP2A, MAP2B | untimed; mtime 10-17 12:06 | 2901 | 0 | 0 / 0 |
| 2 | `Activision Value/Magic Wand/DNGH2/Bin/dh2coop.log` | mod | host | MAP1, MAP2A, MAP2B | 10-17 11:33:57 to 10-17 12:06:44 | 18716 | 1 | 0 / 11 |
| 3 | `dh2coop-npcarms/game/Bin/debug.log` | eng | solo | HUNTMAP4 | untimed; mtime 10-17 10:04 | 715 | 0 | 0 / 0 |
| 4 | `dh2coop-npcarms/game/Bin/dh2coop.log` | mod | solo | HUNTMAP4 | 10-17 09:57:12 to 10-17 10:04:29 | 1727 | 1 | 0 / 0 |
| 5 | `dh2coop-replay/game2/Bin/debug.1.log` | eng | client (stub) | - | header only | 5 | 0 | 0 / 0 |
| 6 | `dh2coop-replay/game2/Bin/debug.2.log` | eng | client (stub) | - | header only | 5 | 0 | 0 / 0 |
| 7 | `dh2coop-replay/game2/Bin/debug.log` | eng | client | MAP1 | untimed; mtime 10-17 16:09 | 294 | 0 | 0 / 0 |
| 8 | `dh2coop-replay/game2/Bin/dh2coop.log` | mod | client | MAP1 | 10-17 16:08:08 to 10-17 16:09:12 | 5808 | 1 | 1 / 0 |
| 9 | `dh2coop-replay/game3/Bin/debug.1.log` | eng | client (stub) | - | header only | 5 | 0 | 0 / 0 |
| 10 | `dh2coop-replay/game3/Bin/debug.log` | eng | client | MAP1 | untimed; mtime 10-17 16:07 | 277 | 0 | 0 / 0 |
| 11 | `dh2coop-replay/game3/Bin/dh2coop.log` | mod | client | MAP1 | 10-17 16:07:05 to 10-17 16:07:52 | 6601 | 1 | 1 / 0 |
| 12 | `dh2coop-replay/game/Bin/debug.1.log` | eng | host (stub) | - | header only | 5 | 0 | 0 / 0 |
| 13 | `dh2coop-replay/game/Bin/debug.2.log` | eng | host (stub) | - | header only | 5 | 0 | 0 / 0 |
| 14 | `dh2coop-replay/game/Bin/debug.log` | eng | host | MAP1 | untimed; mtime 10-17 16:09 | 296 | 0 | 0 / 0 |
| 15 | `dh2coop-replay/game/Bin/dh2coop.log` | mod | host | MAP1 | 10-17 16:08:07 to 10-17 16:09:12 | 1077 | 1 | 0 / 1 |
| 16 | `dh2coop-smoke/game2/Bin/debug.1.log` | eng | client (stub) | - | header only | 5 | 0 | 0 / 0 |
| 17 | `dh2coop-smoke/game2/Bin/debug.2.log` | eng | client (stub) | - | header only | 5 | 0 | 0 / 0 |
| 18 | `dh2coop-smoke/game2/Bin/debug.log` | eng | client | MAP3B | untimed; mtime 10-17 16:26 | 258 | 0 | 0 / 0 |
| 19 | `dh2coop-smoke/game2/Bin/dh2coop.log` | mod | client | MAP3B | 10-17 16:25:06 to 10-17 16:26:12 | 813 | 1 | 0 / 0 |
| 20 | `dh2coop-smoke/game/Bin/debug.1.log` | eng | host (stub) | - | header only | 5 | 0 | 0 / 0 |
| 21 | `dh2coop-smoke/game/Bin/debug.2.log` | eng | host (stub) | - | header only | 5 | 0 | 0 / 0 |
| 22 | `dh2coop-smoke/game/Bin/debug.log` | eng | host | MAP3B | untimed; mtime 10-17 16:26 | 259 | 0 | 0 / 0 |
| 23 | `dh2coop-smoke/game/Bin/dh2coop.log` | mod | host | MAP3B | 10-17 16:25:04 to 10-17 16:26:12 | 799 | 1 | 0 / 0 |
| 24 | `dh2coop-weapons/game/Bin/debug.1.log` | eng | solo (stub) | - | header only | 5 | 0 | 0 / 0 |
| 25 | `dh2coop-weapons/game/Bin/debug.2.log` | eng | solo (stub) | - | header only | 5 | 0 | 0 / 0 |
| 26 | `dh2coop-weapons/game/Bin/debug.log` | eng | solo | MAP3B | untimed; mtime 10-17 15:53 | 202 | 0 | 0 / 0 |
| 27 | `dh2coop-weapons/game/Bin/dh2coop.log` | mod | solo | MAP3B | 10-17 15:53:04 to 10-17 15:53:11 | 167 | 1 | 0 / 0 |
| 28 | `dh2coop-yeti/game2/Bin/debug.1.log` | eng | client (stub) | - | header only | 5 | 0 | 0 / 0 |
| 29 | `dh2coop-yeti/game2/Bin/debug.2.log` | eng | client (stub) | - | header only | 5 | 0 | 0 / 0 |
| 30 | `dh2coop-yeti/game2/Bin/debug.log` | eng | client | MAP3B | untimed; mtime 10-17 16:24 | 359 | 0 | 0 / 0 |
| 31 | `dh2coop-yeti/game2/Bin/dh2coop.log` | mod | client | MAP3B | 10-17 16:22:00 to 10-17 16:24:31 | 821 | 1 | 0 / 0 |
| 32 | `dh2coop-yeti/game/Bin/debug.1.log` | eng | host (stub) | - | header only | 5 | 0 | 0 / 0 |
| 33 | `dh2coop-yeti/game/Bin/debug.2.log` | eng | host (stub) | - | header only | 4 | 0 | 0 / 0 |
| 34 | `dh2coop-yeti/game/Bin/debug.log` | eng | host | MAP3B | untimed; mtime 10-17 16:24 | 347 | 0 | 0 / 0 |
| 35 | `dh2coop-yeti/game/Bin/dh2coop.log` | mod | host | MAP3B | 10-17 16:21:58 to 10-17 16:24:31 | 813 | 1 | 0 / 0 |
| 36 | `dh2coop/game2/Bin/debug.1.log` | eng | client (stub) | - | header only | 5 | 0 | 0 / 0 |
| 37 | `dh2coop/game2/Bin/debug.2.log` | eng | client (stub) | - | header only | 5 | 0 | 0 / 0 |
| 38 | `dh2coop/game2/Bin/debug.log` | eng | client | MAP1 | untimed; mtime 10-17 16:30 | 286 | 0 | 0 / 0 |
| 39 | `dh2coop/game2/Bin/dh2coop.log` | mod | client | MAP1 | 10-17 16:29:35 to 10-17 16:30:16 | 734 | 1 | 0 / 0 |
| 40 | `dh2coop/game3/Bin/debug.1.log` | eng | client (stub) | - | header only | 5 | 0 | 0 / 0 |
| 41 | `dh2coop/game3/Bin/debug.log` | eng | client | MAP2A | untimed; mtime 10-17 15:58 | 259 | 0 | 0 / 0 |
| 42 | `dh2coop/game3/Bin/dh2coop.log` | mod | client | MAP2A | 10-17 15:57:54 to 10-17 15:58:36 | 4622 | 1 | 0 / 0 |
| 43 | `dh2coop/game4/Bin/debug.log` | eng | client | HUNTMAP4 | untimed; mtime 10-17 10:12 | 307 | 0 | 0 / 0 |
| 44 | `dh2coop/game4/Bin/dh2coop.log` | mod | client | HUNTMAP4 | 10-17 10:12:01 to 10-17 10:12:52 | 755 | 1 | 0 / 4 |
| 45 | `dh2coop/game/Bin/debug.1.log` | eng | host (stub) | - | header only | 5 | 0 | 0 / 0 |
| 46 | `dh2coop/game/Bin/debug.2.log` | eng | host (stub) | - | header only | 5 | 0 | 0 / 0 |
| 47 | `dh2coop/game/Bin/debug.log` | eng | host | MAP1 | untimed; mtime 10-17 16:30 | 288 | 0 | 0 / 0 |
| 48 | `dh2coop/game/Bin/dh2coop.log` | mod | host | MAP1 | 10-17 16:29:33 to 10-17 16:30:16 | 728 | 1 | 0 / 2 |
| 49 | `dh2coop/re/notes/coop-local/focus-test.txt` | notes-txt | n/a (notes) | - | untimed; mtime 10-16 13:40 | 26 | 0 | 0 / 0 |
| 50 | `dh2coop/re/notes/coop-local/map12c-client-debug.log` | notes-log | client | MAP12C | untimed; mtime 10-16 22:08 | 2703 | - | 0 / 0 |
| 51 | `dh2coop/re/notes/coop-local/map12c-client.log` | notes-log | client | MAP12C | 10-16 22:08:04 to 10-16 22:08:24 | 177 | 1 | 16 / 3 |
| 52 | `dh2coop/re/notes/coop-local/map12c-host-debug.log` | notes-log | host | MAP12C | untimed; mtime 10-16 22:08 | 267 | - | 0 / 0 |
| 53 | `dh2coop/re/notes/coop-local/map12c-host.log` | notes-log | host | MAP12C | 10-16 22:07:55 to 10-16 22:08:24 | 242 | 1 | 0 / 1 |
| 54 | `dh2coop/re/notes/coop-local/run-log.txt` | notes-txt | n/a (notes) | - | untimed; mtime 10-17 16:30 | 51 | 0 | 0 / 0 |
| 55 | `dh2coop/re/notes/coop-local/story-s1-digest.txt` | notes-txt | host + client (role column) | - | untimed (HH:MM:SS only); 21:35-21:49 | 206 | 0 | 0 / 0 |
| 56 | `dh2coop/re/notes/coop-local/story-s1-smoke.txt` | notes-txt | host + client sections | MAP12B, MAP12C, MAP4A, MAP11B | 10-16 21:52:49 to 10-16 21:55:10 | 1236 | 4 | 0 / 4 |

Totals: 16 timestamped mod sessions: 6 host, 8 client, 2 solo (the 14 `dh2coop.log` files plus the two `map12c-*.log` copies). Engine debug logs have no timestamps, so their span is the file mtime.

## 2. Grouped table

Sorted by severity, then count. Columns: N = lines, Files = distinct source files, Boot/Post = lines before and after the first map load. "First/Last seen" is the log time; untimed groups show the file mtime range.

| # | Sev | N | Files | Roles | Maps | Boot/Post | First seen | Last seen | Flags | Normalized message | Example (shortest) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | error | 18 | 16 | client, host, solo | MAP1, MAP2A | 16/2 | mtime 10-16 22:08 | mtime 10-17 16:30 | - | `[SHADERMAN] Shader error '~' from shader '~'` | `Shader error 'attempt to call a nil value' from shader 'LightMapShader'` |
| 2 | error | 15 | 15 | client, host, solo | - | 15/0 | mtime 10-16 22:08 | mtime 10-17 16:30 | BOOT-ONLY | `[APP] Registry error: unable to open registry !` | `Registry error: unable to open registry !` |
| 3 | error | 11 | 4 | client, host | HUNTMAP4, MAP1, MAP2B | 0/11 | 10-17 10:12:52 | 10-17 16:30:16 | POST-SPMAP | `first-chance exception 0x# at dh2coop.asi+0x# eip=0x#` | `first-chance exception 0xc0000005 at dh2coop.asi+0xcd727 eip=0x6157d727` |
| 4 | error | 9 | 3 | host | MAP1, MAP12C, MAP2A, MAP2B | 0/9 | 10-16 21:54:00 | 10-17 12:06:44 | HOST-ONLY POST-SPMAP | `first-chance exception 0x# at <unknown>+0x# eip=0x#` | `first-chance exception 0xc0000005 at <unknown>+0x616d7548 eip=0x616d7548` |
| 5 | error | 7 | 3 | client | MAP12C | 0/7 | 10-16 21:53:53 | 10-16 22:08:24 | CLIENT-ONLY ONE-MAP POST-SPMAP | `first-chance exception 0x# at EngineDll7.dll+0x# eip=0x#` | `first-chance exception 0xc0000005 at EngineDll7.dll+0x5e73e eip=0xcfe73e` |
| 6 | error | 7 | 1 | client, host | - | 0/7 | mtime 10-16 21:49 | mtime 10-16 21:49 | POST-SPMAP ONE-FILE | `first-chance exception 0x# at nvgpucomp32.dll+0x# eip=0x#` | `first-chance exception 0xc0000005 at nvgpucomp32.dll+0x107554 eip=0x616d7554` |
| 7 | warn | 1151 | 1 | client | MAP12C | 0/1151 | mtime 10-16 22:08 | mtime 10-16 22:08 | CLIENT-ONLY ONE-MAP POST-SPMAP ONE-FILE | `[SYSTEM] Could not forward animation idleattack1 from Legs to Legs model <skin>` | `Could not forward animation idleattack1 from Legs to Legs model objects/animals/yeti/yeti.skin` |
| 8 | warn | 380 | 4 | client, host, solo | HUNTMAP4, MAP1, MAP12C, MAP2A, MAP2B, MAP3B | 0/380 | mtime 10-16 22:08 | mtime 10-17 16:24 | POST-SPMAP | `[MESHMAN] Could not set anim '~' on bone '~' on model '~'` | `Could not set anim 'risehead' on bone 'Bip' on model 'objects/animals/reddeer/reddeer.skin'` |
| 9 | warn | 252 | 16 | client, host, solo | HUNTMAP4, MAP1, MAP12C, MAP2A, MAP3B | 16/236 | mtime 10-16 22:08 | mtime 10-17 16:30 | - | `[SYSTEM] LUA: '~'` | `LUA: 'attempt to call a nil value'` |
| 10 | warn | 78 | 16 | client, host, solo | HUNTMAP4, MAP1, MAP12C, MAP2A, MAP3B | 0/78 | mtime 10-16 22:08 | mtime 10-17 16:30 | POST-SPMAP | `[FX] couldn't load FX: '~'` | `couldn't load FX: 'bush_imp_MAP1'` |
| 11 | warn | 32 | 16 | client, host, solo | HUNTMAP4, MAP1, MAP12C, MAP2A, MAP3B | 0/32 | mtime 10-16 22:08 | mtime 10-17 16:30 | POST-SPMAP | `[SOUND] '~' : '~' in '~' at line #` | `'protoIndex>=0 && protoIndex < m_sound.GetSize()' : 'Invalid sound index for release' in 'Core\Src\Sou...` |
| 12 | warn | 23 | 16 | client, host, solo | HUNTMAP4, MAP1, MAP12C, MAP2A, MAP2B | 16/7 | mtime 10-16 22:08 | mtime 10-17 16:30 | - | `[PAK] file '~' not found` | `file 'fx\scripts\fx.ini' not found` |
| 13 | warn | 18 | 16 | client, host, solo | MAP1, MAP2A | 16/2 | mtime 10-16 22:08 | mtime 10-17 16:30 | - | `[SYSTEM] Couldn't load shader '~'` | `Couldn't load shader 'LightMapShader'` |
| 14 | warn | 16 | 2 | client, host | - | 16/0 | mtime 10-16 22:08 | mtime 10-16 22:08 | BOOT-ONLY | `[BSP] WARNING: model <skin> changed since map compile!` | `WARNING: model objects/level/msn/msn12/trees/msn12b2.skin changed since map compile!` |
| 15 | warn | 16 | 1 | client | MAP12C | 0/16 | 10-16 22:08:05 | 10-16 22:08:16 | CLIENT-ONLY ONE-MAP POST-SPMAP ONE-FILE | `animals: SetBodyState id=# '~' -> -#` | `animals: SetBodyState id=24 'Run' -> -1` |
| 16 | warn | 10 | 5 | client, host | HUNTMAP4, MAP1 | 0/10 | mtime 10-17 10:12 | mtime 10-17 16:09 | POST-SPMAP | `[MESHMAN] Could not create scripted body state machine with class cBodyHandgun3npc` | `Could not create scripted body state machine with class cBodyHandgun3npc` |
| 17 | warn | 10 | 5 | client, host | HUNTMAP4, MAP1 | 0/10 | mtime 10-17 10:12 | mtime 10-17 16:09 | POST-SPMAP | `[MESHMAN] Could not create scripted body state machine: cBodyHandgun3npc` | `Could not create scripted body state machine: cBodyHandgun3npc` |
| 18 | warn | 6 | 1 | client | HUNTMAP4 | 0/6 | mtime 10-17 10:12 | mtime 10-17 10:12 | CLIENT-ONLY ONE-MAP POST-SPMAP ONE-FILE | `[MESHMAN] Could not create scripted body state machine with class cGuideLegs` | `Could not create scripted body state machine with class cGuideLegs` |
| 19 | warn | 6 | 1 | client | HUNTMAP4 | 0/6 | mtime 10-17 10:12 | mtime 10-17 10:12 | CLIENT-ONLY ONE-MAP POST-SPMAP ONE-FILE | `[MESHMAN] Could not create scripted body state machine: cGuideLegs` | `Could not create scripted body state machine: cGuideLegs` |
| 20 | warn | 4 | 1 | host | MAP1, MAP2A, MAP2B | 0/4 | mtime 10-17 12:06 | mtime 10-17 12:06 | HOST-ONLY POST-SPMAP ONE-FILE | `[RENDER] WARNING:DofDX pipeline requires post processing texture enabled, DOF effect disabled!` | `WARNING:DofDX pipeline requires post processing texture enabled, DOF effect disabled!` |
| 21 | warn | 2 | 2 | client, host | MAP12C | 0/2 | mtime 10-16 22:08 | mtime 10-16 22:08 | ONE-MAP POST-SPMAP | `[MESHMAN] Could not create scripted body state machine with class cBodyShotgun1npc` | `Could not create scripted body state machine with class cBodyShotgun1npc` |
| 22 | warn | 2 | 2 | client, host | MAP12C | 0/2 | mtime 10-16 22:08 | mtime 10-16 22:08 | ONE-MAP POST-SPMAP | `[MESHMAN] Could not create scripted body state machine: cBodyShotgun1npc` | `Could not create scripted body state machine: cBodyShotgun1npc` |
| 23 | warn | 2 | 1 | client | MAP12C | 0/2 | mtime 10-16 22:08 | mtime 10-16 22:08 | CLIENT-ONLY ONE-MAP POST-SPMAP ONE-FILE | `[SHUTDOWN] The shutdown procedure '~' with address 0x# failed !` | `The shutdown procedure 'DoneScript' with address 0x1025a000 failed !` |
| 24 | warn | 2 | 2 | client | MAP1 | 0/2 | 10-17 16:07:07 | 10-17 16:08:32 | CLIENT-ONLY ONE-MAP POST-SPMAP | `story: prop anim id=# '~': no local entity` | `story: prop anim id=4394 'cs02cam01': no local entity` |
| 25 | warn | 1 | 1 | host | MAP1 | 0/1 | mtime 10-17 12:06 | mtime 10-17 12:06 | HOST-ONLY ONE-MAP POST-SPMAP ONE-FILE | `[MESHMAN] Could not create scripted body state machine with class cBodyRifle1npc` | `Could not create scripted body state machine with class cBodyRifle1npc` |
| 26 | warn | 1 | 1 | host | MAP1 | 0/1 | mtime 10-17 12:06 | mtime 10-17 12:06 | HOST-ONLY ONE-MAP POST-SPMAP ONE-FILE | `[MESHMAN] Could not create scripted body state machine: cBodyRifle1npc` | `Could not create scripted body state machine: cBodyRifle1npc` |
| 27 | warn | 1 | 1 | host | MAP2A | 0/1 | mtime 10-17 12:06 | mtime 10-17 12:06 | HOST-ONLY ONE-MAP POST-SPMAP ONE-FILE | `[SCRIPT] Function not found for: ClearFlags` | `Function not found for: ClearFlags` |
| 28 | warn | 1 | 1 | host | MAP2A | 0/1 | mtime 10-17 12:06 | mtime 10-17 12:06 | HOST-ONLY ONE-MAP POST-SPMAP ONE-FILE | `[SCRIPT] Member name not found: gamemenu_bkg` | `Member name not found: gamemenu_bkg` |
| 29 | warn | 1 | 1 | client | MAP12C | 0/1 | mtime 10-16 22:08 | mtime 10-16 22:08 | CLIENT-ONLY ONE-MAP POST-SPMAP ONE-FILE | `[SYSTEM] Warning: Texture (ENGINERENDERTARGETTEXTURE) still has (#) references!` | `Warning: Texture (ENGINERENDERTARGETTEXTURE) still has (1) references!` |
| 30 | warn | 1 | 1 | client | - | 0/1 | mtime 10-16 21:49 | mtime 10-16 21:49 | CLIENT-ONLY POST-SPMAP ONE-FILE | `events: applied host mission end code=# text='Mission failed` | `events: applied host mission end code=6 text='Mission failed` |
| 31 | warn | 1 | 1 | client | - | 0/1 | mtime 10-16 21:49 | mtime 10-16 21:49 | CLIENT-ONLY POST-SPMAP ONE-FILE | `events: apply commit kind=# subject=# actor=# i0=# key='Mission failed` | `events: apply commit kind=2 subject=0 actor=0 i0=6 key='Mission failed` |
| 32 | warn | 1 | 1 | host | - | 0/1 | mtime 10-16 21:49 | mtime 10-16 21:49 | HOST-ONLY POST-SPMAP ONE-FILE | `events: commit kind=# subject=# actor=# i0=# key='Mission failed` | `events: commit kind=2 subject=0 actor=0 i0=6 key='Mission failed` |
| 33 | warn | 1 | 1 | host | - | 0/1 | mtime 10-16 21:49 | mtime 10-16 21:49 | HOST-ONLY POST-SPMAP ONE-FILE | `events: host mission message code=# text='Mission failed` | `events: host mission message code=6 text='Mission failed` |
| 34 | warn | 1 | 1 | client | - | 0/1 | mtime 10-16 21:49 | mtime 10-16 21:49 | CLIENT-ONLY POST-SPMAP ONE-FILE | `story: trigger commit id=# '~': no such trigger here` | `story: trigger commit id=117 'cScrTask01': no such trigger here` |
| 35 | kw | 16 | 16 | client, host, solo | - | 16/0 | 10-16 22:07:55 | 10-17 16:29:35 | BOOT-ONLY | `crash filter installed; dump path <path>` | `crash filter installed; dump path D:\scary hunt\dh2coop\game\Bin\dh2coop-crash.dmp` |
| 36 | kw | 8 | 2 | host | MAP1, MAP12C, MAP2A, MAP2B | 0/8 | 10-16 22:08:23 | 10-17 12:06:44 | HOST-ONLY POST-SPMAP | `entity_api: UnlinkPart owner=0x# tag='~' -> -# (SetSyncWith(nullptr))` | `entity_api: UnlinkPart owner=0xe9472c8 tag='BipRHand' -> -1 (SetSyncWith(nullptr))` |
| 37 | kw | 4 | 2 | host | MAP1, MAP12C, MAP2A | 0/4 | 10-16 22:08:23 | 10-17 11:58:07 | HOST-ONLY POST-SPMAP | `coop: timeout player #` | `coop: timeout player 1` |

## 3. Notable (top 15 for the lead)

Each entry gives the group number from section 2, the evidence (file and line, all in the `raw` copies), and why it matters. Line numbers are the line in the source file.

1. **#3 first-chance AV in `dh2coop.asi` (11).** Four in ORIG (host, MAP1/MAP2A/MAP2B, 11:49 to 12:06), four in the client `game4` at exit (10:12:52.525), two in host `game` at exit (16:30:16.867), one in the replay host (16:08:32.805). The four ORIG AVs and the replay one are each the line just before `story: prop anim id=N 'csNNcamNN' name='<unreadable>'` (ORIG lines 6732, 7535, 8004, 18426; replay host line 509). Every `<unreadable>` prop-anim line in the mod logs is preceded by an AV (5 of 5). The exit-time AVs sit inside `puppets: unloading remote ... ref -99->-99`. Why: mod code is faulting on cutscene camera props. VERIFIED ordering; cause UNVERIFIED.
2. **#4 AV at `<unknown>+0x616d7548` (9).** ORIG 7 times (lines 832, 2099, 6292, 7350, 11910, 16234, 18713), host `map12c` once (line 232), and once in the smoke notes (21:54:00, a separate MAP12C run). The eip bytes (little-endian 48 75 6d 61) read as ASCII "Huma". That looks like execution jumping into string data, usually a clobbered function pointer or vtable. The preceding lines are puppet weapon/anim updates, attack-target removal, remote-player updates and event sync. Why: same signature every time, host only. UNVERIFIED interpretation.
3. **#5 client AVs in `EngineDll7.dll` (7).** In `map12c-client.log`: 22:08:17.127 (`+0x28b337`, line 175), right after `tracker: animal id=24`; then 22:08:24.162 and .164 (`+0x5e73e`, `+0x28b43a`, lines 176-177). The log ends at 22:08:24.164. The host logs `net: peer timed out` at 22:08:23.095. Why: the client dies in engine code in the same window as #15 (the yeti rejects state changes). Cause UNVERIFIED. The smoke notes have three more client AVs from a separate 21:53 MAP12C run (eip `0x00f2b337` vs `0x1028b337`, so a different EngineDll7 base).
4. **#6 `nvgpucomp32.dll` AV (7, digest only).** Seven digest entries attribute `eip=0x616d75xx` to `nvgpucomp32.dll`. The raw logs never name that DLL and show `<unknown>` for the same eip family. Why: the module name looks like a symbolizer mistake; the lead should confirm before using it. UNVERIFIED.
5. **#7 "Could not forward animation idleattack1 from Legs to Legs model yeti" (1151).** One file, `map12c-client-debug.log` (554 KB, client, MAP12C). Why: a flood that buries everything else in the file. It sits in the same yeti (id=24) window as #15.
6. **#15 `animals: SetBodyState id=24 'Attack'|'IdleAttack'|'Run' -> -1` (16).** Client, `map12c-client.log` lines 120-172, 22:08:05.599 to 22:08:16.317, all warning level. Every `id=24` state line in that file says `state='CutScene'` (8 of 8, from 22:08:05.563). Why: the client rejects every attack/run state change while the yeti is frozen in a cutscene. VERIFIED co-occurrence; cause UNVERIFIED. This is the `-> -1` case from the brief.
7. **#36 `entity_api: UnlinkPart ... tag='BipRHand'|'BipLHand' -> -1 (SetSyncWith(nullptr))` (8).** Host only: ORIG 7, host `map12c` 1. All 8 come 1 to 3 lines after an AV (8 of 8). A successful unlink on the client (game4) returns `-> 1`. Why: the unlink on the host fails right after a fault. VERIFIED ordering.
8. **#37 `coop: timeout player 1` (4, info level).** Host only: ORIG at 11:50:02, 11:53:01 and 11:58:07.435; `map12c-host` at 22:08:23.096. The 11:58:07 timeout is 1 ms after AV line 11910. The 22:08:23 timeout is 1 ms after AV line 232, which follows `net: peer timed out` (line 231). The other two timeouts have no AV within 29 s and 58 s. Why: a chain of client drop, peer timeout, host AV, unlink failure and timeout. VERIFIED ordering; causality UNVERIFIED.
9. **#24 `story: prop anim id=# 'cs02cam01': no local entity` (2, client, MAP1).** Replay `game2` at 16:08:32.817 for id 28477, and replay `game3` at 16:07:07.657 for id 4394. The host logged the same id 28477 at 16:08:32.806, 1 ms after its AV at 16:08:32.805 (#1). Why: the strongest host-to-client link in the set; the client cannot find the local entity for a prop the host just failed on. VERIFIED ids and times; cause UNVERIFIED. (The 16:07 session has no host log; see section 4.)
10. **#8 `Could not set anim '~' on bone '~' on model '~'` (380).** ORIG 308, `npcarms` 68, others 4. Every map, every role. Examples: `risehead` on bone `Bip` on `reddeer.skin`; `GMOH05a` on bone `jaw` on `oldhugh.skin`. Why: the most common engine warning in the set. Probably missing anim/bone pairs in content, not co-op. UNVERIFIED cause.
11. **#9 `[SYSTEM] LUA: 'attempt to call a nil value'` (252).** 16 files: 16 at boot (one per run) and 236 after map load; ORIG has 45. Why: a Lua call to a nil function in every run. Same message text as #1 (shader `LightMapShader`). Finding the calling script is the next step. UNVERIFIED which script.
12. **#11 `[SOUND] 'protoIndex>=0 && ...' : 'Invalid sound index for release' ... SoundLoad.cpp line 1166` (32).** Two per debug log in all 16 files, including solo runs. Why: an engine assertion on sound release that fires in every run, co-op or not. It may matter for sound-handle leaks. UNVERIFIED impact.
13. **#16-19, #21-22, #25-26: "Could not create scripted body state machine" (38 in total).** Classes `cBodyHandgun3npc` (20, host and client, 5 files), `cGuideLegs` (12, client, HUNTMAP4), `cBodyShotgun1npc` (4, MAP12C), `cBodyRifle1npc` (2, host, MAP1). Why: NPC classes without a state machine. Relevant to the npc-arms and npc-follow work. UNVERIFIED cause.
14. **#34 `story: trigger commit id=117 'cScrTask01': no such trigger here` (1, digest, client).** Why: a story trigger committed on the client that has no matching trigger. A possible story desync. One occurrence, digest only; the raw run is not in the set.
15. **#23 `[SHUTDOWN] The shutdown procedure 'DoneScript' / 'DoneConsole' failed` (2, client, MAP12C).** Why: exit-time failure on the same client that died in #5. Low-to-medium priority.

Noise, not in the top 15: #2 Registry error and #13 `LightMapShader` load failure (boot only, every run); #12 `fx\scripts\fx.ini not found` (boot); #14 `msn12*.skin changed since map compile` (boot, MAP12C debug logs); #10 `couldn't load FX 'bush_imp_<map>'` (missing per-map FX, every map); #20 DOF pipeline warning (config); #35 `crash filter installed` (info line, matched by the word "crash"); #30-33 "Mission failed" (game text, event code 6, matched by "failed").

## 4. Coverage gaps

- No `dh2coop.1.log` or `dh2coop.2.log` exists in any `Bin` folder. 11 of the 14 mod logs open with a header that says "previous runs: dh2coop.1.log, dh2coop.2.log, debug.1.log". None of those files exists, so only the latest run per folder survives, plus 5-line `debug.1/.2.log` stubs. Earlier sessions are gone.
- The replay session at 16:07 has a client (`dh2coop-replay\game3`, joined "Host" at 16:07:07) but no host log. The host log in `dh2coop-replay\game` starts at 16:08:07.
- The ORIG session (11:33 to 12:06) has only the host log. The `hotbabe` client's logs are not in the set.
- The `following host` string (listed in the brief) does not appear in any of the 56 files.
- The coop-local `story-s1-smoke.txt` has four sections (MAP12B, MAP12C, MAP4A, MAP11B). None of their raw logs is in the set. The MAP12C section is a 21:52 run, not the 22:07 `map12c-*` run. MAP12B, MAP4A and MAP11B exist only in this file.
- The digest's 7 `nvgpucomp32` AVs (21:35 to 21:43 on 10-16; the digest does not name the map) have no raw counterpart in the set.

## 5. Verdicts

VERIFIED:
- 5 of 5 `story: prop anim ... name='<unreadable>'` lines are preceded by a first-chance AV (ORIG 4, replay host 1).
- 8 of 8 `UnlinkPart ... -> -1` lines come 1 to 3 lines after a first-chance AV (ORIG 7, host `map12c` 1).
- 2 of 4 `coop: timeout player 1` lines come 1 ms after a host AV. The other 2 have none within 29 s.
- 16 of 16 `SetBodyState id=24 ... -> -1` lines fall in the client's yeti CutScene window (22:08:05.563 to 22:08:16.317).
- Host and client both log prop id 28477 for the cs02cam01 story prop within 11 ms (16:08:32.806 and .817).
- No `dh2coop-crash.dmp` exists anywhere under `D:\scary hunt\` (recursive search), although every one of the 16 mod sessions logs "crash filter installed; dump path ...". So none of the AVs produced a dump. Whether the engine handled them is UNVERIFIED.

UNVERIFIED:
- That the eip bytes spell ASCII "Huma" because execution jumped into string data (interpretation of the bytes, not confirmed by disassembly).
- That the digest's `nvgpucomp32.dll` attribution is wrong.
- Any causal chain (AV leading to timeout, timeout leading to unlink failure, etc.). Only ordering is shown.
- Cause of the `<unreadable>` story-prop name, and of the animal state rejections.

## 6. Open questions for the lead

- Why does the story prop lookup return `name='<unreadable>'` for `cs0Xcam0X` props, and is that read the source of the host AV? (RE slot L could check the story prop-anim hook.)
- What does `ref -99->-99` mean in `puppets: unloading remote`? It shows up on the unload lines around the exit-time AVs. Is the exit crash (client game4, host game at 16:30) the same unload path?
- Should the client drop attack/run state changes for a yeti in CutScene? Or is the 1151-line "Could not forward" flood the expected result of that?
- Is the host AV at 22:08:23 a result of the peer timeout cleanup (client AV at 22:08:17, timeout at 22:08:23, host AV, UnlinkPart -1, timeout)? A targeted run with a forced client drop would settle it.
- Do the "no such trigger" (story) and "no local entity" (prop) warnings need their own probe?

## 7. Proposed MEMORY facts

For the lead to merge. I did not edit `MEMORY.md` or `mod-learnings.md`.

- Cutscene camera prop anims (`story: prop anim ... 'csNNcamNN' name='<unreadable>'`) are each preceded by a first-chance AV in `dh2coop.asi` (ORIG 11:49 to 12:06, replay host 16:08). That covers 5 of the 11 `dh2coop.asi` AVs in the set. The other 6 happen at exit, inside the puppet unload.
- eip `0x616d7548` (bytes spell "Huma") is the signature of the 7 ORIG AVs and the map12c host AV in the puppet/teardown path. Unverified as a corrupt pointer.
- `entity_api: UnlinkPart ... -> -1` is always 1 to 3 lines after an AV on the host (8 of 8).
- `coop: timeout player N` is info level, not warning. It follows a host AV 1 ms later in 2 of 4 cases.
- In map12c (22:08), the client yeti (id 24) stays in CutScene and rejects every Attack/IdleAttack/Run SetBodyState (16 of 16, `-> -1`). Its anim-forward message then floods the client debug log (1151 lines).
- Engine `debug*.log` files carry no timestamps. Their span is the file mtime. The `dh2coop.log` header lists rotated `.1/.2` files that no longer exist.
- The `following host` string does not occur in any log.
- `story: prop anim id=28477 'cs02cam01'` is logged by host and client in the same replay session, 11 ms apart.
