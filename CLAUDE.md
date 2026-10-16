# DH2 Co-op mod — agent guide

Read `MEMORY.md` (verified facts) and `PLAN.md` (roadmap) before any work. Background: `docs/research-packet.md`.

## Layout
- `game/` — sandbox copy of the retail install. All runtime tests and mod installs go here.
- `D:\scary hunt\Activision Value\Magic Wand\DNGH2\` — original install. **Read-only. Never write to it.**
- `re/exports/` — export lists and class counts per DLL (generated).
- `re/ghidra/` — headless Ghidra projects. `re/notes/` — per-topic findings. `re/ce/` — Cheat Engine tables. `re/scripts/` — analysis scripts.
- `mod/` — ASI mod source (C++20, CMake, Win32 only).
- `tools/` — Ghidra, extractors, downloads.

## Binaries (v1.0, build 434, Nov 1 2005)
- `Bin\Main7.exe` — thin launcher, imports 459 functions from the engine.
- `Bin\EngineDll7.dll` (base 0x10000000, relocatable) — engine, D3D8/9, DInput8, WS2_32 netcode, Lua.
- `Bin\CommonDll7.dll` — AI, world and scripted-entity classes.
- `Data\GameDll7.dll` — game rules, weapons, hunter profile.
All DLLs relocate: hook by export name or byte pattern, never by absolute address.

## Rules
- Target **Win32 (x86)** only. MSVC v143, static CRT.
- Record facts in `MEMORY.md` only once verified, with evidence (export name, RVA, pattern, or log line). Hypotheses go in `re/notes/`.
- Never ship or commit game binaries, assets, or extracted archives. `game/`, `tools/`, `re/ghidra/` and extracted data are gitignored.
- No DRM or disc-check work. The retail disc image is mounted for play.
- Keep project branding neutral (no Cabela's/Activision trademarks).
- One owner per file per agent wave; coordinate through notes, not by editing someone else's file.

## Tools
- Ghidra 12.1.4: `tools/ghidra_12.1.4_PUBLIC/` (JDK 21). Headless: `support\analyzeHeadless.bat`.
- REA MCP (`rea`), local scope for this project: read-only Ghidra analysis on Windows.
- x32dbg, ImHex, PE-bear, ReClass.NET, Ninja, VS 2022 Build Tools.
- Python: use `py` (3.14), `pefile` installed.
