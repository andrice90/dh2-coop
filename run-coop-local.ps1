# Local sandbox instances of the game, each with its own Bin directory and logs.
# Usage:
#   .\run-coop-local.ps1 [-Players 2..4] [-Seconds N] [-NoArrange] [-Capture] [-Map HUNTMAP4|MAP3B|...] [-LateJoin N]
#   .\run-coop-local.ps1 -AddClient <2..4> [-Map HUNTMAP4]
# One example per switch:
#   .\run-coop-local.ps1 -Players 4
#   .\run-coop-local.ps1 -Seconds 60
#   .\run-coop-local.ps1 -NoArrange
#   .\run-coop-local.ps1 -Capture -Seconds 30
#   .\run-coop-local.ps1 -Map MAP3B
#   .\run-coop-local.ps1 -Players 3 -LateJoin 20 -Seconds 60 -Map MAP3B
#   .\run-coop-local.ps1 -AddClient 3 -Map HUNTMAP4
#   .\run-coop-local.ps1 -Map MAP3B -Combat
#   .\run-coop-local.ps1 -Map MENU          (everyone at the main menu; the host's first map load is followed)
#   .\run-coop-local.ps1 -Lobby             (main menu, no -coophost/-coopjoin: host and join from the lobby window)
# -Port       UDP port written into every copied ini; clients join 127.0.0.1:<Port>. Default 27420. Give each
#             worktree its own port when sessions from several checkouts run on this PC at the same time.
# -Combat     Forces [Puppets] Physics=1 AttackTarget=1 in every copied ini (host animals attack the nearest
#             player, clients included). Both are the mod\dh2coop.ini defaults since 2026-10-17, so this only
#             matters if you turned them off there. re/notes/lead-combat.md.
# -Set        ini overrides 'Section.Key=Value' written into every copied ini, e.g. -Set 'Puppets.Weapons=0'.
# -Players    2 (default) through 4. Trees: game (host), game2, game3, game4.
#             Names written into each Bin\dh2coop.ini: Host, P2, P3, P4. Every client joins 127.0.0.1.
# -LateJoin   Seconds to wait, after the previous client is welcomed (or after host start, when the
#             last client is the only client), before starting the last client. Default 0.
# -AddClient  Start only that client tree against a host already running from game\Bin.
#             Does not stop, mirror over, or rearrange the instances that are already up.
# -NoArrange  do not move the windows
# -Capture    every 10s of a -Seconds run, save each window under re\notes\coop-local\
#             Names: <stamp>-game.png, <stamp>-game2.png, ...
# The host starts first. Each client starts only after the host log says "coop: host start", and
# each later client waits until the previous client's log says "coop: welcomed" (60s timeout).
# The -Seconds countdown starts after the last welcome.
# Waiting for engine time 8s drops the client in after a short story intro (MAP12C fires
# its opening trigger about 6s after load), which leaves the client stuck in that cutscene.
# Both games are started with Win32_Process.Create so they outlive the launching shell: an agent
# shell's job object kills Start-Process children when the command returns (the 2026-10-16 16:36
# and 16:53 "both crashed" runs).
# Build first: .\mod\build.ps1
param(
    [int]$Seconds = 0,
    [switch]$NoArrange,
    [switch]$Capture,
    [string]$Map = 'HUNTMAP4',
    [int]$Players = 2,
    [int]$LateJoin = 0,
    [int]$AddClient = 0,
    [switch]$Combat,
    [int]$Port = 27420,
    [switch]$Lobby,
    [string[]]$Set = @()
)
if ($Lobby) { $Map = 'MENU' }
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$game = Join-Path $root 'game'
$ccd = 'D:\scary hunt\CABELAS.DANGEROUS.HUNTS.2-Clone\Dngh2.ccd'
$asi = Join-Path $root 'mod\build\dh2coop.asi'
$ini = Join-Path $root 'mod\dh2coop.ini'
$cfg = Join-Path $root 'tools-src\console.windowed.cfg'
$captureScript = Join-Path $root 'tools-src\capture-window.ps1'
$shotDir = Join-Path $root 're\notes\coop-local'
# -Map MENU starts every instance at the main menu (no -spmap), the way the friends' kit launches.
$launch = if ($Map -ieq 'MENU') { "+mod data -nointro -developper -cheats" } else { "+mod data -nointro -developper -cheats -spmap $Map" }
$windowTimeoutSec = 90

