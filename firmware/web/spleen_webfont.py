"""Turn the camera's own Spleen 8x16 bitmap font (firmware/main/font_spleen.c) into a small
web font, so the phone page can use the same lettering as the camera's screen.

    ../../cad/.venv/bin/python spleen_webfont.py      # writes spleen.woff next to this file

Each lit pixel becomes a square; runs of pixels in a row are merged into one rectangle.
One pixel is 1/16 em, so the font is pixel-sharp at 16, 32 and 48 px.
Spleen is BSD-2-Clause (Frederic Cambus); the licence travels in the font's name table.
"""

import os
import re

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "main", "font_spleen.c")
OUT = os.path.join(HERE, "spleen.woff")

W, H, ASCENT = 8, 16, 12      # cell size; rows 12-15 hold descenders
PX = 64                       # font units per pixel, so 1 em = 16 px = 1024 units


def glyph_rows():
    text = open(SRC, encoding="latin-1").read()
    body = re.search(r"FONT_BODY_bits\[[^]]*\]\s*=\s*\{(.*?)\};", text, re.S).group(1)
    data = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", body)]
    for i, code in enumerate(range(32, 127)):
        yield code, data[i * H:(i + 1) * H]


def draw(rows):
    pen = TTGlyphPen(None)
    for r, bits in enumerate(rows):
        top = (ASCENT - r) * PX
        x = 0
        while x < W:
            if bits >> x & 1:                     # LSB is the leftmost pixel
                start = x
                while x < W and bits >> x & 1:
                    x += 1
                x0, x1, y0, y1 = start * PX, x * PX, top - PX, top
                pen.moveTo((x0, y0)); pen.lineTo((x0, y1)); pen.lineTo((x1, y1)); pen.lineTo((x1, y0))
                pen.closePath()
            else:
                x += 1
    return pen.glyph()


def main():
    names, glyphs, cmap = [".notdef"], {".notdef": TTGlyphPen(None).glyph()}, {}
    for code, rows in glyph_rows():
        name = "uni%04X" % code
        names.append(name)
        glyphs[name] = draw(rows)
        cmap[code] = name

    fb = FontBuilder(H * PX, isTTF=True)
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap(cmap)
    fb.setupGlyf(glyphs)
    fb.setupHorizontalMetrics({n: (W * PX, 0) for n in names})
    fb.setupHorizontalHeader(ascent=ASCENT * PX, descent=-(H - ASCENT) * PX)
    fb.setupNameTable({
        "familyName": "Spleen",
        "styleName": "Regular",
        "copyright": "Copyright (c) 2018-2026, Frederic Cambus. BSD-2-Clause.",
    })
    fb.setupOS2(sTypoAscender=ASCENT * PX, sTypoDescender=-(H - ASCENT) * PX, sTypoLineGap=0,
                usWinAscent=ASCENT * PX, usWinDescent=(H - ASCENT) * PX)
    fb.setupPost()
    fb.font.flavor = "woff"
    fb.save(OUT)
    print(f"{OUT}: {os.path.getsize(OUT)} bytes")


if __name__ == "__main__":
    main()
