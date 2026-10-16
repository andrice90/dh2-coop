// Lists every reference (read/write/call) to the given RVAs, with the containing function.
// Usage: -postScript DataRefs.java <outFile> <rvaHex> [<rvaHex> ...]
// @category DH2

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

import java.io.FileWriter;
import java.io.PrintWriter;

public class DataRefs extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] a = getScriptArgs();
        long base = currentProgram.getImageBase().getOffset();
        try (PrintWriter out = new PrintWriter(new FileWriter(a[0], true))) {
            for (int i = 1; i < a.length; i++) {
                long rva = Long.parseLong(a[i].replace("0x", ""), 16);
                Address at = toAddr(base + rva);
                out.printf("TARGET RVA 0x%x%n", rva);
                ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(at);
                while (ri.hasNext()) {
                    Reference r = ri.next();
                    Address from = r.getFromAddress();
                    Function f = getFunctionContaining(from);
                    out.printf("    %s from RVA 0x%x in %s%n", r.getReferenceType(), from.getOffset() - base,
                            f == null ? "(no function)" : f.getName(true) + " @RVA 0x" + Long.toHexString(f.getEntryPoint().getOffset() - base));
                }
            }
        }
        println("DataRefs done");
    }
}
