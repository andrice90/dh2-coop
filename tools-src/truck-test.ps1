# Vehicle / ride tests for the MAP4A and MAP5 trucks (vehicles agent, re/notes/vehicles.md).
# Starts host + client from this checkout, runs one scenario through the dh2coop.cmd command files, captures
# both windows every -Every seconds under re\notes\coop-local\truck-<scenario>-<n>-<tree>.png, prints the
# vehicles / story / combat lines, then closes this checkout's games (by full path).
# Scenarios:
#   map5-host     MAP5: host walks into cScrTask06 (cutscene 3, boarding); client stays at the camp.
#   map5-client   MAP5: client walks into cScrTask06 first; host stays.
#   map5-both     MAP5: both walk into cScrTask06 (host first, client 2 s later).
#   map4a         MAP4A: the ride from the start (both board in the map-start cutscene).
#   map4a-late    MAP4A: client joins -LateJoin seconds into the ride.
# Usage: .\tools-src\truck-test.ps1 -Scenario map5-host [-Watch 90] [-Every 10] [-Port 27650] [-Keep]
#        [-Cmd 'game2:coop_down@40','game:coop_knock 1@50']   extra commands: <tree>:<line>@<seconds after start>
param(
    [Parameter(Mandatory)][string]$Scenario,
    [int]$Watch = 90,
    [int]$Every = 10,
    [int]$Port = 27650,
    [int]$LateJoin = 25,
    [string[]]$Cmd = @(),
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
    "  [{0:N0}s] {1} <- {2}" -f ((Get-Date) - $script:t0).TotalSeconds, $tree, $line
    Start-Sleep -Milliseconds 1200
}
function Wait-Log([string]$tree, [string]$pattern, [int]$timeoutSec) {
    $path = Join-Path $root "$tree\Bin\dh2coop.log"
    $deadline = (Get-Date).AddSeconds($timeoutSec)
    while ((Get-Date) -lt $deadline) {
        try {
            $fs = [IO.File]::Open($path, 'Open', 'Read', 'ReadWrite')
            $txt = (New-Object IO.StreamReader($fs)).ReadToEnd()
            $fs.Close()
            if ($txt -match $pattern) { return $true }
        } catch {}
        Start-Sleep -Milliseconds 500
    }
    "  timeout waiting for /$pattern/ in $tree"
    return $false
}
function Read-Log([string]$tree) {
    $fs = [IO.File]::Open((Join-Path $root "$tree\Bin\dh2coop.log"), 'Open', 'Read', 'ReadWrite')
    $txt = (New-Object IO.StreamReader($fs)).ReadToEnd()
    $fs.Close()
    return $txt -split "`r?`n"
}

$map = if ($Scenario -like 'map5*') { 'MAP5' } else { 'MAP4A' }
$runArgs = @{ Players = 2; Map = $map; Port = $Port }
if ($Scenario -eq 'map4a-late') { $runArgs.LateJoin = $LateJoin }
$script:t0 = Get-Date
& (Join-Path $root 'run-coop-local.ps1') @runArgs | Out-Null
$script:t0 = Get-Date
"started $map ($Scenario)"

# MAP5 cScrTask06 volume (BSP 60592 111 13580 cm). Both start at the camp spawn.
$task06 = 'coop_tp 605.9:2:135.8'
switch ($Scenario) {
    'map5-host' {
        [void](Wait-Log 'game' "trigger FIRE id=\d+ name='cScrTask01'" 60)
        Start-Sleep -Seconds 3
        Send-Cmd 'game' $task06
    }
    'map5-client' {
        [void](Wait-Log 'game2' "applied host fire 'cScrTask01'|trigger FIRE id=\d+ name='cScrTask01'" 60)
        Start-Sleep -Seconds 3
        Send-Cmd 'game2' $task06
    }
    'map5-both' {
        [void](Wait-Log 'game' "trigger FIRE id=\d+ name='cScrTask01'" 60)
        Start-Sleep -Seconds 3
        Send-Cmd 'game' $task06
        Start-Sleep -Seconds 1
        Send-Cmd 'game2' $task06
    }
    default { }
}

$pending = @()
foreach ($c in $Cmd) {
    if ($c -match '^(game\d?):(.+)@(\d+)$') { $pending += [pscustomobject]@{ Tree = $Matches[1]; Line = $Matches[2]; At = [int]$Matches[3] } }
}
$n = 0
$start = Get-Date
$nextShot = $start.AddSeconds($Every)
while (((Get-Date) - $start).TotalSeconds -lt $Watch) {
    $el = ((Get-Date) - $start).TotalSeconds
    foreach ($p in @($pending | Where-Object { $_.At -le $el })) {
        Send-Cmd $p.Tree $p.Line
        $pending = @($pending | Where-Object { $_ -ne $p })
    }
    if ((Get-Date) -ge $nextShot) {
        $n++
        foreach ($t in 'game', 'game2') {
            $g = Get-Game $t
            if ($g) {
                & (Join-Path $PSScriptRoot 'capture-window.ps1') -ProcessId $g.Id -Out (Join-Path $shots "truck-$Scenario-$n-$t.png") -NoFocus -PrintWindow 2>&1 | Out-Null
            } else { "  $t is not running at shot $n" }
        }
        $nextShot = $nextShot.AddSeconds($Every)
    }
    Start-Sleep -Milliseconds 250
}

$pat = 'vehicles:|story: (\[|applied|client|defer|cutscene|will skip|skipped)|coop: (teleport|F9)|combat: |exception|puppets: (puppet .* (rides|left)|rider \d seated=)|Could not forward'
foreach ($t in 'game', 'game2') {
    "--- $t"
    Read-Log $t | Where-Object { $_ -match $pat } | Select-Object -Last 70 | ForEach-Object {
        $_.Substring([Math]::Min(12, $_.Length), [Math]::Min(230, [Math]::Max(0, $_.Length - 12)))
    }
}
if (-not $Keep) {
    foreach ($t in 'game2', 'game') { $g = Get-Game $t; if ($g) { [void]$g.CloseMainWindow() } }
    Start-Sleep -Seconds 5
    foreach ($t in 'game2', 'game') { $g = Get-Game $t; if ($g) { Stop-Process -Id $g.Id -Force -ErrorAction SilentlyContinue } }
    'closed'
}
