# Sends keyboard input to a game window as hardware scan codes (SendInput + KEYEVENTF_SCANCODE), which
# DirectInput reads; window messages (PostMessage/SendKeys) do not reach it.
# Usage: .\tools-src\game-input.ps1 -ProcessId 1234 -Keys 'RETURN','W:2000','WAIT:500','DOWN'
#   NAME        tap (60 ms hold)      NAME:ms   hold for ms      WAIT:ms   pause
#   MOUSE:dx,dy relative mouse move (turn / look)    LMB / RMB[:ms] mouse button click
# Names: RETURN ESCAPE SPACE UP DOWN LEFT RIGHT W A S D E R F Q SHIFT CTRL TAB 1-4 GRAVE
param(
    [Parameter(Mandatory)][int]$ProcessId,
    [Parameter(Mandatory)][string[]]$Keys
)
$ErrorActionPreference = 'Stop'
if (-not ('Dh2InputV4' -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Dh2InputV4 {
    // Full union so Marshal.SizeOf(INPUT) is right on both x86 (28) and x64 (40) PowerShell.
    [StructLayout(LayoutKind.Sequential)]
    struct MOUSEINPUT { public int dx; public int dy; public uint data; public uint dwFlags; public uint time; public IntPtr extra; }
    [StructLayout(LayoutKind.Sequential)]
    struct KEYBDINPUT { public ushort wVk; public ushort wScan; public uint dwFlags; public uint time; public IntPtr extra; }
    [StructLayout(LayoutKind.Explicit)]
    struct UNION { [FieldOffset(0)] public MOUSEINPUT mi; [FieldOffset(0)] public KEYBDINPUT ki; }
    [StructLayout(LayoutKind.Sequential)]
    struct INPUT { public uint type; public UNION u; }
    [DllImport("user32.dll", SetLastError = true)] static extern uint SendInput(uint n, INPUT[] inputs, int size);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] static extern bool BringWindowToTop(IntPtr h);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("kernel32.dll")] static extern uint GetCurrentThreadId();
    [DllImport("user32.dll")] static extern bool AttachThreadInput(uint a, uint b, bool attach);
    [DllImport("user32.dll")] static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    public static void Focus(IntPtr h) {
        uint unused;
        uint fore = GetWindowThreadProcessId(GetForegroundWindow(), out unused);
        uint self = GetCurrentThreadId();
        bool attached = fore != 0 && fore != self && AttachThreadInput(fore, self, true);
        keybd_event(0x12, 0, 0, UIntPtr.Zero); // ALT lets SetForegroundWindow succeed from a background process
        ShowWindow(h, 9);
        BringWindowToTop(h);
        SetForegroundWindow(h);
        keybd_event(0x12, 0, 2, UIntPtr.Zero);
        if (attached) AttachThreadInput(fore, self, false);
    }
    public static uint Key(ushort scan, bool extended, bool up) {
        var i = new INPUT[1];
        i[0].type = 1;  // INPUT_KEYBOARD
        i[0].u.ki.wScan = scan;
        i[0].u.ki.dwFlags = 0x0008u | (extended ? 0x0001u : 0u) | (up ? 0x0002u : 0u);  // SCANCODE | EXTENDED | KEYUP
        return SendInput(1, i, Marshal.SizeOf(typeof(INPUT)));
    }
    public static uint MouseButton(uint flags) {
        var i = new INPUT[1];
        i[0].type = 0;  // INPUT_MOUSE
        i[0].u.mi.dwFlags = flags;
        return SendInput(1, i, Marshal.SizeOf(typeof(INPUT)));
    }
    public static uint MouseMove(int dx, int dy) {
        var i = new INPUT[1];
        i[0].type = 0;  // INPUT_MOUSE
        i[0].u.mi.dx = dx;
        i[0].u.mi.dy = dy;
        i[0].u.mi.dwFlags = 0x0001u;  // MOUSEEVENTF_MOVE (relative)
        return SendInput(1, i, Marshal.SizeOf(typeof(INPUT)));
    }
}
"@
}
# DIK scan codes; arrows are extended (E0) keys.
$codes = @{
    RETURN = @(0x1C, $false); ESCAPE = @(0x01, $false); SPACE = @(0x39, $false); TAB = @(0x0F, $false)
    UP = @(0x48, $true); DOWN = @(0x50, $true); LEFT = @(0x4B, $true); RIGHT = @(0x4D, $true)
    W = @(0x11, $false); A = @(0x1E, $false); S = @(0x1F, $false); D = @(0x20, $false)
    E = @(0x12, $false); R = @(0x13, $false); F = @(0x21, $false); Q = @(0x10, $false)
    SHIFT = @(0x2A, $false); CTRL = @(0x1D, $false); GRAVE = @(0x29, $false); F9 = @(0x43, $false)
    '1' = @(0x02, $false); '2' = @(0x03, $false); '3' = @(0x04, $false); '4' = @(0x05, $false)
}
$h = (Get-Process -Id $ProcessId).MainWindowHandle
if ([Dh2InputV4]::GetForegroundWindow() -ne $h) {
    [Dh2InputV4]::Focus($h)
    Start-Sleep -Milliseconds 400
}
# SendInput goes to whatever window is in front. Windows often refuses SetForegroundWindow while the user works in
# another app; the keys then landed in that app (2026-10-17: "wwww...sss" typed into the user's chat). Never send
# unless the game is in front, and check again before every key.
function Assert-GameInFront([string]$what) {
    if ([Dh2InputV4]::GetForegroundWindow() -ne $h) {
        throw "game-input: pid $ProcessId is not the foreground window; not sending $what (click the game window, or retry)"
    }
}
Assert-GameInFront ($Keys -join ' ')
foreach ($k in $Keys) {
    $name, $ms = $k.Split(':')
    if ($name -eq 'WAIT') { Start-Sleep -Milliseconds ([int]$ms); continue }
    Assert-GameInFront $k
    if ($name -eq 'LMB' -or $name -eq 'RMB') {
        # LMB / RMB[:ms] - mouse button click (hold ms, default 60)
        $down, $up = if ($name -eq 'LMB') { 0x0002, 0x0004 } else { 0x0008, 0x0010 }
        [void][Dh2InputV4]::MouseButton([uint32]$down)
        Start-Sleep -Milliseconds $(if ($ms) { [int]$ms } else { 60 })
        [void][Dh2InputV4]::MouseButton([uint32]$up)
        Start-Sleep -Milliseconds 80
        continue
    }
    if ($name -eq 'MOUSE') {
        # MOUSE:dx,dy - relative move, sent in 10 steps 15 ms apart (DirectInput reads relative deltas)
        $dx, $dy = $ms.Split(',') | ForEach-Object { [int]$_ }
        for ($s = 0; $s -lt 10; $s++) { [void][Dh2InputV4]::MouseMove([int]($dx / 10), [int]($dy / 10)); Start-Sleep -Milliseconds 15 }
        Start-Sleep -Milliseconds 80
        continue
    }
    $c = $codes[$name.ToUpper()]
    if (-not $c) { throw "unknown key '$name'" }
    $hold = if ($ms) { [int]$ms } else { 60 }
    [void][Dh2InputV4]::Key([uint16]$c[0], $c[1], $false)
    # A long hold ends early (key released) if the user brings another window to the front meanwhile.
    for ($left = $hold; $left -gt 0; $left -= 50) {
        Start-Sleep -Milliseconds ([Math]::Min(50, $left))
        if ([Dh2InputV4]::GetForegroundWindow() -ne $h) { break }
    }
    [void][Dh2InputV4]::Key([uint16]$c[0], $c[1], $true)
    Start-Sleep -Milliseconds 80
}
"sent $($Keys -join ' ') to pid $ProcessId (foreground ok=$([Dh2InputV4]::GetForegroundWindow() -eq $h))"
