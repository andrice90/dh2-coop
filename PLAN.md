# DH2 Co-op — Phased Plan

Goal: 2–4 player online co-op for the 2005 PC retail build (v1.0, build 434), shipped as an ASI mod. Players supply their own game copy.

Source context: `docs/research-packet.md`. Verified facts live in `MEMORY.md`; this file is the roadmap.

## What Phase 0 recon changed

The research packet assumed a single-player engine with no symbols. Recon on 2026-10-15 found:

1. **Named exports.** `EngineDll7.dll` exports 8587 C++-mangled functions (571 classes), `CommonDll7.dll` 3174 (199 classes, including AI and world objects), `Data\GameDll7.dll` 133. Class and method names come free; RE is mapping semantics, not guessing names.
2. **Built-in netcode.** `EngineDll7.dll` imports `WS2_32` (`sendto`/`recvfrom`/`bind`) and exports `cNetServer` (`Create(int,int,int)`, `CheckNewClients`, `DisconnectClient`, `DispatchMessages`, `AreRemotesIgnored`), `cNetClient` (`ConnectionSucceeded`, `EnableRemotePing`), `cNetGameState` and `cNetClientInfo`. `debug.log` shows the game runs an internal server (`[SERVER] loading time ... (map, client)`, Lua state `SpServer`).
3. **Lua inside.** The engine embeds Lua (`EngLuaExecuteScript`, `EngLuaExecuteBuffer`); managers (resource, FX, `SpServer`) run Lua states.
4. **Game DLL exists.** `Data\GameDll7.dll` is loaded at runtime (game/interface version 1.0), so gameplay logic is split from the engine as in the `7r` titles.
5. **Renderer.** Engine imports `d3d8.dll!Direct3DCreate8` and `d3d9.dll!Direct3DCreate9`; which one is used at runtime is to be confirmed.

This opens a second route to co-op: **revive the engine's own client/server path** instead of building a full sync layer from scratch. Phase 1 decides between the two.

## Phases

**Phase status (2026-10-16):** Phase 0 done · Phase 1 done (Route E) · **Phase 2 in progress** — see `STATUS.md`.

### Phase 0 — Tooling and ground truth *(done 2026-10-15)*
- Install: JDK 21, Ghidra 12.1.4, REA MCP (Ghidra backend), x32dbg, VS 2022 Build Tools (Win32), Ninja, ImHex, PE-bear, 7-Zip, ReClass.NET. Cheat Engine dropped (see Decisions).
- Sandbox game copy at `game/` (never mod the original install).
- Hashes, imports, export catalogs recorded (`re/exports/`, `MEMORY.md`).
- Headless Ghidra projects for `Main7.exe`, `EngineDll7.dll`, `CommonDll7.dll`, `GameDll7.dll`.
- Build the AlphaYellow FOV fix unchanged and load it in `game/` (proves ASI route).
- **Exit:** tools runnable, Ghidra analysis saved, FOV fix loads, `MEMORY.md` populated.

### Phase 1 — Recon spikes (parallel)
| Track | Question | Output |
|---|---|---|
| A. Net layer | Is `cNetServer`/`cNetClient` a live loopback used in single player? What is the packet format, port, and message dispatch? Can a second client connect? | `re/notes/net-layer.md`, go/no-go on native route |
| B. Object model | Player, camera, entity list, animal (`cAIHuntBrain_Animal`, `cObjEntity`) layouts; spawn/damage/death functions | `re/notes/object-model.md`, `mod/src/game/*.h` structs |
| C. Assets/scripts | `data.arc` format (zlib entries + `cArcFile` index); extract; inventory Lua and mission scripts | `tools/arc/` extractor, `re/notes/assets.md` |
| D. Loop/timing | Fixed tick or variable dt; frame/game/server update order; slow-mo time scale | `re/notes/timing.md` |
| E. Renderer | D3D8 vs D3D9 at runtime; present hook point for ImGui | `re/notes/renderer.md` |
- **Exit / decision gate:** pick **Route N** (native netcode) or **Route E** (custom ENet sync). Hybrid allowed: native entity replication + ENet transport.

### Phase 2 — Mod skeleton *(in progress)*
ASI DLL (Win32, CMake, safetyhook, spdlog, Dear ImGui), pattern-scan helpers keyed to the export table, ImGui overlay with host/join panel, ENet transport, config ini. Jobs: **p2a** core (modules, Present frame hook, console API + `coop_host`/`coop_join`, multi-instance mutex hook, slow-mo off, player tracker, `run-coop-local.ps1`), **p2c** ENet `net::Session` + protocol v1 + interpolation + standalone tests, then **p2b** D3D8 ImGui overlay and **p2d** integration. **Exit:** two instances on one PC exchange `PlayerState`, each logs/shows the other's position.

