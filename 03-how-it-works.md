# How the camera software works

8 Oct 2026. Covers `firmware/` (runs on the board) and `host/` (the laptop simulator). The board does all the camera work; the laptop only stands in for the screen and buttons until they arrive.

```
┌──────────────────────── board (ESP32-S3) ────────────────────────┐        ┌──────────── laptop ─────────────┐
│                                                                  │        │                                 │
│  OV3660 ──DVP──▶ esp32-camera driver ──▶ cam_task ──▶ SD card     │  USB   │  link.py ──▶ server.py ──▶ browser│
│                   (2 frame buffers        │  ▲        (DCIM/)     │ serial │  (frames,    (HTTP:      (index.html│
│                    in PSRAM)              │  │                    │ 1.5 Mb │   commands)   stream,     = screen + │
│                                           ▼  │ commands           │◀──────▶│               API, cache)  buttons)  │
│                                  tx_task ──▶ link (UART0) ◀── rx_task│      │                                 │
└──────────────────────────────────────────────────────────────────┘        └─────────────────────────────────┘
```

## 1. Firmware: what runs on the board

### Files

| File | Job |
| --- | --- |
| `main/main.c` | The camera's brain: boot sequence, the two tasks, and every command (photo, video, gallery, delete, power) |
| `main/lens.c` | The sensor: init, preview vs still sizes, the six film looks |
| `main/storage.c` | The SD card: mounting, file names, safe saves, listing, reading, deleting |
| `main/avi.c` | Writes video files frame by frame |
| `main/link.c` | The USB serial link to the laptop: framing, logs, incoming commands |
| `main/pins.h` | Which GPIO does what on the GOOUUU board |

### Boot (`app_main` in main.c)

1. Allocates three buffers in PSRAM: the outgoing preview frame (256 KB) and two thumbnail buffers (128 KB each).
2. `link_init`: starts UART0 at 1.5 Mbaud and redirects every `ESP_LOG` message into the link, so log text can't land in the middle of a binary frame.
3. `storage_init`: mounts the microSD (SDMMC, 1-bit, 40 MHz), creates `DCIM/` and `DCIM/.thumbs/`, deletes any half-written temp files from a power cut, and sets the next file number: the higher of the card's highest and the one kept in NVS (`next_num`, saved with every new file), so a format and a restart don't start again at 0001. It also creates `DCIM/.small/`, where `/small` keeps the frame's copies.
4. `lens_init`: starts the OV3660 at full 2048×1536, then drops to the 320×240 preview size (why: section "lens.c" below).
5. Sends a `boot` event and starts the two tasks.

### The two tasks

**`cam_task`** (core 1) owns the camera and the SD card. Nothing else touches them, so they never race. Its loop, simplified:

```c
for (;;) {
    run any commands waiting in the queue          // photo, look, list, get, delete...
    every 500 ms: send a status message            // mode, look, fps, SD space, recording time
    if powered off: sleep 50 ms and loop
    fb = grab the newest frame from the sensor
    if recording:      append it to the AVI file  (first frame is kept as the thumbnail)
    else if photo mode: copy it as the "last preview" (becomes the next photo's thumbnail)
    if streaming and the outgoing slot is free: copy it there and wake tx_task
    return the frame buffer to the driver
    if recording too long or card nearly full: stop recording
}
```

**`tx_task`** (core 0) waits for a frame in the slot and sends it to the laptop. Because sending is slow (a 640×480 frame takes about 0.1 s) and capture is fast, the slot is a "latest frame only" mailbox: while one frame is being sent, newer frames are skipped for the laptop but still recorded to video. That's why the laptop sees about 7 fps in video mode while the file gets about 20.

### lens.c: preview vs full-resolution photos

The camera driver sizes its JPEG frame buffers once, at start-up, from the starting resolution (width × height ÷ 5). So `lens_init` starts at 2048×1536, giving two 629 KB buffers in PSRAM, then switches to 320×240 for preview. Taking a photo (`take_photo` in main.c):

1. Switch the sensor to 2048×1536 at JPEG quality 8 (lower = better).
2. Throw away 2 frames: they were captured at the old size, and exposure needs a moment to settle.
3. Grab the next frame: about 0.4 s total so far.
4. Save it with the last preview frame as its thumbnail (about 0.6–1.0 s).
5. Switch back to the preview size.

**Film looks** are not filters applied afterwards. They set the sensor's own registers (special effect, white balance, saturation, contrast, brightness), so they cost no CPU time and the preview shows exactly what gets saved.

### storage.c: the card

```
DCIM/DSC_0001.JPG          photo
DCIM/MOV_0002.AVI          video (photos and videos share one counter)
DCIM/.thumbs/DSC_0001.JPG  small preview copy, for a fast gallery
```

