// Lists functions whose instructions mention a displacement (substring of the instruction text).
// Usage: -postScript ScanDisp.java <outFile> <dispHex> [<dispHex> ...]
// dispHex is matched as "0x" + hex (lowercase) inside Instruction.toString().
// @category DH2

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;

import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.regex.Pattern;

public class ScanDisp extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] a = getScriptArgs();
        long base = currentProgram.getImageBase().getOffset();
        Pattern[] needles = new Pattern[a.length - 1];
        String[] labels = new String[a.length - 1];
        for (int i = 1; i < a.length; i++) {
            labels[i - 1] = "0x" + a[i].replace("0x", "").toLowerCase();
            needles[i - 1] = Pattern.compile(labels[i - 1] + "(?![0-9a-f])");
        }
        try (PrintWriter out = new PrintWriter(new FileWriter(a[0], true))) {
            FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
            while (it.hasNext() && !monitor.isCancelled()) {
                Function f = it.next();
                int[] hits = new int[needles.length];
                InstructionIterator ii = currentProgram.getListing().getInstructions(f.getBody(), true);
                while (ii.hasNext()) {
                    String s = ii.next().toString().toLowerCase();
                    for (int n = 0; n < needles.length; n++) {
                        if (needles[n].matcher(s).find()) hits[n]++;
                    }
                }
                boolean any = false;
                for (int h : hits) if (h > 0) any = true;
                if (!any) continue;
                long rva = f.getEntryPoint().getOffset() - base;
                StringBuilder sb = new StringBuilder();
                sb.append(String.format("FUNC %s RVA 0x%x", f.getName(true), rva));
                boolean all = true;
                for (int h : hits) if (h == 0) all = false;
                if (!all) continue;
                for (int n = 0; n < needles.length; n++) {
                    sb.append(String.format("  %s=%d", labels[n], hits[n]));
                }
                out.println(sb);
            }
        }
        println("ScanDisp done");
    }
}
