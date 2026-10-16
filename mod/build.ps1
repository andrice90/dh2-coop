# Builds dh2coop.asi (Win32/x86, Release, static CRT) and installs it into game\Bin.
# Usage:  .\mod\build.ps1 [-Config Release|RelWithDebInfo|Debug] [-Clean] [-NoInstall] [-ForceIni]
param(
    [ValidateSet('Release', 'RelWithDebInfo', 'Debug')][string]$Config = 'Release',
    [switch]$Clean,        # wipe mod\build first (needed when switching generator/config)
    [switch]$NoInstall,    # build only, do not copy into game\Bin
    [switch]$ForceIni      # overwrite game\Bin\dh2coop.ini with mod\dh2coop.ini
)
$ErrorActionPreference = 'Stop'
$modDir   = $PSScriptRoot
$root     = Split-Path $modDir -Parent
$buildDir = Join-Path $modDir 'build'
$binDir   = Join-Path $root 'game\Bin'

$vsRoot  = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools"
$vcvars  = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars32.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars32.bat not found: $vcvars" }

# vcvars32.bat needs the VS Installer directory on PATH (vswhere).
$env:PATH = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer;$env:PATH"

# Locate ninja: PATH, winget package, or the copy bundled with VS Build Tools.
$ninja = (Get-Command ninja -ErrorAction SilentlyContinue).Source
if (-not $ninja) {
    $ninja = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Recurse -Filter ninja.exe -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $ninja) {
    $ninja = Get-ChildItem $vsRoot -Recurse -Filter ninja.exe -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $ninja) { throw 'ninja.exe not found (winget install Ninja-build.Ninja)' }
$ninjaDir = Split-Path $ninja -Parent

if ($Clean -and (Test-Path $buildDir)) { Remove-Item $buildDir -Recurse -Force }
New-Item -ItemType Directory -Force $buildDir | Out-Null

# Run configure+build in one cmd.exe so the vcvars32 environment applies.
$bat = Join-Path $buildDir '_build.bat'
@"
@echo off
call "$vcvars" >nul || exit /b 1
set "PATH=$ninjaDir;%PATH%"
cmake -S "$modDir" -B "$buildDir" -G Ninja -DCMAKE_BUILD_TYPE=$Config -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl || exit /b 1
cmake --build "$buildDir" || exit /b 1
"@ | Set-Content -Encoding ASCII $bat

& cmd.exe /c "`"$bat`""
if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }

$asi = Join-Path $buildDir 'dh2coop.asi'
if (-not (Test-Path $asi)) { throw "expected output not found: $asi" }
"Built: $asi ($([math]::Round((Get-Item $asi).Length / 1KB)) KB)"

if ($NoInstall) { return }

if (Get-Process Main7 -ErrorAction SilentlyContinue) { throw 'Main7.exe is running; close it before installing the ASI.' }
Copy-Item $asi (Join-Path $binDir 'dh2coop.asi') -Force
$iniDst = Join-Path $binDir 'dh2coop.ini'
if ($ForceIni -or -not (Test-Path $iniDst)) { Copy-Item (Join-Path $modDir 'dh2coop.ini') $iniDst -Force }
"Installed to $binDir"
