"""Bench tools for the camera while it's wired up on the desk.

    python bench.py shot [file.png]   save what's on the camera's screen (works with no screen fitted)
    python bench.py btn left|right|ok|hold|shutter   press a button from the laptop
    python bench.py status            camera, card, battery and memory
    python bench.py log [seconds]     print the camera's log (press real buttons to see them here)
    python bench.py lcd SWAP MX MY GX GY INV   line up a screen module, e.g. lcd 1 1 0 40 53 1
    python bench.py remote            press the camera's buttons from the keyboard:
                                      left/right arrows = ◀ ▶, return = ●, h = hold ●,
                                      space or up arrow = shutter, q = quit

Stop the simulator first: only one program can hold the serial port.
"""

import json
import os
import struct
import sys
import time
import zlib

from link import Link
from server import find_port, screen_png


def png(path, w, h, rgb565_be, scale=3):
    with open(path, "wb") as f:
        f.write(screen_png(w, h, rgb565_be, scale))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    what, args = sys.argv[1], sys.argv[2:]
    link = Link(find_port())
    link.connect()
    link.command("stream", on=False)   # hello turns the laptop stream on; the bench doesn't need it

    if what == "shot":
        reply, chunks = link._request("shot", 10.0, True, {})
        data = b"".join(chunks[k] for k in sorted(chunks))
        out = args[0] if args else time.strftime("shots/screen-%H%M%S.png")
        os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
        png(out, reply["w"], reply["h"], data)
        print(out)
    elif what == "btn":
        print(link.command("btn", b=args[0]))
    elif what == "status":
        link.command("status")
        time.sleep(0.3)
        print(json.dumps(link.status, indent=1))
    elif what == "log":
        end = time.time() + (float(args[0]) if args else 30)
        seen = 0
        while time.time() < end:
            for seq, t, line in list(link.logs):
                if seq > seen:
                    print(t, line, flush=True)
                    seen = seq
            time.sleep(0.2)
    elif what == "lcd":
        keys = ["swap", "mx", "my", "gx", "gy", "inv"]
        print(link.command("lcd", **{k: int(v) for k, v in zip(keys, args)}))
    elif what == "remote":
        remote(link)
    else:
        print(__doc__)


def remote(link):
    import termios
    import tty
    keys = {"\x1b[D": "left", "\x1b[C": "right", "\r": "ok", "\n": "ok", "h": "hold",
            " ": "shutter", "\x1b[A": "shutter"}
    print("Camera remote: arrows ◀ ▶, return ●, h hold ●, space shutter, q quit")
    fd = sys.stdin.fileno()
    old = termios.tcgetattr(fd)
    try:
        tty.setcbreak(fd)
        while True:
            k = os.read(fd, 3).decode(errors="ignore")
            if k in ("q", "Q", "\x03"):
                break
            if k in keys:
                link.command("btn", b=keys[k])
                print("  " + {"left": "◀", "right": "▶", "ok": "●", "hold": "hold ●", "shutter": "shutter"}[keys[k]], flush=True)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old)


if __name__ == "__main__":
    main()
