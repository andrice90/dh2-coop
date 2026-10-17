@echo off
rem Co-op host. The DH2 HOST desktop shortcut starts this (it lives in the game's Bin folder, next to Main7.exe).
rem   host.bat          asks which map, remembers the last one
rem   host.bat MAP3B    hosts that map directly
cd /d "%~dp0"
rem The last answer is kept per user: a game under Program Files cannot be written by cmd.
set STORE=%LOCALAPPDATA%\dh2coop
if not exist "%STORE%" mkdir "%STORE%"
if not exist Main7.exe (
  echo Main7.exe not found. Run install.bat from the kit first.
  pause
  exit /b 1
)
set MAP=%~1
if not "%MAP%"=="" goto run
set LAST=HUNTMAP4
if exist "%STORE%\lastmap.txt" set /p LAST=<"%STORE%\lastmap.txt"
echo Story maps: MAP1 MAP2A MAP2B MAP2C MAP3A MAP3B MAP3C MAP4A MAP4B MAP5 MAP6 MAP7 MAP8 MAP9 MAP10
echo             MAP11A MAP11B MAP11C MAP12A MAP12B MAP12C    Hunting maps: HUNTMAP1 HUNTMAP2 HUNTMAP3 HUNTMAP4
set /p MAP=Map to host [Enter = %LAST%]:
if "%MAP%"=="" set MAP=%LAST%
:run
>"%STORE%\lastmap.txt" echo %MAP%
echo Hosting co-op on UDP 27420, map %MAP%. Tell your friends your VPN IP address.
start "" Main7.exe +mod data -nointro -developper -cheats -spmap %MAP% -coophost
