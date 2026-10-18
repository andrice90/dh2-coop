# DH2 co-op kit installer.
# Finds the game's Bin folder (...\Activision Value\Magic Wand\DNGH2\Bin, the folder with Main7.exe) on the local
# fixed drives, copies the mod files into it, and puts two shortcuts on the desktop: DH2 HOST and DH2 JOIN.
# Double-click install.bat. Or from PowerShell in this folder:
#   powershell -ExecutionPolicy Bypass -File install.ps1 [-GameDir <...\DNGH2\Bin>] [-PlayerName <name>]
#   powershell -ExecutionPolicy Bypass -File install.ps1 -Uninstall      (removes the files and the shortcuts)
#   powershell -ExecutionPolicy Bypass -File install.ps1 -FindOnly       (only lists the game folders it finds)
# Another mod's dinput8.dll is kept as dinput8.dll.dh2coop-backup and put back by -Uninstall.
param(
    [string]$GameDir = '',
    [string]$PlayerName = '',
    [string]$ShortcutDir = '',
    [switch]$Uninstall,
    [switch]$FindOnly
)
$ErrorActionPreference = 'Stop'
$kit = $PSScriptRoot
$modFiles = @('dinput8.dll', 'dh2coop.asi', 'dh2coop.ini', 'host.bat', 'join.bat', 'host_dlss5.bat')
$tail = 'Activision Value\Magic Wand\DNGH2\Bin'
$shortcuts = @('DH2 HOST', 'DH2 JOIN', 'DH2 HOST DLSS')

