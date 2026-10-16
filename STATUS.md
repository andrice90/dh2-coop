# STATUS — resume here

**Last updated:** 2026-10-16 10:30 (America/Chicago) by the Claude lead session.

New session? Follow **Resume protocol** at the bottom. This file is the single source of truth for *where we are*. `PLAN.md` = roadmap, `MEMORY.md` = verified engine facts, `docs/mod-learnings.md` = mod/build/tooling learnings, `re/notes/*.md` = per-topic detail.

## Current phase
**Phase 2 — Mod skeleton** (Route E: custom ENet sync layer). Phase 0 and Phase 1 done.

## In flight
| Job | Started | Owner | Log | Deliverable |
|---|---|---|---|---|
| p2a-core (xhigh) | 10:30 | Grok 4.7, slot E, **owns game launches** | `re/jobs/p2a-core.out.log` | module system, D3D8 Present frame hook, console API + `coop_*` commands, multi-instance mutex hook, `run-coop-local.ps1`, slow-mo off, player tracker; `docs/mod-architecture.md`, `re/notes/p2a-runtime.md` |
| p2c-net (high) | 10:30 | Grok 4.7, no game, no Ghidra | `re/jobs/p2c-net.out.log` | `mod/src/net/**` (Session, protocol v1, interpolation), `mod/net-test/` (`dh2net_test.exe`), `docs/net-protocol.md`, `re/notes/p2c-net.md` |

File ownership is split so they can't collide: p2c owns only `mod/src/net/**` + `mod/net-test/**`; p2a owns the rest of `mod/` (CMake globs `src/**/*.cpp`). A job is finished when its log ends with `=== <name> end ... exit=N`.

## Next steps (in order)
1. Review p2a and p2c when done: build from clean, run `dh2net_test.exe`, read runtime proofs, spot-check claims; merge Proposed MEMORY facts; learnings → `docs/mod-learnings.md`; commit.
2. **P2b overlay** (after p2a): Dear ImGui on D3D8 (custom DX8 render backend or d3d8to9-style wrap) using p2a's device/Present hook; F8 toggle; status panel (role, peers, RTT) + debug panel (local/remote player positions).
3. **P2d integration** (after p2a + p2c): wire `coop::RequestHost/RequestJoin` to `net::Session`, send local `PlayerState` at 20 Hz from the player tracker, receive remote states; prove with `run-coop-local.ps1` that each instance logs the other's position. = **Phase 2 exit.**
4. Phase 3 ghost mode: spawn puppet (`cScrPlayer`/`cObjHumanEx`, null brain, `SetPos`/`SetAngle`) driven by interpolated remote state.

## Done
- **Phase 1 (2026-10-16)** — all six tracks reviewed and merged into `MEMORY.md`:
  - A net layer → Route E (engine netcode is dead code). `re/notes/net-layer.md`
  - B object model → positions `+0xE8`, health `+0x19C`, per-animal `FreezeBrain` (+0x79), puppet plan. `re/notes/object-model.md`, `mod/src/game/types_draft.h`
  - C assets → archive format, loose-file override, maps, `AddAnimal` scripts. `re/notes/assets.md`
  - D loop/console → variable dt + `sv_fps` cap, console API (`AddStaticFuncf`), console unlock, slow-mo via `mod_slowmotionFrameTime`. `re/notes/timing.md`, `re/notes/console.md`
  - E runtime → D3D8, `-spmap` direct map load, windowed via loose cfg, no UDP in any MP experiment. `re/notes/renderer.md`
  - F catalog/strings. `re/notes/class-catalog.md`, `re/notes/strings.md`
- Phase 0 (2026-10-15): tools, sandbox `game\`, ASI route proven.

## Blockers / risks
- None blocking. Watch: Claude usage limits (keep Claude to orchestration + review); two instances on one PC (DirectInput exclusive focus, windowed mode via loose cfg).

## Recent results (newest first)
- 2026-10-16 10:30: Phase 2 started: p2a-core + p2c-net launched on Grok.
- 2026-10-16 10:25: Track D done → console API, console password, variable-dt loop, slow-mo cvar. Phase 1 complete.
- 2026-10-16 10:10: Track B done → per-animal AI freeze, entity offsets.
- 2026-10-16 09:55: Track E done → `-spmap` dev loop, windowed via loose cfg.
- 2026-10-16 09:40: Track C done → loose files override `data.arc`; test maps HUNTMAP4 / MAPARENA.
- 2026-10-16 09:20: Route E decided.
- 2026-10-16 09:05: Grok runner stderr bug fixed.

## Resume protocol (new session)
1. Open the session in `D:\scary hunt\dh2coop` (loads `CLAUDE.md`; REA MCP is registered for this folder only).
2. Read this file, then `PLAN.md` decisions, then skim `MEMORY.md`.
3. Check jobs: `Get-Process grok`, and for each in-flight log look for the `=== end` line. A job with no end line and no `grok` process was killed — rerun it: `.\tools-src\grokjob.ps1 -Name <job> -Effort <effort>` (deliverables it already wrote stay; tell it to continue from them by editing its prompt if needed).
4. `git status` / `git log -5` to see uncommitted work since the last checkpoint.
5. Continue with **Next steps** above. Update this file after every step and commit. Edit docs with the Write/Edit tools, not PowerShell `Get-Content`/`Set-Content` (encoding mangling, see mod-learnings).
