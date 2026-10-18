# DH2 co-op kit installer.
# Finds the game's Bin folder (...\Activision Value\Magic Wand\DNGH2\Bin, the folder with Main7.exe) on the local
# fixed drives, copies the mod files into it, puts one shortcut on the desktop (DH2 COOP: the game at its main menu;
# host or join from the co-op lobby window there), installs Tailscale if needed and gets this PC onto the host's
# tailnet with the host's JOIN CODE, and opens the Windows Firewall for Tailscale traffic only.
# Double-click install.bat. Or from PowerShell in this folder:
#   powershell -ExecutionPolicy Bypass -File install.ps1 [-GameDir <...\DNGH2\Bin>] [-PlayerName <name>] [-JoinCode <code>]
#   powershell -ExecutionPolicy Bypass -File install.ps1 -IAmHost     (host: no join code question)
#   powershell -ExecutionPolicy Bypass -File install.ps1 -Uninstall   (runs uninstall.ps1: every version's files, the
#                                                                      shortcuts, the firewall rule)
#   powershell -ExecutionPolicy Bypass -File install.ps1 -FindOnly    (only lists the game folders it finds)
#   -DryRun   prints every Tailscale, winget and firewall action instead of running it (the files are still copied;
#             no administrator prompt). Reading Tailscale's status stays real. Use it to see what an install would do.
#   -Yes      answers every Y/N question with yes and skips the "press Enter" waits (tests).
#   -Simulate <file.json>   test only, implies -DryRun: Tailscale / winget answers come from the file, not this PC.
#   -SelfTest               test only: checks the join code functions against known vectors and exits.
# One administrator prompt (UAC): Tailscale and the firewall rule need it.
# Another mod's dinput8.dll is kept as dinput8.dll.dh2coop-backup and put back by -Uninstall. -Uninstall leaves
# Tailscale installed (other programs may use it).
param(
    [string]$GameDir = '',
    [string]$PlayerName = '',
    [string]$ShortcutDir = '',
    [string]$JoinCode = '',
    [switch]$IAmHost,
    [switch]$Uninstall,
    [switch]$FindOnly,
    [switch]$DryRun,
    [switch]$Yes,
    [string]$Simulate = '',
    [switch]$SelfTest,
    # internal: set when this script restarts itself elevated
    [string]$JoinCodeFile = '',
    [string]$LobbyIni = '',
    [switch]$Elevated
)
$ErrorActionPreference = 'Stop'
$kit = $PSScriptRoot
$modFiles = @('dinput8.dll', 'dh2coop.asi', 'dh2coop.ini', 'host.bat', 'join.bat', 'uninstall.bat', 'uninstall.ps1')
$tail = 'Activision Value\Magic Wand\DNGH2\Bin'
$shortcuts = @('DH2 COOP')
$oldShortcuts = @('DH2 HOST', 'DH2 JOIN', 'DH2 HOST DLSS')  # earlier kits; removed on install and uninstall
$launchArgs = '+mod data -nointro -developper -cheats'
$fwRuleName = 'DH2 coop (Tailscale)'
$gamePort = 27420
$progFiles = if ($env:ProgramW6432) { $env:ProgramW6432 } else { $env:ProgramFiles }
$tsExe = Join-Path $progFiles 'Tailscale\tailscale.exe'
$urlAcl = 'https://login.tailscale.com/admin/acls/file'
$urlKeys = 'https://login.tailscale.com/admin/settings/keys'
$urlTsDownload = 'https://tailscale.com/download/windows'
if ($Simulate) { $DryRun = $true }

# ------------------------------------------------------------------------------------------------------------------
# Join code: PowerShell twin of mod\src\coop\joincode.cpp. "DH2-" + base64url (no padding) of the ASCII text
#   v1|<tailnet ipv4>|<port>|<tailscale auth key or empty>|<host player name>
# Keep both in step; -SelfTest and mod\net-test check the same vectors. The key is a secret: never print it whole.
# ------------------------------------------------------------------------------------------------------------------
$script:B64 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_'
$script:JoinCodeError = ''
$kMaxCode = 300
$kMaxName = 32

function Test-TailnetIp([string]$ip) {
    if ($null -eq $ip) { return $false }
    $m = [regex]::Match($ip, '\A(0|[1-9][0-9]{0,2})\.(0|[1-9][0-9]{0,2})\.(0|[1-9][0-9]{0,2})\.(0|[1-9][0-9]{0,2})\z')
    if (-not $m.Success) { return $false }
    $o = @(1..4 | ForEach-Object { [int]$m.Groups[$_].Value })
    foreach ($x in $o) { if ($x -gt 255) { return $false } }
    return ($o[0] -eq 100 -and $o[1] -ge 64 -and $o[1] -le 127)
}

function Test-AuthKey([string]$key) {
    return ($null -ne $key) -and ($key -cmatch '\Atskey-auth-[A-Za-z0-9-]+\z')
}

function Get-CleanName([string]$name) {
    # Printable ASCII only, '|' and anything else becomes '_', at most 32 characters, trimmed.
    $sb = New-Object System.Text.StringBuilder
    foreach ($c in $name.ToCharArray()) {
        if ($sb.Length -ge $kMaxName) { break }
        $n = [int]$c
        if ($n -ge 0x20 -and $n -le 0x7E -and $c -ne '|') { [void]$sb.Append($c) } else { [void]$sb.Append('_') }
    }
    return $sb.ToString().Trim(' ')
}

function ConvertTo-Base64Url([byte[]]$bytes) {
    $sb = New-Object System.Text.StringBuilder
    $i = 0
    while ($i + 3 -le $bytes.Length) {
        $v = ([int]$bytes[$i] -shl 16) -bor ([int]$bytes[$i + 1] -shl 8) -bor [int]$bytes[$i + 2]
        [void]$sb.Append($script:B64[($v -shr 18) -band 63]).Append($script:B64[($v -shr 12) -band 63])
        [void]$sb.Append($script:B64[($v -shr 6) -band 63]).Append($script:B64[$v -band 63])
        $i += 3
    }
    $rest = $bytes.Length - $i
    if ($rest -eq 1) {
        $v = [int]$bytes[$i] -shl 16
        [void]$sb.Append($script:B64[($v -shr 18) -band 63]).Append($script:B64[($v -shr 12) -band 63])
    } elseif ($rest -eq 2) {
        $v = ([int]$bytes[$i] -shl 16) -bor ([int]$bytes[$i + 1] -shl 8)
        [void]$sb.Append($script:B64[($v -shr 18) -band 63]).Append($script:B64[($v -shr 12) -band 63])
        [void]$sb.Append($script:B64[($v -shr 6) -band 63])
    }
    return $sb.ToString()
}

