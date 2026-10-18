# Two-window replay test (re/notes/replay-crash.md): host (game) and one client (game2) from this checkout. The
# host runs each -Steps map with `spmap` through game\Bin\dh2coop.cmd; the client must follow every start,
# same-map replays included, and no teardown may crash. Logs are copied to re\notes\coop-local\.
# Usage: .\tools-src\replay-coop-test.ps1 [-Map MAP1] [-Steps MAP1,MAP1,MAP2A,MAP1] [-Port 27450] [-Settle 10] [-Set 'Puppets.Weapons=0']
param(
    [string]$Map = 'MAP1',
    [string[]]$Steps = @('MAP1', 'MAP1', 'MAP2A', 'MAP1'),
    [int]$Port = 27450,
    [int]$Settle = 10,
    [int]$LoadTimeout = 120,
    [string]$Tag = 'coop',
    [string[]]$Set = @()   # ini overrides passed to run-coop-local.ps1 -Set
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$outDir = Join-Path $root 're\notes\coop-local'
$stamp = Get-Date -Format 'HHmmss'
$trees = @('game', 'game2')

function Read-Shared([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return '' }
    try {
        $fs = [IO.File]::Open($Path, 'Open', 'Read', 'ReadWrite')
        try { return (New-Object IO.StreamReader($fs)).ReadToEnd() } finally { $fs.Dispose() }
    } catch { return '' }
}

function Get-Procs {
    $map = @{}
    foreach ($t in $trees) {
        $exe = [IO.Path]::GetFullPath((Join-Path $root "$t\Bin\Main7.exe"))
        foreach ($p in @(Get-Process Main7 -ErrorAction SilentlyContinue)) {
            $path = $null; try { $path = $p.Path } catch { }
            if ($path -and $path -ieq $exe) { $map[$t] = $p }
        }
    }
    return $map
}

function Get-State([string]$Tree) {
    $bin = Join-Path $root "$Tree\Bin"
    $d = Read-Shared (Join-Path $bin 'debug.log')
    $m = Read-Shared (Join-Path $bin 'dh2coop.log')
    $crash = [regex]::Match($d, '(?m)^.*(caught in function|Access violation).*$')
    [pscustomobject]@{
        Loads = ([regex]::Matches($d, 'loading time :')).Count
        Crash = $(if ($crash.Success) { $crash.Value.Trim() } else { $null })
        Mod = $m
    }
}

& (Join-Path $root 'run-coop-local.ps1') -Map $Map -Port $Port -NoArrange -Set $Set | Out-Null
$procs = Get-Procs
if ($procs.Count -ne 2) { throw "expected 2 games from this checkout, found $($procs.Count)" }
Write-Host "[$Tag] host pid=$($procs['game'].Id) client pid=$($procs['game2'].Id)"
$result = 'ok'
$done = @($Map)

function Wait-Both([int[]]$Want, [string]$What) {
    $deadline = (Get-Date).AddSeconds($LoadTimeout)
    while ((Get-Date) -lt $deadline) {
        $ok = $true
        for ($i = 0; $i -lt 2; $i++) {
            $p = $procs[$trees[$i]]
            $p.Refresh()
            $s = Get-State $trees[$i]
            if ($s.Crash) { return "$($trees[$i]) crash: $($s.Crash)" }
            if ($p.HasExited) { return "$($trees[$i]) exited" }
            if ($s.Loads -lt $Want[$i]) { $ok = $false }
        }
        if ($ok) { return $null }
        Start-Sleep -Milliseconds 500
    }
    return "timeout waiting for $What"
}

function Settle-Both {
    $end = (Get-Date).AddSeconds($Settle)
    while ((Get-Date) -lt $end) {
        foreach ($t in $trees) {
            $s = Get-State $t
            if ($s.Crash) { return "$t crash: $($s.Crash)" }
            $procs[$t].Refresh()
            if ($procs[$t].HasExited) { return "$t exited" }
        }
        Start-Sleep -Milliseconds 500
    }
    return $null
}

try {
    $want = @((Get-State 'game').Loads, (Get-State 'game2').Loads)
    $err = Wait-Both @(1, 1) "first map"
    if (-not $err) { $err = Settle-Both }
    if ($err) { $result = "first map: $err" }
    else {
        foreach ($s in $Steps) {
            $want = @(((Get-State 'game').Loads + 1), ((Get-State 'game2').Loads + 1))
            [IO.File]::WriteAllText((Join-Path $root 'game\Bin\dh2coop.cmd'), "spmap $s`r`n")
            Write-Host "[$Tag] host: spmap $s"
            $err = Wait-Both $want "both on $s"
            if (-not $err) { $err = Settle-Both }
            if ($err) { $result = "spmap ${s}: $err"; break }
            $done += $s
        }
    }
} finally {
    foreach ($t in $trees) { try { $procs[$t].Refresh(); if (-not $procs[$t].HasExited) { [void]$procs[$t].CloseMainWindow() } } catch { } }
    Start-Sleep 5
    foreach ($t in $trees) { try { $procs[$t].Refresh(); if (-not $procs[$t].HasExited) { Stop-Process -Id $procs[$t].Id -Force } } catch { } }
    Start-Sleep 1
    foreach ($t in $trees) {
        $bin = Join-Path $root "$t\Bin"
        foreach ($f in 'debug.log', 'dh2coop.log') {
            if (Test-Path (Join-Path $bin $f)) { Copy-Item (Join-Path $bin $f) (Join-Path $outDir "replay-$Tag-$stamp-$t-$f") -Force }
        }
    }
}
Write-Host "[$Tag] maps done: $($done -join ' -> ')"
Write-Host "[$Tag] RESULT: $result"
foreach ($t in $trees) {
    Write-Host "---- $t"
    Get-Content (Join-Path $outDir "replay-$Tag-$stamp-$t-dh2coop.log") |
        Where-Object { $_ -match 'StartGame\(|map loader:|following host|already loaded|commit kind=4|empty slot|still use|exception|coop: (welcomed|player)' } |
        ForEach-Object { $_ -replace '^\[\d{4}-\d\d-\d\d ', '[' } | Write-Host
}
