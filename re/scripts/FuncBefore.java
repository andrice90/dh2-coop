// For each RVA, decompiles the nearest function that starts before it and is not an EH funclet (Catch@/Unwind@).
// MSVC places a function's catch funclets right after its body, so a profiler-label string referenced only from a
// Catch@ block leads to its parent this way.
// Usage: -postScript FuncBefore.java <outFile> <rvaHex> [<rvaHex> ...]
// @category DH2

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

import java.io.FileWriter;

public class FuncBefore extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        long base = currentProgram.getImageBase().getOffset();
        DecompInterface ifc = new DecompInterface();
        ifc.openProgram(currentProgram);
        try (FileWriter out = new FileWriter(args[0], true)) {
            for (int i = 1; i < args.length; ++i) {
                Address at = toAddr(base + Long.parseLong(args[i], 16));
                FunctionIterator it = currentProgram.getFunctionManager().getFunctions(at, false);
                Function pick = null;
                while (it.hasNext()) {
                    Function f = it.next();
                    String n = f.getName();
                    if (n.startsWith("Catch") || n.startsWith("Unwind")) continue;
                    pick = f;
                    break;
                }
                if (pick == null) {
                    out.write("// no function before RVA 0x" + args[i] + "\n");
                    continue;
                }
                long rva = pick.getEntryPoint().getOffset() - base;
                out.write("// ---- before RVA 0x" + args[i] + ": " + pick.getName() + " @RVA 0x" + Long.toHexString(rva) + "\n");
                DecompileResults r = ifc.decompileFunction(pick, 60, monitor);
                out.write(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// decompile failed\n");
                out.write("\n");
            }
        }
        println("FuncBefore: done");
    }
}
