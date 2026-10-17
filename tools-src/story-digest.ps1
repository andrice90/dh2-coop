# Merge host and client dh2coop logs into one time-sorted story/event table.
# Usage: .\tools-src\story-digest.ps1 [-HostLog path] [-ClientLog path]
# kind is one of: arm, fire, request, apply, mission, task, exception.
param(
    [string]$HostLog = '',
    [string]$ClientLog = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $HostLog) { $HostLog = Join-Path $root 'game\Bin\dh2coop.log' }
if (-not $ClientLog) { $ClientLog = Join-Path $root 'game2\Bin\dh2coop.log' }

function Get-StoryKind([string]$msg) {
    # Client "sent to host" / "request" lines are requests even when they also say fire or trigger.
    if ($msg -match 'Access violation|first-chance exception|unexpected bytes') { return 'exception' }
    if ($msg -match 'story:.*CompleteTask') { return 'task' }
    if ($msg -match '\brequest\b|sent to host') { return 'request' }
    if ($msg -match 'trigger FIRE|applied host fire|fire commit|-> fire\b|fired by player') { return 'fire' }
    if ($msg -match 'trigger event activate|applied host trigger|trigger commit|-> active\b|volume activate') { return 'arm' }
    if ($msg -match 'events: apply commit kind=6') { return 'fire' }
    if ($msg -match 'events: apply commit kind=5') { return 'arm' }
    if ($msg -match 'events: commit kind=6') { return 'fire' }
    if ($msg -match 'events: commit kind=5') { return 'arm' }
    if ($msg -match 'events: (apply commit|commit) kind=(2|3|4)|mission end|mission message|CheckMissionStatus|trophy |claim |host map |following host') { return 'mission' }
    if ($msg -match 'cMission|MISSION') { return 'mission' }
    if ($msg -match '\bexception\b') { return 'exception' }
    # Remaining events: lines: journal, registry, replay, AnimalDied (kind=1) commit/apply.
    return 'apply'
}

function Get-StoryRows([string]$path, [string]$role) {
    if (-not (Test-Path -LiteralPath $path)) { throw "missing log: $path" }
    # The game keeps dh2coop.log open. ReadAllLines takes an exclusive read and fails.
    $stream = New-Object System.IO.FileStream($path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    $reader = New-Object System.IO.StreamReader($stream)
    $text = $reader.ReadToEnd()
    $reader.Dispose()
    $stream.Dispose()
    $rows = @()
    $ordinal = 0
    foreach ($line in ($text -split "`r?`n")) {
        if ($line -notmatch 'story:|events:|mission|exception|Access violation') { continue }
        if ($line -notmatch '^\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\]') { continue }
        $stamp = $Matches[1]
        $msg = $line
        # Drop "[time] [level] [thread] " so detail starts at the story:/events: text.
        $third = $line.IndexOf('] ', $line.IndexOf('] ') + 2)
        if ($third -ge 0) {
            $fourth = $line.IndexOf('] ', $third + 2)
            if ($fourth -ge 0) { $msg = $line.Substring($fourth + 2) }
        }
        $ordinal++
        $rows += [pscustomobject]@{
            Time    = [datetime]::ParseExact($stamp, 'yyyy-MM-dd HH:mm:ss.fff', [Globalization.CultureInfo]::InvariantCulture)
            Role    = $role
            Kind    = (Get-StoryKind $msg)
            Detail  = $msg
            Ordinal = $ordinal
        }
    }
    return $rows
}

$rows = @(Get-StoryRows $HostLog 'host') + @(Get-StoryRows $ClientLog 'client')
$rows = @($rows | Sort-Object Time, @{ Expression = { if ($_.Role -eq 'host') { 0 } else { 1 } } }, Ordinal)

'time | role | kind | detail'
foreach ($row in $rows) {
    '{0} | {1} | {2} | {3}' -f $row.Time.ToString('HH:mm:ss.fff'), $row.Role, $row.Kind, $row.Detail
}
'rows={0}' -f $rows.Count
