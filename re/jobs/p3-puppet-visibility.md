# Job: P3 — make the puppet visible (Phase 3 ghost mode). YOU OWN GAME LAUNCHES. Ghidra slot P.

Context: the lead (Claude) built the puppet system and handed this off at a session limit. **Read `re/notes/lead-puppets.md` in full first.** Its "Runtime status" and "Next steps" sections are your plan. Also read `STATUS.md`, `MEMORY.md` (sections "Lead runtime findings" and "Object model") and `docs/mod-learnings.md` (Testing loop, Hooking, game-thread rule).

Code: `mod/src/coop/puppets.cpp`, `mod/src/engine/entity_api.*`, `mod/src/coop/player_tracker.*`. Config: `game\Bin\dh2coop.ini` `[Puppets]` (`Spike=1` spawns a local test puppet; `SpikeSign`, `Kind`, `Class`, `Animate`).

## Problem
A runtime-spawned `obj_humanEx` puppet with the hunter's model exists, and the user collides with it, but it is not visible on screen. The user looked in both directions, so orientation is ruled out. **This is a rendering problem. Start at task 2.**

## How to see the screen
The free-hunt start shows a "CHOOSE ITEM" weapon menu that synthetic input cannot dismiss. Your screenshots therefore show the menu, not the world.

Options, in this order:
- Ask the user: the lead's handoff says the user can run the game and report.
- Find a console command or engine call that closes the menu. Track D found `closemainmenu` and `closegamemenu`; try `dh2::console::Exec("closegamemenu")` from a debug path after map load, and verify that it really closes the menu.
- Rely on logs: hook the draw functions and log whether the puppet is drawn.

Screenshots use `tools-src/capture-window.ps1`, in windowed mode: copy `tools-src\console.windowed.cfg` to `game\Data\Cfg\console.cfg`, and delete it afterwards.

## Tasks, in order (stop when the puppet is visible)
1. (Done by the user: not visible in either direction.)
2. Add a **logging hook** on `cObjEntity::Draw` (EngineDll7 export `?Draw@cObjEntity@@UAEXXZ`). Log once per second whether `this` equals the puppet entity, and also log one deer's draw (entity index 120). Pass through to the original. This tells you whether the puppet is ever drawn.
3. If `Draw` is never called for the puppet, investigate BSP linkage per `useServer` side (see the notes, step 3). Keep any `UseServer` toggling minimal, and always restore the previous value.
4. If `Draw` is called but nothing shows, work through the notes' step 4, then step 5 (`Class=obj_cschex`).
5. Once visible: set `Spike=0`, then run `.\run-coop-local.ps1 -Seconds 120`. Each instance should spawn a puppet for the other player (the `puppets: spawned puppet (remote N)` log line). Get a screenshot or user confirmation that each window shows the other player moving.

## Rules
- Keep changes inside `mod/src/coop/puppets.cpp`, `mod/src/engine/entity_api.*`, and new files under `mod/src/coop/` or `mod/src/engine/`. Do not change networking (`mod/src/net/**`) or the framework (`mod/src/core/**`).
- Every engine call must stay on the game thread (`OnFrame` / `OnMapLoaded`) and be SEH-guarded through `entity_api`.
- Record every experiment and its result in `re/notes/lead-puppets.md` under a new `## Grok session log` section: what you changed, what the log said, and what the user or screenshot showed.
- Update `STATUS.md` → "In flight" / "Recent results" when you finish, or when you stop.

## Deliverables
- A visible puppet, or a precise written account of which step failed and why.
- Notes in `re/notes/lead-puppets.md` and `## Proposed MEMORY facts` there.
