# Boss / sync-attack fight on two local windows (re/notes/yeti-bosses.md).
# Starts host + client with run-coop-local, turns god mode on (hits still reach each player's own hunter), fires the
# given host triggers, then runs the phases: each phase teleports the host and the client and watches for -PhaseSec.
# Every 10 s both windows are captured to re\notes\coop-local\<Prefix>-<phase>-<n>-<tree>.png. At the end the key log
# lines are printed and both logs are copied next to the screenshots.
# Usage:
#   .\tools-src\boss-test.ps1 -Map MAP12C -Port 27460 -Phases 'stay|stay','-187.6:1.6:-121|stay','stay|-187.6:1.6:-121'
#   .\tools-src\boss-test.ps1 -Map MAP12B -Triggers 'cScrTask03' -Phases 'stay|stay' -Keep
# A phase is '<host>|<client>': 'stay', 'tohost' (client only), or 'x:y:z' (metres, as the logs print them).
param(
    [string]$Map = 'MAP12C',
    [int]$Port = 27460,
    [string[]]$Triggers = @(),
    [string[]]$Phases = @('stay|stay'),
    [int]$StartWait = 12,
    [int]$PhaseSec = 40,
    [string]$Prefix = 'yeti',
    [switch]$NoGod,
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
function Send-Cmd([string]$tree, [string]$line) {
    [IO.File]::WriteAllLines((Join-Path $root "$tree\Bin\dh2coop.cmd"), [string[]]@($line))
    Start-Sleep -Milliseconds 1200
}
function Read-Shared([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) { return @() }
    $fs = [IO.File]::Open($path, 'Open', 'Read', 'ReadWrite')
    try { $sr = New-Object IO.StreamReader($fs); return $sr.ReadToEnd() -split "`r?`n" } finally { $fs.Dispose() }
}
function Move-Player([string]$tree, [string]$where) {
    if ($where -eq 'stay' -or -not $where) { return }
    if ($where -eq 'tohost') { Send-Cmd $tree 'coop_tohost'; return }
    Send-Cmd $tree "coop_tp $where"
}
$stamp = Get-Date -Format 'HHmmss'
& (Join-Path $root 'run-coop-local.ps1') -Players 2 -Map $Map -Port $Port | Out-Null
Start-Sleep -Seconds $StartWait
if (-not $NoGod) { Send-Cmd 'game' 'godMode 1'; Send-Cmd 'game2' 'godMode 1' }
foreach ($t in $Triggers) { Send-Cmd 'game' "coop_trigger $t"; Send-Cmd 'game' "coop_trigger $t" }
Push-Location $root
$alive = @{ game = $true; game2 = $true }
for ($ph = 0; $ph -lt $Phases.Count; $ph++) {
    $parts = $Phases[$ph].Split('|')
    "phase $ph host=$($parts[0]) client=$($parts[1]) at $(Get-Date -Format HH:mm:ss)"
    Move-Player 'game' $parts[0]
    if ($parts.Count -gt 1) { Move-Player 'game2' $parts[1] }
    for ($i = 1; $i -le [Math]::Max(1, [int]($PhaseSec / 10)); $i++) {
        Start-Sleep -Seconds 10
        foreach ($t in 'game', 'game2') {
            $p = Get-Game $t
            if ($p) {
                & (Join-Path $PSScriptRoot 'capture-window.ps1') -ProcessId $p.Id -Out "re\notes\coop-local\$Prefix-$stamp-p$ph-$i-$t.png" -NoFocus -Print 2>&1 | Out-Null
            } elseif ($alive[$t]) {
                $alive[$t] = $false
                "$t EXITED during phase $ph step $i ($(Get-Date -Format HH:mm:ss))"
            }
        }
    }
}
Pop-Location
foreach ($t in 'game', 'game2') {
    foreach ($f in 'dh2coop.log', 'debug.log') {
        $src = Join-Path $root "$t\Bin\$f"
        if (Test-Path $src) { Copy-Item $src (Join-Path $shots "$Prefix-$stamp-$t-$f") -Force }
    }
}
$pat = 'sync attack|plays the host|unlinked|combat:|faulted|stops body|thaw|exception|mission message|applied host fire|died|death'
"--- host"
Read-Shared (Join-Path $root 'game\Bin\dh2coop.log') | Where-Object { $_ -match $pat } | Select-Object -Last 40 |
    ForEach-Object { $_.Substring([Math]::Min(12, $_.Length)) }
"--- client"
Read-Shared (Join-Path $root 'game2\Bin\dh2coop.log') | Where-Object { $_ -match $pat } | Select-Object -Last 40 |
    ForEach-Object { $_.Substring([Math]::Min(12, $_.Length)) }
foreach ($t in 'game', 'game2') {
    $dbg = Read-Shared (Join-Path $root "$t\Bin\debug.log")
    $fwd = @($dbg | Where-Object { $_ -match 'Could not forward' }).Count
    $av = @($dbg | Where-Object { $_ -match 'Access violation' }).Count
    "$t debug.log: forward errors=$fwd access violations=$av alive=$([bool](Get-Game $t))"
}
"logs: $shots\$Prefix-$stamp-*"
if (-not $Keep) {
    foreach ($t in 'game2', 'game') { $p = Get-Game $t; if ($p) { $p.CloseMainWindow() | Out-Null } }
    Start-Sleep -Seconds 5
    foreach ($t in 'game2', 'game') { $p = Get-Game $t; if ($p) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } }
    'closed'
}
