@echo off
rem Installs the DH2 co-op mod into your game folder and makes the DH2 HOST / DH2 JOIN desktop shortcuts.
rem   install.bat              install (or update)
rem   install.bat -Uninstall   remove the mod and the shortcuts
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1" %*
echo.
pause
