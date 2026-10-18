# DH2 co-op uninstaller: removes every file any version of the co-op kit or the mod created, so an old version is
# gone completely. Double-click uninstall.bat (in the kit folder, or in the game's Bin folder where the installer
# puts a copy). From PowerShell:
#   powershell -ExecutionPolicy Bypass -File uninstall.ps1 [-GameDir <...\DNGH2\Bin>]
#   powershell -ExecutionPolicy Bypass -File uninstall.ps1 -DryRun   (lists what it would remove, changes nothing)
# What goes:
#   - in the game's Bin: dh2coop.asi, dh2coop.ini, every dh2coop* file the mod writes (logs, crash dump, lobby
#     settings with the saved friend key, command and test files), the debug.1/2.log copies, host.bat, join.bat,
#     host_dlss5.bat, this uninstaller's copy
#   - the ASI loader dinput8.dll when this kit put it there; another mod's loader that the installer kept as
#     dinput8.dll.dh2coop-backup is put back
#   - %LOCALAPPDATA%\dh2coop (logs and settings of a game under Program Files, join.bat's last host) and old files
#     Windows redirected to %LOCALAPPDATA%\VirtualStore
#   - the desktop shortcuts DH2 COOP, DH2 HOST, DH2 JOIN, DH2 HOST DLSS
#   - Windows Firewall rules named "DH2 coop ..." (one administrator prompt, only when there is one to remove or the
#     game folder needs it)
# What stays: the game itself, Tailscale (other programs may use it), and the firewall entries Windows made itself
# for Main7.exe when it asked the first time (the game uses them without the mod too).
# Run from the kit folder it finds every game folder with the mod in it; run from a Bin folder it cleans that one.
param(
    [string]$GameDir = '',      # one folder, or several separated by |
    [string]$ShortcutDir = '',
    [string]$DataDir = '',      # default %LOCALAPPDATA%\dh2coop
    [string]$VirtualStore = '', # default %LOCALAPPDATA%\VirtualStore
    [string]$KitDir = '',       # the kit whose dinput8.dll counts as ours (install.ps1 -Uninstall passes it)
    [switch]$DryRun,            # change nothing, print what would go
    [switch]$FilesOnly,         # remove files, only print the firewall step (install.ps1 -DryRun -Uninstall, tests)
    [switch]$Yes,               # no questions, no "press Enter"
    [switch]$Elevated           # internal: the restart as administrator (the question was already answered)
)
$ErrorActionPreference = 'Stop'
$tail = 'Activision Value\Magic Wand\DNGH2\Bin'
$shortcutNames = @('DH2 COOP', 'DH2 HOST', 'DH2 JOIN', 'DH2 HOST DLSS')
$shortcutTargets = @('Main7.exe', 'host.bat', 'join.bat', 'host_dlss5.bat')
$batFiles = @('host.bat', 'join.bat', 'host_dlss5.bat', 'uninstall.bat')
$fwPattern = 'DH2 coop*'
if (-not $DataDir -and $env:LOCALAPPDATA) { $DataDir = Join-Path $env:LOCALAPPDATA 'dh2coop' }
if (-not $VirtualStore -and $env:LOCALAPPDATA) { $VirtualStore = Join-Path $env:LOCALAPPDATA 'VirtualStore' }
$desk = if ($ShortcutDir) { $ShortcutDir } else { [Environment]::GetFolderPath('Desktop') }

$script:removed = 0
$script:kept = New-Object System.Collections.Generic.List[string]

function Say([string]$t) { Write-Host "   $t" }
function Gone([string]$what) {
    $script:removed++
    if ($DryRun) { Write-Host "   would remove $what" -ForegroundColor DarkYellow } else { Write-Host "   removed $what" }
}
function Keep([string]$why) { $script:kept.Add($why); Write-Host "   kept $why" -ForegroundColor Yellow }

function Remove-Path([string]$path, [string]$label = '') {
    if (-not (Test-Path -LiteralPath $path)) { return }
    if (-not $label) { $label = $path }
    if (-not $DryRun) { Remove-Item -LiteralPath $path -Recurse -Force }
    Gone $label
}

