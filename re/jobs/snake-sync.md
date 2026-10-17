# Job: snake-sync — mission cobras shared like the other animals (C++; git worktree; game launches when free)

## Goal (user decision 2026-10-17)
One cobra for everyone: anyone can kill it, the kill counts for everyone (MAP7 task 6 "kill 1 cobra"; MAP3B has 3, MAP3C 4). Read `re/notes/snake.md` first (research; the recipe below is its `## Proposed change`), then `mod/src/coop/animals.cpp` (host snapshot `HostTick`, client `ApplyMirror`, `OnAnimalHit`, `PresentDeathOnce`, `kAnimalDied`), `player_tracker.cpp` (`Animals()` is built from the `cScrAnimal` vtable `GameDll7+0xff470`), `combat.cpp` (`RetargetToNearestPlayer`), `CLAUDE.md` (hook/RVA rules).

## Where you work
Git worktree `D:\scary hunt\dh2coop-snake`, branch `snake-sync` (the lead creates it). Build with `.\mod\build.ps1` there. Game copies: `robocopy "D:\scary hunt\dh2coop\game" "D:\scary hunt\dh2coop-snake\game" /MIR /XF *.log dh2coop.cmd /R:1 /W:1 /NFL /NDL /NP`, same into `game2`. Never write to `D:\scary hunt\dh2coop\` or `D:\scary hunt\Activision Value\`. Before launching check `Get-Process Main7`; wait (poll 30 s, up to 30 min) if any runs; close only your own games. Note: `tools-src\game-input.ps1` now refuses to send keys unless the game window is in front; prefer console commands (`dh2coop.cmd`).

## Build
1. Tracker: `cObjCobra` (vtable `GameDll7+0x10e920`) counts as an animal next to `cScrAnimal`. Never `cObjSnake` (`+0x10e470`; its `+0x200` is a float).
2. Client mirror, cobras only: after the brain freeze, clear `+0x728` (the bite target) so the local cobra never bites or starts its own death timer; apply host body-state names (`Rise`, `Idle`, `Attack`, `TurnLeft`, `TurnRight`, `Death`), never `Dead`.
3. Death: the host's cobra counts its own kill (`+0x154` = `FUN_10080320` zeroes `+0x19C` and counts `ANIMAL_COBRA`). Clients call that slot exactly once per cobra per process (existing `PresentDeathOnce` path; make sure nothing else calls it, and no `ClaimCallback`).
4. A client's hit on a cobra (`AnimalHit` on the host): not the health-subtract path. Do what the cobra's own bullet `OnHit` does: body state `Death` and `+0x738 = 50.0` (verify both in the research dumps), and let the host's `Update` call `+0x154` once.
5. Optional, only if 1-4 pass: on the host, point a cobra's `+0x728` at the nearest living player (`puppets::NearestLivingPuppet`, or the host hunter), so it can bite clients; a bite on a puppet must reach that player (check which hurt entry it uses: `combat::ForwardPuppetHurt` covers `cObjScriptedEntity::OnHurt` and `cObjCharacter::OnHurt`). Log what happens; leave it behind `[Animals] CobraBitesNearest=1` (default 0) if unsure.

## Test (record in `re/notes/snake-sync.md`, quotes from both logs, screenshots)
Two windows on MAP3B (`run-coop-local.ps1 -Map MAP3B` in your worktree). Host `coop_trigger cScrTask01` (arm/fire) if the cobras need it; find cobra ids in the host log (`animals: sync role=host id=` rows; add a one-time log of cobra ids at map load). Check: one cobra per id on the client (`mirror freeze id=<cobra>`), positions match (`drift`), client `coop_kill <cobra>:10`: host shows `Death` then the kill counted once; client shows the death once; no mission fail. Then the host kills one: same. Screenshots of a cobra near a player in both windows (`coop_tp` the hunters next to it).

## Deliverables
Code in the worktree, `re/notes/snake-sync.md`. Commit on the branch (author `andri <andrice90@gmail.com>`, trailer `Co-Authored-By: Grok 4.7 <noreply@x.ai>`). Do not merge.
