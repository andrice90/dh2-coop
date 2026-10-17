# Phase 5 test: puppets with a physics body in the host animals' attack list ([Puppets] Physics=1 AttackTarget=1).
# Turns those keys on in the copied ini only (mod\dh2coop.ini is untouched), starts a pair, has the host fire the
# given trigger twice (arm, then fire) and logs what the animals do to the client for -Watch seconds.
# Usage: .\tools-src\combat-test.ps1 [-Map MAP3B] [-Trigger cScrTask02] [-Watch 40] [-ClientWalkMs 0] [-Keep]
param(
    [string]$Map = 'MAP3B',
    [string]$Trigger = 'cScrTask02',
    [int]$Watch = 40,
    [int]$ClientWalkMs = 0,
    [switch]$Keep
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

function Get-Game([string]$tree) {
    Get-Process Main7 -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "*\dh2coop\$tree\Bin\Main7.exe" } |
        Select-Object -First 1
}
function Send-Cmd([string]$tree, [string]$line) {
    [IO.File]::WriteAllLines((Join-Path $root "$tree\Bin\dh2coop.cmd"), [string[]]@($line))
}

& (Join-Path $root 'run-coop-local.ps1') -Players 2 -Map $Map -Combat | Out-Null
"pair up on $Map with Physics=1 AttackTarget=1; settling 15s"
Start-Sleep -Seconds 15
if ($ClientWalkMs -gt 0) {
    $c = Get-Game 'game2'
    & (Join-Path $PSScriptRoot 'game-input.ps1') -ProcessId $c.Id -Keys "W:$ClientWalkMs" | Out-Null
}
"host: coop_trigger $Trigger (arm, then fire)"
Send-Cmd 'game' "coop_trigger $Trigger"
Start-Sleep -Seconds 2
Send-Cmd 'game' "coop_trigger $Trigger"
Start-Sleep -Seconds $Watch
Push-Location $root
foreach ($t in 'game', 'game2') {
    $p = Get-Game $t
    if ($p) { & (Join-Path $PSScriptRoot 'capture-window.ps1') -ProcessId $p.Id -Out "re\notes\coop-local\combat-$t.png" -NoFocus 2>&1 | Out-Null }
    else { "$t is not running" }
}
Pop-Location
foreach ($t in 'game', 'game2') {
    "--- $t"
    Select-String -Path (Join-Path $root "$t\Bin\dh2coop.log") -Pattern 'combat:|attack target|unlinked physics|story: coop_trigger|exception|player \d left' |
        Select-Object -First 30 | ForEach-Object { $l = $_.Line; if ($l.Length -gt 200) { $l.Substring(0, 200) } else { $l } }
    Select-String -Path (Join-Path $root "$t\Bin\debug.log") -Pattern 'Access violation|Could not forward' -ErrorAction SilentlyContinue |
        Select-Object -First 3 | ForEach-Object { "debug: $($_.Line)" }
}
if (-not $Keep) {
    foreach ($t in 'game2', 'game') { $p = Get-Game $t; if ($p) { $p.CloseMainWindow() | Out-Null } }
    Start-Sleep -Seconds 5
    foreach ($t in 'game2', 'game') { $p = Get-Game $t; if ($p) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } }
    'closed'
}
