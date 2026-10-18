DH2 co-op mod: test kit (build 2026-10-17)
==================================================

Needs: the retail game, v1.0 (build 434), installed and starting normally on its own.
Both players need the same game version and the same kit.

INSTALL (everyone)
1. Unzip the whole kit anywhere (for example your Downloads folder).
2. Double-click install.bat. It finds the game (...\Activision Value\Magic Wand\DNGH2\Bin, on any
   local drive), asks for your player name once, copies the mod in, and puts one shortcut on your
   desktop: DH2 COOP. Windows asks for permission once: the installer adds Tailscale (the network
   the game uses) and one firewall rule (Tailscale addresses only).
   It also asks for the join code from your host. The host just presses Enter.
   Game somewhere else?  powershell -ExecutionPolicy Bypass -File install.ps1 -GameDir "<your Bin folder>"
3. Updating to a newer kit: run install.bat from the new kit (your player name is kept; the old
   DH2 HOST / DH2 JOIN shortcuts are removed).
4. Uninstall: double-click uninstall.bat (in this kit, or in the game's Bin folder, where the
   installer leaves a copy). It removes every version of the mod completely: the mod files, its
   logs and settings (also in %LOCALAPPDATA%\dh2coop), the shortcuts and the firewall rule. A
   dinput8.dll from another mod is put back; Tailscale stays installed.
   uninstall.bat -DryRun only lists what it would remove. install.bat -Uninstall does the same.

NETWORK (Tailscale, free)
Host, once:
1. Run install.bat, press Enter at the join code question. It installs Tailscale and opens your
   browser: sign in (a free Tailscale account).
2. The installer prints two things for the Tailscale admin pages and opens them (the first is
   already on your clipboard): paste the policy text over the policy file and Save; then
   Settings > Keys > Generate auth key: Reusable on, Ephemeral on, Tags tag:dh2friend, expiry 90
   days. The policy replaces the default allow-everything policy: if you have your own rules,
   merge by hand. The lobby button "Tailscale setup help" shows these steps again.
3. Start DH2 COOP, paste the key into "Friend key" in the HOST section, press Save.
Host, every time: DH2 COOP, HOST, "Copy join code", send the code to your friends.
Friend: run install.bat and paste the code when asked. Tailscale installs and joins the host's
network by itself: no account, no browser. Then DH2 COOP, JOIN (the host's address is filled in).

PLAY
1. Everyone: DH2 COOP on the desktop. The game opens at its main menu with the co-op lobby window.
   Type your name and pick the character the others will see you as.
2. Host: press HOST. The lobby shows "Lobby open" and the address friends use.
3. Friends: the host's address and port are already in the JOIN box (from the join code; you can
   also paste a code there, or type the host's IP). Press JOIN. The status line says "Connected",
   or what went wrong. Everyone in the lobby sees the player list.
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

IF JOIN DOES NOT CONNECT
The host must have pressed HOST and have Tailscale running; the friend needs Tailscale running too
(tray icon). Run install.bat again after a Windows or Tailscale reinstall.

IF SOMETHING BREAKS
Send the log files from both players. They are in the game's Bin folder, or, when the game is installed
under Program Files, in %LOCALAPPDATA%\dh2coop (paste that into the Explorer address bar):
  dh2coop.log, debug.1.log (this run's engine log is debug.log in Bin), and after a crash and a restart also
  dh2coop.1.log (the run before) and debug.2.log.