if ($Players -lt 2 -or $Players -gt 4) { throw '-Players must be from 2 to 4' }
if ($LateJoin -lt 0) { throw '-LateJoin must be >= 0' }
if ($AddClient -ne 0 -and ($AddClient -lt 2 -or $AddClient -gt 4)) { throw '-AddClient must be 2, 3, or 4' }
if ($Capture -and -not (Test-Path -LiteralPath $captureScript)) { throw "missing: $captureScript" }

foreach ($need in @($asi, $ini, $cfg, (Join-Path $game 'Bin\Main7.exe'))) {
    if (-not (Test-Path -LiteralPath $need)) { throw "missing: $need" }
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
if (-not ('Dh2Ini' -as [type])) {
    Add-Type @"
using System.Runtime.InteropServices;
public static class Dh2Ini {
    [DllImport("kernel32.dll", CharSet=CharSet.Ansi)]
    public static extern bool WritePrivateProfileString(string section, string key, string value, string file);
}
"@
}

$script:runLog = $null
$script:instances = @()

function Write-Run([string]$Message) {
    Write-Host $Message
    if ($script:runLog) {
        Add-Content -LiteralPath $script:runLog -Value $Message -Encoding UTF8
    }
}

function Get-SlotRole([int]$Slot) {
    if ($Slot -le 1) { return 'game' }
    return "game$Slot"
}

function Get-SlotTree([int]$Slot) {
    return (Join-Path $root (Get-SlotRole $Slot))
}

function Get-SlotPlayerName([int]$Slot) {
    if ($Slot -le 1) { return 'Host' }
    return "P$Slot"
}

function Assert-DiscMounted {
    if (-not (Get-Volume | Where-Object { $_.DriveType -eq 'CD-ROM' -and $_.FileSystemLabel -eq 'Dngh2' })) {
        & 'C:\Program Files (x86)\Elaborate Bytes\VirtualCloneDrive\VCDMount.exe' $ccd
        Start-Sleep 4
    }
}

function Sync-ClientTree([string]$Dest) {
    # Same robocopy line the two-instance launcher used for game2.
    & robocopy $game $Dest /MIR /XF *.log console.cfg /R:1 /W:1 /NFL /NDL /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed (exit $LASTEXITCODE)" }
}

function Set-PlayerNameIni([string]$Path, [string]$Name) {
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $text = [System.Text.Encoding]::ASCII.GetString($bytes)
    if ($text -notmatch '(?m)^PlayerName=') { throw "PlayerName= line missing in $Path" }
    $updated = [regex]::Replace($text, '(?m)^PlayerName=.*$', "PlayerName=$Name")
    $ascii = New-Object System.Text.ASCIIEncoding
    [System.IO.File]::WriteAllText($Path, $updated, $ascii)
}

# Width=0 means one window at the desktop size. Two of those do not fit side by side, so a
# multi-window launch writes a tile that does. An explicit Width in mod\dh2coop.ini is left alone.
function Set-TiledDisplay([string]$IniPath) {
    if ($script:Players -lt 2) { return }
    $text = [System.IO.File]::ReadAllText($IniPath)
    $section = ''
    if ($text -match '(?s)\[Display\](.*?)(\r?\n\[|\z)') { $section = $Matches[1] }
    $width = 0
    if ($section -match '(?m)^Width=(\d+)') { $width = [int]$Matches[1] }
    if ($width -gt 0) { return }
    $cols = 2
    $rows = $(if ($script:Players -le 2) { 1 } else { 2 })
    $sw = [Dh2Arrange]::GetSystemMetrics(0)
    $sh = [Dh2Arrange]::GetSystemMetrics(1)
    $w = [Math]::Max(640, [int]($sw / $cols) - 24)
    $h = [Math]::Max(480, [int]($sh / $rows) - 80)
    if ($text -notmatch '(?m)^\[Display\]') {
        $text += "`r`n[Display]`r`nEnabled=1`r`nWidth=$w`r`nHeight=$h`r`nWindowed=1`r`nHorPlus=1`r`nLockHud=1`r`n"
    } else {
        if ($section -match '(?m)^Width=') {
            $text = [regex]::Replace($text, '(?m)^Width=.*$', "Width=$w", 1)
        } else {
            $text = $text -replace '(?m)^\[Display\]', "[Display]`r`nWidth=$w"
        }
        if ($text -match '(?m)^Height=') {
            $text = [regex]::Replace($text, '(?m)^Height=.*$', "Height=$h", 1)
        } else {
            $text = $text -replace '(?m)^\[Display\]', "[Display]`r`nHeight=$h"
        }
    }
    [System.IO.File]::WriteAllText($IniPath, $text, (New-Object System.Text.ASCIIEncoding))
}

function Install-TreeFiles([string]$Tree, [string]$PlayerName) {
    $bin = Join-Path $Tree 'Bin'
    Copy-Item $asi (Join-Path $bin 'dh2coop.asi') -Force
    $iniDest = Join-Path $bin 'dh2coop.ini'
    Copy-Item $ini $iniDest -Force
    Set-TiledDisplay $iniDest
    Set-PlayerNameIni $iniDest $PlayerName
    if ($Port -ne 27420) {
        $text = [System.IO.File]::ReadAllText($iniDest)
        $text = [regex]::Replace($text, '(?m)^Port=\d+', "Port=$Port")
        [System.IO.File]::WriteAllText($iniDest, $text, (New-Object System.Text.ASCIIEncoding))
    }
    if ($Combat) {
        # Phase 5 test: host animals may attack client puppets (needs the puppet's physics body).
        $text = [System.IO.File]::ReadAllText($iniDest)
        $text = [regex]::Replace($text, '(?m)^Physics=\d', 'Physics=1')
        $text = [regex]::Replace($text, '(?m)^AttackTarget=\d', 'AttackTarget=1')
        [System.IO.File]::WriteAllText($iniDest, $text, (New-Object System.Text.ASCIIEncoding))
    }
    # Every window of a local session talks over 127.0.0.1: loopback-only sockets never raise the Windows
    # Firewall "allow access" prompt for a new game copy. -Set 'Coop.BindAddress=' restores all interfaces.
    [void][Dh2Ini]::WritePrivateProfileString('Coop', 'BindAddress', '127.0.0.1', $iniDest)
    foreach ($kv in $Set) {
        if ($kv -notmatch '^([^.]+)\.([^=]+)=(.*)$') { throw "bad -Set '$kv' (Section.Key=Value)" }
        [void][Dh2Ini]::WritePrivateProfileString($Matches[1], $Matches[2], $Matches[3], $iniDest)
    }
    # A command file left by a crashed test would otherwise run in the new game (dh2coop.cmd channel).
    Remove-Item (Join-Path $bin 'dh2coop.cmd') -ErrorAction SilentlyContinue
    # Drop the previous log so a welcome/host line from an older run cannot satisfy the wait.
    Remove-Item (Join-Path $bin 'dh2coop.log') -ErrorAction SilentlyContinue
    $cfgDir = Join-Path $Tree 'Data\Cfg'
    New-Item -ItemType Directory -Force $cfgDir | Out-Null
    Copy-Item $cfg (Join-Path $cfgDir 'console.cfg') -Force
    Remove-Item (Join-Path $bin 'debug.log') -ErrorAction SilentlyContinue
}

function Get-RunningMain7 {
    $rows = New-Object System.Collections.Generic.List[object]
    foreach ($p in @(Get-Process -Name Main7 -ErrorAction SilentlyContinue)) {
        $exe = $null
        try { $exe = $p.Path } catch { }
        if ([string]::IsNullOrWhiteSpace($exe)) {
            try {
                $exe = (Get-CimInstance Win32_Process -Filter "ProcessId=$($p.Id)" -ErrorAction Stop).ExecutablePath
            } catch { }
        }
        $bin = $null
        $tree = $null
        if (-not [string]::IsNullOrWhiteSpace($exe)) {
            $bin = [System.IO.Path]::GetFullPath((Split-Path -Parent $exe))
            $tree = Split-Path -Leaf (Split-Path -Parent $bin)
        }
        $rows.Add([pscustomobject]@{ Id = $p.Id; Exe = $exe; Bin = $bin; Tree = $tree })
    }
    return $rows
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
    foreach ($inst in $script:instances) {
        if ($null -ne $inst.Proc -and [int]$procId -eq $inst.Proc.Id) { $role = $inst.Role }
    }
    $name = ''
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

function Get-PlacePoint([int]$Index, $HostRect) {
    # 2 players: side by side. 3 or 4: 2x2, host top-left, game2 top-right, game3 bottom-left, game4 bottom-right.
    # If the bottom row would leave the screen, overlap the rows vertically instead.
    $x2 = 0
    $yBottom = 0
    if ($null -ne $HostRect) {
        $x2 = $HostRect.R + 10
        if ($script:instances.Count -gt 2) {
            $sh = [Dh2Arrange]::GetSystemMetrics(1)
            $yBottom = $HostRect.B + 10
            if (($yBottom + $HostRect.H) -gt $sh) { $yBottom = [Math]::Max(0, $sh - $HostRect.H) }
        }
    }
    $x = 0
    $y = 0
    if (($Index % 2) -eq 1) { $x = $x2 }
    if ($Index -ge 2) { $y = $yBottom }
    [pscustomobject]@{ X = [int]$x; Y = [int]$y }
}

function Test-Placed([object[]]$Rects) {
    if ($null -eq $Rects -or $Rects.Count -lt 2) { return $false }
    if ($null -eq $Rects[0] -or $null -eq $Rects[1]) { return $false }
    if ($Rects.Count -le 2) {
        $gap = $Rects[1].L - $Rects[0].R
        $gameOk = ([math]::Abs($Rects[0].L) -le 40) -and ([math]::Abs($Rects[0].T) -le 40)
        $game2Ok = ([math]::Abs($Rects[1].T) -le 40) -and ([math]::Abs($gap - 10) -le 40)
        return ($gameOk -and $game2Ok)
    }
    for ($i = 0; $i -lt $Rects.Count; $i++) {
        if ($null -eq $Rects[$i]) { return $false }
        $pt = Get-PlacePoint $i $Rects[0]
        if (([math]::Abs($Rects[$i].L - $pt.X) -gt 40) -or ([math]::Abs($Rects[$i].T - $pt.Y) -gt 40)) { return $false }
    }
    return $true
}

function Get-InstanceWindows {
    $wins = New-Object object[] $script:instances.Count
    for ($i = 0; $i -lt $script:instances.Count; $i++) {
        $wins[$i] = Get-LiveWindow $script:instances[$i].Proc
    }
    return ,$wins
}

function Test-WindowsSized($Wins) {
    if ($null -eq $Wins) { return $false }
    for ($i = 0; $i -lt $Wins.Count; $i++) {
        $w = $Wins[$i]
        if ($null -eq $w) { return $false }
        if ($w.Rect.W -lt 640 -or $w.Rect.H -lt 480) { return $false }
    }
    return ($Wins.Count -ge 2)
}

function Get-WindowRects($Wins) {
    $rects = New-Object object[] $Wins.Count
    for ($i = 0; $i -lt $Wins.Count; $i++) {
        if ($null -ne $Wins[$i]) { $rects[$i] = $Wins[$i].Rect }
    }
    return ,$rects
}

function Invoke-Arrange {
    param([switch]$Quiet)
    $wins = Get-InstanceWindows
    foreach ($w in $wins) {
        if ($null -eq $w) {
            if ($script:instances.Count -eq 2) { throw 'arrange called before both main windows existed' }
            throw 'arrange called before every main window existed'
        }
    }
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
    # SWP_NOACTIVATE: do not pull focus onto a game window.
    Move-KeepSize $wins[0].Handle 0 0 | Out-Null
    $r1 = Get-OuterRect $wins[0].Handle
    if ($null -eq $r1) { throw "GetWindowRect failed for game hwnd=$($wins[0].Handle)" }
    $n = $script:instances.Count
    if ($n -le 2) {
        $x2 = $r1.R + 10
        if (-not $Quiet) {
            Write-Run "place game=(0,0) game2=($x2,0) flags=SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE"
        }
        Move-KeepSize $wins[1].Handle $x2 0 | Out-Null
    } else {
        $bits = @('game=(0,0)')
        for ($i = 1; $i -lt $n; $i++) {
            $pt = Get-PlacePoint $i $r1
            $bits += "$($script:instances[$i].Role)=($($pt.X),$($pt.Y))"
            Move-KeepSize $wins[$i].Handle $pt.X $pt.Y | Out-Null
        }
        if (-not $Quiet) {
            Write-Run ("place {0} flags=SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE" -f ($bits -join ' '))
        }
    }
    Start-Sleep -Milliseconds 200
    $rects = New-Object object[] $n
    for ($i = 0; $i -lt $n; $i++) {
        $rects[$i] = Get-OuterRect $wins[$i].Handle
        Write-Run (Format-Outer $script:instances[$i].Role $wins[$i].Handle $rects[$i])
    }
    $known = $true
    foreach ($r in $rects) { if ($null -eq $r) { $known = $false } }
    if ($known -and $n -le 2) {
        $gap = $rects[1].L - $rects[0].R
        $overlap = ($rects[0].L -lt $rects[1].R -and $rects[1].L -lt $rects[0].R -and $rects[0].T -lt $rects[1].B -and $rects[1].T -lt $rects[0].B)
        $inside = ($rects[0].L -ge 0 -and $rects[0].T -ge 0 -and $rects[0].R -le $sw -and $rects[0].B -le $sh -and $rects[1].L -ge 0 -and $rects[1].T -ge 0 -and $rects[1].R -le $sw -and $rects[1].B -le $sh)
        Write-Run ("gap={0}px overlap={1} both_fully_inside={2}" -f $gap, $(if ($overlap) { 'yes' } else { 'no' }), $(if ($inside) { 'yes' } else { 'no' }))
    } elseif ($known) {
        $inside = $true
        foreach ($r in $rects) {
            if ($r.L -lt 0 -or $r.T -lt 0 -or $r.R -gt $sw -or $r.B -gt $sh) { $inside = $false }
        }
        $rowOverlap = 'n/a'
        if ($n -ge 3) {
            $a = $rects[0]
            $b = $rects[2]
            $hit = ($a.T -lt $b.B -and $b.T -lt $a.B)
            $rowOverlap = $(if ($hit) { 'yes' } else { 'no' })
        }
        Write-Run ("row_overlap={0} all_fully_inside={1}" -f $rowOverlap, $(if ($inside) { 'yes' } else { 'no' }))
    }
    if (-not $Quiet) { Write-Fg 'foreground after arrange' }
    return (Test-Placed $rects)
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

function Read-SharedText([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return $null }
    $stream = $null
    $reader = $null
    try {
        $stream = New-Object System.IO.FileStream($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
        $reader = New-Object System.IO.StreamReader($stream)
        return $reader.ReadToEnd()
    } catch {
        return $null
    } finally {
        if ($null -ne $reader) { $reader.Dispose() }
        elseif ($null -ne $stream) { $stream.Dispose() }
    }
}

function Find-LogLine([string]$Path, [string]$Pattern) {
    $text = Read-SharedText $Path
    if (-not $text) { return $null }
    $m = [regex]::Match($text, "(?m)^.*$([regex]::Escape($Pattern)).*$")
    if (-not $m.Success) { return $null }
    return $m.Value.Trim()
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
    # Hosting starts in OnMapLoaded ("coop: hosting on UDP <port> -> true|false"). The log path is beside debug.log.
    # "-> false" is a failed bind: another game (often another checkout) already hosts on that port.
    $logPath = Join-Path (Split-Path -Parent $DebugPath) 'dh2coop.log'
    $deadline = (Get-Date).AddSeconds(90)
    while ((Get-Date) -lt $deadline) {
        try { $Proc.Refresh() } catch { throw "game pid=$($Proc.Id) disappeared during startup" }
        if ($Proc.HasExited) { throw "game pid=$($Proc.Id) exited during startup (code $($Proc.ExitCode))" }
        $line = Find-LogLine $logPath 'coop: hosting on UDP'
        if ($line -and $line -notmatch '-> true') {
            throw "host pid=$($Proc.Id) could not listen ($line). Another game already hosts on UDP $Port; pass a free -Port."
        }
        if ($line) {
            $engine = Get-LatestEngineTime $DebugPath
            Write-Run ("host ready pid={0} engineTime={1:N3} (listening) | {2}" -f $Proc.Id, $engine, $line)
            return
        }
        Start-Sleep -Milliseconds 200
    }
    throw "host pid=$($Proc.Id) did not log 'coop: hosting on UDP' within 90s"
}

function Wait-ClientWelcomed([System.Diagnostics.Process]$Proc, [string]$LogPath, [string]$Role, [datetime]$StartedAt) {
    $deadline = (Get-Date).AddSeconds(60)
    while ((Get-Date) -lt $deadline) {
        try { $Proc.Refresh() } catch { throw "$Role pid=$($Proc.Id) disappeared during startup" }
        if ($Proc.HasExited) { throw "$Role pid=$($Proc.Id) exited during startup (code $($Proc.ExitCode))" }
        $line = Find-LogLine $LogPath 'coop: welcomed'
        if ($line) {
            $sec = [math]::Round(((Get-Date) - $StartedAt).TotalSeconds, 1)
            Write-Run ("{0} welcomed pid={1} after {2}s | {3}" -f $Role, $Proc.Id, $sec, $line)
            return $line
        }
        Start-Sleep -Milliseconds 200
    }
    throw "$Role pid=$($Proc.Id) did not log 'coop: welcomed' within 60s"
}

function Wait-LateJoin([string]$Why) {
    if ($LateJoin -le 0) { return }
    Write-Run "late join: waiting ${LateJoin}s after $Why"
    $end = (Get-Date).AddSeconds($LateJoin)
    while ((Get-Date) -lt $end) {
        Assert-InstancesAlive
        $remain = [int]($end - (Get-Date)).TotalMilliseconds
        if ($remain -le 0) { break }
        Start-Sleep -Milliseconds ([Math]::Min(500, $remain))
    }
}

function Stop-LaunchedInstances {
    foreach ($inst in $script:instances) {
        $p = $inst.Proc
        if ($null -eq $p) { continue }
        try {
            $p.Refresh()
            if (-not $p.HasExited) { $p.CloseMainWindow() | Out-Null }
        } catch { }
    }
    Start-Sleep -Seconds 5
    foreach ($inst in $script:instances) {
        $p = $inst.Proc
        if ($null -eq $p) { continue }
        try {
            $p.Refresh()
            if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }
        } catch { }
    }
}

function Assert-InstancesAlive {
    foreach ($inst in $script:instances) {
        $p = $inst.Proc
        if ($null -eq $p) { continue }
        try { $p.Refresh() } catch { continue }
        if ($p.HasExited) {
            throw "$($inst.Role) pid=$($p.Id) exited before the run finished (code $($p.ExitCode))"
        }
    }
}

function Format-SizeWait([double]$Elapsed, $Wins) {
    if ($script:instances.Count -eq 2 -and $Wins[0] -and $Wins[1]) {
        return ("t={0}s windows exist but outer size is game={1}x{2} game2={3}x{4}; waiting for >= 640x480" -f $Elapsed, $Wins[0].Rect.W, $Wins[0].Rect.H, $Wins[1].Rect.W, $Wins[1].Rect.H)
    }
    $bits = @()
    for ($i = 0; $i -lt $script:instances.Count; $i++) {
        $w = $Wins[$i]
        if ($w) { $bits += "$($script:instances[$i].Role)=$($w.Rect.W)x$($w.Rect.H)" }
        else { $bits += "$($script:instances[$i].Role)=none" }
    }
    return ("t={0}s windows exist but outer size is {1}; waiting for >= 640x480" -f $Elapsed, ($bits -join ' '))
}

function Invoke-CaptureAll([string]$Stamp) {
    Write-Fg "capture $Stamp foreground-before"
    $shots = @()
    foreach ($inst in $script:instances) {
        $out = Join-Path $shotDir "$Stamp-$($inst.Role).png"
        $shots += $out
        [void](Invoke-WindowCapture -ProcessId $inst.Proc.Id -OutPath $out)
        Write-Fg "capture $Stamp foreground-after-$($inst.Role)"
    }
    foreach ($shot in $shots) {
        if (Test-Path -LiteralPath $shot) {
            Write-Run ("capture file {0} bytes={1}" -f $shot, (Get-Item -LiteralPath $shot).Length)
        } else {
            Write-Run "capture file missing $shot"
        }
    }
}

function Add-Launched([string]$Role, [string]$Tree, $Proc) {
    $script:instances += [pscustomobject]@{ Role = $Role; Tree = $Tree; Proc = $Proc }
}

if ($AddClient -ne 0) {
    $running = @(Get-RunningMain7)
    foreach ($row in $running) {
        if (-not $row.Bin) { throw "Main7 pid=$($row.Id) has no readable path; refusing -AddClient" }
    }
    $hostBin = [System.IO.Path]::GetFullPath((Join-Path $game 'Bin'))
    $hostUp = $false
    foreach ($row in $running) {
        if ($row.Bin -ieq $hostBin) { $hostUp = $true }
    }
    if (-not $hostUp) { throw "no host: no Main7.exe from $hostBin is running" }
    $role = Get-SlotRole $AddClient
    $tree = Get-SlotTree $AddClient
    $targetBin = [System.IO.Path]::GetFullPath((Join-Path $tree 'Bin'))
    foreach ($row in $running) {
        if ($row.Bin -ieq $targetBin) { throw "target tree already running: $role pid=$($row.Id)" }
    }
    Assert-DiscMounted
    "launch: $launch -coopjoin 127.0.0.1:$Port (AddClient $AddClient, $role)"
    Sync-ClientTree $tree
    Install-TreeFiles $tree (Get-SlotPlayerName $AddClient)
    $startedAt = Get-Date
    $proc = Start-Instance $tree "-coopjoin 127.0.0.1:$Port"
    $welcome = Wait-ClientWelcomed $proc (Join-Path $tree 'Bin\dh2coop.log') $role $startedAt
    Write-Host $welcome
    return
}

# Only this checkout's trees: other worktrees may run their own sessions at the same time (each with its own -Port).
$rootFull = [System.IO.Path]::GetFullPath($root).TrimEnd('\') + '\'
foreach ($row in @(Get-RunningMain7)) {
    if (-not $row.Bin) { throw "Main7 pid=$($row.Id) has no readable path. Close it before run-coop-local.ps1." }
    if ($row.Bin.StartsWith($rootFull, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Main7.exe from this checkout is already running (pid=$($row.Id), $($row.Bin)). Close it first."
    }
}

Assert-DiscMounted

for ($slot = 2; $slot -le $Players; $slot++) {
    Sync-ClientTree (Get-SlotTree $slot)
}
for ($slot = 1; $slot -le $Players; $slot++) {
    Install-TreeFiles (Get-SlotTree $slot) (Get-SlotPlayerName $slot)
}

"launch: $launch"
if ($Capture -or -not $NoArrange) {
    New-Item -ItemType Directory -Force -Path $shotDir | Out-Null
    $script:runLog = Join-Path $shotDir 'run-log.txt'
    Set-Content -LiteralPath $script:runLog -Encoding UTF8 -Value ("run {0} Seconds={1} NoArrange={2} Capture={3} Players={4} LateJoin={5} Map={6}" -f (Get-Date -Format o), $Seconds, [bool]$NoArrange, [bool]$Capture, $Players, $LateJoin, $Map)
}

$completed = $false
try {
    # Instance 1 hosts. Each later instance joins 127.0.0.1 (dh2coop.asi applies these after the first map load).
    # -Lobby: no role switches at all; every instance waits at the main menu with the lobby open (drive it with
    # coop_lobby host / coop_lobby join 127.0.0.1:<Port> through Bin\dh2coop.cmd, or by hand).
    $hostTree = Get-SlotTree 1
    $hostArgs = $(if ($Lobby) { '' } else { '-coophost' })
    $hostProc = Start-Instance $hostTree $hostArgs
    Add-Launched 'game' $hostTree $hostProc
    Write-Run "role=game pid=$($hostProc.Id) args=$hostArgs"
    if ($Lobby) {
        for ($slot = 2; $slot -le $Players; $slot++) {
            $tree = Get-SlotTree $slot
            $proc = Start-Instance $tree ''
            Add-Launched (Get-SlotRole $slot) $tree $proc
            Write-Run "role=$(Get-SlotRole $slot) pid=$($proc.Id) args=(lobby)"
        }
    } else {
    Wait-HostInMap $hostProc (Join-Path $hostTree 'Bin\debug.log')

    for ($slot = 2; $slot -le $Players; $slot++) {
        $role = Get-SlotRole $slot
        if ($slot -eq $Players) {
            $why = $(if ($slot -eq 2) { 'host ready' } else { "$(Get-SlotRole ($slot - 1)) welcomed" })
            Wait-LateJoin $why
        }
        $tree = Get-SlotTree $slot
        $startedAt = Get-Date
        $proc = Start-Instance $tree "-coopjoin 127.0.0.1:$Port"
        Add-Launched $role $tree $proc
        Write-Run "role=$role pid=$($proc.Id) args=-coopjoin 127.0.0.1:$Port"
        [void](Wait-ClientWelcomed $proc (Join-Path $tree 'Bin\dh2coop.log') $role $startedAt)
    }
    }

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
            $wins = Get-InstanceWindows
            $allLive = $true
            foreach ($w in $wins) { if ($null -eq $w) { $allLive = $false } }
            $sized = (Test-WindowsSized $wins)
            if ($allLive -and -not $sized -and -not $sawSmall) {
                Write-Run (Format-SizeWait $elapsed $wins)
                $sawSmall = $true
            }
            if (-not $NoArrange -and $allLive -and -not $arranged) {
                if ($sized -or $now -ge $windowDeadline) {
                    if (-not $sized) { Write-Run "window timeout: placing the current outer rects" }
                    [void](Invoke-Arrange)
                    $arranged = $true
                    $arrangeCount++
                }
            } elseif (-not $NoArrange -and $arranged -and $allLive -and $now -ge $nextDrift) {
                $nextDrift = $now.AddSeconds(2)
                $placed = Test-Placed (Get-WindowRects $wins)
                if (-not $placed) {
                    $arrangeCount++
                    if (-not $driftLogged) {
                        Write-Run "t=${elapsed}s windows drifted; placing again"
                        $driftLogged = $true
                    }
                    [void](Invoke-Arrange -Quiet)
                }
            }
            if (-not $NoArrange -and -not $arranged -and $now -ge $windowDeadline -and -not $allLive) {
                if ($script:instances.Count -eq 2) {
                    $h1 = $(if ($wins[0]) { $wins[0].Handle } else { [IntPtr]::Zero })
                    $h2 = $(if ($wins[1]) { $wins[1].Handle } else { [IntPtr]::Zero })
                    throw "timed out after ${windowTimeoutSec}s waiting for both MainWindowHandle values (game=$h1 game2=$h2)"
                }
                $bits = @()
                for ($i = 0; $i -lt $script:instances.Count; $i++) {
                    $h = $(if ($wins[$i]) { $wins[$i].Handle } else { [IntPtr]::Zero })
                    $bits += "$($script:instances[$i].Role)=$h"
                }
                throw "timed out after ${windowTimeoutSec}s waiting for every MainWindowHandle ($($bits -join ' '))"
            }
            if ($now -ge $nextFg) {
                $fg = Get-FgInfo
                $line = ("fg t={0}s role={1} pid={2} hwnd={3} name={4}" -f $elapsed, $fg.Role, $fg.Pid, $fg.Hwnd, $fg.Name)
                if ($script:runLog) { Add-Content -LiteralPath $script:runLog -Value $line -Encoding UTF8 }
                $nextFg = $now.AddSeconds(1)
            }
            if ($Capture -and $now -ge $nextCap) {
                if (-not $allLive) {
                    Write-Run "capture skipped at t=${elapsed}s: main window not ready"
                } else {
                    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
                    Invoke-CaptureAll $stamp
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
                $wins = Get-InstanceWindows
                if (Test-WindowsSized $wins) {
                    $ready = $true
                    break
                }
                Start-Sleep -Milliseconds 250
            }
            $wins = Get-InstanceWindows
            $allLive = $true
            foreach ($w in $wins) { if ($null -eq $w) { $allLive = $false } }
            if (-not $allLive) {
                if ($script:instances.Count -eq 2) {
                    throw "timed out after ${windowTimeoutSec}s waiting for both MainWindowHandle values"
                }
                throw "timed out after ${windowTimeoutSec}s waiting for every MainWindowHandle"
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
    if ($Seconds -gt 0 -and $script:instances.Count -gt 0) {
        Stop-LaunchedInstances
        if ($completed) { "closed after ${Seconds}s (graceful, then force)" }
        else { "closed after error (graceful, then force)" }
    }
}
