# Job: story-objectives — objective types per story map, co-op risk, and a test checklist (read-only; no game launches; no Ghidra)

## Goal
Co-op works on MAP3B and MAP2A (trigger arm/fire sync, Uncle Hugh's position mirror, paired cutscene meshes, animal deaths, mission end, map-to-map follow). The user is worried about objective types nobody has tested yet: "guide the NPC" missions, escorts, and anything more complicated than "walk into a trigger, kill the animal". Produce a map-by-map list of the objective mechanics, how each one would behave under the current co-op model, and a short test plan the user can play.

## Inputs
- `re/notes/story-inventory.md` (gitignored, large; regenerate with `py re\scripts\story_inventory.py` only if missing). Sections 3, 5, 6, 7, 8 are the useful ones.
- Extracted scripts under `re/extracted/SCRIPTS/` (triggers per mission in `TRIGGERS/TRIGGERMISSION*.SCR`, cutscenes in `CUTSCENE/<MAP>/`, NPC scripts under `MODELS/NPC/`).
- The co-op model: `re/notes/lead-story-coop.md`, `docs/handoff-story-resume.md`, `STATUS.md` (top worker log), `mod/src/coop/story.cpp`, `mod/src/coop/animals.cpp` (NPC mirror, `kNpc`), `mod/src/coop/events.h` (event kinds).

Summary of the model, so you can judge risk:
- The host runs every script. Trigger *arming* (`cObjTriggerEvent`, GameDll `0x68eb0`) and *firing* (`0x68130`, runs the trigger's `OnActivate`) are decided on the host and replayed on clients by entity id + name. A client walking into a trigger sends a request; the host fires it.
- Clients run the same scripts too, but their own arm/fire decisions are swallowed (become requests). Mission end is host-only.
- Animals and world NPCs (`/npc/` models) are simulated on the host and mirrored on clients (brains frozen; position, yaw, body-state name copied). NPC arms/gun are not mirrored.
- Script `"hunter"` means the local player on each machine. On the host that is only the host's hunter: an NPC that follows "hunter" follows the host; a distance check against "hunter" measures the host.
- `PlaySA` (paired cutscene animations) and prop `SetAnim` are replayed on clients. Snakes (`cObjSnake`/`cObjCobra`) are not mirrored.
- Pause menus are per-process. A client death does not end the level (F9 stands the client up at the host). A host death does.

## Deliverable
`re/notes/story-objectives.md` only. Do not quote game scripts beyond short identifiers (names of functions, triggers, tasks, NPC objects) and at most one short line per finding; paraphrase the rest. This file will be committed.

Sections:
1. **Campaign order.** The story maps in play order with the branch points (3A/3B/3C, 11A/11B/11C, 12A/12B/12C, etc.) as far as the scripts show it (map-end names, `exit_*` triggers, map-load tokens). Mark guesses UNVERIFIED.
2. **Objective mechanics catalogue.** One row per mechanic type you find, for example: reach a trigger zone; kill N animals / a named animal; protect or escort an NPC; NPC guides the player (follows a path, waits for "hunter" to come close); NPC follows the player; timed objective; collect/pick up/use an item; track (footprints, blood); defend a position/waves; climb/rope/vehicle; QTE/struggle/melee; sniping a target; cutscene-gated progress; checkpoints/restart; player-death fail conditions; NPC-death fail conditions; anything else. For each: the natives/script calls that implement it, the maps that use it, and the co-op behaviour under the model above: `OK` / `RISK` / `BROKEN (expected)` with a one-line reason. Pay most attention to anything whose decision reads `"hunter"` (distance, line of sight, "player near NPC", "player looking at") — that is decided on the host with the host's hunter only.
3. **Per map.** For each story map: objectives in order (task names / trigger names), which mechanic rows they use, the NPCs present (object name, model, script, brain class) and what they do (guide, follow, escort target, scripted only), and the risk rating.
4. **Top risks.** The 5–10 most likely co-op breakages, ranked, each with a concrete proposed fix direction for the lead (one or two sentences, marked as a proposal).
5. **Test checklist for the user.** For the 6–8 maps that cover the most untested mechanic types (guide/escort NPC first): the command `.\run-coop-local.ps1 -Map <MAP>`, what to do in the host window and the client window, what to watch for, and the log lines (`story:`, `events:`, `animals:`) that show success or the failure. Order it so the riskiest mechanics are tested first.
6. `## Proposed MEMORY facts` with evidence (file path and line).

Keep a `## Progress` line at the top and fill sections as you go.

## Rules
- Edit only `re/notes/story-objectives.md` (and a helper under `re\scripts\` if you need one). No game launches, no Ghidra, no `mod/src` changes, no commits.
