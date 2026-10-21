"""Exports every part of the 3D assembly guide to ../guide/models/<name>.stl (binary).
Run: python3 export_guide.py
"""
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "guide", "models")
PARTS = """frame sled back_plate front_plate lens_ring vf_front vf_back caps shutter_cap
battery foam board_pcb board_pads board_parts screen_pcb screen_glass screen_active charger_pcb charger_parts
camera camera_glass buttons button_feet shutter_sw slide_sw inserts_back inserts_front inserts_sled screws_back screws_front
screws_sled screws_board screws_screen tape w_screen_0 w_screen_1 w_screen_2 w_screen_3 w_screen_4 w_screen_5
w_screen_6 w_screen_7 w_bat_red w_bat_black w_buttons w_button_gnd w_power w_gnd w_sense resistors ribbon""".split()


def export(name):
    out = os.path.join(OUT, name + ".stl")
    r = subprocess.run(["openscad", "--backend=manifold", "--export-format=binstl", "-o", out,
                        "-D", f'gpart="{name}"', os.path.join(HERE, "guide_parts.scad")],
                       capture_output=True, text=True)
    bad = [l for l in (r.stdout + r.stderr).splitlines() if "ERROR" in l or "empty" in l]
    return name, os.path.getsize(out) if os.path.exists(out) else 0, bad


os.makedirs(OUT, exist_ok=True)
with ThreadPoolExecutor(8) as ex:
    results = list(ex.map(export, PARTS))
total = 0
for name, size, bad in results:
    total += size
    if bad or not size:
        print("PROBLEM", name, bad)
print(f"{len(results)} parts, {total / 1e6:.1f} MB")
failed = any(b or not s for _, s, b in results)

# The artifact host serves JSON but not STL, so the page loads one bundle of base64 STLs
import base64
import json
bundle = {n: base64.b64encode(open(os.path.join(OUT, n + ".stl"), "rb").read()).decode() for n in PARTS}
with open(os.path.join(OUT, "..", "models.json"), "w") as f:
    json.dump(bundle, f)
print("models.json", os.path.getsize(os.path.join(OUT, "..", "models.json")) // 1024, "KB")
sys.exit(1 if failed else 0)
