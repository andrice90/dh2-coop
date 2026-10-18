# DH2 co-op kit installer.
# Finds the game's Bin folder (...\Activision Value\Magic Wand\DNGH2\Bin, the folder with Main7.exe) on the local
# fixed drives, copies the mod files into it, puts one shortcut on the desktop (DH2 COOP: the game at its main menu;
# host or join from the co-op lobby window there, F8) and lets the game port through Windows Firewall for hosting.
# Joining needs nothing else: the host's lobby shows a join code (internet, through the router's UPnP port forward)
# and its tailnet / LAN addresses; friends paste or type one into JOIN. Tailscale is not set up here (manual fallback,
# see README.txt).
# Double-click install.bat. Or from PowerShell in this folder:
#   powershell -ExecutionPolicy Bypass -File install.ps1 [-GameDir <...\DNGH2\Bin>] [-PlayerName <name>]
#   powershell -ExecutionPolicy Bypass -File install.ps1 -Uninstall   (runs uninstall.ps1: every version's files, the
#                                                                      shortcuts, the firewall rules)
#   powershell -ExecutionPolicy Bypass -File install.ps1 -FindOnly    (only lists the game folders it finds)
#   -DryRun   prints the firewall step instead of running it (the files are still copied; no administrator prompt).
#   -Yes      no questions, no "press Enter" (tests).
# One administrator prompt (UAC) for the firewall rule (and for a game under Program Files). Saying No still installs
# the mod when the game folder is writable; Windows then asks about the firewall the first time you host.
# Another mod's dinput8.dll is kept as dinput8.dll.dh2coop-backup and put back by the uninstaller.
param(
    [string]$GameDir = '',
    [string]$PlayerName = '',
    [string]$ShortcutDir = '',
    [switch]$Uninstall,
    [switch]$FindOnly,
    [switch]$DryRun,
    [switch]$Yes,
    # tests: -Uninstall passes this to uninstall.ps1 instead of %LOCALAPPDATA%dh2coop
    [string]$DataDir = '',
    # internal: set when this script restarts itself elevated
    [switch]$Elevated
)
$ErrorActionPreference = 'Stop'
$kit = $PSScriptRoot
$modFiles = @('dinput8.dll', 'dh2coop.asi', 'dh2coop.ini', 'host.bat', 'join.bat', 'uninstall.bat', 'uninstall.ps1')
$tail = 'Activision Value\Magic Wand\DNGH2\Bin'
$oldShortcuts = @('DH2 HOST', 'DH2 JOIN', 'DH2 HOST DLSS')  # earlier kits; removed on install
$launchArgs = '+mod data -nointro -developper -cheats'
$fwRuleName = 'DH2 coop (game)'
$fwOldRules = @('DH2 coop (Tailscale)')  # the previous kit's rule
$gamePort = 27420

function Write-Step([string]$t) { Write-Host ''; Write-Host "== $t" -ForegroundColor Cyan }
function Write-Ok([string]$t) { Write-Host "   $t" -ForegroundColor Green }
function Write-Problem([string]$t) { Write-Host "   $t" -ForegroundColor Yellow }
function Write-Info([string]$t) { Write-Host "   $t" }
function Write-Dry([string]$t) { Write-Host "   [dry run] would run: $t" -ForegroundColor DarkYellow }

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

function Test-IsAdmin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    return ([Security.Principal.WindowsPrincipal]$id).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Same-File([string]$a, [string]$b) {
    if (-not (Test-Path -LiteralPath $a) -or -not (Test-Path -LiteralPath $b)) { return $false }
    return (Get-FileHash -LiteralPath $a).Hash -eq (Get-FileHash -LiteralPath $b).Hash
}

# One key=value line of dh2coop.ini ([^\r\n]: the file's CRLF line ends stay as they are).
function Set-IniValue([string]$path, [string]$key, [string]$value) {
    $text = [IO.File]::ReadAllText($path)
    $text = [regex]::Replace($text, "(?m)^$key=[^\r\n]*", { param($m) "$key=$value" })
    [IO.File]::WriteAllText($path, $text, (New-Object System.Text.ASCIIEncoding))
}

function Get-IniValue([string]$path, [string]$key) {
    if (-not (Test-Path -LiteralPath $path)) { return '' }
    $m = [regex]::Match([IO.File]::ReadAllText($path), "(?m)^$key=([^\r\n]*)")
    if ($m.Success) { return $m.Groups[1].Value.Trim() }
    return ''
}

function New-Shortcut([string]$lnk, [string]$target, [string]$workDir, [string]$icon, [string]$description,
                      [string]$arguments = '') {
    $shell = New-Object -ComObject WScript.Shell
    $s = $shell.CreateShortcut($lnk)
    $s.TargetPath = $target
    $s.Arguments = $arguments
    $s.WorkingDirectory = $workDir
    $s.IconLocation = "$icon,0"
    $s.Description = $description
    $s.Save()
}

