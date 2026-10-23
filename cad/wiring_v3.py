"""Wiring pictures for the v3 bodies, drawn from the model's own positions.

    ../cad/.venv/bin/python wiring_v3.py

  diagrams/wiring-v3.png          every wire, part pin to board pad
  diagrams/routes-v3-clear.png    where each wire runs in the clear body, and how long to cut it
  diagrams/routes-v3-retro.png    the same for the retro body
  diagrams/wire-lengths-v3.json   the cut lengths, for the guide
"""
import json
import math
import os
import re
import subprocess

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "diagrams")
FONT = "/System/Library/Fonts/Helvetica.ttc"

RED, BLACK, WHITE_W, GREY = (214, 48, 49), (30, 30, 34), (120, 120, 130), (150, 150, 160)
SIG = (66, 110, 220)       # signal wires are drawn blue so they show; any light colour is fine in the camera


def font(size, bold=False):
    return ImageFont.truetype(FONT, size, index=1 if bold else 0)


def coords(style):
    r = subprocess.run(["openscad", "--backend=manifold", "-D", f'style="{style}"', "-D", 'part="coords"',
                        "-o", "/dev/null", "--export-format", "stl", os.path.join(HERE, "camera1_v3.scad")],
                       capture_output=True, text=True)
    m = re.search(r'COORDS (\{.*\})"', r.stdout + r.stderr)
    return json.loads(m.group(1).replace("\\", ""))


