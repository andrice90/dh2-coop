# STATUS — resume here

**Last updated:** 2026-10-16 13:50 (America/Chicago, system clock) by the Claude lead session.

New session? Follow **Resume protocol** at the bottom. This file is the single source of truth for *where we are*. `PLAN.md` = roadmap, `MEMORY.md` = verified engine facts, `docs/mod-learnings.md` = mod/build/tooling learnings, `re/notes/*.md` = per-topic detail.

## Current phase
**Phase 3 (ghost mode): core working (13:48), user-confirmed 13:53.** Two local instances connect. Each one shows the other player as an NPC body (mountain guide) that follows the remote position and facing, stands on the ground, and switches walk clips by movement direction. This is verified by an automated test with screenshots. Still open in Phase 3: confirm the walk cycle animates (not a frozen pose), foot-slide/anim speed, and a hands-on test by the user.

## In flight
Nothing. No Grok jobs running.

## Latest results (13:35–13:50, lead)
- **Grok deliverables reviewed and accepted:** p3-anim-names (`re/notes/anim-names.md`, `re/scripts/ap_list.py`) and p3-side-by-side (`run-coop-local.ps1` placement/capture, `re/notes/p3-side-by-side.md`). Facts merged into MEMORY.md.
  - Correction: standing idle is `idlenormalneutral`, not `idle`.
- **Background stall: not an engine throttle.**
  - Decompile shows that with `m_bAllowBackground=1`, focus loss only unacquires DirectInput.
  - `tools-src/focus-test.ps1` (13:37–13:40) ran both instances focused, unfocused, fully covered, minimized and restored. Both held avg_dt 0.010 s (~100 fps) and rx ≈ 19/s in every phase.
  - The earlier ~9 fps stall is most likely a window drag: the Win32 modal move loop blocks the message pump. UNVERIFIED. Windows are now auto-placed, so no dragging is needed.
- **Scripted input works:** `tools-src/game-input.ps1` sends scan-code `SendInput` to the foreground window, and DirectInput reads it. The CHOOSE ITEM menu closes with **Enter, Left, Enter** (user).
- **Automated ghost test:** `tools-src/move-test.ps1`.
- **Puppet stuck at spawn: FIXED.**
  - Cause: the physics body (`+0x118`) is not moved by `SetPos`, and the physics step snaps the entity back to it. The overlapping capsules also pushed the puppet onto the hunter's head (user: "stacked, seeing the feet").
  - Fix: `entity::UnlinkPhysics` (`cObjScriptedEntity::UnlinkWithPhysics(1)`) right after spawn (`[Puppets] Physics=0`, default).
  - After the fix: the puppet translates, faces correctly, and has its feet on the ground (hunter `+0xE8` = feet, `YOffset=0`). Clips switch W→`walk/forward`, S→`walk/backward`, A→`walk/left`, D→`walk/right`, stop→`idlenormalneutral`.
  - Screenshots: `re/notes/coop-local/move-134802-*`.
- **User confirmed at 13:53:** "it looks good to me, i saw the puppet move and strafe" (watching the automated two-instance run). My own frame bursts were inconclusive: the puppet was too far away, then the camera pitched to the sky in Spike mode. The leg cycle has not been checked frame by frame. The user's view is the acceptance.

## Next steps (in order)
1. **Walk cycle (low priority):** the user saw movement and strafing. If a later hands-on test shows a frozen pose, tick the animation ourselves: `cObjEntity::Update(dt)` calls model vt `+0x8c(dt)`.
2. **Anim speed:** the hunter walks ≈ 5 m/s, and the NPC walk clip is slower, so feet may slide. Option: `cBodyStateMachine::SetAnimSpeedFactor`, or `run/forward` above ~3 m/s. Judge from the user test.
3. **User hands-on test:** `.\run-coop-local.ps1`. It places both windows side by side and leaves them running.
   - Close the menu in each window (Enter, Left, Enter), then walk around in one window and watch the other.
   - Don't drag the windows.
4. Remove/guard the Draw diagnostic hook (only installed with `Spike=1`), and delete obsolete spike code once ghost mode is stable.
5. **Phase 4 prep (animals):**
   - Freeze brains on clients (`FreezeBrain`).
   - Host broadcasts transforms plus body-command bits (`cAIHuntBrain_Animal::GetBodyCommands/SetBodyCommand`).
   - Animal physics may need the same unlink treatment on clients (see the puppet physics lesson in MEMORY).
6. Spawn overlap: both players spawn on the same point. Puppets are now non-solid, so this is harmless. Later we could use HUNTMAP4's second spawn point for player 2.

## Done
- **Phase 3 core (lead, 13:35–13:48):**
  - Directional clip choice (`choose_clip` in `mod/src/coop/puppets.cpp`).
  - Puppet physics unlink.
  - Focus-stall investigation closed.
  - Automated test tooling: `tools-src/focus-test.ps1`, `game-input.ps1`, `move-test.ps1`, `capture-window.ps1 -NoFocus`.
- **Puppet rendering (11:45):** the NPC body is required, because the player model is first-person arms only.
- **Puppet spawn recipe (10:40–11:31):** `CreateEntity("obj_humanEx")` → `HandleKey("model_name")` → `AddEntity` → `Precache` → `Spawn` → (now) `UnlinkPhysics`. Then per frame: `SetPos`/`SetAngle`, world AABB, and `cBspTree::UpdateEntity`. Details: `re/notes/lead-puppets.md`.
- **P2b overlay** (F8), **hunter transform**, **Phase 2 core** (host/client PlayerState at 20 Hz, console commands, tracker, slow-mo block, background running), **Phase 1** (Route E), **Phase 0**: see MEMORY.md / PLAN.md.

## Blockers / risks
- Animation may be frozen (step 1).
- Claude usage limits: keep Claude on expert work and review; delegate mechanical jobs to Grok (`tools-src/grokjob.ps1`).

## Resume protocol (new session)
1. Open the session in `D:\scary hunt\dh2coop` (loads `CLAUDE.md`; the REA MCP is registered for this folder only).
2. Read this file, then the `PLAN.md` decisions, then skim `MEMORY.md`, then `re/notes/lead-puppets.md`.
3. Check jobs:
   - Run `Get-Process grok`.
   - For each in-flight log, look for the `=== end` line.
   - A job with no end line and no `grok` process was killed. Rerun it with `.\tools-src\grokjob.ps1 -Name <job> -Effort <effort>`.
4. Run `git status` / `git log -5` to see uncommitted work since the last checkpoint.
5. Build and test:
   - Build: `.\mod\build.ps1`.
   - Solo test: `.\run-game.ps1 -ExtraArgs '-nointro -developper -cheats -spmap HUNTMAP4'` (no `-Seconds` when the user plays).
   - Automated two-instance tests: `.\tools-src\move-test.ps1` and `.\tools-src\focus-test.ps1`.
   - Logs: `game\Bin\dh2coop.log`, `game2\Bin\dh2coop.log`.
6. Continue with **Next steps** above. Update this file after every step and commit (timestamps from `Get-Date`). Edit docs with the Write/Edit tools, not PowerShell `Get-Content`/`Set-Content`.
