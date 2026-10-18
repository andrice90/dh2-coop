@echo off
rem Co-op client without the lobby window: the game starts at its main menu and joins the host (retrying until the
rem host is up), then follows the host into its game. The DH2 COOP shortcut (lobby, F8) does the same with buttons.
rem   join.bat                 asks for the host's address, remembers the last one
rem   join.bat 100.64.1.2      joins that host directly
cd /d "%~dp0"
rem The last answer is kept per user: a game under Program Files cannot be written by cmd.
set STORE=%LOCALAPPDATA%\dh2coop
if not exist "%STORE%" mkdir "%STORE%"
if not exist Main7.exe (
  echo Main7.exe not found. Run install.bat from the kit first.
  pause
  exit /b 1
)
set HOSTIP=%~1
if not "%HOSTIP%"=="" goto have
set LAST=
if exist "%STORE%\lasthost.txt" set /p LAST=<"%STORE%\lasthost.txt"
if "%LAST%"=="" goto ask
set /p HOSTIP=Host tailnet IP address [Enter = %LAST%]:
if "%HOSTIP%"=="" set HOSTIP=%LAST%
goto have
:ask
set /p HOSTIP=Host tailnet IP address:
:have
if "%HOSTIP%"=="" (
  echo No address given.
  pause
  exit /b 1
)
>"%STORE%\lasthost.txt" echo %HOSTIP%
echo Joining %HOSTIP%:27420 (retries every few seconds until the host is up).
rem Resolution and FOV come from the mod: [Display] in dh2coop.ini (desktop size, wide horizontal FOV, 4:3 HUD).
start "" Main7.exe +mod data -nointro -developper -cheats -coopjoin %HOSTIP%
