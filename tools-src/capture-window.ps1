# Captures the main window of a process to a PNG (window must be visible and not minimized; windowed mode works best).
# Usage: .\tools-src\capture-window.ps1 -ProcessId 1234 -Out re\notes\shot.png
param(
    [Parameter(Mandatory)][int]$ProcessId,
    [Parameter(Mandatory)][string]$Out,
    [switch]$NoFocus,  # side-by-side windows are visible without focus; focusing would steal DirectInput
    [switch]$PrintWindow  # ask the window to draw itself (PW_RENDERFULLCONTENT) instead of copying the screen:
                          # other windows on top (the agents' 2x2 grids overlap) do not end up in the picture
)
if (-not [System.IO.Path]::IsPathRooted($Out)) { $Out = Join-Path (Get-Location) $Out }
$Out = [System.IO.Path]::GetFullPath($Out)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Win {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
"@
if ($PrintWindow) { [void][Win]::SetProcessDPIAware() }  # real pixel size of the window; the default mode is unchanged
$p = Get-Process -Id $ProcessId
$h = $p.MainWindowHandle
if ($h -eq [IntPtr]::Zero) { throw "process $ProcessId has no main window" }
if (-not $NoFocus) { [Win]::SetForegroundWindow($h) | Out-Null }
Start-Sleep -Milliseconds 400
$r = New-Object Win+RECT
[Win]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.R - $r.L; $hgt = $r.B - $r.T
$bmp = New-Object System.Drawing.Bitmap $w, $hgt
$g = [System.Drawing.Graphics]::FromImage($bmp)
if ($PrintWindow) {
    # PrintWindow of a D3D window sometimes returns a black frame (2 of 3 tries in testing): retry until the
    # picture has some light in it.
    for ($try = 0; $try -lt 8; $try++) {
        $hdc = $g.GetHdc()
        $ok = [Win]::PrintWindow($h, $hdc, 2)
        $g.ReleaseHdc($hdc)
        if (-not $ok) { throw 'PrintWindow failed' }
        $lit = 0
        for ($yy = 8; $yy -lt $hgt; $yy += [Math]::Max(1, [int]($hgt / 24))) {
            for ($xx = 8; $xx -lt $w; $xx += [Math]::Max(1, [int]($w / 24))) {
                $c = $bmp.GetPixel($xx, $yy)
                if (($c.R + $c.G + $c.B) -gt 0) { $lit++ }
            }
        }
        if ($lit -gt 12) { break }
        Start-Sleep -Milliseconds 250
    }
} else {
    $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
}
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
"captured ${w}x${hgt} -> $Out"
