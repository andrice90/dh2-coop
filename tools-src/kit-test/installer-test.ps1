# Checks tools-src\kit\install.ps1 without touching Tailscale, winget, the firewall, the clipboard or the browser:
#   - -SelfTest: the join code functions against the vectors mod\net-test checks too
#   - -Simulate <sim\*.json> -DryRun: friend and host paths for a fresh PC (with and without winget), a PC already on
#     the host's tailnet, on another tailnet, signed out, stopped; an IP-only code; -Uninstall; a damaged code
# It copies the kit to a temp folder with a fake game folder (an empty Main7.exe) and never writes outside it.
# Usage: .\tools-src\kit-test\installer-test.ps1
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$kitSrc = Join-Path (Split-Path $here -Parent) 'kit'
$work = Join-Path $env:TEMP ('dh2coop-installer-test-' + [guid]::NewGuid().ToString('N'))
$kit = Join-Path $work 'kit'
$bin = Join-Path $work 'game\Bin'
$desk = Join-Path $work 'desktop'
New-Item -ItemType Directory -Force $kit, $bin, $desk | Out-Null
Copy-Item (Join-Path $kitSrc '*') $kit -Recurse
foreach ($f in 'dinput8.dll', 'dh2coop.asi') { [IO.File]::WriteAllText((Join-Path $kit $f), 'x') }
Copy-Item (Join-Path (Split-Path (Split-Path $here -Parent) -Parent) 'mod\dh2coop.ini') $kit
[IO.File]::WriteAllText((Join-Path $bin 'Main7.exe'), 'x')

$fakeKey = 'tskey-auth-kTEST1234-ABCDEFGH'
$codeKey = 'DH2-djF8MTAwLjEwMS4xMDIuMTAzfDI3NDIwfHRza2V5LWF1dGgta1RFU1QxMjM0LUFCQ0RFRkdIfEFuZHJp'  # 100.101.102.103:27420, fake key
$codeIp = 'DH2-djF8MTAwLjY0LjAuMXwyNzQyMHx8SG9zdA'  # 100.64.0.1:27420, no key
$codeBad = 'DH2-djF8MTAwLjY0LjAuMXwyNzQyMHx8SG9zdB'  # trailing bits not zero

$fail = 0
function Run-Install([string[]]$extra) {
    $a = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $kit 'install.ps1'), '-GameDir', $bin,
           '-ShortcutDir', $desk, '-PlayerName', 'Andri Test', '-LobbyIni', (Join-Path $bin 'dh2coop-lobby.ini'), '-DryRun', '-Yes') + $extra
    $out = & powershell @a 2>&1 | Out-String
    return [pscustomobject]@{ Out = $out; Code = $LASTEXITCODE }
}
function Check([string]$name, $r, [string[]]$must, [int]$exit = 0, [string[]]$mustNot = @()) {
    $ok = $r.Code -eq $exit
    $why = @()
    if (-not $ok) { $why += "exit $($r.Code), wanted $exit" }
    foreach ($p in $must) { if ($r.Out -notmatch $p) { $ok = $false; $why += "missing: $p" } }
    foreach ($p in @($fakeKey, $codeKey, 'ABCDEFGH')) {
        if ($r.Out.Contains($p)) { $ok = $false; $why += "printed a secret: $($p.Substring(0, 8))..." }
    }
    foreach ($p in $mustNot) { if ($r.Out -match $p) { $ok = $false; $why += "should not print: $p" } }
    if ($ok) { Write-Host "pass  $name" } else { Write-Host "FAIL  $name  ($($why -join '; '))"; $script:fail++ }
}
function Sim([string]$n) { return @('-Simulate', (Join-Path $here "sim\$n.json")) }

# join code twin
$st = & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $kit 'install.ps1') -SelfTest 2>&1 | Out-String
if ($LASTEXITCODE -eq 0 -and $st -match '0 failed') { Write-Host "pass  -SelfTest ($($st.Trim()))" } else { Write-Host "FAIL  -SelfTest $st"; $fail++ }

