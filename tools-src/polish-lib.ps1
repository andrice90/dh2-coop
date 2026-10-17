# Helpers for the puppet-polish tests (dot-source: . .\tools-src\polish-lib.ps1).
# Everything matches processes by this checkout's full game path, never by name alone.
$script:Root = Split-Path $PSScriptRoot -Parent
$script:Port = 27500
$script:ShotDir = Join-Path $script:Root 're\notes\coop-local'
New-Item -ItemType Directory -Force $script:ShotDir | Out-Null

if (-not ('PolishWin' -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class PolishWin {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr after, int X, int Y, int cx, int cy, uint flags);
}
"@
}
[void][PolishWin]::SetProcessDPIAware()

function Get-Game([string]$tree) {
    Get-Process Main7 -ErrorAction SilentlyContinue |
        Where-Object { $_.Path -ieq (Join-Path $script:Root "$tree\Bin\Main7.exe") } | Select-Object -First 1
}

function Send-Cmd([string]$tree, [string[]]$lines) {
    [IO.File]::WriteAllLines((Join-Path $script:Root "$tree\Bin\dh2coop.cmd"), $lines)
}

# Moves this checkout's windows to the bottom-right of the (3440x1440) screen, away from the 0,0 grid the other
# worktrees' launchers use. SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE.
function Place-Windows {
    $spots = @{ 'game' = @(2100, 900); 'game2' = @(2770, 900); 'game3' = @(2100, 380); 'game4' = @(2770, 380) }
    foreach ($t in $spots.Keys) {
        $p = Get-Game $t
        if ($p -and $p.MainWindowHandle -ne [IntPtr]::Zero) {
            [void][PolishWin]::SetWindowPos($p.MainWindowHandle, [IntPtr]::Zero, $spots[$t][0], $spots[$t][1], 0, 0, 0x0001 -bor 0x0004 -bor 0x0010)
        }
    }
}

function Shot([string]$tag, [string[]]$trees = @('game', 'game2')) {
    Push-Location $script:Root
    try {
        foreach ($t in $trees) {
            $p = Get-Game $t
            if ($p) {
                & (Join-Path $script:Root 'tools-src\capture-window.ps1') -ProcessId $p.Id -Out "re\notes\coop-local\polish-$tag-$t.png" -NoFocus 2>&1 | Out-Null
            } else { "$t is not running" }
        }
    } finally { Pop-Location }
}

function Start-Pair([string]$Map = 'MAP3B', [int]$Players = 2, [int]$LateJoin = 0, [string[]]$Extra = @()) {
    $h = @{ Players = $Players; Map = $Map; NoArrange = $true; Port = $script:Port; LateJoin = $LateJoin }
    & (Join-Path $script:Root 'run-coop-local.ps1') @h | Out-Null
    Place-Windows
}

function Stop-Games {
    $trees = 'game4', 'game3', 'game2', 'game'
    foreach ($t in $trees) { $p = Get-Game $t; if ($p) { [void]$p.CloseMainWindow() } }
    Start-Sleep -Seconds 5
    foreach ($t in $trees) { $p = Get-Game $t; if ($p) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } }
    'closed'
}

function Read-Log([string]$tree, [string]$name = 'dh2coop.log') {
    $path = Join-Path $script:Root "$tree\Bin\$name"
    if (-not (Test-Path -LiteralPath $path)) { return @() }
    $fs = [IO.File]::Open($path, 'Open', 'Read', 'ReadWrite')
    try { $sr = New-Object IO.StreamReader($fs); return ($sr.ReadToEnd() -split "`r?`n") } finally { $fs.Dispose() }
}

function Grep-Log([string]$tree, [string]$pattern, [string]$name = 'dh2coop.log', [int]$max = 60) {
    Read-Log $tree $name | Where-Object { $_ -match $pattern } | Select-Object -First $max |
        ForEach-Object { if ($_.Length -gt 260) { $_.Substring(0, 260) } else { $_ } }
}
