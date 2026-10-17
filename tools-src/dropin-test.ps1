# Drop-in test: host + one client on a story map, the host walks away, then a third player joins late.
# Checks the late joiner's catch-up (re/notes/lead-dropin.md): sync request, replayed commits, skipped
# cutscenes, teleport to the host. Prints the decisive log lines and saves a screenshot of every window.
# Usage: .\tools-src\dropin-test.ps1 [-Map MAP12C] [-WaitBefore 25] [-WalkMs 8000] [-WaitAfter 30] [-Keep]
#   -Keep  leave the three games running at the end (for a user to look at)
param(
    [string]$Map = 'MAP12C',
    [int]$WaitBefore = 25,
    [int]$WalkMs = 8000,
    [int]$WaitAfter = 30,
    [switch]$Keep
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$shots = Join-Path $root 're\notes\coop-local'
New-Item -ItemType Directory -Force $shots | Out-Null

function Get-Game([string]$tree) {
    Get-Process Main7 -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$root\$tree\Bin\Main7.exe" } |
        Select-Object -First 1
}

& (Join-Path $root 'run-coop-local.ps1') -Players 2 -Map $Map | Out-Null
$hostProc = Get-Game 'game'
if (-not $hostProc) { throw 'host did not start' }
"pair up on $Map; waiting ${WaitBefore}s"
Start-Sleep -Seconds $WaitBefore
if ($WalkMs -gt 0) {
    "host walks forward for $WalkMs ms"
    & (Join-Path $PSScriptRoot 'game-input.ps1') -ProcessId $hostProc.Id -Keys "W:$WalkMs" | Out-Null
}
"adding client 3"
& (Join-Path $root 'run-coop-local.ps1') -AddClient 3 -Map $Map | Out-Null
Start-Sleep -Seconds $WaitAfter

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
# capture-window.ps1 joins -Out onto the cwd (PS 5.1 Join-Path mangles a rooted child): pass a relative path.
Push-Location $root
foreach ($t in 'game', 'game2', 'game3') {
    $p = Get-Game $t
    if ($p) {
        & (Join-Path $PSScriptRoot 'capture-window.ps1') -ProcessId $p.Id -Out "re\notes\coop-local\dropin-$stamp-$t.png" -NoFocus 2>&1 | Out-Null
    } else {
        "$t is not running"
    }
}
Pop-Location
"--- host"
Select-String -Path (Join-Path $root 'game\Bin\dh2coop.log') -Pattern 'events: sync player|joined|left|story: host skipped' |
    ForEach-Object { $_.Line }
"--- late joiner (game3)"
Select-String -Path (Join-Path $root 'game3\Bin\dh2coop.log') -Pattern 'events: (request kind=9|synced|catch-up|host map|following|sync done)|story: (will skip|skipped|defer|applied host fire)|teleport|exception|welcomed' |
    ForEach-Object { $_.Line }
"--- debug.log errors"
foreach ($t in 'game', 'game2', 'game3') {
    Select-String -Path (Join-Path $root "$t\Bin\debug.log") -Pattern 'Access violation|EXCEPTION|Could not' -ErrorAction SilentlyContinue |
        Select-Object -First 3 | ForEach-Object { "$t : $($_.Line)" }
}
"screenshots: re\notes\coop-local\dropin-$stamp-*.png"
if (-not $Keep) {
    foreach ($t in 'game3', 'game2', 'game') {
        $p = Get-Game $t
        if ($p) { $p.CloseMainWindow() | Out-Null }
    }
    Start-Sleep -Seconds 5
    foreach ($t in 'game3', 'game2', 'game') {
        $p = Get-Game $t
        if ($p) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }
    }
    'closed'
}
