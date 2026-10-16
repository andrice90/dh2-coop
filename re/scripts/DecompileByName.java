// Decompiles every function whose name (or demangled name) contains one of the given substrings.
// Headless usage (see re/scripts/decomp.ps1):
//   analyzeHeadless <proj> DH2 -process EngineDll7.dll -noanalysis -readOnly
//     -scriptPath re/scripts -postScript DecompileByName.java <outFile> <substr1> [substr2 ...]
// Prefix a substring with '=' for an exact match.
// @category DH2

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

import java.io.FileWriter;
import java.io.PrintWriter;

public class DecompileByName extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            printerr("args: <outFile> <substr...>");
            return;
        }
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        long base = currentProgram.getImageBase().getOffset();
        int count = 0;
        try (PrintWriter out = new PrintWriter(new FileWriter(args[0], true))) {
            FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
            while (it.hasNext() && !monitor.isCancelled()) {
                Function f = it.next();
                String name = f.getName(true);
                boolean hit = false;
                for (int i = 1; i < args.length && !hit; i++) {
                    String a = args[i];
                    hit = a.startsWith("=") ? name.equals(a.substring(1)) || f.getName().equals(a.substring(1))
                                            : name.contains(a);
                }
                if (!hit) continue;
                long rva = f.getEntryPoint().getOffset() - base;
                out.printf("// ===== %s  %s  RVA 0x%x  (%s)%n", currentProgram.getName(), name, rva, f.getSignature());
                DecompileResults r = decomp.decompileFunction(f, 60, monitor);
                out.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// decompile failed: " + r.getErrorMessage());
                count++;
            }
        }
        println("DecompileByName: " + count + " functions -> " + args[0]);
    }
}
