# Checks tools-src\kit\install.ps1 and uninstall.ps1 without touching the firewall, the clipboard or the browser:
#   - install -DryRun: files, shortcut, the firewall step printed (the old Tailscale rule removed, the game rule added)
#   - install.ps1 -Uninstall, and uninstall.ps1 / uninstall.bat on their own (dry run, from Bin, loader cases)
# It copies the kit to a temp folder with a fake game folder (an empty Main7.exe) and never writes outside it.
# Usage: .\tools-src\kit-test\installer-test.ps1
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$kitSrc = Join-Path (Split-Path $here -Parent) 'kit'
$work = Join-Path $env:TEMP ('dh2coop-installer-test-' + [guid]::NewGuid().ToString('N'))
$kit = Join-Path $work 'kit'
$bin = Join-Path $work 'game\Bin'
$desk = Join-Path $work 'desktop'
$data = Join-Path $work 'localappdata\dh2coop'
New-Item -ItemType Directory -Force $kit, $bin, $desk | Out-Null
Copy-Item (Join-Path $kitSrc '*') $kit -Recurse
foreach ($f in 'dinput8.dll', 'dh2coop.asi') { [IO.File]::WriteAllText((Join-Path $kit $f), 'x') }
Copy-Item (Join-Path (Split-Path (Split-Path $here -Parent) -Parent) 'mod\dh2coop.ini') $kit
[IO.File]::WriteAllText((Join-Path $bin 'Main7.exe'), 'x')

$fail = 0
function Run-Install([string[]]$extra) {
    $a = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $kit 'install.ps1'), '-GameDir', $bin,
           '-ShortcutDir', $desk, '-PlayerName', 'Andri Test', '-DataDir', $data, '-DryRun', '-Yes') + $extra
    $out = & powershell @a 2>&1 | Out-String
    return [pscustomobject]@{ Out = $out; Code = $LASTEXITCODE }
}
function Check([string]$name, $r, [string[]]$must, [int]$exit = 0, [string[]]$mustNot = @()) {
    $ok = $r.Code -eq $exit
    $why = @()
    if (-not $ok) { $why += "exit $($r.Code), wanted $exit" }
    foreach ($p in $must) { if ($r.Out -notmatch $p) { $ok = $false; $why += "missing: $p" } }
    foreach ($p in $mustNot) { if ($r.Out -match $p) { $ok = $false; $why += "should not print: $p" } }
    if ($ok) { Write-Host "pass  $name" } else { Write-Host "FAIL  $name  ($($why -join '; '))"; $script:fail++ }
}

# install
Check 'install -DryRun' (Run-Install @()) @(
    "Installed for player 'Andri Test'", 'Desktop shortcut: DH2 COOP',
    "would run: Get-NetFirewallRule -DisplayName 'DH2 coop \(Tailscale\)' \| Remove-NetFirewallRule",
    "would run: New-NetFirewallRule -DisplayName 'DH2 coop \(game\)' -Direction Inbound -Program '.*Main7\.exe' -Protocol UDP -LocalPort 27420 -Action Allow -Profile Any",
    'paste the host''s join code') 0 @('winget', 'tailscale up', 'ERROR')
$names = (Get-ChildItem $bin | ForEach-Object Name | Sort-Object) -join ','
if ($names -eq 'dh2coop.asi,dh2coop.ini,dinput8.dll,host.bat,join.bat,Main7.exe,uninstall.bat,uninstall.ps1' -and
    (Test-Path (Join-Path $desk 'DH2 COOP.lnk')) -and (Get-Content (Join-Path $bin 'dh2coop.ini') -Raw) -match '(?m)^PlayerName=Andri Test') {
    Write-Host 'pass  install copied the files, the uninstaller, the shortcut and the player name'
} else { Write-Host "FAIL  install left: $names"; $fail++ }
$iniFile = Join-Path $bin 'dh2coop.ini'
[IO.File]::WriteAllText($iniFile, ([IO.File]::ReadAllText($iniFile) -replace '(?m)^Password=[^\r\n]*', 'Password=pw 123'))
[void](Run-Install @())
$iniText = [IO.File]::ReadAllText($iniFile)
if ($iniText -match '(?m)^Password=pw 123\r$' -and $iniText -match '(?m)^PlayerName=Andri Test\r$' -and $iniText -notmatch '[^\r]\n') {
    Write-Host 'pass  an update keeps the player name and the password, CRLF intact'
} else { Write-Host 'FAIL  update: password / name / line ends'; $fail++ }
Check 'install.ps1 -FindOnly runs' ([pscustomobject]@{ Out = (& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $kit 'install.ps1') -FindOnly 2>&1 | Out-String); Code = $LASTEXITCODE }) @()

# install.ps1 -Uninstall
Check '-Uninstall' (Run-Install (@('-Uninstall'))) @('== Windows Firewall', 'Tailscale stays installed')
if ((Get-ChildItem $bin | ForEach-Object Name) -join ',' -eq 'Main7.exe') { Write-Host 'pass  -Uninstall removed the mod files' } else { Write-Host "FAIL  files left: $((Get-ChildItem $bin | ForEach-Object Name) -join ',')"; $fail++ }