function Find-GameBins {
    $found = New-Object System.Collections.Generic.List[string]
    $drives = @(Get-CimInstance Win32_LogicalDisk -Filter 'DriveType=3' | ForEach-Object { $_.DeviceID + '\' })
    foreach ($d in $drives) {
        foreach ($base in @($d, (Join-Path $d 'Program Files (x86)'), (Join-Path $d 'Program Files'), (Join-Path $d 'Games'))) {
            $bin = Join-Path $base $tail
            if ((Test-Path -LiteralPath (Join-Path $bin 'Main7.exe')) -and -not $found.Contains($bin)) { $found.Add($bin) }
        }
    }
    foreach ($d in $drives) {
        Get-ChildItem -LiteralPath $d -Directory -Filter 'Activision Value' -Recurse -Depth 4 -ErrorAction SilentlyContinue |
            ForEach-Object {
                $bin = Join-Path $_.FullName 'Magic Wand\DNGH2\Bin'
                if ((Test-Path -LiteralPath (Join-Path $bin 'Main7.exe')) -and -not $found.Contains($bin)) { $found.Add($bin) }
            }
    }
    return $found
}

# Anything of ours in this Bin?
function Test-HasMod([string]$bin) {
    if (@(Get-ChildItem -LiteralPath $bin -Filter 'dh2coop*' -File -ErrorAction SilentlyContinue).Count -gt 0) { return $true }
    if (Test-Path -LiteralPath (Join-Path $bin 'dinput8.dll.dh2coop-backup')) { return $true }
    foreach ($b in $batFiles) { if (Test-OurBat (Join-Path $bin $b)) { return $true } }
    return $false
}

# host.bat / join.bat / host_dlss5.bat / uninstall.bat with these names could be someone else's: only ours go.
function Test-OurBat([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) { return $false }
    $text = [IO.File]::ReadAllText($path)
    return ($text -match 'co-?op' -and ($text -match 'Main7\.exe' -or $text -match 'uninstall\.ps1'))
}

function Test-Writable([string]$dir) {
    $probe = Join-Path $dir ('dh2coop-write-test-' + [guid]::NewGuid().ToString('N') + '.tmp')
    try { [IO.File]::WriteAllText($probe, 'x'); Remove-Item -LiteralPath $probe -Force; return $true } catch { return $false }
}

function Test-IsAdmin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    return ([Security.Principal.WindowsPrincipal]$id).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Same-File([string]$a, [string]$b) {
    if (-not (Test-Path -LiteralPath $a) -or -not (Test-Path -LiteralPath $b)) { return $false }
    return (Get-FileHash -LiteralPath $a).Hash -eq (Get-FileHash -LiteralPath $b).Hash
}

function Get-FirewallRules {
    try { return @(Get-NetFirewallRule -DisplayName $fwPattern -ErrorAction SilentlyContinue) } catch { return @() }
}

function Remove-ModFromBin([string]$bin) {
    Write-Host ''
    Write-Host "== $bin" -ForegroundColor Cyan
    # The ASI loader decision needs to know whether other ASI mods stay, so the mod itself goes first.
    foreach ($f in @(Get-ChildItem -LiteralPath $bin -Filter 'dh2coop*' -File -ErrorAction SilentlyContinue)) {
        Remove-Path $f.FullName $f.Name
    }
    foreach ($f in @('debug.1.log', 'debug.2.log')) { Remove-Path (Join-Path $bin $f) $f }  # the mod's copies of the engine log
    foreach ($b in $batFiles) {
        $p = Join-Path $bin $b
        # Started from uninstall.bat in this folder: the .bat deletes itself after this script (cmd still reads it).
        if ($b -eq 'uninstall.bat' -and $env:DH2COOP_FROM_BAT -eq '1' -and $bin -eq $PSScriptRoot) { continue }
        if (Test-OurBat $p) { Remove-Path $p $b } elseif (Test-Path -LiteralPath $p) { Keep "$b (not the co-op kit's)" }
    }
    $self = Join-Path $bin 'uninstall.ps1'
    if ((Test-Path -LiteralPath $self) -and ([IO.File]::ReadAllText($self) -match 'DH2 co-op uninstaller')) { Remove-Path $self 'uninstall.ps1' }

    # dinput8.dll: put a previous loader back, or remove ours when no other ASI mod needs it.
    $loader = Join-Path $bin 'dinput8.dll'
    $bak = Join-Path $bin 'dinput8.dll.dh2coop-backup'
    if (Test-Path -LiteralPath $bak) {
        if (-not $DryRun) { Move-Item -LiteralPath $bak -Destination $loader -Force }
        Gone 'dinput8.dll (the earlier loader from dinput8.dll.dh2coop-backup is back in its place)'
    } elseif (Test-Path -LiteralPath $loader) {
        $ours = $KitDir -and (Same-File $loader (Join-Path $KitDir 'dinput8.dll'))
        $isUal = (Get-Item -LiteralPath $loader).VersionInfo.FileDescription -eq 'Ultimate ASI Loader'
        $others = @()
        foreach ($d in @($bin, (Join-Path $bin 'scripts'), (Join-Path $bin 'plugins'))) {
            $others += @(Get-ChildItem -LiteralPath $d -Filter '*.asi' -File -ErrorAction SilentlyContinue |
                         Where-Object { $_.Name -ne 'dh2coop.asi' })
        }
        if ($others.Count -gt 0) {
            Keep "dinput8.dll (other ASI mods use it: $(($others | ForEach-Object Name) -join ', '))"
        } elseif ($ours -or $isUal) {
            Remove-Path $loader 'dinput8.dll (Ultimate ASI Loader)'
        } else {
            Keep 'dinput8.dll (not the ASI loader the kit installs; another program''s file)'
        }
    }

    # Files Windows redirected to the VirtualStore when an old version wrote into a game under Program Files.
    if ($VirtualStore -and $bin -match '^[A-Za-z]:\\(.+)$') {
        $vs = Join-Path $VirtualStore $Matches[1]
        if (Test-Path -LiteralPath $vs) {
            foreach ($f in @(Get-ChildItem -LiteralPath $vs -Filter 'dh2coop*' -File -ErrorAction SilentlyContinue)) {
                Remove-Path $f.FullName "VirtualStore\...\$($f.Name)"
            }
            foreach ($f in @('debug.1.log', 'debug.2.log')) { Remove-Path (Join-Path $vs $f) "VirtualStore\...\$f" }
        }
    }
}

function Remove-Shortcuts {
    Write-Host ''
    Write-Host "== Desktop shortcuts ($desk)" -ForegroundColor Cyan
    $shell = New-Object -ComObject WScript.Shell
    foreach ($name in $shortcutNames) {
        $lnk = Join-Path $desk "$name.lnk"
        if (-not (Test-Path -LiteralPath $lnk)) { continue }
        $target = $shell.CreateShortcut($lnk).TargetPath
        $leaf = if ($target) { Split-Path -Leaf $target } else { '' }
        if (-not $target -or $shortcutTargets -contains $leaf) { Remove-Path $lnk "$name.lnk" }
        else { Keep "$name.lnk (it starts $target, not the game)" }
    }
}

function Remove-FirewallRules([object[]]$rules) {
    Write-Host ''
    Write-Host '== Windows Firewall' -ForegroundColor Cyan
    if ($rules.Count -eq 0) { Say 'no "DH2 coop" rules'; return }
    foreach ($r in $rules) {
        if ($DryRun -or $FilesOnly) {
            Write-Host "   [dry run] would run: Get-NetFirewallRule -DisplayName '$($r.DisplayName)' | Remove-NetFirewallRule" -ForegroundColor DarkYellow
            $script:removed++
        } else {
            $r | Remove-NetFirewallRule
            Gone "firewall rule '$($r.DisplayName)'"
        }
    }
}

# ---------------------------------------------------------------------------------------------------------------
$exitCode = 0
try {
    # Which game folders: the one given, the one this copy sits in, or every one on this PC that has the mod.
    $bins = New-Object System.Collections.Generic.List[string]
    if ($GameDir) {
        foreach ($g in ($GameDir -split '\|')) {
            if (-not $g) { continue }
            if (-not (Test-Path -LiteralPath (Join-Path $g 'Main7.exe'))) { throw "Main7.exe not found in $g" }
            $bins.Add((Resolve-Path -LiteralPath $g).Path)
        }
    } elseif (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'Main7.exe')) {
        $bins.Add($PSScriptRoot)
    } else {
        Write-Host 'Looking for the game on this PC ...'
        foreach ($b in @(Find-GameBins)) { if (Test-HasMod $b) { $bins.Add($b) } }
    }

    Write-Host ''
    Write-Host 'DH2 co-op uninstaller' -ForegroundColor Cyan
    if ($DryRun) { Write-Host '(dry run: nothing is changed)' -ForegroundColor DarkYellow }
    if ($bins.Count -eq 0) { Say 'No game folder with the co-op mod in it was found (use -GameDir <...\DNGH2\Bin> for another place).' }
    foreach ($b in $bins) { Say "game folder: $b" }
    $dataHas = $DataDir -and (Test-Path -LiteralPath $DataDir)
    if ($dataHas) { Say "settings and logs: $DataDir" }
    $rules = Get-FirewallRules
    if ($rules.Count -gt 0) { Say "firewall rules: $(($rules | ForEach-Object DisplayName) -join ', ')" }

    # A running game holds dh2coop.asi open.
    $running = @(Get-CimInstance Win32_Process -Filter "Name='Main7.exe'" -ErrorAction SilentlyContinue |
                 Where-Object { $_.ExecutablePath -and ($bins -contains (Split-Path -Parent $_.ExecutablePath)) })
    if ($running.Count -gt 0 -and -not $DryRun) { throw 'The game is running from that folder. Close it, then run the uninstaller again.' }

    if (-not $DryRun -and -not $Yes -and -not $Elevated) {
        $a = Read-Host 'Remove the co-op mod from all of these? [Y/n]'
        if ($a -and $a.Trim() -notmatch '^(y|yes)$') { Write-Host 'Nothing was removed.'; exit 0 }
    }

    # Administrator rights only when a game folder needs them (Program Files) or a firewall rule has to go.
    # (A dry run writes nothing, not even the write probe.)
    $needAdmin = (-not $DryRun) -and -not $Elevated -and -not (Test-IsAdmin) -and
                 (($rules.Count -gt 0 -and -not $FilesOnly) -or (@($bins | Where-Object { -not (Test-Writable $_) }).Count -gt 0))
    if ($needAdmin) {
        Write-Host 'Windows will ask for permission once (the firewall rule or the game folder needs administrator rights).'
        $argList = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"", '-Elevated',
                     '-ShortcutDir', "`"$desk`"")
        if ($Yes) { $argList += '-Yes' }
        if ($bins.Count -gt 0) { $argList += @('-GameDir', "`"$($bins -join '|')`"") }
        if ($DataDir) { $argList += @('-DataDir', "`"$DataDir`"") }
        if ($VirtualStore) { $argList += @('-VirtualStore', "`"$VirtualStore`"") }
        if ($KitDir) { $argList += @('-KitDir', "`"$KitDir`"") }
        try {
            $p = Start-Process -FilePath 'powershell.exe' -ArgumentList ($argList -join ' ') -Verb RunAs -Wait -PassThru
            exit $p.ExitCode
        } catch {
            Write-Host 'Administrator permission was not given, so nothing was removed. Run uninstall.bat again and press Yes.' -ForegroundColor Yellow
            exit 1
        }
    }

    foreach ($b in $bins) { Remove-ModFromBin $b }
    if ($dataHas -and -not ($bins -contains (Resolve-Path -LiteralPath $DataDir).Path)) {
        Write-Host ''
        Write-Host "== $DataDir" -ForegroundColor Cyan
        # The whole folder only when it is the mod's own (%LOCALAPPDATA%\dh2coop); elsewhere only the mod's files.
        if ((Split-Path -Leaf $DataDir) -eq 'dh2coop' -and -not (Test-Path -LiteralPath (Join-Path $DataDir 'Main7.exe'))) {
            Remove-Path $DataDir 'the folder and everything in it'
        } else {
            foreach ($f in @(Get-ChildItem -LiteralPath $DataDir -Filter 'dh2coop*' -File -ErrorAction SilentlyContinue)) {
                Remove-Path $f.FullName $f.Name
            }
        }
    }
    Remove-Shortcuts
    Remove-FirewallRules $rules

    Write-Host ''
    Write-Host '== Done' -ForegroundColor Cyan
    if ($DryRun) { Say "$($script:removed) items would be removed; run again without -DryRun to remove them." }
    elseif ($script:removed -eq 0) { Say 'Nothing of the co-op mod was left to remove.' }
    else { Say 'The co-op mod is removed.' }
    if ($script:kept.Count -gt 0) { Say "Kept on purpose: $($script:kept.Count) item(s), listed above." }
    Say 'Tailscale stays installed (other programs may use it). Remove it in Windows Settings, Apps, if you want to.'
    Say 'If you joined a host''s tailnet and want off it: Tailscale icon in the taskbar, Log out.'
} catch {
    Write-Host ''
    Write-Host ('ERROR: ' + $_.Exception.Message) -ForegroundColor Red
    $exitCode = 1
}
if ($Elevated -and -not $Yes) { [void](Read-Host 'Press Enter to close this window') }
exit $exitCode
