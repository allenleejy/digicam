"""Build index.html (the published prototype) from src.html: inlines the Spleen font and the icon sprite."""
import base64, glob, os, re

HERE = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(HERE, "src.html")).read()
font = base64.b64encode(open(os.path.join(HERE, "..", "..", "..", "firmware", "web", "spleen.woff"), "rb").read()).decode()
symbols = []
for f in sorted(glob.glob(os.path.join(HERE, "..", "icons", "*.svg"))):
    inner = re.search(r"<svg[^>]*>(.*)</svg>", open(f).read(), re.S).group(1)
    symbols.append(f'<symbol id="{os.path.basename(f)[:-4]}" viewBox="0 0 256 256" fill="currentColor">{inner}</symbol>')
out = src.replace("/*FONT*/", font).replace("<!--SPRITE-->", "".join(symbols))
open(os.path.join(HERE, "index.html"), "w").write(out)
print("index.html", len(out), "bytes")

# local preview with the skeleton the Artifact host adds at publish time
skeleton = ('<!doctype html><html><head><meta charset="utf-8">'
            '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">'
            '<style>:root{color-scheme:light;padding-top:env(safe-area-inset-top);padding-bottom:env(safe-area-inset-bottom)}'
            'body{margin:0;font:14px system-ui}img{max-width:100%}[hidden]{display:none!important}</style></head><body>')
open(os.path.join(HERE, "preview.html"), "w").write(skeleton + out + "</body></html>")
