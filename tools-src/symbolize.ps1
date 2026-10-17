# Resolve dh2coop.asi+0xRVA addresses (from dh2coop.log exception lines) to function+offset with the build PDB.
# Usage: .\tools-src\symbolize.ps1 0xb8dbd 0x3226f   (default module: mod\build\dh2coop.asi; the PDB must match)
param(
    [Parameter(Mandatory, ValueFromRemainingArguments)][string[]]$Rva,
    [string]$Module = (Join-Path (Split-Path $PSScriptRoot -Parent) 'mod\build\dh2coop.asi')
)
$src = @'
using System;
using System.Runtime.InteropServices;
public static class Sym {
    [DllImport("dbghelp.dll", SetLastError = true)] public static extern bool SymInitialize(IntPtr h, string path, bool invade);
    [DllImport("dbghelp.dll", SetLastError = true)] public static extern uint SymSetOptions(uint o);
    [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Ansi)]
    public static extern ulong SymLoadModuleEx(IntPtr h, IntPtr f, string image, string mod, ulong b, uint size, IntPtr d, uint flags);
    [DllImport("dbghelp.dll", SetLastError = true)] public static extern bool SymFromAddr(IntPtr h, ulong a, out ulong disp, IntPtr info);
    [DllImport("dbghelp.dll", SetLastError = true)] public static extern bool SymGetLineFromAddr64(IntPtr h, ulong a, out uint disp, ref LINE l);
    [StructLayout(LayoutKind.Sequential)] public struct LINE { public uint SizeOfStruct; public IntPtr Key; public uint LineNumber; public IntPtr FileName; public ulong Address; }
    public static string Resolve(string image, ulong[] rvas) {
        IntPtr h = new IntPtr(0x7777);
        SymSetOptions(0x2 | 0x10);  // UNDNAME | LOAD_LINES
        if (!SymInitialize(h, System.IO.Path.GetDirectoryName(image), false)) return "SymInitialize failed";
        ulong b = SymLoadModuleEx(h, IntPtr.Zero, image, null, 0x10000000, 0, IntPtr.Zero, 0);
        if (b == 0) return "SymLoadModuleEx failed " + Marshal.GetLastWin32Error();
        var sb = new System.Text.StringBuilder();
        foreach (ulong r in rvas) {
            IntPtr info = Marshal.AllocHGlobal(88 + 512);
            Marshal.WriteInt32(info, 0, 88); Marshal.WriteInt32(info, 80, 500);
            ulong disp;
            string name = SymFromAddr(h, b + r, out disp, info) ? Marshal.PtrToStringAnsi(new IntPtr(info.ToInt64() + 84)) + "+0x" + disp.ToString("x") : "?";
            var l = new LINE(); l.SizeOfStruct = (uint)Marshal.SizeOf(typeof(LINE)); uint ld;
            string line = SymGetLineFromAddr64(h, b + r, out ld, ref l) ? Marshal.PtrToStringAnsi(l.FileName) + ":" + l.LineNumber : "";
            sb.AppendLine("0x" + r.ToString("x") + "  " + name + "  " + line);
            Marshal.FreeHGlobal(info);
        }
        return sb.ToString();
    }
}
'@
Add-Type -TypeDefinition $src
$vals = $Rva | ForEach-Object { [Convert]::ToUInt64(($_ -replace '^0x', ''), 16) }
[Sym]::Resolve((Resolve-Path $Module).Path, [uint64[]]$vals)