# The host's side of a join over the internet or the LAN: Main7.exe may receive UDP on the game port.
function Set-GameFirewall([string]$exe) {
    foreach ($old in $fwOldRules) {
        if ($DryRun) { Write-Dry "Get-NetFirewallRule -DisplayName '$old' | Remove-NetFirewallRule" }
        else { Get-NetFirewallRule -DisplayName $old -ErrorAction SilentlyContinue | Remove-NetFirewallRule }
    }
    $cmdAdd = "New-NetFirewallRule -DisplayName '$fwRuleName' -Direction Inbound -Program '$exe' -Protocol UDP -LocalPort $gamePort -Action Allow -Profile Any"
    if ($DryRun) {
        Write-Dry "Get-NetFirewallRule -DisplayName '$fwRuleName' | Remove-NetFirewallRule"
        Write-Dry $cmdAdd
    } else {
        Get-NetFirewallRule -DisplayName $fwRuleName -ErrorAction SilentlyContinue | Remove-NetFirewallRule
        [void](New-NetFirewallRule -DisplayName $fwRuleName -Direction Inbound -Program $exe -Protocol UDP `
            -LocalPort $gamePort -Action Allow -Profile Any `
            -Description 'DH2 co-op: friends can reach the game you host (UDP 27420, only while Main7.exe listens).')
        Write-Ok "Firewall rule '$fwRuleName': Main7.exe may receive UDP $gamePort (for hosting)."
    }
    # Read only: a Block rule that Windows made when someone pressed Cancel on its first-run prompt wins over Allow.
    try {
        $blocks = @(Get-NetFirewallApplicationFilter -Program $exe -ErrorAction Stop | Get-NetFirewallRule -ErrorAction Stop |
            Where-Object { $_.Enabled -eq 'True' -and $_.Direction -eq 'Inbound' -and $_.Action -eq 'Block' })
        if ($blocks.Count -gt 0) {
            Write-Problem ('A Windows Firewall rule BLOCKS this game: ' + (($blocks | ForEach-Object { $_.DisplayName }) -join ', '))
            Write-Problem 'Blocking beats allowing. Delete that rule (Windows Security, Firewall, Advanced settings, Inbound Rules).'
        }
    } catch { }
}

