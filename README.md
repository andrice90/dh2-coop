# Cabela's Dangerous Hunts 2 Coop Mod

Online co-op for 2 to 4 players, added to a 2005 single-player PC hunting game: Cabela's Dangerous Hunts 2. You and your friends play the story campaign and the free-hunt map together: you see each other, the animals are the same for everyone, and the host's story progress (trigger zones, cutscenes, guide characters, map changes) runs on every PC.

The mod is an ASI plugin (`dh2coop.asi`) that loads into the unmodified retail game. No game files are changed or included, and every player needs their own copy of the game.

> **Status: playable test build.** Every story map has been played in co-op sections with 2 to 4 copies of the game on one PC. Full playthroughs between separate PCs over the internet are still being tested. See [Known issues](#known-issues).

## Contents

- [Features](#features)
- [Requirements](#requirements)
- [Install](#install)
- [Play](#play)
- [Connecting to friends](#connecting-to-friends)
- [Settings](#settings)
- [Uninstall](#uninstall)
- [Troubleshooting](#troubleshooting)
- [Known issues](#known-issues)
- [How it works](#how-it-works)
- [Building from source](#building-from-source)
- [Repository layout](#repository-layout)
- [License](#license)
- [Credits](#credits)
- [Disclaimer](#disclaimer)

## Features

- **2 to 4 players.** One player hosts and the others join from an in-game lobby window (F8).
- **See each other.** Other players appear as characters you pick in the lobby, carrying the weapon they hold. You can see them move, aim, fire and reload.
- **Shared animals.** The host runs the animals, and everyone sees the same ones in the same places. Animals go for the nearest player and can hurt anyone. Any player can shoot, kill and claim an animal, and the kill counts for the mission.
- **Shared story.** The host's trigger zones, cutscenes, guide characters, tasks and animal deaths run for everyone, and the next story map loads for everyone. Vehicle rides, the helicopter sections and the boss fights are synchronized too.
- **Join any time.** A player who joins mid-level catches up on the story and appears next to the host.
- **Second chances.** A player who dies presses F9 to stand back up next to the host. The level ends only when the host dies.
- **Level controls for the host.** Restart the current level or skip to the next one, and everyone comes along.
- **Player markers.** F7 shows names and distances above the other players, or an arrow at the screen edge.
- **Internet play without router setup.** While you host, the mod asks your router to open the game port (UPnP), shows a join code to send to friends, and closes the port again when you stop hosting or quit.
- **Widescreen and display fixes.** The game starts at your screen's own resolution as a borderless window, widens the view on wide screens instead of stretching it (Hor+), keeps the HUD and compass in shape, and asks Windows for the real screen size when display scaling is on.
- Slow motion is turned off for everyone in co-op.

## Requirements

- Windows 10 or 11, on every player's PC.
- The retail PC game *Cabela's Dangerous Hunts 2*, version 1.0 (build 434), installed and starting normally on its own. The mod checks the build, and all players need the same game version.
- The same mod kit on every PC. A host refuses a player who has a different mod build and tells them why.
- To host over the internet: a router with UPnP turned on, or a port forward you set up yourself (UDP 27420), or a VPN such as Tailscale. See [Connecting to friends](#connecting-to-friends).

## Install

1. Download `dh2coop-kit.zip` from the [Releases](https://github.com/andrice90/dh2-coop/releases) page, or [build the kit yourself](#building-from-source).
2. Unzip the whole kit anywhere, for example your Downloads folder.
3. Double-click `install.bat`. The installer:
   - finds the game's `Bin` folder (the one with `Main7.exe`, normally `...\Activision Value\Magic Wand\DNGH2\Bin`) on any local drive,
   - asks for your player name once,
   - copies the mod into `Bin`,
   - puts one shortcut on your desktop: **DH2 COOP**,
   - asks Windows for permission once (UAC), to add a firewall rule so friends can reach a game you host: `Main7.exe`, inbound UDP 27420.

   If your game is somewhere else, run the installer from PowerShell in the kit folder and point it at the `Bin` folder:

   ```powershell
   powershell -ExecutionPolicy Bypass -File install.ps1 -GameDir "D:\Games\DNGH2\Bin"
   ```

4. **To update**, run `install.bat` from the new kit. Your player name and password are kept.

If another mod already uses `dinput8.dll`, the installer keeps it as `dinput8.dll.dh2coop-backup` and the uninstaller puts it back.

### Manual install

Copy `dinput8.dll` (Ultimate ASI Loader), `dh2coop.asi` and `dh2coop.ini` from the kit into the game's `Bin` folder. Start the game as usual. To host, let `Main7.exe` through Windows Firewall for inbound UDP 27420.

## Play

1. **Everyone:** start the game with the **DH2 COOP** shortcut. The game opens at its main menu with the co-op lobby window on top. Type your name and pick the character the others will see you as.
2. **Host:** press **HOST**. The lobby lists the addresses friends can use and a join code, each with a Copy button.
3. **Friends:** paste the join code into the JOIN box, or type the host's IP address (`IP:port` if the port is not 27420), and press **JOIN**. The status line says *Connected*, or what went wrong.
4. **Host:** press **F8** to close the lobby, then start a New Game or load a save from the game's menu. Friends stay in their main menu, and the game takes them along to the host's map.
5. On the free-hunt map, close the CHOOSE ITEM menu with Enter, Left, Enter.

Friends can also join later, while the host is already playing.

### Keys

| Key | What it does |
|---|---|
| F8 | Open or close the lobby window (players and their ping). While it is open, the game ignores your keyboard and mouse. |
| F7 | Show or hide markers over the other players. |
| F9 | After you die: stand back up next to the host. |
| F5, F5 | Host only, press twice within 3 seconds: restart the current level from its start, for everyone. |
| F6, F6 | Host only, press twice within 3 seconds: skip to the next story level, for everyone. |
| F | Claim a killed animal, as in the single-player game. Any player can claim. |

The restart and skip buttons are also in the lobby window while you host a level. The game's own checkpoints are level starts, so a restart goes back to the start of the level.

Shooting or kicking another player knocks them over for a couple of seconds. It does no damage.

## Connecting to friends

When you press HOST, the lobby shows every way in, best first:

| Row | Who uses it | How it works |
|---|---|---|
| **Internet** + **Join code** | Friends anywhere | The mod asks your router to forward the game port to your PC (UPnP) and shows your public address. Send friends the join code (or the address). The forward is removed when you stop hosting or quit the game, and a forward left behind by a crash is removed the next time the mod starts. If your router says no, this row says why: UPnP is off, or your internet provider shares one public address between customers (CGNAT). |
| **Tailnet** | Friends on your Tailscale network | Your Tailscale address, if you use [Tailscale](https://tailscale.com). Set it up yourself: everyone joins the same tailnet, or you share your PC with them. |
| **LAN** | Players on your home network | Your local address. |

A **join code** looks like `DH2-…`. It holds the host's address, port, password (if set) and name. Treat it as private if you set a password.

### Password

By default anyone who has your address and port can join while you host. To keep strangers out, set a password in the `[Coop]` section of `dh2coop.ini` (in the game's `Bin` folder):

```ini
[Coop]
Password=pick-something
```

The join code then carries the password, so friends who paste the code need nothing else. Friends who type your address instead need the same `Password=` in their own `dh2coop.ini`. After 5 wrong passwords in 10 minutes, the host ignores that address for 10 minutes.

### No UPnP?

Forward **UDP 27420** on your router to your PC and give friends your public IP. Or use Tailscale, ZeroTier or a similar VPN and give friends that address.

## Settings

Settings live in `dh2coop.ini` next to the game's `Main7.exe`. If the game is installed under `Program Files`, the mod keeps its settings and logs in `%LOCALAPPDATA%\dh2coop` instead. Every key has a comment in the file. The ones you are most likely to change:

| Section | Key | Default | Meaning |
|---|---|---|---|
| `[Coop]` | `PlayerName` | `Hunter` | Your name in the lobby. The installer sets it. |
| `[Coop]` | `Port` | `27420` | UDP port for hosting. |
| `[Coop]` | `Password` | empty | Session password (see above). |
| `[Coop]` | `UPnP` | `1` | Ask the router to open the port while hosting. `0` turns it off. |
| `[Overlay]` | `Markers` | `1` | Player markers on at start (F7 toggles them). |
| `[Display]` | `Width`, `Height` | `0` | Start resolution. `0` means your screen's own size (borderless full screen). |
| `[Display]` | `Windowed` | `1` | `0` starts in real full screen. |
| `[Display]` | `DpiAware` | `1` | Get the real screen size when Windows display scaling is above 100%. |

The `[Animals]`, `[Combat]`, `[Story]`, `[Vehicles]`, `[Scale]` and `[Bosses]` sections hold gameplay switches. Where both sides must agree (for example `[Scale]`), the host's values apply to everyone.

## Uninstall

Double-click `uninstall.bat`, either in the kit or in the game's `Bin` folder (the installer leaves a copy there). It removes every version of the mod:

- the mod files and logs in `Bin`, and `%LOCALAPPDATA%\dh2coop`,
- the DH2 COOP shortcuts,
- the `DH2 coop` firewall rules,
- a port forward the mod left on your router after a crash.

A `dinput8.dll` from another mod is put back. `uninstall.bat -DryRun` only lists what it would remove. Close the game first.

## Troubleshooting

**The mod does not load, and the log or a message says `dh2coop.asi ... error 0xc0e90002`.** Windows 11 Smart App Control blocked the mod, because it refuses DLLs that are not code-signed. The game then starts without co-op. The installer warns you if Smart App Control is on. The mod only loads with Smart App Control turned off (Windows Security > App & browser control > Smart App Control settings). On most Windows 11 versions it cannot be turned back on without resetting Windows, so decide for yourself.

**Join does not connect.**
- The host must have pressed HOST.
- Over the internet, the host's **Internet** row must show an address, not a reason. If it shows a reason, use a port forward, the Tailnet row or the LAN row.
- On the host PC, run `install.bat` again and press Yes at the Windows prompt, so the firewall rule is added.
- Everyone needs the same kit.

**Something breaks.** Send the logs from every player. They are in the game's `Bin` folder, or in `%LOCALAPPDATA%\dh2coop` when the game is under Program Files:
- `dh2coop.log` (this run) and `dh2coop.1.log` (the run before),
- `debug.1.log` and `debug.2.log` (the game's own log, kept by the mod).

The logs never contain your password or the full join code, and public IP addresses are masked.

## Known issues

- The host's truck sometimes rolls over on the ramp in the MAP4A truck ride.
- The final boss fight (MAP12C) and the MAP4A truck ride have not yet been played through on two real PCs.
- Guide and companion characters follow and wait for the host only.
- Pause menus are per player. When the host pauses, the animals freeze for everyone.
- Windows Smart App Control blocks the mod (see [Troubleshooting](#troubleshooting)). Code-signing the mod would fix this; it is not signed yet.

## How it works

### Loading into the game

The game's engine DLL imports `dinput8.dll`. [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) is placed in `Bin` under that name, and loads `dh2coop.asi` from the same folder when the engine starts. From `DllMain`, the mod hooks the single-instance mutex (so several copies of the game can run on one PC for testing), makes the process DPI aware, and starts an init thread that waits for the engine and game DLLs.

### Reverse engineering

The game is built on a 2005 engine split into `Main7.exe`, `EngineDll7.dll`, `CommonDll7.dll` and `GameDll7.dll`. A lucky find shaped the whole project: the DLLs export about 11,900 C++-mangled functions (8,587 from the engine alone), so class and method names come for free. The work was mapping what they do, not guessing names.

- **Static analysis** in [Ghidra](https://ghidra-sre.org) (headless projects and scripts in `re/scripts/`).
- **Runtime checks** from inside the mod: logging probes, memory dumps and test console commands, verified with two to four game windows side by side.
- **Hooks** by export name, byte pattern, or module offset with a prologue check, because every DLL relocates. Only version 1.0 build 434 is supported, and a hook whose bytes do not match is skipped instead of patching blind.
- Verified findings are recorded with evidence in [`MEMORY.md`](MEMORY.md); per-topic research is in [`re/notes/`](re/notes).

### Networking

The engine has client/server classes, but research showed that single player never uses a real socket: the "server" is an in-process object, the network classes are never called, and no entity replication exists ([`re/notes/net-layer.md`](re/notes/net-layer.md)). So the mod brings its own:

- **[ENet](https://github.com/lsalzman/enet)** over UDP, with one host and up to three clients in a star. The host is authoritative and relays player states between clients.
- **Its own protocol** ([`docs/net-protocol.md`](docs/net-protocol.md)): a handshake with a version and game-build check, unreliable snapshots for players and animals (interpolated on the receiving side), and reliable events for hits, kills, story triggers, cutscenes, vehicles and map changes.
- **UPnP port forwarding** through the Windows UPnP device finder for discovery and plain SOAP requests for control. The forward is removed in a hook on `ExitProcess`, so it closes even on a normal quit ([`re/notes/upnp.md`](re/notes/upnp.md)).

### Other players in your world

The game has no third-person body for the player (the hunter model is only first-person arms). Remote players are therefore spawned with the engine's own entity spawn routine as NPC characters ("puppets") with a body model you choose. Their physics body is dropped so the mod can place them directly. Their weapon is attached to the hand bone and plays matching animation clips. Their view direction comes from each player's render camera.

### Shared world

- **Animals:** the host runs the AI. On clients, the animals' own AI is suppressed and they follow the host's snapshots. A client's shot becomes a hit request that the host applies. Target selection considers every player, so animals go for the nearest one.
- **Story:** story trigger volumes, tasks, cutscenes, journal entries and mission results are decided on the host and replayed on clients. A late joiner gets a catch-up of what already happened. A watchdog skips a client cutscene that waits on something only the host has.
- **Vehicles, helicopter and bosses** each have their own sync (seats, riders, paths, boss phases).

### Overlay

The lobby and markers are drawn with [Dear ImGui](https://github.com/ocornut/imgui), through a Direct3D 8 backend written for this mod, hooked into the device's `Present`. The D3D8 declarations in `mod/third_party/d3d8` were written from scratch rather than copied from an SDK. The overlay rebinds when the game recreates its device, for example after a resolution change.

### Testing

- `run-coop-local.ps1` starts 2 to 4 copies of the game on one PC (one sandbox folder each), joins them to each other on `127.0.0.1`, and can jump straight to a map, add a late joiner, or capture screenshots.
- `mod/net-test` holds about 4,000 offline checks for the protocol, sessions, join codes and UPnP, with a fake router.
- `tools-src/kit-test/installer-test.ps1` tests the installer and uninstaller in a scratch folder.
- More test scripts for specific features (combat, vehicles, bosses, map flow) are in `tools-src/`.

### Development

Built with [Claude Code](https://claude.com/claude-code): an AI lead session designed the hooks and sync, reviewed every change and kept the notes, with AI sub-agents working in parallel on research, features and tests in their own git worktrees. The project's working notes are kept in the repo: [`STATUS.md`](STATUS.md) (progress log), [`PLAN.md`](PLAN.md) (phases and decisions), [`MEMORY.md`](MEMORY.md) (verified engine facts) and [`docs/mod-learnings.md`](docs/mod-learnings.md) (build, hooking and testing lessons).

## Building from source

### Tools

- Visual Studio 2022 Build Tools with the C++ workload (MSVC v143, x86), in its default install folder (`build.ps1` looks there).
- CMake 3.28 or newer, and [Ninja](https://ninja-build.org).
- Git (CMake downloads the dependencies at configure time).

### Build the mod

```powershell
.\mod\build.ps1
```

This builds `mod\build\dh2coop.asi` (Win32, Release, static CRT) and copies it into the sandbox `game\Bin` (see below; add `-NoInstall` if you have none). Options: `-Config RelWithDebInfo|Debug`, `-Clean`, `-NoInstall`, `-ForceIni`.

The offline tests (builds and runs `dh2net_test` and `dh2upnp_test`):

```powershell
.\mod\net-test\build.ps1
```

### Development sandbox

Never test against your real install. Copy your game folder to `game\` in the repo (it is gitignored), put the 32-bit `dinput8.dll` from an [Ultimate ASI Loader release](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) in `game\Bin`, then build. `run-coop-local.ps1` mirrors `game\` into `game2\`..`game4\` for extra players:

```powershell
.\run-coop-local.ps1 -Players 2 -Map MENU
```

### Build the player kit

```powershell
.\tools-src\kit\package.ps1
```

This copies the kit files from `tools-src\kit\`, `mod\build\dh2coop.asi`, `mod\dh2coop.ini`, Ultimate ASI Loader's `dinput8.dll` (from `game\Bin`) and `LICENSE` into `dist\dh2coop-kit\`, and zips them as `dist\dh2coop-kit.zip`.

## Repository layout

| Path | What it holds |
|---|---|
| `mod/src/` | The mod (C++20). `core/` startup, modules, config, logs; `engine/` D3D8, DirectInput, console and entity hooks; `net/` ENet session, protocol, UPnP; `coop/` everything co-op (players, puppets, animals, story, vehicles, bosses, lobby); `ui/` overlay; `display/` resolution and HUD fixes. |
| `mod/dh2coop.ini` | Default settings, with comments. |
| `mod/net-test/` | Offline tests. |
| `tools-src/kit/` | The player kit: installer, uninstaller, host/join scripts, player README. |
| `tools-src/` | Test and analysis scripts. |
| `run-coop-local.ps1` | Starts several local game copies for testing. |
| `re/` | Reverse-engineering notes (`notes/`), Ghidra scripts (`scripts/`), export lists of the game DLLs (`exports/`) and work orders for the AI agents (`jobs/`). |
| `docs/` | Architecture, protocol and lessons learned. |
| `STATUS.md`, `PLAN.md`, `MEMORY.md` | Progress log, roadmap and verified engine facts. |

No game binaries, assets, extracted archives or decompiled code are in the repo.

## License

The mod's own code and documentation are released under the [MIT License](LICENSE). The license does not cover the game or anything from it. Third-party components keep their own licenses; their full texts ship in the kit as `THIRD-PARTY-NOTICES.txt`.

## Credits

- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) by ThirteenAG (MIT), shipped in the kit as `dinput8.dll`.
- [safetyhook](https://github.com/cursey/safetyhook) by cursey (BSL-1.0), with [Zydis](https://github.com/zyantific/zydis) and [Zycore](https://github.com/zyantific/zycore-c) (MIT).
- [ENet](https://github.com/lsalzman/enet) by Lee Salzman (MIT).
- [Dear ImGui](https://github.com/ocornut/imgui) by Omar Cornut (MIT).
- [spdlog](https://github.com/gabime/spdlog) by Gabi Melman (MIT), with [{fmt}](https://github.com/fmtlib/fmt) (MIT).
- [Ghidra](https://ghidra-sre.org) (Apache 2.0), for the analysis.

## Disclaimer

This is an unofficial fan project. It is not affiliated with, endorsed by or supported by the game's publisher or developer, and all trademarks belong to their owners. It contains no game code or assets: you need your own retail copy of the game. It does not touch the game's copy protection. Use it at your own risk, and keep a backup of your saves.
