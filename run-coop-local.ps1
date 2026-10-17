# Two sandbox instances of the game, each with its own Bin directory and logs.
# Usage: .\run-coop-local.ps1 [-Seconds N] [-NoArrange] [-Capture] [-Map HUNTMAP4|MAP3B|...]
#   -NoArrange  do not move the windows
#   -Capture    every 10s of a -Seconds run, save both windows under re\notes\coop-local\
# The client starts after the host debug.log reaches engine time 8s (host in the map and listening).
# Both games are started with Win32_Process.Create so they outlive the launching shell: an agent
# shell's job object kills Start-Process children when the command returns (the 2026-10-16 16:36
# and 16:53 "both crashed" runs).
# Build first: .\mod\build.ps1
param(
    [int]$Seconds = 0,
    [switch]$NoArrange,
    [switch]$Capture,
    [string]$Map = 'HUNTMAP4'
)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$game = Join-Path $root 'game'
$game2 = Join-Path $root 'game2'
$ccd = 'D:\scary hunt\CABELAS.DANGEROUS.HUNTS.2-Clone\Dngh2.ccd'
$asi = Join-Path $root 'mod\build\dh2coop.asi'
$ini = Join-Path $root 'mod\dh2coop.ini'
$cfg = Join-Path $root 'tools-src\console.windowed.cfg'
$captureScript = Join-Path $root 'tools-src\capture-window.ps1'
$shotDir = Join-Path $root 're\notes\coop-local'
$launch = "+mod data -nointro -developper -cheats -spmap $Map"
$windowTimeoutSec = 90

if ($Capture -and -not (Test-Path -LiteralPath $captureScript)) { throw "missing: $captureScript" }

if (Get-Process Main7 -ErrorAction SilentlyContinue) {
    throw 'Main7.exe is already running. Close it before run-coop-local.ps1.'
}
foreach ($need in @($asi, $ini, $cfg, (Join-Path $game 'Bin\Main7.exe'))) {
    if (-not (Test-Path -LiteralPath $need)) { throw "missing: $need" }
}

if (-not (Get-Volume | Where-Object { $_.DriveType -eq 'CD-ROM' -and $_.FileSystemLabel -eq 'Dngh2' })) {
    & 'C:\Program Files (x86)\Elaborate Bytes\VirtualCloneDrive\VCDMount.exe' $ccd
    Start-Sleep 4
}

# Refresh game2 from game. Skip logs and the loose console cfg; both are rewritten below.
& robocopy $game $game2 /MIR /XF *.log console.cfg /R:1 /W:1 /NFL /NDL /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy failed (exit $LASTEXITCODE)" }

foreach ($tree in @($game, $game2)) {
    $bin = Join-Path $tree 'Bin'
    Copy-Item $asi (Join-Path $bin 'dh2coop.asi') -Force
    Copy-Item $ini (Join-Path $bin 'dh2coop.ini') -Force
    # A command file left by a crashed test would otherwise run in the new game (dh2coop.cmd channel).
    Remove-Item (Join-Path $bin 'dh2coop.cmd') -ErrorAction SilentlyContinue
    $cfgDir = Join-Path $tree 'Data\Cfg'
    New-Item -ItemType Directory -Force $cfgDir | Out-Null
    Copy-Item $cfg (Join-Path $cfgDir 'console.cfg') -Force
    Remove-Item (Join-Path $bin 'debug.log') -ErrorAction SilentlyContinue
}

