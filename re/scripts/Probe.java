// Multi-purpose probe: xrefs, callers, vtables, strings, decompile-by-address.
// Headless usage (same invocation as DecompileByName.java):
//   analyzeHeadless <proj> DH2 -process EngineDll7.dll -noanalysis -readOnly
//     -scriptPath re/scripts -postScript Probe.java <outFile> <cmd> [cmd ...]
// Commands (addresses are RVAs in hex, e.g. 0xb20d0; names are Ghidra names, e.g. cNetServer::Create
// or a mangled/plain symbol name):
//   xref:<name|rva>        list every reference to the symbol/address (from-address, containing function, ref type)
//   callers:<name|rva>     like xref but de-duplicated to calling functions (name + entry RVA)
//   dec:<name|rva>         decompile the function containing the address / named function
//   deccallers:<name|rva>  decompile every function that references the target
//   vt:<rva>:<n>           dump n dwords at rva as pointers, naming the function each points to
//   str:<substring>        find defined strings containing substring (case-sensitive), print RVA + xref functions
//   bytes:<rva>:<n>        hex dump n bytes
// @category DH2

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;

import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class Probe extends GhidraScript {
    private PrintWriter out;
    private DecompInterface decomp;
    private long base;

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            printerr("args: <outFile> <cmd...>");
            return;
        }
        base = currentProgram.getImageBase().getOffset();
        decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        try (PrintWriter pw = new PrintWriter(new FileWriter(args[0], true))) {
            out = pw;
            for (int i = 1; i < args.length; i++) {
                String c = args[i];
                out.println("// ######## " + c);
                try {
                    if (c.startsWith("xref:")) xref(c.substring(5), false);
                    else if (c.startsWith("callers:")) xref(c.substring(8), true);
                    else if (c.startsWith("dec:")) decTarget(c.substring(4));
                    else if (c.startsWith("deccallers:")) decCallers(c.substring(11));
                    else if (c.startsWith("vt:")) vt(c.substring(3));
                    else if (c.startsWith("str:")) str(c.substring(4));
                    else if (c.startsWith("bytes:")) bytes(c.substring(6));
                    else if (c.startsWith("owner:")) owner(c.substring(6), false);
                    else if (c.startsWith("decowner:")) owner(c.substring(9), true);
                    else if (c.startsWith("funcs:")) funcs(c.substring(6));
                    else out.println("// unknown command");
                } catch (Exception e) {
                    out.println("// error: " + e);
                }
                out.println();
            }
        }
        println("Probe: done -> " + args[0]);
    }

    private String rva(Address a) {
        return String.format("0x%x", a.getOffset() - base);
    }

    private List<Address> resolve(String s) {
        List<Address> res = new ArrayList<>();
        if (s.startsWith("0x")) {
            res.add(toAddr(base + Long.parseLong(s.substring(2), 16)));
            return res;
        }
        // function by full or short name
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
        while (it.hasNext()) {
            Function f = it.next();
            if (f.getName(true).equals(s) || f.getName().equals(s)) res.add(f.getEntryPoint());
        }
        if (!res.isEmpty()) return res;
        SymbolIterator si = currentProgram.getSymbolTable().getAllSymbols(true);
        while (si.hasNext()) {
            Symbol sym = si.next();
            if (sym.getName(true).equals(s) || sym.getName().equals(s)) res.add(sym.getAddress());
        }
        return res;
    }

    private String fname(Address a) {
        Function f = getFunctionContaining(a);
        if (f == null) return "<no func>";
        return f.getName(true) + "@" + rva(f.getEntryPoint());
    }

    private void xref(String target, boolean dedupe) {
        for (Address t : resolve(target)) {
            out.println("// target " + target + " @ " + rva(t));
            Map<String, Integer> seen = new LinkedHashMap<>();
            ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(t);
            while (ri.hasNext()) {
                Reference r = ri.next();
                Address from = r.getFromAddress();
                if (dedupe) {
                    String k = fname(from);
                    seen.merge(k, 1, Integer::sum);
                } else {
                    out.println("//   from " + rva(from) + "  in " + fname(from) + "  (" + r.getReferenceType() + ")");
                }
            }
            if (dedupe) for (Map.Entry<String, Integer> e : seen.entrySet())
                out.println("//   caller " + e.getKey() + "  x" + e.getValue());
        }
    }

    private void decFunc(Function f) {
        if (f == null) { out.println("// no function"); return; }
        out.printf("// ===== %s  RVA %s  (%s)%n", f.getName(true), rva(f.getEntryPoint()), f.getSignature());
        DecompileResults r = decomp.decompileFunction(f, 90, monitor);
        out.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// decompile failed: " + r.getErrorMessage());
    }

    private void decTarget(String target) {
        List<Address> as = resolve(target);
        if (as.isEmpty()) out.println("// unresolved " + target);
        for (Address a : as) {
            Function f = getFunctionContaining(a);
            if (f == null) f = getFunctionAt(a);
            decFunc(f);
        }
    }

    private void decCallers(String target) {
        for (Address t : resolve(target)) {
            Map<Address, Function> fs = new LinkedHashMap<>();
            ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(t);
            while (ri.hasNext()) {
                Function f = getFunctionContaining(ri.next().getFromAddress());
                if (f != null) fs.put(f.getEntryPoint(), f);
            }
            out.println("// callers of " + target + ": " + fs.size());
            for (Function f : fs.values()) decFunc(f);
        }
    }

    private void vt(String spec) throws Exception {
        String[] p = spec.split(":");
        Address a = toAddr(base + Long.parseLong(p[0].substring(2), 16));
        int n = Integer.parseInt(p[1]);
        Memory m = currentProgram.getMemory();
        for (int i = 0; i < n; i++) {
            Address slot = a.add(i * 4L);
            long v = m.getInt(slot) & 0xffffffffL;
            Address tgt = toAddr(v);
            Function f = getFunctionAt(tgt);
            Symbol s = getSymbolAt(tgt);
            out.printf("//   +0x%02x  %s -> 0x%x  %s%n", i * 4, rva(slot), v - base,
                    f != null ? f.getName(true) : (s != null ? s.getName(true) : "?"));
        }
    }

    private void str(String sub) {
        DataIterator di = currentProgram.getListing().getDefinedData(true);
        while (di.hasNext()) {
            Data d = di.next();
            if (!d.hasStringValue()) continue;
            Object v = d.getValue();
            if (v == null || !v.toString().contains(sub)) continue;
            StringBuilder sb = new StringBuilder();
            ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(d.getAddress());
            while (ri.hasNext()) sb.append(" ").append(fname(ri.next().getFromAddress()));
            out.println("// str " + rva(d.getAddress()) + " \"" + v.toString().replace("\n", "\\n") + "\"  refs:" + sb);
        }
    }

    // For "__func_name"-style strings referenced only from MSVC catch funclets ("Catch@..."), the owning
    // function is the nearest preceding function that is not itself a funclet.
    private Function ownerOf(Address from) {
        Function f = getFunctionContaining(from);
        if (f == null) f = getFunctionBefore(from);
        while (f != null && f.getName().startsWith("Catch")) f = getFunctionBefore(f.getEntryPoint());
        return f;
    }

    private void owner(String sub, boolean dec) {
        DataIterator di = currentProgram.getListing().getDefinedData(true);
        Map<Address, String> done = new LinkedHashMap<>();
        while (di.hasNext()) {
            Data d = di.next();
            if (!d.hasStringValue()) continue;
            Object v = d.getValue();
            if (v == null || !v.toString().contains(sub)) continue;
            ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(d.getAddress());
            Address best = null;
            while (ri.hasNext()) {
                Address from = ri.next().getFromAddress();
                Function f = getFunctionContaining(from);
                if (f == null || !f.getName().startsWith("Catch")) continue;
                Function o = ownerOf(from);
                if (o != null && (best == null || o.getEntryPoint().compareTo(best) < 0)) best = o.getEntryPoint();
            }
            if (best == null) { out.println("// " + v + "  -> <no owner>"); continue; }
            out.println("// " + v + "  -> " + getFunctionAt(best).getName(true) + "@" + rva(best));
            done.put(best, v.toString());
        }
        if (dec) for (Map.Entry<Address, String> e : done.entrySet()) {
            out.println("// ---- " + e.getValue());
            decFunc(getFunctionAt(e.getKey()));
        }
    }

    private void funcs(String spec) {
        String[] p = spec.split(":");
        Address a = toAddr(base + Long.parseLong(p[0].substring(2), 16));
        Address b = toAddr(base + Long.parseLong(p[1].substring(2), 16));
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(a, true);
        while (it.hasNext()) {
            Function f = it.next();
            if (f.getEntryPoint().compareTo(b) > 0) break;
            out.printf("//   %s  %s  size %d%n", rva(f.getEntryPoint()), f.getName(true), f.getBody().getNumAddresses());
        }
    }

    private void bytes(String spec) throws Exception {
        String[] p = spec.split(":");
        Address a = toAddr(base + Long.parseLong(p[0].substring(2), 16));
        int n = Integer.parseInt(p[1]);
        byte[] b = new byte[n];
        currentProgram.getMemory().getBytes(a, b);
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < n; i++) {
            if (i % 16 == 0) sb.append(String.format("%n//   %s ", rva(a.add(i))));
            sb.append(String.format(" %02x", b[i] & 0xff));
        }
        out.println(sb);
    }
}
