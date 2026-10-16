# Automated ghost test: two local instances, close the CHOOSE ITEM menu on both (Enter), walk the host's
# hunter with scripted keys, capture both windows, then print the puppet / remote log lines.
# Usage: .\tools-src\move-test.ps1 [-Walk 'W:3000'] [-Settle 30] [-Keep]
#   -Walk   key script for the host after the menu closes (see tools-src\game-input.ps1)
#   -Keep   leave both games running afterwards
param(
    [string[]]$Walk = @('W:2500', 'WAIT:1500', 'S:1500', 'WAIT:1000', 'A:1500', 'WAIT:500', 'D:1500'),
    [int]$Settle = 30,
    [switch]$Keep
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$shots = 're\notes\coop-local'
$keys = Join-Path $PSScriptRoot 'game-input.ps1'
$capture = Join-Path $PSScriptRoot 'capture-window.ps1'
$stamp = Get-Date -Format 'HHmmss'
# CHOOSE ITEM: Enter picks the highlighted weapon, Left moves the confirm dialog to YES, Enter accepts (user, 2026-10-16).
$menu = 'RETURN', 'WAIT:700', 'LEFT', 'WAIT:400', 'RETURN', 'WAIT:1500'

function Shot([System.Diagnostics.Process]$p, [string]$tag) {
    & $capture -ProcessId $p.Id -Out "$shots\move-$stamp-$tag.png" -NoFocus | Write-Host
}

& (Join-Path $root 'run-coop-local.ps1') | Out-Null
$procs = Get-Process Main7
$p1 = $procs | Where-Object { $_.Path -like '*\game\Bin\Main7.exe' } | Select-Object -First 1
$p2 = $procs | Where-Object { $_.Path -like '*\game2\Bin\Main7.exe' } | Select-Object -First 1
Write-Host "game pid=$($p1.Id) game2 pid=$($p2.Id); settle ${Settle}s"
Start-Sleep $Settle
$t0 = Get-Date
try {
    Shot $p1 'game-0menu'
    & $keys -ProcessId $p2.Id -Keys $menu
    & $keys -ProcessId $p1.Id -Keys $menu
    Shot $p1 'game-1nomenu'
    Shot $p2 'game2-1start'
    & $keys -ProcessId $p1.Id -Keys $Walk
    Shot $p1 'game-2walked'
    Start-Sleep -Milliseconds 300
    Shot $p2 'game2-2walked'
    # Second sample while moving: walk again and grab the client view mid-walk.
    $job = Start-Job -ScriptBlock { param($s, $id) & $s -ProcessId $id -Keys 'W:3000' } -ArgumentList $keys, $p1.Id
    Start-Sleep -Milliseconds 1500
    Shot $p2 'game2-3midwalk'
    Wait-Job $job | Receive-Job | Write-Host
} finally {
    if (-not $Keep) {
        foreach ($p in @($p1, $p2)) { try { if (-not $p.HasExited) { [void]$p.CloseMainWindow() } } catch { } }
        Start-Sleep 5
        foreach ($p in @($p1, $p2)) { try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { } }
    }
}
foreach ($tree in 'game', 'game2') {
    Write-Host "---- $tree"
    Get-Content "$tree\Bin\dh2coop.log" | Where-Object { $_ -match 'puppets: (entity|spawned)|coop: remote' } |
        Where-Object { $_ -match '^\[[\d-]+ ([\d:]+)' -and [datetime]::ParseExact($Matches[1], 'HH:mm:ss', $null).TimeOfDay -ge $t0.AddSeconds(-1).TimeOfDay } |
        Select-Object -First 60 | Write-Host
}
exit 0
