# Background-stall test: two local instances, scripted focus / cover / minimize phases, then per-phase
# heartbeat and network receive counts from both dh2coop.log files.
# Usage: .\tools-src\focus-test.ps1 [-PhaseSeconds 40]
# Phases: A game2 focused (host visible, unfocused) | B game focused | C game fully covered by game2 |
#         D game minimized | E game restored, side by side, game focused.
param([int]$PhaseSeconds = 40)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $root 're\notes\coop-local\focus-test.txt'
New-Item -ItemType Directory -Force (Split-Path $out) | Out-Null

if (-not ('Dh2Focus' -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Dh2Focus {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint f);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
}
"@
}
[void][Dh2Focus]::SetProcessDPIAware()

function Log([string]$m) {
    $line = "[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss.fff'), $m
    Write-Host $line
    Add-Content -LiteralPath $out -Value $line -Encoding UTF8
}

function Focus([IntPtr]$h, [string]$label) {
    [void][Dh2Focus]::SetForegroundWindow($h)
    Start-Sleep -Milliseconds 300
    if ([Dh2Focus]::GetForegroundWindow() -ne $h) {
        # Foreground lock: the process that sent the last input may take the foreground. Tap F24
        # (unbound; not Alt, which would put the window into system-menu mode and block its loop).
        [Dh2Focus]::keybd_event(0x87, 0, 0, [UIntPtr]::Zero)
        [Dh2Focus]::keybd_event(0x87, 0, 2, [UIntPtr]::Zero)
        [void][Dh2Focus]::SetForegroundWindow($h)
        Start-Sleep -Milliseconds 300
    }
    $ok = [Dh2Focus]::GetForegroundWindow() -eq $h
    Log "focus $label -> $ok"
}

Set-Content -LiteralPath $out -Encoding UTF8 -Value "focus-test $(Get-Date -Format o) PhaseSeconds=$PhaseSeconds"
& (Join-Path $root 'run-coop-local.ps1')   # launches both, places them side by side, returns
$procs = Get-Process Main7
$p1 = $procs | Where-Object { $_.Path -like "*\game\Bin\Main7.exe" } | Select-Object -First 1
$p2 = $procs | Where-Object { $_.Path -like "*\game2\Bin\Main7.exe" } | Select-Object -First 1
$h1 = $p1.MainWindowHandle; $h2 = $p2.MainWindowHandle
Log "game pid=$($p1.Id) hwnd=$h1; game2 pid=$($p2.Id) hwnd=$h2"
Log "settle 30s (map load, join)"
Start-Sleep 30

$phases = @()
function Phase([string]$name) {
    $script:phases += [pscustomobject]@{ Name = $name; Start = Get-Date }
    Log "PHASE $name"
}

try {
    Phase 'A game2-focused'; Focus $h2 'game2'; Start-Sleep $PhaseSeconds
    Phase 'B game-focused'; Focus $h1 'game'; Start-Sleep $PhaseSeconds
    Phase 'C game-covered'
    [void][Dh2Focus]::SetWindowPos($h2, [IntPtr]::Zero, 0, 0, 0, 0, 0x0001)  # SWP_NOSIZE, top of z-order
    Focus $h2 'game2'; Start-Sleep $PhaseSeconds
    Phase 'D game-minimized'
    [void][Dh2Focus]::ShowWindow($h1, 6)  # SW_MINIMIZE
    Log "game iconic=$([Dh2Focus]::IsIconic($h1))"
    Focus $h2 'game2'; Start-Sleep $PhaseSeconds
    Phase 'E game-restored'
    [void][Dh2Focus]::ShowWindow($h1, 9)  # SW_RESTORE
    [void][Dh2Focus]::SetWindowPos($h2, [IntPtr]::Zero, 660, 0, 0, 0, 0x0001 -bor 0x0004)
    Focus $h1 'game'; Start-Sleep $PhaseSeconds
    Phase 'end'
} finally {
    foreach ($p in @($p1, $p2)) { try { if (-not $p.HasExited) { [void]$p.CloseMainWindow() } } catch { } }
    Start-Sleep 5
    foreach ($p in @($p1, $p2)) { try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { } }
}

# Per-phase summary: Present frames (heartbeat window_frames, summed by wall time) and rx growth.
$rxPat = 'coop: remote \d+ .* rx=(\d+)'
foreach ($tree in 'game', 'game2') {
    $lines = Get-Content (Join-Path $root "$tree\Bin\dh2coop.log")
    $rows = foreach ($l in $lines) {
        if ($l -match '^\[(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d+)\] (.*)$') {
            [pscustomobject]@{ T = [datetime]::ParseExact($Matches[1], 'yyyy-MM-dd HH:mm:ss.fff', $null); Msg = $Matches[2] }
        }
    }
    for ($i = 0; $i -lt $phases.Count - 1; $i++) {
        $a = $phases[$i].Start; $b = $phases[$i + 1].Start
        $in = $rows | Where-Object { $_.T -ge $a -and $_.T -lt $b }
        $hb = @($in | Where-Object { $_.Msg -match 'heartbeat .*avg_dt=([\d.]+)s' } | ForEach-Object { [double]($_.Msg -replace '.*avg_dt=([\d.]+)s.*', '$1') })
        $rx = @($in | Where-Object { $_.Msg -match $rxPat } | ForEach-Object { [int]($_.Msg -replace '.*rx=(\d+).*', '$1') })
        $rxd = if ($rx.Count -ge 2) { $rx[-1] - $rx[0] } else { 0 }
        $secs = ($b - $a).TotalSeconds
        Log ("{0,-6} {1,-18} heartbeats={2} avg_dt={3} rx/s={4:N1} remote-lines={5}" -f $tree, $phases[$i].Name, $hb.Count, (($hb | ForEach-Object { '{0:N3}' -f $_ }) -join ','), ($rxd / $secs), $rx.Count)
    }
}
Log "done -> $out"
exit 0  # robocopy in run-coop-local leaves $LASTEXITCODE=1 ("files copied")
