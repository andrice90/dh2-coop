# Automated world-event test: two local instances, close CHOOSE ITEM on both, kill a deer on the host
# (coop_kill through Bin\dh2coop.cmd), claim it from the client (coop_claim), capture both windows, then
# print the events/animals log lines.
# Usage: .\tools-src\event-test.ps1 [-Animal 120] [-Settle 25] [-ClaimFrom client|host|none] [-Keep]
param(
    [int]$Animal = 120,
    [int]$Settle = 25,
    [ValidateSet('client', 'host', 'none')][string]$ClaimFrom = 'client',
    [switch]$Keep
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$shots = 're\notes\coop-local'
$keys = Join-Path $PSScriptRoot 'game-input.ps1'
$capture = Join-Path $PSScriptRoot 'capture-window.ps1'
$stamp = Get-Date -Format 'HHmmss'
$menu = 'RETURN', 'WAIT:700', 'LEFT', 'WAIT:400', 'RETURN', 'WAIT:1500'

function Shot([System.Diagnostics.Process]$p, [string]$tag) {
    & $capture -ProcessId $p.Id -Out "$shots\event-$stamp-$tag.png" -NoFocus | Write-Host
}
function Send-Cmd([string]$tree, [string[]]$lines) {
    $path = Join-Path $root "$tree\Bin\dh2coop.cmd"
    [IO.File]::WriteAllLines($path, $lines)
    Write-Host "cmd -> ${tree}: $($lines -join '; ')"
}

& (Join-Path $root 'run-coop-local.ps1') | Out-Null
$procs = Get-Process Main7
$p1 = $procs | Where-Object { $_.Path -like '*\game\Bin\Main7.exe' } | Select-Object -First 1
$p2 = $procs | Where-Object { $_.Path -like '*\game2\Bin\Main7.exe' } | Select-Object -First 1
Write-Host "host pid=$($p1.Id) client pid=$($p2.Id); settle ${Settle}s"
Start-Sleep $Settle
try {
    & $keys -ProcessId $p2.Id -Keys $menu
    & $keys -ProcessId $p1.Id -Keys $menu
    Start-Sleep 2
    Shot $p1 'host-0'
    Shot $p2 'client-0'
    Send-Cmd 'game' "coop_kill $Animal"
    Start-Sleep 4
    Shot $p1 'host-1killed'
    Shot $p2 'client-1killed'
    if ($ClaimFrom -ne 'none') {
        $tree = if ($ClaimFrom -eq 'client') { 'game2' } else { 'game' }
        Send-Cmd $tree "coop_claim $Animal"
        for ($i = 0; $i -lt 4; $i++) {
            Start-Sleep 2
            Shot $p1 "host-2claim$i"
            Shot $p2 "client-2claim$i"
        }
    }
} finally {
    if (-not $Keep) {
        foreach ($p in @($p1, $p2)) { try { if (-not $p.HasExited) { [void]$p.CloseMainWindow() } } catch { } }
        Start-Sleep 5
        foreach ($p in @($p1, $p2)) { try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { } }
    }
}
foreach ($tree in 'game', 'game2') {
    Write-Host "---- $tree"
    Get-Content "$tree\Bin\dh2coop.log" |
        Where-Object { $_ -match 'events:|coop_kill|coop: cmd file|present death|death (commit|event)|hook cMission|hook cObjAnimal|joined|welcomed' } |
        Select-Object -First 80 | Write-Host
}
exit 0
