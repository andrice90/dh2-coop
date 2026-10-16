# Runs one headless Grok Build job (grok-4.7) from a prompt file in re\jobs\.
# Usage: .\tools-src\grokjob.ps1 -Name track-a-net [-Effort high]
# Prompt: re\jobs\<Name>.md (re\jobs\_rules.md is prepended).
# Output: re\jobs\<Name>.out.log (stdout + start/end markers), re\jobs\<Name>.err.log (stderr: compaction notices etc.)
# Permission mode is Grok's 'auto' (its classifier approves safe actions); never bypassPermissions.
# Start-Process with file redirection is used on purpose: in Windows PowerShell 5.1, piping a native
# command's stderr (2>&1) turns lines like "Auto-compacting conversation..." into terminating errors
# under ErrorActionPreference=Stop, which killed a job on 2026-10-16.
param(
    [Parameter(Mandatory)][string]$Name,
    [ValidateSet('low', 'medium', 'high', 'xhigh')][string]$Effort = 'high',
    [int]$MaxTurns = 400
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$jobs = Join-Path $root 're\jobs'
$prompt = Join-Path $jobs "$Name.md"
if (-not (Test-Path $prompt)) { throw "missing $prompt" }

$combined = Join-Path $jobs "$Name.prompt.tmp"
(Get-Content (Join-Path $jobs '_rules.md') -Raw) + "`n`n" + (Get-Content $prompt -Raw) |
    Set-Content -Encoding utf8 $combined

$log = Join-Path $jobs "$Name.out.log"
$err = Join-Path $jobs "$Name.err.log"
$stdout = Join-Path $jobs "$Name.stdout.tmp"
"=== $Name start $(Get-Date -Format s) effort=$Effort" | Set-Content -Encoding utf8 $log

$grokArgs = "--prompt-file `"$combined`" -m grok-4.7 --reasoning-effort $Effort --max-turns $MaxTurns " +
            "--cwd `"$root`" --permission-mode auto --output-format plain --no-alt-screen"
$p = Start-Process -FilePath 'grok' -ArgumentList $grokArgs -NoNewWindow -PassThru `
        -RedirectStandardOutput $stdout -RedirectStandardError $err
$null = $p.Handle   # cache the handle so ExitCode is available after exit
$p.WaitForExit()

Get-Content $stdout -Raw -Encoding UTF8 -ErrorAction SilentlyContinue | Out-File -Append -Encoding utf8 $log
"=== $Name end $(Get-Date -Format s) exit=$($p.ExitCode)" | Out-File -Append -Encoding utf8 $log
Remove-Item $combined, $stdout -ErrorAction SilentlyContinue
Get-Content $log -Tail 40
