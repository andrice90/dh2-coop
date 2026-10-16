// Decompiles the functions at the given RVAs (hex). Creates a function in memory if none is defined there
// (read-only project: nothing is saved). Output is appended.
// Usage: -postScript DecompileAt.java <outFile> <rvaHex> [<rvaHex> ...]
// @category DH2

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

import java.io.FileWriter;
import java.io.PrintWriter;

public class DecompileAt extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] a = getScriptArgs();
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        long base = currentProgram.getImageBase().getOffset();
        try (PrintWriter out = new PrintWriter(new FileWriter(a[0], true))) {
            for (int i = 1; i < a.length; i++) {
                if (a[i].contains("-")) {
                    // range "start-end" (hex RVAs): decompile every function whose entry lies in the range
                    String[] pr = a[i].split("-");
                    long lo = Long.parseLong(pr[0].replace("0x", ""), 16);
                    long hi = Long.parseLong(pr[1].replace("0x", ""), 16);
                    ghidra.program.model.listing.FunctionIterator fit = currentProgram.getFunctionManager().getFunctions(toAddr(base + lo), true);
                    while (fit.hasNext()) {
                        Function g = fit.next();
                        long grva = g.getEntryPoint().getOffset() - base;
                        if (grva > hi) break;
                        out.printf("// ===== %s  %s  RVA 0x%x  (%s)%n", currentProgram.getName(), g.getName(true), grva, g.getSignature());
                        DecompileResults rr = decomp.decompileFunction(g, 60, monitor);
                        out.println(rr.decompileCompleted() ? rr.getDecompiledFunction().getC() : "// decompile failed: " + rr.getErrorMessage());
                    }
                    continue;
                }
                long rva = Long.parseLong(a[i].replace("0x", ""), 16);
                Address at = toAddr(base + rva);
                Function f = getFunctionAt(at);
                if (f == null) f = getFunctionContaining(at);
                if (f == null) {
                    try {
                        disassemble(at);
                        f = createFunction(at, null);
                    } catch (Exception e) {
                        f = null;
                    }
                }
                if (f == null) {
                    out.printf("// ===== %s RVA 0x%x : could not create function%n%n", currentProgram.getName(), rva);
                    continue;
                }
                long frva = f.getEntryPoint().getOffset() - base;
                out.printf("// ===== %s  %s  RVA 0x%x  (requested 0x%x)  (%s)%n", currentProgram.getName(), f.getName(true), frva, rva, f.getSignature());
                DecompileResults r = decomp.decompileFunction(f, 60, monitor);
                out.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// decompile failed: " + r.getErrorMessage());
            }
        }
        println("DecompileAt done");
    }
}
