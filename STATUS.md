# STATUS — resume here

**Last updated:** 2026-10-16 11:35 (America/Chicago, system clock) by the Claude lead session (handoff at user's session limit).

New session? Follow **Resume protocol** at the bottom. This file is the single source of truth for *where we are*. `PLAN.md` = roadmap, `MEMORY.md` = verified engine facts, `docs/mod-learnings.md` = mod/build/tooling learnings, `re/notes/*.md` = per-topic detail.

## Current phase
**Phase 2 done. Phase 3 (ghost mode) in progress — puppet rendering SOLVED (~11:45).**
Two instances connect, exchange PlayerState at 20 Hz, show each other in the F8 overlay. Puppets are now visible: the player model is first-person arms only, so puppets use an NPC body (`[Puppets] Model=objects/npc/mountainguide/mountainguide.skin,game.ap`). Next: two-instance ghost test (each window shows the other player's puppet moving).

## In flight
Nothing running. `re/jobs/p3-puppet-visibility.md` is **obsolete** (solved) — do not run it.

## Latest result (~11:50)
Two-instance run (`run-coop-local.ps1 -Seconds 120`): **both instances spawned a puppet for the other player** (`puppets: spawned puppet (remote 1)` on host, `(remote 0)` on client) and switched animations idle → walk/forward → run/forward from the remote player's networked speed. No crashes. Visual confirmation from the user pending.

## Next steps (in order)
1. **Two-instance ghost test (logs ✔, visual pending):** `.\run-coop-local.ps1 -Seconds 120` with `[Puppets] Spike=0` (default in `mod/dh2coop.ini`); each `dh2coop.log` should show `puppets: spawned puppet (remote N)`; user confirms each window shows the other player's NPC body moving. Then tune: puppet y offset (hunter pos y is eye/feet-relative?), anim names for NPC GAME.AP (`walk/forward`, `run/forward`; "idle" name unverified), facing (`SetAngle` yaw; check it matches movement).
2. Remove/guard the Draw diagnostic hook (only installed with `Spike=1`).
3. Puppet animation quality: idle/walk/run via `SetAnim` (vt `+0x174`) from interpolated speed (already wired; verify visually).
4. Phase 4 prep: animals — freeze brains on clients (`FreezeBrain`), host broadcasts transforms + body-command bits (`cAIHuntBrain_Animal::GetBodyCommands/SetBodyCommand`).

## Done
- **Phase 3 (partial, lead, 10:40–11:31):** puppet spawn recipe found and implemented (`mod/src/engine/entity_api.*`, `mod/src/coop/puppets.cpp`): `CreateEntity("obj_humanEx")` → `HandleKey("model_name")` → manager `AddEntity` → `Precache` → `Spawn` → per-frame `SetPos`/`SetAngle` + world AABB + `cBspTree::UpdateEntity`. Engine class list dumped (`game\Bin\dh2coop-classes.txt`, via `cObjManager::DumpClassesKeys`). Hunter model = `Objects/Characters/Teenm/Body/teenmm.skin,game.ap,HUNTMAP4.ap`. Details: `re/notes/lead-puppets.md`.
- **P2b overlay (Grok, reviewed 11:06):** F8 ImGui overlay on D3D8 (session, positions, remotes, RTT). Screenshots `re/notes/p2b-overlay*.png`.
- **Hunter transform (user walk test 10:24):** `+0xE8` world pos, `+0xF4.y` yaw degrees (atan2(dx,dz)).
- **Phase 2 core (10:20):** host/client PlayerState exchange; console commands `coop_status/host/join/leave`; tracker; slow-mo block; background running; OnFrame gated to the main thread.
- **Phase 1** (Route E) and **Phase 0** — see MEMORY.md.

## Blockers / risks
- Puppet not rendered (see above). Free-hunt start has a "CHOOSE ITEM" menu that blocks automated screenshots (synthetic input doesn't reach DirectInput) — visual checks need the user or a menu-closing console command.
- Claude usage limits — keep Claude to expert work + review.

## Recent results (newest first)
- 11:32: user: puppet not visible in any direction; collision felt.
- 11:30: puppet BSP-linked (`+0x124` valid), AABB follows it, `+0x08&8` set; kind byte `+0xA7` experiment (4) no visible change.
- 11:10: puppet spawns without crash (index 127); user collides with it.
- 11:06: overlay reviewed and merged.

## Resume protocol (new session)
1. Open the session in `D:\scary hunt\dh2coop` (loads `CLAUDE.md`; REA MCP is registered for this folder only).
2. Read this file, then `PLAN.md` decisions, then skim `MEMORY.md`, then `re/notes/lead-puppets.md` (current problem).
3. Check jobs: `Get-Process grok`, and for each in-flight log look for the `=== end` line. A job with no end line and no `grok` process was killed — rerun it: `.\tools-src\grokjob.ps1 -Name <job> -Effort <effort>`.
4. `git status` / `git log -5` to see uncommitted work since the last checkpoint.
5. Build: `.\mod\build.ps1`; solo test: `.\run-game.ps1 -ExtraArgs '-nointro -developper -cheats -spmap HUNTMAP4'` (no `-Seconds` when the user plays); two-instance test: `.\run-coop-local.ps1 -Seconds 100`; logs in `game\Bin\dh2coop.log`.
6. Continue with **Next steps** above. Update this file after every step and commit (timestamps from `Get-Date`). Edit docs with the Write/Edit tools, not PowerShell `Get-Content`/`Set-Content`.
