"""Digicam simulator: runs the real camera over USB and shows a virtual camera screen in the browser.

    python server.py                 # auto-detects the board, opens http://127.0.0.1:8765
    python server.py --name Sam      # name on the boot splash

The board does the real work (sensor, photos, video, SD card). This server only bridges
the serial link to the page in web/, which stands in for the screen, dial and shutter.
"""

import argparse
import glob
import json
import mimetypes
import os
import re
import shutil
import struct
import threading
import time
import webbrowser
import zlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

from link import DeviceError, Link

HERE = os.path.dirname(os.path.abspath(__file__))
WEB = os.path.join(HERE, "web")
CACHE = os.path.join(HERE, "cache")
PHONE_PAGE = os.path.join(HERE, "..", "firmware", "web", "phone.html")
NAME_RE = re.compile(r"^(DSC|MOV)_\d{4}\.(JPG|AVI)$")

# Slow commands get longer replies; everything else answers within a second.
TIMEOUTS = {"photo": 15.0, "rec_stop": 30.0, "rec_start": 10.0, "mode": 10.0, "list": 10.0}


def screen_png(w, h, rgb565_be, scale=1):
    """The camera's screen (RGB565, big-endian) as PNG bytes."""
    rows = []
    for y in range(h):
        row = bytearray()
        for x in range(w):
            v = rgb565_be[2 * (y * w + x)] << 8 | rgb565_be[2 * (y * w + x) + 1]
            row += bytes(((v >> 11) * 255 // 31, (v >> 5 & 63) * 255 // 63, (v & 31) * 255 // 31)) * scale
        rows += [b"\0" + bytes(row)] * scale

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w * scale, h * scale, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(b"".join(rows), 6)) + chunk(b"IEND", b""))


FRAME = {"doc": None}   # the phone page's frame mailbox, for trying the page on the laptop


class Bridge:
    def __init__(self, port, owner):
        self.fixed_port = port               # None: find the board, and keep looking until it's plugged in
        self.port = port or "(no board yet)"
        self.owner = owner
        self.link = None
        self.hello = None
        self.error = None
        self.media_lock = threading.Lock()   # one file transfer at a time
        self.gen_lock = threading.Lock()
        self.gen = 0                          # bumps on every media request
        self.big_transfer = False             # a full photo or video is downloading
        self.rate = {"t": time.time(), "bytes": 0, "kbps": 0.0}
        threading.Thread(target=self._connect_loop, daemon=True).start()

    def _connect_loop(self):
        while True:
            if self.hello is not None:
                time.sleep(0.5)
                if self.link.reopened:          # the board was unplugged and is back
                    self.link.reopened = False
                    self.port = self.link.port_name
                    self.hello = None
                    print("board reconnected")
                continue
            try:
                if self.link is None:
                    found = self.fixed_port or find_port()
                    if not found:
                        self.port = "USB"
                        raise DeviceError("no board found: plug in the USB-C port marked COM")
                    self.port = found
                    self.link = Link(self.port)
                self.hello = self.link.connect()
                self.error = None
                print(f"connected: firmware {self.hello['fw']}, sensor {self.hello['sensor']}, "
                      f"SD {'ok' if self.hello['sd'] else 'missing'}")
            except Exception as e:   # board unplugged, still booting, or port busy
                self.error = str(e)
                if self.link:
                    self.link.close()
                    self.link = None
                time.sleep(2)

    def state(self, since_log, since_event):
        link = self.link
        if not (link and self.hello):
            return {"connected": False, "port": self.port, "error": self.error, "owner": self.owner}
        now = time.time()
        if now - self.rate["t"] >= 1:
            self.rate["kbps"] = (link.bytes_in - self.rate["bytes"]) / (now - self.rate["t"]) / 1024
            self.rate.update(t=now, bytes=link.bytes_in)
        return {
            # Any frame counts: during a long download the board sends file data, not status.
            "connected": now - link.last_rx < 3,
            "port": self.port,
            "owner": self.owner,
            "hello": self.hello,
            "status": link.status,
            "link": {"kbps": round(self.rate["kbps"], 1), "crc_errors": link.crc_errors,
                     "frame": link.frame_size},
            "logs": [l for l in link.logs if l[0] > since_log],
            "events": [e for e in link.events if e[0] > since_event],
        }

    def command(self, body):
        cmd = body.pop("cmd")
        reply = self.link.command(cmd, timeout=TIMEOUTS.get(cmd, 5.0), **body)
        if cmd == "delete" and reply.get("ok"):
            for thumb in (False, True):
                path = self.cache_path(body["name"], thumb)
                if os.path.exists(path):
                    os.remove(path)
        return reply

    @staticmethod
    def cache_path(name, thumb):
        return os.path.join(CACHE, ("thumb_" if thumb else "") + name)

    def media(self, name, thumb):
        path = self.cache_path(name, thumb)
        if not os.path.exists(path):
            # A newer request (the user turned the dial on) cancels a big download in
            # progress: any command makes the board stop sending, and get_file stops resuming.
            with self.gen_lock:
                self.gen += 1
                my_gen = self.gen
            if self.big_transfer:
                try:
                    self.link.command("status")
                except DeviceError:
                    pass
            with self.media_lock:
                if not os.path.exists(path):
                    self.big_transfer = not thumb
                    try:
                        data = self.link.get_file(name, thumb=thumb,
                                                  cancelled=lambda: not thumb and self.gen != my_gen)
                    finally:
                        self.big_transfer = False
                    os.makedirs(CACHE, exist_ok=True)
                    with open(path + ".part", "wb") as f:
                        f.write(data)
                    os.replace(path + ".part", path)
        with open(path, "rb") as f:
            return f.read()


