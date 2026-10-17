# Handoff to the Cursor worker: story mode co-op (S1 continued)

**Written:** 2026-10-16 ~21:35 (America/Chicago) by the Claude lead at ~80% of its 5-hour limit. **Last commit:** see `git log -1`.
You are Grok 4.7 running through the Cursor CLI (`cursor-agent`, model `grok-4.7-medium-fast`). Work in `D:\scary hunt\dh2coop`. The user (andri) can watch and play the game windows and answer questions.

## Project in one paragraph
This is a 2–4 player co-op mod (an ASI loaded via `game\Bin\dinput8.dll`) for the 2005 PC game Cabela's Dangerous Hunts 2. **The goal is the story mode in co-op.**
- The host's game is authoritative.
- Clients see the other players as NPC-bodied "puppets", and mirror the host's animals.
- Clients replay the host's decisions (mission end, trophy claim, map start, script trigger arming/firing) through one reliable `GameEvent` message.
- **Story S1 just worked on MAP3B:** with trigger sync, the leopards attacked and interacted with both players (user, 21:28).

## Read first, in order
1. `CLAUDE.md` (rules, doc map), then `STATUS.md` (where we are, resume protocol).
2. `re/notes/lead-story-coop.md`: the story design and the S1 results. **The most important file.**
3. `re/notes/lead-mission.md` (mission end / claim), `MEMORY.md` section "Story triggers and map flow".
4. `docs/mod-learnings.md`: build/test tooling, the automation channel `Bin\dh2coop.cmd`, PowerShell 5.1 pitfalls.
5. Code you will touch: `mod/src/coop/story.cpp` (trigger sync), `mod/src/coop/events.cpp|h` (event kinds).

## Hard rules
- **Never write to** `D:\scary hunt\Activision Value\...`. Use `game\` and `game2\` only.
- **Never commit** game binaries, assets, extracted data or decompiled code. `re/notes/story-inventory.md` is gitignored on purpose because it quotes game scripts.
- **Commit** your own source and doc changes after each verified step. Author `andri <andrice90@gmail.com>`, trailer `Co-Authored-By: Grok 4.7 <noreply@x.ai>`. Small commits.
- **Don't edit `MEMORY.md`.** Put facts under `## Proposed MEMORY facts` in your notes.
- **Update `STATUS.md`** after every step, in a section `## Cursor worker log (lead away)` with `Get-Date` timestamps. Edit docs with file-edit tools, not PowerShell `Set-Content`.
- `mod\build.ps1` cannot install while `Main7.exe` runs. **Ask the user before closing windows they are playing in.** Windows you launched for an automated test you may close.
- **No Ghidra / decompiling of new engine code.** If something needs engine RE, write the question into STATUS "Next steps" for the lead.
- Honest notes: write UNVERIFIED for anything you did not observe, and paste the decisive log lines.
- Python is `py`. In PowerShell, `[IO.File]` APIs resolve relative paths against `C:\Users\andri`: use absolute paths.

## How to run things
- **Build:** `.\mod\build.ps1` (installs to `game\Bin`; the launcher copies to `game2\Bin`).
- **Two windows on a story map:** `.\run-coop-local.ps1 -Map MAP3B`. Host = left (`game\`), client = right (`game2\`). It leaves the windows running. Story maps have no CHOOSE ITEM menu; HUNTMAP4 does (Enter, Left, Enter).
- **Drive a running game:** write console lines to `game\Bin\dh2coop.cmd` (host) or `game2\Bin\dh2coop.cmd` (client). Use absolute paths with `[IO.File]::WriteAllLines`. Commands:
  - `coop_trigger <name>`: arm a trigger event by name, and fire it if it is already armed.
  - `coop_kill <id>[:dmg]`, `coop_claim <id>`.
  - Engine commands, for example `spmap MAP12B`.
- **Logs:** `game\Bin\dh2coop.log`, `game2\Bin\dh2coop.log`. Story lines start with `story:`; events with `events:`. A new launch truncates both logs, so quote what you need first. The engine's own crash dialog text is in `Bin\debug.log` (`Access violation ... caught in function ...`).
- **Screenshot without stealing focus:** `.\tools-src\capture-window.ps1 -ProcessId <pid> -Out re\notes\coop-local\<name>.png -NoFocus`.
- **Symbolize** a `dh2coop.asi+0x…` crash address: `.\tools-src\symbolize.ps1 -Rva 'b8dbd'` (quoted hex).

## Your tasks, in order
1. **MAP3B full play-through with the user** (job spec `re/jobs/story-s1-playtest.md`).
   - Open the pair. Ask the user to play MAP3B to the end in the host window, with the client player moving too.
   - Record what diverges: cutscenes, tasks/HUD, animals, NPCs, mission end, deaths. Use the user's words verbatim, the log timeline, and screenshots.
   - Write a digest tool `tools-src\story-digest.ps1`. It takes both logs and prints one merged, time-sorted table of `story:`/`events:`/mission lines with a role column.
   - Deliverable: `re/notes/story-s1-map3b.md`.
2. **Next maps, automated smoke test.** For each of MAP12B, MAP12C, MAP4A, MAP11B (the lightest story maps):
   - `-Map <name>`, wait 30 s, screenshot both windows.
   - Record: does the client join? How many animals mirrored? Which trigger events armed at start on each side? Any `exception` lines?
   - Add a row per map to the same note.
3. **Small, safe fixes** you can verify without engine RE, for example log noise, tooling, ini docs. Anything about cutscenes, task completion, player damage, NPCs, death or map transitions is **lead work**: write the observation and the question into STATUS "Next steps".

## Known issues (don't chase them)
- After a same-map replay (`spmap` of the map already loaded), the next world teardown crashes in `cSndDrvDS::WaveRelease` (STATUS "PIVOT" section). Avoid replays in tests; restart the games instead.
- Clients are effectively invulnerable: host animals cannot hurt client players yet (Phase 5).
- Puppets are not animal attack targets (`[Puppets] AttackTarget=0`): enabling it crashes the host.

## When to stop and ask the user
- Any visual judgment, and any time the user is playing.
- A crash you can't explain from the logs: copy the `debug.log` crash text into your note and stop.

## Expected deliverables when the lead returns
- `re/notes/story-s1-map3b.md` (play-through findings + smoke-test table), `tools-src\story-digest.ps1`.
- `STATUS.md` with the worker log and accurate "Next steps".
- Commits for each verified step.
