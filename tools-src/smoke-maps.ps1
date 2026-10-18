# Smoke pass over the story maps (job smoke-all-maps, re\notes\smoke-all-maps.md).
# Usage: .\tools-src\smoke-maps.ps1 -Maps MAP1,MAP2A [-Phase base|restart|next|players3|late3|walk|exit] [-Port 27690]
#   base      run-coop-local -Map <MAP> -Seconds 120 -Capture (two windows, no input). A watchdog polls every 3 s:
#             both processes alive and responding, no WerFault dialog for them (alive.txt). Then the logs are copied
#             and one table row is produced (smoke-row.sh).
#   restart   F5 test: 25 s after the last welcome, "coop_level restart" is written twice into game\Bin\dh2coop.cmd
#             (1.2 s apart, inside the 3 s window of the F5 confirm). Run 120 s: do both windows come back to the map?
#   next      F6 test: the same with "coop_level next" (MAP3C -> MAP4A).
#   players3  -Players 3 -Seconds 120 -Capture (the third window joins after the second one is welcomed).
#   late3     -Players 3 -LateJoin 20 -Seconds 60 -Capture.
#   walk      session without -Seconds; first tasks in script order and the exit task (8 s apart, captures after each):
#             the host is coop_tp'd into the task's trigger volume (BSP position / 100: the real OnHit/BeginCutScene flow);
#             when that does not fire it, coop_trigger on the host command file (arm + fire directly, skips OnHit and
#             BeginCutScene). -NoTp = coop_trigger only. coop_tohost on the client once; then logs are copied.
#   exit      fires the exit task on the host (ActivateTriggerLevel only arms the exit zone), then coop_tp the host into the
#             zone (BSP trigger_level position / 100) and watches 40 s: which map loads, does the client follow.
#             (walk and exit are the earlier agent's phases; not run in the 2026-10-18 baseline pass.)
# A session that fails to START (no host log line, window timeout, ...) is repeated once (dir <phase>-rerun); a game that
# dies after the welcome is a result and is not repeated.
# Output: re\notes\coop-local\smoke\<MAP>\<phase>\ (logs of every tree, run-output.txt, alive.txt, status.txt, summary row),
# rows appended to re\notes\coop-local\smoke\rows.md; screenshots re\notes\coop-local\smoke-<MAP>-<phase>-<stamp>-<role>.png
# (the run script captures every 10 s; only -KeepShots of them are kept, spread over the run).
# Only processes whose path is this checkout's game*\Bin\Main7.exe are touched. No keyboard or mouse input is sent.
param(
    [Parameter(Mandatory)][string[]]$Maps,
    [ValidateSet('base', 'restart', 'next', 'players3', 'late3', 'walk', 'exit')][string]$Phase = 'base',
    [int]$Port = 27690,
    [int]$Seconds = 120,
    [int]$KeepShots = 4,
    [int]$StepWait = 8,
    [switch]$NoTp
)
$ErrorActionPreference = 'Stop'
# powershell -File passes "MAP1,MAP2A" as one string
$Maps = @($Maps | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$root = Split-Path $PSScriptRoot -Parent
$runScript = Join-Path $root 'run-coop-local.ps1'
$smokeRoot = Join-Path $root 're\notes\coop-local\smoke'
$shotDir = Join-Path $root 're\notes\coop-local'
$shotRoot = 're\notes\coop-local\smoke'   # relative: capture-window.ps1 joins -Out onto the cwd (walk / exit phases)
$trees = 'game', 'game2', 'game3', 'game4'
$bash = Join-Path $env:ProgramFiles 'Git\bin\bash.exe'
New-Item -ItemType Directory -Force $smokeRoot | Out-Null

# Trigger walk (walk phase): first ~6 task triggers in script order (SetName order in SCRIPTS\TRIGGERS\TRIGGER*.SCR), then the
# exit / mission-end task last.
$walk = @{
    MAP1 = '01 02 03 04 05 06 09'; MAP2A = '01 02 03 04 05 14 08'; MAP2B = '01 02 03 04 05 06 18'
    MAP2C = '01 02 03 04 05 08 09'; MAP3A = '01 15 16 02 03 04 14'; MAP3B = '01 02 03 04 06 07 05'
    MAP3C = '01 02 03 04 05'; MAP4A = '01 02 03 04 05 06 09'; MAP4B = '01 02 03 04 05 06 20'
    MAP5 = '01 02 03 04 05 06 20'; MAP6 = '01 10 02 03 04 05 09'; MAP7 = '01 02 03 04 05 06 11'
    MAP8 = '01 02 03 04 05 69 15'; MAP9 = '01 02 03 04 05 06 11'; MAP10 = '01 22 02 03 21 04 17'
    MAP11A = '01 09 06 03 07 10 08'; MAP11B = '01 02 03 04 05 10 09'; MAP11C = '01 02 06 03 07 10 08'
    MAP12A = '01 02 03 04 05 10 09'; MAP12B = '01 02'; MAP12C = '01 02'
    HUNTMAP1 = '01 10'; HUNTMAP2 = '01 10'; HUNTMAP3 = '01 10'; HUNTMAP4 = '01 10'
    MAPARENA = '01'; MAPISLAND = '01 09'; MAPTSAVO = '01'
}

# Exit zones (exit phase): map, exit task, token, host teleport target (trigger_level "position" in the BSP entity text,
# / 100 -> m). BSP "target" of each: MAP2A exit_1 map2b, MAP2B exit_1 map2b, MAP3A exit_1 map3b/human1 and exit_2
# map3b/human2, MAP3B exit_1 map3c, MAP4A exit4b map4b, MAP11A exit_1 map11b, MAP11B exit_1 map11c, MAP12A exit_1 map12b,
# MAP12B exit_2 map12b.
$exits = @(
    @{ Map = 'MAP2A'; Task = 'cScrTask08'; Token = 'exit_1'; Pos = '-44.92:44.93:-33.24' },
    @{ Map = 'MAP2B'; Task = 'cScrTask18'; Token = 'exit_1'; Pos = '61.10:44.93:-63.11' },
    @{ Map = 'MAP3A'; Task = 'cScrTask14'; Token = 'exit_1'; Pos = '-40.195:10.835:-22.805' },
    @{ Map = 'MAP3A'; Task = 'cScrTask14'; Token = 'exit_2'; Pos = '-129.96:10.835:-11.305' },
    @{ Map = 'MAP3B'; Task = 'cScrTask05'; Token = 'exit_1'; Pos = '-213.5:10.835:5.805' },
    @{ Map = 'MAP4A'; Task = 'cScrTask09'; Token = 'exit4b'; Pos = '191.81:10.835:193.125' },
    @{ Map = 'MAP11A'; Task = 'cScrTask08'; Token = 'exit_1'; Pos = '155.145:44.93:275.70' },
    @{ Map = 'MAP11B'; Task = 'cScrTask09'; Token = 'exit_1'; Pos = '894.295:142.094:-419.765' },
    @{ Map = 'MAP12A'; Task = 'cScrTask09'; Token = 'exit_1'; Pos = '-155.14:12.74:-10.62' },
    @{ Map = 'MAP12B'; Task = 'cScrTask02'; Token = 'exit_2'; Pos = '-280.94:12.74:18.13' }
)

# UTF-8 without a BOM (Set-Content -Encoding UTF8 writes one in Windows PowerShell 5.1)
function Put-Text([string]$Path, [string]$Text, [switch]$Append) {
    $enc = New-Object Text.UTF8Encoding($false)
    if ($Append) { [IO.File]::AppendAllText($Path, $Text + "`r`n", $enc) } else { [IO.File]::WriteAllText($Path, $Text, $enc) }
}

function Progress([string]$m) {
    # a reader holding the file open must not abort the run: try a few times, then give up on the file
    for ($i = 0; $i -lt 5; $i++) {
        try {
            Add-Content -LiteralPath (Join-Path $smokeRoot 'progress.txt') -Value ("{0} {1}" -f (Get-Date -Format 'HH:mm:ss'), $m) -ErrorAction Stop
            break
        } catch { Start-Sleep -Milliseconds 300 }
    }
    Write-Host $m
}

function Get-Ours([string]$tree) {
    $want = [IO.Path]::GetFullPath((Join-Path $root "$tree\Bin\Main7.exe"))
    foreach ($p in @(Get-Process Main7 -ErrorAction SilentlyContinue)) {
        $exe = $null
        try { $exe = $p.Path } catch { }
        if ($exe -and ([IO.Path]::GetFullPath($exe) -ieq $want)) { return $p }
    }
    return $null
}

function Stop-Ours {
    foreach ($t in $trees) { $p = Get-Ours $t; if ($p) { try { $p.CloseMainWindow() | Out-Null } catch { } } }
    Start-Sleep -Seconds 5
    foreach ($t in $trees) { $p = Get-Ours $t; if ($p) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } }
}

