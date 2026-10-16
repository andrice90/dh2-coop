# Cabela's Dangerous Hunts 2 — Co-op Mod Research Packet

Oct 15, 2026 · @Andrew

## Purpose and how to use this packet

This packet is the starting context for agents building a 2–4 player co-op mod for the 2005 PC release of Cabela's Dangerous Hunts 2 (CDH2). Public knowledge of this specific game's internals is thin, so most engine facts must be confirmed by reverse engineering in the first milestone.

Confidence legend used throughout:

- **\[Confirmed\]** stated by a source linked in this doc.
- **\[Likely\]** inferred from the developer's other titles, the era, or the platform set; verify before building on it.
- **\[Unknown\]** no public information; listed as an RE task in the last section.

Agents should treat every \[Likely\] and \[Unknown\] item as a hypothesis, record what they verify in the project's MEMORY.md, and update this packet when a fact is settled.

## Game facts

CDH2 is a single-player, story-driven survival hunting game; the Windows build was made by Magic Wand Productions, a sister studio of Sand Grain Studios and Fun Labs.

| Field | Value | Confidence |
| --- | --- | --- |
| Publisher | Activision (Activision Value line); Zoo Digital listed for some regions ([Wikipedia](https://en.wikipedia.org/wiki/Cabela%27s_Dangerous_Hunts_2)) | Confirmed |
| Windows developer | Magic Wand Productions; PS2 by Sand Grain Studios, Xbox by Fun Labs, GameCube by Magic Wand ([Wikipedia](https://en.wikipedia.org/wiki/Cabela%27s_Dangerous_Hunts_2)) | Confirmed |
| Studio group | Romanian group of Sand Grain, Fun Labs and Magic Wand, sharing tech across Cabela's titles ([MobyGames](https://www.mobygames.com/company/3358)) | Confirmed |
| Release | NA Nov 15, 2005 (consoles); Windows NA/EU Nov 24, 2005 | Confirmed |
| PC SKU | CD-ROM, $19.99 MSRP, ESRB T ([Activision PR](https://investor.activision.com/node/16551)) | Confirmed |
| Players | Single-player only; story mode is the only mode | Confirmed |
| Content | \~20 animals, 12 locations (Alaska, Zimbabwe, India, Australia, Siberia and more), Yeti boss | Confirmed |
| Architecture | 32-bit Windows, x87 FPU math (seen in hook code) | Confirmed |
| Renderer | Direct3D 8 or 9 | Likely; verify by imports |
| Copy protection | Disc check of the era (SecuROM/SafeDisc-type) | Unknown; affects debugging and hooking |

Related port: Cabela's Dangerous Hunts: Ultimate Challenge (PSP, 2006) is a CDH2 port that adds a Quick Action mode and two animals ([Wikipedia](https://en.wikipedia.org/wiki/Cabela%27s_Dangerous_Hunts:_Ultimate_Challenge)).

## Engine and technology

CDH2 runs on the in-house engine of the Sand Grain / Magic Wand / Fun Labs group, version 7, shipped as `Bin\Main7.exe` plus `Bin\EngineDll7.dll`. It is not CloakNT: that is Cauldron's engine, used by the later Dangerous Hunts 2013 (`CloakNTEngine.dll`).

### What is confirmed from public source code

The only open-source code that touches CDH2 internals is AlphaYellow's FOV fix ([source](https://github.com/alphayellow1/AlphaYellowWidescreenFixes/blob/main/source/fixes/CabelasDangerousHunts2FOVFix), [release notes](https://newreleases.io/project/github/alphayellow1/AlphaYellowWidescreenFixes/release/cabelasdangeroushunts2)). Read it before anything else; it is a working, minimal injection template for this exact binary.

- **Process layout:** the game exe is `Main7.exe` in `<GameDir>\Bin\`. Engine code lives in `EngineDll7.dll`, loaded after process start; the fix polls `GetModuleHandleA("EngineDll7.dll")` every 100 ms until it appears.
- **Load method:** the fix is an `.asi` loaded by ThirteenAG's Ultimate ASI Loader, installed as the 32-bit `winmm.dll` proxy in `Bin\`.
- **Hooking:** mid-function hooks with safetyhook, found by byte-pattern scans inside `EngineDll7.dll`, not fixed addresses.
- **Camera object (register `edi` at the hook):** `+0xD0` = vertical FOV in degrees (float; gameplay values 57–75), `+0xD4` = 0.75 aspect term (1/(4:3)), `+0xD8` = 1.0 scale term, `+0xDC` = derived value stored just before. Patterns: FOV read `D9 87 D0 00 00 00 D8 0D ?? ?? ?? ?? D9 F2 DD D8` (fld, fmul by constant, fptan); aspect `D9 97 DC 00 00 00 D9 87 D4 00 00 00`.
- **Math:** x87 FPU instructions throughout, consistent with a 2005 MSVC build without SSE.
- **Copy protection:** a plain CD check; the 2005 no-CD fix replaces `BIN\ENGINEDLL7.DLL`, so the disc check lives in the engine DLL ([GameCopyWorld](https://gamecopyworld.com/games/pc_cabelas_dangerous_hunts_2.shtml)). Expect an attached debugger to be fine once the check is bypassed; verify that no anti-debug exists.

### Engine version lineage (useful for cross-referencing)

The same camera struct layout (`+0xD0/+0xD4/+0xD8`) appears across every title on this engine, which means symbols, strings or structs recovered from any sibling transfer to CDH2.

| Engine DLL | Titles (exe) | Note |
| --- | --- | --- |
| `EngineDll8r.dll` + `GameDll8r.dll` | BGH 2008 Trophy Bucks (`Ctb.exe`), Outdoor Adventures 2009 (`COA.exe`) | Game logic split into a separate DLL |
| `EngineDll7r.dll` + `GameDll7r.dll` | Harley-Davidson: Race to the Rally (`Harley7r.exe`) | Same split; good reference for where gameplay code moved |
| `EngineDll7.dll` | **CDH2 (`Main7.exe`)**, BGH 2007 Alaskan Adventure (`BGH10.exe`) | Closest sibling to CDH2 |
| `EngineDll6.dll` | BGH 2005 Adventures, BGH 2006 Trophy Season, Deer Hunt 2005 | Older version |
| `EngineDll.dll` | BGH 2004, African Safari (`CAS.exe`), 4x4 Off-Road Adventure 3, U.S. Most Wanted (`Engine.exe`) | Oldest |

All rows come from fixes in the same repo ([AlphaYellowWidescreenFixes](https://github.com/alphayellow1/AlphaYellowWidescreenFixes)).

### Still unknown

- Renderer: D3D8 vs D3D9. Check the import table of `EngineDll7.dll` for `d3d8.dll` / `d3d9.dll`. This decides which overlay/hook library to use.
- Whether gameplay logic lives in `Main7.exe` or in `EngineDll7.dll` (no `GameDll7.dll` is referenced for CDH2, unlike the `7r` builds).
- Asset archive format and whether any gameplay scripting exists. The later CloakNT titles use Lua 5.1 bytecode and `.CRS` archives, but that does not apply here.
- Whether `Main7.exe` ships with symbols, RTTI or debug strings. Check with a strings pass; RTTI class names would map the object model quickly.
- Network code: none is expected (single-player only on every platform), but check imports for `ws2_32.dll` / `dplayx.dll` in case leftover libraries exist.

## Gameplay systems relevant to co-op

The game is a linear, scripted story campaign with free-hunt unlocks; every system below assumes one player and has to be either synced, made per-player, or disabled for co-op.

| System | What is known | Co-op implication |
| --- | --- | --- |
| Mode structure | Story mode only, 5 regions (Alaska, Tanzania, India, Australia, Siberia); finishing a mission unlocks its area for free hunting ([Wikipedia](https://en.wikipedia.org/wiki/Cabela%27s_Dangerous_Hunts_2), [CheatCC review](https://www.cheatcc.com/?p=74351)) | **Free-hunt areas are the MVP target**: no scripted triggers, cutscenes or NPC escort logic |
| Missions | Escorts, rescues, timed defense, a bow-only level, tranquilizer missions, helicopter shooting segment, bosses (Hogzilla, Big Grimm, Yeti) ([Gamicus](https://gamicus.fandom.com/wiki/Cabela%27s_Dangerous_Hunts_2)) | Mission scripts likely test "the player" directly; plan to run them on host only and teleport/replicate clients |
| NPC guides | Named companions (Hugh, Abby, Reginald, Wirake) accompany the player in some missions | Companion AI targets the local player; host-only |
| Animal AI | Passive species flee; predators charge or flank: cats crouch then pounce, bears charge, wolves flank in packs | Target selection must be patched to choose among N players. Host-authoritative AI |
| Slow motion | Hold-to-use bullet time drawing from a meter ([CheatCC review](https://www.cheatcc.com/?p=74351)) | Global time scale. Options: disable in co-op, or host-only vote. Must not desync tick rate |
| Health | Limited energy, injuries, regen when standing still, health packs; hazards cause limping or staggering | Per-player state; damage events must route to the right player entity |
| Melee / QTE | Hand-to-claw close-range fights (defend, attack, escape) per [Activision PR](https://investor.activision.com/node/16551) | Likely a camera-locked sequence bound to the single player; hardest piece to share. Lock other players out or skip |
| Weapons | Handguns, rifles, shotguns, knives, machetes, bow, tranquilizer | Fire events + hit results replicated; ammo per-player |
| Saves | Autosave at checkpoints | Host saves only; clients don't write saves |
| Cheats | Stump-collection unlock (80 stumps) grants unlimited ammo / infinite energy codes | The cheat flags are a cheap way to find player-state fields in memory |

Camera, input, HUD and audio listener are all single-instance. Each client keeps its own local copy of these; only world state is shared.

## Existing mods, patches and community notes

There is effectively no modding scene for CDH2 on PC: one open-source FOV fix, one 2005 no-CD exe, and console cheat codes. No PCGamingWiki page, no Cheat Engine table on FearLess Revolution, no asset tools and no prior multiplayer attempts turned up.

| Item | What it is | Value for this project |
| --- | --- | --- |
| [AlphaYellow FOV Fix v1.1](https://github.com/alphayellow1/AlphaYellowWidescreenFixes/blob/main/source/fixes/CabelasDangerousHunts2FOVFix) | ASI plugin, safetyhook mid-hooks in `EngineDll7.dll`, ini config, spdlog logging ([mirror](https://www.gry-online.pl/download/cabelas-dangerous-hunts-2-widescreen-fov-fix-mod/zc150bd)) | **High.** Proven injection path, build setup and two verified engine patterns. Fork its project layout as the mod's skeleton |
| No-CD fixed `ENGINEDLL7.DLL` (TNT, 2005-11-20) | Cracked engine DLL for v1.0 English ([GameCopyWorld](https://gamecopyworld.com/games/pc_cabelas_dangerous_hunts_2.shtml)) | Tells us v1.0 is the only PC version and the CD check is in the engine DLL. Don't distribute or depend on it; the mod's own loader can patch the check at runtime if needed |
| Official patches | None found; only v1.0 exists | Every player has the same binary, so pattern scans and offsets stay stable |
| PS2 CodeBreaker codes | Infinite ammo, never reload, soft lock on/off, show HUD, "idiot AI animals", unlock all missions ([Almar's Guides](https://almarsguides.com/retro/walkthroughs/ps2/games/cabelasdangeroushunts2/codebreaker/)) | The PS2 build has the same flag set; matching PC flags exist. "Idiot AI" points to a global AI aggression toggle worth finding |
| In-game cheat codes | Symbol-sequence codes: double damage, double health, infinite ammo, fast regen ([CheatCC](https://www.cheatcc.com/?p=82502)) | Toggling a code and diffing memory in Cheat Engine is the fastest way to locate player health/ammo fields |
| Disc preservation | Cabela's PC collection redump on [Internet Archive](https://archive.org/details/cabela-pc-collection) | Reference only; use your own retail copy |

GameCube build runs "playable" in Dolphin ([Dolphin wiki](https://wiki.dolphin-emu.org/forum/Cabela's%20Dangerous%20Hunts%202/embed)). That is irrelevant for the PC mod, but the GameCube and PS2 builds can be loaded in Ghidra for cross-reference if their symbols are richer (GameCube discs sometimes ship `.map` files).

## Related titles worth cross-referencing

The best RE accelerators are the sibling games on the same engine version; anything learned on one usually maps to CDH2 by pattern scan.

- **Cabela's Big Game Hunter 2007 Alaskan Adventure (`BGH10.exe`, `EngineDll7.dll`).** Same engine DLL generation as CDH2; camera code matches with only a register change (`ebp` vs `edi`). First choice for side-by-side diffing in BinDiff/Diaphora.
- **Harley-Davidson: Race to the Rally (`EngineDll7r.dll` + `GameDll7r.dll`).** The "7r" revision split gameplay into its own DLL. Its export/import table between the two DLLs can reveal the engine API surface that CDH2 calls internally.
- **BGH 2008 Trophy Bucks / Outdoor Adventures 2009 (`EngineDll8r.dll`).** Newer revision; useful if a debug string or symbol name only survived there.
- **Cabela's Dangerous Hunts (2003) PC.** Predecessor by the same studio group ([Internet Archive](https://archive.org/details/DangerousHunts)); likely on the older `EngineDll.dll`. Shares animal AI design.
- **Cabela's Dangerous Hunts: Ultimate Challenge (PSP, 2006).** Direct port of CDH2 ([Wikipedia](https://en.wikipedia.org/wiki/Cabela%27s_Dangerous_Hunts:_Ultimate_Challenge)); PSP ELFs sometimes keep more symbols.
- **Not related:** Dangerous Hunts 2013 and BGH Pro Hunts run on Cauldron's CloakNT engine ([ModDB](https://www.moddb.com/engines/cloaknt)) with `.CRS` archives and Lua 5.1 bytecode ([Steam modding thread](https://steamcommunity.com/app/247770/discussions/0/558752449891981321)). Agents should not apply that tooling to CDH2.

## Precedents: how single-player games got co-op

Every successful retrofit uses the same shape: an injected client DLL that reverse-engineers the game's types, spawns "puppet" entities for remote players, and lets one authority (host or server) own world state. The three below are open source and worth reading for structure, not for code reuse.

| Project | Game | What to borrow |
| --- | --- | --- |
| [Skyrim Together (TiltedEvolution)](https://github.com/tiltedphoques/TiltedEvolution) | Skyrim SE | Folder split: `client/` holds engine hooks, reverse-engineered game types and sync logic; separate `server/`, launcher and early-load DLL. Ships an `AGENTS.md` for agent-driven contribution |
| [MafiaHub Framework](https://github.com/MafiaHub/Framework) | Mafia series (generic) | A game-agnostic MP layer: networking, ECS for entity management, Dear ImGui UI, logging. Ships `AGENTS.md` and `CLAUDE.md`. Model for keeping CDH2-specific code thin over a reusable sync core |
| [sm64coopdx](https://github.com/coop-deluxe/sm64coopdx) | Super Mario 64 (decomp) | Proof that syncing **enemies and level events**, not just players, is what makes co-op feel real. Its object-ownership model maps well to animals |
| [open.mp](https://github.com/openmultiplayer/open.mp) | GTA San Andreas | Long-running example of a server-authoritative model bolted onto a 32-bit, D3D9-era single-player exe |

Recurring lessons from these projects:

1. **Puppets first.** Remote players are spawned as existing NPC or player-model entities whose input comes from the network. Getting a second human model to stand in the world is the first real milestone.
2. **One owner per entity.** AI runs only on its owner (host). Clients suppress the AI tick for replicated animals and interpolate transforms.
3. **Hook events, not state.** Sync "shot fired", "damage applied", "animal killed", "mission flag set" rather than diffing memory every frame.
4. **Scripted content is last.** Free-roam/sandbox ships long before story missions work in co-op; many projects never fully support campaigns.
5. **Global effects need a policy.** Pause, time-scale (here: slow motion), saves and cutscenes need an explicit co-op rule, usually host-only or disabled.

## Tooling

The recommended stack is Ghidra (with GhidraMCP so agents can query it) + x32dbg + Cheat Engine/ReClass.NET for RE, and an ASI-loaded C++ DLL using safetyhook, Dear ImGui and ENet for the mod. Everything must target **32-bit (Win32/x86)**. All repos below were checked to exist on 2026-10-15.

### Static analysis and decompilation

| Tool | Use here |
| --- | --- |
| [Ghidra](https://github.com/NationalSecurityAgency/ghidra) | Primary decompiler for `Main7.exe` and `EngineDll7.dll`; free, scriptable, handles MSVC x86 and RTTI recovery |
| [GhidraMCP](https://github.com/LaurieWired/GhidraMCP) | Exposes a running Ghidra project over MCP, so Claude Code agents can list functions, decompile and rename symbols directly |
| [ida-pro-mcp](https://github.com/mrexodia/ida-pro-mcp) | Same idea for IDA Pro, if IDA is used instead |
| [Diaphora](https://github.com/joxeankoret/diaphora) / [BinDiff](https://github.com/google/bindiff) | Diff `EngineDll7.dll` against sibling engine DLLs (BGH 2007, Harley `7r`) to transfer names |
| [Cutter](https://github.com/rizinorg/cutter) / [RetDec](https://github.com/avast/retdec) | Second-opinion decompilers |
| [PE-bear](https://github.com/hasherezade/pe-bear) | Imports/exports, sections, and spotting the renderer DLL and any packer |
| [ImHex](https://github.com/WerWolv/ImHex) | Hex editor with pattern language, for save files and asset archives |

### Dynamic analysis

| Tool | Use here |
| --- | --- |
| [x64dbg (x32dbg)](https://github.com/x64dbg/x64dbg) | Live debugging, breakpoints on the camera/player structs found by the FOV fix |
| [ScyllaHide](https://github.com/x64dbg/ScyllaHide) | Only if the CD check or any anti-debug interferes |
| [Cheat Engine](https://github.com/cheat-engine/cheat-engine) | Find player health/ammo/position via value scans; "find what writes" to reach functions; build a shared `.CT` table |
| [ReClass.NET](https://github.com/ReClassNET/ReClass.NET) | Map player, animal and camera class layouts; export C++ structs for the mod |
| [Frida](https://github.com/frida/frida) | Fast scripted tracing of function calls (e.g. log every damage call with args) before writing C++ hooks |
| [apitrace](https://github.com/apitrace/apitrace) | Capture and replay D3D8/D3D9 calls; RenderDoc does not support D3D8/9 |

### Injection and hooking (mod runtime)

| Library | Use here |
| --- | --- |
| [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) | Loads the mod as an `.asi` via a `winmm.dll` (or `dinput8.dll`) proxy; same as the FOV fix, so both coexist |
| [safetyhook](https://github.com/cursey/safetyhook) | Inline and mid-function hooks; already proven on this binary |
| [MinHook](https://github.com/TsudaKageyu/minhook) / [PolyHook 2](https://github.com/stevemk14ebr/PolyHook_2_0) | Alternatives if safetyhook hits an edge case |
| [kiero](https://github.com/Rebzzel/kiero) | Universal D3D8/9 hook for `EndScene`/`Present`, to draw the overlay |
| [Dear ImGui](https://github.com/ocornut/imgui) | In-game lobby, connect dialog, debug panels (has a DX9 backend) |
| [d3d8to9](https://github.com/crosire/d3d8to9) | If the game is D3D8: converts to D3D9 so DX9 tooling, ImGui and ReShade work |
| [ReShade](https://github.com/crosire/reshade) source | Reference for robust D3D8/9 hook and overlay code |
| [WidescreenFixesPack](https://github.com/ThirteenAG/WidescreenFixesPack) | Many examples of pattern-scan hooks in 2000s-era games |

### Networking

| Library | Use here |
| --- | --- |
| [ENet](https://github.com/lsalzman/enet) | **Recommended.** Small C UDP library with reliable and unreliable channels; builds for Win32 without fuss |
| [yojimbo](https://github.com/mas-bandwidth/yojimbo) | More complete game-netcode library (connect tokens, encryption); heavier |
| [GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets) | Valve's open-source transport; heavier build, 32-bit support should be verified |

NAT traversal: for an MVP, have the host port-forward or have players use a virtual LAN (Tailscale, ZeroTier). Add a relay later if needed.

### Build environment

- Visual Studio 2022 or later with the **Win32** platform target, or CMake + vcpkg (`x86-windows-static` triplet).
- Fork the AlphaYellow fix's dependency set (safetyhook, spdlog, inipp) to start from a known-good build.
- Test on a real Windows machine with the retail game; Wine is fine for build steps but not for reliable D3D hooking tests.

## Proposed MVP architecture and phased plan

The MVP is 2–4 players in a free-hunt area, with one host running the real game simulation and clients running the same game with animal AI suppressed and remote players shown as puppets. Story missions are out of scope for the MVP.

&#91;embedded content: co-op mod architecture · host and client, 4 layers each\]

Each PC runs the retail game plus one ASI mod. Only the host's sync core decides what animals do and whether a shot killed something; clients send inputs and hit requests and draw what the host reports. A Dear ImGui overlay (hooked on the D3D present call) carries the host/join dialog and debug panels.

### Phases

1. **Phase 0: Tooling and ground truth.** Build the FOV fix from source, set up Ghidra (+ GhidraMCP), x32dbg and a Cheat Engine table. Exit: binary hashes, renderer, and player/camera pointers recorded in MEMORY.md.
2. **Phase 1: Ghost mode.** Each player runs their own world; the mod sends local position, rotation and current weapon over ENet and draws the others as puppets (or debug boxes first). Exit: two PCs see each other move smoothly in the same free-hunt area.
3. **Phase 2: Shared animals.** Host owns all animals; clients disable AI ticks for replicated animals and interpolate host transforms. Spawns and deaths sync as reliable events. Exit: both players see the same bear charge the same target.
4. **Phase 3: Shared combat.** Client shots become hit requests validated by the host; damage and kills replicate; animal AI picks targets among all players. Exit: either player can kill an animal attacking the other.
5. **Phase 4: Hardening.** Join-in-progress, disconnects, 4-player test, slow-mo policy, host-only saves. Exit: a 30-minute 4-player session without desync.
6. **Later: Missions.** Host-run mission scripts with clients teleported on load and mission flags replicated; skip or lock out melee/QTE sequences for non-target players.

The main risk sits in Phase 2: whether the animal AI tick can be cleanly suppressed per entity. Prove that with a one-animal spike before building the full sync core.

## Open questions and first-week RE tasks

The first week should settle the \[Unknown\] items above; each task below has a concrete output agents can record in MEMORY.md.

- [ ] Inventory the install: list every file under `<GameDir>` and `Bin\`, file sizes, and hash `Main7.exe` and `EngineDll7.dll` (SHA-256) so all agents know they work on the same v1.0 binary.
- [ ] Read imports/exports of `Main7.exe` and `EngineDll7.dll` in PE-bear. Record: `d3d8` vs `d3d9`, `dinput8` vs raw input, `dsound`, any `ws2_32`/`dplayx`, and the engine DLL's export list.
- [ ] Load both into Ghidra with RTTI analysis. Record whether class names survive; dump `strings` and grep for file extensions, `.cfg`/`.ini` names, mission or script names, and debug text.
- [ ] Build and run the AlphaYellow FOV fix unchanged. Confirm the ASI loader route works on the target machine.
- [ ] Cheat Engine: find local player world position (3 floats), yaw/pitch, health, ammo, slow-mo meter. Use the in-game cheat codes as memory diffs. Find the pointer path to the player object.
- [ ] From the camera struct at `+0xD0`, walk back to the camera owner and find the camera's link to the player entity.
- [ ] Find the entity list or manager that holds animals: iterate in ReClass.NET, record class size, transform offset, health, AI state.
- [ ] Locate the functions for: spawn entity, apply damage, kill/death, weapon fire/hit test, slow-mo toggle, save, and mission/level load.
- [ ] Test whether a second player-model instance can be spawned via the game's own spawn routine (the "puppet" spike).
- [ ] Identify the asset archive format under the game folder and whether level/mission data is scripted or hard-coded.
- [ ] Decide the timing model: does the game run a fixed tick or a variable `dt`? This decides how state is interpolated.

Open design questions for the user:

- Slow motion in co-op: disable entirely, or host-only?
- Scope: free-hunt only for MVP, or also attempt early Alaska missions?
- Distribution: GitHub releases of the ASI + ini only.

## Legal notes

- Ship only original code (the ASI, ini, and launcher). Never redistribute `Main7.exe`, `EngineDll7.dll`, game assets, or a no-CD binary; players supply their own retail copy.
- Reverse engineering for interoperability is broadly tolerated for non-commercial mods, but the trademarks (Cabela's, Activision) should not appear as branding. Use a neutral project name and describe compatibility instead.
- This is a general summary, not legal advice.

## Sources

- [AlphaYellow FOV fix source](https://github.com/alphayellow1/AlphaYellowWidescreenFixes/blob/main/source/fixes/CabelasDangerousHunts2FOVFix) and [release notes](https://newreleases.io/project/github/alphayellow1/AlphaYellowWidescreenFixes/release/cabelasdangeroushunts2)
- [AlphaYellowWidescreenFixes repo](https://github.com/alphayellow1/AlphaYellowWidescreenFixes) (sibling engine titles)
- [Wikipedia: Cabela's Dangerous Hunts 2](https://en.wikipedia.org/wiki/Cabela%27s_Dangerous_Hunts_2)
- [Wikipedia: Dangerous Hunts: Ultimate Challenge](https://en.wikipedia.org/wiki/Cabela%27s_Dangerous_Hunts:_Ultimate_Challenge)
- [MobyGames: Sand Grain Studios](https://www.mobygames.com/company/3358)
- [Activision press release](https://investor.activision.com/node/16551)
- [CheatCC review](https://www.cheatcc.com/?p=74351) and [cheat codes](https://www.cheatcc.com/?p=82502)
- [Gamicus wiki](https://gamicus.fandom.com/wiki/Cabela%27s_Dangerous_Hunts_2)
- [GameCopyWorld file listing](https://gamecopyworld.com/games/pc_cabelas_dangerous_hunts_2.shtml)
- [Almar's Guides PS2 codes](https://almarsguides.com/retro/walkthroughs/ps2/games/cabelasdangeroushunts2/codebreaker/)
- [Steam: BGH Pro Hunts modding thread](https://steamcommunity.com/app/247770/discussions/0/558752449891981321)
- [ModDB: CloakNT engine](https://www.moddb.com/engines/cloaknt)
