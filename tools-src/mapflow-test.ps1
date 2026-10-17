# Automated map-flow test: the host changes maps through console commands (Bin\dh2coop.cmd) and the
# client must follow (kMapStart). Steps: replay the same map, switch to another hunt map, go to the menu.
# Usage: .\tools-src\mapflow-test.ps1 [-Settle 25] [-Other HUNTMAP1] [-Keep]
param(
    [int]$Settle = 25,
    [string]$Other = 'HUNTMAP1',
    [int]$StepWait = 20,
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
    & $capture -ProcessId $p.Id -Out "$shots\mapflow-$stamp-$tag.png" -NoFocus | Write-Host
}
function Send-Cmd([string]$tree, [string[]]$lines) {
    [IO.File]::WriteAllLines((Join-Path $root "$tree\Bin\dh2coop.cmd"), $lines)
    Write-Host "cmd -> ${tree}: $($lines -join '; ')"
}
function Step([string]$tag, [string]$cmd) {
    Send-Cmd 'game' $cmd
    Start-Sleep $StepWait
    Shot $script:p1 "host-$tag"
    Shot $script:p2 "client-$tag"
}

& (Join-Path $root 'run-coop-local.ps1') | Out-Null
$procs = Get-Process Main7
$script:p1 = $procs | Where-Object { $_.Path -like '*\game\Bin\Main7.exe' } | Select-Object -First 1
$script:p2 = $procs | Where-Object { $_.Path -like '*\game2\Bin\Main7.exe' } | Select-Object -First 1
Write-Host "host pid=$($p1.Id) client pid=$($p2.Id); settle ${Settle}s"
Start-Sleep $Settle
try {
    & $keys -ProcessId $p2.Id -Keys $menu
    & $keys -ProcessId $p1.Id -Keys $menu
    Step '1replay' 'spmap HUNTMAP4'
    Step "2$Other" "spmap $Other"
    Step '3menu' 'menumap MENUMAP'
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
        Where-Object { $_ -match 'StartGame|events: (journal|commit kind=4|following|host map)|mirror first place|client received|exception|puppets: spawned' } |
        Select-Object -First 60 | Write-Host
}
exit 0