# ---------------------------------------------------------------- 1. part pin to board pad
def schematic():
    img = Image.new("RGB", (1700, 2000), "white")
    d = ImageDraw.Draw(img)
    d.text((40, 26), "Camera wiring: part pin to board pad (both bodies)", fill=BLACK, font=font(34, True))
    d.text((40, 72), "Match each pad by the label printed beside it on the board. Pad positions differ between boards,"
           " so follow the labels, not the order drawn here.", fill=(90, 90, 100), font=font(19))
    # rows: (board pad, part pin, colour). Each part pin sits level with its pad, so signal wires are straight.
    rows = [("3V3", "VCC", RED), ("GND", "GND", BLACK), ("GPIO 21", "SCL", SIG), ("GPIO 47", "SDA", SIG),
            ("EN", "RES", SIG), ("GPIO 41", "DC", SIG), ("GPIO 42", "CS", SIG), ("GPIO 46", "BLK", SIG), None,
            ("GPIO 2", "left button", SIG), ("GPIO 14", "OK button", SIG), ("GPIO 45", "right button", SIG),
            ("GPIO 48", "shutter", SIG), ("GND", "other leg of all four", BLACK), None,
            ("5V", "", RED), ("GND", "", BLACK), ("GPIO 1", "", SIG)]
    ys, y, gaps = [], 196, iter([100, 150])
    for r in rows:
        ys.append(y if r else None)
        y += 58 if r else next(gaps)
    bx = 1260
    d.rounded_rectangle([bx, 120, bx + 300, y + 4], 18, fill=(42, 107, 74))
    d.text((bx + 22, 128), "ESP32-S3-CAM board", fill="white", font=font(22, True))
    for r, yy in zip(rows, ys):
        if not r:
            continue
        d.ellipse([bx - 12, yy - 12, bx + 12, yy + 12], fill=(230, 200, 90), outline=BLACK, width=2)
        d.text((bx + 30, yy - 14), r[0], fill="white", font=font(24, True))

    def box(x0, y0, x1, y1, title, sub="", fill=(248, 247, 250)):
        d.rounded_rectangle([x0, y0, x1, y1], 14, outline=BLACK, width=3, fill=fill)
        d.text((x0 + 16, y0 + 10), title, fill=BLACK, font=font(22, True))
        if sub:
            d.text((x0 + 16, y0 + 40), sub, fill=(90, 90, 100), font=font(16))

    def dot(x, y):
        d.ellipse([x - 9, y - 9, x + 9, y + 9], fill=(230, 200, 90), outline=BLACK, width=2)

    # screen and buttons: pins on the right edge, wire straight across to the pad
    box(40, 110, 420, ys[7] + 40, "Screen", "8 wires, 30 AWG")
    box(40, ys[9] - 80, 420, ys[13] + 40, "Buttons and shutter", "tactile switches: use two diagonal feet")
    for i in list(range(8)) + list(range(9, 14)):
        pad, pin, col = rows[i]
        d.text((60, ys[i] - 12), pin, fill=BLACK, font=font(21))
        dot(420, ys[i])
        d.line([(429, ys[i]), (bx - 12, ys[i])], fill=col, width=5)
    d.text((440, ys[13] + 12), "chain the GND legs of all four together, then one wire to GND", fill=(90, 90, 100), font=font(15))

    # power: battery -> charge module -> switch -> 5V; OUT- -> GND; B+ -> divider -> GPIO 1
    y5, yg, y1 = ys[15], ys[16], ys[17]
    box(40, y5 - 70, 260, y5 + 80, "Battery", "plug cut off", fill=(250, 238, 200))
    box(360, y5 - 70, 620, y1 + 50, "Charge module", "", fill=(220, 232, 252))
    labels = [("B+", y5 - 10), ("B-", y5 + 40)]
    for lab, yy in labels:
        d.text((376, yy - 12), lab.replace("-", "−"), fill=BLACK, font=font(20))
        dot(360, yy)
    d.line([(260, y5 - 10), (351, y5 - 10)], fill=RED, width=6)        # red lead to B+
    d.line([(260, y5 + 40), (351, y5 + 40)], fill=BLACK, width=6)      # black lead to B-
    d.text((270, y5 - 40), "red", fill=RED, font=font(15)); d.text((270, y5 + 48), "black", fill=BLACK, font=font(15))
    for lab, yy in [("OUT+", y5), ("OUT−", yg)]:
        d.text((540, yy - 12), lab, fill=BLACK, font=font(20))
        dot(620, yy)
    # slide switch: OUT+ to the middle leg, one end leg on to 5V
    box(700, y5 - 78, 900, y5 - 18, "Slide switch")
    for i, lab in enumerate(["end", "middle", "end"]):
        lx = 730 + i * 70
        dot(lx, y5 - 18)
        d.text((lx - 20, y5 - 112), lab, fill=(90, 90, 100), font=font(14))
    d.line([(629, y5), (800, y5), (800, y5 - 27)], fill=RED, width=6)           # OUT+ -> middle
    d.line([(870, y5 - 27), (870, y5 + 12), (bx - 12, y5 + 12), (bx - 12, y5)], fill=RED, width=6)   # end -> 5V
    d.text((910, y5 - 70), "the other end leg: no wire", fill=(90, 90, 100), font=font(15))
    d.line([(629, yg), (bx - 12, yg)], fill=BLACK, width=6)                       # OUT- -> GND
    # divider, beside the board's GPIO 1 pad: B+ -> 100k -> (GPIO 1) -> 100k -> GND
    dy0 = y1 + 34
    d.rounded_rectangle([700, dy0, 1180, dy0 + 80], 14, outline=BLACK, width=3, fill=(248, 247, 250))
    d.text((700, dy0 + 90), "Battery level: two 100 kΩ resistors, soldered at the GPIO 1 pad, each in heat-shrink",
           fill=BLACK, font=font(17, True))
    rr = dy0 + 40
    r1, r2 = (760, rr), (1000, rr)
    d.line([(352, y5 - 10), (330, y5 - 10), (330, rr), (r1[0], rr)], fill=RED, width=3)
    for (x, yy) in (r1, r2):
        d.rectangle([x, yy - 10, x + 70, yy + 10], outline=BLACK, width=2, fill="white")
        d.text((x + 16, yy - 9), "100k", fill=BLACK, font=font(15))
    d.line([(r1[0] + 70, rr), (r2[0], rr)], fill=SIG, width=3)
    d.line([(910, rr), (910, y1), (bx - 12, y1)], fill=SIG, width=5)          # midpoint -> GPIO 1
    d.line([(r2[0] + 70, rr), (1215, rr), (1215, yg)], fill=BLACK, width=3)    # bottom resistor -> the GND wire
    d.ellipse([1209, yg - 6, 1221, yg + 6], fill=BLACK)
    d.text((560, rr - 24), "from B+", fill=RED, font=font(15))
    y = dy0 + 130

    # legend
    ly = y + 50
    for i, (col, lab) in enumerate([(RED, "power +: 26 AWG from the battery through the module and switch to 5V; 30 AWG for the screen's VCC"),
                                    (BLACK, "GND: 26 AWG for the module's OUT−; 30 AWG elsewhere"), (SIG, "signals: 30 AWG, any light colour")]):
        d.line([(40, ly + i * 32), (110, ly + i * 32)], fill=col, width=7)
        d.text((124, ly + i * 32 - 12), lab, fill=BLACK, font=font(18))
    img = img.crop((0, 0, 1700, ly + 110))
    img.save(os.path.join(OUT, "wiring-v3.png"))


