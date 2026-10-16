# STATUS — resume here

**Last updated:** 2026-10-16 09:40 (America/Chicago, from the system clock) by the Claude lead session.

New session? Follow **Resume protocol** at the bottom. This file is the single source of truth for *where we are*. `PLAN.md` = roadmap, `MEMORY.md` = verified engine facts, `docs/mod-learnings.md` = mod/build/tooling learnings, `re/notes/*.md` = per-topic detail. Times below come from job logs / `Get-Date` (earlier entries before 09:40 were estimated and ran ahead of the clock; corrected here).

## Current phase
**Phase 2 — Mod skeleton** (Route E: custom ENet sync layer). Phase 0 and Phase 1 done.

## In flight
| Job | Started | Owner | Log / notes | Deliverable |
|---|---|---|---|---|
| p2c-net (high) | 09:20 | Grok 4.7, no game, no Ghidra | `re/jobs/p2c-net.out.log` | `mod/src/net/**` (Session, protocol v1, interpolation), `mod/net-test/` (`dh2net_test.exe`), `docs/net-protocol.md`, `re/notes/p2c-net.md` |
| p2a-framework (high) | 09:34 | Grok 4.7, **owns game launches** | `re/jobs/p2a-framework.out.log` | scaffolding only: module system, probe→module, D3D8 Present `OnFrame`, basic `console::Exec`, multi-instance mutex hook, `run-coop-local.ps1`, crash dumps; `docs/mod-architecture.md`, `re/notes/p2a-runtime.md` |
| lead-expert (main thread) | 09:30 | Claude Opus (lead) | analysis done: `re/notes/lead-console-cmds.md`, `lead-slowmo.md`, `lead-player-tracker.md` | implement `mod/src/engine/console_cmds.*`, `mod/src/coop/{coop,slowmo,player_tracker}.*` on top of the framework once p2a lands; then runtime-verify |

File ownership: p2c → `mod/src/net/**`, `mod/net-test/**`; lead → `mod/src/engine/console_cmds.*`, `mod/src/coop/**`; p2a → rest of `mod/` (CMake globs `src/**/*.cpp`). A job is finished when its log ends with `=== <name> end ... exit=N`.

## Lead findings this phase (details in the notes)
- `cConsole::AddStaticFuncf` is **__cdecl** with `this` as first stack arg (mangled `QAA`; Ghidra mislabels it thiscall). Handlers are plain cdecl; string params arrive as `char*`.
- Slow-mo: `cHUDSlowMotion::Enable` (GameDll7 `0xca690`) → `SetFrameTime` (`0xca400`) → console `frametime`. Plan: detour `Enable` to no-op (signature-checked) + `frametime 1` on map load.
- Entity world angle tVect3 at `+0xF4` (pos `+0xE8`); manager captured via non-virtual `cObjManager::Get` hook (virtual `AddEntity` unreliable because Main7 replaces the manager vtable).

## Next steps (in order)
1. When p2a lands: review it (clean build, runtime proofs), then lead implements console commands, slow-mo hook, player tracker on its module API and verifies in a `-spmap HUNTMAP4` run (angle component order/units, vtable histogram).
2. When p2c lands: review (clean build of `mod/net-test`, run `dh2net_test.exe`, read protocol doc).
3. **P2b overlay** (Grok scaffold + lead review): Dear ImGui on D3D8 via the framework's device/Present hook; F8 toggle; status + debug panels.
4. **P2d integration** (lead): wire `coop::RequestHost/RequestJoin` → `net::Session`, 20 Hz `PlayerState` from the tracker, two-instance proof with `run-coop-local.ps1` = **Phase 2 exit**.
5. Phase 3 ghost mode (lead-designed): puppet spawn driven by interpolated remote state.

## Done
- **Phase 1** — six tracks reviewed and merged into `MEMORY.md`: A net layer → Route E; B object model; C assets; D loop/console; E runtime; F catalog/strings (see `re/notes/`).
- Phase 0 (2026-10-15): tools, sandbox `game\`, ASI route proven.

## Blockers / risks
- None blocking. Watch: Claude usage limits; two instances on one PC (DirectInput focus, windowed via loose cfg).

## Recent results (newest first)
- 09:30–09:40: lead analysis of console registration, slow-mo, player tracker (notes above).
- 09:29: **Delegation policy (user):** expert work → lead main thread / Opus subagents; Grok only for well-defined Sonnet-level work. p2a-core stopped before writing files; replaced by p2a-framework + lead-expert.
- 09:20: Phase 2 started (p2c-net).
- 09:17: Track D done. 09:15: Track B done. 08:56: Track E deliverable. 08:54: Track C done. 08:50: Track A deliverable → Route E.
- ~08:44: Grok runner stderr bug fixed; B and D restarted.

## Resume protocol (new session)
1. Open the session in `D:\scary hunt\dh2coop` (loads `CLAUDE.md`; REA MCP is registered for this folder only).
2. Read this file, then `PLAN.md` decisions, then skim `MEMORY.md`.
3. Check jobs: `Get-Process grok`, and for each in-flight log look for the `=== end` line. A job with no end line and no `grok` process was killed — rerun it: `.\tools-src\grokjob.ps1 -Name <job> -Effort <effort>` (deliverables it already wrote stay; tell it to continue from them by editing its prompt if needed).
4. `git status` / `git log -5` to see uncommitted work since the last checkpoint.
5. Continue with **Next steps** above. Update this file after every step and commit (use `Get-Date` for timestamps). Edit docs with the Write/Edit tools, not PowerShell `Get-Content`/`Set-Content` (encoding mangling, see mod-learnings).
