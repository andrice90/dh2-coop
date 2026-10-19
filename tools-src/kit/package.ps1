# Builds the player kit: dist\dh2coop-kit\ and dist\dh2coop-kit.zip (flat zip, no subfolder).
# Usage:  .\tools-src\kit\package.ps1 [-Asi <path>] [-Loader <path>]
#   -Asi     the mod build to ship (default mod\build\dh2coop.asi; build it with .\mod\build.ps1 first)
#   -Loader  the Ultimate ASI Loader dinput8.dll (default game\Bin\dinput8.dll; must be the 32-bit build)
param(
    [string]$Asi = '',
    [string]$Loader = ''
)
$ErrorActionPreference = 'Stop'
$kitSrc = $PSScriptRoot
$root   = Split-Path (Split-Path $kitSrc -Parent) -Parent
if (-not $Asi)    { $Asi    = Join-Path $root 'mod\build\dh2coop.asi' }
if (-not $Loader) { $Loader = Join-Path $root 'game\Bin\dinput8.dll' }
foreach ($f in $Asi, $Loader) { if (-not (Test-Path $f)) { throw "missing: $f" } }
$desc = (Get-Item $Loader).VersionInfo.FileDescription
if ($desc -ne 'Ultimate ASI Loader') { throw "$Loader is not Ultimate ASI Loader (FileDescription '$desc')" }

$dist = Join-Path $root 'dist'
$out  = Join-Path $dist 'dh2coop-kit'
$zip  = Join-Path $dist 'dh2coop-kit.zip'
if (Test-Path $out) { Remove-Item $out -Recurse -Force }
New-Item -ItemType Directory -Force $out | Out-Null

$kitFiles = 'README.txt', 'THIRD-PARTY-NOTICES.txt', 'install.bat', 'install.ps1', 'uninstall.bat', 'uninstall.ps1',
            'host.bat', 'join.bat'
foreach ($f in $kitFiles) { Copy-Item (Join-Path $kitSrc $f) (Join-Path $out $f) }
Copy-Item $Asi (Join-Path $out 'dh2coop.asi')
Copy-Item (Join-Path $root 'mod\dh2coop.ini') (Join-Path $out 'dh2coop.ini')
Copy-Item $Loader (Join-Path $out 'dinput8.dll')
# LICENSE uses LF in the repo; Notepad-friendly CRLF in the kit.
$lic = [IO.File]::ReadAllText((Join-Path $root 'LICENSE')) -replace "`r`n", "`n" -replace "`n", "`r`n"
[IO.File]::WriteAllText((Join-Path $out 'LICENSE.txt'), $lic)

if (Test-Path $zip) { Remove-Item $zip -Force }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($out, $zip, [IO.Compression.CompressionLevel]::Optimal, $false)

Get-ChildItem $out | Sort-Object Name | ForEach-Object { '{0,-26} {1,10:N0}' -f $_.Name, $_.Length }
$hash = (Get-FileHash $zip -Algorithm SHA256).Hash
"Kit: $zip ($('{0:N0}' -f (Get-Item $zip).Length) bytes)"
"SHA-256: $hash"
