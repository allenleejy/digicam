"""Serial link to the digicam firmware (see firmware/main/link.h for the frame format).

Device -> laptop: A5 5A | type | len u32 LE | payload | crc32 u32 LE
Laptop -> device: one JSON command per line.
"""

import collections
import itertools
import json
import struct
import threading
import time
import zlib

import serial

BAUD = 1_500_000             # matches LINK_BAUD in firmware/main/link.h
FALLBACK_BAUDS = (921_600, 2_000_000, 1_000_000, 115_200)
MAGIC = b"\xa5\x5a"


class DeviceError(Exception):
    pass


class Link:
    def __init__(self, port, baud=BAUD):
        self.port_name = port
        self.ser = serial.Serial()
        self.ser.port = port
        self.ser.baudrate = baud
        self.ser.timeout = 0.05
        # Opening the CH340 port must not pulse DTR/RTS: those lines reset the ESP32.
        self.ser.dtr = False
        self.ser.rts = False
        self.ser.open()

        self.ids = itertools.count(1)
        self.cmd_lock = threading.Lock()
        self.pending = {}          # id -> {"event", "reply", "data", "size"}
        self.frame = None          # latest preview JPEG
        self.frame_size = (0, 0)
        self.frame_seq = 0
        self.frame_cond = threading.Condition()
        self.status = {}
        self.status_time = 0.0
        self.logs = collections.deque(maxlen=500)
        self.log_seq = 0
        self.events = collections.deque(maxlen=50)
        self.event_seq = 0
        self.bytes_in = 0
        self.last_rx = 0.0          # time of the last good frame of any type
        self.crc_errors = 0
        self.running = True
        self.reader = threading.Thread(target=self._read_loop, daemon=True)
        self.reader.start()

    # ---- receiving -------------------------------------------------------

    def _read_loop(self):
        buf = bytearray()
        while self.running:
            try:
                chunk = self.ser.read(65536)
            except serial.SerialException as e:
                self._log(f"[host] serial error: {e}")
                time.sleep(0.5)
                continue
            if chunk:
                self.bytes_in += len(chunk)
                buf += chunk
                self._parse(buf)

    def _parse(self, buf):
        while True:
            start = buf.find(MAGIC)
            if start < 0:
                del buf[:-1]
                return
            if start:
                del buf[:start]   # boot ROM text or noise before a frame
            if len(buf) < 7:
                return
            ftype = chr(buf[2])
            (length,) = struct.unpack_from("<I", buf, 3)
            if length > 8 * 1024 * 1024:
                del buf[:2]
                continue
            if len(buf) < 7 + length + 4:
                return
            payload = bytes(buf[7:7 + length])
            (crc,) = struct.unpack_from("<I", buf, 7 + length)
            if zlib.crc32(payload) != crc:
                self.crc_errors += 1
                del buf[:2]
                continue
            del buf[:7 + length + 4]
            self.last_rx = time.time()
            self._dispatch(ftype, payload)

    def _dispatch(self, ftype, payload):
        if ftype == "F":
            w, h = struct.unpack_from("<HH", payload, 0)
            with self.frame_cond:
                self.frame = payload[4:]
                self.frame_size = (w, h)
                self.frame_seq += 1
                self.frame_cond.notify_all()
        elif ftype == "S":
            self.status = json.loads(payload)
            self.status_time = time.time()
        elif ftype == "L":
            self._log(payload.decode("utf-8", "replace"))
        elif ftype == "R":
            msg = json.loads(payload)
            rid = msg.get("id", 0)
            if rid == 0:
                self.event_seq += 1
                self.events.append((self.event_seq, msg))
                self._log(f"[event] {json.dumps(msg)}")
                return
            p = self.pending.get(rid)
            if p:
                p["reply"] = msg
                p["replied"].set()
        elif ftype == "D":
            rid, offset, total = struct.unpack_from("<III", payload, 0)
            p = self.pending.get(rid)
            if p is not None:
                p["chunks"][offset] = payload[12:]
                p["received"] += len(payload) - 12
                if offset + len(payload) - 12 >= total:
                    p["done"].set()

    def _log(self, line):
        self.log_seq += 1
        self.logs.append((self.log_seq, time.strftime("%H:%M:%S"), line))

    # ---- commands --------------------------------------------------------

    def command(self, cmd, timeout=5.0, **args):
        """Send a command and wait for its JSON reply."""
        return self._request(cmd, timeout, False, args)[0]

    def _request(self, cmd, timeout, want_data, args):
        rid = next(self.ids)
        p = {"replied": threading.Event(), "done": threading.Event(),
             "reply": None, "chunks": {}, "received": 0}
        self.pending[rid] = p
        line = json.dumps({"id": rid, "cmd": cmd, **args}) + "\n"
        try:
            with self.cmd_lock:
                self.ser.write(line.encode())
            if not p["replied"].wait(timeout):
                raise DeviceError(f"no reply to '{cmd}' within {timeout:.0f}s")
            reply = p["reply"]
            if want_data and reply.get("ok") and reply.get("size", 0) > reply.get("offset", 0):
                # Wait for the last chunk, or until data stops arriving (a lost chunk means
                # "done" never fires). Resuming only once the device has gone quiet avoids
                # queueing the next request behind the rest of this transfer.
                last = -1
                while not p["done"].wait(1.5):
                    if p["received"] == last:
                        break
                    last = p["received"]
            return reply, p["chunks"]
        finally:
            self.pending.pop(rid, None)

    def get_file(self, name, thumb=False, attempts=5, cancelled=lambda: False):
        """Download a photo, video or thumbnail. Resumes from the first gap after a bad chunk."""
        data = bytearray()
        total = None
        for _ in range(attempts):
            if cancelled():
                raise DeviceError(f"{name}: download cancelled")
            reply, chunks = self._request("get", 10.0, True,
                                          {"name": name, "thumb": thumb, "offset": len(data)})
            if not reply.get("ok"):
                raise DeviceError(reply.get("error", "get failed"))
            total = reply["size"]
            for off in sorted(chunks):
                if off != len(data):
                    break
                data += chunks[off]
            if len(data) >= total:
                return bytes(data[:total])
            self._log(f"[host] {name}: resuming at {len(data)} of {total} bytes")
        raise DeviceError(f"{name}: download incomplete ({len(data)} of {total} bytes)")

    def connect(self):
        """Find the link speed (the device may still be at a rate set earlier) and say hello."""
        for rate in (BAUD,) + FALLBACK_BAUDS:
            self.ser.baudrate = rate
            try:
                # The default rate gets longer: the device may still be booting (camera init ~2 s).
                hello = self.command("hello", timeout=5.0 if rate == BAUD else 1.5)
            except DeviceError:
                # Good frames arriving means the rate is right and the device is just busy
                # (finishing a file transfer, say): wait for it rather than trying other rates.
                deadline = time.time() + 60
                hello = None
                while rate == BAUD and time.time() - self.last_rx < 1 and time.time() < deadline:
                    try:
                        hello = self.command("hello", timeout=5.0)
                        break
                    except DeviceError:
                        pass
                if hello is None:
                    continue
            if rate != BAUD:
                self.command("baud", rate=BAUD)
                time.sleep(0.1)
                self.ser.baudrate = BAUD
                hello = self.command("hello")
            return hello
        raise DeviceError(f"no digicam firmware answering on {self.port_name}")

    def wait_frame(self, after_seq, timeout=2.0):
        with self.frame_cond:
            self.frame_cond.wait_for(lambda: self.frame_seq != after_seq, timeout)
            return self.frame_seq, self.frame, self.frame_size

    def reset_device(self):
        """Pulse EN through RTS to reboot the ESP32 into the app."""
        self.ser.dtr = False
        self.ser.rts = True
        time.sleep(0.1)
        self.ser.rts = False

    def close(self):
        self.running = False
        self.ser.close()