function Copy-Shared([string]$Src, [string]$Dst) {
    if (-not (Test-Path -LiteralPath $Src)) { return }
    $in = $null; $out = $null
    try {
        $in = New-Object IO.FileStream($Src, 'Open', 'Read', 'ReadWrite,Delete')
        $out = New-Object IO.FileStream($Dst, 'Create', 'Write', 'None')
        $in.CopyTo($out)
    } catch { Write-Host "copy failed $Src : $($_.Exception.Message)" } finally {
        if ($out) { $out.Dispose() }
        if ($in) { $in.Dispose() }
    }
}

function Read-Shared([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return '' }
    $fs = $null; $sr = $null
    try {
        $fs = New-Object IO.FileStream($Path, 'Open', 'Read', 'ReadWrite,Delete')
        $sr = New-Object IO.StreamReader($fs)
        return $sr.ReadToEnd()
    } catch { return '' } finally {
        if ($sr) { $sr.Dispose() } elseif ($fs) { $fs.Dispose() }
    }
}

# The game keeps its logs open: copy with shared access. Only trees that wrote a log during this run.
function Save-Logs([string]$Dest, [datetime]$Since) {
    New-Item -ItemType Directory -Force $Dest | Out-Null
    foreach ($t in $trees) {
        $bin = Join-Path $root "$t\Bin"
        if (-not (Test-Path -LiteralPath $bin)) { continue }
        $log = Join-Path $bin 'dh2coop.log'
        if ((Test-Path -LiteralPath $log) -and (Get-Item -LiteralPath $log).LastWriteTime -ge $Since) {
            Copy-Shared $log (Join-Path $Dest "$t-dh2coop.log")
            Copy-Shared (Join-Path $bin 'debug.log') (Join-Path $Dest "$t-debug.log")
        }
        $dmp = Join-Path $bin 'dh2coop-crash.dmp'
        if ((Test-Path -LiteralPath $dmp) -and (Get-Item -LiteralPath $dmp).LastWriteTime -ge $Since) {
            Copy-Shared $dmp (Join-Path $Dest "$t-crash.dmp")
        }
    }
    $rl = Join-Path $shotDir 'run-log.txt'
    if (Test-Path -LiteralPath $rl) { Copy-Item -LiteralPath $rl (Join-Path $Dest 'run-log.txt') -Force }
}

