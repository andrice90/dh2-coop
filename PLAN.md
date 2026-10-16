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

### Phase 0 — Tooling and ground truth *(in progress)*
- Install: JDK 21, Ghidra 12.1.4, REA MCP (Ghidra backend), x32dbg, VS 2022 Build Tools (Win32), Ninja, ImHex, PE-bear, 7-Zip, ReClass.NET. Cheat Engine: user installs manually (bundled-offer installer).
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

### Phase 2 — Mod skeleton
ASI DLL (Win32, CMake, safetyhook, spdlog, Dear ImGui), pattern-scan helpers keyed to the export table, ImGui overlay with host/join panel, ENet transport (or native net wrapper), config ini. **Exit:** overlay shows local player position; two instances on one PC exchange heartbeat packets.

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
| Lead | Opus 5.5 | Net-layer RE (Track A), object model (B), route decision, sync architecture, hard hooks, review of Sonnet output |
| Builder | Sonnet 5.5 | Mod skeleton, ENet/overlay code, `data.arc` extractor (C), renderer probe (E), Ghidra scripts |
| Grunt | Haiku 5.5 | Export catalog classification, string dumps, Lua file inventory, MEMORY.md bookkeeping, test checklists |

Rules: every agent reads `CLAUDE.md` + `MEMORY.md` first; writes findings to `re/notes/` and verified facts to `MEMORY.md` with evidence (address, export name, or log line); never touches the original install; one owner per file per wave.

## Open decisions for the user
- Slow motion in co-op: disable, or host-only?
- MVP scope: free-hunt only (recommended), or early Alaska missions too?
- Project name for release (neutral, no trademarks).
