// Finds defined strings containing any of the given substrings (case-sensitive; prefix '=' = exact match)
// and prints each string's address (RVA), plus every reference to it with the containing function and RVA.
// Usage: -postScript XrefStrings.java <outFile> <substr...>
// @category DH2

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.StringDataInstance;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

import java.io.FileWriter;
import java.io.PrintWriter;

public class XrefStrings extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] a = getScriptArgs();
        long base = currentProgram.getImageBase().getOffset();
        int hits = 0;
        try (PrintWriter out = new PrintWriter(new FileWriter(a[0], true))) {
            DataIterator it = currentProgram.getListing().getDefinedData(true);
            while (it.hasNext() && !monitor.isCancelled()) {
                Data d = it.next();
                Object v = d.getValue();
                if (!(v instanceof String)) continue;
                String s = (String) v;
                boolean hit = false;
                for (int i = 1; i < a.length && !hit; i++) {
                    hit = a[i].startsWith("=") ? s.equals(a[i].substring(1)) : s.contains(a[i]);
                }
                if (!hit) continue;
                hits++;
                String shown = s.replace("\n", "\\n").replace("\r", "\\r");
                out.printf("STR RVA 0x%x  \"%s\"%n", d.getAddress().getOffset() - base, shown);
                ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(d.getAddress());
                while (ri.hasNext()) {
                    Reference r = ri.next();
                    Address from = r.getFromAddress();
                    Function f = getFunctionContaining(from);
                    out.printf("    ref from RVA 0x%x in %s%n", from.getOffset() - base,
                            f == null ? "(no function)" : f.getName(true) + " @RVA 0x" + Long.toHexString(f.getEntryPoint().getOffset() - base));
                }
            }
        }
        println("XrefStrings: " + hits + " strings");
    }
}
