# Builds tools-src\replaycmd\dh2cmd.asi (x86, static CRT). Used by tools-src\replay-test.ps1 -NoAsi.
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$vcvars = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
$env:PATH = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer;$env:PATH"
$out = Join-Path $here 'build'
New-Item -ItemType Directory -Force $out | Out-Null
$bat = Join-Path $out '_build.bat'
@"
@echo off
call "$vcvars" >nul || exit /b 1
cd /d "$out"
cl /nologo /O2 /MT /EHsc /W3 /LD "$here\dh2cmd.cpp" /Fe:dh2cmd.asi /link user32.lib || exit /b 1
"@ | Set-Content -Encoding ASCII $bat
& cmd.exe /c "`"$bat`" 2>&1"
if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }
"Built: $(Join-Path $out 'dh2cmd.asi')"
