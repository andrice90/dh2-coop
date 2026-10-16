// Dumps N consecutive 32-bit pointers starting at an RVA (e.g. a vtable) and resolves each to a function name.
// Usage: -postScript DumpPtrs.java <outFile> <rvaHex> <count> [<rvaHex> <count> ...]
// @category DH2

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Symbol;

import java.io.FileWriter;
import java.io.PrintWriter;

public class DumpPtrs extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] a = getScriptArgs();
        long base = currentProgram.getImageBase().getOffset();
        try (PrintWriter out = new PrintWriter(new FileWriter(a[0], true))) {
            for (int k = 1; k + 1 < a.length; k += 2) {
                long rva = Long.decode(a[k].startsWith("0x") ? a[k] : "0x" + a[k]);
                int n = Integer.parseInt(a[k + 1]);
                out.printf("// ---- %s  table at RVA 0x%x (%d entries)%n", currentProgram.getName(), rva, n);
                for (int i = 0; i < n; i++) {
                    Address at = toAddr(base + rva + i * 4L);
                    long v;
                    try {
                        v = getInt(at) & 0xffffffffL;
                    } catch (Exception e) {
                        out.printf("[%2d] +0x%02x  <unreadable>%n", i, i * 4);
                        continue;
                    }
                    Address t = toAddr(v);
                    String nm = "?";
                    if (t != null) {
                        Function f = getFunctionAt(t);
                        if (f == null) f = getFunctionContaining(t);
                        if (f != null) nm = f.getName(true);
                        else {
                            Symbol s = getSymbolAt(t);
                            nm = s != null ? s.getName(true) : "(no function)";
                        }
                    }
                    out.printf("[%2d] +0x%02x  0x%08x (RVA 0x%x)  %s%n", i, i * 4, v, v - base, nm);
                }
            }
        }
        println("DumpPtrs done");
    }
}
