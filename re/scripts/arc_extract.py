#!/usr/bin/env python3
"""arc_extract.py - list / verify / extract the Chameleon-engine `data.arc` container (DNGH2 build 434).

Format (reverse-engineered from EngineDll7.dll cArchive/cArcFile, confirmed against the retail file):

  * The file is a sequence of 2048-byte sectors.  Each entry starts on a sector boundary and is either a
    single zlib stream (RFC1950, header 78 9C) or stored raw.
  * Footer (24 bytes) at  floor((filesize - 24) / 2048) * 2048  (the last sector that holds data):
        u32 'ARC\\0'  u32 version(0x100)  u32 dir_sector  u32 dir_unc_size  u32 dir_comp_size  u32 entry_count
  * Directory = zlib stream at dir_sector*2048 (dir_comp_size bytes).  Inflated it is `entry_count` records
    followed by zero padding.  Record (28-byte header + name):
        u32 'ARCH'  u32 first_sector  u32 sector_count  u32 unc_size  u32 comp_size  i32 comp_flag  u32 name_len
        char name[name_len]            (NUL terminated, name_len includes the NUL; upper-case, '\\' separators)
    comp_flag: -1 = zlib stream, 0 = stored raw (comp_size == unc_size).  No per-entry CRC exists; the only
    integrity checks are the zlib Adler-32 trailer and the size fields.
  * Entry data lives at first_sector*2048; sector_count == ceil(comp_size/2048); entries are packed back to
    back in directory order (no gaps).
  * `.mtp` entries are a second container ("MltiFile", magic 'MULA'):
        u32 'MULA'  u32 count  count*{u32 size, u32 name_offset}  u32 names_size  char names[names_size]
        (pad to 4)  then file data back to back, each file padded to a 4-byte boundary.

Usage:
  py re/scripts/arc_extract.py --list [--filter GLOB] [--arc PATH]
  py re/scripts/arc_extract.py --verify [--arc PATH]
  py re/scripts/arc_extract.py --out re/extracted [--filter GLOB] [--mtp] [--arc PATH]

Extracted content is gitignored; never commit it.
"""
import argparse
import fnmatch
import mmap
import os
import struct
import sys
import zlib

SECTOR = 0x800
FOOTER_SIZE = 0x18
ARC_MAGIC = 0x00435241        # 'ARC\0'
ARCH_MAGIC = b"ARCH"
MTP_MAGIC = 0x414C554D        # 'MULA'
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(SCRIPT_DIR, "..", ".."))
DEFAULT_ARC = os.path.join(ROOT, "game", "Data", "data.arc")
DEFAULT_OUT = os.path.join(ROOT, "re", "extracted")


class Entry:
    __slots__ = ("index", "sector", "count", "unc", "comp", "flag", "name")

    def __init__(self, index, sector, count, unc, comp, flag, name):
        self.index, self.sector, self.count = index, sector, count
        self.unc, self.comp, self.flag, self.name = unc, comp, flag, name

    @property
    def offset(self):
        return self.sector * SECTOR

    @property
    def stored(self):
        return self.flag == 0


class Arc:
    def __init__(self, path):
        self.path = path
        self.fh = open(path, "rb")
        self.size = os.fstat(self.fh.fileno()).st_size
        self.mm = mmap.mmap(self.fh.fileno(), 0, access=mmap.ACCESS_READ)
        self._read_footer()
        self._read_directory()

    def close(self):
        self.mm.close()
        self.fh.close()

    def _read_footer(self):
        off = (self.size - FOOTER_SIZE) // SECTOR * SECTOR
        f = struct.unpack_from("<6I", self.mm, off)
        if f[0] != ARC_MAGIC:
            raise ValueError("bad ARC footer magic at 0x%x: %08x" % (off, f[0]))
        self.footer_off = off
        self.version, self.dir_sector, self.dir_unc, self.dir_comp, self.count = f[1:]

    def _read_directory(self):
        o = self.dir_sector * SECTOR
        raw = self.mm[o:o + self.dir_comp]
        d = zlib.decompress(raw)
        if len(d) != self.dir_unc:
            raise ValueError("directory size mismatch %d != %d" % (len(d), self.dir_unc))
        ents, pos = [], 0
        for i in range(self.count):
            if d[pos:pos + 4] != ARCH_MAGIC:
                raise ValueError("bad ARCH record %d at dir+0x%x" % (i, pos))
            sec, cnt, unc, comp, flag, nl = struct.unpack_from("<IIIIiI", d, pos + 4)
            name = d[pos + 28:pos + 28 + nl].rstrip(b"\0").decode("latin-1")
            ents.append(Entry(i, sec, cnt, unc, comp, flag, name))
            pos += 28 + nl
        self.entries = ents
        self.dir_used = pos      # rest of inflated directory is zero padding

    def read(self, e, verify=True):
        raw = self.mm[e.offset:e.offset + e.comp]
        if len(raw) != e.comp:
            raise ValueError("truncated entry")
        if e.stored:
            data = bytes(raw)
        else:
            do = zlib.decompressobj()
            data = do.decompress(raw)          # raises on bad data; Adler-32 checked at stream end
            if verify and not do.eof:
                raise ValueError("zlib stream incomplete")
        if verify and len(data) != e.unc:
            raise ValueError("size mismatch: got %d expected %d" % (len(data), e.unc))
        return data


