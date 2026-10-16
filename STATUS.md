# STATUS — resume here

**Last updated:** 2026-10-16 09:25 (America/Chicago) by the Claude lead session.

New session? Follow **Resume protocol** at the bottom. This file is the single source of truth for *where we are*. `PLAN.md` = roadmap, `MEMORY.md` = verified engine facts, `docs/mod-learnings.md` = mod/build/tooling learnings, `re/notes/*.md` = per-topic detail.

## Current phase
**Phase 1 — Recon spikes**, wave 1b (Grok 4.7 workers). Phase 0 done. **Route decision made: Route E** (custom ENet sync layer; see PLAN.md → Decisions 2026-10-16).

## In flight
| Job | Started | Owner | Log | Deliverable |
|---|---|---|---|---|
| track-b-objmodel (restart #1) | 09:04 | Grok 4.7, slot B | `re/jobs/track-b-objmodel.out.log` | `re/notes/object-model.md`, `mod/src/game/types_draft.h` |
| track-c-assets | 08:39 (old runner) | Grok 4.7, slot C | `re/jobs/track-c-assets.out.log` | `re/notes/assets.md` |
| track-d-loop (restart #1) | 09:10 | Grok 4.7, slot D | `re/jobs/track-d-loop.out.log` | `re/notes/timing.md`, `re/notes/console.md` |
| track-e-runtime | 08:39 (old runner) | Grok 4.7, slot E, **owns game launches** | `re/jobs/track-e-runtime.out.log` | `re/notes/renderer.md` (experiment captures already in `re/notes/runtime-*.txt`) |

A job is finished when its log ends with `=== <name> end ... exit=N`. Plain-mode output only appears at the end, so a log with just the start line means "running or killed".

## Done this phase
- **Track A (net layer)** — `re/notes/net-layer.md` complete (job process died afterwards on the stderr bug, deliverable intact). Reviewed: import/WSAStartup/export-RVA claims re-checked with pefile. Facts merged into `MEMORY.md` → "Net layer". Verdict Route E accepted.
- **Track F (catalog/strings)** — `re/notes/class-catalog.md`, `re/notes/strings.md`.

## Next steps (in order)
1. As each remaining job finishes: review (spot-check 2–3 claims), merge `## Proposed MEMORY facts` into `MEMORY.md`, add learnings to `docs/mod-learnings.md`, commit. Track E: if it dies before writing `renderer.md`, the captures in `re/notes/runtime-*.txt` already cover the MP experiments — rerun only for the write-up.
2. If a deliverable is weak: follow-up job `re/jobs/<track>-2.md` naming the gaps.
3. Pick the MVP free-hunt test map (from assets.md).
4. Write Phase 2 job specs (Route E): ASI core (frame hook, GameDll7 late-load hooks), single-instance mutex bypass for 2 local instances, D3D8 overlay (ImGui via d3d8to9 or DX8 backend) with host/join panel, ENet transport with heartbeat + local player state packets, console commands `coop_host`/`coop_join` via the engine console registration API (from console.md).
5. Phase 3 (ghost mode) depends on Track B: player transform offsets + puppet spawn strategy.

## Blockers / risks
- **C and E still run under the old runner** (stderr bug): they die the first time Grok auto-compacts. If one ends with exit 1 and no deliverable, rerun with the fixed runner: `.\tools-src\grokjob.ps1 -Name <job> -Effort high`.
- Watch: Claude usage limits (keep Claude work to review + orchestration); two game instances on one PC: mutex `"GI UNIQUE MUTEX"` blocks a 2nd process unless `-join`/`-dedicated` is passed (or we hook it).

## Recent results (newest first)
- 2026-10-16 09:20: **Route E decided.** Engine netcode is dead code (no callers, no WSAStartup, no replication); SP "server" is in-process `cSpServer`. Runtime: no UDP port opens under `-dedicated`, `-join`, `sv_multiplayer 1`, `-map`.
- 2026-10-16 09:05: Grok runner stderr bug fixed (Start-Process redirection). B and D restarted.
- 2026-10-16: Worker layer switched to Grok Build CLI (`grok -m grok-4.7`, permission mode `auto`). Runner `tools-src/grokjob.ps1`.
- 2026-10-15 wave 1 (Claude subagents, cut off by usage limit): Track F finished; others left raw dumps, `re/scripts/arc_extract.py` (data.arc decoded, 9,710 files in `re/extracted/`), and a working runtime probe ASI (`mod/`).

## Resume protocol (new session)
1. Open the session in `D:\scary hunt\dh2coop` (loads `CLAUDE.md`; REA MCP is registered for this folder only).
2. Read this file, then `PLAN.md` decisions, then skim `MEMORY.md`.
3. Check jobs: `Get-Process grok`, and for each in-flight log look for the `=== end` line. A job with no end line and no `grok` process was killed — rerun it: `.\tools-src\grokjob.ps1 -Name <job> -Effort <effort>` (deliverables it already wrote stay; tell it to continue from them by editing its prompt if needed).
4. `git status` / `git log -5` to see uncommitted work since the last checkpoint.
5. Continue with **Next steps** above. Update this file after every step (move items between In flight / Done / Next steps / Recent results) and commit.
