"""Export camera1_v3.scad: every printable part of both styles, the renders, and the fit check.

    python3 export_v3.py            ->  export/v3/<style>/*.stl, renders/v3/*.png
    python3 export_v3.py retro      ->  one style only

Each STL is already in its printing orientation (flat face down, no supports needed).
"""
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
SCAD = os.path.join(HERE, "camera1_v3.scad")

PARTS = {
    "clear": ["frame", "back_plate", "front_plate", "sled", "carrier", "caps", "lens_ring", "panel", "bezel"],
    "retro": ["frame", "back_plate", "front_plate", "sled", "carrier", "caps", "top_plate", "top_bits",
              "lens_barrel", "leatherette", "back_leatherette", "window_inserts"],
}
# camera eye and centre for each render (the assembled camera faces -Y in the views)
VIEWS = {
    "front": ("assembly", "150,-175,95,56,-10,36"),
    "back": ("assembly", "-50,195,80,56,-10,36"),
    "exploded": ("exploded", "260,-230,170,56,-10,40"),
    "inside": ("inside", "-60,200,40,56,-10,34"),
}


def run(args):
    r = subprocess.run(["openscad", "--backend=manifold", *args, SCAD], capture_output=True, text=True)
    bad = [l for l in (r.stdout + r.stderr).splitlines() if "ERROR" in l or "WARNING" in l]
    return (args[-1], bad)


def jobs(style):
    out = os.path.join(HERE, "export", "v3", style)
    os.makedirs(out, exist_ok=True)
    os.makedirs(os.path.join(HERE, "renders", "v3"), exist_ok=True)
    for p in PARTS[style]:
        yield ["-D", f'style="{style}"', "-D", f'part="{p}"', "-o", os.path.join(out, p + ".stl")]
    if style == "clear":   # the clear plates can also be laser-cut from 2 mm acrylic
        for p in ["back_plate_2d", "front_plate_2d"]:
            yield ["-D", f'style="{style}"', "-D", f'part="{p}"', "-o", os.path.join(out, p.replace("_2d", "_lasercut") + ".dxf")]
    for name, (part, cam) in VIEWS.items():
        yield ["-D", f'style="{style}"', "-D", f'part="{part}"', "--imgsize=1600,1100", "--colorscheme=Tomorrow",
               f"--camera={cam}", "-o", os.path.join(HERE, "renders", "v3", f"{style}_{name}.png")]


styles = sys.argv[1:] or ["clear", "retro"]
with ThreadPoolExecutor(8) as ex:
    results = list(ex.map(run, [j for s in styles for j in jobs(s)]))
for out, bad in results:
    print(("PROBLEM " if bad else "ok      ") + os.path.relpath(out, HERE), *bad, sep="\n  " if bad else "")
sys.exit(subprocess.call([sys.executable, os.path.join(HERE, "check_v3.py"), *styles]))
