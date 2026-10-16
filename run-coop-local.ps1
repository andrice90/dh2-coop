# Two sandbox instances of the game, each with its own Bin directory and logs.
# Usage: .\run-coop-local.ps1 [-Seconds N]
# Build first: .\mod\build.ps1
param(
    [int]$Seconds = 0
)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$game = Join-Path $root 'game'
$game2 = Join-Path $root 'game2'
$ccd = 'D:\scary hunt\CABELAS.DANGEROUS.HUNTS.2-Clone\Dngh2.ccd'
$asi = Join-Path $root 'mod\build\dh2coop.asi'
$ini = Join-Path $root 'mod\dh2coop.ini'
$cfg = Join-Path $root 'tools-src\console.windowed.cfg'
$launch = '+mod data -nointro -developper -cheats -spmap HUNTMAP4'

if (Get-Process Main7 -ErrorAction SilentlyContinue) {
    throw 'Main7.exe is already running. Close it before run-coop-local.ps1.'
}
foreach ($need in @($asi, $ini, $cfg, (Join-Path $game 'Bin\Main7.exe'))) {
    if (-not (Test-Path $need)) { throw "missing: $need" }
}

if (-not (Get-Volume | Where-Object { $_.DriveType -eq 'CD-ROM' -and $_.FileSystemLabel -eq 'Dngh2' })) {
    & 'C:\Program Files (x86)\Elaborate Bytes\VirtualCloneDrive\VCDMount.exe' $ccd
    Start-Sleep 4
}

# Refresh game2 from game. Skip logs and the loose console cfg; both are rewritten below.
& robocopy $game $game2 /MIR /XF *.log console.cfg /R:1 /W:1 /NFL /NDL /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy failed (exit $LASTEXITCODE)" }

foreach ($tree in @($game, $game2)) {
    $bin = Join-Path $tree 'Bin'
    Copy-Item $asi (Join-Path $bin 'dh2coop.asi') -Force
    Copy-Item $ini (Join-Path $bin 'dh2coop.ini') -Force
    $cfgDir = Join-Path $tree 'Data\Cfg'
    New-Item -ItemType Directory -Force $cfgDir | Out-Null
    Copy-Item $cfg (Join-Path $cfgDir 'console.cfg') -Force
    Remove-Item (Join-Path $bin 'debug.log') -ErrorAction SilentlyContinue
}

function Start-Instance([string]$tree, [string]$extra) {
    $bin = Join-Path $tree 'Bin'
    $p = Start-Process (Join-Path $bin 'Main7.exe') -ArgumentList "$launch $extra" -WorkingDirectory $bin -PassThru
    Write-Host "pid=$($p.Id) bin=$bin"
    Write-Host "  dh2coop.log=$(Join-Path $bin 'dh2coop.log')"
    Write-Host "  debug.log=$(Join-Path $bin 'debug.log')"
    return $p
}

"launch: $launch"
# Instance 1 hosts, instance 2 joins it (dh2coop.asi applies these after the first map load).
$p1 = Start-Instance $game '-coophost'
Start-Sleep 3
$p2 = Start-Instance $game2 '-coopjoin 127.0.0.1'

if ($Seconds -gt 0) {
    Start-Sleep $Seconds
    foreach ($p in @($p1, $p2)) {
        if (-not $p.HasExited) { $p.CloseMainWindow() | Out-Null }
    }
    Start-Sleep 5
    foreach ($p in @($p1, $p2)) {
        if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    }
    "closed after ${Seconds}s (graceful, then force)"
}
