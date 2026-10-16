// Finds every function that contains a CALL whose target (direct, thunk or external import) has a name containing
// one of the given substrings, then decompiles each such caller and appends it to the output file.
// Usage: -postScript CallersDecomp.java <outFile> <calleeSubstr> [<calleeSubstr> ...]
// @category DH2

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.ExternalLocation;
import ghidra.program.model.symbol.ExternalReference;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.Symbol;

import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.LinkedHashSet;
import java.util.Set;

public class CallersDecomp extends GhidraScript {
    private String[] pats;

    private boolean match(String n) {
        if (n == null) return false;
        for (int i = 1; i < pats.length; i++) if (n.contains(pats[i])) return true;
        return false;
    }

    private boolean targetMatches(Reference r) {
        if (r.isExternalReference()) {
            try {
                ExternalLocation el = ((ExternalReference) r).getExternalLocation();
                String l = el.getLabel();
                String o = el.getOriginalImportedName();
                if (match(l) || match(o)) return true;
            } catch (Exception e) { }
        }
        Address to = r.getToAddress();
        if (to == null) return false;
        Function tf = getFunctionAt(to);
        if (tf != null) {
            if (match(tf.getName(true)) || match(tf.getName())) return true;
            Function th = tf.getThunkedFunction(true);
            if (th != null && (match(th.getName(true)) || match(th.getName()))) return true;
        }
        Symbol s = getSymbolAt(to);
        if (s != null && (match(s.getName(true)) || match(s.getName()))) return true;
        return false;
    }

    @Override
    protected void run() throws Exception {
        pats = getScriptArgs();
        long base = currentProgram.getImageBase().getOffset();
        Set<Function> callers = new LinkedHashSet<>();
        FunctionIterator fi = currentProgram.getFunctionManager().getFunctions(true);
        while (fi.hasNext() && !monitor.isCancelled()) {
            Function f = fi.next();
            InstructionIterator it = currentProgram.getListing().getInstructions(f.getBody(), true);
            while (it.hasNext()) {
                Instruction ins = it.next();
                if (!ins.getFlowType().isCall()) continue;
                for (Reference r : ins.getReferencesFrom()) {
                    if (targetMatches(r)) { callers.add(f); break; }
                }
            }
        }
        println("CallersDecomp: " + callers.size() + " callers");
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        try (PrintWriter out = new PrintWriter(new FileWriter(pats[0], true))) {
            for (Function f : callers) {
                long rva = f.getEntryPoint().getOffset() - base;
                out.printf("// ===== %s  %s  RVA 0x%x  (%s)%n", currentProgram.getName(), f.getName(true), rva, f.getSignature());
                DecompileResults r = decomp.decompileFunction(f, 90, monitor);
                out.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// decompile failed: " + r.getErrorMessage());
            }
        }
        println("CallersDecomp done");
    }
}