### Phase 3 — Ghost mode
Remote players drawn as puppets (debug box first, then human model via game spawn routine). **Exit:** two PCs see each other move smoothly in one free-hunt area.

### Phase 4 — Shared animals
Host owns animals; client AI ticks suppressed per entity; transforms interpolated; spawn/death reliable. One-animal spike first. **Exit:** both players see the same bear charge the same target.

### Phase 5 — Shared combat
Client shots become host-validated hit requests; damage/kills replicate; animal target selection across N players. **Exit:** either player can kill an animal attacking the other.

### Phase 6 — Hardening
Join-in-progress, disconnects, 4-player test, slow-mo policy, host-only saves, NAT notes (Tailscale/ZeroTier). **Exit:** 30-minute 4-player session without desync.

### Phase 7 — Missions (stretch)
Host-run mission scripts, client teleport on load, mission flags replicated, melee/QTE lockout for non-targets.

## Agent workflow (from Phase 1)

| Tier | Model | Owns |
|---|---|---|
| Lead / orchestrator | Claude Opus 5.5 (main session) | Job specs, review of every deliverable, route decision, sync architecture, merging verified facts into `MEMORY.md`, commits |
| Workers | Grok 4.7 via Grok Build CLI (`grok`, headless) | All defined work: RE tracks, builds, runtime experiments, catalogs, tooling. Effort `xhigh` for critical RE, `high` by default, `medium` for mechanical jobs |
| Escalation | Claude Opus subagent | Only when a Grok deliverable fails review twice or a task needs deep cross-checking |

Mechanics (since 2026-10-16): a job is a prompt file `re/jobs/<name>.md` (with `re/jobs/_rules.md` prepended) run by `tools-src/grokjob.ps1 -Name <name> [-Effort high]`. It uses Grok permission mode `auto` (never `bypassPermissions`) and writes its log to `re/jobs/<name>.out.log`. Each Ghidra job gets its own slot letter. Exactly one job per wave owns game launches.

Rules: workers read `CLAUDE.md`, `MEMORY.md` and `PLAN.md` first; write only their listed deliverables, and put verified facts in a `## Proposed MEMORY facts` section that the lead merges after review. They never touch the original install and never commit.

History: wave 1 (2026-10-15) used Claude Opus/Sonnet/Haiku subagents; the Haiku track finished, and the others were cut off by usage limits after producing raw dumps, the arc extractor and the mod probe. Wave 1b (2026-10-16) re-ran tracks A–E on Grok from those artifacts.

## Decisions (user, 2026-10-15)
- Slow motion: **disabled for all players** in co-op (no host-only mode).
- MVP scope: **free-hunt areas only**. Story missions stay in Phase 7.
- Cheat Engine: **not used** (Windows Defender blocks the installer as PUA). Memory work uses ReClass.NET, x32dbg, and logging/memory-dump panels inside our own ASI.
- Game window may pop up during automated runtime tests.

## Decisions (lead, 2026-10-16)
- **Route E chosen** (Phase 1 gate): custom sync layer on ENet inside our ASI; host-authoritative. The engine's `cRdpDriver`/`cNetServer` are dead code with no replication, and single player has no client/server split to reuse (`re/notes/net-layer.md`, runtime UDP checks). Borrow the *shape* of `cSpServer` (host ticks world, client is a view). Reuse from the engine only: `-join`/`-dedicated`-style mutex bypass knowledge for running two instances on one PC (needed for local testing).
- **Test maps:** `HUNTMAP4` (Argentina free-hunt: smallest hunt area, 2 spawn points, 4 red deer + 1 puma) for Phases 3–4; `MAPARENA` (bonus Aztec Temple, 377 KB, several predators) for Phase 5 combat. Both are non-story, consistent with the free-hunt MVP scope.
- **Loose-file overrides are a mod tool:** files under `game\Data\` win over `data.arc`, so level scripts/configs can be overridden without repacking (e.g. animal placement in `WORLD.SCR`, if the VM compiles `.SCR` or we can ship `.XSC`). Investigate before writing spawn hooks in C++.
- Two local instances need the `"GI UNIQUE MUTEX"` check bypassed; our ASI should handle this (hook `CreateMutexA` or pass `-join`) rather than relying on `-dedicated` (which disables rendering).

## Open decisions
- Project name for release (neutral, no trademarks).