if (-not ('Dh2Arrange' -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Dh2Arrange {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern int GetSystemMetrics(int n);
    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    public const uint SWP_NOSIZE = 0x0001;
    public const uint SWP_NOZORDER = 0x0004;
    public const uint SWP_NOACTIVATE = 0x0010;
}
"@
}
[void][Dh2Arrange]::SetProcessDPIAware()

$script:runLog = $null
$script:p1 = $null
$script:p2 = $null

function Write-Run([string]$Message) {
    Write-Host $Message
    if ($script:runLog) {
        Add-Content -LiteralPath $script:runLog -Value $Message -Encoding UTF8
    }
}

function Get-OuterRect([IntPtr]$Hwnd) {
    $r = New-Object Dh2Arrange+RECT
    $ok = [Dh2Arrange]::GetWindowRect($Hwnd, [ref]$r)
    if (-not $ok) { return $null }
    [pscustomobject]@{ L = $r.L; T = $r.T; R = $r.R; B = $r.B; W = ($r.R - $r.L); H = ($r.B - $r.T) }
}

function Format-Outer([string]$Label, [IntPtr]$Hwnd, $Rect) {
    if ($null -eq $Rect) { return "$Label hwnd=$Hwnd rect=UNAVAILABLE" }
    return "$Label hwnd=$Hwnd rect=($($Rect.L),$($Rect.T))-($($Rect.R),$($Rect.B)) size=$($Rect.W)x$($Rect.H)"
}

function Get-LiveWindow([System.Diagnostics.Process]$Proc) {
    if ($null -eq $Proc) { return $null }
    try { $Proc.Refresh() } catch { return $null }
    if ($Proc.HasExited) { return $null }
    $hwnd = $Proc.MainWindowHandle
    if ($hwnd -eq [IntPtr]::Zero) { return $null }
    $rect = Get-OuterRect $hwnd
    if ($null -eq $rect -or $rect.W -le 0 -or $rect.H -le 0) { return $null }
    [pscustomobject]@{ Handle = $hwnd; Rect = $rect }
}

function Get-FgInfo {
    $hwnd = [Dh2Arrange]::GetForegroundWindow()
    $procId = [uint32]0
    [void][Dh2Arrange]::GetWindowThreadProcessId($hwnd, [ref]$procId)
    $role = 'other'
    $name = ''
    if ($null -ne $script:p1 -and [int]$procId -eq $script:p1.Id) { $role = 'game' }
    elseif ($null -ne $script:p2 -and [int]$procId -eq $script:p2.Id) { $role = 'game2' }
    try { $name = (Get-Process -Id ([int]$procId) -ErrorAction SilentlyContinue).ProcessName } catch { }
    [pscustomobject]@{ Hwnd = $hwnd; Pid = [int]$procId; Role = $role; Name = $name }
}

function Write-Fg([string]$Tag) {
    $fg = Get-FgInfo
    $nameBit = $(if ($fg.Name) { " name=$($fg.Name)" } else { '' })
    Write-Run "$Tag role=$($fg.Role) pid=$($fg.Pid) hwnd=$($fg.Hwnd)$nameBit"
}

function Move-KeepSize([IntPtr]$Hwnd, [int]$X, [int]$Y) {
    $flags = [Dh2Arrange]::SWP_NOSIZE -bor [Dh2Arrange]::SWP_NOZORDER -bor [Dh2Arrange]::SWP_NOACTIVATE
    $ok = [Dh2Arrange]::SetWindowPos($Hwnd, [IntPtr]::Zero, $X, $Y, 0, 0, $flags)
    if (-not $ok) {
        $err = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
        Write-Run "SetWindowPos failed hwnd=$Hwnd to=($X,$Y) win32=$err"
    }
    return $ok
}

function Test-Placed($Rect1, $Rect2) {
    if ($null -eq $Rect1 -or $null -eq $Rect2) { return $false }
    $gap = $Rect2.L - $Rect1.R
    $gameOk = ([math]::Abs($Rect1.L) -le 40) -and ([math]::Abs($Rect1.T) -le 40)
    $game2Ok = ([math]::Abs($Rect2.T) -le 40) -and ([math]::Abs($gap - 10) -le 40)
    return ($gameOk -and $game2Ok)
}

function Invoke-Arrange {
    param([switch]$Quiet)
    $w1 = Get-LiveWindow $script:p1
    $w2 = Get-LiveWindow $script:p2
    if ($null -eq $w1 -or $null -eq $w2) { throw 'arrange called before both main windows existed' }
    $sw = [Dh2Arrange]::GetSystemMetrics(0)
    $sh = [Dh2Arrange]::GetSystemMetrics(1)
    if (-not $Quiet) {
        $vx = [Dh2Arrange]::GetSystemMetrics(76)
        $vy = [Dh2Arrange]::GetSystemMetrics(77)
        $vw = [Dh2Arrange]::GetSystemMetrics(78)
        $vh = [Dh2Arrange]::GetSystemMetrics(79)
        Write-Run "screen ${sw}x${sh} virtual=(${vx},${vy}) ${vw}x${vh}"
        Write-Fg 'foreground before arrange'
    }
    # SWP_NOACTIVATE: do not pull focus onto game. The later process (game2) should stay in front.
    Move-KeepSize $w1.Handle 0 0 | Out-Null
    $r1 = Get-OuterRect $w1.Handle
    if ($null -eq $r1) { throw "GetWindowRect failed for game hwnd=$($w1.Handle)" }
    $x2 = $r1.R + 10
    if (-not $Quiet) {
        Write-Run "place game=(0,0) game2=($x2,0) flags=SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE"
    }
    Move-KeepSize $w2.Handle $x2 0 | Out-Null
    Start-Sleep -Milliseconds 200
    $r1 = Get-OuterRect $w1.Handle
    $r2 = Get-OuterRect $w2.Handle
    Write-Run (Format-Outer 'game' $w1.Handle $r1)
    Write-Run (Format-Outer 'game2' $w2.Handle $r2)
    if ($r1 -and $r2) {
        $gap = $r2.L - $r1.R
        $overlap = ($r1.L -lt $r2.R -and $r2.L -lt $r1.R -and $r1.T -lt $r2.B -and $r2.T -lt $r1.B)
        $inside = ($r1.L -ge 0 -and $r1.T -ge 0 -and $r1.R -le $sw -and $r1.B -le $sh -and $r2.L -ge 0 -and $r2.T -ge 0 -and $r2.R -le $sw -and $r2.B -le $sh)
        Write-Run ("gap={0}px overlap={1} both_fully_inside={2}" -f $gap, $(if ($overlap) { 'yes' } else { 'no' }), $(if ($inside) { 'yes' } else { 'no' }))
    }
    if (-not $Quiet) { Write-Fg 'foreground after arrange' }
    return (Test-Placed $r1 $r2)
}

function Invoke-WindowCapture {
    param([int]$ProcessId, [string]$OutPath)
    # capture-window.ps1 joins the -Out path onto the cwd. PS 5.1 Join-Path
    # concatenates a rooted child, so an absolute -Out throws. Keep rooted paths.
    $template = @'
function Join-Path {
    param([string]$Path, [string]$ChildPath)
    if ([System.IO.Path]::IsPathRooted($ChildPath)) { return $ChildPath }
    Microsoft.PowerShell.Management\Join-Path -Path $Path -ChildPath $ChildPath
}
& '__SCRIPT__' -ProcessId __PID__ -Out '__OUT__'
'@
    $cmd = $template.Replace('__SCRIPT__', $captureScript).Replace('__PID__', "$ProcessId").Replace('__OUT__', $OutPath)
    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $lines = & powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -Command $cmd 2>&1
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $prev
    }
    $text = (($lines | ForEach-Object { "$_" }) -join ' | ')
    Write-Run "capture-window pid=$ProcessId exit=$code $text"
    return $code
}

