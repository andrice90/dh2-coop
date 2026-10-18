DH2 co-op mod: test kit (build 2026-10-17)
==================================================

Needs: the retail game, v1.0 (build 434), installed and starting normally on its own.
Both players need the same game version and the same kit.

INSTALL (everyone)
1. Unzip the whole kit anywhere (for example your Downloads folder).
2. Double-click install.bat. It finds the game (...\Activision Value\Magic Wand\DNGH2\Bin, on any
   local drive), asks for your player name once, copies the mod in, and puts one shortcut on your
   desktop: DH2 COOP. If the game is under Program Files, Windows asks for permission.
   Game somewhere else?  powershell -ExecutionPolicy Bypass -File install.ps1 -GameDir "<your Bin folder>"
3. Updating to a newer kit: run install.bat from the new kit (your player name is kept; the old
   DH2 HOST / DH2 JOIN shortcuts are removed).
4. Uninstall: install.bat -Uninstall  (removes the mod files and the shortcut; a dinput8.dll from
   another mod is put back).

NETWORK (Tailscale)
- Everyone installs Tailscale and joins the same tailnet (the host can share their machine with friends).
- The host's tailnet IP (100.x.y.z) is shown in the lobby window, with a Copy button.
- The host must let Main7.exe receive UDP port 27420. Windows Firewall asks the first time you
  host: tick "Private" (and "Public" if your Tailscale adapter is marked public) and press Allow.

PLAY
1. Everyone: DH2 COOP on the desktop. The game opens at its main menu with the co-op lobby window.
   Type your name and pick the character the others will see you as.
2. Host: press HOST. The lobby shows "Lobby open" and the address friends use.
3. Friends: type the host's tailnet IP (and port, normally 27420) and press JOIN. The status line
   says "Connected", or what went wrong. Everyone in the lobby sees the player list.
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
- MAP12C (the yeti): joining players crash when the yeti fight starts. Avoid that map for now.
- Do NOT press ESC "replay mission" on the results screen: the next map change crashes.
  Quit and start again instead.
- Guide and companion characters follow / wait for the HOST only.
- Pause menus are per player (the host pausing freezes animals for everyone).

IF SOMETHING BREAKS
Send the log files from both players. They are in the game's Bin folder, or, when the game is installed
under Program Files, in %LOCALAPPDATA%\dh2coop (paste that into the Explorer address bar):
  dh2coop.log, debug.1.log (this run's engine log is debug.log in Bin), and after a crash and a restart also
  dh2coop.1.log (the run before) and debug.2.log.