# $null when the text is not canonical base64url (bad character, length 1 mod 4, non-zero trailing bits, padding).
function ConvertFrom-Base64Url([string]$text) {
    if ($text.Length % 4 -eq 1) { return $null }
    $bytes = New-Object System.Collections.Generic.List[byte]
    $acc = 0
    $bits = 0
    foreach ($c in $text.ToCharArray()) {
        $v = $script:B64.IndexOf($c)  # ordinal, case-sensitive
        if ($v -lt 0) { return $null }
        $acc = (($acc -shl 6) -bor $v) -band 0xFFFFFF
        $bits += 6
        if ($bits -ge 8) {
            $bits -= 8
            $bytes.Add([byte](($acc -shr $bits) -band 0xFF))
        }
    }
    if ($bits -gt 0 -and (($acc -band ((1 -shl $bits) - 1)) -ne 0)) { return $null }
    return , $bytes.ToArray()
}

# The code, or $null (the reason in $script:JoinCodeError).
function New-JoinCode([string]$ip, [int]$port, [string]$key, [string]$name) {
    if (-not (Test-TailnetIp $ip)) { $script:JoinCodeError = 'not a Tailscale address'; return $null }
    if ($port -lt 1 -or $port -gt 65535) { $script:JoinCodeError = 'bad port'; return $null }
    if ($key -and -not (Test-AuthKey $key)) { $script:JoinCodeError = 'bad key'; return $null }
    $payload = 'v1|' + $ip + '|' + $port + '|' + $key + '|' + (Get-CleanName $name)
    $code = 'DH2-' + (ConvertTo-Base64Url ([System.Text.Encoding]::ASCII.GetBytes($payload)))
    if ($code.Length -gt $kMaxCode) { $script:JoinCodeError = 'too long'; return $null }
    return $code
}

# { Ip; Port; Key; Name }, or $null with the reason (written for the friend) in $script:JoinCodeError.
function ConvertFrom-JoinCode([string]$text) {
    $damaged = 'That join code is damaged (copy it again, all of it).'
    $code = ($text -replace '\s', '')
    if ($code.Length -gt $kMaxCode) { $script:JoinCodeError = 'That join code is too long.'; return $null }
    if ($code.Length -lt 4 -or -not $code.Substring(0, 4).Equals('DH2-', [StringComparison]::OrdinalIgnoreCase)) {
        $script:JoinCodeError = 'A join code starts with DH2-'
        return $null
    }
    $bytes = ConvertFrom-Base64Url $code.Substring(4)
    if ($null -eq $bytes) { $script:JoinCodeError = $damaged; return $null }
    foreach ($b in $bytes) {
        if ($b -lt 0x20 -or $b -gt 0x7E) { $script:JoinCodeError = $damaged; return $null }
    }
    $parts = [System.Text.Encoding]::ASCII.GetString($bytes).Split('|')
    if ($parts.Count -ne 5) { $script:JoinCodeError = $damaged; return $null }
    if ($parts[0] -cne 'v1') { $script:JoinCodeError = 'That join code is from a newer kit. Ask the host for the new kit.'; return $null }
    if (-not (Test-TailnetIp $parts[1])) { $script:JoinCodeError = 'That join code holds an address that is not a Tailscale address.'; return $null }
    if ($parts[2] -notmatch '\A(0|[1-9][0-9]{0,4})\z' -or [int]$parts[2] -lt 1 -or [int]$parts[2] -gt 65535) {
        $script:JoinCodeError = 'That join code holds an invalid port.'
        return $null
    }
    if ($parts[3] -and -not (Test-AuthKey $parts[3])) { $script:JoinCodeError = 'That join code holds an invalid friend key.'; return $null }
    if ($parts[4].Length -gt $kMaxName) { $script:JoinCodeError = 'That join code holds a host name that is too long.'; return $null }
    return [pscustomobject]@{ Ip = $parts[1]; Port = [int]$parts[2]; Key = $parts[3]; Name = $parts[4] }
}

function Get-MaskedKey([string]$key) {
    if (-not $key) { return '(none)' }
    return $key.Substring(0, [Math]::Min(14, $key.Length)) + '...'
}

# Anything that may be printed goes through this: whole keys and whole codes never reach the screen or a log.
function Hide-Secrets([string]$s) {
    $s = [regex]::Replace($s, 'tskey-[A-Za-z0-9-]+', [System.Text.RegularExpressions.MatchEvaluator]{ param($m) Get-MaskedKey $m.Value })
    # Case-sensitive and long: a host name like dh2-andri (lower case) is not a join code.
    return [regex]::Replace($s, 'DH2-[A-Za-z0-9_-]{16,}', 'DH2-...(hidden)')
}

