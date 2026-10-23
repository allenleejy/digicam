# Context prompt: Digicam gift cameras (for a partner-product session)

Paste everything below the line into a new session. Fill in the **Partner product** section first.

---

You are joining an existing hardware project. Below is the state of a DIY retro digital camera I've built, its hardware, firmware and "backend" (there is no cloud: the camera serves everything itself). I want to develop a **partner product** that works alongside it. Read this, then look at the repo before proposing anything.

## Partner product (fill in)

- What it is: _______________________
- How it should talk to the camera (Wi-Fi to the camera's network / USB / SD card / other): ____
- Who uses it, deadline, budget: ____

## Where things are

- Repo: https://github.com/zyrvad/digicam (branch `main`). Local copy and main source of truth: `~/Development/engineering/digicam/gift-cameras/`.
- Docs, in order: `README.md` (index; some of its hardware notes are older), `02-build-plan.md` (decisions, specs, pin plan), `03-how-it-works.md`, `04-case-design.md`, `05-bench-bring-up.md` (wiring + tests), `06-assembly-guide.md`, `07-shopping-list.md`, `08-phone-app-plan.md` (Phone mode).
- `firmware/` ESP-IDF project, `firmware/web/phone.html` the phone page, `firmware/sim/` Mac renderer for the UI, `host/` laptop tools, `cad/` case models, `guide/` 3D assembly guide.

## The project

Two DIY retro digital cameras, built as gifts for friends:
1. **Camera 1**, InstantSnap.novo-style flat body. Due 31 Oct 2026. The priority.
2. **Camera 2**, Yashica Mini Snoopy-style with a 2.8" screen. Built afterwards.

Priorities: finish and feel first, customisable through the 3D-printed case, photo quality about Chuzhao level, cost under buying the real cameras (about US$25–45 in parts). The owner (me) has no Apple Developer account and doesn't want to pay for one.

## Hardware

| Part | Detail |
| --- | --- |
| Board | GOOUUU ESP32-S3-CAM Core **N16R8** (16 MB flash, 8 MB PSRAM), CH340 USB serial (shows as `/dev/cu.wchusbserial10` on my Mac). 62.4 × 28.1 mm, 3 mm holes at the corners |
| Sensor | **OV3660** (3 MP) on a 24-pin ribbon (extension). OV5640 considered as an upgrade; same connector |
| Screen now | 1.14" ST7789 IPS, 240 × 135, SPI. A 2.0" 320 × 240 is on order; camera 2 gets 2.8" 320 × 240 |
| Controls | 4 tactile buttons (5 × 5 × 6 mm SMD, flat feet), each between a GPIO and GND: ◀, ●, ▶ on the back, shutter on top. Slide power switch |
| Power | 3.7 V 2000 mAh LiPo 103450 (10 × 34 × 50 mm, protected, JST plug, likely PH2.0) → USB-C charge + 5 V boost module (IP5306 type, 11.8 × 16.5 mm) → switch → board 5V pin. Est. 4 h live view at ~300 mA |
| Storage | Board's microSD slot, SDMMC 1-bit, FAT |

**Pin map** (`firmware/main/pins.h`):
- SD: CMD 38, CLK 39, D0 40. Camera: fixed by the board (XCLK 15, SDA 4, SCL 5, VSYNC 6, HREF 7, PCLK 13, D0–D7 11 9 8 10 12 18 17 16).
- Screen on SPI2: SCK 21, MOSI 47, CS 42, DC 41, BL 46 (PWM, LEDC timer 1). RES tied to EN.
- Buttons (internal pull-ups): ◀ 2, ● 14, ▶ 45 (strapping pin, fine low), shutter 48 (shared with the onboard RGB LED).
- Battery sense: GPIO1 through a 100k/100k divider.
- Free pins are few; check `pins.h` and the board's camera/PSRAM/flash pins before assigning any.

## Firmware (v0.3.0, flashed and working)

- **ESP-IDF v5.5.4** (`source ~/Development/esp-idf-v5.5.4/export.sh`), plain C, no LVGL. Components: esp32-camera ^2.0.0, esp_jpeg ^1.3.0, espressif/qrcode. Build/flash: `idf.py -p /dev/cu.wchusbserial10 flash`.
- **Images:** stills 2048 × 1536 JPEG (quality 8); live preview 320 × 240 at ~27 fps; video 640 × 480 MJPEG AVI at ~20 fps (~0.28 MB/s). Six "film looks" (Natural, Warm film, Faded, Mono, Sepia, Cool), done with sensor settings so they cost no CPU.
- **Files on the card:** `/sdcard/DCIM/DSC_0001.JPG` and `MOV_0002.AVI`. Photos and videos share one counter, and numbers are never reused (even after format). 320 × 240 thumbnails in `/sdcard/DCIM/.thumbs/thumb_<name>`. Writes go to a temp file then rename. AVI: 224-byte header, then `00dc` + u32 length + JPEG chunks, index at the end.
- **On-device UI:** drawing code is portable (`gfx.c`, `ui_draw.c`, no ESP-IDF calls) into a 240 × 135 RGB565 framebuffer. Spleen 8 × 16 and 16 × 32 fonts, pixel icons, pastel palette (lilac, orange). Views: boot, live view (grid, self-timer, REC), mode dial (hold ●: Photo / Video / Phone / Settings), gallery with delete, settings, phone QR screens. `firmware/sim/render.sh` renders every screen to PNG on the Mac.
- **Settings** in NVS (namespace `digicam`, blob `cfg`): self-timer 0/3/10 s, grid, brightness 30/60/100 %, sleep 1/2/5 min/never. Screen dims after 30 s.
- **Per-camera:** `firmware/main/personal.h` holds `OWNER_NAME` (boot screen "hi, <name>").

## "Backend": what the camera serves

There is no cloud server. Two interfaces exist.

**1. Phone mode (Wi-Fi), `firmware/main/phone.c`**
- The camera starts a **WPA2 soft AP**: SSID `digicam-<owner>` or `digicam-xxxx` (from the MAC), 8-character random password kept in NVS (`wifi_pass`). Channel 6, max 2 clients. Camera IP **192.168.4.1**.
- Screen shows a Wi-Fi QR (`WIFI:T:WPA;S:…;P:…;;`), then a URL QR once a phone joins.
- `esp_http_server` routes, plain HTTP:
  - `GET /` the photo page (`firmware/web/phone.html`, embedded in firmware; single file, inline CSS/JS)
  - `GET /api/list` → `{"camera","owner","battery":<0-100|null>,"items":[{"n":"DSC_0012.JPG","s":<bytes>,"v":0|1}]}`, newest first
  - `GET /thumb/<name>` 320 × 240 JPEG; `GET /file/<name>` full file (JPEG or `video/x-msvideo`). Both chunked from PSRAM, `Cache-Control: immutable`
  - `POST /api/delete` body = file name → `{"ok":true|false}`
  - Names are validated against `(DSC|MOV)_\d{4}\.(JPG|AVI)`; nothing outside DCIM is reachable.
- The camera is idle (no capture) while Phone mode is on; the web server's task is the only SD reader. About 54 KB internal heap free in this mode; Wi-Fi/lwIP buffers are allowed in PSRAM.
- iPhone saving: long-press → Save to Photos; many at once via a free Shortcut ("Save from Digicam") launched with `shortcuts://x-callback-url/run-shortcut`. iPhones can't play or save the AVI files.

**2. USB serial link, `firmware/main/link.c` + `host/`**
- 1.5 Mbaud on the CH340. Device → host frames: `A5 5A | type u8 | len u32 LE | payload | crc32 LE`. Types: `F` preview JPEG, `S` status JSON, `R` reply JSON, `D` file data (req id, offset, total, bytes), `L` log line.
- Host → device: one JSON per line, e.g. `{"id":3,"cmd":"photo"}`. Commands: hello, status, stream, power, mode, look, photo, rec_start, rec_stop, list, get, delete, btn (`left|right|ok|hold|shutter`), shot (screen capture), lcd (screen alignment), baud, reboot.
- `host/server.py` bridges this to a browser simulator (and serves the phone page under `/phone/` for testing). `host/bench.py`: `shot`, `btn`, `status`, `log`, `lcd`, `remote` (keyboard remote).

## Case

- `cad/camera1_v2.scad` (OpenSCAD, Manifold): 110 × 64 × 15.1 mm, 11.9 mm cavity, an electronics sled so parts don't rattle, wiring lanes. **Model space is mirrored** (X left to right seen from the back), so print exports go through `real()`. Exports in `cad/export/v2/`. Fusion 360 model is the older v1.
- Headers are desoldered; 30 AWG silicone wire for signals, 26 AWG for power.

## Known limits and decisions (don't re-litigate without reason)

- iPhones can't receive files over Bluetooth from a custom device, and Safari has no Web Bluetooth. Transfer is Wi-Fi only.
- No native iOS app: it needs the US$99/yr Apple Developer Program (free provisioning expires after 7 days).
- The phone page is plain HTTP, so no service worker, offline mode or Web Share with files. A home-screen web app (Add to Home Screen, still HTTP) is the recommended next step; HTTPS (self-signed CA profile, or a real domain cert renewed every ≤200 days) is optional and costs RAM.
- Not yet verified on a real iPhone: joining via QR, Save to Photos, the Shortcut. Physical screen orientation/colours not yet confirmed.

## Related codebase

The camera and SD code came from Love Letter Box's `firmware/camera-test` (now only on the `june-2026` branch of github.com/lovingdailyreminders/development) (Love Letter Box, a separate commercial product: ESP32-S3, Cloudflare Workers + Durable Objects, Supabase EU, Expo app). Reuse is fine, but keep the projects separate unless I say otherwise.

## How I like to work

- Local files are the main copy; keep docs in the repo up to date as you go.
- Plan first and walk me through the stages before implementing anything large.
- Plain-language explanations; I'm comfortable soldering and flashing but want clear wiring steps.
