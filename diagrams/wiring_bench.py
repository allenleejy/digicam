"""Draws wiring-bench.png: every wire on camera 1, part pin to board pin.
Run: ../cad/.venv/bin/python wiring_bench.py
"""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch

RED, BLACK, YEL, GRN, WHT, BLU, PUR, ORA, GRY = ("#d83b3b", "#222222", "#e0b400", "#2e9e5b", "#bbbbbb",
                                                 "#2f6fd6", "#8a4fd0", "#f08a2c", "#888888")
GROUPS = [
    ("1  Screen  (1.14 in ST7789, 8 pins)", "Step 1", [
        ("GND", "GND", BLACK, ""),
        ("VCC", "3V3", RED, "3.3 V, never 5 V"),
        ("SCL", "GPIO 21", YEL, "clock"),
        ("SDA", "GPIO 47", GRN, "data"),
        ("RES", "EN  (or 3V3)", WHT, "resets with the board"),
        ("DC", "GPIO 41", BLU, ""),
        ("CS", "GPIO 42", PUR, ""),
        ("BLK", "GPIO 46", ORA, "backlight"),
    ]),
    ("2  Buttons  (6x6 mm: one leg to the pin, the diagonal leg to GND)", "Step 2", [
        ("◀  left", "GPIO 2", GRY, "previous look / photo"),
        ("●  middle", "GPIO 14", GRY, "gallery; hold: photo/video, delete"),
        ("▶  right", "GPIO 45", GRY, "next look / photo"),
        ("Shutter", "GPIO 48", GRY, "top edge"),
        ("other legs", "GND", BLACK, "all four share one GND wire"),
    ]),
    ("3  Power  (only after steps 1-2 work on USB)", "Step 3", [
        ("Cell +  /  −", "Module B+ / B−", RED, "check polarity with a meter first"),
        ("Module OUT+", "Switch middle leg", RED, "5 V"),
        ("Switch outer leg", "Board 5V", RED, "switch OFF whenever laptop USB is plugged in"),
        ("Module OUT−", "Board GND", BLACK, ""),
        ("Cell + → 100k →", "GPIO 1 → 100k → GND", GRN, "battery level"),
    ]),
]

H = 1.5 + sum(1.05 + 0.42 * len(g[2]) for g in GROUPS)
fig = plt.figure(figsize=(11, H), dpi=150)
ax = fig.add_axes([0, 0, 1, 1])
ax.set_xlim(0, 11)
ax.set_ylim(0, H)
ax.axis("off")
ax.text(0.4, H - 0.55, "Camera 1 bench wiring: part pin → board pin", fontsize=17, weight="bold", va="center")
y = H - 1.25
for title, _, wires in GROUPS:
    ax.text(0.4, y, title, fontsize=12.5, weight="bold", va="center", color="#333")
    y -= 0.55
    for part, board, colour, note in wires:
        ax.add_patch(FancyBboxPatch((0.45, y - 0.15), 2.2, 0.3, boxstyle="round,pad=0.02,rounding_size=0.06",
                                    fc="#f4f1fb", ec="#c9b8f2", lw=1))
        ax.text(0.6, y, part, fontsize=10.5, va="center")
        ax.plot([2.7, 4.75], [y, y], color=colour, lw=3.2, solid_capstyle="round")
        if colour == WHT:
            ax.plot([2.7, 4.75], [y, y], color="#999", lw=0.6, ls=(0, (2, 2)))
        ax.add_patch(FancyBboxPatch((4.8, y - 0.15), 2.3, 0.3, boxstyle="round,pad=0.02,rounding_size=0.06",
                                    fc="#eef6f0", ec="#8cc9a0", lw=1))
        ax.text(4.95, y, board, fontsize=10.5, va="center", weight="bold")
        ax.text(7.3, y, note, fontsize=9.5, va="center", color="#666")
        y -= 0.42
    y -= 0.5
fig.savefig("wiring-bench.png", facecolor="white")
print("wrote wiring-bench.png")
