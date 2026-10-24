"""Fit check for camera1_v3.scad: every pair of parts that must not overlap, in both styles.
Run: python3 check_v3.py [clear|retro]   (needs openscad on the PATH)
"""
import itertools
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
SCAD = os.path.join(HERE, "camera1_v3.scad")
STYLES = sys.argv[1:] or ["clear", "retro"]
GHOSTS = ["battery", "board", "screen", "charger", "camera", "buttons", "shutter", "switch", "caps", "knob"]
SOLIDS = ["frame", "sled", "carrier", "back_plate", "front_plate", "top_plate", "panel", "bezel"]

pairs = [(g, s) for g in GHOSTS for s in SOLIDS]
pairs += list(itertools.combinations(GHOSTS, 2))
pairs += list(itertools.combinations(SOLIDS, 2))
jobs = [(st, a, b) for st in STYLES for a, b in pairs
        if not (st == "clear" and "top_plate" in (a, b)) and not (st == "retro" and {"panel", "bezel"} & {a, b})]


def overlap(job):
    style, a, b = job
    with tempfile.TemporaryDirectory() as d:
        out = os.path.join(d, "x.stl")
        r = subprocess.run(["openscad", "--backend=manifold", "-o", out, "-D", f'style="{style}"', "-D", 'part="check"',
                            "-D", f'check_a="{a}"', "-D", f'check_b="{b}"', SCAD],
                           capture_output=True, text=True)
        log = r.stdout + r.stderr
        if "top level object is empty" in log:
            return style, a, b, None
        if not os.path.exists(out):
            return style, a, b, f"error (exit {r.returncode}): " + (log.strip().splitlines() or ["no output"])[-1]
        # bounding box of the overlap, so it's clear where it is, and its volume
        pts = [tuple(map(float, l.split()[1:4])) for l in open(out) if l.split()[:1] == ["vertex"]]
        xs, ys, zs = zip(*pts)
        vol = abs(sum(  # signed tetrahedra from the origin
            (p[0] * (q[1] * r[2] - q[2] * r[1]) - p[1] * (q[0] * r[2] - q[2] * r[0]) + p[2] * (q[0] * r[1] - q[1] * r[0])) / 6
            for p, q, r in zip(pts[0::3], pts[1::3], pts[2::3])))
        if vol < 0.01 or min(max(v) - min(v) for v in (xs, ys, zs)) < 0.05:
            return style, a, b, None   # touching faces (a part resting on its seat), not an overlap
        return style, a, b, f"{vol:.2f} mm3 at x {min(xs):.1f}..{max(xs):.1f}  y {min(ys):.1f}..{max(ys):.1f}  z {min(zs):.1f}..{max(zs):.1f}"


with ThreadPoolExecutor(8) as ex:
    results = list(ex.map(overlap, jobs))
bad = [r for r in results if r[3]]
for style, a, b, where in bad:
    print(f"OVERLAP [{style}] {a} / {b}: {where}")
print(f"{len(jobs)} pairs checked, {len(bad)} overlapping")
sys.exit(1 if bad else 0)