# uninstall.ps1 on its own, run from the copy the installer leaves in Bin (an old install without the kit at hand)
$vstore = Join-Path $work 'virtualstore'
$vsBin = Join-Path $vstore ($bin -replace '^[A-Za-z]:\\', '')
$ual = Join-Path (Split-Path (Split-Path $here -Parent) -Parent) 'game\Bin\dinput8.dll'  # a real Ultimate ASI Loader
function Run-Uninstall([string[]]$extra) {
    $a = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $bin 'uninstall.ps1'), '-ShortcutDir', $desk,
           '-DataDir', $data, '-VirtualStore', $vstore, '-Yes') + $extra
    $out = & powershell @a 2>&1 | Out-String
    return [pscustomobject]@{ Out = $out; Code = $LASTEXITCODE }
}
function Fresh-Install {
    [void](Run-Install @())
    if (Test-Path $ual) { Copy-Item $ual (Join-Path $bin 'dinput8.dll') -Force }
}
function New-Lnk([string]$name, [string]$target) {
    $s = (New-Object -ComObject WScript.Shell).CreateShortcut((Join-Path $desk "$name.lnk")); $s.TargetPath = $target; $s.Save()
}
function Names([string]$dir) { return ((Get-ChildItem $dir -ErrorAction SilentlyContinue | ForEach-Object Name | Sort-Object) -join ',') }

Fresh-Install
foreach ($f in 'dh2coop.1.log', 'dh2coop.2.log', 'dh2coop-crash.dmp', 'debug.1.log', 'dh2coop-lastmap.txt', 'dh2coop-joincode.txt') {
    [IO.File]::WriteAllText((Join-Path $bin $f), 'x')
}
New-Item -ItemType Directory -Force $data, $vsBin | Out-Null
[IO.File]::WriteAllText((Join-Path $data 'lasthost.txt'), '100.64.0.1')
[IO.File]::WriteAllText((Join-Path $vsBin 'dh2coop.log'), 'x')
New-Lnk 'DH2 HOST' (Join-Path $bin 'host.bat')
New-Lnk 'DH2 JOIN' (Join-Path $env:WINDIR 'notepad.exe')  # same name, not ours: stays
$before = Names $bin
Check 'uninstall.ps1 -DryRun lists, changes nothing' (Run-Uninstall @('-DryRun')) @('would remove dh2coop\.asi', 'would remove dh2coop-crash\.dmp',
    'would remove the folder', 'would remove DH2 COOP\.lnk', 'kept DH2 JOIN\.lnk', 'nothing is changed')
if ((Names $bin) -eq $before -and (Test-Path $data)) { Write-Host 'pass  -DryRun left every file' } else { Write-Host 'FAIL  -DryRun removed something'; $fail++ }
$r = Run-Uninstall @('-FilesOnly')
Check 'uninstall.ps1 from Bin' $r @('removed dh2coop\.asi', 'removed uninstall\.ps1', 'removed debug\.1\.log', 'VirtualStore', 'The co-op mod is removed')
$want = if (Test-Path $ual) { 'Main7.exe' } else { 'dinput8.dll,Main7.exe' }  # without a real loader the fake one is not recognised
$deskLeft = Names $desk
if ((Names $bin) -eq $want -and -not (Test-Path $data) -and -not (Test-Path (Join-Path $vsBin 'dh2coop.log')) -and $deskLeft -eq 'DH2 JOIN.lnk') {
    Write-Host 'pass  uninstall.ps1 left only the game (data, VirtualStore, our shortcuts gone; the foreign one kept)'
} else { Write-Host "FAIL  bin: $(Names $bin); desk: $deskLeft; data: $(Test-Path $data)"; $fail++ }
Remove-Item (Join-Path $desk 'DH2 JOIN.lnk') -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $bin 'dinput8.dll') -Force -ErrorAction SilentlyContinue

# another mod's loader kept aside by the installer comes back
[IO.File]::WriteAllText((Join-Path $bin 'dinput8.dll'), 'other mod loader')
Fresh-Install
$r = Run-Uninstall @('-FilesOnly')
$ok = (Test-Path (Join-Path $bin 'dinput8.dll')) -and ([IO.File]::ReadAllText((Join-Path $bin 'dinput8.dll')) -eq 'other mod loader') -and
      -not (Test-Path (Join-Path $bin 'dinput8.dll.dh2coop-backup'))
if ($ok -and $r.Out -match 'earlier loader') { Write-Host 'pass  uninstall put the earlier dinput8.dll back' } else { Write-Host "FAIL  backup restore: $(Names $bin)"; $fail++ }
Remove-Item (Join-Path $bin 'dinput8.dll') -Force

# the loader stays when another ASI mod needs it
if (Test-Path $ual) {
    Fresh-Install
    [IO.File]::WriteAllText((Join-Path $bin 'other.asi'), 'x')
    $r = Run-Uninstall @('-FilesOnly')
    if ((Names $bin) -eq 'dinput8.dll,Main7.exe,other.asi' -and $r.Out -match 'other ASI mods use it') { Write-Host 'pass  loader kept for other.asi' } else { Write-Host "FAIL  other.asi: $(Names $bin)"; $fail++ }
    Remove-Item (Join-Path $bin 'other.asi'), (Join-Path $bin 'dinput8.dll') -Force
}

# uninstall.bat from Bin deletes itself without cmd complaining about the missing batch file
Fresh-Install
$out = cmd /c "echo.| `"$(Join-Path $bin 'uninstall.bat')`" -Yes -FilesOnly -ShortcutDir `"$desk`" -DataDir `"$data`" -VirtualStore `"$vstore`"" 2>&1 | Out-String
$want = if (Test-Path $ual) { 'Main7.exe' } else { 'dinput8.dll,Main7.exe' }
if ((Names $bin) -eq $want -and $out -notmatch 'cannot be found') { Write-Host 'pass  uninstall.bat removed everything, itself included' } else { Write-Host "FAIL  uninstall.bat: $(Names $bin) $out"; $fail++ }

Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
if ($fail -gt 0) { Write-Host "$fail failed"; exit 1 }
Write-Host 'installer-test passed'
