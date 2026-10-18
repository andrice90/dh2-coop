# Repeatable HUD check. Starts MAP2A, skips the opening cutscene with one Enter, captures the window.
# Usage: .\tools-src\hud-layout-test.ps1
# Exit 0 when the capture's compass, reticle, and status bar line up. Exit 1 when they do not.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $root 're\notes'
New-Item -ItemType Directory -Force $outDir | Out-Null
$shot = Join-Path $outDir 'hud-layout-map2a.png'

Get-CimInstance Win32_Process -Filter "Name='Main7.exe'" | ForEach-Object {
    if ($_.ExecutablePath -like '*\dh2coop\game\Bin\Main7.exe') {
        Stop-Process -Id $_.ProcessId -Force
    }
}
Start-Sleep -Milliseconds 600

& (Join-Path $root 'run-game.ps1') -ExtraArgs '-nointro -developper -cheats -spmap MAP2A'
$log = Join-Path $root 'game\Bin\dh2coop.log'
$deadline = (Get-Date).AddSeconds(30)
do {
    Start-Sleep -Milliseconds 300
    if ((Test-Path $log) -and (Select-String -Path $log -Pattern "Bsp\\MAP2A" -Quiet)) { break }
} while ((Get-Date) -lt $deadline)
if (-not (Select-String -Path $log -Pattern "Bsp\\MAP2A" -Quiet)) { throw 'MAP2A did not load' }

Start-Sleep -Seconds 3
$game = Get-CimInstance Win32_Process -Filter "Name='Main7.exe'" |
    Where-Object { $_.ExecutablePath -like '*\dh2coop\game\Bin\Main7.exe' } |
    Select-Object -First 1
if (-not $game) { throw 'game exited before the cutscene skip' }

$focused = $false
foreach ($try in 1..8) {
    try {
        & (Join-Path $root 'tools-src\game-input.ps1') -ProcessId $game.ProcessId -Keys 'RETURN'
        $focused = $true
        break
    } catch {
        Start-Sleep -Milliseconds 500
    }
}
if (-not $focused) { throw 'could not focus the game window to skip the cutscene' }
Start-Sleep -Seconds 3
& (Join-Path $root 'tools-src\capture-window.ps1') -ProcessId $game.ProcessId -Out $shot -NoFocus

py -I (Join-Path $root 'tools-src\hud_layout_check.py') $shot
exit $LASTEXITCODE
