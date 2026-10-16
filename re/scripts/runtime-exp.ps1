# One sandbox launch for Track E. Calls run-game.ps1, samples UDP while Main7 is alive,
# and copies Bin\debug.log / Bin\dh2coop.log into re\notes\runtime-<Name>-*.txt.
# Usage: .\re\scripts\runtime-exp.ps1 -Name baseline -Seconds 40
#        .\re\scripts\runtime-exp.ps1 -Name map -Seconds 55 -ExtraArgs '-developper -cheats -map HUNTMAP1' -SendConsoleKeys
param(
    [Parameter(Mandatory = $true)][string]$Name,
    [int]$Seconds = 45,
    [string]$ExtraArgs = '',
    [switch]$SendConsoleKeys
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$notes = Join-Path $root 're\notes'
New-Item -ItemType Directory -Force $notes | Out-Null
$udpPath = Join-Path $notes "runtime-$Name-udp.txt"
$metaPath = Join-Path $notes "runtime-$Name-meta.txt"

if (Get-Process Main7 -ErrorAction SilentlyContinue) {
    Stop-Process -Name Main7 -Force
    Start-Sleep 2
}

# Poll from a runspace so run-game.ps1 can block in this thread.
$poll = [powershell]::Create().AddScript({
    param($udpPath, $metaPath, $seconds, $sendKeys)
    $ErrorActionPreference = 'Continue'
    function Tap-Scan([byte]$scan) {
        # KEYEVENTF_SCANCODE=0x8, KEYEVENTF_KEYUP=0x2. DirectInput reads scan codes.
        [KeyInj]::keybd_event(0, $scan, 0x8, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 40
        [KeyInj]::keybd_event(0, $scan, 0xA, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 60
    }
    if (-not ('KeyInj' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class KeyInj {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
}
'@
    }
    $lines = New-Object System.Collections.Generic.List[string]
    $sw = [Diagnostics.Stopwatch]::StartNew()
    $seen = $false
    $keysDone = $false
    $pidSeen = 0
    while ($sw.Elapsed.TotalSeconds -lt ($seconds + 20)) {
        $p = Get-Process Main7 -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $p) {
            if ($seen) { $lines.Add(('{0:HH:mm:ss} Main7 exited' -f (Get-Date))); break }
            Start-Sleep -Milliseconds 400
            continue
        }
        $seen = $true
        $pidSeen = $p.Id
        $udp = @()
        try {
            $udp = @(Get-NetUDPEndpoint -OwningProcess $p.Id -ErrorAction Stop |
                ForEach-Object { '{0}:{1}' -f $_.LocalAddress, $_.LocalPort })
        } catch {
            # CIM returns an error object when the process owns no UDP endpoints.
            if ($_.Exception.Message -notmatch 'No MSFT_NetUDPEndpoint') {
                $udp = @('Get-NetUDPEndpoint-error: ' + $_.Exception.Message)
            }
        }
        $ns = @()
        netstat -ano -p udp | ForEach-Object {
            if ($_ -match "\s$($p.Id)\s*$") { $ns += $_.Trim() }
        }
        $title = ''
        try { $title = $p.MainWindowTitle } catch {}
        $lines.Add(('{0:HH:mm:ss} pid={1} title="{2}" hwnd={3} udp=[{4}] netstat=[{5}]' -f `
            (Get-Date), $p.Id, $title, $p.MainWindowHandle, ($udp -join ', '), ($ns -join ' | ')))

        if ($sendKeys -and -not $keysDone -and $sw.Elapsed.TotalSeconds -ge 36 -and $p.MainWindowHandle -ne 0) {
            $keysDone = $true
            $fg = [KeyInj]::SetForegroundWindow($p.MainWindowHandle)
            $lines.Add(('key inject focus={0} hwnd={1}' -f $fg, $p.MainWindowHandle))
            # DIK_GRAVE=0x29, DIK_F1..F12 = 0x3B..0x46, plus OEM_3 vk via scan 0x29 twice.
            Tap-Scan 0x29
            Tap-Scan 0x29
            for ($s = 0x3B; $s -le 0x46; $s++) { Tap-Scan ([byte]$s) }
            $lines.Add('key inject sent grave x2 and F1-F12 scan codes')
        }
        Start-Sleep -Seconds 4
    }
    if (-not $seen) { $lines.Add('Main7 never observed') }
    Set-Content -Path $udpPath -Value $lines -Encoding UTF8
    Set-Content -Path $metaPath -Value @(
        "name=$metaPath",
        "pid=$pidSeen",
        "keysDone=$keysDone",
        "elapsedSec=$([int]$sw.Elapsed.TotalSeconds)"
    ) -Encoding UTF8
}).AddArgument($udpPath).AddArgument($metaPath).AddArgument($Seconds).AddArgument([bool]$SendConsoleKeys)

$handle = $poll.BeginInvoke()
$launchLog = Join-Path $notes "runtime-$Name-launch.txt"
try {
    & (Join-Path $root 'run-game.ps1') -Seconds $Seconds -ExtraArgs $ExtraArgs *>&1 |
        Tee-Object -FilePath $launchLog
} finally {
    Start-Sleep -Seconds 2
    if ($poll.InvocationStateInfo.State -eq 'Running') { $poll.Stop() | Out-Null }
    try { $poll.EndInvoke($handle) | Out-Null } catch {}
    $poll.Dispose()
    if (Get-Process Main7 -ErrorAction SilentlyContinue) {
        Stop-Process -Name Main7 -Force -ErrorAction SilentlyContinue
        Add-Content $launchLog 'forced Main7 stop after run-game returned'
    }
}

$bin = Join-Path $root 'game\Bin'
Copy-Item (Join-Path $bin 'debug.log') (Join-Path $notes "runtime-$Name-debug.txt") -Force -ErrorAction SilentlyContinue
Copy-Item (Join-Path $bin 'dh2coop.log') (Join-Path $notes "runtime-$Name-probe.txt") -Force -ErrorAction SilentlyContinue
"experiment $Name done"