if ($SelfTest) {
    $script:pass = 0
    $script:fail = 0
    function Check([bool]$ok, [string]$what) {
        if ($ok) { $script:pass++ } else { $script:fail++; Write-Host "FAIL $what" }
    }
    # Vectors shared with mod\net-test (made by python's urlsafe_b64encode).
    $vFull = 'DH2-djF8MTAwLjEwMS4xMDIuMTAzfDI3NDIwfHRza2V5LWF1dGgta1RFU1QxMjM0LUFCQ0RFRkdIfEFuZHJp'
    $vNoKey = 'DH2-djF8MTAwLjY0LjAuMXwyNzQyMHx8SG9zdA'
    $vEdge = 'DH2-djF8MTAwLjEyNy4yNTUuMjU0fDF8dHNrZXktYXV0aC1rMS1BYkN8WA'
    Check ((New-JoinCode '100.101.102.103' 27420 'tskey-auth-kTEST1234-ABCDEFGH' 'Andri') -ceq $vFull) 'full vector encodes'
    Check ((New-JoinCode '100.64.0.1' 27420 '' 'Host') -ceq $vNoKey) 'no-key vector encodes'
    Check ((New-JoinCode '100.127.255.254' 1 'tskey-auth-k1-AbC' 'X') -ceq $vEdge) 'edge vector encodes'
    $f = ConvertFrom-JoinCode $vFull
    Check ($f -and $f.Ip -eq '100.101.102.103' -and $f.Port -eq 27420 -and $f.Key -ceq 'tskey-auth-kTEST1234-ABCDEFGH' -and $f.Name -ceq 'Andri') 'full vector decodes'
    $f = ConvertFrom-JoinCode $vNoKey
    Check ($f -and $f.Ip -eq '100.64.0.1' -and $f.Key -eq '' -and $f.Name -ceq 'Host') 'no-key vector decodes'
    $f = ConvertFrom-JoinCode $vEdge
    Check ($f -and $f.Port -eq 1 -and $f.Ip -eq '100.127.255.254') 'edge vector decodes'
    # Round trip, name length 0..32.
    $rt = $true
    foreach ($len in 0..32) {
        $nm = ''
        if ($len -gt 0) { $nm = -join (0..($len - 1) | ForEach-Object { [char](97 + (($_ * 7) % 26)) }) }
        $kk = if ($len % 2) { 'tskey-auth-kAB12-xyz' } else { '' }
        $c = New-JoinCode '100.100.100.100' (1000 + $len) $kk $nm
        $b = if ($c) { ConvertFrom-JoinCode $c } else { $null }
        if (-not $b -or $b.Ip -ne '100.100.100.100' -or $b.Port -ne (1000 + $len) -or $b.Key -cne $kk -or $b.Name -cne $nm) { $rt = $false }
    }
    Check $rt 'round trip, name length 0..32'
    Check ($null -eq (New-JoinCode '100.63.255.255' 27420 '' 'N')) 'encode rejects 100.63.x.x'
    Check ($null -eq (New-JoinCode '100.128.0.0' 27420 '' 'N')) 'encode rejects 100.128.0.0'
    Check ($null -eq (New-JoinCode '192.168.1.2' 27420 '' 'N')) 'encode rejects a LAN address'
    Check ($null -eq (New-JoinCode '100.64.0.1' 0 '' 'N')) 'encode rejects port 0'
    Check ($null -eq (New-JoinCode '100.64.0.1' 27420 'tskey-api-kABC' 'N')) 'encode rejects a non-auth key'
    Check ($null -eq (New-JoinCode '100.64.0.1' 27420 'tskey-auth-' 'N')) 'encode rejects an empty key body'
    $codeOf = { param($p) 'DH2-' + (ConvertTo-Base64Url ([System.Text.Encoding]::ASCII.GetBytes($p))) }
    $bad = [ordered]@{
        'empty text'                 = ''
        'no prefix'                  = 'djF8MTAwLjY0LjAuMXwyNzQyMHx8SG9zdA'
        'wrong prefix'               = 'DH3-djF8MTAwLjY0LjAuMXwyNzQyMHx8SG9zdA'
        'prefix only'                = 'DH2-'
        'standard base64 characters' = 'DH2-djF8MTAwLjY0LjAuMXwyNzQyMHx8SG9zdA+/'
        'base64 padding'             = $vNoKey + '=='
        'base64 length 1 mod 4'      = $vNoKey + 'A'
        'non-zero trailing bits'     = 'DH2-djF8MTAwLjY0LjAuMXwyNzQyMHx8SG9zdB'
        'truncated code'             = $vFull.Substring(0, 40)
        'unknown version'            = (& $codeOf 'v2|100.64.0.1|27420||Host')
        'too few fields'             = (& $codeOf 'v1|100.64.0.1|27420|Host')
        'too many fields'            = (& $codeOf 'v1|100.64.0.1|27420||Host|extra')
        'LAN address'                = (& $codeOf 'v1|192.168.0.5|27420||Host')
        '100.63.255.255'             = (& $codeOf 'v1|100.63.255.255|27420||Host')
        '100.128.0.0'                = (& $codeOf 'v1|100.128.0.0|27420||Host')
        'address 100.64.0.256'       = (& $codeOf 'v1|100.64.0.256|27420||Host')
        'address leading zero'       = (& $codeOf 'v1|100.064.0.1|27420||Host')
        'address 3 parts'            = (& $codeOf 'v1|100.64.1|27420||Host')
        'address 5 parts'            = (& $codeOf 'v1|100.64.0.1.2|27420||Host')
        'port 0'                     = (& $codeOf 'v1|100.64.0.1|0||Host')
        'port 65536'                 = (& $codeOf 'v1|100.64.0.1|65536||Host')
        'port empty'                 = (& $codeOf 'v1|100.64.0.1|||Host')
        'port text'                  = (& $codeOf 'v1|100.64.0.1|27x20||Host')
        'port negative'              = (& $codeOf 'v1|100.64.0.1|-5||Host')
        'port leading zero'          = (& $codeOf 'v1|100.64.0.1|027420||Host')
        'api key'                    = (& $codeOf 'v1|100.64.0.1|27420|tskey-api-kABC|Host')
        'key without body'           = (& $codeOf 'v1|100.64.0.1|27420|tskey-auth-|Host')
        'key with a bad character'   = (& $codeOf 'v1|100.64.0.1|27420|tskey-auth-kA_B|Host')
        'key with a space'           = (& $codeOf 'v1|100.64.0.1|27420|tskey-auth-kA B|Host')
        'name of 33 characters'      = (& $codeOf ('v1|100.64.0.1|27420||' + ('n' * 33)))
        'name with a control char'   = (& $codeOf ('v1|100.64.0.1|27420||Ho' + [char]1 + 'st'))
        'longer than 300 characters' = (& $codeOf ('v1|100.64.0.1|27420|tskey-auth-k' + ('A' * 260) + '|Host'))
    }
    foreach ($name in $bad.Keys) {
        $script:JoinCodeError = ''
        $r = ConvertFrom-JoinCode $bad[$name]
        Check (($null -eq $r) -and [bool]$script:JoinCodeError) "decode rejects $name"
    }
    Check ($null -ne (ConvertFrom-JoinCode (& $codeOf ('v1|100.64.0.1|27420||' + ('n' * 32))))) 'a 32 character name decodes'
    Check ($null -ne (ConvertFrom-JoinCode ('  ' + $vFull + "`r`n"))) 'surrounding whitespace ignored'
    Check ($null -ne (ConvertFrom-JoinCode ($vFull.Substring(0, 30) + "`n" + $vFull.Substring(30)))) 'a line break inside ignored'
    Check ($null -ne (ConvertFrom-JoinCode ('dh2-' + $vFull.Substring(4)))) 'prefix is case-insensitive'
    Check ((Get-MaskedKey 'tskey-auth-kTEST1234-ABCDEFGH') -ceq 'tskey-auth-kTE...') 'Get-MaskedKey'
    $h = Hide-Secrets ('tailscale up --auth-key=tskey-auth-kTEST1234-ABCDEFGH and ' + $vFull)
    Check ($h -ceq 'tailscale up --auth-key=tskey-auth-kTE... and DH2-...(hidden)') 'Hide-Secrets masks keys and codes'
    Write-Host ('install.ps1 join code self test: {0} passed, {1} failed' -f $script:pass, $script:fail)
    if ($script:fail -gt 0) { exit 1 }
    return
}

