"""Dump printable ASCII strings (length >= 5) from the four game binaries.

Output: re/notes/strings/<file>.txt, one line per string:
    <file offset hex>  <rva hex>  <section>  <string>
Files are read only. The game is not run.

Usage:  py re\\scripts\\dump_strings.py
"""

import re
import sys
from pathlib import Path

import pefile

PROJECT_ROOT = Path(__file__).resolve().parents[2]
GAME = PROJECT_ROOT / "game"
OUT_DIR = PROJECT_ROOT / "re" / "notes" / "strings"

TARGETS = [
    GAME / "Bin" / "Main7.exe",
    GAME / "Bin" / "EngineDll7.dll",
    GAME / "Bin" / "CommonDll7.dll",
    GAME / "Data" / "GameDll7.dll",
]

MIN_LEN = 5
ASCII_RUN = re.compile(rb"[\x20-\x7e]{%d,}" % MIN_LEN)


def sections_of(pe):
    out = []
    for s in pe.sections:
        name = s.Name.rstrip(b"\x00").decode("ascii", errors="replace")
        out.append((s.PointerToRawData, s.PointerToRawData + s.SizeOfRawData,
                    s.VirtualAddress, name))
    return out


def locate(sections, off):
    for raw_start, raw_end, va, name in sections:
        if raw_start <= off < raw_end:
            return va + (off - raw_start), name
    return None, "(header)"


def dump(path):
    data = path.read_bytes()
    pe = pefile.PE(data=data, fast_load=True)
    sections = sections_of(pe)
    pe.close()

    out_path = OUT_DIR / f"{path.name}.txt"
    count = 0
    by_section = {}
    with out_path.open("w", encoding="ascii", newline="\n") as fh:
        fh.write(f"# {path.name}  size={len(data)}  min_len={MIN_LEN}  ascii only\n")
        fh.write("# offset  rva  section  string\n")
        for m in ASCII_RUN.finditer(data):
            off = m.start()
            rva, sec = locate(sections, off)
            rva_s = f"{rva:08x}" if rva is not None else "--------"
            text = m.group().decode("ascii")
            fh.write(f"{off:08x}  {rva_s}  {sec:<8}  {text}\n")
            count += 1
            by_section[sec] = by_section.get(sec, 0) + 1
    return out_path, count, by_section


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for target in TARGETS:
        if not target.is_file():
            print(f"MISSING {target}", file=sys.stderr)
            return 1
        out_path, count, by_section = dump(target)
        summary = ", ".join(f"{k}={v}" for k, v in sorted(by_section.items(), key=lambda kv: -kv[1]))
        print(f"{target.name}: {count} strings -> {out_path.relative_to(PROJECT_ROOT)} [{summary}]")
    return 0


if __name__ == "__main__":
    sys.exit(main())