# Screenshots the run script took (<stamp>-game*.png, every 10 s): rename the ones of this run to
# smoke-<MAP>-<phase>-<stamp>-<role>.png and keep $KeepShots stamps spread over the run (first, last, between).
function Keep-Shots([string]$M, [string]$Name, [datetime]$Since) {
    $files = @(Get-ChildItem -LiteralPath $shotDir -Filter '2*-game*.png' -ErrorAction SilentlyContinue |
        Where-Object { $_.LastWriteTime -ge $Since })
    $stamps = @($files | ForEach-Object { $_.Name.Substring(0, 15) } | Sort-Object -Unique)
    if ($stamps.Count -eq 0) { return }
    $keep = @{}
    if ($stamps.Count -le $KeepShots) { foreach ($s in $stamps) { $keep[$s] = $true } }
    else {
        for ($i = 0; $i -lt $KeepShots; $i++) { $keep[$stamps[[int][Math]::Round($i * ($stamps.Count - 1) / ($KeepShots - 1))]] = $true }
    }
    foreach ($f in $files) {
        $stamp = $f.Name.Substring(0, 15)
        if ($keep.ContainsKey($stamp)) {
            $role = $f.BaseName.Substring(16)
            Move-Item -LiteralPath $f.FullName -Destination (Join-Path $shotDir "smoke-$M-$Name-$stamp-$role.png") -Force
        } else {
            Remove-Item -LiteralPath $f.FullName -Force
        }
    }
}

