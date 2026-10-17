# MAP2A mountain lion vs a client far from the host (two-PC test 2026-10-17: lion vanished on the host, client crashed).
# Host fires cScrTask04 (arm, fire), the client stands next to the lion, the host goes far away; then -Watch seconds.
# Prints the host's combat / lion lines and the client's combat lines; screenshots under re\notes\coop-local\lion-*.png.
# Usage: .\tools-src\lion-test.ps1 [-Watch 60] [-Keep]
param([int]$Watch = 60, [switch]$Keep)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
function Get-Game([string]$tree) {
    Get-Process Main7 -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "*\dh2coop\$tree\Bin\Main7.exe" } |
        Select-Object -First 1
}
function Send-Cmd([string]$tree, [string]$line) {
    [IO.File]::WriteAllLines((Join-Path $root "$tree\Bin\dh2coop.cmd"), [string[]]@($line))
    Start-Sleep -Milliseconds 1200
}
& (Join-Path $root 'run-coop-local.ps1') -Players 2 -Map MAP2A | Out-Null
Start-Sleep -Seconds 12
Send-Cmd 'game' 'coop_trigger cScrTask04'
Send-Cmd 'game' 'coop_trigger cScrTask04'
Send-Cmd 'game2' 'coop_tp -46:41.7:30'
Send-Cmd 'game' 'coop_tp -139:36.6:109.7'
Push-Location $root
for ($i = 1; $i -le [Math]::Max(1, [int]($Watch / 10)); $i++) {
    Start-Sleep -Seconds 10
    foreach ($t in 'game', 'game2') {
        $p = Get-Game $t
        if ($p) { & (Join-Path $PSScriptRoot 'capture-window.ps1') -ProcessId $p.Id -Out "re\notes\coop-local\lion-$i-$t.png" -NoFocus 2>&1 | Out-Null }
        else { "$t is not running at step $i" }
    }
}
Pop-Location
"--- host"
Select-String -Path (Join-Path $root 'game\Bin\dh2coop.log') -Pattern 'combat:|id=157 state|sync role=host id=157|player \d left' |
    Select-Object -Last 30 | ForEach-Object { $_.Line.Substring(12, [Math]::Min(170, $_.Line.Length - 12)) }
"--- client"
Select-String -Path (Join-Path $root 'game2\Bin\dh2coop.log') -Pattern 'combat:|exception' | Select-Object -Last 15 |
    ForEach-Object { $_.Line.Substring(12, [Math]::Min(170, $_.Line.Length - 12)) }
if (-not $Keep) {
    foreach ($t in 'game2', 'game') { $p = Get-Game $t; if ($p) { $p.CloseMainWindow() | Out-Null } }
    Start-Sleep -Seconds 5
    foreach ($t in 'game2', 'game') { $p = Get-Game $t; if ($p) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } }
    'closed'
}
