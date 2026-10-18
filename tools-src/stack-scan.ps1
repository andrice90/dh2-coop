# Raw stack scan of one thread of a (hung) 32-bit game process: every stack word that points into a loaded
# module, as "[slot] module+offset" (the same view the mod's first-chance logger prints). No debugger needed.
# Only run it on a process you started from your own checkout.
#   .\tools-src\stack-scan.ps1 -ProcessId 2092 -ThreadId 12824 [-Words 400]
param([Parameter(Mandatory)][int]$ProcessId, [Parameter(Mandatory)][int]$ThreadId, [int]$Words = 400)

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class StackScan {
    [StructLayout(LayoutKind.Sequential)] public struct FLOATING_SAVE_AREA { public uint a,b,c,d,e,f,g; [MarshalAs(UnmanagedType.ByValArray, SizeConst=80)] public byte[] reg; public uint h; }
    [StructLayout(LayoutKind.Sequential)] public struct WOW64_CONTEXT {
        public uint ContextFlags, Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
        public FLOATING_SAVE_AREA FloatSave;
        public uint SegGs, SegFs, SegEs, SegDs, Edi, Esi, Ebx, Edx, Ecx, Eax, Ebp, Eip, SegCs, EFlags, Esp, SegSs;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=512)] public byte[] Ext;
    }
    [DllImport("kernel32.dll")] static extern IntPtr OpenThread(uint access, bool inherit, int tid);
    [DllImport("kernel32.dll")] static extern uint SuspendThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern uint ResumeThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool Wow64GetThreadContext(IntPtr h, ref WOW64_CONTEXT ctx);
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll")] static extern bool ReadProcessMemory(IntPtr h, IntPtr addr, byte[] buf, int size, out int read);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("psapi.dll")] static extern bool EnumProcessModulesEx(IntPtr h, IntPtr[] mods, int cb, out int needed, uint filter);
    [DllImport("psapi.dll", CharSet=CharSet.Unicode)] static extern uint GetModuleFileNameEx(IntPtr h, IntPtr mod, StringBuilder sb, int size);
    [StructLayout(LayoutKind.Sequential)] struct MODULEINFO { public IntPtr Base; public uint Size; public IntPtr Entry; }
    [DllImport("psapi.dll")] static extern bool GetModuleInformation(IntPtr h, IntPtr mod, out MODULEINFO mi, int cb);

    public static string Run(int pid, int tid, int words) {
        var sb = new StringBuilder();
        IntPtr hp = OpenProcess(0x1F0FFF, false, pid);
        IntPtr ht = OpenThread(0x1F03FF, false, tid);
        if (hp == IntPtr.Zero || ht == IntPtr.Zero) return "open failed";
        var mods = new List<Tuple<long, long, string>>();
        IntPtr[] arr = new IntPtr[1024]; int need;
        if (EnumProcessModulesEx(hp, arr, arr.Length * IntPtr.Size, out need, 1)) {
            for (int i = 0; i < need / IntPtr.Size; i++) {
                MODULEINFO mi; var name = new StringBuilder(260);
                GetModuleInformation(hp, arr[i], out mi, Marshal.SizeOf(typeof(MODULEINFO)));
                GetModuleFileNameEx(hp, arr[i], name, 260);
                mods.Add(Tuple.Create((long)mi.Base, (long)mi.Base + mi.Size, System.IO.Path.GetFileName(name.ToString())));
            }
        }
        SuspendThread(ht);
        try {
            var ctx = new WOW64_CONTEXT(); ctx.ContextFlags = 0x10007; ctx.FloatSave.reg = new byte[80]; ctx.Ext = new byte[512];
            if (!Wow64GetThreadContext(ht, ref ctx)) return "Wow64GetThreadContext failed " + Marshal.GetLastWin32Error();
            Func<uint, string> desc = v => { foreach (var m in mods) if (v >= m.Item1 && v < m.Item2) return m.Item3 + "+0x" + (v - m.Item1).ToString("x"); return null; };
            sb.AppendLine(String.Format("eip={0} esp=0x{1:x} ebp=0x{2:x} eax=0x{3:x} ecx=0x{4:x} edx=0x{5:x} ebx=0x{6:x} esi=0x{7:x} edi=0x{8:x}", desc(ctx.Eip) ?? ("0x" + ctx.Eip.ToString("x")), ctx.Esp, ctx.Ebp, ctx.Eax, ctx.Ecx, ctx.Edx, ctx.Ebx, ctx.Esi, ctx.Edi));
            byte[] buf = new byte[words * 4]; int read;
            ReadProcessMemory(hp, (IntPtr)ctx.Esp, buf, buf.Length, out read);
            for (int i = 0; i < read / 4; i++) {
                uint v = BitConverter.ToUInt32(buf, i * 4);
                string d = desc(v);
                if (d != null) sb.AppendLine(String.Format("[{0}] {1}", i, d));
            }
        } finally { ResumeThread(ht); CloseHandle(ht); CloseHandle(hp); }
        return sb.ToString();
    }
}
'@

[StackScan]::Run($ProcessId, $ThreadId, $Words)
