# STATUS — resume here

**Last updated:** 2026-10-16 15:25 (America/Chicago, system clock) by the Claude lead session.

New session? Follow **Resume protocol** at the bottom. This file is the single source of truth for *where we are*. `PLAN.md` = roadmap, `MEMORY.md` = verified engine facts, `docs/mod-learnings.md` = mod/build/tooling learnings, `re/notes/*.md` = per-topic detail.

## Current phase
**Phase 3 (ghost mode): DONE, user-confirmed 15:30** ("This is it! they look great, animations, gun model alignment, muzzle flash"). Two local instances connect. Each one shows the other player as an NPC body (mountain guide) that:
- follows the remote position and **view yaw** (also while standing still);
- stands on the ground;
- plays walk/strafe/back/run clips by movement direction;
- holds the remote player's **weapon model** in hand;
- plays the armed idle/walk clips, **fire per shot**, **reload**, and a crouch pose.

User-confirmed: movement, rotation, speed, armed poses, fire and reload, weapon-in-hand alignment and the muzzle flash. Not yet seen by the user: shotgun/rifle (left hand) and the crouch pose.

## In flight
Nothing. No Grok jobs running. Two game windows may be open for the user's weapon check.

## Latest results (14:30–15:22)
- **Actions networked** (`7ebea6f`):
  - Tracker polls per frame: weapon state `Fire`/`Reload` via `cObjItemEx::GetCurrentState`, crouch from eye height (< 1.40 m), and weapon kind from the active item's model name.
  - `PlayerState.flags` = crouched/reloading/firing plus a 4-bit shot counter; `weaponId` = kind.
  - Puppet clips: `handgun3/*` or `shotgun1/*` (the rifle uses shotgun), a fire clip per shot (`FireMs`), reload, and crouch (`CrouchClip`, default `coveridle`).
  - User: armed walk/idle, fire and reload look good.
- **Weapon model in hand** (`e69eabb`):
  - A third-person `objects/items/game/weapons/<name>/<name>.skin` entity, `Attach`ed to `BipRHand` (handgun) or `BipLHand` (long guns), then `LinkPart`. This mirrors the NPC weapon scripts.
  - First version: the gun pointed sideways and the muzzle flash was always on (user). Cause: no clip on the weapon. Fix: mirror the body clip onto the weapon with the prefix stripped.
  - Screenshots `re/notes/coop-local/wpn-*.png`: the handgun points forward and recoils on fire.
- **Scripted LMB/RMB** in `tools-src/game-input.ps1`. A scripted fire test passed (2 shots → 2 remote fire clips).
- **Earlier today (13:35–14:20):**
  - Grok deliverables reviewed.
  - Focus-stall investigation closed: no engine throttle; the stall was likely a window drag.
  - Puppet physics unlinked, which fixed "stuck at spawn".
  - View yaw comes from the render camera (`cRender::SetCamera` hook).
  - Details in MEMORY.md and `re/notes/lead-puppets.md`.

## Next steps (in order)
1. **Leftover Phase 3 checks (low priority):**
   - Try the shotgun and the rifle (left hand), and give a verdict on the crouch pose (`coveridle`; alternatives `cover`, `crouchidlecar`, `hurtsit` via `[Puppets] CrouchClip`).
2. **Crouch key unknown.** Ask the user, then add it to scripted tests.
3. **Polish (optional):**
   - Feet sliding (anim speed factor).
   - Pitch on the puppet (aim up/down).
   - Weapon sounds on remote shots (the weapon `.model` has `Sound*Fire` bones; check whether the fire clip already plays them).
4. **Cleanup:**
   - Remove/guard the Draw diagnostic hook and spike code.
   - `[Tracker] DiffHunter` diagnostics stay (default off).
5. **Phase 4 (animals):**
   - Freeze brains on clients (`FreezeBrain`).
   - Host broadcasts animal transforms plus body-command bits (`cAIHuntBrain_Animal::GetBodyCommands/SetBodyCommand`).
   - Animal physics on clients probably needs the unlink treatment (puppet lesson).
   - Shots from clients must hit host animals.
6. Spawn overlap: both players spawn on the same point. Puppets are non-solid, so this is harmless. Option: HUNTMAP4's second spawn point.

## Done
- **Phase 3:**
  - Puppet spawn recipe + physics unlink.
  - NPC body.
  - Directional clips.
  - Render-camera view yaw/pitch.
  - Action flags.
  - Armed clip sets.
  - Weapon model attach.
  - Automated test tooling: `tools-src/game-input.ps1` (keys, mouse moves, mouse buttons), `move-test.ps1`, `focus-test.ps1`, `capture-window.ps1 -NoFocus`.
- **P2b overlay** (F8), **hunter transform**, **Phase 2 core** (host/client PlayerState at 20 Hz, console commands, tracker, slow-mo block, background running), **Phase 1** (Route E), **Phase 0**: see MEMORY.md / PLAN.md.

## Blockers / risks
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
   - Build: `.\mod\build.ps1` (fails while `Main7.exe` runs).
   - Solo test: `.\run-game.ps1 -ExtraArgs '-nointro -developper -cheats -spmap HUNTMAP4'` (no `-Seconds` when the user plays).
   - Two windows for the user: `.\run-coop-local.ps1`. Close the CHOOSE ITEM menu with Enter, Left, Enter.
   - Automated: `.\tools-src\move-test.ps1`, `.\tools-src\focus-test.ps1`.
   - Logs: `game\Bin\dh2coop.log`, `game2\Bin\dh2coop.log`.
6. Continue with **Next steps** above. Update this file after every step and commit (timestamps from `Get-Date`). Edit docs with the Write/Edit tools, not PowerShell `Get-Content`/`Set-Content`.
