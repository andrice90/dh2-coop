DH2 co-op mod 0.2.0: test kit (build 2026-10-18)
==================================================
Source, updates and full guide: https://github.com/andrice90/dh2-coop

Needs: the retail game, v1.0 (build 434), installed and starting normally on its own.
All players need the same game version and the same kit.

INSTALL (everyone)
1. Unzip the whole kit anywhere (for example your Downloads folder).
2. Double-click install.bat. It finds the game (...\Activision Value\Magic Wand\DNGH2\Bin, on any
   local drive), asks for your player name once, copies the mod in, and puts one shortcut on your
   desktop: DH2 COOP. Windows asks for permission once: the installer adds one firewall rule so
   friends can reach a game you host (Main7.exe, UDP 27420).
   Game somewhere else?  powershell -ExecutionPolicy Bypass -File install.ps1 -GameDir "<your Bin folder>"
3. Updating to a newer kit: run install.bat from the new kit (your player name is kept).
4. Uninstall: double-click uninstall.bat (in this kit, or in the game's Bin folder, where the
   installer leaves a copy). It removes every version of the mod completely: the mod files, its
   logs and settings (also in %LOCALAPPDATA%\dh2coop), the shortcuts and the firewall rules. A
   dinput8.dll from another mod is put back.
   uninstall.bat -DryRun only lists what it would remove. install.bat -Uninstall does the same.

HOW FRIENDS REACH THE HOST
When you press HOST, the lobby lists every way in, best first:
- Internet: the mod asks your router to open the game port (UPnP) while you host, and shows your
  public address and a JOIN CODE. Send the code (or the address) to your friends. The port closes
  again when you stop hosting or quit the game. Most home routers allow this; if yours does not,
  the line says why (UPnP off in the router, or an internet provider that shares one address).
- Tailnet: your Tailscale address, if you use Tailscale (set up by hand: everyone installs
  Tailscale from tailscale.com and joins the same tailnet, or the host shares this PC with them).
- LAN: your address at home, for players on the same network.
Anyone who has the address and port can join while you host. To keep strangers out, set
Password= in the [Coop] section of dh2coop.ini; the join code then carries it, and friends who type
an address need the same Password= in their dh2coop.ini.

PLAY
1. Everyone: DH2 COOP on the desktop. The game opens at its main menu with the co-op lobby window.
   Type your name and pick the character the others will see you as.
2. Host: press HOST. The lobby shows the join code and the addresses (Copy buttons next to them).
3. Friends: paste the join code into the JOIN box, or type the host's IP (IP:port when the port is
   not 27420), and press JOIN. The status line says "Connected", or what went wrong. Everyone in
   the lobby sees the player list.
4. Host: press F8 to close the lobby, then start a New Game or Load a save from the game's menu.
   Friends stay in their main menu: they follow the host into the game automatically.
5. F8 opens and closes the lobby window at any time (it shows the players and their ping).
   While it is open the game ignores your keys and mouse.
6. Hunting map: close the CHOOSE ITEM menu with Enter, Left, Enter.
Friends can also join later, while the host is already playing: they catch up and appear next to the host.

IF A LEVEL GETS STUCK (host only, everyone comes along)
- F5, then F5 again within 3 seconds: restart the current level from its start (the game's own
  checkpoints are level starts; there are none inside a level).
- F6, then F6 again within 3 seconds: skip to the next story level.
- The same two buttons are in the lobby window (F8) while you host a level.
- A player stuck somewhere can press F9 after dying to stand up next to the host.

WHAT WORKS TODAY
- Up to 4 players. You see each other (the character you picked, with your weapon), moving, aiming, firing.
- F7 toggles markers over the other players (name and distance, or an arrow at the screen edge).
- Hunting map: the same deer for everyone. Anyone can shoot, kill and claim (F); the mission ends
  with MISSION SUCCESSFUL for all.
- Story maps: the host's progress (trigger zones, cutscenes, the guide characters, animal deaths)
  runs for everyone, and the next story map loads for everyone.
- Animals go for the nearest player, the joining players included, and hurt them.
- A player who dies presses F9 to stand back up next to the host. If the HOST dies, the level ends.
- Join any time: a late player catches up on the story and is placed next to the host.
- Shooting or kicking another player knocks them over for a couple of seconds (no damage).

NOT YET / KNOWN PROBLEMS
- The final yeti fight (MAP12C) and the truck ride (MAP4A) were fixed in this build but not yet
  played through by two real PCs. The host's truck sometimes rolls over on the MAP4A ramp.
- Guide and companion characters follow / wait for the HOST only.
- Pause menus are per player (the host pausing freezes animals for everyone).

IF JOIN DOES NOT CONNECT
- The host must have pressed HOST. Over the internet, the host's lobby line "Internet" must show an
  address (not a reason). If it shows a reason, use the tailnet or LAN address instead.
- The host's firewall: run install.bat again on the host PC and press Yes at the Windows prompt.
- Both players need the same kit (the host refuses a different mod build and says so).

IF THE MOD DOES NOT LOAD ("dh2coop.asi ... error 0xc0e90002", then the game starts without co-op)
Windows 11 Smart App Control blocked the mod: it refuses DLLs that are not signed. The installer
says so when it is on. The mod only loads with Smart App Control off (Windows Security, App &
browser control, Smart App Control settings). On most Windows 11 versions it cannot be turned back
on without resetting Windows, so decide for yourself.

IF SOMETHING BREAKS
Send the log files from both players. They are in the game's Bin folder, or, when the game is installed
under Program Files, in %LOCALAPPDATA%\dh2coop (paste that into the Explorer address bar):
  dh2coop.log, debug.1.log (this run's engine log is debug.log in Bin), and after a crash and a restart also
  dh2coop.1.log (the run before) and debug.2.log.

LICENSE
The mod is MIT licensed (LICENSE.txt). It is an unofficial fan project, not affiliated with the game's
publisher or developer, and contains no game files. The ASI loader (dinput8.dll) and the libraries built
into dh2coop.asi keep their own licenses: see THIRD-PARTY-NOTICES.txt.