# friend, with a key
Check 'friend, fresh PC, winget' (Run-Install ((Sim 'fresh-winget') + @('-JoinCode', $codeKey))) @(
    'would run: winget install --id Tailscale\.Tailscale -e --silent --accept-package-agreements --accept-source-agreements',
    'would run: tailscale up --auth-key=tskey-auth-kTE\.\.\. --unattended --hostname=dh2-andri-test', 'The host answers',
    "New-NetFirewallRule -DisplayName 'DH2 coop \(Tailscale\)' -Direction Inbound -Program '.*Main7\.exe' -Protocol UDP -RemoteAddress 100\.64\.0\.0/10 -Action Allow -Profile Any")
Check 'friend, fresh PC, no winget' (Run-Install ((Sim 'fresh-nowinget') + @('-JoinCode', $codeKey))) @(
    'winget .* is not available', 'would run: open https://tailscale\.com/download/windows', 'would run: tailscale up --auth-key=')
Check 'friend, already on the host tailnet' (Run-Install ((Sim 'running-ok') + @('-JoinCode', $codeKey))) @(
    'already on the host.s tailnet', 'nothing to do') 0 @('tailscale up', 'tailscale login')
Check 'friend, other tailnet: second login' (Run-Install ((Sim 'running-other-tailnet') + @('-JoinCode', $codeKey))) @(
    'someone-else\.example', 'Add a second Tailscale login', 'would run: tailscale login --auth-key=tskey-auth-kTE\.\.\. --unattended --hostname=dh2-andri-test',
    "tailscale switch")
Check 'friend, signed out: up with the key' (Run-Install ((Sim 'needslogin') + @('-JoinCode', $codeKey))) @(
    'would run: tailscale up --auth-key=tskey-auth-kTE\.\.\. --unattended') 0 @('tailscale login')
Check 'friend, stopped tailscale' (Run-Install ((Sim 'stopped') + @('-JoinCode', $codeKey))) @('state: Stopped', 'tailscale login --auth-key=')
Check 'friend, IP-only code, signed out' (Run-Install ((Sim 'needslogin') + @('-JoinCode', $codeIp))) @(
    'no friend key', 'would run: tailscale up --unattended --hostname=dh2-andri-test') 0 @('--auth-key')
Check 'friend, IP-only code, running' (Run-Install ((Sim 'running-ok') + @('-JoinCode', $codeIp))) @('The host answers \(100\.64\.0\.1\)')
# the lobby prefill
$ini = Get-Content (Join-Path $bin 'dh2coop-lobby.ini') -Raw
if ($ini -match 'JoinHost=100\.64\.0\.1' -and $ini -match 'JoinPort=27420') { Write-Host 'pass  lobby ini JoinHost / JoinPort written' } else { Write-Host "FAIL  lobby ini: $ini"; $fail++ }

# host
Check 'host, signed out' (Run-Install ((Sim 'needslogin') + @('-IAmHost'))) @(
    'would run: tailscale up --unattended', 'tailnet IP\): 100\.100\.0\.9', '"tag:dh2friend": \["autogroup:admin"\]',
    '"dst": \["100\.100\.0\.9:27420"\]', 'Reusable ON, Ephemeral ON, Tags: tag:dh2friend', 'would run: open https://login\.tailscale\.com/admin/acls/file',
    'would run: open https://login\.tailscale\.com/admin/settings/keys', 'REPLACES the default')
Check 'host, already running' (Run-Install ((Sim 'running-ok') + @('-IAmHost'))) @('tailnet IP\): 100\.90\.80\.70') 0 @('tailscale up')
Check 'host, stopped' (Run-Install ((Sim 'stopped') + @('-IAmHost'))) @('signed in but not connected', 'would run: tailscale up --unattended')

# bad input and uninstall
Check 'damaged join code' (Run-Install ((Sim 'running-ok') + @('-JoinCode', $codeBad))) @('ERROR: Join code: That join code is damaged') 1
Check '-Uninstall' (Run-Install (@('-Uninstall'))) @('== Windows Firewall', 'Tailscale stays installed')
if ((Get-ChildItem $bin | ForEach-Object Name) -join ',' -eq 'Main7.exe') { Write-Host 'pass  -Uninstall removed the mod files and the lobby ini' } else { Write-Host "FAIL  files left: $((Get-ChildItem $bin | ForEach-Object Name) -join ',')"; $fail++ }

# uninstall.ps1 on its own, run from the copy the installer leaves in Bin (an old install without the kit at hand)
$data = Join-Path $work 'localappdata\dh2coop'
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
    [void](Run-Install ((Sim 'running-ok') + @('-JoinCode', $codeIp)))
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