def safe_relpath(name):
    parts = [p for p in name.replace("/", "\\").split("\\") if p not in ("", ".")]
    if any(p == ".." or ":" in p for p in parts):
        raise ValueError("unsafe path in archive: %r" % name)
    return os.path.join(*parts) if parts else None


def unpack_mtp(data):
    """Return [(name, bytes)] from a 'MULA' multi-file blob."""
    magic, count = struct.unpack_from("<II", data, 0)
    if magic != MTP_MAGIC:
        raise ValueError("not a MULA blob")
    tab = struct.unpack_from("<%dI" % (count * 2), data, 8)
    pos = 8 + count * 8
    (names_size,) = struct.unpack_from("<I", data, pos)
    pos += 4
    names = data[pos:pos + names_size]
    pos += names_size
    pos = (pos + 3) & ~3
    out = []
    for i in range(count):
        size, noff = tab[2 * i], tab[2 * i + 1]
        name = names[noff:names.index(b"\0", noff)].decode("latin-1")
        out.append((name, data[pos:pos + size]))
        pos = (pos + size + 3) & ~3
    return out


def cmd_list(arc, flt):
    tot_u = tot_c = 0
    n = 0
    print("#idx   sector   offset      comp       unc  fl  name")
    for e in arc.entries:
        if flt and not fnmatch.fnmatch(e.name.upper(), flt.upper()):
            continue
        n += 1
        tot_u += e.unc
        tot_c += e.comp
        print("%5d %8d 0x%09x %9d %9d %3d  %s" % (e.index, e.sector, e.offset, e.comp, e.unc, e.flag, e.name))
    print("# %d entries, %d bytes stored, %d bytes uncompressed" % (n, tot_c, tot_u))


def cmd_verify(arc):
    bad = 0
    cur = 0
    seen = {}
    for e in arc.entries:
        if e.sector != cur:
            print("GAP/overlap before %s: sector %d expected %d" % (e.name, e.sector, cur))
            bad += 1
        if (e.comp + SECTOR - 1) // SECTOR != e.count:
            print("sector_count mismatch %s" % e.name)
            bad += 1
        if e.stored and e.comp != e.unc:
            print("stored size mismatch %s" % e.name)
            bad += 1
        cur = e.sector + e.count
        k = e.name.upper()
        if k in seen:
            print("duplicate name %s (entries %d, %d)" % (e.name, seen[k], e.index))
            bad += 1
        seen[k] = e.index
        try:
            arc.read(e)
        except Exception as ex:                      # noqa: BLE001
            print("FAIL %s: %s" % (e.name, ex))
            bad += 1
    print("verify: %d entries, %d problems, data end sector %d (dir at %d)" % (len(arc.entries), bad, cur, arc.dir_sector))
    return bad


def cmd_extract(arc, out, flt, do_mtp):
    n = skipped = 0
    for e in arc.entries:
        if flt and not fnmatch.fnmatch(e.name.upper(), flt.upper()):
            continue
        rel = safe_relpath(e.name)
        if not rel:
            skipped += 1
            continue
        dest = os.path.join(out, rel)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        data = arc.read(e)
        with open(dest, "wb") as fh:
            fh.write(data)
        n += 1
        if do_mtp and e.name.upper().endswith(".MTP"):
            sub = os.path.join(out, "_mtp", os.path.splitext(os.path.basename(rel))[0])
            for name, blob in unpack_mtp(data):
                mdest = os.path.join(sub, safe_relpath(name))
                os.makedirs(os.path.dirname(mdest), exist_ok=True)
                with open(mdest, "wb") as fh:
                    fh.write(blob)
    print("extracted %d entries to %s (%d skipped)" % (n, out, skipped))


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--arc", default=DEFAULT_ARC, help="path to data.arc (default: game/Data/data.arc)")
    ap.add_argument("--list", action="store_true", help="list entries")
    ap.add_argument("--verify", action="store_true", help="inflate every entry, check sizes/Adler-32/layout")
    ap.add_argument("--out", nargs="?", const=DEFAULT_OUT, help="extract to DIR (default re/extracted)")
    ap.add_argument("--filter", help="fnmatch glob on the archive path, e.g. 'BSP\\*' or '*.SCR'")
    ap.add_argument("--mtp", action="store_true", help="also unpack .mtp (MULA) multi-files into <out>/_mtp/<name>/")
    a = ap.parse_args(argv)
    if not (a.list or a.verify or a.out):
        ap.error("nothing to do: pass --list, --verify and/or --out")
    arc = Arc(a.arc)
    print("# %s: %d bytes, version 0x%x, %d entries, dir sector %d (%d->%d bytes)" %
          (a.arc, arc.size, arc.version, arc.count, arc.dir_sector, arc.dir_comp, arc.dir_unc))
    rc = 0
    if a.list:
        cmd_list(arc, a.filter)
    if a.verify:
        rc = 1 if cmd_verify(arc) else 0
    if a.out:
        cmd_extract(arc, a.out, a.filter, a.mtp)
    arc.close()
    return rc


if __name__ == "__main__":
    sys.exit(main())
