"""Phone-mode test over the camera's own Wi-Fi: speed, data loss and the frame mailbox.

Run it on a laptop joined to the camera's Wi-Fi (Phone mode on), then rejoin your usual
Wi-Fi. It needs only Python's standard library.

    python3 wifi_test.py                  # report in wifi-report.json next to this file
    python3 wifi_test.py --mailbox        # also run Allen's /api/frame tests (rewrites frame.json!)

What it measures:
  - /api/list, then every thumbnail and every full file: bytes, time, KB/s
  - each file against the list: same size, a whole JPEG (or AVI with every frame whole)
  - the same photo three times in a row, and three photos at once (like the grid does)
  - the frame mailbox as the camera has it now (and, with --mailbox, Allen's tests)
"""

import argparse
import http.client
import json
import os
import struct
import threading
import time
import urllib.error
import urllib.request

CAM = "http://192.168.4.1"
HERE = os.path.dirname(os.path.abspath(__file__))


def get(path, timeout=60):
    t0 = time.time()
    with urllib.request.urlopen(CAM + path, timeout=timeout) as r:
        data = r.read()
        return r.status, data, time.time() - t0


def post(path, body, timeout=20):
    req = urllib.request.Request(CAM + path, data=body, method="POST")
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return r.status, r.read()
    except urllib.error.HTTPError as e:
        return e.code, b""
    except (urllib.error.URLError, ConnectionError) as e:   # 413 closes the connection
        return f"closed ({e})", b""


def jpeg_ok(d):
    return d[:2] == b"\xff\xd8" and d.rstrip(b"\x00")[-2:] == b"\xff\xd9" and b"\xff\xda" in d


def avi_ok(d):
    if d[:4] != b"RIFF":
        return False, 0
    frames, p = 0, 224
    while p + 8 <= len(d) and d[p:p + 4] == b"00dc":
        n = struct.unpack_from("<I", d, p + 4)[0]
        if not jpeg_ok(d[p + 8:p + 8 + n]):
            return False, frames
        frames += 1
        p += 8 + n + (n & 1)
    return d[p:p + 4] == b"idx1", frames


def kbs(n, s):
    return round(n / 1024 / s) if s > 0 else None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mailbox", action="store_true", help="run Allen's /api/frame tests (replaces frame.json)")
    a = ap.parse_args()
    rep = {"when": time.strftime("%Y-%m-%d %H:%M:%S"), "problems": []}

    st, body, secs = get("/api/list")
    lst = json.loads(body)
    rep["list"] = {"ms": round(secs * 1000), "items": len(lst["items"]), "camera": lst.get("camera")}
    st, frame, _ = get("/api/frame")
    rep["frame_json_now"] = frame.decode() if st == 200 else f"HTTP {st}"

    files, total_b, total_s = [], 0, 0.0
    for it in lst["items"]:
        n = it["n"]
        try:
            _, th, ts = get("/thumb/" + n)
            _, data, fs = get("/file/" + n, timeout=300)
        except (http.client.IncompleteRead, urllib.error.URLError, ConnectionError, TimeoutError) as e:
            rep["problems"].append(f"{n}: download cut short or failed ({type(e).__name__})")
            print(f"{n}: download cut short or failed ({type(e).__name__})")
            continue
        total_b += len(data)
        total_s += fs
        e = {"name": n, "kb": round(len(data) / 1024), "file_s": round(fs, 2), "file_kb_s": kbs(len(data), fs),
             "thumb_ms": round(ts * 1000)}
        if len(data) != it["s"]:
            rep["problems"].append(f"{n}: got {len(data)} bytes, listed {it['s']}")
        if it["v"]:
            ok, frames = avi_ok(data)
            e["frames"] = frames
            if not ok:
                rep["problems"].append(f"{n}: video damaged after frame {frames}")
        elif not jpeg_ok(data):
            rep["problems"].append(f"{n}: not a whole JPEG")
        if not jpeg_ok(th):
            rep["problems"].append(f"{n}: thumbnail not a whole JPEG")
        files.append(e)
        print(f"{n}: {e['kb']} KB in {e['file_s']} s ({e['file_kb_s']} KB/s), thumbnail {e['thumb_ms']} ms")
    rep["files"] = files
    rep["overall_kb_s"] = kbs(total_b, total_s)

    photos = [i["n"] for i in lst["items"] if not i["v"]]
    if photos:
        rep["same_photo_3x_s"] = [round(get("/file/" + photos[0])[2], 2) for _ in range(3)]
        out = {}
        ths = [threading.Thread(target=lambda n=n: out.__setitem__(n, get("/file/" + n)[2])) for n in photos[:3]]
        t0 = time.time()
        [t.start() for t in ths]
        [t.join() for t in ths]
        rep["three_at_once_s"] = round(time.time() - t0, 2)

    if a.mailbox:
        mb = {}
        doc = b'{"v":1,"rev":1,"interval":10,"transition":"fade","look":"vivid","order":"newest","brightness":100,"items":[]}'
        mb["post"] = post("/api/frame", doc)[0]
        st, back, _ = get("/api/frame")
        mb["round_trip_identical"] = back == doc
        mb["too_big"] = post("/api/frame", b"a" * 20000)[0]
        mb["nonsense"] = post("/api/frame", b"nonsense")[0]
        rep["mailbox_tests"] = mb
        if frame and st == 200 and rep["frame_json_now"].startswith("{"):
            post("/api/frame", frame)       # put the phone page's document back
            mb["restored"] = True

    path = os.path.join(HERE, "wifi-report.json")
    with open(path, "w") as f:
        json.dump(rep, f, indent=1)
    print(f"\noverall {rep['overall_kb_s']} KB/s, problems: {rep['problems'] or 'none'}\nreport: {path}")


if __name__ == "__main__":
    main()