# ------------------------------------------------------------------------------------------------------------------
# Game folder, files, shortcuts (as before)
# ------------------------------------------------------------------------------------------------------------------
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

# dh2coop-lobby.ini sits where the mod's logs go (core::log_dir): the game's Bin, or %LOCALAPPDATA%\dh2coop when the
# game is under Program Files (a player cannot write there).
function Get-LobbyIniPath([string]$bin) {
    $b = $bin.TrimEnd('\').ToLowerInvariant() + '\'
    foreach ($root in @($env:ProgramFiles, ${env:ProgramFiles(x86)}, $env:ProgramW6432)) {
        if ($root -and $b.StartsWith($root.TrimEnd('\').ToLowerInvariant())) {
            return (Join-Path (Join-Path $env:LOCALAPPDATA 'dh2coop') 'dh2coop-lobby.ini')
        }
    }
    return (Join-Path $bin 'dh2coop-lobby.ini')
}

function Set-LobbyIniValue([string]$ini, [string]$key, [string]$value) {
    if (-not ('Dh2Kit.Ini' -as [type])) {
        Add-Type -Namespace Dh2Kit -Name Ini -MemberDefinition '[System.Runtime.InteropServices.DllImport("kernel32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)] public static extern bool WritePrivateProfileString(string section, string key, string value, string file);'
    }
    New-Item -ItemType Directory -Force (Split-Path -Parent $ini) | Out-Null
    if (-not [Dh2Kit.Ini]::WritePrivateProfileString('Lobby', $key, $value, $ini)) { throw "Could not write $ini" }
}

# ------------------------------------------------------------------------------------------------------------------
# Console helpers
# ------------------------------------------------------------------------------------------------------------------
function Write-Step([string]$t) { Write-Host ''; Write-Host "== $t" -ForegroundColor Cyan }
function Write-Ok([string]$t) { Write-Host "   $t" -ForegroundColor Green }
function Write-Problem([string]$t) { Write-Host "   $t" -ForegroundColor Yellow }
function Write-Info([string]$t) { Write-Host "   $t" }
function Write-Dry([string]$t) { Write-Host "   [dry run] would run: $t" -ForegroundColor DarkYellow }

function Read-YesNo([string]$question, [bool]$default = $true) {
    $suffix = if ($default) { '[Y/n]' } else { '[y/N]' }
    if ($Yes) { Write-Host "   $question $suffix y (-Yes)"; return $true }
    $a = (Read-Host "   $question $suffix").Trim()
    if (-not $a) { return $default }
    return ($a -match '^(y|yes)$')
}

function Wait-Enter([string]$message) {
    if ($Yes) { Write-Host "   $message (skipped, -Yes)"; return }
    [void](Read-Host "   $message")
}

function Open-Url([string]$url) {
    if ($url -notmatch '\Ahttps://(login\.tailscale\.com|tailscale\.com)/') { Write-Problem "Not opening $url"; return }
    if ($DryRun) { Write-Dry "open $url in the browser"; return }
    # explorer.exe hands the address to the normal (not administrator) shell, so the browser is not started elevated.
    Start-Process -FilePath 'explorer.exe' -ArgumentList $url
}

# Runs a program without letting its stderr turn into a PowerShell error; returns { Code; Stdout; Out; TimedOut }.
function Invoke-Native([string]$exe, [string[]]$exeArgs, [int]$timeoutSec = 60) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.Arguments = (@($exeArgs | ForEach-Object { if ($_ -match '[\s"]' -or $_ -eq '') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ } }) -join ' ')
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $p = [System.Diagnostics.Process]::Start($psi)
    $o = $p.StandardOutput.ReadToEndAsync()
    $e = $p.StandardError.ReadToEndAsync()
    if (-not $p.WaitForExit($timeoutSec * 1000)) {
        try { $p.Kill() } catch { }
        return [pscustomobject]@{ Code = -1; Stdout = ''; Out = 'timed out'; TimedOut = $true }
    }
    $p.WaitForExit()
    return [pscustomobject]@{ Code = $p.ExitCode; Stdout = $o.Result; Out = (($o.Result + "`n" + $e.Result).Trim()); TimedOut = $false }
}

# ------------------------------------------------------------------------------------------------------------------
# Tailscale. Reading its status is always real (except -Simulate); everything that changes something is printed
# instead of run under -DryRun.
# ------------------------------------------------------------------------------------------------------------------
$script:sim = $null
if ($Simulate) {
    $script:sim = Get-Content -LiteralPath $Simulate -Raw | ConvertFrom-Json
}
$script:assumeFresh = $false  # -DryRun after a pretend install: the next steps see a fresh Tailscale

function Test-TsInstalled {
    if ($script:sim) { return [bool]$script:sim.installed }
    return (Test-Path -LiteralPath $tsExe)
}

function Get-WingetPath {
    if ($script:sim) { if ($script:sim.winget) { return 'winget' } else { return $null } }
    $c = Get-Command winget -ErrorAction SilentlyContinue
    if ($c) { return $c.Source }
    return $null
}

function Invoke-TsSim([string[]]$a, [string]$shown) {
    $s = $script:sim
    $ok = { param($out) [pscustomobject]@{ Ok = $true; Code = 0; Stdout = $out; Out = $out } }
    switch ($a[0]) {
        'status' { return (& $ok ($s.status | ConvertTo-Json -Depth 6 -Compress)) }
        'version' { return (& $ok '1.0.0-simulated') }
        'ip' { return (& $ok (@($s.status.Self.TailscaleIPs) -join "`n")) }
        'ping' {
            Write-Host "   [simulated] $shown"
            if ($s.pingOk) { return (& $ok 'pong from host (simulated) in 12ms') }
            return [pscustomobject]@{ Ok = $false; Code = 1; Stdout = ''; Out = 'no matching peer for ip' }
        }
        default {
            # up / login: pretend it worked and left the PC running on the host's tailnet.
            Write-Dry $shown
            $s.status = [pscustomobject]@{ BackendState = 'Running'; HaveNodeKey = $true; AuthURL = '';
                Self = [pscustomobject]@{ TailscaleIPs = @('100.100.0.9', 'fd7a:115c:a1e0::9') };
                CurrentTailnet = [pscustomobject]@{ Name = 'simulated.example' } }
            $s.pingOk = $true
            return (& $ok '')
        }
    }
}

function Invoke-Ts([string[]]$tsArgs, [switch]$ReadOnly, [int]$TimeoutSec = 60) {
    $shown = 'tailscale ' + (Hide-Secrets ($tsArgs -join ' '))
    if ($script:sim) { return (Invoke-TsSim $tsArgs $shown) }
    if ($DryRun -and -not $ReadOnly) {
        Write-Dry $shown
        return [pscustomobject]@{ Ok = $true; Code = 0; Stdout = ''; Out = ''; DryRun = $true }
    }
    if (-not (Test-Path -LiteralPath $tsExe)) {
        return [pscustomobject]@{ Ok = $false; Code = -1; Stdout = ''; Out = 'Tailscale is not installed' }
    }
    if (-not $ReadOnly) { Write-Info "> $shown" }
    $r = Invoke-Native $tsExe $tsArgs $TimeoutSec
    return [pscustomobject]@{ Ok = ($r.Code -eq 0); Code = $r.Code; Stdout = $r.Stdout; Out = $r.Out }
}

# { Known; State; LoggedIn; Ip; Tailnet; AuthUrl }. State is Tailscale's BackendState (Running, Stopped, Starting,
# NeedsLogin, NoState, NeedsMachineAuth) or Unreachable when the Tailscale service does not answer.
function Get-TsState {
    if ($script:assumeFresh) {
        return [pscustomobject]@{ Known = $true; State = 'NeedsLogin'; LoggedIn = $false; Ip = ''; Tailnet = ''; AuthUrl = '' }
    }
    $r = Invoke-Ts @('status', '--json') -ReadOnly -TimeoutSec 15
    $o = $null
    if ($r.Ok -and $r.Stdout) { try { $o = $r.Stdout | ConvertFrom-Json } catch { $o = $null } }
    if (-not $o) {
        return [pscustomobject]@{ Known = $false; State = 'Unreachable'; LoggedIn = $false; Ip = ''; Tailnet = ''; AuthUrl = '' }
    }
    $ip = ''
    if ($o.Self -and $o.Self.TailscaleIPs) {
        $v4 = @($o.Self.TailscaleIPs | Where-Object { $_ -match '\A[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+\z' })
        if ($v4.Count -gt 0) { $ip = [string]$v4[0] }
    }
    $tn = ''
    if ($o.CurrentTailnet -and $o.CurrentTailnet.Name) { $tn = [string]$o.CurrentTailnet.Name }
    $state = [string]$o.BackendState
    $loggedIn = ($o.HaveNodeKey -eq $true) -and ($state -ne 'NeedsLogin') -and ($state -ne 'NoState')
    $auth = ''
    if ($o.AuthURL) { $auth = [string]$o.AuthURL }
    return [pscustomobject]@{ Known = $true; State = $state; LoggedIn = $loggedIn; Ip = $ip; Tailnet = $tn; AuthUrl = $auth }
}

# Polls until Tailscale is Running (or the time is up). Opens the sign-in address once if Tailscale offers one.
function Wait-TsRunning([int]$seconds, [bool]$openSignIn = $false) {
    $end = (Get-Date).AddSeconds($seconds)
    $shownUrl = ''
    while ($true) {
        $st = Get-TsState
        if ($st.State -eq 'Running' -and $st.Ip) { return $st }
        if ($openSignIn -and $st.AuthUrl -and $st.AuthUrl -ne $shownUrl) {
            $shownUrl = $st.AuthUrl
            Write-Info 'Sign in to Tailscale in the browser window (the page is opening). If it does not open, go to:'
            Write-Info "  $($st.AuthUrl)"
            Open-Url $st.AuthUrl
        }
        if ((Get-Date) -ge $end) { return $st }
        Start-Sleep -Seconds 2
    }
}

function Install-Tailscale {
    if (Test-TsInstalled) { Write-Ok "Tailscale is installed ($tsExe)"; return $true }
    Write-Info 'Tailscale is not installed on this PC.'
    $winget = Get-WingetPath
    if ($winget) {
        $wa = @('install', '--id', 'Tailscale.Tailscale', '-e', '--silent', '--accept-package-agreements', '--accept-source-agreements')
        if ($DryRun) {
            Write-Dry ('winget ' + ($wa -join ' '))
            if ($script:sim) { $script:sim.installed = $true } else { $script:assumeFresh = $true }
            return $true
        }
        Write-Info '> winget install Tailscale (this can take a minute)'
        & $winget @wa | Out-Host
        for ($i = 0; $i -lt 15 -and -not (Test-Path -LiteralPath $tsExe); $i++) { Start-Sleep -Seconds 2 }
        if (Test-Path -LiteralPath $tsExe) { Write-Ok 'Tailscale installed.'; return $true }
        Write-Problem 'winget did not install Tailscale.'
    } else {
        Write-Problem 'winget (the Windows package installer) is not available on this PC.'
    }
    # No winget, or it failed: the person installs it from the download page.
    for ($attempt = 1; $attempt -le 3; $attempt++) {
        if ($DryRun) {
            Write-Dry "open $urlTsDownload and wait for you to install Tailscale"
            if ($script:sim) { $script:sim.installed = $true } else { $script:assumeFresh = $true }
            return $true
        }
        Write-Info "Opening $urlTsDownload : download and run the Tailscale installer, then come back here."
        Open-Url $urlTsDownload
        Wait-Enter 'Press Enter when Tailscale is installed'
        if (Test-Path -LiteralPath $tsExe) { Write-Ok 'Tailscale found.'; return $true }
        Write-Problem "Still not found at $tsExe"
    }
    return $false
}

function Get-HostnameFor([string]$name) {
    $s = ($name.ToLowerInvariant() -replace '[^a-z0-9]+', '-').Trim('-')
    if (-not $s) { $s = 'hunter' }
    $h = 'dh2-' + $s
    if ($h.Length -gt 63) { $h = $h.Substring(0, 63).TrimEnd('-') }
    return $h
}

function Get-AclSnippet([string]$hostIp, [int]$port) {
    $dst = $(if ($hostIp) { $hostIp } else { '<HOST_TAILNET_IP>' }) + ':' + $port
    return ("{`n  `"tagOwners`": { `"tag:dh2friend`": [`"autogroup:admin`"] },`n  `"acls`": [`n" +
            "    { `"action`": `"accept`", `"src`": [`"autogroup:member`"], `"dst`": [`"*:*`"] },`n" +
            "    { `"action`": `"accept`", `"src`": [`"tag:dh2friend`"], `"dst`": [`"$dst`"] }`n  ]`n}`n")
}

# $true / $false, or $null when nothing was run (-DryRun).
function Show-Reachability([string]$hostIp) {
    $r = Invoke-Ts @('ping', '-c', '1', '--timeout', '5s', $hostIp) -TimeoutSec 15
    if ($r.DryRun) { return $null }
    if ($r.Ok) { Write-Ok "The host answers ($hostIp): $((Hide-Secrets $r.Out) -replace '\s+', ' ')"; return $true }
    Write-Problem "The host does not answer yet ($hostIp). That is normal if the host has not started Tailscale or the game."
    return $false
}

# Friend: get this PC onto the host's tailnet. Returns a short status text for the summary.
function Join-HostTailnet($jc, [string]$playerName) {
    $hostName = Get-HostnameFor $playerName
    $st = Get-TsState
    if (-not $st.Known) {
        Write-Info 'Waiting for the Tailscale service to start ...'
        $st = Wait-TsRunning 30
    }
    if (-not $jc.Key) {
        # An IP-only code: the friend is on the host's tailnet already (invited), or signs in to the account that was.
        if (-not $st.LoggedIn) {
            Write-Info 'This join code has no friend key: you need a Tailscale account that the host has invited.'
            Write-Info 'Sign in to Tailscale with that account (a browser window opens).'
            $r = Invoke-Ts @('up', '--unattended', "--hostname=$hostName") -TimeoutSec 20
            if (-not $r.DryRun -and -not $script:sim) { $st = Wait-TsRunning 300 $true }
        }
    } else {
        if ($st.State -eq 'Running') {
            $reach = Invoke-Ts @('ping', '-c', '1', '--timeout', '5s', $jc.Ip) -TimeoutSec 15
            if ($reach.DryRun) {
                Write-Info '(dry run: the ping was not run; the next step assumes the host does not answer)'
            } elseif ($reach.Ok) {
                Write-Ok "This PC is already on the host's tailnet ($($st.Ip)) and the host answers: nothing to do."
                return 'already on the host''s tailnet'
            }
        }
        if (-not $st.LoggedIn) {
            Write-Info "Joining the host's tailnet as $hostName (no account needed) ..."
            $r = Invoke-Ts @('up', "--auth-key=$($jc.Key)", '--unattended', "--hostname=$hostName") -TimeoutSec 90
        } else {
            $who = if ($st.Tailnet) { "tailnet '$($st.Tailnet)'" } else { 'a Tailscale account' }
            Write-Info "Tailscale on this PC is signed in to $who (state: $($st.State)) and it cannot reach the host ($($jc.Ip))."
            Write-Info 'Either the host is not running Tailscale right now, or this account is on a different tailnet.'
            Write-Info "Adding a second login for the host's tailnet keeps your own account: 'tailscale switch' goes back to it."
            if (-not (Read-YesNo "Add a second Tailscale login for the host's tailnet?" $true)) {
                return 'skipped: not on the host''s tailnet (run install.bat again to add it)'
            }
            $r = Invoke-Ts @('login', "--auth-key=$($jc.Key)", '--unattended', "--hostname=$hostName") -TimeoutSec 90
        }
        if (-not $r.Ok) {
            Write-Problem ('Tailscale said: ' + (Hide-Secrets $r.Out))
            return 'FAILED: tailscale could not join the host''s tailnet'
        }
    }
    if ($script:sim -or $DryRun) {
        $st = Get-TsState
        if ($st.State -ne 'Running') { return 'dry run: would wait up to 60 s for Tailscale to reach Running, then ping the host' }
    } else {
        Write-Info 'Waiting for Tailscale to connect (up to 60 s) ...'
        $st = Wait-TsRunning 60
    }
    if ($st.State -ne 'Running') {
        Write-Problem "Tailscale is not Running yet (state: $($st.State))."
        return "NOT CONNECTED: Tailscale state is $($st.State)"
    }
    Write-Ok "Tailscale is running, this PC is $($st.Ip)."
    $reached = Show-Reachability $jc.Ip
    if ($null -eq $reached) { return 'dry run: would be on the host''s tailnet (nothing was changed)' }
    if ($reached -eq $false) { return 'on the host''s tailnet; the host does not answer yet' }
    return 'on the host''s tailnet'
}

# Host: Tailscale signed in, the tailnet IP. Returns { Status; Ip }.
function Set-UpHostTailscale {
    $st = Get-TsState
    if (-not $st.Known) {
        Write-Info 'Waiting for the Tailscale service to start ...'
        $st = Wait-TsRunning 30
    }
    if (-not $st.LoggedIn) {
        Write-Info 'Sign in to Tailscale (Google, Microsoft, GitHub, ... it is free). A browser window opens: sign in once.'
        $r = Invoke-Ts @('up', '--unattended') -TimeoutSec 20
        if (-not $r.DryRun -and -not $script:sim) {
            $st = Wait-TsRunning 300 $true
            if ($st.State -ne 'Running') {
                return [pscustomobject]@{ Status = "NOT SIGNED IN: Tailscale state is $($st.State)"; Ip = '' }
            }
        } else {
            $st = Get-TsState
        }
    } elseif ($st.State -ne 'Running') {
        Write-Info "Tailscale is signed in but not connected (state: $($st.State)); connecting ..."
        $r = Invoke-Ts @('up', '--unattended') -TimeoutSec 60
        if (-not $r.Ok) { Write-Problem ('Tailscale said: ' + (Hide-Secrets $r.Out) + ' (connect it from the Tailscale tray icon)') }
        if ($script:sim) { $st = Get-TsState } elseif (-not $r.DryRun) { $st = Wait-TsRunning 60 }
    }
    $ip = $st.Ip
    if (-not $ip -and -not $DryRun) {
        $r = Invoke-Ts @('ip', '-4') -ReadOnly -TimeoutSec 15
        if ($r.Ok) { $ip = (($r.Stdout -split "`r?`n")[0]).Trim() }
    }
    if ($ip -and (Test-TailnetIp $ip)) {
        Write-Ok "Your Tailscale address (tailnet IP): $ip"
    } else {
        $ip = ''
        Write-Info '(no tailnet address yet)'
    }
    $text = if ($st.State -eq 'Running') { 'signed in, running' } else { "state $($st.State)" }
    return [pscustomobject]@{ Status = $text; Ip = $ip }
}

function Show-HostFriendSteps([string]$hostIp, [int]$port) {
    $acl = Get-AclSnippet $hostIp $port
    Write-Host ''
    Write-Host '   ONE-TIME SETUP so friends can join with a join code (no Tailscale account needed for them):' -ForegroundColor White
    Write-Info '1. Tailscale admin, Access controls (policy file): paste this over the whole file and Save.'
    Write-Info '   It REPLACES the default "allow everything" policy: your own devices keep full access, friends'' devices'
    Write-Info '   (tag:dh2friend) may only reach this PC on the game port. If you already have your own rules, merge by hand.'
    Write-Host ''
    foreach ($l in ($acl -split "`n")) { if ($l) { Write-Host "      $l" } }
    Write-Host ''
    Write-Info '2. Tailscale admin, Settings, Keys, "Generate auth key": Reusable ON, Ephemeral ON, Tags: tag:dh2friend,'
    Write-Info '   Expiration 90 days. Copy the key (tskey-auth-...).'
    Write-Info '3. In the game: DH2 COOP, paste the key into "Friend key" in the HOST section, Save. Then "Copy join code"'
    Write-Info '   and send the code to your friends. (The lobby button "Tailscale setup help" repeats these steps.)'
    Write-Host ''
    if (Read-YesNo 'Open the two Tailscale admin pages now and copy the policy text to the clipboard?' $true) {
        if ($DryRun) {
            Write-Dry 'copy the policy text to the clipboard (Set-Clipboard)'
        } else {
            try { Set-Clipboard -Value $acl; Write-Ok 'The policy text is on your clipboard.' } catch { Write-Problem 'Could not use the clipboard; copy the text above by hand.' }
        }
        Open-Url $urlAcl
        Open-Url $urlKeys
    }
}

# ------------------------------------------------------------------------------------------------------------------
# Windows Firewall: inbound UDP for Main7.exe from Tailscale addresses (100.64.0.0/10) only.
# ------------------------------------------------------------------------------------------------------------------
function Set-GameFirewall([string]$exe) {
    $cmdRemove = "Get-NetFirewallRule -DisplayName '$fwRuleName' -ErrorAction SilentlyContinue | Remove-NetFirewallRule"
    $cmdAdd = "New-NetFirewallRule -DisplayName '$fwRuleName' -Direction Inbound -Program '$exe' -Protocol UDP -RemoteAddress 100.64.0.0/10 -Action Allow -Profile Any"
    if ($DryRun) {
        Write-Dry $cmdRemove
        Write-Dry $cmdAdd
    } else {
        Get-NetFirewallRule -DisplayName $fwRuleName -ErrorAction SilentlyContinue | Remove-NetFirewallRule
        [void](New-NetFirewallRule -DisplayName $fwRuleName -Direction Inbound -Program $exe -Protocol UDP `
            -RemoteAddress '100.64.0.0/10' -Action Allow -Profile Any `
            -Description 'DH2 co-op: lets friends on your tailnet reach the game (Tailscale addresses only).')
        Write-Ok "Firewall rule '$fwRuleName': Main7.exe may receive UDP from Tailscale addresses (100.64.0.0/10)."
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

function Test-IsAdmin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    return ([Security.Principal.WindowsPrincipal]$id).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
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
    if ($LobbyIni) { $u.DataDir = Split-Path -Parent $LobbyIni }
    if ($DryRun) { $u.FilesOnly = $true }
    if ($Yes) { $u.Yes = $true }
    & (Join-Path $kit 'uninstall.ps1') @u
    exit $LASTEXITCODE
}

$exitCode = 0
$fwFailed = $false
try {
    $bin = Select-GameBin
    $desk = if ($ShortcutDir) { $ShortcutDir } else { [Environment]::GetFolderPath('Desktop') }
    $iniPath = Join-Path $bin 'dh2coop.ini'
    if (-not $LobbyIni) { $LobbyIni = Get-LobbyIniPath $bin }
    if ($JoinCodeFile) {
        # From the parent window: the code came through a temporary file, not the command line.
        $JoinCode = ([IO.File]::ReadAllText($JoinCodeFile)).Trim()
        Remove-Item -LiteralPath $JoinCodeFile -Force -ErrorAction SilentlyContinue
    }
    Write-Host "Game folder: $bin"
    if ($DryRun) { Write-Host '(dry run: Tailscale, winget and firewall steps are printed, not run)' -ForegroundColor DarkYellow }

    foreach ($f in $modFiles) {
        if (-not (Test-Path -LiteralPath (Join-Path $kit $f))) { throw "The kit is missing $f (unzip the whole kit first)." }
    }
    # The questions come first, in the window the person is already looking at: player name, then the join code.
    if (-not $PlayerName) { $PlayerName = Get-IniPlayerName $iniPath }
    if (-not $PlayerName -or $PlayerName -eq 'Hunter') {
        $typed = ''
        if (-not $Yes) { $typed = Read-Host "Your player name (Enter = $env:USERNAME)" }
        $PlayerName = if ($typed) { $typed } else { $env:USERNAME }
    }
    $PlayerName = ($PlayerName -replace '[^A-Za-z0-9 _\-]', '').Trim()
    if (-not $PlayerName) { $PlayerName = 'Hunter' }
    if ($PlayerName.Length -gt 31) { $PlayerName = $PlayerName.Substring(0, 31) }

    if (-not $JoinCode -and -not $IAmHost -and -not $Yes) {
        for ($try = 0; $try -lt 3 -and -not $JoinCode; $try++) {
            $typed = (Read-Host 'Paste the join code from your host (press Enter if you are the host)').Trim()
            if (-not $typed) { break }
            $script:JoinCodeError = ''
            if (ConvertFrom-JoinCode $typed) { $JoinCode = $typed } else { Write-Problem $script:JoinCodeError }
        }
    }
    $jc = $null
    if ($JoinCode) {
        $script:JoinCodeError = ''
        $jc = ConvertFrom-JoinCode $JoinCode
        if (-not $jc) { throw "Join code: $($script:JoinCodeError)" }
    }

    # One administrator prompt for everything: Tailscale and the firewall rule need it, and so does Program Files.
    if (-not $Elevated -and -not $DryRun -and -not (Test-IsAdmin)) {
        Write-Host 'Windows will ask for permission once (Tailscale and the firewall need administrator rights).'
        $argList = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"", '-GameDir', "`"$bin`"",
                     '-ShortcutDir', "`"$desk`"", '-LobbyIni', "`"$LobbyIni`"", '-Elevated')
        if ($PlayerName) { $argList += @('-PlayerName', "`"$PlayerName`"") }
        if ($Yes) { $argList += '-Yes' }
        if ($JoinCode) {
            $tmp = Join-Path $env:TEMP ('dh2coop-join-' + [guid]::NewGuid().ToString('N') + '.tmp')
            [IO.File]::WriteAllText($tmp, $JoinCode)
            $argList += @('-JoinCodeFile', "`"$tmp`"")
        } else {
            $argList += '-IAmHost'  # the question was asked here: no code means host
        }
        try {
            $p = Start-Process -FilePath 'powershell.exe' -ArgumentList ($argList -join ' ') -Verb RunAs -Wait -PassThru
            $exitCode = $p.ExitCode
        } catch {
            Write-Problem 'Administrator permission was not given, so nothing was installed. Run install.bat again and press Yes.'
            $exitCode = 1
        } finally {
            if ($tmp) { Remove-Item -LiteralPath $tmp -Force -ErrorAction SilentlyContinue }
        }
        exit $exitCode
    }

    # ---- the mod files and the shortcut (as before) ----
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
    Set-IniPlayerName $iniPath $PlayerName
    $exe = Join-Path $bin 'Main7.exe'
    New-Item -ItemType Directory -Force $desk | Out-Null
    foreach ($name in $oldShortcuts) { Remove-Item -LiteralPath (Join-Path $desk "$name.lnk") -Force -ErrorAction SilentlyContinue }
    New-Shortcut (Join-Path $desk 'DH2 COOP.lnk') $exe $bin $exe 'Dangerous Hunts 2 co-op: host or join from the lobby (F8) at the main menu' $launchArgs
    Write-Ok "Installed for player '$PlayerName' in $bin"
    Write-Ok "Desktop shortcut: DH2 COOP ($desk)"

    # ---- Tailscale ----
    Write-Step 'Tailscale'
    $tsStatus = ''
    if (Install-Tailscale) {
        if ($jc) {
            Write-Info "Join code: host '$($jc.Name)' at $($jc.Ip), port $($jc.Port), key $(Get-MaskedKey $jc.Key)"
            $tsStatus = Join-HostTailnet $jc $PlayerName
        } else {
            $h = Set-UpHostTailscale
            $tsStatus = $h.Status
            Show-HostFriendSteps $h.Ip $gamePort
        }
    } else {
        $tsStatus = 'NOT INSTALLED: install Tailscale from tailscale.com/download, then run install.bat again'
        Write-Problem $tsStatus
    }
    if ($tsStatus -match '\A(FAILED|NOT )') { $exitCode = 2 }

    # ---- Windows Firewall ----
    Write-Step 'Windows Firewall'
    try { Set-GameFirewall $exe } catch { Write-Problem "Could not set the firewall rule: $($_.Exception.Message)"; $fwFailed = $true }
    if ($fwFailed) { $exitCode = 2 }

    # ---- the lobby's JOIN fields ----
    if ($jc) {
        Write-Step 'Lobby'
        Set-LobbyIniValue $LobbyIni 'JoinHost' $jc.Ip
        Set-LobbyIniValue $LobbyIni 'JoinPort' ([string]$jc.Port)
        Write-Ok "The lobby's JOIN is filled in: $($jc.Ip) port $($jc.Port) ($LobbyIni)"
    }

    Write-Step 'Done'
    Write-Info "Tailscale: $tsStatus"
    if ($fwFailed) { Write-Problem 'Firewall: the rule was NOT created. Run install.bat again.' }
    else { Write-Info "Firewall: '$fwRuleName' (Main7.exe, UDP, Tailscale addresses only)" }
    if ($jc) {
        Write-Info 'Now: start DH2 COOP, the lobby opens, press JOIN (the host''s address is already there).'
    } else {
        Write-Info 'Now: start DH2 COOP, HOST, then "Copy join code" and send it to your friends.'
    }
    if ($DryRun) { Write-Host '(dry run: nothing was changed in Tailscale, winget or the firewall)' -ForegroundColor DarkYellow }
} catch {
    Write-Host ''
    Write-Host ('ERROR: ' + (Hide-Secrets $_.Exception.Message)) -ForegroundColor Red
    $exitCode = 1
}
if ($Elevated -and -not $Yes) {
    Write-Host ''
    [void](Read-Host 'Press Enter to close this window')
}
exit $exitCode