function Shot([string]$Rel, [string]$Tag) {
    foreach ($t in 'game', 'game2') {
        $p = Get-Ours $t
        if (-not $p) { continue }
        # capture-window.ps1 joins -Out onto its cwd: run it from the checkout root with a relative path
        $cmd = "Set-Location -LiteralPath '$root'; & '$(Join-Path $root 'tools-src\capture-window.ps1')' -ProcessId $($p.Id) -Out '$Rel\$Tag-$t.png' -NoFocus"
        try { & powershell.exe -NoProfile -ExecutionPolicy Bypass -Command $cmd 2>&1 | Out-Null } catch { }
    }
}

function Send-Cmd([string]$Tree, [string]$Line, [int]$WaitMs = 1500) {
    [IO.File]::WriteAllLines((Join-Path $root "$Tree\Bin\dh2coop.cmd"), [string[]]@($Line))
    Start-Sleep -Milliseconds $WaitMs
}

# One poll of the watchdog: "game=ok game2=ok". DEAD = no process from this tree; NOTRESP = message pump stuck;
# WER = a WerFault.exe was started for that pid (crash dialog up).
function Get-AliveLine([int]$Players) {
    $wer = @()
    if (Get-Process WerFault -ErrorAction SilentlyContinue) {
        try { $wer = @(Get-CimInstance Win32_Process -Filter "Name='WerFault.exe'" -ErrorAction Stop | ForEach-Object { $_.CommandLine }) } catch { }
    }
    $bits = @()
    for ($i = 1; $i -le $Players; $i++) {
        $t = $(if ($i -eq 1) { 'game' } else { "game$i" })
        $p = Get-Ours $t
        if (-not $p) { $bits += "$t=DEAD"; continue }
        $state = 'ok'
        if (-not $p.Responding) { $state = 'NOTRESP' }
        foreach ($c in $wer) { if ($c -match "(^|\s)-p\s+$($p.Id)(\s|$)") { $state = 'WER' } }
        $bits += "$t=$state"
    }
    return ($bits -join ' ')
}

# coop_trigger arms an unarmed trigger and fires an armed one: write again when the log says it only armed.
function Send-Trigger([string]$Name, [scriptblock]$Note) {
    $hostLog = Join-Path $root 'game\Bin\dh2coop.log'
    for ($try = 1; $try -le 2; $try++) {
        Send-Cmd 'game' "coop_trigger $Name"
        $line = (Read-Shared $hostLog) -split "`n" | Where-Object { $_ -match "coop_trigger '$Name'" } | Select-Object -Last 1
        if (-not $line) { & $Note "trigger $Name : no coop_trigger log line after write $try"; continue }
        if ($line -match 'not found') { & $Note "trigger $Name : not found"; return }
        if ($line -match '-> arm') { & $Note "trigger $Name : armed (write $try), writing again"; continue }
        & $Note "trigger $Name : fired (write $try)"
        return
    }
}

# Trigger-event volumes: BSP entity text ("trigger_event" position, script class name), position / 100 = metres.
$bspDir = @("$root\re\extracted\BSP", 'D:\scary hunt\dh2coop\re\extracted\BSP') | Where-Object { Test-Path -LiteralPath $_ } |
    Select-Object -First 1
function Get-TriggerPos([string]$Map) {
    $h = @{}
    if (-not $bspDir) { return $h }
    $path = Join-Path $bspDir "$Map.BSP"
    if (-not (Test-Path -LiteralPath $path)) { return $h }
    $text = [regex]::Replace([Text.Encoding]::GetEncoding(28591).GetString([IO.File]::ReadAllBytes($path)), '[^\x20-\x7e]', '.')
    $rx = [regex]'entity\("trigger_event"\)\.+property\("position", "([^"]*)"\)\.+property\("angle", "[^"]*"\)\.+property\("name", "[^"]*"\)\.+property\("target", "[^"]*"\)\.+property\("target_name", "[^"]*"\)\.+property\("script class name", "([^"]*)"\)'
    $inv = [Globalization.CultureInfo]::InvariantCulture
    foreach ($m in $rx.Matches($text)) {
        $cls = $m.Groups[2].Value
        $p = $m.Groups[1].Value -split ' '
        if ($h.ContainsKey($cls) -or $p.Count -ne 3) { continue }
        $v = $p | ForEach-Object { ([double]::Parse($_, $inv) / 100).ToString('0.###', $inv) }
        $h[$cls] = $v -join ':'
    }
    return $h
}

