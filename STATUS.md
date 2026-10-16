# STATUS — resume here

**Last updated:** 2026-10-16 10:22 (America/Chicago, system clock) by the Claude lead session.

New session? Follow **Resume protocol** at the bottom. This file is the single source of truth for *where we are*. `PLAN.md` = roadmap, `MEMORY.md` = verified engine facts, `docs/mod-learnings.md` = mod/build/tooling learnings, `re/notes/*.md` = per-topic detail.

## Current phase
**Phase 2 — Mod skeleton: core exit criterion met (log-based).** Two instances on one PC connect over ENet and each logs the other's position at 20 Hz (`coop: remote ...` lines). Remaining for Phase 2: in-game overlay (P2b) and resolving the hunter position source.

## In flight
Nothing running. (No Grok jobs active.)

## Next steps (in order)
1. **Hunter position source (lead, expert):** hunter `+0xE8` reads a constant (0,-1.69,0) with no input. Find where the player's world position/yaw actually live (phys object `+0x118`, camera `m_pActiveCamera`, or a player controller). Needs either (a) static RE of the hunter movement/camera update, or (b) a human movement test: user runs `run-game.ps1 -ExtraArgs '-nointro -developper -cheats -spmap HUNTMAP4'`, walks around for 20 s, we diff the logs. Prefer (a) first, then confirm with (b).
2. **P2b overlay (Grok scaffold, lead review):** Dear ImGui on the D3D8 device via `dh2::d3d8::AddPresentCallback` (main thread only); F8 toggle; panels: session (role, id, peers, RTT), local hunter sample, remote players, animal count. Spec still to write (`re/jobs/p2b-overlay.md`).
3. **Phase 3 ghost mode (lead):** spawn a puppet per remote player (`cObjManager::CreateEntity("cScrPlayer"/human class)` + `AddEntity`, null brain) and drive it with `net::Interp` samples. Design note first: `re/notes/lead-puppets.md`.
4. Character facing: animals' `+0xF4` stays 0 while moving — find the real heading field (needed for Phase 4 animal sync).

## Done
- **P2 integration (lead, 10:00–10:20)** — `mod/src/coop/coop.cpp` (settings, `coop_status/host/join/leave` console commands, `m_bAllowBackground`, ENet session pump, 20 Hz PlayerState send, remote log), `coop/player_tracker.cpp` (manager via `AddEntity` hook, vtable classification, hunter/animal samples), `coop/slowmo.cpp` (signature-checked `cHUDSlowMotion::Enable` block + `frametime 1`), `engine/console_cmds.*` (cdecl `AddStaticFuncf`). Framework fix: `OnFrame` only on the device-window thread. `run-coop-local.ps1` now passes `-coophost` / `-coopjoin 127.0.0.1`. Runtime-verified in two-instance runs (see MEMORY "Lead runtime findings").
- **p2a-framework (Grok, 09:34–10:06)** — module system, probe→module, D3D8 Present/Reset hooks, `console::Exec`, `CreateMutexA` multi-instance hook, `run-coop-local.ps1`, crash dumps, `docs/mod-architecture.md`. Job ended before writing runtime proofs; lead ran them (both instances reach HUNTMAP4, mutex renamed). Lead fixed `mod/build.ps1` stderr handling.
- **p2c-net (Grok, 09:20–09:53)** — `mod/src/net/` + `mod/net-test/` (932/932 checks, lead-verified).
- **Phase 1** — six tracks merged into `MEMORY.md` (Route E). **Phase 0** — tools, sandbox, ASI route.

## Blockers / risks
- None blocking. Hunter position source unknown (step 1). Watch Claude usage limits.

## Recent results (newest first)
- 10:20: **Phase 2 exit criterion met** — host and client exchange PlayerState; each logs the other's remote state (rx counting at ~20 Hz).
- 10:15: tracker finds hunter (`cScr_ModHunter`) + 5 animals on HUNTMAP4; animal positions are world-space, y-up.
- 10:10: loading-screen thread found calling Present → OnFrame gated to main thread.
- 10:08: game pauses when unfocused → `m_bAllowBackground=1`.
- 10:06: console command registration + `coop_status` handler verified at runtime (cdecl ABI correct).
- 09:29: delegation policy (user): expert work → lead/Opus; Grok → Sonnet-level work only.

## Resume protocol (new session)
1. Open the session in `D:\scary hunt\dh2coop` (loads `CLAUDE.md`; REA MCP is registered for this folder only).
2. Read this file, then `PLAN.md` decisions, then skim `MEMORY.md`.
3. Check jobs: `Get-Process grok`, and for each in-flight log look for the `=== end` line. A job with no end line and no `grok` process was killed — rerun it: `.\tools-src\grokjob.ps1 -Name <job> -Effort <effort>`.
4. `git status` / `git log -5` to see uncommitted work since the last checkpoint.
5. Build: `.\mod\build.ps1`; two-instance test: `.\run-coop-local.ps1 -Seconds 100`; logs in `game\Bin\dh2coop.log` and `game2\Bin\dh2coop.log`.
6. Continue with **Next steps** above. Update this file after every step and commit (timestamps from `Get-Date`). Edit docs with the Write/Edit tools, not PowerShell `Get-Content`/`Set-Content`.
