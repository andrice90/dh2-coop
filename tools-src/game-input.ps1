# Sends keyboard input to a game window as hardware scan codes (SendInput + KEYEVENTF_SCANCODE), which
# DirectInput reads; window messages (PostMessage/SendKeys) do not reach it.
# Usage: .\tools-src\game-input.ps1 -ProcessId 1234 -Keys 'RETURN','W:2000','WAIT:500','DOWN'
#   NAME        tap (60 ms hold)      NAME:ms   hold for ms      WAIT:ms   pause
# Names: RETURN ESCAPE SPACE UP DOWN LEFT RIGHT W A S D E R F Q SHIFT CTRL TAB 1-4 GRAVE
param(
    [Parameter(Mandatory)][int]$ProcessId,
    [Parameter(Mandatory)][string[]]$Keys
)
$ErrorActionPreference = 'Stop'
if (-not ('Dh2Input' -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Dh2Input {
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
    public static uint Key(ushort scan, bool extended, bool up) {
        var i = new INPUT[1];
        i[0].type = 1;  // INPUT_KEYBOARD
        i[0].u.ki.wScan = scan;
        i[0].u.ki.dwFlags = 0x0008u | (extended ? 0x0001u : 0u) | (up ? 0x0002u : 0u);  // SCANCODE | EXTENDED | KEYUP
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
    SHIFT = @(0x2A, $false); CTRL = @(0x1D, $false); GRAVE = @(0x29, $false)
    '1' = @(0x02, $false); '2' = @(0x03, $false); '3' = @(0x04, $false); '4' = @(0x05, $false)
}
$h = (Get-Process -Id $ProcessId).MainWindowHandle
if ([Dh2Input]::GetForegroundWindow() -ne $h) {
    [void][Dh2Input]::SetForegroundWindow($h)
    Start-Sleep -Milliseconds 300
}
foreach ($k in $Keys) {
    $name, $ms = $k.Split(':')
    if ($name -eq 'WAIT') { Start-Sleep -Milliseconds ([int]$ms); continue }
    $c = $codes[$name.ToUpper()]
    if (-not $c) { throw "unknown key '$name'" }
    $hold = if ($ms) { [int]$ms } else { 60 }
    [void][Dh2Input]::Key([uint16]$c[0], $c[1], $false)
    Start-Sleep -Milliseconds $hold
    [void][Dh2Input]::Key([uint16]$c[0], $c[1], $true)
    Start-Sleep -Milliseconds 80
}
"sent $($Keys -join ' ') to pid $ProcessId (foreground ok=$([Dh2Input]::GetForegroundWindow() -eq $h))"
