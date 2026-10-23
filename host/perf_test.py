"""Speed and data-loss test for the camera, run through a simulator that's already connected.

    python perf_test.py                       # simulator at http://127.0.0.1:8765
    python perf_test.py --base http://127.0.0.1:8767 --photos 10 --video 15

It goes through the simulator's HTTP API, so it can share the USB port with a running
simulator. It takes photos back to back, records a video, then downloads every new file
over USB and checks it: size as listed, a whole JPEG (and, where PIL is installed, one that
decodes), a thumbnail for each, and for videos every frame and the index.

New files stay on the card (their names are in the report).
"""

import argparse
import io
import json
import struct
import time
import urllib.request

try:
    from PIL import Image
except ImportError:          # structural checks only
    Image = None


def call(base, body, timeout=60):
    req = urllib.request.Request(base + "/api/cmd", data=json.dumps(body).encode(), method="POST")
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return json.load(r)


def state(base):
    with urllib.request.urlopen(base + "/api/state?log=0&event=0", timeout=10) as r:
        return json.load(r)


def media(base, name, thumb=False):
    url = f"{base}/api/media/{name}" + ("?thumb=1" if thumb else "")
    t0 = time.time()
    with urllib.request.urlopen(url, timeout=600) as r:
        data = r.read()
    return data, time.time() - t0


def jpeg_problem(data, decode=True):
    """None for a whole JPEG, else what's wrong with it."""
    if len(data) < 4 or data[:2] != b"\xff\xd8":
        return "no start marker"
    if data.rstrip(b"\x00")[-2:] != b"\xff\xd9":
        return "cut short (no end marker)"
    if b"\xff\xda" not in data:
        return "no image data"
    if Image and decode:
        try:
            Image.open(io.BytesIO(data)).load()
        except Exception as e:      # noqa: BLE001
            return f"doesn't decode: {e}"
    return None


def avi_check(data):
    """Frames, frame rate in the header, and problems found in an MJPEG AVI."""
    problems = []
    if data[:4] != b"RIFF" or data[8:12] != b"AVI ":
        return 0, 0, ["not an AVI"]
    us_per_frame = struct.unpack_from("<I", data, 32)[0]
    header_frames = struct.unpack_from("<I", data, 48)[0]
    frames, p = 0, 224
    while p + 8 <= len(data) and data[p:p + 4] == b"00dc":
        n = struct.unpack_from("<I", data, p + 4)[0]
        frame = data[p + 8:p + 8 + n]
        if len(frame) < n:
            problems.append(f"frame {frames} cut short")
            break
        bad = jpeg_problem(frame, decode=False)
        if bad:
            problems.append(f"frame {frames}: {bad}")
        frames += 1
        p += 8 + n + (n & 1)
    if data[p:p + 4] != b"idx1":
        problems.append("no index after the frames")
    else:
        idx_entries = struct.unpack_from("<I", data, p + 4)[0] // 16
        if idx_entries != frames:
            problems.append(f"index lists {idx_entries} frames, file has {frames}")
    if header_frames != frames:
        problems.append(f"header says {header_frames} frames, file has {frames}")
    fps = 1e6 / us_per_frame if us_per_frame else 0
    return frames, fps, problems


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", default="http://127.0.0.1:8765")
    ap.add_argument("--photos", type=int, default=8)
    ap.add_argument("--video", type=float, default=15, help="seconds of video")
    a = ap.parse_args()
    base = a.base.rstrip("/")
    report = {"firmware": None, "photos": [], "video": None, "files": [], "problems": []}

    st = state(base)
    if not st.get("connected"):
        raise SystemExit("the simulator isn't connected to the camera")
    report["firmware"] = st["hello"]["fw"]
    if st["status"].get("view") == "phone":                 # leave Phone mode first
        call(base, {"cmd": "btn", "b": "shutter"})
        time.sleep(1.5)
    # The laptop's live view shares the USB link and now and then corrupts a reply: pause it
    call(base, {"cmd": "stream", "on": False})
    before = {i["n"] for i in call(base, {"cmd": "list"})["items"]}

    # ---- photos, back to back
    call(base, {"cmd": "mode", "mode": "photo"})
    time.sleep(1.0)
    report["preview_fps"] = state(base)["status"].get("fps")
    for _ in range(a.photos):
        t0 = time.time()
        r = call(base, {"cmd": "photo"})
        wall = time.time() - t0
        if not r.get("ok"):
            report["problems"].append(f"photo failed: {r}")
            continue
        report["photos"].append({"name": r["name"], "kb": round(r["size"] / 1024), "capture_ms": r["capture_ms"],
                                 "save_ms": r["save_ms"], "shot_to_ready_s": round(wall, 2)})

    # ---- video
    if a.video > 0:
        call(base, {"cmd": "mode", "mode": "video"})
        time.sleep(1.5)
        call(base, {"cmd": "rec_start"})
        t0 = time.time()
        fps_seen = []
        while time.time() - t0 < a.video:
            time.sleep(1)
            s = state(base)["status"]
            fps_seen.append(s.get("fps"))
        r = call(base, {"cmd": "rec_stop"}, timeout=60)
        report["video"] = {"reply": r, "seconds": round(time.time() - t0, 1),
                           "sensor_fps_while_recording": fps_seen}
        call(base, {"cmd": "mode", "mode": "photo"})

    # ---- every new file, checked
    items = call(base, {"cmd": "list"})["items"]
    for it in items:
        if it["n"] in before:
            continue
        data, secs = media(base, it["n"])
        entry = {"name": it["n"], "listed": it["s"], "got": len(data), "usb_kb_s": round(len(data) / 1024 / secs)}
        if len(data) != it["s"]:
            report["problems"].append(f"{it['n']}: got {len(data)} bytes, listed {it['s']}")
        if it["v"]:
            frames, fps, probs = avi_check(data)
            entry.update(frames=frames, header_fps=round(fps, 1))
            report["problems"] += [f"{it['n']}: {p}" for p in probs]
        else:
            bad = jpeg_problem(data)
            if bad:
                report["problems"].append(f"{it['n']}: {bad}")
        thumb, _ = media(base, it["n"], thumb=True)
        bad = jpeg_problem(thumb)
        entry["thumb_kb"] = round(len(thumb) / 1024, 1)
        if bad:
            report["problems"].append(f"{it['n']} thumbnail: {bad}")
        report["files"].append(entry)

    report["usb_crc_errors"] = state(base)["link"]["crc_errors"]
    call(base, {"cmd": "stream", "on": True})
    print(json.dumps(report, indent=1))


if __name__ == "__main__":
    main()
