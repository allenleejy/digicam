"""Export every piece of the v3 interactive guide, both bodies, and bundle them for the page.

    python3 export_guide_v3.py   ->  ../guide-v3/models.json  ({"clear/frame": base64 STL, ...})

The artifact host serves JSON but not STL, so the page loads one bundle.
"""
import base64
import json
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
SCAD = os.path.join(HERE, "guide_parts_v3.scad")
OUT = os.path.join(HERE, "..", "guide-v3")

COMMON = """frame sled carrier back_plate front_plate caps shutter_cap knob lens_trim battery foam board_pcb board_parts
board_pads camera camera_glass camera_tape ribbon screen_pcb screen_glass screen_active screen_pad charger_pcb
charger_parts charger_tape buttons shutter_sw slide_sw inserts screws_back screws_front screws_sled screws_board
screws_carrier w_bat_red w_bat_black w_out_plus w_5v w_gnd w_sense resistors w_buttons w_button_gnd w_shutter""".split()
COMMON += [f"w_screen_{i}" for i in range(8)]
PARTS = {"clear": COMMON + ["panel", "bezel"],
         "retro": COMMON + ["top_plate", "top_bits", "covering_front", "covering_back", "windows", "screws_top", "screws_barrel"]}


def export(job):
    style, name = job
    with tempfile.TemporaryDirectory() as d:
        out = os.path.join(d, "p.stl")
        r = subprocess.run(["openscad", "--backend=manifold", "--export-format=binstl", "-o", out,
                            "-D", f'style="{style}"', "-D", f'gpart="{name}"', SCAD], capture_output=True, text=True)
        log = r.stdout + r.stderr
        bad = [l for l in log.splitlines() if "ERROR" in l or "WARNING" in l or "empty" in l]
        data = open(out, "rb").read() if os.path.exists(out) else b""
    return f"{style}/{name}", data, bad


jobs = [(s, n) for s, names in PARTS.items() for n in names]
with ThreadPoolExecutor(8) as ex:
    results = list(ex.map(export, jobs))
bundle, failed = {}, False
for key, data, bad in results:
    if bad or not data:
        print("PROBLEM", key, *bad[:2])
        failed = True
    else:
        bundle[key] = base64.b64encode(data).decode()
os.makedirs(OUT, exist_ok=True)
with open(os.path.join(OUT, "models.json"), "w") as f:
    json.dump(bundle, f)
print(f"{len(bundle)} pieces, models.json {os.path.getsize(os.path.join(OUT, 'models.json')) / 1e6:.1f} MB")
sys.exit(1 if failed else 0)
