#!/bin/sh
# Renders every screen to out/*.png and out/sheet.png using the firmware's drawing code.
set -e
cd "$(dirname "$0")"
PY=../../cad/.venv/bin/python
mkdir -p out
[ -f photo.rgb565 ] || $PY - <<'PYEOF'
from PIL import Image
import glob
src = sorted(glob.glob("../../host/cache/DSC_*.JPG"))[-1]
im = Image.open(src).convert("RGB").resize((320, 240))
out = bytearray()
for r, g, b in im.getdata():
    v = (r >> 3) << 11 | (g >> 2) << 5 | b >> 3
    out += bytes((v >> 8, v & 255))
open("photo.rgb565", "wb").write(out)
PYEOF
QR=../managed_components/espressif__qrcode
cc -std=c11 -O1 -Wall -Wextra -Wno-unused-parameter -I../main -I$QR -o out/sim sim.c ../main/ui_draw.c ../main/gfx.c ../main/font_spleen.c $QR/qrcodegen.c
./out/sim
$PY - <<'PYEOF'
from PIL import Image, ImageDraw
import glob, os
files = sorted(glob.glob("out/*.ppm"))
tiles = []
for f in files:
    im = Image.open(f).resize((240 * 3, 135 * 3), Image.NEAREST)
    im.save(f[:-4] + ".png")
    os.remove(f)
    tiles.append((os.path.basename(f)[:-4], im))
cols = 4
w, h = 240 * 2, 135 * 2 + 30
sheet = Image.new("RGB", (cols * (w + 16) + 16, ((len(tiles) + cols - 1) // cols) * (h + 16) + 16), (236, 233, 242))
d = ImageDraw.Draw(sheet)
for i, (name, im) in enumerate(tiles):
    x, y = 16 + (i % cols) * (w + 16), 16 + (i // cols) * (h + 16)
    sheet.paste(im.resize((w, h - 30), Image.NEAREST), (x, y + 30))
    d.text((x, y + 8), name, fill=(40, 36, 50))
sheet.save("out/sheet.png")
print(len(tiles), "screens -> out/sheet.png")
PYEOF
