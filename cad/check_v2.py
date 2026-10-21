"""Fit check for camera1_v2.scad: every pair of parts that must not overlap.
Run: python3 check_v2.py   (needs openscad on the PATH)
"""
import itertools
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
SCAD = os.path.join(HERE, "camera1_v2.scad")
GHOSTS = ["battery", "board", "screen", "charger", "camera", "buttons", "shutter", "switch", "caps"]
SOLIDS = ["frame", "sled", "back_plate", "front_plate"]

pairs = [(g, s) for g in GHOSTS for s in SOLIDS]
pairs += list(itertools.combinations(GHOSTS, 2))
pairs += [("frame", "sled"), ("sled", "back_plate"), ("sled", "front_plate")]


def overlap(pair):
    a, b = pair
    with tempfile.TemporaryDirectory() as d:
        out = os.path.join(d, "x.stl")
        r = subprocess.run(["openscad", "--backend=manifold", "-o", out, "-D", 'part="check"',
                            "-D", f'check_a="{a}"', "-D", f'check_b="{b}"', SCAD],
                           capture_output=True, text=True)
        log = r.stdout + r.stderr
        if "top level object is empty" in log:
            return a, b, None
        if not os.path.exists(out):
            return a, b, "error: " + log.strip().splitlines()[-1]
        # bounding box of the overlap, so it's clear where it is
        xs, ys, zs = [], [], []
        for line in open(out):
            p = line.split()
            if p and p[0] == "vertex":
                xs.append(float(p[1])); ys.append(float(p[2])); zs.append(float(p[3]))
        if min(max(v) - min(v) for v in (xs, ys, zs)) < 0.05:
            return a, b, None   # touching faces (a part resting on its seat), not an overlap
        return a, b, f"x {min(xs):.1f}..{max(xs):.1f}  y {min(ys):.1f}..{max(ys):.1f}  z {min(zs):.1f}..{max(zs):.1f}"


with ThreadPoolExecutor(8) as ex:
    results = list(ex.map(overlap, pairs))
bad = [r for r in results if r[2]]
for a, b, where in bad:
    print(f"OVERLAP {a} / {b}: {where}")
print(f"{len(pairs)} pairs checked, {len(bad)} overlapping")
sys.exit(1 if bad else 0)
