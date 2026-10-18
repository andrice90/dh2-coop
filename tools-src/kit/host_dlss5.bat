@echo off
rem Co-op host with the DLSS 5 chain. Same map prompt as host.bat.
rem The mod supplies the desktop resolution and wide FOV ([Display] in dh2coop.ini).
rem DLSS 5 needs the files the launcher already put in this folder, plus dgVoodoo2 2.83.2's
rem D3D8.dll and D3D9.dll. This game draws with Direct3D 8, so ReShade's dxgi.dll is never
rem loaded until that wrapper is here. Do not use dgVoodoo 2.87.x on an RTX 50 card.
rem   host_dlss5.bat          asks which map, remembers the last one
rem   host_dlss5.bat MAP3B    hosts that map directly
cd /d "%~dp0"
set STORE=%LOCALAPPDATA%\dh2coop
if not exist "%STORE%" mkdir "%STORE%"
if not exist Main7.exe (
  echo Main7.exe not found. Run install.bat from the kit first.
  pause
  exit /b 1
)
if not exist dxgi.dll (
  echo dxgi.dll is missing. Turn on DLSS 5 for this game in the DLSS 5 Launcher first.
  pause
  exit /b 1
)
if not exist dlss5-feed.addon32 (
  echo dlss5-feed.addon32 is missing. Turn on DLSS 5 for this game in the DLSS 5 Launcher first.
  pause
  exit /b 1
)
if not exist host64\dlss5-feed-host64.exe (
  echo host64\dlss5-feed-host64.exe is missing. Turn on DLSS 5 for this game in the DLSS 5 Launcher first.
  pause
  exit /b 1
)
if not exist D3D8.dll (
  echo D3D8.dll from dgVoodoo2 2.83.2 is missing next to Main7.exe.
  echo Copy MS\x86\D3D8.dll and MS\x86\D3D9.dll from dgVoodoo2_83_2.zip into this folder,
  echo with a dgVoodoo.conf that sets OutputAPI = d3d11_fl11_0, DisableAndPassThru = false, and VRAM = 1024.
  pause
  exit /b 1
)
if not exist dgVoodoo.conf (
  echo dgVoodoo.conf is missing next to Main7.exe.
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
echo Hosting co-op on UDP 27420, map %MAP%, with the desktop resolution, wide FOV, and DLSS 5.
echo Home opens the ReShade overlay. F6 toggles the neural pass.
start "" Main7.exe +mod data -nointro -developper -cheats -spmap %MAP% -coophost
