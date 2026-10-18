# Helpers for mission-flow tests (results screen, pause, map follow). Dot-source it:
#   . .\tools-src\flow-lib.ps1
#   Start-Flow -Map HUNTMAP4            # run-coop-local.ps1 with this checkout's port
#   Send-Cmd game 'coop_kill 120:1000'  # one console line per write, 1.2 s apart
#   Shot game 'kill'                    # the game's own back buffer (console coop_shot)
#   Find-Log game2 'events: applied'    # lines from that tree's dh2coop.log (shared read)
#   Stop-Flow                           # closes only this checkout's games
# Every process lookup matches this checkout's full path, never the bare name: other agents run games too.
$script:FlowRoot = Split-Path $PSScriptRoot -Parent
if (-not $script:FlowPort) { $script:FlowPort = 27640 }
$script:FlowShots = Join-Path $script:FlowRoot 're\notes\coop-local'

Add-Type -AssemblyName System.Drawing

function Get-FlowProcs {
    $want = @{}
    foreach ($t in 'game', 'game2', 'game3', 'game4') {
        $want[(Join-Path $script:FlowRoot "$t\Bin\Main7.exe").ToLowerInvariant()] = $t
    }
    $out = @{}
    foreach ($p in Get-Process Main7 -ErrorAction SilentlyContinue) {
        $path = try { $p.Path } catch { $null }
        if (-not $path) { continue }
        $tree = $want[$path.ToLowerInvariant()]
        if ($tree) { $out[$tree] = $p }
    }
    return $out
}

function Start-Flow([string]$Map = 'HUNTMAP4', [int]$Players = 2, [int]$LateJoin = 0) {
    $launcher = Join-Path $script:FlowRoot 'run-coop-local.ps1'
    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        & $launcher -Port $script:FlowPort -Map $Map -Players $Players -LateJoin $LateJoin 2>&1 |
            Where-Object { "$_" -match 'pid=|welcomed|ready|error|failed' } | ForEach-Object { "$_" } | Write-Host
    } finally {
        $ErrorActionPreference = $prev
    }
    $procs = Get-FlowProcs
    Write-Host ("flow: running " + (($procs.GetEnumerator() | Sort-Object Name | ForEach-Object { "$($_.Name)=$($_.Value.Id)" }) -join ' '))
}

function Add-FlowClient([int]$Slot, [string]$Map = 'HUNTMAP4') {
    $launcher = Join-Path $script:FlowRoot 'run-coop-local.ps1'
    & $launcher -Port $script:FlowPort -AddClient $Slot -Map $Map 2>&1 | ForEach-Object { "$_" } |
        Where-Object { $_ -match 'pid=|welcomed|error' } | Write-Host
}

function Send-Cmd([string]$Tree, [string[]]$Lines) {
    $path = Join-Path $script:FlowRoot "$Tree\Bin\dh2coop.cmd"
    foreach ($l in $Lines) {
        # The game reads and deletes the file every 0.5 s. Wait for the previous line to be taken.
        for ($i = 0; $i -lt 40 -and (Test-Path -LiteralPath $path); $i++) { Start-Sleep -Milliseconds 100 }
        [IO.File]::WriteAllLines($path, [string[]]@($l))
        Write-Host "cmd -> ${Tree}: $l"
        Start-Sleep -Milliseconds 1200
    }
}

# coop_shot saves the game's own back buffer (overlay included) as Bin\shots\<name>.bmp; here it becomes a PNG
# under re\notes\coop-local. A screen capture showed whatever window was in front (other agents' games, the IDE).
function Shot([string]$Tree, [string]$Tag) {
    $name = ("flow-{0}-{1}" -f $Tag, $Tree) -replace '[^A-Za-z0-9_-]', '_'
    $bmp = Join-Path $script:FlowRoot "$Tree\Bin\shots\$name.bmp"
    if (Test-Path -LiteralPath $bmp) { Remove-Item -LiteralPath $bmp -Force }
    $path = Join-Path $script:FlowRoot "$Tree\Bin\dh2coop.cmd"
    for ($i = 0; $i -lt 40 -and (Test-Path -LiteralPath $path); $i++) { Start-Sleep -Milliseconds 100 }
    [IO.File]::WriteAllLines($path, [string[]]@("coop_shot $name"))
    for ($i = 0; $i -lt 50 -and -not (Test-Path -LiteralPath $bmp); $i++) { Start-Sleep -Milliseconds 100 }
    if (-not (Test-Path -LiteralPath $bmp)) { Write-Host "shot ${Tree}: no file"; return }
    Start-Sleep -Milliseconds 200
    $img = [System.Drawing.Image]::FromFile($bmp)
    $out = Join-Path $script:FlowShots "$name.png"
    $img.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
    $img.Dispose()
    Write-Host "shot ${Tree} -> $out"
}

function Read-Shared([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return '' }
    $fs = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    try { return (New-Object System.IO.StreamReader($fs)).ReadToEnd() } finally { $fs.Dispose() }
}

function Find-Log([string]$Tree, [string]$Pattern, [int]$Last = 40, [string]$File = 'dh2coop.log') {
    $text = Read-Shared (Join-Path $script:FlowRoot "$Tree\Bin\$File")
    $lines = $text -split "`r?`n" | Where-Object { $_ -match $Pattern }
    $lines | Select-Object -Last $Last
}

function Wait-Log([string]$Tree, [string]$Pattern, [int]$Seconds = 60) {
    $end = (Get-Date).AddSeconds($Seconds)
    while ((Get-Date) -lt $end) {
        $hit = Find-Log $Tree $Pattern -Last 1
        if ($hit) { return $hit }
        Start-Sleep -Milliseconds 500
    }
    return $null
}

function Save-FlowLogs([string]$Tag) {
    foreach ($kv in (Get-ChildItem (Join-Path $script:FlowRoot 'game*') -Directory)) {
        foreach ($f in 'dh2coop.log', 'debug.log') {
            $src = Join-Path $kv.FullName "Bin\$f"
            if (-not (Test-Path -LiteralPath $src)) { continue }
            $dst = Join-Path $script:FlowShots ("flow-{0}-{1}-{2}" -f $Tag, $kv.Name, $f)
            [IO.File]::WriteAllText($dst, (Read-Shared $src))
        }
    }
    Write-Host "flow: logs saved as flow-$Tag-*"
}

function Stop-Flow {
    $procs = Get-FlowProcs
    foreach ($p in $procs.Values) { try { if (-not $p.HasExited) { [void]$p.CloseMainWindow() } } catch { } }
    Start-Sleep 5
    foreach ($p in $procs.Values) { try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { } }
    Write-Host "flow: stopped $($procs.Count) game(s)"
}
