"""Build phone.html, the page the camera serves, from phone.src.html.

    python3 build.py

Puts the camera's pixel font (spleen.woff, from spleen_webfont.py) and the Phosphor icons
(icons/*.svg, MIT licence) inside the page, since the camera's Wi-Fi has no internet.
The firmware embeds phone.html and icon-180.png (main/CMakeLists.txt).
"""

import base64
import glob
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    src = open(os.path.join(HERE, "phone.src.html")).read()
    font = base64.b64encode(open(os.path.join(HERE, "spleen.woff"), "rb").read()).decode()
    symbols = []
    for f in sorted(glob.glob(os.path.join(HERE, "icons", "*.svg"))):
        inner = re.search(r"<svg[^>]*>(.*)</svg>", open(f).read(), re.S).group(1)
        symbols.append(f'<symbol id="{os.path.basename(f)[:-4]}" viewBox="0 0 256 256" fill="currentColor">{inner}</symbol>')
    used = set(re.findall(r'icon\("([a-z-]+)"', src))
    missing = used - {os.path.basename(f)[:-4] for f in glob.glob(os.path.join(HERE, "icons", "*.svg"))}
    if missing:
        raise SystemExit(f"icons missing from web/icons: {sorted(missing)}")
    out = src.replace("/*FONT*/", font).replace("<!--SPRITE-->", "".join(symbols))
    with open(os.path.join(HERE, "phone.html"), "w") as f:
        f.write(out)
    print(f"phone.html: {len(out)} bytes, {len(symbols)} icons")


if __name__ == "__main__":
    main()