function Start-Instance([string]$tree, [string]$extra) {
    $bin = Join-Path $tree 'Bin'
    $exe = Join-Path $bin 'Main7.exe'
    # Win32_Process.Create breaks out of a tool shell's job object. Start-Process
    # children are killed when that job closes, which looks like both windows vanishing.
    $cmd = '"{0}" {1} {2}' -f $exe, $launch, $extra
    $r = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{
        CommandLine = $cmd
        CurrentDirectory = $bin
    }
    if ($r.ReturnValue -ne 0 -or -not $r.ProcessId) {
        throw "failed to start $exe (Win32 $($r.ReturnValue))"
    }
    $p = $null
    for ($i = 0; $i -lt 20 -and $null -eq $p; $i++) {
        $p = Get-Process -Id ([int]$r.ProcessId) -ErrorAction SilentlyContinue
        if (-not $p) { Start-Sleep -Milliseconds 50 }
    }
    if (-not $p) { throw "started pid $($r.ProcessId) but could not open the process" }
    Write-Host "pid=$($p.Id) bin=$bin"
    Write-Host "  dh2coop.log=$(Join-Path $bin 'dh2coop.log')"
    Write-Host "  debug.log=$(Join-Path $bin 'debug.log')"
    return $p
}

function Get-LatestEngineTime([string]$DebugPath) {
    if (-not (Test-Path -LiteralPath $DebugPath)) { return -1.0 }
    $fs = $null
    try {
        $fs = [System.IO.File]::Open($DebugPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
        $len = $fs.Length
        if ($len -le 0) { return -1.0 }
        $take = [int][Math]::Min(16384, $len)
        [void]$fs.Seek($len - $take, [System.IO.SeekOrigin]::Begin)
        $buf = New-Object byte[] $take
        $n = $fs.Read($buf, 0, $take)
        if ($n -le 0) { return -1.0 }
        $text = [System.Text.Encoding]::ASCII.GetString($buf, 0, $n)
    } catch {
        return -1.0
    } finally {
        if ($null -ne $fs) { $fs.Dispose() }
    }
    $found = [regex]::Matches($text, 'time=([0-9]+(?:\.[0-9]+)?)')
    if ($found.Count -eq 0) { return -1.0 }
    return [double]$found[$found.Count - 1].Groups[1].Value
}

function Wait-HostInMap([System.Diagnostics.Process]$Proc, [string]$DebugPath) {
    # Host must be in the map (it starts hosting after the first map load) before the client joins.
    $need = 8.0
    $deadline = (Get-Date).AddSeconds(90)
    $last = -1.0
    while ((Get-Date) -lt $deadline) {
        try { $Proc.Refresh() } catch { throw "game pid=$($Proc.Id) disappeared during startup" }
        if ($Proc.HasExited) { throw "game pid=$($Proc.Id) exited during startup (code $($Proc.ExitCode))" }
        $last = Get-LatestEngineTime $DebugPath
        if ($last -ge $need) {
            Write-Run ("host ready pid={0} engineTime={1:N3} (client waits until {2:N0}s)" -f $Proc.Id, $last, $need)
            return
        }
        Start-Sleep -Milliseconds 200
    }
    throw ("host pid={0} did not reach engine time {1:N0}s (last={2})" -f $Proc.Id, $need, $last)
}

function Stop-LaunchedInstances {
    foreach ($p in @($script:p1, $script:p2)) {
        if ($null -eq $p) { continue }
        try {
            $p.Refresh()
            if (-not $p.HasExited) { $p.CloseMainWindow() | Out-Null }
        } catch { }
    }
    Start-Sleep -Seconds 5
    foreach ($p in @($script:p1, $script:p2)) {
        if ($null -eq $p) { continue }
        try {
            $p.Refresh()
            if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }
        } catch { }
    }
}

