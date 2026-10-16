#!/usr/bin/env python3
"""Inventory the extracted DH2 data.arc tree for Track C.

Reads re/extracted (gitignored). Prints a text report. Does not modify assets.
"""
import collections
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(SCRIPT_DIR, "..", ".."))
EXTRACTED = os.path.join(ROOT, "re", "extracted")

FOCUS = {
    ".SCR", ".SCRIPT", ".XSC", ".SPJ", ".OUT", ".AP", ".CFG", ".FX", ".INC", ".MAT",
    ".SPL", ".SND", ".DAT", ".LUA", ".EXP", ".TXT",
}
LUA_MAGIC = b"\x1bLua"
TEXT_EXT = {
    ".SCR", ".SCRIPT", ".SPJ", ".CFG", ".FX", ".INC", ".MAT", ".TXT", ".LUA", ".INI",
    ".XML", ".H", ".EXP", ".CSV", ".LOG",
}
MP_RARE = (
    "sv_multiplayer", "cl_join", "cflobby", "maxplayers", "deathmatch",
    "info_player", "spawnpoint", "player2", "multiplayer", "coop", "co-op",
    "dedicated", "sv_port", "sv_password", "cl_password", "cl_disconnect",
    "cl_playername",
)
MP_BROAD = ("lobby", "server", "client", "network", "team")
ANIMAL_HINTS = (
    "bear", "wolf", "moose", "caribou", "deer", "elk", "boar", "leopard", "tiger",
    "lion", "croc", "crocodile", "buffalo", "rhino", "elephant", "kangaroo", "dingo",
    "snake", "cobra", "bison", "reindeer", "fox", "hyena", "wildebeest", "zebra",
    "gazelle", "impala", "gaur", "nilgai", "camel", "emu", "grizzly", "cougar",
    "puma", "lynx", "gator", "alligator", "hog", "sheep", "goat", "yak", "ibex",
    "markhor", "oryx", "kudu", "eland", "baboon", "python", "anaconda", "jaguar",
    "panther", "cheetah", "wildboar", "blackbear", "brownbear", "polarbear",
    "animal", "predator", "spawn",
)


def rel(path):
    return os.path.relpath(path, EXTRACTED)


def ext_of(name):
    return os.path.splitext(name)[1].upper()


def is_text_bytes(blob):
    if not blob:
        return False
    if b"\x00" in blob[:512]:
        return False
    sample = blob[:512]
    good = sum(1 for b in sample if b in (9, 10, 13) or 32 <= b < 127)
    return good / len(sample) >= 0.90


def ascii_strings(blob, min_len=5, limit=4000):
    out = []
    cur = []
    for b in blob:
        if 32 <= b < 127:
            cur.append(chr(b))
            if len(cur) > 200:
                cur = cur[-200:]
        else:
            if len(cur) >= min_len:
                out.append("".join(cur))
                if len(out) >= limit:
                    return out
            cur = []
    if len(cur) >= min_len:
        out.append("".join(cur))
    return out


def head_lines(blob, n=12, width=140):
    text = blob.decode("latin-1", errors="replace")
    lines = []
    for line in text.splitlines():
        line = line.replace("\t", " ")
        if len(line) > width:
            line = line[:width] + "..."
        lines.append(line)
        if len(lines) >= n:
            break
    return lines


def hex_head(blob, n=32):
    chunk = blob[:n]
    hexpart = " ".join("%02X" % b for b in chunk)
    asc = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
    return hexpart + "  " + asc


