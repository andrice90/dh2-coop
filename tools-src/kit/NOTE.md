# Friends' kit (text files)

install.ps1 holds the PowerShell twin of the join code format (mod/src/coop/joincode.cpp): `install.ps1 -SelfTest` checks it against the vectors in mod/net-test. `-DryRun` shows the Tailscale, winget and firewall steps without running them; `-Simulate <json>` (tests only) fakes Tailscale's answers.

Source of the files in `dist\dh2coop-kit\` (gitignored). To build the kit: copy these files, `mod\build\dh2coop.asi`, `mod\dh2coop.ini` and the Ultimate ASI Loader `dinput8.dll` (from `game\Bin`) into `dist\dh2coop-kit\`, then zip it to `dist\dh2coop-kit.zip`.
