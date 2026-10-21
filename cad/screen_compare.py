"""Draw the back of camera 1 to scale with each screen option, showing a real 4:3 photo
the way the camera would display it (whole photo, black bars where the shapes differ).

    python3 screen_compare.py [photo.jpg]   ->  renders/screen-compare.png

Body layout and sizes match instantsnap_body.scad. Screen sizes are Waveshare's published
module and active-area dimensions (landscape); generic modules differ slightly.
"""

import os
import sys

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
PHOTO = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "..", "host", "cache", "DSC_0013.JPG")
OUT = os.path.join(HERE, "renders", "screen-compare.png")

S = 6                     # pixels per mm
W, H, R, WALL = 116, 66, 7, 2.2
PANEL_W, PANEL_H = int(W * S) + 80, int(H * S) + 190

# name, pixels (landscape), module size mm, active area mm, verdict
SCREENS = [
    ("1.14 in", (240, 135), (35.0, 32.0), (24.91, 14.86), "fits"),
    ("1.69 in", (280, 240), (39.0, 31.5), (32.63, 27.97), "fits"),
    ("2.0 in", (320, 240), (58.0, 35.0), (40.80, 30.60), "recommended"),
    ("2.4 in", (320, 240), (70.5, 43.3), (48.96, 36.72), "too big"),
]

BOARD_CX = 46 + 67 / 2    # screen centred over the board, as in the model
SCREEN_Y0 = 3.0

FONT = "/System/Library/Fonts/Helvetica.ttc"


def font(size, bold=False):
    try:
        return ImageFont.truetype(FONT, size, index=1 if bold else 0)
    except OSError:
        return ImageFont.load_default()


def to_px(ox, oy, x, y):
    """Back-view mm (origin bottom-left, y up) to image pixels."""
    return ox + x * S, oy + (H - y) * S


def rect(draw, ox, oy, x, y, w, h, **kw):
    x0, y1 = to_px(ox, oy, x, y)
    x1, y0 = to_px(ox, oy, x + w, y + h)
    r = kw.pop("r", 0)
    draw.rounded_rectangle([x0, y0, x1, y1], radius=r * S, **kw)


def circle(draw, ox, oy, cx, cy, d, **kw):
    x, y = to_px(ox, oy, cx, cy)
    draw.ellipse([x - d * S / 2, y - d * S / 2, x + d * S / 2, y + d * S / 2], **kw)


def fit(photo_size, box):
    pw, ph = photo_size
    bw, bh = box
    k = min(bw / pw, bh / ph)
    return pw * k, ph * k