def main():
    if not os.path.isdir(EXTRACTED):
        print("missing", EXTRACTED)
        return 1

    ext_count = collections.Counter()
    top_count = collections.Counter()
    focus_files = collections.defaultdict(list)
    all_files = []
    folder_children = collections.defaultdict(set)

    for dirpath, dirnames, filenames in os.walk(EXTRACTED):
        dirnames.sort()
        rdir = rel(dirpath)
        parts = [] if rdir == "." else rdir.split(os.sep)
        if len(parts) >= 1:
            top_count[parts[0]] += len(filenames)
        if len(parts) <= 2:
            folder_children[rdir].update(dirnames)
        for fn in filenames:
            path = os.path.join(dirpath, fn)
            e = ext_of(fn)
            ext_count[e] += 1
            rp = rel(path)
            all_files.append(rp)
            if e in FOCUS or e in (".BSP", ".LUA", ".TXT", ".XML", ".INI", ".EXP", ".H"):
                focus_files[e].append(path)

    print("=== EXT COUNTS ===")
    for e, n in ext_count.most_common():
        print("%6d  %s" % (n, e or "(none)"))
    print("files", sum(ext_count.values()))
    print("=== TOP FOLDERS ===")
    for name, n in top_count.most_common():
        print("%6d  %s" % (n, name))

    print("=== TREE depth<=2 ===")
    for key in sorted(folder_children):
        kids = sorted(folder_children[key])
        if kids:
            print("%s/ -> %s" % (key, ", ".join(kids[:80])))

    print("=== FOCUS CLASSIFY ===")
    # also scan every non-media file's first bytes for Lua
    lua_hits = []
    media_skip = {".DDS", ".WAV", ".TGA", ".BMP", ".PNG", ".JPG"}
    class_rows = []

    interesting_ext = set(FOCUS) | {".BSP", ".LUA", ".TXT", ".XML", ".INI", ".EXP", ".H", ".SCRIPT"}
    for e, paths in sorted(focus_files.items()):
        magics = collections.Counter()
        n_text = n_lua = n_bin = 0
        versions = collections.Counter()
        samples_text = []
        samples_bin = []
        for path in paths:
            with open(path, "rb") as fh:
                blob = fh.read(4096)
            if blob.startswith(LUA_MAGIC):
                n_lua += 1
                ver = blob[4] if len(blob) > 4 else -1
                versions[ver] += 1
                magics["Lua %02X" % ver] += 1
                if len(samples_bin) < 2:
                    samples_bin.append((path, blob[:64], os.path.getsize(path)))
            elif is_text_bytes(blob):
                n_text += 1
                magics["text"] += 1
                if len(samples_text) < 2:
                    samples_text.append((path, blob, os.path.getsize(path)))
            else:
                n_bin += 1
                magics[blob[:4].hex()] += 1
                if len(samples_bin) < 3:
                    samples_bin.append((path, blob[:64], os.path.getsize(path)))
        print("EXT %s count=%d text=%d lua=%d bin=%d versions=%s magics=%s" % (
            e, len(paths), n_text, n_lua, n_bin, dict(versions), magics.most_common(8)))
        class_rows.append((e, samples_text, samples_bin))

    print("=== LUA SCAN (first 8 bytes, non-media) ===")
    n_scanned = 0
    for dirpath, dirnames, filenames in os.walk(EXTRACTED):
        for fn in filenames:
            if ext_of(fn) in media_skip:
                continue
            path = os.path.join(dirpath, fn)
            n_scanned += 1
            with open(path, "rb") as fh:
                b = fh.read(8)
            if b.startswith(LUA_MAGIC):
                lua_hits.append((rel(path), b[4] if len(b) > 4 else -1, os.path.getsize(path)))
    print("scanned", n_scanned, "lua", len(lua_hits))
    verc = collections.Counter(v for _, v, _ in lua_hits)
    print("lua versions", dict(verc))
    by_ext = collections.Counter(ext_of(p) for p, _, _ in lua_hits)
    print("lua by ext", dict(by_ext))
    for row in lua_hits[:15]:
        print("  lua", row)

    print("=== SNIPPETS ===")
    for e, texts, bins in class_rows:
        for path, blob, size in texts[:1]:
            print("--- TEXT %s (%d bytes) %s" % (e, size, rel(path)))
            for line in head_lines(blob, 10):
                print(" | " + line)
        for path, blob, size in bins[:2]:
            print("--- BIN %s (%d bytes) %s" % (e, size, rel(path)))
            print(" | " + hex_head(blob, 48))

    print("=== STEM PAIRS ===")
    stems = collections.defaultdict(set)
    for e in (".SCR", ".SCRIPT", ".XSC", ".SPJ", ".OUT", ".AP"):
        for path in focus_files.get(e, []):
            stem = os.path.splitext(rel(path))[0].upper()
            stems[stem].add(e)
    combo = collections.Counter()
    for stem, exts in stems.items():
        combo[tuple(sorted(exts))] += 1
    for c, n in combo.most_common():
        print("%5d  %s" % (n, "+".join(c)))
    # show a few multi-ext stems
    shown = 0
    for stem, exts in sorted(stems.items()):
        if len(exts) >= 2 and shown < 25:
            print("  pair", stem, ",".join(sorted(exts)))
            shown += 1

    print("=== BSP ===")
    bsp_keep = re.compile(
        r"class|spawn|animal|origin|mission|trigger|player|hunt|bear|wolf|deer|moose|"
        r"leopard|tiger|croc|snake|target|entity|script|zone|predator|boss",
        re.I,
    )
    for path in sorted(focus_files.get(".BSP", [])):
        size = os.path.getsize(path)
        print("%10d  %s" % (size, rel(path)))
        with open(path, "rb") as fh:
            blob = fh.read()
        strs = ascii_strings(blob, 4, 8000)
        picked = []
        for s in strs:
            if bsp_keep.search(s) and len(s) < 120:
                picked.append(s)
        # unique, preserve order
        seen = set()
        uniq = []
        for s in picked:
            if s not in seen:
                seen.add(s)
                uniq.append(s)
        print("  strings %d kept %d" % (len(strs), len(uniq)))
        for s in uniq[:40]:
            print("   ", s)

    print("=== CFG FILES ===")
    for path in sorted(focus_files.get(".CFG", [])):
        size = os.path.getsize(path)
        with open(path, "rb") as fh:
            blob = fh.read()
        print("--- CFG %d %s" % (size, rel(path)))
        if is_text_bytes(blob[:512]):
            lines = head_lines(blob, 40, 160)
            for line in lines:
                print(" | " + line)
            extra = blob.decode("latin-1", errors="replace").count("\n") + 1
            if extra > 40:
                print(" | ... (%d lines total)" % extra)
        else:
            print(" | " + hex_head(blob, 32))

    print("=== KEYWORDS ===")
    # Search text focus files fully (they are small) and BSP/AP/XSC/OUT via strings.
    def search_blob(path, blob, terms, bucket):
        if is_text_bytes(blob[:800]) or ext_of(path) in TEXT_EXT:
            hay = blob.decode("latin-1", errors="replace").lower()
            lines = hay.splitlines()
        else:
            lines = [s.lower() for s in ascii_strings(blob, 4, 8000)]
            hay = "\n".join(lines)
        for term in terms:
            if term in hay:
                # one context line
                ctx = ""
                for line in lines:
                    if term in line:
                        ctx = line.strip()[:160]
                        break
                bucket[term].append((rel(path), ctx))

    rare_hits = collections.defaultdict(list)
    broad_hits = collections.defaultdict(list)
    animal_files = collections.Counter()
    # path-only animal hits
    print("--- path animal/spawn tokens ---")
    path_hits = collections.Counter()
    for rp in all_files:
        low = rp.lower().replace("\\", "/")
        for hint in ANIMAL_HINTS:
            if hint in low:
                path_hits[hint] += 1
                if path_hits[hint] <= 8:
                    print("  path", hint, rp)
    print("path token totals", dict(path_hits))

    search_exts = FOCUS | {".BSP", ".LUA", ".TXT", ".XML", ".INI", ".EXP", ".H"}
    for e, paths in focus_files.items():
        if e not in search_exts:
            continue
        for path in paths:
            size = os.path.getsize(path)
            # cap huge BSP reads at 8 MB window from start and a middle slice is enough for entity chunk
            # but entities can sit anywhere; read whole if <= 32 MB else strings from first+last 4MB
            with open(path, "rb") as fh:
                if size <= 32 * 1024 * 1024:
                    blob = fh.read()
                else:
                    blob = fh.read(4 * 1024 * 1024)
                    fh.seek(max(0, size - 4 * 1024 * 1024))
                    blob += fh.read()
            search_blob(path, blob, MP_RARE, rare_hits)
            if e in (".CFG", ".SCR", ".SCRIPT", ".FX", ".TXT", ".XML", ".LUA", ".INI", ".INC", ".MAT", ".SPJ"):
                search_blob(path, blob, MP_BROAD, broad_hits)
            low = blob.lower() if is_text_bytes(blob[:400]) else b""
            # animal content: filename already counted; content for script-like
            if e in (".SCR", ".SCRIPT", ".SPJ", ".CFG", ".TXT", ".LUA", ".XML", ".OUT", ".XSC", ".BSP", ".AP"):
                strs = blob.decode("latin-1", errors="replace").lower() if (e in TEXT_EXT or is_text_bytes(blob[:400])) else "\n".join(ascii_strings(blob, 4, 5000)).lower()
                for hint in ("animal", "predator", "spawn", "bear", "wolf", "leopard", "tiger", "croc", "moose", "deer"):
                    if hint in strs:
                        animal_files[hint] += 1

    print("--- rare mp ---")
    for term in MP_RARE:
        hits = rare_hits.get(term, [])
        print("TERM %s count=%d" % (term, len(hits)))
        for rp, ctx in hits[:12]:
            print("  %s :: %s" % (rp, ctx))
    print("--- broad mp (script/cfg/text only) ---")
    for term in MP_BROAD:
        hits = broad_hits.get(term, [])
        print("TERM %s count=%d" % (term, len(hits)))
        for rp, ctx in hits[:8]:
            print("  %s :: %s" % (rp, ctx))
    print("--- animal content file counts ---")
    print(dict(animal_files))

    print("=== INTERFACE NAMES ===")
    iface = os.path.join(EXTRACTED, "INTERFACE")
    if os.path.isdir(iface):
        for dirpath, dirnames, filenames in os.walk(iface):
            dirnames.sort()
            rdir = rel(dirpath)
            depth = 0 if rdir == "." else rdir.count(os.sep)
            if depth <= 3:
                print("%s/ [%d files] %s" % (rdir, len(filenames), ", ".join(sorted(dirnames)[:40])))

    print("=== LEVEL / SCRIPT DIRS ===")
    for top in ("LEVELS", "BSP", "SCRIPTS", "CFG", "OBJECTS"):
        base = os.path.join(EXTRACTED, top)
        if not os.path.isdir(base):
            print("missing", top)
            continue
        print("--", top)
        for dirpath, dirnames, filenames in os.walk(base):
            dirnames.sort()
            rdir = rel(dirpath)
            depth = rdir.count(os.sep)
            if depth <= 3:
                exts = collections.Counter(ext_of(f) for f in filenames)
                interesting = [f for f in sorted(filenames) if ext_of(f) in FOCUS or ext_of(f) in (".BSP", ".LUA", ".TXT", ".XML")]
                if depth <= 2 or interesting or exts:
                    print("  %s  files=%d exts=%s names=%s" % (
                        rdir, len(filenames), dict(exts), ", ".join(interesting[:18])))

    print("=== DONE ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