# One task of the walk: coop_tp the host into the trigger volume first (real flow: OnHit, BeginCutScene, the zone test);
# when that does not fire it within 5 s, coop_trigger (arm + fire directly, skips OnHit/BeginCutScene).
function Invoke-Task([string]$Name, [string]$Pos, [scriptblock]$Note) {
    $hostLog = Join-Path $root 'game\Bin\dh2coop.log'
    $fireRx = "\[host\] trigger FIRE id=\d+ name='$Name'"
    $before = @((Read-Shared $hostLog) -split "`n" | Where-Object { $_ -match $fireRx }).Count
    if ($Pos -and -not $NoTp) {
        Send-Cmd 'game' "coop_tp $Pos" 1000
        for ($k = 0; $k -lt 5; $k++) {
            Start-Sleep -Seconds 1
            $now = @((Read-Shared $hostLog) -split "`n" | Where-Object { $_ -match $fireRx }).Count
            if ($now -gt $before) { & $Note "trigger $Name : FIRED BY coop_tp into the volume at $Pos (real flow)"; return }
        }
        & $Note "trigger $Name : coop_tp $Pos did not fire it (not armed, already fired, or zone not reached); forcing"
    } elseif (-not $Pos) {
        & $Note "trigger $Name : no volume position in the BSP; forcing"
    }
    Send-Trigger $Name $Note
}

# A launch problem (not a game that died after the welcome) is worth one repeat.
function Test-StartupFailure([string]$Text) {
    return [bool]($Text -match 'exited during startup|did not log|could not listen|disappeared during startup|timed out after|failed to start|windows were not arranged|missing:|no host')
}

# Timed session: the run script (-Seconds) in a child process, a watchdog loop here. $Cmds = @(@{ At = s; Tree = 'game';
# Line = '...' }) are written into <tree>\Bin\dh2coop.cmd At seconds after the last welcome (the run script's countdown start).
function Run-Timed([string]$M, [string]$Name, [int]$Players, [int]$Secs, $Cmds, [int]$Late = 0) {
    $dest = Join-Path $smokeRoot "$M\$Name"
    New-Item -ItemType Directory -Force $dest | Out-Null
    $since = Get-Date
    $outFile = Join-Path $dest 'run-output.txt'
    $errFile = Join-Path $dest 'run-error.txt'
    $aliveFile = Join-Path $dest 'alive.txt'
    foreach ($f in $outFile, $errFile, $aliveFile) { Remove-Item -LiteralPath $f -ErrorAction SilentlyContinue }
    Progress "[$M] $Name start (players $Players, ${Secs}s)"
    if (Get-Process Main7 -ErrorAction SilentlyContinue | Where-Object { try { $_.Path -like "$root\game*\Bin\Main7.exe" } catch { $false } }) { Stop-Ours }
    $ra = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$runScript`"", '-Map', $M, '-Port', $Port,
        '-Seconds', $Secs, '-Capture', '-Players', $Players)
    if ($Late -gt 0) { $ra += @('-LateJoin', $Late) }
    $proc = Start-Process powershell.exe -ArgumentList $ra -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput $outFile -RedirectStandardError $errFile
    $t0 = $null; $sent = @{}; $nextPoll = Get-Date; $lastLine = ''
    while (-not $proc.HasExited) {
        $now = Get-Date
        if (-not $t0) {
            $welcomed = ([regex]::Matches((Read-Shared $outFile), 'welcomed pid=')).Count
            if ($welcomed -ge ($Players - 1)) { $t0 = $now }
        }
        $el = $(if ($t0) { ($now - $t0).TotalSeconds } else { -1 })
        if ($t0) {
            for ($i = 0; $i -lt @($Cmds).Count; $i++) {
                if (-not $sent.ContainsKey($i) -and $el -ge $Cmds[$i].At) {
                    $sent[$i] = $true
                    Send-Cmd $Cmds[$i].Tree $Cmds[$i].Line 1200
                    Add-Content -LiteralPath $aliveFile -Value ("# t=+{0:0}s sent to {1}: {2}" -f $el, $Cmds[$i].Tree, $Cmds[$i].Line)
                }
            }
        }
        if ($t0 -and $now -ge $nextPoll -and $el -lt ($Secs - 4)) {
            $nextPoll = $now.AddSeconds(3)
            $lastLine = "t=+{0:0}s {1}" -f $el, (Get-AliveLine $Players)
            Add-Content -LiteralPath $aliveFile -Value $lastLine
        }
        Start-Sleep -Milliseconds 500
    }
    $proc.WaitForExit()
    $text = (Read-Shared $outFile) + (Read-Shared $errFile)
    $status = 'ok'
    if ($text -match '(game\d* pid=\d+ exited before the run finished \(code [^)]*\))') { $status = 'DIED ' + $Matches[1] }
    elseif ($lastLine -match 'DEAD|NOTRESP|WER') { $status = "last poll: $lastLine" }
    elseif (Test-StartupFailure $text) { $status = 'startup-error' }
    elseif ($text -notmatch 'closed after') { $status = 'run script ended without "closed after"' }
    Stop-Ours
    Save-Logs $dest $since
    Keep-Shots $M $Name $since
    Put-Text (Join-Path $dest 'status.txt') $status
    $rel = "re/notes/coop-local/smoke/$M/$Name"
    if (Test-Path -LiteralPath $bash) {
        Push-Location $root
        try {
            $row = (& $bash 'tools-src/smoke-row.sh' $rel) -join "`n"
            Put-Text (Join-Path $dest 'row.md') $row
            Put-Text (Join-Path $smokeRoot 'rows.md') $row -Append
        } catch { Write-Host "row failed: $($_.Exception.Message)" } finally { Pop-Location }
    }
    Progress "[$M] $Name done: $status"
    return $status
}

