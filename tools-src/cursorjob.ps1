# Runs one headless worker job through the Cursor CLI, pinned to Grok 4.7 Medium Fast (500K context).
# Replaces grokjob.ps1 (user, 2026-10-16: Grok Build CLI performance was not good enough).
# Usage: .\tools-src\cursorjob.ps1 -Name p5a-something [-Model grok-4.7-medium-fast] [-Force]
# Prompt: re\jobs\<Name>.md (re\jobs\_rules.md is prepended into re\jobs\<Name>.prompt.md).
# Output: re\jobs\<Name>.out.log (stdout + start/end markers), re\jobs\<Name>.err.log (stderr).
# Permissions: --auto-review by default (Cursor's classifier runs safe tool calls). -Force passes --force
# (run every command unless denied); use it only for jobs whose spec lists every command they run.
# The prompt goes in a file because a long prompt on the command line hits the 32K Windows limit.
param(
    [Parameter(Mandatory)][string]$Name,
    [string]$Model = 'grok-4.7-medium-fast',
    [switch]$Force
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$jobs = Join-Path $root 're\jobs'
$spec = Join-Path $jobs "$Name.md"
if (-not (Test-Path $spec)) { throw "missing $spec" }

$combined = Join-Path $jobs "$Name.prompt.md"
(Get-Content (Join-Path $jobs '_rules.md') -Raw) + "`n`n" + (Get-Content $spec -Raw) |
    Set-Content -Encoding utf8 $combined

$log = Join-Path $jobs "$Name.out.log"
$err = Join-Path $jobs "$Name.err.log"
$stdout = Join-Path $jobs "$Name.stdout.tmp"
$perm = if ($Force) { '--force' } else { '--auto-review' }
"=== $Name start $(Get-Date -Format s) model=$Model perm=$perm" | Set-Content -Encoding utf8 $log

$rel = "re/jobs/$Name.prompt.md"
$ask = "Read the file $rel in this workspace and carry out the job it describes, exactly as written. " +
       "When finished, print a short summary of what you changed and what you verified."
# cursor-agent is a .ps1 shim around node; call node through powershell so Start-Process can redirect.
$shim = (Get-Command cursor-agent).Source
$argList = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$shim`"",
             '-p', '--trust', $perm, '--model', $Model, '--workspace', "`"$root`"",
             '--output-format', 'text', "`"$ask`"") -join ' '
$p = Start-Process -FilePath 'powershell' -ArgumentList $argList -NoNewWindow -PassThru `
        -RedirectStandardOutput $stdout -RedirectStandardError $err -WorkingDirectory $root
$null = $p.Handle   # cache the handle so ExitCode is available after exit
$p.WaitForExit()

Get-Content $stdout -Raw -Encoding UTF8 -ErrorAction SilentlyContinue | Out-File -Append -Encoding utf8 $log
"=== $Name end $(Get-Date -Format s) exit=$($p.ExitCode)" | Out-File -Append -Encoding utf8 $log
Remove-Item $stdout -ErrorAction SilentlyContinue
Get-Content $log -Tail 40
