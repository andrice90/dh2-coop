"""Small static-analysis helper for the DH2 DLLs (pefile + capstone).

Usage (py -I re/scripts/petool.py <cmd> ...), DLL = path to EngineDll7.dll / CommonDll7.dll / GameDll7.dll:
  vt   <DLL> <vtable export substring|0xRVA> [max]   dump vtable slots -> export names
  dis  <DLL> <0xRVA|export substring> [maxinsn]      disassemble a function, calls/imports annotated
  xref <DLL> <0xRVA|export substring>                find rel32 call/jmp and absolute dword refs to target
  sym  <DLL> <substring>                             list exports matching substring (demangled)
All addresses printed are RVAs (image base subtracted).
"""
import sys, struct, re, pefile, capstone, subprocess, glob, os, tempfile, functools

_cache = {}

def undname_many(names):
    pats = glob.glob(r"C:\Program Files (x86)\Microsoft Visual Studio\*\*\VC\Tools\MSVC\*\bin\Hostx86\x86\undname.exe")
    if not pats or not names:
        return {n: n for n in names}
    with tempfile.NamedTemporaryFile('w', delete=False, suffix='.txt') as f:
        f.write("\n".join(names) + "\n"); tmp = f.name
    res = subprocess.run([pats[0], tmp], capture_output=True, text=True)
    os.unlink(tmp)
    lines = res.stdout.splitlines()[-len(names):]
    return {m: d.strip() for m, d in zip(names, lines)}


class Img:
    def __init__(self, path):
        self.path = path
        self.pe = pefile.PE(path, fast_load=True)
        self.pe.parse_data_directories(directories=[
            pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXPORT'],
            pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT']])
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.mem = self.pe.get_memory_mapped_image()
        self.exp = {}
        raw = []
        if hasattr(self.pe, 'DIRECTORY_ENTRY_EXPORT'):
            for s in self.pe.DIRECTORY_ENTRY_EXPORT.symbols:
                if s.name:
                    raw.append((s.address, s.name.decode()))
        dem = undname_many([n for _, n in raw if n.startswith('?')])
        self.exp_m = {}
        for a, n in raw:
            self.exp.setdefault(a, dem.get(n, n))
            self.exp_m[a] = n
        self.imp = {}
        if hasattr(self.pe, 'DIRECTORY_ENTRY_IMPORT'):
            names = []
            for e in self.pe.DIRECTORY_ENTRY_IMPORT:
                for i in e.imports:
                    nm = i.name.decode() if i.name else f"ord{i.ordinal}"
                    names.append((i.address - self.base, e.dll.decode(), nm))
            dem = undname_many([n for _, _, n in names if n.startswith('?')])
            for a, d, n in names:
                self.imp[a] = f"[{d}] " + dem.get(n, n)
        self.text = [s for s in self.pe.sections if s.Characteristics & 0x20000000]
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = False

    def is_code(self, rva):
        return any(s.VirtualAddress <= rva < s.VirtualAddress + s.Misc_VirtualSize for s in self.text)

    def dword(self, rva):
        return struct.unpack_from('<I', self.mem, rva)[0]

    def name(self, rva):
        if rva in self.exp:
            return self.exp[rva]
        if rva in self.imp:
            return self.imp[rva]
        return None

    def label(self, rva):
        n = self.name(rva)
        return f"0x{rva:x} {n}" if n else f"0x{rva:x}"

    def resolve(self, arg):
        if arg.lower().startswith('0x'):
            return int(arg, 16)
        hits = [a for a, n in self.exp.items() if arg in n or arg in self.exp_m.get(a, '')]
        if len(hits) != 1:
            for a in hits[:40]:
                print(f"  candidate 0x{a:x} {self.exp[a]}")
            raise SystemExit(f"'{arg}' matched {len(hits)} exports")
        return hits[0]

    def disasm(self, rva, maxinsn=400, stop_at_ret=True):
        out = []
        code = self.mem[rva:rva + maxinsn * 8]
        n = 0
        for ins in self.md.disasm(code, self.base + rva):
            r = ins.address - self.base
            ann = ''
            op = ins.op_str
            m = re.search(r'0x([0-9a-f]{6,8})', op)
            if m:
                t = int(m.group(1), 16) - self.base
                if 0 <= t < len(self.mem):
                    nm = self.name(t)
                    if nm:
                        ann = '  ; ' + nm
                    elif ins.mnemonic in ('call', 'jmp') and self.is_code(t):
                        ann = f'  ; FUN_{t + 0x10000000:08x}' if self.base != 0x10000000 else ''
            out.append(f"{r:08x}  {ins.mnemonic:6} {op}{ann}")
            n += 1
            if stop_at_ret and ins.mnemonic in ('ret', 'retn'):
                break
            if n >= maxinsn:
                break
        return out

    def xrefs(self, target):
        res = []
        tva = target + self.base
        for s in self.text:
            data = self.mem[s.VirtualAddress:s.VirtualAddress + s.Misc_VirtualSize]
            for i in range(len(data) - 5):
                b = data[i]
                if b in (0xE8, 0xE9):
                    rel = struct.unpack_from('<i', data, i + 1)[0]
                    src = s.VirtualAddress + i
                    if src + 5 + rel == target:
                        res.append((src, 'call' if b == 0xE8 else 'jmp'))
        # absolute refs anywhere in the image
        pat = struct.pack('<I', tva)
        start = 0
        while True:
            i = self.mem.find(pat, start)
            if i < 0:
                break
            res.append((i, 'abs'))
            start = i + 1
        return res

    def func_start(self, rva):
        """Heuristic: nearest export at or below rva, else scan back for CC/90 padding."""
        best = None
        for a in self.exp:
            if a <= rva and self.is_code(a) and (best is None or a > best):
                best = a
        i = rva
        while i > 0 and rva - i < 0x4000:
            if self.mem[i - 1] in (0xCC, 0x90) and self.mem[i - 2] in (0xCC, 0x90, 0xC3) :
                break
            i -= 1
        if best is not None and best >= i:
            return best, self.exp[best]
        return i, None


def load(path):
    if path not in _cache:
        _cache[path] = Img(path)
    return _cache[path]


def main():
    cmd, path = sys.argv[1], sys.argv[2]
    img = load(path)
    if cmd == 'vt':
        rva = img.resolve(sys.argv[3])
        mx = int(sys.argv[4]) if len(sys.argv) > 4 else 200
        print(f"vtable 0x{rva:x} {img.name(rva)}")
        for k in range(mx):
            p = img.dword(rva + 4 * k)
            t = p - img.base
            if k > 0 and (rva + 4 * k) in img.exp:
                break
            if not (0 <= t < len(img.mem)) or not img.is_code(t):
                break
            print(f"  [{k:3}] +0x{4*k:03x}  {img.label(t)}")
    elif cmd == 'dis':
        rva = img.resolve(sys.argv[3])
        mx = int(sys.argv[4]) if len(sys.argv) > 4 else 400
        print(f"; {img.label(rva)}")
        print("\n".join(img.disasm(rva, mx)))
    elif cmd == 'xref':
        rva = img.resolve(sys.argv[3])
        for src, kind in img.xrefs(rva):
            fs, fn = img.func_start(src) if kind != 'abs' or img.is_code(src) else (src, None)
            print(f"  {kind:4} from 0x{src:x}  in 0x{fs:x} {fn or ''}")
    elif cmd == 'sym':
        for a in sorted(img.exp):
            if sys.argv[3] in img.exp[a] or sys.argv[3] in img.exp_m[a]:
                print(f"0x{a:08x}  {img.exp[a]}")


if __name__ == '__main__':
    main()
