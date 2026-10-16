# Run re\scripts\Probe.java against one binary in a Ghidra slot. Output is appended to -Out.
# Usage: .\re\scripts\probe.ps1 -Slot A -Binary EngineDll7.dll -Out re\notes\net-probe.c -Cmds 'callers:cNetServer::Create','str:[SERVER]','vt:0x412e78:8'
# Commands: xref:, callers:, dec:, deccallers:, vt:<rva>:<n>, str:<substr>, bytes:<rva>:<n>  (see Probe.java header).
param(
    [Parameter(Mandatory)][ValidatePattern('^[A-Z]$')][string]$Slot,
    [Parameter(Mandatory)][ValidateSet('Main7.exe', 'EngineDll7.dll', 'CommonDll7.dll', 'GameDll7.dll')][string]$Binary,
    [Parameter(Mandatory)][string]$Out,
    [Parameter(Mandatory)][string[]]$Cmds
)
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$env:JAVA_HOME = [Environment]::GetEnvironmentVariable('JAVA_HOME', 'User')
$proj = Join-Path $root "re\ghidra-slots\$Slot"
if (-not (Test-Path "$proj\DH2.gpr")) {
    New-Item -ItemType Directory -Force $proj | Out-Null
    Copy-Item "$root\re\ghidra\DH2.gpr" $proj
    Copy-Item "$root\re\ghidra\DH2.rep" $proj -Recurse
}
$outAbs = if ([IO.Path]::IsPathRooted($Out)) { $Out } else { Join-Path $root $Out }
& "$root\tools\ghidra_12.1.4_PUBLIC\support\analyzeHeadless.bat" $proj DH2 `
    -process $Binary -noanalysis -readOnly `
    -scriptPath "$root\re\scripts" -postScript Probe.java $outAbs @Cmds 2>&1 |
    Select-String 'Probe|ERROR|Exception' | ForEach-Object Line
