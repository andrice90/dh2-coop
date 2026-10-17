// Names functions by their profiler label: MSVC places a function's Catch@/Unwind@ funclets right after its body,
// and those funclets reference the "cClass::Method" string the profiler macro uses. For each RVA (or every caller
// of a target with "callers:<rva>"), prints the containing function, the "::" strings its funclets (and its own
// body) reference, and how many times it calls the target.
// Usage: -postScript FuncLabels.java <outFile> <rvaHex | callers:rvaHex> [...]
// @category DH2

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.StringDataInstance;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

public class FuncLabels extends GhidraScript {
    private long base;

    private String strAt(Address a) {
        Data d = getDataAt(a);
        if (d == null || !d.hasStringValue()) return null;
        Object v = d.getValue();
        return v == null ? null : v.toString();
    }

    private void collect(Function f, Set<String> out) {
        InstructionIterator ii = currentProgram.getListing().getInstructions(f.getBody(), true);
        while (ii.hasNext()) {
            Instruction ins = ii.next();
            for (Reference r : ins.getReferencesFrom()) {
                String s = strAt(r.getToAddress());
                if (s != null && s.contains("::")) out.add(s);
            }
        }
    }

    private String label(Function f) {
        Set<String> names = new LinkedHashSet<>();
        collect(f, names);
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(f.getEntryPoint().add(1), true);
        int guard = 0;
        while (it.hasNext() && guard++ < 12) {
            Function g = it.next();
            String n = g.getName();
            if (!(n.startsWith("Catch") || n.startsWith("Unwind"))) break;
            collect(g, names);
        }
        return names.isEmpty() ? "?" : String.join(" | ", names);
    }

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        base = currentProgram.getImageBase().getOffset();
        try (PrintWriter out = new PrintWriter(new FileWriter(a[0], true))) {
            for (int i = 1; i < a.length; i++) {
                if (a[i].startsWith("callers:")) {
                    long rva = Long.parseLong(a[i].substring(8).replace("0x", ""), 16);
                    Address t = toAddr(base + rva);
                    Map<Function, Integer> callers = new LinkedHashMap<>();
                    ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(t);
                    while (ri.hasNext()) {
                        Reference r = ri.next();
                        Function f = getFunctionContaining(r.getFromAddress());
                        if (f == null) continue;
                        callers.merge(f, 1, Integer::sum);
                    }
                    out.printf("// ==== callers of 0x%x (%d functions)%n", rva, callers.size());
                    for (Map.Entry<Function, Integer> e : callers.entrySet()) {
                        Function f = e.getKey();
                        out.printf("0x%x\tx%d\t%s\t%s%n", f.getEntryPoint().getOffset() - base, e.getValue(), f.getName(), label(f));
                    }
                    continue;
                }
                long rva = Long.parseLong(a[i].replace("0x", ""), 16);
                Function f = getFunctionContaining(toAddr(base + rva));
                if (f == null) {
                    out.printf("0x%x\t<no function>%n", rva);
                    continue;
                }
                out.printf("0x%x\t%s @0x%x\t%s%n", rva, f.getName(), f.getEntryPoint().getOffset() - base, label(f));
            }
        }
        println("FuncLabels: done");
    }
}
