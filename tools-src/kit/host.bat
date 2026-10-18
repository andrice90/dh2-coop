@echo off
rem Co-op host without the lobby window: the game starts at its main menu and already hosts on UDP 27420.
rem Start a New Game or Load a save there; friends follow. The DH2 COOP shortcut (lobby, F8) does the same with buttons.
rem   host.bat          main menu, hosting
rem   host.bat MAP3B    hosts that map directly (testing)
cd /d "%~dp0"
if not exist Main7.exe (
  echo Main7.exe not found. Run install.bat from the kit first.
  pause
  exit /b 1
)
set MAP=%~1
rem Resolution and FOV come from the mod: [Display] in dh2coop.ini (desktop size, wide horizontal FOV, 4:3 HUD).
if "%MAP%"=="" (
  start "" Main7.exe +mod data -nointro -developper -cheats -coophost
) else (
  start "" Main7.exe +mod data -nointro -developper -cheats -spmap %MAP% -coophost
)