# ---------------------------------------------------------------- 2. routes inside the body, seen from the front
def routes(style):
    c = coords(style)
    W, H = c["W"], c["H"]
    S = 9.0                                 # pixels per mm
    pad = 60
    top_extra = c["tp_h"]
    img = Image.new("RGB", (int(W * S) + 2 * pad + 470, int((H + top_extra) * S) + 2 * pad + 60), "white")
    d = ImageDraw.Draw(img)

    def P(x, y):   # model (seen from the back) -> picture seen from the FRONT, back plate down
        return (pad + (W - x) * S, pad + (H + top_extra - y) * S + 40)

    def rect(r, **kw):
        x, y, w, h = r
        a, b = P(x + w, y + h), P(x, y)
        d.rounded_rectangle([a[0], a[1], b[0], b[1]], 6, **kw)

    def label(x, y, text, size=17, col=BLACK, bold=False):
        q = P(x, y)
        d.text((q[0], q[1]), text, fill=col, font=font(size, bold), anchor="mm")

    title = "Clear" if style == "clear" else "Retro"
    d.text((pad, 14), f"{title} body: wire routes, seen from the front with the front plate off", fill=BLACK, font=font(26, True))
    rect([0, 0, W, H], outline=GREY, width=4)
    if top_extra:
        rect([0, H, W, top_extra], outline=GREY, width=3, fill=(240, 240, 236))
        label(W / 2, H + top_extra / 2, "top plate (shutter, switch)", 16, GREY)
    for p in c["posts"]:
        q = P(*p)
        d.ellipse([q[0] - 2.4 * S, q[1] - 2.4 * S, q[0] + 2.4 * S, q[1] + 2.4 * S], outline=GREY, width=2)
    rect(c["screen"], outline=(80, 80, 90), width=2)
    rect(c["carrier"], fill=(255, 214, 180), outline=(200, 110, 40), width=2)
    for b in c["buttons"]:
        q = P(*b)
        d.ellipse([q[0] - 2.5 * S, q[1] - 2.5 * S, q[0] + 2.5 * S, q[1] + 2.5 * S], fill=(255, 160, 90))
    rect(c["battery"], fill=(250, 238, 200), outline=(170, 140, 60), width=2)
    rect(c["board"], fill=(196, 226, 206), outline=(42, 107, 74), width=3)
    rect(c["charger"], fill=(205, 222, 250), outline=(60, 90, 200), width=2)
    bx, by, bw, bh = c["board"]
    label(bx + bw / 2 + 10, by + bh / 2 + 4, "board (component side up)", 16, (42, 107, 74), True)
    label(bx + bw / 2 + 10, by + bh / 2 - 3, "screen underneath, glass down", 14, (80, 80, 90))
    q = P(*c["lens"]); d.ellipse([q[0] - 4 * S, q[1] - 4 * S, q[0] + 4 * S, q[1] + 4 * S], fill=(40, 40, 46))
    ba = c["battery"]; label(ba[0] + ba[2] / 2, ba[1] + ba[3] / 2 + 8, "battery", 17, (150, 120, 40), True)
    ch = c["charger"]; label(ch[0] + ch[2] / 2, ch[1] - 2.2, "charge module (pads at its inner end)", 14, (60, 90, 200))
    car = c["carrier"]; label(car[0] + car[2] / 2, car[1] + car[3] + 1.6, "button carrier (on the back plate)", 14, (200, 110, 40))
    sh, swc = c["shutter"], c["switch"]
    q = P(sh[0], sh[1] + 3); d.rectangle([q[0] - 2.5 * S, q[1] - 2.5 * S, q[0] + 2.5 * S, q[1] + 2.5 * S], fill=(255, 140, 61))
    label(sh[0], sh[1] + 9, "shutter", 15)
    q = P(swc[0], swc[1] + 1.8); d.rectangle([q[0] - 4.3 * S, q[1] - 1.8 * S, q[0] + 4.3 * S, q[1] + 1.8 * S], fill=(150, 150, 160))
    label(swc[0], swc[1] + 5.4, "switch", 15)
    # USB ports
    usb_y = c["usb"][0]
    label(W + 2.6, usb_y, "USB-C\n(board)", 13, (60, 60, 70))
    label(-3.2, ch[1] + ch[3] / 2, "USB-C\n(charge)", 13, (60, 60, 70))

    top_row = by + bh - 1.5          # the board's pad rows run along its long edges
    pads_x = bx + 8                  # the board's end nearest the battery
    lane_y = ba[1] + ba[3] + 2.0     # the lane above the battery
    chg_pads = (ch[0] + ch[2] - 2.0, ch[1] + ch[3] / 2)
    routes = {
        "Battery + / − to charge module B+ / B−": (RED, [(ba[0] + ba[2] - 6, ba[1] + ba[3]), (ba[0] + ba[2] - 6, lane_y),
                                                          (chg_pads[0], lane_y), chg_pads]),
        "Module OUT+ to switch (middle leg)": (RED, [chg_pads, (chg_pads[0] + 3, lane_y + 0.6), (swc[0], lane_y + 0.6), (swc[0], swc[1])]),
        "Switch (end leg) to board 5V": (RED, [(swc[0] + 2.5, swc[1]), (swc[0] + 2.5, lane_y + 1.2), (pads_x, lane_y + 1.2), (pads_x, top_row)]),
        "Module OUT− to board GND": (BLACK, [chg_pads, (chg_pads[0] + 4.5, lane_y + 1.8), (pads_x + 3, lane_y + 1.8), (pads_x + 3, top_row)]),
        "Battery level: module B+ to the divider at GPIO 1": (RED, [chg_pads, (chg_pads[0] + 6, lane_y + 2.4), (pads_x + 6, lane_y + 2.4), (pads_x + 6, top_row)]),
        "Shutter to GPIO 48 and GND": (SIG, [(sh[0], sh[1]), (sh[0], lane_y + 3.0) if style == "clear" else (sh[0], top_row + 6),
                                             (pads_x + 9, lane_y + 3.0) if style == "clear" else (sh[0] - 3, top_row + 6),
                                             (pads_x + 9, top_row) if style == "clear" else (sh[0] - 3, top_row)]),
        "Buttons: left, OK, right + GND (4 wires)": (SIG, [(car[0] + car[2] / 2, car[1]), (car[0] + car[2] / 2, top_row)]),
        "Screen (8 wires), through the sled's slot": (SIG, [(c["screen"][0] + c["screen"][2] - 2.3, c["screen"][1] + c["screen"][3] / 2),
                                                             (bx + bw - 6, c["screen"][1] + c["screen"][3] / 2), (bx + bw - 6, top_row)]),
    }
    lengths = {}
    for name, (col, pts) in routes.items():
        d.line([P(*p) for p in pts], fill=col, width=5, joint="curve")
        q = P(*pts[-1]); d.ellipse([q[0] - 6, q[1] - 6, q[0] + 6, q[1] + 6], fill=col)
        mm = sum(math.dist(a, b) for a, b in zip(pts, pts[1:]))
        # +10 mm slack, +25 mm for running along the board's pad row to the pad, rounded up to 5 mm
        lengths[name] = int(math.ceil((mm + 10 + 25) / 5) * 5)
    lx = int(W * S) + 2 * pad
    d.text((lx, 70), "Cut lengths (route + slack)", fill=BLACK, font=font(20, True))
    for i, (name, (col, _)) in enumerate(routes.items()):
        y = 110 + i * 62
        d.line([(lx, y + 10), (lx + 40, y + 10)], fill=col, width=6)
        d.text((lx + 52, y - 4), name, fill=BLACK, font=font(16))
        d.text((lx + 52, y + 18), f"about {lengths[name]} mm", fill=(90, 90, 100), font=font(16))
    d.text((lx, 110 + len(routes) * 62 + 10), "The board's pads run along its long edges.\nWires end at the edge nearest them;\n"
           "follow the row to the labelled pad.", fill=(90, 90, 100), font=font(15))
    img.save(os.path.join(OUT, f"routes-v3-{style}.png"))
    return lengths


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    schematic()
    lengths = {s: routes(s) for s in ("clear", "retro")}
    json.dump(lengths, open(os.path.join(OUT, "wire-lengths-v3.json"), "w"), indent=1)
    print(json.dumps(lengths, indent=1))