function Find-GameBins {
    $found = New-Object System.Collections.Generic.List[string]
    $drives = @(Get-CimInstance Win32_LogicalDisk -Filter 'DriveType=3' | ForEach-Object { $_.DeviceID + '\' })
    foreach ($d in $drives) {
        # Usual places first.
        foreach ($base in @($d, (Join-Path $d 'Program Files (x86)'), (Join-Path $d 'Program Files'), (Join-Path $d 'Games'))) {
            $bin = Join-Path $base $tail
            if ((Test-Path -LiteralPath (Join-Path $bin 'Main7.exe')) -and -not $found.Contains($bin)) { $found.Add($bin) }
        }
    }
    if ($found.Count -gt 0) { return $found }
    foreach ($d in $drives) {
        Write-Host "Searching $d for $tail ..."
        Get-ChildItem -LiteralPath $d -Directory -Filter 'Activision Value' -Recurse -Depth 4 -ErrorAction SilentlyContinue |
            ForEach-Object {
                $bin = Join-Path $_.FullName 'Magic Wand\DNGH2\Bin'
                if ((Test-Path -LiteralPath (Join-Path $bin 'Main7.exe')) -and -not $found.Contains($bin)) { $found.Add($bin) }
            }
    }
    return $found
}

function Select-GameBin {
    if ($GameDir) {
        if (-not (Test-Path -LiteralPath (Join-Path $GameDir 'Main7.exe'))) { throw "Main7.exe not found in $GameDir" }
        return (Resolve-Path -LiteralPath $GameDir).Path
    }
    $bins = @(Find-GameBins)
    if ($bins.Count -eq 0) {
        throw "Could not find '$tail' with Main7.exe on any local drive. Run again with -GameDir <your game's Bin folder>."
    }
    if ($bins.Count -eq 1) { return $bins[0] }
    Write-Host 'Found more than one game folder:'
    for ($i = 0; $i -lt $bins.Count; $i++) { Write-Host ("  {0}) {1}" -f ($i + 1), $bins[$i]) }
    $pick = Read-Host "Which one (1-$($bins.Count))"
    $n = 0
    if (-not [int]::TryParse($pick, [ref]$n) -or $n -lt 1 -or $n -gt $bins.Count) { throw 'No folder chosen.' }
    return $bins[$n - 1]
}

function Test-Writable([string]$dir) {
    $probe = Join-Path $dir ('dh2coop-write-test-' + [guid]::NewGuid().ToString('N') + '.tmp')
    try {
        [IO.File]::WriteAllText($probe, 'x')
        Remove-Item -LiteralPath $probe -Force
        return $true
    } catch {
        return $false
    }
}

function Same-File([string]$a, [string]$b) {
    if (-not (Test-Path -LiteralPath $a) -or -not (Test-Path -LiteralPath $b)) { return $false }
    return (Get-FileHash -LiteralPath $a).Hash -eq (Get-FileHash -LiteralPath $b).Hash
}

function Set-IniPlayerName([string]$path, [string]$name) {
    $text = [IO.File]::ReadAllText($path)
    $text = [regex]::Replace($text, '(?m)^PlayerName=.*$', "PlayerName=$name")
    [IO.File]::WriteAllText($path, $text, (New-Object System.Text.ASCIIEncoding))
}

function Get-IniPlayerName([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) { return '' }
    $m = [regex]::Match([IO.File]::ReadAllText($path), '(?m)^PlayerName=(.*)$')
    if ($m.Success) { return $m.Groups[1].Value.Trim() }
    return ''
}

function New-Shortcut([string]$lnk, [string]$target, [string]$workDir, [string]$icon, [string]$description) {
    $shell = New-Object -ComObject WScript.Shell
    $s = $shell.CreateShortcut($lnk)
    $s.TargetPath = $target
    $s.WorkingDirectory = $workDir
    $s.IconLocation = "$icon,0"
    $s.Description = $description
    $s.Save()
}

if ($FindOnly) {
    $bins = @(Find-GameBins)
    if ($bins.Count -eq 0) { 'No game folder found.' } else { $bins }
    return
}

$bin = Select-GameBin
$desk = if ($ShortcutDir) { $ShortcutDir } else { [Environment]::GetFolderPath('Desktop') }
Write-Host "Game folder: $bin"

if (-not (Test-Writable $bin)) {
    # Program Files needs administrator rights. Ask once, and pass the folders on so the shortcuts still land on
    # this user's desktop.
    Write-Host 'That folder needs administrator rights; Windows will ask for permission.'
    $argList = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"", '-GameDir', "`"$bin`"",
                 '-ShortcutDir', "`"$desk`"")
    if ($PlayerName) { $argList += @('-PlayerName', "`"$PlayerName`"") }
    if ($Uninstall) { $argList += '-Uninstall' }
    $p = Start-Process -FilePath 'powershell.exe' -ArgumentList ($argList -join ' ') -Verb RunAs -Wait -PassThru
    exit $p.ExitCode
}

if ($Uninstall) {
    foreach ($f in @('dh2coop.asi', 'dh2coop.ini', 'host.bat', 'join.bat', 'host_dlss5.bat', 'dh2coop.log', 'dh2coop.cmd',
                     'dh2coop-lastmap.txt', 'dh2coop-lasthost.txt', 'dh2coop-classes.txt')) {
        Remove-Item -LiteralPath (Join-Path $bin $f) -Force -ErrorAction SilentlyContinue
    }
    # Only remove the loader if it is the one this kit installed; put a previous one back.
    $loader = Join-Path $bin 'dinput8.dll'
    if (Same-File $loader (Join-Path $kit 'dinput8.dll')) { Remove-Item -LiteralPath $loader -Force }
    $bak = Join-Path $bin 'dinput8.dll.dh2coop-backup'
    if (Test-Path -LiteralPath $bak) { Move-Item -LiteralPath $bak -Destination $loader -Force }
    foreach ($name in $shortcuts) { Remove-Item -LiteralPath (Join-Path $desk "$name.lnk") -Force -ErrorAction SilentlyContinue }
    Write-Host 'Removed the co-op mod and the desktop shortcuts.'
    return
}

foreach ($f in $modFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $kit $f))) { throw "The kit is missing $f (unzip the whole kit first)." }
}

# Keep the player name of an earlier install; otherwise ask once.
$iniPath = Join-Path $bin 'dh2coop.ini'
if (-not $PlayerName) { $PlayerName = Get-IniPlayerName $iniPath }
if (-not $PlayerName -or $PlayerName -eq 'Hunter') {
    $typed = Read-Host "Your player name (Enter = $env:USERNAME)"
    $PlayerName = if ($typed) { $typed } else { $env:USERNAME }
}
$PlayerName = ($PlayerName -replace '[^A-Za-z0-9 _\-]', '').Trim()
if (-not $PlayerName) { $PlayerName = 'Hunter' }
if ($PlayerName.Length -gt 31) { $PlayerName = $PlayerName.Substring(0, 31) }

# A loader that is not ours (another mod's dinput8.dll) is kept aside once. An existing dh2coop.ini is ours: only
# its player name is carried over.
$loader = Join-Path $bin 'dinput8.dll'
$loaderBak = "$loader.dh2coop-backup"
if ((Test-Path -LiteralPath $loader) -and -not (Test-Path -LiteralPath $loaderBak) -and
    -not (Same-File $loader (Join-Path $kit 'dinput8.dll'))) {
    Copy-Item -LiteralPath $loader -Destination $loaderBak
}
foreach ($f in $modFiles) { Copy-Item -LiteralPath (Join-Path $kit $f) -Destination (Join-Path $bin $f) -Force }
Set-IniPlayerName $iniPath $PlayerName

$exe = Join-Path $bin 'Main7.exe'
New-Item -ItemType Directory -Force $desk | Out-Null
New-Shortcut (Join-Path $desk 'DH2 HOST.lnk') (Join-Path $bin 'host.bat') $bin $exe 'Host a Dangerous Hunts 2 co-op game'
New-Shortcut (Join-Path $desk 'DH2 JOIN.lnk') (Join-Path $bin 'join.bat') $bin $exe 'Join a Dangerous Hunts 2 co-op game'
New-Shortcut (Join-Path $desk 'DH2 HOST DLSS.lnk') (Join-Path $bin 'host_dlss5.bat') $bin $exe 'Host a co-op game with the desktop resolution, wide FOV, and DLSS 5'

Write-Host ''
Write-Host "Installed for player '$PlayerName' in $bin"
Write-Host "Desktop shortcuts: DH2 HOST, DH2 JOIN ($desk)"
Write-Host 'The host needs to allow Main7.exe through Windows Firewall the first time (UDP 27420).'
