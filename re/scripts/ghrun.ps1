# Run any Ghidra postScript from re\scripts against a slot project (read-only, no analysis).
# Usage: .\re\scripts\ghrun.ps1 -Slot D -Binary Main7.exe -Script DumpPtrs.java -ScriptArgs 'out.txt','29748','20'
param(
    [Parameter(Mandatory)][ValidatePattern('^[A-Z]$')][string]$Slot,
    [Parameter(Mandatory)][ValidateSet('Main7.exe', 'EngineDll7.dll', 'CommonDll7.dll', 'GameDll7.dll')][string]$Binary,
    [Parameter(Mandatory)][string]$Script,
    [string[]]$ScriptArgs = @()
)
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
if (-not $env:JAVA_HOME) { $env:JAVA_HOME = [Environment]::GetEnvironmentVariable('JAVA_HOME', 'User') }
$proj = Join-Path $root "re\ghidra-slots\$Slot"
if (-not (Test-Path "$proj\DH2.gpr")) {
    New-Item -ItemType Directory -Force $proj | Out-Null
    Copy-Item "$root\re\ghidra\DH2.gpr" $proj
    Copy-Item "$root\re\ghidra\DH2.rep" $proj -Recurse
}
# first script arg is the output file: make absolute if relative
if ($ScriptArgs.Count -gt 0 -and -not [IO.Path]::IsPathRooted($ScriptArgs[0])) { $ScriptArgs[0] = Join-Path $root $ScriptArgs[0] }
& "$root\tools\ghidra_12.1.4_PUBLIC\support\analyzeHeadless.bat" $proj DH2 `
    -process $Binary -noanalysis -readOnly `
    -scriptPath "$root\re\scripts" -postScript $Script @ScriptArgs 2>&1 |
    Select-String "$([IO.Path]::GetFileNameWithoutExtension($Script))|ERROR|Exception" | ForEach-Object Line
