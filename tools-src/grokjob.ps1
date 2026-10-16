# Runs one headless Grok Build job (grok-4.7) from a prompt file in re\jobs\.
# Usage: .\tools-src\grokjob.ps1 -Name track-a-net [-Effort high]
# Prompt: re\jobs\<Name>.md (re\jobs\_rules.md is prepended). Output: re\jobs\<Name>.out.log
# Permission mode is Grok's 'auto' (its classifier approves safe actions); never bypassPermissions.
param(
    [Parameter(Mandatory)][string]$Name,
    [ValidateSet('low', 'medium', 'high', 'xhigh')][string]$Effort = 'high',
    [int]$MaxTurns = 400
)
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$jobs = Join-Path $root 're\jobs'
$prompt = Join-Path $jobs "$Name.md"
if (-not (Test-Path $prompt)) { throw "missing $prompt" }

$combined = Join-Path $jobs "$Name.prompt.tmp"
(Get-Content (Join-Path $jobs '_rules.md') -Raw) + "`n`n" + (Get-Content $prompt -Raw) |
    Set-Content -Encoding utf8 $combined

$log = Join-Path $jobs "$Name.out.log"
"=== $Name start $(Get-Date -Format s) effort=$Effort" | Set-Content -Encoding utf8 $log
& grok --prompt-file $combined -m grok-4.7 --reasoning-effort $Effort --max-turns $MaxTurns `
    --cwd $root --permission-mode auto --output-format plain --no-alt-screen 2>&1 |
    Out-File -Append -Encoding utf8 $log
"=== $Name end $(Get-Date -Format s) exit=$LASTEXITCODE" | Out-File -Append -Encoding utf8 $log
Remove-Item $combined -ErrorAction SilentlyContinue
Get-Content $log -Tail 40
