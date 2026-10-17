DH2 co-op mod: test kit (build 2026-10-17)
==================================================

Needs: the retail game, v1.0 (build 434), installed and starting normally on its own.
Both players need the same game version and the same kit.

INSTALL (everyone)
1. Unzip the whole kit anywhere (for example your Downloads folder).
2. Double-click install.bat. It finds the game (...\Activision Value\Magic Wand\DNGH2\Bin, on any
   local drive), asks for your player name once, copies the mod in, and puts two shortcuts on your
   desktop: DH2 HOST and DH2 JOIN. If the game is under Program Files, Windows asks for permission.
   Game somewhere else?  powershell -ExecutionPolicy Bypass -File install.ps1 -GameDir "<your Bin folder>"
3. Updating to a newer kit: run install.bat from the new kit (your player name is kept).
4. Uninstall: install.bat -Uninstall  (removes the mod files and the shortcuts; a dinput8.dll from
   another mod is put back).

NETWORK (VPN: Tailscale, ZeroTier, Hamachi, ...)
- Both join the same VPN. The host looks up its own VPN IP (for example 100.x.y.z in Tailscale).
- The host must let Main7.exe receive UDP port 27420. Windows Firewall asks the first time you
  host: tick "Private" (and "Public" if your VPN adapter is marked public) and press Allow.

PLAY
1. Host: DH2 HOST on the desktop. Type the map (MAP1 is the start of the story; HUNTMAP4 a hunting map)
   or press Enter for the last one.
2. Friends: DH2 JOIN on the desktop. Type the host's VPN IP once; next time Enter reuses it.
   Friends follow whatever map the host is on, and can join at any time.
3. Hunting map: close the CHOOSE ITEM menu with Enter, Left, Enter.
4. F8 toggles a small status overlay: role, the other player, ping (rtt).
   "role client" plus a row for the host means you are connected.

WHAT WORKS TODAY
- Up to 4 players. You see each other (stand-in character with your weapon), moving, aiming, firing.
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

