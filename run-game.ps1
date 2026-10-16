# Launches the sandbox game copy the same way Launcher.exe does.
# Usage: .\run-game.ps1 [-Seconds 30] [-ExtraArgs '-developper']
param(
    [int]$Seconds = 0,          # >0: stop the game after N seconds (for automated smoke tests)
    [string]$ExtraArgs = ''
)
$ErrorActionPreference = 'Stop'
$bin = Join-Path $PSScriptRoot 'game\Bin'
$ccd = 'D:\scary hunt\CABELAS.DANGEROUS.HUNTS.2-Clone\Dngh2.ccd'

if (-not (Get-Volume | Where-Object { $_.DriveType -eq 'CD-ROM' -and $_.FileSystemLabel -eq 'Dngh2' })) {
    & 'C:\Program Files (x86)\Elaborate Bytes\VirtualCloneDrive\VCDMount.exe' $ccd
    Start-Sleep 4
}

Remove-Item (Join-Path $bin 'debug.log') -ErrorAction SilentlyContinue
$p = Start-Process (Join-Path $bin 'Main7.exe') -ArgumentList "+mod data $ExtraArgs".Trim() -WorkingDirectory $bin -PassThru
"Main7.exe pid $($p.Id)"

if ($Seconds -gt 0) {
    Start-Sleep $Seconds
    if (-not $p.HasExited) {
        $p.CloseMainWindow() | Out-Null
        Start-Sleep 3
        if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    }
}
