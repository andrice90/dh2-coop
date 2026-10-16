# STATUS — resume here

**Last updated:** 2026-10-16 08:45 (America/Chicago) by the Claude lead session.

New session? Follow **Resume protocol** at the bottom. This file is the single source of truth for *where we are*. `PLAN.md` = roadmap, `MEMORY.md` = verified engine facts, `docs/mod-learnings.md` = mod/build/tooling learnings, `re/notes/*.md` = per-topic detail.

## Current phase
**Phase 1 — Recon spikes**, wave 1b (Grok 4.7 workers). Phase 0 done (tools, sandbox, ASI route proven).

## In flight
| Job | Started | Owner | Log | Deliverable |
|---|---|---|---|---|
| track-a-net (xhigh) | 2026-10-16 08:39 | Grok 4.7, slot A | `re/jobs/track-a-net.out.log` | `re/notes/net-layer.md` |
| track-b-objmodel | 08:39 | Grok 4.7, slot B | `re/jobs/track-b-objmodel.out.log` | `re/notes/object-model.md`, `mod/src/game/types_draft.h` |
| track-c-assets | 08:39 | Grok 4.7, slot C | `re/jobs/track-c-assets.out.log` | `re/notes/assets.md` |
| track-d-loop | 08:39 | Grok 4.7, slot D | `re/jobs/track-d-loop.out.log` | `re/notes/timing.md`, `re/notes/console.md` |
| track-e-runtime | 08:39 | Grok 4.7, slot E, **owns game launches** | `re/jobs/track-e-runtime.out.log` | `re/notes/renderer.md`, `re/notes/runtime-*.txt` |

A job is finished when its log ends with `=== <name> end ... exit=N`. Plain-mode output only appears at the end, so a log with just the start line means "running or killed".

## Next steps (in order)
1. As each job finishes: review its deliverable against the evidence (spot-check 2–3 claims in Ghidra/logs), merge its `## Proposed MEMORY facts` into `MEMORY.md`, add mod/tooling learnings to `docs/mod-learnings.md`, commit.
2. If a deliverable is weak: write a follow-up job `re/jobs/<track>-2.md` that names the gaps; rerun.
3. **Route decision** (Phase 1 exit gate) from net-layer.md + runtime MP experiments: Route N (revive engine netcode: `cRdpDriver`, `sv_multiplayer`, `-join`) vs Route E (custom ENet) vs hybrid. Record in `PLAN.md` → Decisions.
4. Write Phase 2 job specs (mod skeleton: overlay on D3D8 Present at `EngineDll7+0x354cfd` caller, console commands `coop_host`/`coop_join`, transport per route).
5. Pick MVP free-hunt test map (from assets.md).

## Blockers / risks
- None blocking. Watch: Claude usage limits (keep Claude work to review + orchestration); two game instances on one PC may conflict (disc mount, DirectInput exclusive).

## Recent results (newest first)
- 2026-10-16: Worker layer switched to Grok Build CLI (`grok -m grok-4.7`, permission mode `auto`; `bypassPermissions` is blocked by Claude Code's classifier). Runner `tools-src/grokjob.ps1`.
- 2026-10-15 wave 1 (Claude subagents, cut off by usage limit): Haiku Track F finished (class catalog, strings, demangled exports). Others left raw dumps in `re/notes/*.c|txt`, `re/scripts/arc_extract.py` (data.arc fully decoded, 9,710 files in `re/extracted/`), and a working runtime probe ASI (`mod/`).
- 2026-10-15: Strings show a retail MP surface (`sv_multiplayer`, `cl_join`, `-join`, `-dedicated`, `cRdpDriver`). Probe: renderer is D3D8; `m_nDriverType=0` in normal play; `GetListenPort` (only `sv_port` reader) has no callers.

## Resume protocol (new session)
1. Open the session in `D:\scary hunt\dh2coop` (loads `CLAUDE.md`; REA MCP is registered for this folder only).
2. Read this file, then `PLAN.md` decisions, then skim `MEMORY.md`.
3. Check jobs: `Get-Process grok`, and for each in-flight log look for the `=== end` line. A job with no end line and no `grok` process was killed — rerun it: `.\tools-src\grokjob.ps1 -Name <job> -Effort <effort>` (deliverables it already wrote stay; tell it to continue from them by editing its prompt if needed).
4. `git status` / `git log -5` to see uncommitted work since the last checkpoint.
5. Continue with **Next steps** above. Update this file after every step (move items between In flight / Recent results / Next steps) and commit.
