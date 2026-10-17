"""Print RVAs of ASCII strings in a PE that contain any of the needles."""
import sys
from pathlib import Path

import pefile

NEEDLES = [
    b"obj_target",
    b"TARGET_CAN",
    b"mod_shootstumps",
    b"generic/cheat",
    b"cObjTarget",
    b"msn1bottle",
    b"msn1trunk",
    b"msn1stump",
    b"shootstump",
    b"UpdateMissionStatus",
    b"SetTaskAnimalToShoot",
    b"ANIMAL_KILL",
]


def main() -> None:
    path = Path(sys.argv[1])
    pe = pefile.PE(str(path), fast_load=True)
    data = path.read_bytes()
    base = pe.OPTIONAL_HEADER.ImageBase
    # map file offset -> RVA via sections
    def rva_of(off: int):
        for s in pe.sections:
            start = s.PointerToRawData
            end = start + s.SizeOfRawData
            if start <= off < end:
                return s.VirtualAddress + (off - start)
        return None

    i = 0
    hits = 0
    while i < len(data):
        if 32 <= data[i] < 127:
            j = i
            while j < len(data) and 32 <= data[j] < 127:
                j += 1
            if j - i >= 4:
                blob = data[i:j]
                low = blob.lower()
                if any(n.lower() in low for n in NEEDLES):
                    rva = rva_of(i)
                    shown = blob.decode("ascii", "replace")[:180]
                    print(f"RVA 0x{rva:x}  {shown}" if rva is not None else f"file 0x{i:x}  {shown}")
                    hits += 1
            i = j
        else:
            i += 1
    print(f"{hits} hits in {path.name} imagebase 0x{base:x}")


if __name__ == "__main__":
    main()