function Run-Session([string]$M, [string]$Kind, [string]$Name, $Exit = $null) {
    $dest = Join-Path $smokeRoot "$M\$Name"
    New-Item -ItemType Directory -Force $dest | Out-Null
    $rel = "$shotRoot\$M\$Name"
    $since = Get-Date
    $status = 'ok'
    $notesFile = Join-Path $dest 'walk-notes.txt'
    $note = { param($s) Add-Content -LiteralPath $notesFile -Value ("{0} {1}" -f (Get-Date -Format 'HH:mm:ss'), $s) -Encoding UTF8 }
    Progress "[$M] $Name start"
    if ((Get-Ours 'game') -or (Get-Ours 'game2')) { Stop-Ours }
    try {
        if ($Kind -eq 'exit') {
            & $runScript -Map $M -Port $Port *>&1 | Out-File -LiteralPath (Join-Path $dest 'run-output.txt') -Encoding utf8
            $hostP = Get-Ours 'game'; $cliP = Get-Ours 'game2'
            if (-not $hostP -or -not $cliP) { throw "after launch: host alive=$([bool]$hostP) client alive=$([bool]$cliP)" }
            & $note "session up: host pid $($hostP.Id) client pid $($cliP.Id)"
            Start-Sleep -Seconds 12
            Shot $rel '00-start'
            Send-Trigger $Exit.Task $note          # OnActivate arms the exit zone (ActivateTriggerLevel)
            Start-Sleep -Seconds 6
            Shot $rel '01-armed'
            & $note "host: coop_tp $($Exit.Pos) (exit zone $($Exit.Token))"
            Send-Cmd 'game' "coop_tp $($Exit.Pos)"
            foreach ($sec in 5, 10, 15, 20, 25, 30, 35, 40) {
                Start-Sleep -Seconds 5
                & $note "after tp +${sec}s alive: host=$([bool](Get-Ours 'game')) client=$([bool](Get-Ours 'game2'))"
                if ($sec -eq 10 -or $sec -eq 30) { Shot $rel ("02-tp-p{0}" -f $sec) }
            }
            if (-not (Get-Ours 'game')) { $status = 'host died' } elseif (-not (Get-Ours 'game2')) { $status = 'client died' }
        } else {
            & $runScript -Map $M -Port $Port *>&1 | Out-File -LiteralPath (Join-Path $dest 'run-output.txt') -Encoding utf8
            $hostP = Get-Ours 'game'; $cliP = Get-Ours 'game2'
            if (-not $hostP -or -not $cliP) { throw "after launch: host alive=$([bool]$hostP) client alive=$([bool]$cliP)" }
            & $note "session up: host pid $($hostP.Id) client pid $($cliP.Id)"
            Start-Sleep -Seconds 12
            Shot $rel '00-start'
            $names = @($walk[$M] -split ' ' | ForEach-Object { "cScrTask$_" })
            $trigPos = Get-TriggerPos $M
            $i = 0
            foreach ($t in $names) {
                $i++
                $isExit = ($i -eq $names.Count -and $names.Count -gt 1)
                if (-not (Get-Ours 'game')) { & $note "HOST PROCESS GONE before $t"; $status = 'host died'; break }
                if (-not (Get-Ours 'game2')) { & $note "CLIENT PROCESS GONE before $t"; $status = 'client died' }
                Invoke-Task $t $trigPos[$t] $note
                if ($i -eq 2) { Send-Cmd 'game2' 'coop_tohost'; & $note 'client: coop_tohost' }
                if ($isExit) {
                    # the exit task may load the next map: watch 30 s, capture at 10 s and 25 s
                    foreach ($sec in 5, 10, 15, 20, 25, 30) {
                        Start-Sleep -Seconds 5
                        $alive = "host=$([bool](Get-Ours 'game')) client=$([bool](Get-Ours 'game2'))"
                        & $note "after exit trigger +${sec}s alive: $alive"
                        if ($sec -eq 10 -or $sec -eq 25) { Shot $rel ("{0:00}-{1}-p{2}" -f $i, $t, $sec) }
                    }
                } else {
                    Start-Sleep -Seconds $StepWait
                    Shot $rel ("{0:00}-{1}" -f $i, $t)
                }
            }
            if (-not (Get-Ours 'game')) { if ($status -eq 'ok') { $status = 'host died' } }
            elseif (-not (Get-Ours 'game2')) { if ($status -eq 'ok') { $status = 'client died' } }
            & $note "walk done: host alive=$([bool](Get-Ours 'game')) client alive=$([bool](Get-Ours 'game2'))"
        }
    } catch {
        $status = "error: $($_.Exception.Message)"
        & $note "EXCEPTION $status"
    }
    Stop-Ours
    Save-Logs $dest $since
    Put-Text (Join-Path $dest 'status.txt') $status
    Progress "[$M] $Name done: $status"
    return $status
}

