# STATUS — resume here

**Last updated:** 2026-10-16 09:55 (America/Chicago) by the Claude lead session.

New session? Follow **Resume protocol** at the bottom. This file is the single source of truth for *where we are*. `PLAN.md` = roadmap, `MEMORY.md` = verified engine facts, `docs/mod-learnings.md` = mod/build/tooling learnings, `re/notes/*.md` = per-topic detail.

## Current phase
**Phase 1 — Recon spikes**, wave 1b (Grok 4.7 workers). Phase 0 done. **Route decision made: Route E** (custom ENet sync layer; see PLAN.md → Decisions 2026-10-16).

## In flight
| Job | Started | Owner | Log | Deliverable |
|---|---|---|---|---|
| track-b-objmodel (restart #1) | 09:04 | Grok 4.7, slot B | `re/jobs/track-b-objmodel.out.log` | `re/notes/object-model.md`, `mod/src/game/types_draft.h` |
| track-d-loop (restart #1) | 09:10 | Grok 4.7, slot D | `re/jobs/track-d-loop.out.log` | `re/notes/timing.md`, `re/notes/console.md` |

Both run on the fixed runner. A job is finished when its log ends with `=== <name> end ... exit=N`; plain-mode output only appears at the end.

## Done this phase
- **Track A (net layer)** — `re/notes/net-layer.md`. Reviewed (imports/WSAStartup/export RVAs re-checked with pefile). Merged → `MEMORY.md` "Net layer". Verdict Route E accepted.
- **Track C (assets)** — `re/notes/assets.md`, `re/scripts/asset_inventory.py`. Reviewed (loose-cfg override confirmed at runtime; map files + `AddAnimal` script confirmed). Merged → "Assets". Test maps: HUNTMAP4 (Phases 3–4), MAPARENA (Phase 5).
- **Track E (runtime)** — `re/notes/renderer.md`, captures `re/notes/runtime-*.txt`, `re/scripts/runtime-exp.ps1`. Job process died on the stderr bug after writing the deliverable. Reviewed (`-spmap HUNTMAP1` direct load re-checked in logs). Merged → "Runtime probe" + `docs/mod-learnings.md` "Testing loop".
- **Track F (catalog/strings)** — `re/notes/class-catalog.md`, `re/notes/strings.md`.

## Next steps (in order)
1. Review B and D when they finish (spot-check 2–3 claims, merge Proposed MEMORY facts, add learnings, commit). Weak deliverable → follow-up job `re/jobs/<track>-2.md` naming the gaps.
2. Write Phase 2 job specs (Route E) and run them on Grok:
   - 2a ASI core: frame callback (D3D8 `Present` hook until a game-tick hook is known), GameDll7 late-load hook install, `"GI UNIQUE MUTEX"` bypass so two instances run, `cConsole::Exec`/`RealExec` helpers, register our own console commands (`coop_host`, `coop_join`) via the API from console.md.
   - 2b D3D8 overlay: Dear ImGui via d3d8to9-style wrapper or a DX8 backend; host/join panel + debug panel (local player position once Track B gives offsets).
   - 2c ENet transport: host/client, heartbeat, local player state packet; two-instance local test (windowed via loose `Cfg\console.cfg`, `-spmap HUNTMAP4`).
3. Phase 3 (ghost mode) after Track B: player transform offsets + puppet spawn strategy.
4. Optional, user: check the in-game console key by hand (steps in `re/notes/renderer.md` → Manual steps).

## Blockers / risks
- None blocking. Watch: Claude usage limits (keep Claude to orchestration + review); two instances on one PC (mutex, DirectInput exclusive, shared `debug.log` — use `game\` + `game2\` copies).

## Recent results (newest first)
- 2026-10-16 09:55: Track E done. `-spmap <MAP>` loads a hunt map directly (dev loop). Loose `Cfg\console.cfg` → windowed mode. No UDP socket in any MP experiment.
- 2026-10-16 09:40: Track C done. Loose files in `game\Data\` override `data.arc`; animals placed by level scripts (`WORLD.SCR` `AddAnimal`).
- 2026-10-16 09:20: **Route E decided.** Engine netcode is dead code; SP "server" is in-process `cSpServer`.
- 2026-10-16 09:05: Grok runner stderr bug fixed (Start-Process redirection). B and D restarted.
- 2026-10-16: Worker layer switched to Grok Build CLI (`grok -m grok-4.7`, permission mode `auto`).
- 2026-10-15 wave 1 (Claude subagents, cut off by usage limit): Track F finished; others left raw dumps, `arc_extract.py`, and the runtime probe ASI.

## Resume protocol (new session)
1. Open the session in `D:\scary hunt\dh2coop` (loads `CLAUDE.md`; REA MCP is registered for this folder only).
2. Read this file, then `PLAN.md` decisions, then skim `MEMORY.md`.
3. Check jobs: `Get-Process grok`, and for each in-flight log look for the `=== end` line. A job with no end line and no `grok` process was killed — rerun it: `.\tools-src\grokjob.ps1 -Name <job> -Effort <effort>` (deliverables it already wrote stay; tell it to continue from them by editing its prompt if needed).
4. `git status` / `git log -5` to see uncommitted work since the last checkpoint.
5. Continue with **Next steps** above. Update this file after every step and commit. Edit docs with the Write/Edit tools, not PowerShell `Get-Content`/`Set-Content` (encoding mangling, see mod-learnings).
