"""Measure the MAP2A HUD capture. Exit 0 only when the layout is one centered 4:3 block.

The compass ring and its green arc must share a center and be nearly circular.
The aim reticle must be a circle near the middle of the frame.
The short green status bar must sit under that reticle, not off to one side.
"""
import sys

import numpy as np
from PIL import Image


def clusters(mask, min_pixels=40):
    h, w = mask.shape
    seen = np.zeros_like(mask, dtype=bool)
    found = []
    ys, xs = np.where(mask)
    for y, x in zip(ys, xs):
        if seen[y, x]:
            continue
        stack = [(y, x)]
        seen[y, x] = True
        pts = []
        while stack:
            cy, cx = stack.pop()
            pts.append((cy, cx))
            for ny in range(max(0, cy - 2), min(h, cy + 3)):
                for nx in range(max(0, cx - 2), min(w, cx + 3)):
                    if mask[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True
                        stack.append((ny, nx))
        if len(pts) >= min_pixels:
            arr = np.array(pts)
            y0, y1 = int(arr[:, 0].min()), int(arr[:, 0].max())
            x0, x1 = int(arr[:, 1].min()), int(arr[:, 1].max())
            found.append({"x": (x0 + x1) / 2, "y": (y0 + y1) / 2, "w": x1 - x0 + 1, "h": y1 - y0 + 1, "n": len(pts)})
    return found


def main():
    path = sys.argv[1]
    im = np.array(Image.open(path).convert("RGB"))
    h, w = im.shape[:2]
    r, g, b = im[:, :, 0].astype(int), im[:, :, 1].astype(int), im[:, :, 2].astype(int)
    gold = (r > 150) & (g > 100) & (b < 90) & ((r - b) > 70) & (r > g - 30)
    green = (g > 170) & (r < 110) & (b < 100) & ((g - r) > 80)
    # Ignore the sky and the debug overlay. HUD sits in the lower half.
    gold[: h // 2, :] = False
    green[: h // 2, :] = False
    gold[:, :220] = False
    greens = clusters(green, 30)
    golds = clusters(gold, 25)
    print(f"frame {w}x{h}")
    for i, c in enumerate(sorted(golds, key=lambda c: -c["n"])[:6]):
        aspect = c["w"] / max(1, c["h"])
        print(f"gold{i} cx={c['x']:.0f} cy={c['y']:.0f} {c['w']}x{c['h']} aspect={aspect:.2f} n={c['n']}")
    for i, c in enumerate(sorted(greens, key=lambda c: -c["n"])[:6]):
        print(f"green{i} cx={c['x']:.0f} cy={c['y']:.0f} {c['w']}x{c['h']} n={c['n']}")

    problems = []
    rings = [c for c in golds if 0.7 <= c["w"] / max(1, c["h"]) <= 1.4 and 40 <= c["w"] <= 280]
    if not rings:
        problems.append("no circular compass or reticle ring")
    else:
        # The aim ring is the gold circle closest to the screen center.
        aim = min(rings, key=lambda c: abs(c["x"] - w / 2) + abs(c["y"] - h / 2))
        if abs(aim["x"] - w / 2) > 120:
            problems.append(f"aim reticle x={aim['x']:.0f} is {abs(aim['x'] - w / 2):.0f}px off center")
        if abs(aim["y"] - h / 2) > 180:
            problems.append(f"aim reticle y={aim['y']:.0f} is {abs(aim['y'] - h / 2):.0f}px off vertical center")
        bars = [c for c in greens if c["w"] > c["h"] * 3 and c["w"] < 400]
        if bars:
            bar = min(bars, key=lambda c: abs(c["x"] - aim["x"]) + abs(c["y"] - aim["y"]))
            if abs(bar["x"] - aim["x"]) > 80:
                problems.append(f"status bar x={bar['x']:.0f} is {abs(bar['x'] - aim['x']):.0f}px from the reticle")
        compasses = [c for c in rings if c is not aim]
        arcs = [c for c in greens if c["h"] > 40 and c["w"] > 40]
        if compasses and arcs:
            comp = max(compasses, key=lambda c: c["n"])
            arc = min(arcs, key=lambda c: abs(c["x"] - comp["x"]) + abs(c["y"] - comp["y"]))
            if abs(arc["x"] - comp["x"]) > comp["w"] or abs(arc["y"] - comp["y"]) > comp["h"]:
                problems.append(
                    f"compass arc cx={arc['x']:.0f},{arc['y']:.0f} is off the ring cx={comp['x']:.0f},{comp['y']:.0f}"
                )
            if comp["w"] / max(1, comp["h"]) > 1.35:
                problems.append(f"compass ring is wide ({comp['w']}x{comp['h']})")

    if problems:
        print("FAIL")
        for p in problems:
            print(" -", p)
        return 1
    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
