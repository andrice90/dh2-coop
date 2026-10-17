# Same-map replay crash test (re/notes/replay-crash.md). One solo game from this checkout's game\Bin:
# load -Map, then `spmap` each map in -Steps through Bin\dh2coop.cmd, one after the other, and report whether
# a teardown crashed. Default steps: the same map twice (two replays), then another map.
# Usage:
#   .\tools-src\replay-test.ps1 [-Map MAP1] [-Steps MAP1,MAP1,MAP2A] [-Tag base] [-Set 'Puppets.Enabled=0','Overlay.Enabled=0']
#   .\tools-src\replay-test.ps1 -NoAsi       # dh2coop.asi set aside; tools-src\replaycmd\build\dh2cmd.asi only drives the command file
# -Set       ini overrides "Section.Key=Value" written into game\Bin\dh2coop.ini (a fresh copy of mod\dh2coop.ini).
# -Settle    seconds to stay in each map after it is up (engine frames keep running).
# Only this checkout's game is started or closed (matched by full path). Logs are copied to re\notes\coop-local\.
param(
    [string]$Map = 'MAP1',
    [string[]]$Steps = @('MAP1', 'MAP1', 'MAP2A'),
    [string]$Tag = 'solo',
    [string[]]$Set = @(),
    [switch]$NoAsi,
    [switch]$NoFovFix,      # also set aside the third-party CabelasDangerousHunts2FOVFix.asi
    [int]$Settle = 8,
    [int]$LoadTimeout = 90,
    [int]$Port = 27450,
    [string]$ExtraArgs = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$bin = Join-Path $root 'game\Bin'
$exe = Join-Path $bin 'Main7.exe'
$asiBuilt = Join-Path $root 'mod\build\dh2coop.asi'
$cmdAsi = Join-Path $root 'tools-src\replaycmd\build\dh2cmd.asi'
$iniSrc = Join-Path $root 'mod\dh2coop.ini'
$iniDst = Join-Path $bin 'dh2coop.ini'
$cfg = Join-Path $root 'tools-src\console.windowed.cfg'
$outDir = Join-Path $root 're\notes\coop-local'
$ccd = 'D:\scary hunt\CABELAS.DANGEROUS.HUNTS.2-Clone\Dngh2.ccd'
$stamp = Get-Date -Format 'HHmmss'
New-Item -ItemType Directory -Force $outDir | Out-Null

if (-not ('ReplayIni' -as [type])) {
    Add-Type @"
using System.Runtime.InteropServices;
public static class ReplayIni {
    [DllImport("kernel32.dll", CharSet=CharSet.Ansi)]
    public static extern bool WritePrivateProfileString(string section, string key, string value, string file);
}
"@
}

function Read-Shared([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return '' }
    try {
        $fs = [IO.File]::Open($Path, 'Open', 'Read', 'ReadWrite')
        try { return (New-Object IO.StreamReader($fs)).ReadToEnd() } finally { $fs.Dispose() }
    } catch { return '' }
}

function Count-Loads([string]$Text) { return ([regex]::Matches($Text, 'loading time :')).Count }

function Get-Crash([string]$DebugText, [string]$ModText) {
    $m = [regex]::Match($DebugText, '(?m)^.*(caught in function|Access violation|Unhandled exception).*$')
    if ($m.Success) { return $m.Value.Trim() }
    $m = [regex]::Match($ModText, '(?m)^.*unhandled exception.*$')
    if ($m.Success) { return $m.Value.Trim() }
    return $null
}

foreach ($p in @(Get-Process Main7 -ErrorAction SilentlyContinue)) {
    $path = $null; try { $path = $p.Path } catch { }
    if ($path -and ($path -ieq $exe)) { throw "Main7 from this checkout already runs (pid $($p.Id))" }
}
if (-not (Get-Volume | Where-Object { $_.DriveType -eq 'CD-ROM' -and $_.FileSystemLabel -eq 'Dngh2' })) {
    & 'C:\Program Files (x86)\Elaborate Bytes\VirtualCloneDrive\VCDMount.exe' $ccd
    Start-Sleep 4
}

# ASI set: the mod, or only the command-file driver (baseline without the mod).
$offDir = Join-Path $root 'game\asi-off'
New-Item -ItemType Directory -Force $offDir | Out-Null
Remove-Item (Join-Path $bin 'dh2cmd.asi') -ErrorAction SilentlyContinue
if ($NoAsi) {
    if (Test-Path (Join-Path $bin 'dh2coop.asi')) { Move-Item (Join-Path $bin 'dh2coop.asi') (Join-Path $offDir 'dh2coop.asi') -Force }
    if (-not (Test-Path $cmdAsi)) { throw "missing $cmdAsi (run tools-src\replaycmd\build.ps1)" }
    Copy-Item $cmdAsi (Join-Path $bin 'dh2cmd.asi') -Force
} else {
    Copy-Item $asiBuilt (Join-Path $bin 'dh2coop.asi') -Force
}
$fov = 'CabelasDangerousHunts2FOVFix.asi'
if ($NoFovFix -and (Test-Path (Join-Path $bin $fov))) { Move-Item (Join-Path $bin $fov) (Join-Path $offDir $fov) -Force }
Copy-Item $iniSrc $iniDst -Force
[void][ReplayIni]::WritePrivateProfileString('Coop', 'Port', "$Port", $iniDst)
foreach ($kv in $Set) {
    if ($kv -notmatch '^([^.]+)\.([^=]+)=(.*)$') { throw "bad -Set '$kv' (Section.Key=Value)" }
    [void][ReplayIni]::WritePrivateProfileString($Matches[1], $Matches[2], $Matches[3], $iniDst)
}
New-Item -ItemType Directory -Force (Join-Path $root 'game\Data\Cfg') | Out-Null
Copy-Item $cfg (Join-Path $root 'game\Data\Cfg\console.cfg') -Force
Remove-Item (Join-Path $bin 'dh2coop.cmd') -ErrorAction SilentlyContinue
Remove-Item (Join-Path $bin 'dh2coop.log') -ErrorAction SilentlyContinue
Remove-Item (Join-Path $bin 'dh2cmd.log') -ErrorAction SilentlyContinue
Remove-Item (Join-Path $bin 'debug.log') -ErrorAction SilentlyContinue

$cmdLine = ('"{0}" +mod data -nointro -developper -cheats -spmap {1} {2}' -f $exe, $Map, $ExtraArgs).Trim()
$r = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{ CommandLine = $cmdLine; CurrentDirectory = $bin }
if ($r.ReturnValue -ne 0) { throw "launch failed ($($r.ReturnValue))" }
$proc = Get-Process -Id ([int]$r.ProcessId)
Write-Host "[$Tag] pid=$($proc.Id) asi=$(if ($NoAsi) { 'dh2cmd only' } else { 'dh2coop' }) set=$($Set -join ' ')"

$debugPath = Join-Path $bin 'debug.log'
$modPath = if ($NoAsi) { Join-Path $bin 'dh2cmd.log' } else { Join-Path $bin 'dh2coop.log' }
$result = 'ok'
$crash = $null
$done = @()

function Wait-Loads([int]$Want, [string]$What) {
    $deadline = (Get-Date).AddSeconds($LoadTimeout)
    while ((Get-Date) -lt $deadline) {
        $proc.Refresh()
        $d = Read-Shared $debugPath
        $m = Read-Shared $modPath
        $c = Get-Crash $d $m
        if ($c) { return "crash: $c" }
        if ($proc.HasExited) { return "exited (code $($proc.ExitCode))" }
        if ((Count-Loads $d) -ge $Want) { return $null }
        Start-Sleep -Milliseconds 300
    }
    return "timeout waiting for $What"
}

function Settle-Check {
    $end = (Get-Date).AddSeconds($Settle)
    while ((Get-Date) -lt $end) {
        $proc.Refresh()
        $c = Get-Crash (Read-Shared $debugPath) (Read-Shared $modPath)
        if ($c) { return "crash: $c" }
        if ($proc.HasExited) { return "exited (code $($proc.ExitCode))" }
        Start-Sleep -Milliseconds 300
    }
    return $null
}

try {
    $err = Wait-Loads 1 "first map $Map"
    if (-not $err) { $err = Settle-Check }
    if ($err) { $result = "first load: $err" }
    else {
        $done += $Map
        $want = 1
        foreach ($s in $Steps) {
            $want++
            # The command file is polled every 0.5 s by the mod (or dh2cmd.asi).
            [IO.File]::WriteAllText((Join-Path $bin 'dh2coop.cmd'), "spmap $s`r`n")
            Write-Host "[$Tag] spmap $s"
            $err = Wait-Loads $want "spmap $s"
            if (-not $err) { $err = Settle-Check }
            if ($err) { $result = "step $want (spmap $s): $err"; break }
            $done += $s
        }
    }
} finally {
    $proc.Refresh()
    if (-not $proc.HasExited) {
        [void]$proc.CloseMainWindow()
        Start-Sleep 5
        $proc.Refresh()
        if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue }
    }
    Start-Sleep 1
    $base = Join-Path $outDir "replay-$Tag-$stamp"
    foreach ($pair in @(@($debugPath, "$base-debug.log"), @($modPath, "$base-mod.log"))) {
        if (Test-Path $pair[0]) { Copy-Item $pair[0] $pair[1] -Force }
    }
    if ($NoAsi) {
        Remove-Item (Join-Path $bin 'dh2cmd.asi') -ErrorAction SilentlyContinue
        if (Test-Path (Join-Path $offDir 'dh2coop.asi')) { Move-Item (Join-Path $offDir 'dh2coop.asi') (Join-Path $bin 'dh2coop.asi') -Force }
    }
    if (Test-Path (Join-Path $offDir $fov)) { Move-Item (Join-Path $offDir $fov) (Join-Path $bin $fov) -Force }
}
Write-Host "[$Tag] maps done: $($done -join ' -> ')"
Write-Host "[$Tag] RESULT: $result"
Write-Host "[$Tag] logs: $base-debug.log, $base-mod.log"
