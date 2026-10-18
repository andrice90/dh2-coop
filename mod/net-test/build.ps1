# Configure, build, and run dh2net_test.exe (Win32/x86, Release, static CRT).
# Usage: .\mod\net-test\build.ps1 [-Config Release|Debug] [-Clean]
param(
    [ValidateSet('Release', 'Debug')][string]$Config = 'Release',
    [switch]$Clean
)
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$buildDir = Join-Path $here 'build'

$vsRoot = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools"
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars32.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars32.bat not found: $vcvars" }

# vcvars32.bat needs the VS Installer directory on PATH (vswhere).
$env:PATH = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer;$env:PATH"

$ninja = (Get-Command ninja -ErrorAction SilentlyContinue).Source
if (-not $ninja) {
    $ninja = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Recurse -Filter ninja.exe -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $ninja) {
    $ninja = Get-ChildItem $vsRoot -Recurse -Filter ninja.exe -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $ninja) { throw 'ninja.exe not found' }
$ninjaDir = Split-Path $ninja -Parent

if ($Clean -and (Test-Path $buildDir)) { Remove-Item $buildDir -Recurse -Force }
New-Item -ItemType Directory -Force $buildDir | Out-Null

$bat = Join-Path $buildDir '_build.bat'
@"
@echo off
call "$vcvars" >nul || exit /b 1
set "PATH=$ninjaDir;%PATH%"
cmake -S "$here" -B "$buildDir" -G Ninja -DCMAKE_BUILD_TYPE=$Config -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl || exit /b 1
cmake --build "$buildDir" || exit /b 1
"@ | Set-Content -Encoding ASCII $bat

# Merge stderr inside cmd: in Windows PowerShell 5.1 a native stderr line (e.g. git's "Cloning into ...")
# becomes a terminating NativeCommandError under ErrorActionPreference=Stop when the host captures stderr.
& cmd.exe /c "`"$bat`" 2>&1"
if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }

$exe = Join-Path $buildDir 'dh2net_test.exe'
if (-not (Test-Path $exe)) { throw "expected output not found: $exe" }
"Built: $exe"
& $exe
if ($LASTEXITCODE -ne 0) { throw "dh2net_test failed (exit $LASTEXITCODE)" }
"dh2net_test passed"

# UPnP / wrong-password guard tests (offline; `dh2upnp_test.exe --live [port]` talks to the real router).
& (Join-Path $buildDir 'dh2upnp_test.exe')
if ($LASTEXITCODE -ne 0) { throw "dh2upnp_test failed (exit $LASTEXITCODE)" }
"dh2upnp_test passed"
