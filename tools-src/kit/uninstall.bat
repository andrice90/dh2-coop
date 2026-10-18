@echo off
rem DH2 co-op uninstaller: removes the co-op mod completely, old versions included (details in uninstall.ps1).
rem   uninstall.bat           remove it (asks once)
rem   uninstall.bat -DryRun   only list what would be removed
set DH2COOP_FROM_BAT=1
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0uninstall.ps1" %*
echo.
pause
rem Run from the game's Bin, this file goes last, once the mod is gone. (goto) ends the batch before the delete, so cmd
rem does not look for the missing file afterwards.
if exist "%~dp0Main7.exe" if not exist "%~dp0dh2coop.asi" if not exist "%~dp0uninstall.ps1" (goto) 2>nul & del "%~f0"
