# Gift cameras (2 units, for friends)

Two DIY retro digital cameras, built as gifts, on shared electronics and firmware:

1. **InstantSnap.novo-style** flat body with a 1.69 in screen. Due 31 Oct 2026 (assumed) and built first.
2. **Yashica Mini Snoopy-style** box with a bigger 2.8 in screen. Follows after; date to confirm.

This is a separate project from `../PLAN.md`. That file (July 2026) plans a screenless, disposable-style camera as a commercial product.

## Files

| File | What it is |
| --- | --- |
| `01-research-brief.md` | First research pass: commercial cameras compared, buy vs build, early hardware ideas |
| `02-build-plan.md` | **The current plan**: decisions, 4-week timeline, parts, wiring and charging, pin plan, firmware spec, case finish, checklists |
| `03-how-it-works.md` | How the firmware and simulator work, step by step |
| `04-case-design.md` | Camera 1 case. **v2 test-fit body (10 Oct)** first: electronics sled, layout, wiring lanes, hardware, assembly, printing, what to measure. Then the v1 concept |
| `08-phone-app-plan.md` | Getting photos onto an iPhone: **Phone mode** (camera Wi-Fi, a photo page in Safari, a free Shortcut for saving many), built 15 Oct. Then the earlier app plan |
| `07-shopping-list.md` | What's still to buy to join the parts: battery socket, camera ribbon, wire, inserts, screws, tape, with Lazada search terms |
| `06-assembly-guide.md` | **Step-by-step assembly with a picture per step**, and the link to the interactive 3D guide (`guide/`) |
| `05-bench-bring-up.md` | **Start here with the parts on the desk**: wiring every part to the board, testing the whole pipeline step by step, and removing the pin headers for the case |
| `cad/` | **v2 test-fit body: `camera1_v2.scad`** (print files in `export/v2/`, renders in `renders/v2/`, fit check `check_v2.py`). Also the v1 parametric Fusion model (`fusion/`, not yet updated to v2), the v1 OpenSCAD model and a v1 STEP assembly |
| `firmware/` | ESP-IDF camera firmware (0.2.0): runs the camera on its own with screen, buttons and battery gauge; preview, photos, video, gallery with playback, SD, film looks |
| `host/` | Laptop simulator: a virtual camera in the browser driving the real board. Start with `host/run.sh` ([host/README.md](host/README.md)). `host/bench.py`: screenshots of the camera's screen, button presses, status and log from the terminal |
| `diagrams/` | `wiring-bench.png` (every wire, pin to pin; from `wiring_bench.py`), `timeline.png` and `wiring.png` (block diagram; still shows the dial), `wiring-early.png` (first sketch, superseded), rendered from the claude.ai doc |

**These local files are the main copy** (decided 8 Oct 2026). Edit them here. The claude.ai doc https://claude.ai/artifact/KfYH1emPw4jr6YSeR5GVqt is frozen as of 8 Oct and is no longer updated.

## Hardware at a glance

- Board: GOOUUU ESP32-S3-CAM Core N16R8 with OV3660, on a ribbon extension. You own one (camera 1); buy one more (camera 2).
- Screens: 1.69 in ST7789 IPS 240×280 (camera 1); 2.8 in ST7789 IPS 320×240 (camera 2). Controls: rotary dial (EC11), shutter button, slide switch.
- Power: 3.7 V LiPo (camera 1: the 2000 mAh 103450 you own) → USB-C charge + boost module (IP5306 type) → switch → board 5V pin.
- Storage: the board's built-in microSD slot (SDMMC 1-bit, GPIO 38/39/40).
- Firmware: `firmware/` (ESP-IDF v5.5.4); camera and SD code came from `lovebox/development/firmware/camera-test`. Since 10 Oct the camera runs standalone on the 1.14 in screen; `host/` still works alongside it.
