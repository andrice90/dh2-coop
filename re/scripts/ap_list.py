"""List animation clip names in a Chameleon Engine .AP pack.

Usage:
    py re\\scripts\\ap_list.py <file.AP>

Prints one clip name per line, in pack order. The pack stores that
list once per header repeat; only the first copy is printed.

Layout (little-endian), checked against the retail packs under
re/extracted (see re/notes/anim-names.md):

    0x00  char[4]   magic "FBAF"
    0x04  uint16    version (0x0411 on every pack sampled)
    0x06  uint32    per-file id (algorithm not identified)
    0x0A  uint8     banner length
    0x0B  char[]    banner, typically "Make Model (build 402 - Sep 30 2005)"
          uint8     0x00
          uint16    tag 0xEFF1 (bytes F1 EF)  geometry chunk
          uint32    geometry size A   (field offset + size = geometry end)
          uint32    geometry size B   (field offset + size = same end)
          ...       geometry bytes
          uint8[4]  bridge (not interpreted)
          uint16    tag 0xEFE8 (bytes E8 EF)  animation chunk
          uint32    anim chunk size   (field offset + size = file size - 10)
          uint8     repeat count (same name list, different key data)
          uint16    clip count
          uint16    0xFFFF on the first record (a directory sentinel)
          records   repeat_count passes of clip_count ModelAnimBone records

Each clip record begins:

    uint16  tag, 0xFFFF for most clips. A few clips use another value
            (MOUNTAINGUIDE GAME.AP: "find" is 0x012D; "idlenormalneutral"
            and "syncguidegetup_foot" are 0x013A). Meaning of the other
            values is not identified.
    uint8   class-name length (13)
    char[]  "ModelAnimBone"
    uint8   clip-name length
    char[]  clip name

The file ends with the 6 bytes E0 EF 00 00 00 00. Four bytes immediately
before that trailer sit outside the anim-chunk size.
"""

from __future__ import annotations

import sys
from pathlib import Path

MAGIC = b"FBAF"
GEOM_TAG = b"\xf1\xef"
ANIM_TAG = b"\xe8\xef"
BONE_MARK = bytes([13]) + b"ModelAnimBone"


class ApError(Exception):
    pass


def _u16(buf: bytes, off: int) -> int:
    return int.from_bytes(buf[off : off + 2], "little")


def _u32(buf: bytes, off: int) -> int:
    return int.from_bytes(buf[off : off + 4], "little")


def anim_chunk(data: bytes) -> tuple[int, int, int]:
    """Return (chunk_offset, repeat_count, clip_count) for the anim directory."""
    if len(data) < 16 or data[:4] != MAGIC:
        raise ApError("not an FBAF animation pack")
    banner_len = data[10]
    nul = 11 + banner_len
    if nul + 11 > len(data) or data[nul] != 0 or data[nul + 1 : nul + 3] != GEOM_TAG:
        raise ApError("unrecognized FBAF header")
    geom_end = (nul + 3) + _u32(data, nul + 3)
    anim_at = geom_end + 4
    if anim_at + 11 > len(data) or data[anim_at : anim_at + 2] != ANIM_TAG:
        raise ApError("animation chunk tag E8 EF not at the geometry boundary")
    if (anim_at + 2) + _u32(data, anim_at + 2) != len(data) - 10:
        raise ApError("animation chunk size does not end 10 bytes before EOF")
    parts = data[anim_at + 6]
    count = _u16(data, anim_at + 7)
    if data[anim_at + 9 : anim_at + 11] != b"\xff\xff":
        raise ApError("animation directory is missing the FF FF sentinel")
    if parts < 1 or count < 1:
        raise ApError(f"empty animation directory (parts={parts}, clips={count})")
    return anim_at, parts, count


def _clip_name(data: bytes, mark_at: int) -> str:
    name_at = mark_at + len(BONE_MARK)
    if name_at >= len(data):
        raise ApError("truncated ModelAnimBone record")
    n = data[name_at]
    raw = data[name_at + 1 : name_at + 1 + n]
    if len(raw) != n or any(c < 32 or c > 126 for c in raw):
        raise ApError(f"clip name at offset {name_at} is not printable ASCII")
    return raw.decode("ascii")


def iter_clip_names(data: bytes, start: int):
    i = start
    while True:
        j = data.find(BONE_MARK, i)
        if j < 0:
            return
        yield _clip_name(data, j)
        i = j + len(BONE_MARK)


def list_anims(data: bytes) -> list[str]:
    """Clip names from the first stored copy, in file order."""
    anim_at, parts, count = anim_chunk(data)
    names = []
    for name in iter_clip_names(data, anim_at + 9):
        names.append(name)
        if len(names) == parts * count:
            break
    if len(names) != parts * count:
        raise ApError(
            f"found {len(names)} ModelAnimBone clips, header says {parts}*{count}"
        )
    first = names[:count]
    for part in range(1, parts):
        if names[part * count : (part + 1) * count] != first:
            raise ApError(f"part {part} clip list differs from part 0")
    return first


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: py re\\scripts\\ap_list.py <file.AP>", file=sys.stderr)
        return 2
    path = Path(argv[1])
    try:
        names = list_anims(path.read_bytes())
    except OSError as exc:
        print(f"{path}: {exc}", file=sys.stderr)
        return 1
    except ApError as exc:
        print(f"{path}: {exc}", file=sys.stderr)
        return 1
    for name in names:
        print(name)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
