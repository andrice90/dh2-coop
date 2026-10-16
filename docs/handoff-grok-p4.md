# Handoff to Grok Build: Phase 4 (shared animals)

**Written:** 2026-10-16 ~15:45 (America/Chicago) by the Claude lead at its usage limit. **Last commit:** `97c0e71`.
You (Grok 4.7) continue Phase 4 while the Claude lead is away. The user (andri) can watch and test the game windows and answer questions. Work in `D:\scary hunt\dh2coop`.

## Project in one paragraph
This is a co-op mod for Cabela's Dangerous Hunts 2 (2005, Chameleon engine, D3D8):
- An ASI loaded through `game\Bin\dinput8.dll` hooks the game; our own ENet session handles networking.
- **Phase 3 is done** and user-confirmed: each player sees the others as an NPC body (the mountain guide) with correct movement, facing, animations, weapon in hand, fire and reload.
- **Phase 4 makes the animals shared.** The host's game simulates them; clients mirror the host's animals and forward their bullet hits.

## Read first, in order
1. `CLAUDE.md`: the project rules and doc map.
2. `STATUS.md`: current state. Its resume protocol explains builds and tests.
3. `re/notes/lead-animals.md`: **the Phase 4 design and the verified engine recipe.** It is the most important file.
4. `re/jobs/_rules.md`, `re/jobs/p4a-net-animals.md`, `re/jobs/p4b-animal-sync.md`: your task specs.
5. `MEMORY.md`: verified engine facts. Skim "Entity spawning", "Hunter actions", "Lead runtime findings".
6. `docs/mod-learnings.md`: build/test tooling, the PowerShell 5.1 pitfalls, scripted input.

## Hard rules
- **Never write to** `D:\scary hunt\Activision Value\...` (the original install). Use `game\` and `game2\` only.
- **Never commit** game binaries, assets, extracted data or decompiled code. `.gitignore` already covers them.
- **Commit** your own source and doc changes after each verified step. Author: andri <andrice90@gmail.com>. End the message with the trailer `Co-Authored-By: Grok 4.7 <noreply@x.ai>`. Use small commits with clear messages.
- **Don't touch** `MEMORY.md`. Put proposed facts in your notes under `## Proposed MEMORY facts`; the lead merges them.
- **Keep `STATUS.md` updated** after every step, so a new session can resume. Add a section `## Grok session log (lead away)` with timestamped entries from `Get-Date`. Keep "Next steps" accurate. Edit docs with file-edit tools, not PowerShell `Set-Content` (encoding bugs).
- `mod\build.ps1` fails to install while `Main7.exe` runs. Close the game windows you launched first. **Ask the user before closing windows they are playing in.**
- Be honest in notes: write UNVERIFIED for anything you did not observe. Paste the decisive log lines.

## Current state of Phase 4
- **Done (lead):**
  - `mod/src/coop/animals.cpp`: animal state-name logging, plus a solo experiment, `[Animals] Spike=1`.
  - `entity_api`: `BodyStateName`, `SetBodyState`, `FreezeBrain`, `HandleKey`, `BspUpdate`.
  - `player_tracker`: `Animals()` list, `SetPreRenderCallback` (place entities after physics, before draw).
- **Verified recipe for driving an animal on a client** (details in `lead-animals.md`):
  - `FreezeBrain(animal, true)`. **Keep physics.** `UnlinkPhysics` makes animals invisible.
  - Each frame, in the **pre-render callback**: `HandleKey(animal, "position", "<x*100> <y*100> <z*100>")` and `HandleKey(animal, "angle", "0 <yaw> 0")`, then a world AABB refresh + `BspUpdate`.
  - Animation: `SetBodyState(animal, hostStateName, false)` when the host's state name changes. Deer states seen: `Walk`, `Run`, `IdleAttack`, `CutScene`, `PostPlayAnim`.
  - Damage: `cAIHuntAnimalInterface::FuncInflictDamage(void* animal, float dmg)` (CommonDll7, static cdecl) only subtracts health `+0x19C`.
- **P4a done and lead-reviewed** (commit `14250e0`). `AnimalStates` (id 9) and `AnimalHit` (id 10) are in `mod/src/net/*`, with `Session::SendAnimalStates`/`SendAnimalHit` and events. 3294 net tests pass. Details: `re/notes/p4a-net-animals.md`, `docs/net-protocol.md`.
  - Note: a 32-animal snapshot exceeds the ENet MTU. HUNTMAP4 has 5 animals (~200 B), so this is fine for now.
  - The installed `game\Bin\dh2coop.asi` may be older than the source: run `mod\build.ps1` once with the game closed.

## Known issue to watch in P4b
In the lead's solo experiment the controlled deer went invisible for part of the time **after large teleports**, even though the engine still called `Draw` on it every frame (`re/notes/lead-animals.md`, "Draw-hook result"). The first sync moves each client deer from its own random spawn point to the host's position, which is such a teleport.
- If client deer flicker or vanish, record exactly when, and try the listed workaround (repeat the first placement for a few frames).
- Don't dig into the engine skeleton code. That is lead work: write it into STATUS "Next steps".

## Your tasks, in order
1. ~~P4a~~: done.
2. **P4b: animal sync.** Implement exactly `re/jobs/p4b-animal-sync.md`: host broadcast at 20 Hz, client buffer + apply in pre-render, state-name mirroring, health write-back.
   - Test with two instances.
   - Then ask the user for a visual check: open the pair with `.\run-coop-local.ps1` (it leaves the windows running). The user closes the CHOOSE ITEM menu with Enter, Left, Enter. Ask them whether the same deer appear in the same places in both windows and move the same way.
3. **P4c: hits and death.**
   - Client health-drop detection → `AnimalHit` → host `FuncInflictDamage`.
   - Verify with the user: shoot a deer in the client window, and it reacts and dies on the host. The death shows on the client through the replicated state/health.
   - If the host deer doesn't flee when hit remotely, note it; don't go deep. That is lead work.
4. Clean up: the Spike path stays solo-only and is off by default. Document the new ini keys in `mod/dh2coop.ini`.

## How to test (tools exist; see `docs/mod-learnings.md`)
- **Two windows for the user:** `.\run-coop-local.ps1`. Host = left window (`game\`), client = right (`game2\`). Logs: `game\Bin\dh2coop.log`, `game2\Bin\dh2coop.log`.
- **Automated:** `.\tools-src\move-test.ps1 [-Walk 'W:1000',...]`. It launches the pair, closes the menus, drives the host with keys, captures both windows and prints the logs.
- **Scripted input:** `.\tools-src\game-input.ps1 -ProcessId <pid> -Keys 'RETURN','W:2000','MOUSE:400,0','LMB'`. The target window must be in the foreground; DirectInput reads it.
- **Screenshot without stealing focus:** `.\tools-src\capture-window.ps1 -ProcessId <pid> -Out re\notes\coop-local\x.png -NoFocus`.
- **Solo game:** `.\run-game.ps1 -ExtraArgs '-nointro -developper -cheats -spmap HUNTMAP4'`.

## When to stop and ask the user
- Anything that needs a visual judgment.
- Anything that would need decompiling new engine code (Ghidra): that is lead work. Write the question into `STATUS.md` "Next steps" for the lead instead.
- A crash you can't explain from the logs (`game\Bin\dh2coop-crash.dmp`, the last log lines).

## Expected deliverables when the lead returns
- Commits for P4a, P4b and (if reached) P4c.
- `re/notes/p4a-net-animals.md`, `re/notes/p4b-animal-sync.md` (with test tables and the user's visual verdicts).
- `STATUS.md` with the Grok session log and an accurate "Next steps".