# Read only. Smart App Control (Windows 11) refuses unsigned DLLs it does not know: the game then starts without the
# mod and Windows names dh2coop.asi with error 0xc0e90002 ("An Application Control policy has blocked this file").
function Show-SmartAppControl {
    $state = $null
    try {
        $state = (Get-ItemProperty -LiteralPath 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\Policy' -Name 'VerifiedAndReputablePolicyState' -ErrorAction Stop).VerifiedAndReputablePolicyState
    } catch { return }
    if ($state -eq 1) {
        Write-Step 'Smart App Control'
        Write-Problem 'Smart App Control is ON on this PC. It blocks dh2coop.asi (error 0xc0e90002): the game starts, but'
        Write-Problem 'without co-op. The mod only loads with it off: Windows Security, App & browser control, Smart App'
        Write-Problem 'Control settings, Off. On most Windows 11 versions it cannot be turned back on without resetting'
        Write-Problem 'Windows, so decide for yourself. This installer does not change it.'
    } elseif ($state -eq 2) {
        Write-Step 'Smart App Control'
        Write-Info 'Smart App Control is in evaluation mode. If Windows later turns it on, it will block dh2coop.asi'
        Write-Info '(error 0xc0e90002). Then see README.txt, "If the mod does not load".'
    }
}

# ------------------------------------------------------------------------------------------------------------------
# Main
# ------------------------------------------------------------------------------------------------------------------
if ($FindOnly) {
    $bins = @(Find-GameBins)
    if ($bins.Count -eq 0) { 'No game folder found.' } else { $bins }
    return
}

# -Uninstall is uninstall.ps1 (also copied into Bin): one place that knows every file any version left behind.
# -DryRun keeps its installer meaning here: files go, the firewall step is only printed.
if ($Uninstall) {
    $u = @{ KitDir = $kit }
    if ($GameDir) { $u.GameDir = $GameDir }
    if ($ShortcutDir) { $u.ShortcutDir = $ShortcutDir }
    if ($DataDir) { $u.DataDir = $DataDir }
    if ($DryRun) { $u.FilesOnly = $true }
    if ($Yes) { $u.Yes = $true }
    & (Join-Path $kit 'uninstall.ps1') @u
    exit $LASTEXITCODE
}

$exitCode = 0
$noFirewall = $false
try {
    foreach ($f in $modFiles) {
        if (-not (Test-Path -LiteralPath (Join-Path $kit $f))) { throw "The kit is missing $f (unzip the whole kit first)." }
    }
    $bin = Select-GameBin
    $desk = if ($ShortcutDir) { $ShortcutDir } else { [Environment]::GetFolderPath('Desktop') }
    $iniPath = Join-Path $bin 'dh2coop.ini'
    Write-Host "Game folder: $bin"
    if ($DryRun) { Write-Host '(dry run: the firewall step is printed, not run)' -ForegroundColor DarkYellow }

    # The question comes first, in the window the person is already looking at.
    $oldPassword = Get-IniValue $iniPath 'Password'
    if (-not $PlayerName) { $PlayerName = Get-IniValue $iniPath 'PlayerName' }
    if (-not $PlayerName -or $PlayerName -eq 'Hunter') {
        $typed = ''
        if (-not $Yes) { $typed = Read-Host "Your player name (Enter = $env:USERNAME)" }
        $PlayerName = if ($typed) { $typed } else { $env:USERNAME }
    }
    $PlayerName = ($PlayerName -replace '[^A-Za-z0-9 _\-]', '').Trim()
    if (-not $PlayerName) { $PlayerName = 'Hunter' }
    if ($PlayerName.Length -gt 31) { $PlayerName = $PlayerName.Substring(0, 31) }

    # One administrator prompt: the firewall rule needs it, and so does a game under Program Files.
    if (-not $Elevated -and -not $DryRun -and -not (Test-IsAdmin)) {
        Write-Host 'Windows will ask for permission once (the firewall rule for hosting needs administrator rights).'
        $argList = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"", '-GameDir', "`"$bin`"",
                     '-ShortcutDir', "`"$desk`"", '-PlayerName', "`"$PlayerName`"", '-Elevated')
        if ($Yes) { $argList += '-Yes' }
        $declined = $false
        try {
            $p = Start-Process -FilePath 'powershell.exe' -ArgumentList ($argList -join ' ') -Verb RunAs -Wait -PassThru
            exit $p.ExitCode
        } catch { $declined = $true }
        if ($declined) {
            if (-not (Test-Writable $bin)) {
                throw 'Administrator permission was not given, and the game folder needs it. Run install.bat again and press Yes.'
            }
            Write-Problem 'Administrator permission was not given: installing the mod without the firewall rule.'
            $noFirewall = $true
        }
    }

    # ---- the mod files and the shortcut ----
    Write-Step 'Mod files'
    # A loader that is not ours (another mod's dinput8.dll) is kept aside once. An existing dh2coop.ini is ours:
    # only its player name is carried over.
    $loader = Join-Path $bin 'dinput8.dll'
    $loaderBak = "$loader.dh2coop-backup"
    if ((Test-Path -LiteralPath $loader) -and -not (Test-Path -LiteralPath $loaderBak) -and
        -not (Same-File $loader (Join-Path $kit 'dinput8.dll'))) {
        Copy-Item -LiteralPath $loader -Destination $loaderBak
    }
    foreach ($f in $modFiles) { Copy-Item -LiteralPath (Join-Path $kit $f) -Destination (Join-Path $bin $f) -Force }
    Set-IniValue $iniPath 'PlayerName' $PlayerName
    if ($oldPassword) { Set-IniValue $iniPath 'Password' $oldPassword }  # a host's password survives an update
    $exe = Join-Path $bin 'Main7.exe'
    New-Item -ItemType Directory -Force $desk | Out-Null
    foreach ($name in $oldShortcuts) { Remove-Item -LiteralPath (Join-Path $desk "$name.lnk") -Force -ErrorAction SilentlyContinue }
    New-Shortcut (Join-Path $desk 'DH2 COOP.lnk') $exe $bin $exe 'Dangerous Hunts 2 co-op: host or join from the lobby (F8) at the main menu' $launchArgs
    Write-Ok "Installed for player '$PlayerName' in $bin"
    Write-Ok "Desktop shortcut: DH2 COOP ($desk)"

    # ---- Windows Firewall ----
    Write-Step 'Windows Firewall'
    if ($noFirewall) {
        Write-Problem "Skipped. The first time you HOST, Windows asks to allow Main7.exe: press Allow."
    } else {
        try { Set-GameFirewall $exe } catch { Write-Problem "Could not set the firewall rule: $($_.Exception.Message)"; $exitCode = 2 }
    }

    Show-SmartAppControl

    Write-Step 'Done'
    Write-Info 'Start DH2 COOP. The lobby opens at the main menu (F8 shows / hides it).'
    Write-Info 'Host: press HOST, then send friends the join code (or your tailnet / LAN address) the lobby shows.'
    Write-Info 'Join: paste the host''s join code, or type their IP (IP:port), and press JOIN.'
    if ($DryRun) { Write-Host '(dry run: nothing was changed in the firewall)' -ForegroundColor DarkYellow }
} catch {
    Write-Host ''
    Write-Host ('ERROR: ' + $_.Exception.Message) -ForegroundColor Red
    $exitCode = 1
}
if ($Elevated -and -not $Yes) {
    Write-Host ''
    [void](Read-Host 'Press Enter to close this window')
}
exit $exitCode
