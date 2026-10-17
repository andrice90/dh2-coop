# Job: props-sync — shootable world props (bottles, stumps: `cObjTarget`) shared by every player (C++; git worktree; game launches only when free)

## Goal (user decision 2026-10-17)
A client's shot on a stump, bottle or other shootable target propagates to every session, and items tied to a goal count toward that goal for everyone. Read `re/notes/props-research.md` first (your colleague's research; RVAs and the design below come from it). Also `mod/src/coop/events.h` / `events.cpp` (request/commit envelope, journal, late-join replay), `story.cpp` (how `kTriggerFire` requests and commits are done, `CheckedGameRva`, guarded calls), `CLAUDE.md` (hook rules: check the `55 8b ec 6a ff` prologue before hooking any RVA).

## Where you work
Git worktree `D:\scary hunt\dh2coop-props`, branch `props-sync` (the lead creates it). Build there with `.\mod\build.ps1` (installs into that worktree's `game\Bin`). First-time game copy: `robocopy "D:\scary hunt\dh2coop\game" "D:\scary hunt\dh2coop-props\game" /MIR /XF *.log dh2coop.cmd /R:1 /W:1 /NFL /NDL /NP`, and the same into `game2`. Use the worktree's own `run-coop-local.ps1`. Never write to `D:\scary hunt\dh2coop\` or `D:\scary hunt\Activision Value\`. Before launching check `Get-Process Main7`; wait (poll 30 s, up to 30 min) if any runs; close only your own games.

## Design (implement this)
GameDll7 v1.0: `cObjTarget` vtable `0x1017c8`; `OnHit` `0x277f0` `__thiscall(this, cObjEntity* other)` (vt `+0x8c`); `Boom` `0x27040` `__thiscall(this)` (vt `+0x188`); broken flag `+0x208` (0/1); stump when `+0x21c == 1`; `UpdateMissionStatus` `0x8390` `__cdecl(int action, int value)`, the bottle count is `(1, 0x25)` and is only called when `DAT_101657f0 != 0` (read that global the same way `OnHit` does); stump tally = profile `+0x90` (find in `re/notes/props-target.c` exactly which profile pointer `OnHit` increments and how it reaches it; use the same one for the local player).
1. New event kind `kPropHit = 16` in `events.h` (request and commit; subject = the target's entity id, `i0` = 1 for a stump, 0 otherwise). Not in `events::Send`: it is journaled (`events::Commit`), so a late joiner gets the broken props in its catch-up.
2. New module `mod/src/coop/props.cpp`. Hook `OnHit` (prologue check first).
   - Offline, or `g_in_apply`: original.
   - **Host**: remember `+0x208`, run the original; if it went 0 → 1, `Commit(kPropHit, id, stump ? 1 : 0, 0.f, "", 0)`.
   - **Client**: if `other` is a `mod_obj_bullet` (the same check `OnHit` makes), do not run the original; `Request(kPropHit, id, ...)` once per entity (ignore repeats while waiting). Anything else: original.
   - **Host on request**: `EntityById(id)`, check its vtable is `cObjTarget`; if `+0x208 == 0`: `Boom` (guarded), set `+0x208 = 1`, bottle: `UpdateMissionStatus(1, 0x25)` once (guarded, only if the global is set), stump: tally `+0x90` += 1 on the host's profile; then commit with the requester as actor.
   - **Every client on commit**: if `+0x208 == 0`: `Boom`, set `+0x208 = 1`; stump: tally += 1 on its own profile. Never call `UpdateMissionStatus` on a client.
   - Map change: clear per-map state in `OnMapLoaded`.
3. Test command `coop_prophit <id>`: runs the client path as if the local bullet hit that target (request on a client, the host path on the host). At map load, log every `cObjTarget` (id, position, stump or bottle) once, so a tester can pick ids.
4. **Tests** (record in `re/notes/props-sync.md`, quotes from both logs, screenshots `re/notes/coop-local/props-*.png`):
   - Two windows on MAP1 (`run-coop-local.ps1 -Map MAP1` in your worktree). From the map-load list take two bottle ids and the stump id.
   - Client `coop_prophit <bottle>`: both windows show the bottle broken (screenshot both, standing near it: use `coop_tp <x>:<y>:<z>` to put both hunters 3 m from it), host log shows the mission count once (`events: commit kind=16`; add a log line in your request handler for `UpdateMissionStatus`).
   - Host `coop_prophit <bottle 2>`: client breaks it too, no count on the client.
   - Client `coop_prophit <stump>`: both tallies +1 (log the tally values).
   - Repeat a hit on an already broken one: nothing happens twice.
   - Late join: `run-coop-local.ps1 -AddClient 3 -Map MAP1` after the breaks: game3 shows them broken after its catch-up.

## Deliverables
Code in the worktree (`mod/src/coop/props.cpp`, `events.h`, any small helper), `re/notes/props-sync.md`. Commit on the branch (author `andri <andrice90@gmail.com>`, trailer `Co-Authored-By: Grok 4.7 <noreply@x.ai>`). Do not merge.
