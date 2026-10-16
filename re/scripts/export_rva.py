"""Dump PE exports as 'RVA<TAB>ordinal<TAB>mangled<TAB>demangled' using pefile + MSVC undname.exe.
Usage: py -I export_rva.py <dll> <out.tsv>
"""
import sys, subprocess, pefile, glob, os, tempfile

def find_undname():
    pats = glob.glob(r"C:\Program Files (x86)\Microsoft Visual Studio\*\*\VC\Tools\MSVC\*\bin\Hostx86\x86\undname.exe")
    return pats[0] if pats else None

def main():
    dll, out = sys.argv[1], sys.argv[2]
    pe = pefile.PE(dll, fast_load=True)
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXPORT']])
    rows = []
    for s in pe.DIRECTORY_ENTRY_EXPORT.symbols:
        name = s.name.decode() if s.name else f"#ord{s.ordinal}"
        rows.append((s.address, s.ordinal, name))
    und = find_undname()
    dem = {}
    if und:
        names = [r[2] for r in rows if r[2].startswith('?')]
        with tempfile.NamedTemporaryFile('w', delete=False, suffix='.txt') as f:
            f.write("\n".join(names) + "\n"); tmp = f.name
        res = subprocess.run([und, tmp], capture_output=True, text=True)
        os.unlink(tmp)
        lines = [l for l in res.stdout.splitlines()]
        # undname on a file prints undecorated names one per line in the same order (after a banner)
        body = lines[-len(names):] if len(lines) >= len(names) else []
        for m, d in zip(names, body):
            dem[m] = d.strip()
    with open(out, 'w', encoding='utf-8') as f:
        for rva, o, n in sorted(rows):
            f.write(f"0x{rva:08x}\t{o}\t{n}\t{dem.get(n, n)}\n")
    print(f"{len(rows)} exports -> {out}")

if __name__ == '__main__':
    main()
