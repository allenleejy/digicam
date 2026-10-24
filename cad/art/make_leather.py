"""Pebble-grain leather heightmap for the retro coverings: art/leather.png.

    python3 art/make_leather.py

One pixel = tex_px mm in camera1_v3.scad (0.2 mm), so the 580 x 360 image covers 116 x 72 mm,
enough for the front covering. White is the top of a pebble, black the bottom of a crease.
The pebbles are Voronoi cells about 1.4 mm across with rounded tops, so they still read at
0.4 mm nozzle width; a few long, faint creases run through them like real hide.
"""
import os
import numpy as np
from PIL import Image
from scipy import ndimage as ndi
from scipy.spatial import cKDTree

PX = 0.2                    # mm per pixel
SIZE = (116.0, 72.0)        # mm
CELL = 1.4                  # mean pebble size, mm
rng = np.random.default_rng(31)

w, h = int(SIZE[0] / PX), int(SIZE[1] / PX)
# jittered grid of pebble centres (more even than pure random, like real grain)
gx, gy = np.meshgrid(np.arange(-1, SIZE[0] / CELL + 2), np.arange(-1, SIZE[1] / CELL + 2))
pts = np.column_stack([gx.ravel(), gy.ravel()]) * CELL
pts += rng.uniform(-0.42, 0.42, pts.shape) * CELL
pts[:, 0] += (np.round(pts[:, 1] / CELL) % 2) * CELL / 2
tree = cKDTree(pts)

yy, xx = np.mgrid[0:h, 0:w]
q = np.column_stack([(xx.ravel() + 0.5) * PX, (yy.ravel() + 0.5) * PX])
d, idx = tree.query(q, k=2)
edge = (d[:, 1] - d[:, 0]).reshape(h, w)           # distance to the nearest crease between cells
cell_h = rng.uniform(0.75, 1.0, len(pts))[idx[:, 0]].reshape(h, w)

# rounded pebble: steep near the crease, flat on top
height = np.clip(edge / (0.42 * CELL), 0, 1)
height = np.sqrt(1 - (1 - height) ** 2) * cell_h

# long faint creases (low-frequency noise ridges), and a little fine tooth
noise = ndi.gaussian_filter(rng.standard_normal((h, w)), 22)
noise /= np.abs(noise).max()
creases = np.exp(-(noise / 0.045) ** 2)
fine = ndi.gaussian_filter(rng.standard_normal((h, w)), 1.0)
fine /= np.abs(fine).max()
height = height * (1 - 0.35 * creases) + 0.06 * fine
height = ndi.gaussian_filter(height, 0.6)
height = (height - height.min()) / (height.max() - height.min())

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "leather.png")
Image.fromarray((height * 255).astype(np.uint8)).save(out)
print(out, w, "x", h, "px")
