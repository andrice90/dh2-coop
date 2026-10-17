"""Scan BSP entity lumps for shootable prop class names and model paths.

Read-only. Prints class counts and any entity block whose text mentions
bottle, stump, can, glass, or triggerhit.
"""
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BSP_DIR = ROOT / "re" / "extracted" / "BSP"

NEEDLES = ("bottle", "stump", "canister", "glass", "triggerhit", "obj_hit", "target")


def strings(data: bytes) -> str:
    parts = re.findall(rb"[\x20-\x7e]{4,}", data)
    return "\n".join(p.decode("ascii") for p in parts)


def blocks(text: str):
    # entity("class") ... until the next entity( or end
    parts = re.split(r'(?=entity\(")', text)
    for part in parts:
        if part.startswith('entity("'):
            yield part


def summarize_targets() -> None:
    print("=== obj_target across BSPs ===")
    for path in sorted(BSP_DIR.glob("*.BSP")):
        text = strings(path.read_bytes())
        for b in blocks(text):
            if not b.startswith('entity("obj_target")'):
                continue
            model = re.search(r'property\("model_name", "([^"]*)"\)', b)
            tname = re.search(r'property\("target_name", "([^"]*)"\)', b)
            cheat = re.search(r'property\("generic/cheat", "([^"]*)"\)', b)
            health = re.search(r'property\("health", "([^"]*)"\)', b)
            print(
                f"{path.name:16} model={model.group(1) if model else '?':60} "
                f"name={tname.group(1) if tname else '?':16} "
                f"cheat={cheat.group(1) if cheat else '?'} health={health.group(1) if health else '?'}"
            )


def main() -> None:
    if sys.argv[1:] == ["--targets"]:
        summarize_targets()
        return
    maps = sys.argv[1:] or ["MAP1.BSP"]
    for name in maps:
        path = BSP_DIR / name
        text = strings(path.read_bytes())
        ents = list(blocks(text))
        classes = Counter()
        for b in ents:
            m = re.match(r'entity\("([^"]+)"\)', b)
            if m:
                classes[m.group(1)] += 1
        print(f"=== {name} entities {len(ents)} ===")
        for cls, n in classes.most_common():
            print(f"  {n:4d}  {cls}")
        print("--- hits ---")
        for b in ents:
            low = b.lower()
            if any(n in low for n in NEEDLES):
                shown = " | ".join(line.strip() for line in b.splitlines() if line.strip())
                print(shown[:500])
                print()


if __name__ == "__main__":
    main()