function Assert-InstancesAlive {
    foreach ($pair in @(
            @{ Role = 'game'; Proc = $script:p1 },
            @{ Role = 'game2'; Proc = $script:p2 })) {
        $p = $pair.Proc
        if ($null -eq $p) { continue }
        try { $p.Refresh() } catch { continue }
        if ($p.HasExited) {
            throw "$($pair.Role) pid=$($p.Id) exited before the run finished (code $($p.ExitCode))"
        }
    }
}

"launch: $launch"
if ($Capture -or -not $NoArrange) {
    New-Item -ItemType Directory -Force -Path $shotDir | Out-Null
    $script:runLog = Join-Path $shotDir 'run-log.txt'
    Set-Content -LiteralPath $script:runLog -Encoding UTF8 -Value ("run {0} Seconds={1} NoArrange={2} Capture={3}" -f (Get-Date -Format o), $Seconds, [bool]$NoArrange, [bool]$Capture)
}

$completed = $false
try {
    # Instance 1 hosts, instance 2 joins it (dh2coop.asi applies these after the first map load).
    $script:p1 = Start-Instance $game '-coophost'
    Wait-HostInMap $script:p1 (Join-Path $game 'Bin\debug.log')
    $script:p2 = Start-Instance $game2 '-coopjoin 127.0.0.1'
    Write-Run "role=game pid=$($script:p1.Id) args=-coophost"
    Write-Run "role=game2 pid=$($script:p2.Id) args=-coopjoin 127.0.0.1"

    if ($Seconds -gt 0) {
        $runStart = Get-Date
        $endAt = $runStart.AddSeconds($Seconds)
        $windowDeadline = $runStart.AddSeconds($windowTimeoutSec)
        $nextCap = $runStart.AddSeconds(10)
        $nextFg = $runStart
        $nextDrift = $runStart.AddSeconds(2)
        $arranged = $false
        $arrangeCount = 0
        $driftLogged = $false
        $sawSmall = $false
        while ((Get-Date) -lt $endAt) {
            Assert-InstancesAlive
            $now = Get-Date
            $elapsed = [math]::Round(($now - $runStart).TotalSeconds, 1)
            $w1 = Get-LiveWindow $script:p1
            $w2 = Get-LiveWindow $script:p2
            $both = ($null -ne $w1 -and $null -ne $w2)
            $sized = $both -and $w1.Rect.W -ge 640 -and $w1.Rect.H -ge 480 -and $w2.Rect.W -ge 640 -and $w2.Rect.H -ge 480
            if ($both -and -not $sized -and -not $sawSmall) {
                Write-Run ("t={0}s windows exist but outer size is game={1}x{2} game2={3}x{4}; waiting for >= 640x480" -f $elapsed, $w1.Rect.W, $w1.Rect.H, $w2.Rect.W, $w2.Rect.H)
                $sawSmall = $true
            }
            if (-not $NoArrange -and $both -and -not $arranged) {
                if ($sized -or $now -ge $windowDeadline) {
                    if (-not $sized) { Write-Run "window timeout: placing the current outer rects" }
                    [void](Invoke-Arrange)
                    $arranged = $true
                    $arrangeCount++
                }
            } elseif (-not $NoArrange -and $arranged -and $both -and $now -ge $nextDrift) {
                $nextDrift = $now.AddSeconds(2)
                $placed = Test-Placed $w1.Rect $w2.Rect
                if (-not $placed) {
                    $arrangeCount++
                    if (-not $driftLogged) {
                        Write-Run "t=${elapsed}s windows drifted; placing again"
                        $driftLogged = $true
                    }
                    [void](Invoke-Arrange -Quiet)
                }
            }
            if (-not $NoArrange -and -not $arranged -and $now -ge $windowDeadline -and -not $both) {
                $h1 = $(if ($w1) { $w1.Handle } else { [IntPtr]::Zero })
                $h2 = $(if ($w2) { $w2.Handle } else { [IntPtr]::Zero })
                throw "timed out after ${windowTimeoutSec}s waiting for both MainWindowHandle values (game=$h1 game2=$h2)"
            }
            if ($now -ge $nextFg) {
                $fg = Get-FgInfo
                $line = ("fg t={0}s role={1} pid={2} hwnd={3} name={4}" -f $elapsed, $fg.Role, $fg.Pid, $fg.Hwnd, $fg.Name)
                if ($script:runLog) { Add-Content -LiteralPath $script:runLog -Value $line -Encoding UTF8 }
                $nextFg = $now.AddSeconds(1)
            }
            if ($Capture -and $now -ge $nextCap) {
                if (-not $both) {
                    Write-Run "capture skipped at t=${elapsed}s: main window not ready"
                } else {
                    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
                    Write-Fg "capture $stamp foreground-before"
                    # game first, then game2, so the helper's SetForegroundWindow finishes on the later instance.
                    $out1 = Join-Path $shotDir "$stamp-game.png"
                    $out2 = Join-Path $shotDir "$stamp-game2.png"
                    [void](Invoke-WindowCapture -ProcessId $script:p1.Id -OutPath $out1)
                    Write-Fg "capture $stamp foreground-after-game"
                    [void](Invoke-WindowCapture -ProcessId $script:p2.Id -OutPath $out2)
                    Write-Fg "capture $stamp foreground-after-game2"
                    foreach ($shot in @($out1, $out2)) {
                        if (Test-Path -LiteralPath $shot) {
                            Write-Run ("capture file {0} bytes={1}" -f $shot, (Get-Item -LiteralPath $shot).Length)
                        } else {
                            Write-Run "capture file missing $shot"
                        }
                    }
                }
                do { $nextCap = $nextCap.AddSeconds(10) } while ($nextCap -le (Get-Date))
            }
            $remainMs = [int]($endAt - (Get-Date)).TotalMilliseconds
            if ($remainMs -le 0) { break }
            Start-Sleep -Milliseconds ([Math]::Min(200, $remainMs))
        }
        if (-not $NoArrange -and -not $arranged) {
            throw "windows were not arranged before -Seconds elapsed"
        }
    } else {
        if (-not $NoArrange) {
            $deadline = (Get-Date).AddSeconds($windowTimeoutSec)
            $ready = $false
            while ((Get-Date) -lt $deadline) {
                Assert-InstancesAlive
                $w1 = Get-LiveWindow $script:p1
                $w2 = Get-LiveWindow $script:p2
                if ($w1 -and $w2 -and $w1.Rect.W -ge 640 -and $w1.Rect.H -ge 480 -and $w2.Rect.W -ge 640 -and $w2.Rect.H -ge 480) {
                    $ready = $true
                    break
                }
                Start-Sleep -Milliseconds 250
            }
            $w1 = Get-LiveWindow $script:p1
            $w2 = Get-LiveWindow $script:p2
            if (-not ($w1 -and $w2)) {
                throw "timed out after ${windowTimeoutSec}s waiting for both MainWindowHandle values"
            }
            if (-not $ready) { Write-Run 'window timeout: placing the current outer rects' }
            [void](Invoke-Arrange)
        }
        if ($Capture) {
            Write-Warning '-Capture only records while -Seconds is counting down. No captures taken.'
        }
    }
    $completed = $true
} finally {
    if ($Seconds -gt 0 -and ($null -ne $script:p1 -or $null -ne $script:p2)) {
        Stop-LaunchedInstances
        if ($completed) { "closed after ${Seconds}s (graceful, then force)" }
        else { "closed after error (graceful, then force)" }
    }
}