def panel(photo, spec):
    name, px, module, active, verdict = spec
    img = Image.new("RGBA", (PANEL_W, PANEL_H), (246, 243, 238, 255))
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    ox, oy = 40, 70

    # top-edge controls stand above the body
    rect(d, ox, oy, 96, H - 0.5, 8, 2.6, r=1.3, fill=(255, 138, 61, 255))           # shutter
    rect(d, ox, oy, 79, H - 0.6, 6, 1.2, r=0.4, fill=(70, 70, 80, 255))             # power switch

    # body: lilac frame seen around the clear back plate
    rect(d, ox, oy, 0, 0, W, H, r=R, fill=(201, 184, 242, 255))
    rect(d, ox, oy, WALL, WALL, W - 2 * WALL, H - 2 * WALL, r=R - WALL, fill=(222, 228, 236, 255))
    # internals seen through the clear plate
    rect(d, ox, oy, 9, 2.6, 35, 52, r=2, fill=(196, 198, 204, 255))                  # battery
    rect(d, ox, oy, 46, 8.5, 67, 28, r=1, fill=(52, 104, 78, 255))                   # board
    rect(d, ox, oy, 95.5, 38.5, 18, 14, r=0.5, fill=(91, 141, 239, 255))             # charge module
    for sx, sy in [(5, 5), (W - 5, 5), (W - 5, H - 5), (17, H - 5)]:
        circle(d, ox, oy, sx, sy, 3.6, fill=(150, 150, 160, 255))
    circle(d, ox, oy, 6, H - 6, 4.2, fill=(246, 243, 238, 255), outline=(150, 140, 180, 255), width=2)  # keychain
    rect(d, ox, oy, 46.8, 43.8, 18.4, 14.4, r=2.4, fill=(255, 138, 61, 255))        # viewfinder trim
    rect(d, ox, oy, 48, 45, 16, 12, r=1.6, fill=(170, 200, 225, 255))                # viewfinder window
    for bx in (84, 96, 108):
        circle(d, ox, oy, bx, 50, 6.0, fill=(255, 138, 61, 255))

    # screen module and its picture
    mw, mh = module
    aw, ah = active
    mx = min(BOARD_CX - mw / 2, W - 5 - 3.2 - 0.6 - mw)   # clear of the screw post, as in the model
    cx = mx + mw / 2
    too_big = verdict == "too big"
    rect(d, ox, oy, mx, SCREEN_Y0, mw, mh, r=1, fill=(30, 34, 52, 235),
         outline=(220, 40, 40, 255) if too_big else None, width=3 if too_big else 0)
    ax, ay = cx - aw / 2, SCREEN_Y0 + (mh - ah) / 2
    rect(d, ox, oy, ax, ay, aw, ah, fill=(0, 0, 0, 255))
    img = Image.alpha_composite(img, layer)

    # the photo, fitted whole inside the active area
    iw, ih = fit(photo.size, (aw, ah))
    pic = photo.resize((max(1, round(iw * S)), max(1, round(ih * S))), Image.LANCZOS)
    px0, py0 = to_px(ox, oy, cx - iw / 2, ay + ah / 2 + ih / 2)
    img.paste(pic, (round(px0), round(py0)))

    # captions
    d = ImageDraw.Draw(img)
    fill_pct = round(100 * iw * ih / (aw * ah))
    title = f"{name}  ·  {px[0]}×{px[1]}"
    d.text((ox, 14), title, fill=(42, 37, 48), font=font(26, True))
    if verdict == "recommended":
        tw = d.textlength(title, font=font(26, True))
        d.rounded_rectangle([ox + tw + 14, 14, ox + tw + 196, 44], radius=14, fill=(47, 158, 91))
        d.text((ox + tw + 28, 18), "recommended", fill="white", font=font(20, True))
    lines = [
        f"Photo shown {iw:.1f} × {ih:.1f} mm, fills {fill_pct}% of the screen",
        "Same shape as your photos: no bars, live view pixel-for-pixel" if fill_pct == 100 and not too_big
        else ("Doesn't fit: overlaps the viewfinder and buttons, wider than the board bay" if too_big
              else f"Black bars {'left and right' if aw / ah > 4 / 3 else 'top and bottom'}"),
    ]
    yb = oy + H * S + 22
    for i, t in enumerate(lines):
        d.text((ox, yb + i * 30), t, fill=(90, 84, 100) if i else (42, 37, 48), font=font(21))
    return img


def main():
    photo = Image.open(PHOTO).convert("RGB")
    panels = [panel(photo, s) for s in SCREENS]
    sheet = Image.new("RGB", (PANEL_W * 2, PANEL_H * 2 + 70), (246, 243, 238))
    d = ImageDraw.Draw(sheet)
    d.text((40, 22), "Camera 1, back view, to scale (116 × 66 mm body). Photos are 4:3.",
           fill=(42, 37, 48), font=font(28, True))
    for i, p in enumerate(panels):
        sheet.paste(p, ((i % 2) * PANEL_W, 70 + (i // 2) * PANEL_H))
    # 10 mm scale bar
    x0, y0 = PANEL_W * 2 - 40 - 10 * S, 34
    d.line([x0, y0, x0 + 10 * S, y0], fill=(42, 37, 48), width=3)
    d.text((x0 - 70, y0 - 13), "10 mm", fill=(42, 37, 48), font=font(20))
    sheet.save(OUT)
    print(OUT)


if __name__ == "__main__":
    main()
