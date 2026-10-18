@echo off
rem Installs the DH2 co-op mod into your game folder, makes the DH2 COOP desktop shortcut and lets the game port
rem through Windows Firewall for hosting (one Windows permission prompt).
rem   install.bat              install (or update)
rem   install.bat -Uninstall   remove the mod completely (same as uninstall.bat)
rem   install.bat -DryRun      only print what the firewall step would do
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1" %*
echo.
pause