function Run-Timed-Retry([string]$M, [string]$Name, [int]$Players, [int]$Secs, $Cmds, [int]$Late = 0) {
    $s = Run-Timed $M $Name $Players $Secs $Cmds $Late
    if ($s -eq 'startup-error') { [void](Run-Timed $M "$Name-rerun" $Players $Secs $Cmds $Late) }
}

foreach ($m in $Maps) {
    switch ($Phase) {
        'base' { Run-Timed-Retry $m 'base' 2 $Seconds @() }
        'players3' { Run-Timed-Retry $m 'players3' 3 $Seconds @() }
        'late3' { Run-Timed-Retry $m 'late3' 3 60 @() 20 }
        'restart' {
            # the command arms and confirms by itself (cmd_level calls HostRestartLevel twice); the second write is the
            # second "press" of the brief and lands while the host is already hopping through the main menu
            Run-Timed-Retry $m 'restart' 2 $Seconds @(
                @{ At = 25; Tree = 'game'; Line = 'coop_level restart' },
                @{ At = 25; Tree = 'game'; Line = 'coop_level restart' })
        }
        'next' {
            Run-Timed-Retry $m 'next' 2 $Seconds @(
                @{ At = 25; Tree = 'game'; Line = 'coop_level next' },
                @{ At = 25; Tree = 'game'; Line = 'coop_level next' })
        }
        'walk' {
            if (-not $walk.ContainsKey($m)) { Progress "[$m] no walk table"; continue }
            if ((Run-Session $m 'walk' 'walk') -like 'error*') { [void](Run-Session $m 'walk' 'walk-rerun') }
        }
        'exit' {
            foreach ($e in @($exits | Where-Object { $_.Map -eq $m })) {
                $nm = "exit-$($e.Token)"
                if ((Run-Session $m 'exit' $nm $e) -like 'error*') { [void](Run-Session $m 'exit' "$nm-rerun" $e) }
            }
        }
    }
}
Progress 'ALL DONE'