- **Safe saves:** a photo is written to `DCIM/.photo.tmp`, flushed and synced, then renamed. Video goes to `DCIM/.rec.tmp` and is renamed when stopped. A rename is all-or-nothing, so a power cut leaves either the old state or the finished file, never a broken photo. Leftover temp files are deleted at boot.
- **Numbering:** the next number comes from scanning the card at boot, not from memory on the board, so a different card never gets its photos overwritten.
- **The speed fix (`storage_fwrite`):** camera frames live in PSRAM, which the SD controller's DMA can't read. Handed a PSRAM buffer, the SD driver falls back to one 512-byte sector per transfer, and each sector waits for the card, which gave about 120 KB/s. `storage_fwrite` copies the data through a 16 KB buffer in internal RAM, which DMA can read, so the card receives multi-sector writes. Saves got about 4× faster.
- **Name check:** `get` and `delete` only accept names shaped like `DSC_1234.JPG` or `MOV_1234.AVI`, so a command can never reach a file outside `DCIM/`.

### avi.c: video

An AVI file is a header, then one chunk per frame (`00dc` + size + the JPEG straight from the sensor), then an index listing where each frame sits. While recording:

- `avi_open` writes a placeholder header.
- `avi_add_frame` appends each frame to the card as it arrives, and notes its offset and size in an index held in PSRAM (16 bytes per frame).
- `avi_close` writes the index, then goes back and fills in the header with the real frame count and frame rate (frames ÷ actual duration).

So video is already saved continuously while recording; only the small header and index wait until the end.

### link.c: talking to the laptop

Everything the board sends is a frame:

```
A5 5A | type (1 byte) | length (4 bytes) | payload | CRC32 (4 bytes)
```

| Type | Payload |
| --- | --- |
| `F` | Preview frame: width, height, then the JPEG |
| `S` | Status JSON, twice a second |
| `R` | Reply to a command (JSON), or an event such as `rec_stopped` (id 0) |
| `D` | A 16 KB slice of a file: request id, offset, total size, bytes |
| `L` | One log line |

The CRC lets the laptop discard a frame the USB chip corrupted, and the `A5 5A` marker lets it find the start of the next one. The laptop sends one JSON command per line, such as `{"id":7,"cmd":"photo"}`. The full list: `hello`, `status`, `stream`, `power`, `mode`, `look`, `photo`, `rec_start`, `rec_stop`, `list`, `get`, `delete`, `baud`, `reboot`.

A file transfer (`get`) stops as soon as another command arrives, so a big video download never leaves the camera deaf. The laptop asks again from the `offset` it reached.

## 2. Laptop: the simulator

| File | Job |
| --- | --- |
| `host/link.py` | Opens the serial port (with DTR/RTS held low, because those lines reset the ESP32), splits incoming bytes into frames, checks CRCs, keeps the latest preview frame, status and logs, matches replies to commands by id, and downloads files with automatic resume |
| `host/server.py` | A small web server on 127.0.0.1:8765: `/stream.mjpg` (the live view), `/api/state` (status, logs, events), `/api/cmd` (send a command), `/api/media/NAME` (photo, video or thumbnail, cached in `host/cache/`) |
| `host/web/index.html` | The virtual camera. Its screen is the real 2.0 in screen's 320×240, shown 1:1 on a to-scale drawing of the camera back. It holds the UI rules: which button (shutter, ◀, ●, ▶, power) does what in which mode, dimming and sleep, delete confirmation, and AVI playback |

## 3. What happens when you press things

**Shutter in photo mode:** page `onShutter` → `POST /api/cmd {"cmd":"photo"}` → `link.command` writes `{"id":12,"cmd":"photo"}` → board `rx_task` queues it → `cam_task` runs `take_photo` (switch to full size, capture, save, switch back) → `R` reply `{"ok":true,"name":"DSC_0012.JPG",...}` → page flashes white and shows the thumbnail.

**Shutter in video mode:** `rec_start` opens `.rec.tmp`. Each loop of `cam_task` appends a frame. A second press sends `rec_stop`, which writes the index, patches the header and renames the file to `MOV_xxxx.AVI`.

**Opening the gallery:** the page pauses the live stream (giving the link's full speed to downloads), sends `list`, and shows each item's thumbnail at once. It then fetches the full photo, or the whole video, which the page unpacks into its JPEG frames and plays.

## 4. When the real screen and buttons arrive

The camera side (`lens`, `storage`, `avi`, the command functions in main.c) stays as it is. The work is a new `ui.c` on the board that does what `index.html` does now: draw the screen on the ST7789 and read the dial and shutter. It calls the same `take_photo`, `rec_start` and `set_mode` functions the serial commands call today. The serial link stays useful for debugging.

## 5. Known limits and the next improvements

- **Video drops some frames when the card stalls.** Each frame is written in the capture loop, so when the card pauses (allocating space, internal housekeeping), capture waits and the sensor's newer frames are skipped. In the 8 Oct test the sensor ran at about 22.8 fps and the file got 18.9 fps, about 17% dropped. The fix is a **buffered writer**: a second task that writes to the card, fed by a ring buffer of about 3 MB in PSRAM (roughly 10 s of VGA video). Capture copies each frame into the ring and never waits; the writer drains it in large chunks. This is how commercial cameras record.
- **SD speed after the fix is about 0.34 MB/s**, plus 40–330 ms to sync each file. That's limited by the board's single SD data line (1-bit mode), the no-name 1 GB test card, and the per-photo sync done for safety. A branded A1 or U3 card should help; test one before deciding anything.
- **The USB link carries about 120 KB/s**, so only the simulator is slow to show full-size photos and videos. The real camera reads them straight from the card.
