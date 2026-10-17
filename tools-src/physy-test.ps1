# Finds [Puppets] PhysicsKeyY: with Physics=1 the puppet is drawn from its physics body, which the "position" key
# places. Starts a -Combat pair, walks the client ahead into the host's view, then screenshots the host window for
# each offset (console coop_physy) and prints the drift lines. Usage: .\tools-src\physy-test.ps1 [-Map MAP3B] [-Keep]
param(
    [string]$Map = 'MAP3B',
    [double[]]$Offsets = @(0, 0.9, 1.8, -0.9),
    [switch]$Keep
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
function Get-Game([string]$tree) {
    Get-Process Main7 -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$root\$tree\Bin\Main7.exe" } |
        Select-Object -First 1
}
& (Join-Path $root 'run-coop-local.ps1') -Players 2 -Map $Map -Combat | Out-Null
Start-Sleep -Seconds 15
$c = Get-Game 'game2'
& (Join-Path $PSScriptRoot 'game-input.ps1') -ProcessId $c.Id -Keys 'W:1500' | Out-Null
Start-Sleep -Seconds 2
$h = Get-Game 'game'
Push-Location $root
foreach ($k in $Offsets) {
    [IO.File]::WriteAllLines((Join-Path $root 'game\Bin\dh2coop.cmd'), [string[]]@("coop_physy $k"))
    Start-Sleep -Seconds 2
    & (Join-Path $PSScriptRoot 'capture-window.ps1') -ProcessId $h.Id -Out "re\notes\coop-local\physy-$k.png" -NoFocus 2>&1 | Out-Null
    "offset $k -> re\notes\coop-local\physy-$k.png"
}
Pop-Location
Select-String -Path (Join-Path $root 'game\Bin\dh2coop.log') -Pattern 'PhysicsKeyY|physics drift|collision off' |
    Select-Object -Last 14 | ForEach-Object { $_.Line.Substring(40) }
if (-not $Keep) {
    foreach ($t in 'game2', 'game') { $p = Get-Game $t; if ($p) { $p.CloseMainWindow() | Out-Null } }
    Start-Sleep -Seconds 5
    foreach ($t in 'game2', 'game') { $p = Get-Game $t; if ($p) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } }
    'closed'
}
