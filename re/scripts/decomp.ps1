# Decompile functions by name substring from a headless Ghidra project.
# Usage: .\re\scripts\decomp.ps1 -Slot A -Binary EngineDll7.dll -Out re\notes\net-decomp.c -Names 'cNetServer::','cNetClient::'
# Prefix a name with '=' for an exact match. Output is appended.
# Each agent uses its own -Slot (a private copy of the master project in re\ghidra-slots\<Slot>), because a
# Ghidra project allows one process at a time. Within one slot, run decompiles sequentially.
param(
    [Parameter(Mandatory)][ValidatePattern('^[A-Z]$')][string]$Slot,
    [Parameter(Mandatory)][ValidateSet('Main7.exe', 'EngineDll7.dll', 'CommonDll7.dll', 'GameDll7.dll')][string]$Binary,
    [Parameter(Mandatory)][string]$Out,
    [Parameter(Mandatory)][string[]]$Names
)
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
if (-not $env:JAVA_HOME) { $env:JAVA_HOME = [Environment]::GetEnvironmentVariable('JAVA_HOME', 'User') }
$proj = Join-Path $root "re\ghidra-slots\$Slot"
if (-not (Test-Path "$proj\DH2.gpr")) {
    New-Item -ItemType Directory -Force $proj | Out-Null
    Copy-Item "$root\re\ghidra\DH2.gpr" $proj
    Copy-Item "$root\re\ghidra\DH2.rep" $proj -Recurse
}
$outAbs = if ([IO.Path]::IsPathRooted($Out)) { $Out } else { Join-Path $root $Out }
& "$root\tools\ghidra_12.1.4_PUBLIC\support\analyzeHeadless.bat" $proj DH2 `
    -process $Binary -noanalysis -readOnly `
    -scriptPath "$root\re\scripts" -postScript DecompileByName.java $outAbs @Names 2>&1 |
    Select-String 'DecompileByName|ERROR' | ForEach-Object Line
