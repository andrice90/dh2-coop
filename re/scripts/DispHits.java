// Lists instructions whose memory operand displacement equals one of the given hex offsets.
// Usage: -postScript DispHits.java <outFile> <dispHex> [<dispHex> ...]
// @category DH2

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.lang.OperandType;
import ghidra.program.model.scalar.Scalar;

import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.LinkedHashMap;
import java.util.Map;

public class DispHits extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] a = getScriptArgs();
        long[] want = new long[a.length - 1];
        for (int i = 1; i < a.length; i++) want[i - 1] = Long.parseLong(a[i].replace("0x", ""), 16);
        long base = currentProgram.getImageBase().getOffset();
        Map<String, Integer> counts = new LinkedHashMap<>();
        try (PrintWriter out = new PrintWriter(new FileWriter(a[0], true))) {
            InstructionIterator it = currentProgram.getListing().getInstructions(true);
            while (it.hasNext() && !monitor.isCancelled()) {
                Instruction ins = it.next();
                int nops = ins.getNumOperands();
                for (int op = 0; op < nops; op++) {
                    int typ = ins.getOperandType(op);
                    if ((typ & OperandType.DYNAMIC) == 0 && (typ & OperandType.ADDRESS) == 0
                            && (typ & OperandType.SCALAR) == 0 && (typ & OperandType.REGISTER) == 0) {
                        // still scan scalars below
                    }
                    Object[] objs = ins.getOpObjects(op);
                    for (Object o : objs) {
                        if (!(o instanceof Scalar)) continue;
                        long v = ((Scalar) o).getUnsignedValue();
                        for (long w : want) {
                            if (v != w) continue;
                            Address from = ins.getAddress();
                            Function f = getFunctionContaining(from);
                            String fn = f == null ? "(no function)"
                                    : f.getName(true) + " @0x" + Long.toHexString(f.getEntryPoint().getOffset() - base);
                            String line = String.format("disp 0x%x  %s  RVA 0x%x  in %s", w, ins, from.getOffset() - base, fn);
                            out.println(line);
                            counts.merge(fn + " disp 0x" + Long.toHexString(w), 1, Integer::sum);
                        }
                    }
                }
            }
            out.println("// ---- counts ----");
            for (Map.Entry<String, Integer> e : counts.entrySet()) {
                out.println("// " + e.getValue() + "  " + e.getKey());
            }
        }
        println("DispHits: done");
    }
}