class Handler(BaseHTTPRequestHandler):
    bridge: Bridge = None

    def log_message(self, fmt, *args):
        pass

    def _send(self, code, body, ctype="application/json", headers=None):
        if isinstance(body, (dict, list)):
            body = json.dumps(body).encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        for k, v in (headers or {}).items():
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        url = urlparse(self.path)
        q = parse_qs(url.query)
        b = self.bridge
        try:
            if url.path == "/":
                return self._static("index.html")
            if url.path.startswith("/static/"):
                return self._static(url.path[len("/static/"):])
            if url.path == "/api/state":
                return self._send(200, b.state(int(q.get("log", [0])[0]), int(q.get("event", [0])[0])))
            if url.path == "/api/lcd.png":
                # what's on the camera's own screen right now (menus, QR codes), for the mirror
                reply, chunks = b.link._request("shot", 5.0, True, {})
                if not reply.get("ok"):
                    return self._send(503, {"ok": False})
                data = b"".join(chunks[k] for k in sorted(chunks))
                return self._send(200, screen_png(reply["w"], reply["h"], data), "image/png",
                                  {"Cache-Control": "no-store"})
            if url.path == "/stream.mjpg":
                return self._stream()
            # The phone page (firmware/web/phone.html), served here the way the camera serves
            # it over Wi-Fi, so it can be tried in a browser through the USB link
            if url.path == "/phone/":
                with open(PHONE_PAGE, "rb") as f:
                    return self._send(200, f.read(), "text/html; charset=utf-8")
            if url.path == "/phone/api/frame":
                # the frame mailbox, kept in memory here (the camera keeps it in /sdcard/frame.json)
                if FRAME["doc"] is None:
                    return self._send(204, b"")
                return self._send(200, FRAME["doc"])
            if url.path == "/phone/api/list":
                r = b.command({"cmd": "list"})
                st = (b.link.status if b.link else {}) or {}
                mv = st.get("battery_mv") or 0
                return self._send(200, {"camera": "digicam-dev", "owner": b.owner or "",
                                        "battery": None if not mv else st.get("battery"), "items": r.get("items", [])})
            m = re.match(r"^/phone/(thumb|file)/([A-Z0-9_.]+)$", url.path)
            if m and NAME_RE.match(m.group(2)):
                name, thumb = m.group(2), m.group(1) == "thumb"
                data = b.media(name, thumb)
                return self._send(200, data, "image/jpeg" if thumb or name.endswith(".JPG") else "video/x-msvideo")
            m = re.match(r"^/api/media/([A-Z0-9_.]+)$", url.path)
            if m and NAME_RE.match(m.group(1)):
                name = m.group(1)
                thumb = q.get("thumb", ["0"])[0] == "1"
                data = b.media(name, thumb)
                ctype = "image/jpeg" if thumb or name.endswith(".JPG") else "video/x-msvideo"
                headers = {"Content-Disposition": f'attachment; filename="{name}"'} if "download" in q else {}
                return self._send(200, data, ctype, headers)
            self._send(404, {"error": "not found"})
        except (DeviceError, OSError, AttributeError) as e:
            self._send(503, {"ok": False, "error": str(e)})

    def do_POST(self):
        if self.path == "/phone/api/frame":
            n = int(self.headers.get("Content-Length", 0))
            if n > 16 * 1024:
                self.close_connection = True
                return self._send(413, {"ok": False})
            body = self.rfile.read(n)
            try:
                ok = isinstance(json.loads(body), dict)
            except ValueError:
                ok = False
            if not ok:
                return self._send(400, {"ok": False})
            FRAME["doc"] = body
            return self._send(200, {"ok": True})
        if self.path == "/phone/api/delete":
            name = self.rfile.read(int(self.headers.get("Content-Length", 0))).decode().strip()
            if not NAME_RE.match(name):
                return self._send(400, {"ok": False})
            return self._send(200, self.bridge.command({"cmd": "delete", "name": name}))
        if self.path != "/api/cmd":
            return self._send(404, {"error": "not found"})
        try:
            body = json.loads(self.rfile.read(int(self.headers.get("Content-Length", 0))))
            self._send(200, self.bridge.command(body))
        except (DeviceError, AttributeError, KeyError, ValueError) as e:
            self._send(503, {"ok": False, "error": str(e)})

    def _static(self, rel):
        path = os.path.normpath(os.path.join(WEB, rel))
        if not path.startswith(WEB) or not os.path.isfile(path):
            return self._send(404, {"error": "not found"})
        with open(path, "rb") as f:
            self._send(200, f.read(), mimetypes.guess_type(path)[0] or "application/octet-stream")

    def _stream(self):
        self.send_response(200)
        self.send_header("Content-Type", "multipart/x-mixed-replace; boundary=frame")
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        seq = -1
        try:
            while True:
                link = self.bridge.link
                if not link:
                    time.sleep(0.5)
                    continue
                seq, frame, _ = link.wait_frame(seq, timeout=1.0)
                if frame:
                    self.wfile.write(b"--frame\r\nContent-Type: image/jpeg\r\nContent-Length: "
                                     + str(len(frame)).encode() + b"\r\n\r\n" + frame + b"\r\n")
        except (BrokenPipeError, ConnectionResetError):
            pass


def find_port():
    ports = sorted(glob.glob("/dev/cu.wchusbserial*") + glob.glob("/dev/cu.usbserial*")
                   + glob.glob("/dev/ttyUSB*"))
    return ports[0] if ports else None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", help="serial port (default: auto-detect the CH340)")
    ap.add_argument("--http", type=int, default=8765, help="web port (default 8765)")
    ap.add_argument("--name", default="", help="name shown on the boot splash")
    ap.add_argument("--no-browser", action="store_true", help="don't open the browser")
    args = ap.parse_args()

    # Start with an empty cache: a different or reformatted card reuses file names
    shutil.rmtree(CACHE, ignore_errors=True)
    Handler.bridge = Bridge(args.port, args.name)
    server = ThreadingHTTPServer(("127.0.0.1", args.http), Handler)
    server.daemon_threads = True
    url = f"http://127.0.0.1:{args.http}/"
    print(f"Digicam simulator on {url}  (board on {Handler.bridge.port})")
    if not args.no_browser:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
