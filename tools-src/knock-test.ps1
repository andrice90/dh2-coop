# Player knockdown test (re/notes/knockdown.md, combat.cpp): host knocks the client over, then the client knocks the
# host over, with coop_knock (no aiming). Screenshots ~1 s into each knock show the victim's camera and the puppet.
# Usage: .\tools-src\knock-test.ps1 [-Map MAP3B] [-Keep]
param(
    [string]$Map = 'MAP3B',
    [switch]$Keep
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

function Get-Game([string]$tree) {
    Get-Process Main7 -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$root\$tree\Bin\Main7.exe" } |
        Select-Object -First 1
}
function Send-Cmd([string]$tree, [string]$line) {
    [IO.File]::WriteAllLines((Join-Path $root "$tree\Bin\dh2coop.cmd"), [string[]]@($line))
}
function Shot([string]$tag) {
    Push-Location $root
    foreach ($t in 'game', 'game2') {
        $p = Get-Game $t
        if ($p) { & (Join-Path $PSScriptRoot 'capture-window.ps1') -ProcessId $p.Id -Out "re\notes\coop-local\knock-$tag-$t.png" -NoFocus 2>&1 | Out-Null }
    }
    Pop-Location
}

& (Join-Path $root 'run-coop-local.ps1') -Players 2 -Map $Map | Out-Null
"pair up on $Map; settling 20s"
Start-Sleep -Seconds 20
"host knocks player 1"
Send-Cmd 'game' 'coop_knock 1'
Start-Sleep -Milliseconds 1700
Shot 'client-down'
Start-Sleep -Seconds 4
Shot 'client-up'
"player 1 knocks the host"
Send-Cmd 'game2' 'coop_knock 0'
Start-Sleep -Milliseconds 1700
Shot 'host-down'
Start-Sleep -Seconds 4
Shot 'host-up'

foreach ($t in 'game', 'game2') {
    "--- $t"
    Select-String -Path (Join-Path $root "$t\Bin\dh2coop.log") -Pattern 'combat:|coop_knock|deadfrontup|syncguidegetup|exception' |
        ForEach-Object { $l = $_.Line; if ($l.Length -gt 200) { $l.Substring(0, 200) } else { $l } }
    Select-String -Path (Join-Path $root "$t\Bin\debug.log") -Pattern 'Access violation|Could not' -ErrorAction SilentlyContinue |
        Select-Object -First 4 | ForEach-Object { "debug: $($_.Line)" }
}
"screenshots: re\notes\coop-local\knock-*.png"
if (-not $Keep) {
    foreach ($t in 'game2', 'game') { $p = Get-Game $t; if ($p) { $p.CloseMainWindow() | Out-Null } }
    Start-Sleep -Seconds 5
    foreach ($t in 'game2', 'game') { $p = Get-Game $t; if ($p) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } }
    'closed'
}
