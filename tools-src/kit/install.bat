@echo off
rem Installs the DH2 co-op mod into your game folder, makes the DH2 COOP desktop shortcut, installs Tailscale
rem and opens the firewall for it. Asks for the host's join code (the host presses Enter).
rem   install.bat              install (or update)
rem   install.bat -Uninstall   remove the mod, the shortcut and the firewall rule (Tailscale stays)
rem   install.bat -DryRun      only print what the Tailscale, winget and firewall steps would do
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1" %*
echo.
pause
