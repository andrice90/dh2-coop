# Friends' kit (text files)

Source of the player kit. `package.ps1` builds it: these files (without this note and `package.ps1`), `mod\build\dh2coop.asi`, `mod\dh2coop.ini`, the Ultimate ASI Loader `dinput8.dll` (from `game\Bin`) and the repo `LICENSE` (as `LICENSE.txt`) go into `dist\dh2coop-kit\` (gitignored), zipped to `dist\dh2coop-kit.zip`.

- `install.ps1` / `install.bat`: finds the game, copies the mod, shortcut, one firewall rule; `-DryRun`, `-FindOnly`, `-Uninstall`, `-Yes`.
- `uninstall.ps1` / `uninstall.bat`: removes every version's files, settings, shortcuts, firewall rules and a leftover router forward; `-DryRun`.
- `host.bat`, `join.bat`: start the game hosting / joining without the lobby.
- `README.txt`: the players' guide. `THIRD-PARTY-NOTICES.txt`: license texts of the loader and the libraries in the ASI.
- Tests: `tools-src\kit-test\installer-test.ps1`.
