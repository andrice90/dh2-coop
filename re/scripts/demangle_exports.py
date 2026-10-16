"""Demangle MSVC export lists with the Windows dbghelp UnDecorateSymbolName API.

Reads re/exports/<dll>.exports.txt (one mangled name per line, CRLF tolerated,
blank lines skipped) and writes re/exports/<dll>.demangled.txt as
"mangled<TAB>demangled" lines. Flags = 0 (UNDNAME_COMPLETE).

If dbghelp cannot demangle a name, the mangled name is written unchanged and
the failure is counted in the summary. Names that are not decorated (for example
InitGameDll) come back unchanged from dbghelp, which is expected.

Usage:  py re\\scripts\\demangle_exports.py [DllName ...]
        (default: EngineDll7.dll CommonDll7.dll GameDll7.dll)
"""

import ctypes
import sys
from ctypes import wintypes
from pathlib import Path

DEFAULT_DLLS = ["EngineDll7.dll", "CommonDll7.dll", "GameDll7.dll"]
BUF_SIZE = 16384
UNDNAME_COMPLETE = 0x0000

PROJECT_ROOT = Path(__file__).resolve().parents[2]
EXPORTS_DIR = PROJECT_ROOT / "re" / "exports"


def load_undecorate():
    dbghelp = ctypes.WinDLL("dbghelp.dll")
    fn = dbghelp.UnDecorateSymbolName
    fn.argtypes = [ctypes.c_char_p, ctypes.c_char_p, wintypes.DWORD, wintypes.DWORD]
    fn.restype = wintypes.DWORD
    return fn


def demangle(undecorate, mangled, buf):
    ok = undecorate(mangled.encode("ascii"), buf, BUF_SIZE, UNDNAME_COMPLETE)
    if ok == 0:
        return None
    return buf.value.decode("ascii", errors="replace")


def process(undecorate, dll_name):
    src = EXPORTS_DIR / f"{dll_name}.exports.txt"
    dst = EXPORTS_DIR / f"{dll_name}.demangled.txt"

    names = []
    with src.open("r", encoding="ascii", errors="strict", newline="") as fh:
        for raw in fh:
            name = raw.strip()
            if name:
                names.append(name)

    buf = ctypes.create_string_buffer(BUF_SIZE)
    failed = 0
    undecorated_as_is = 0
    with dst.open("w", encoding="ascii", errors="strict", newline="\n") as out:
        for mangled in names:
            result = demangle(undecorate, mangled, buf)
            if result is None:
                failed += 1
                result = mangled
            elif result == mangled:
                undecorated_as_is += 1
            out.write(f"{mangled}\t{result}\n")

    print(
        f"{dll_name}: {len(names)} exports -> {dst.name} "
        f"(failed={failed}, returned-unchanged={undecorated_as_is})"
    )
    return len(names), failed


def main(argv):
    dlls = argv[1:] or DEFAULT_DLLS
    undecorate = load_undecorate()
    total = 0
    total_failed = 0
    for dll in dlls:
        count, failed = process(undecorate, dll)
        total += count
        total_failed += failed
    print(f"TOTAL: {total} exports, {total_failed} demangle failures")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
