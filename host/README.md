# Digicam simulator

Runs the real camera board over USB and shows a virtual InstantSnap-style camera in your browser: live view, shutter, dial, power switch, gallery. Use it until the 1.69 in screen, dial and shutter arrive.

The board does all the real work: sensor, film looks, photos, video and the SD card. The page only stands in for the screen and buttons.

## Run it

1. Plug the board's **COM** USB-C port (the CH340 one) into the laptop. Put a FAT32 microSD card in the board's slot.
2. Start the simulator:

   ```bash
   ./run.sh
   ```

   It opens http://127.0.0.1:8765. Add `--name Sam` to put a name on the boot splash. Add `--port /dev/cu.xxx` if the board isn't found automatically.

## Controls

The page draws the camera's back to scale (116 × 66 mm, from `cad/instantsnap_body.scad`), with the 2.0 in screen at its real 320 × 240.

| Input | Keyboard / mouse | Live view | Gallery |
| --- | --- | --- | --- |
| Shutter | Space, or click the orange button on top | Photo, or start/stop video | Back to live view; cancels a delete |
| ◀ ▶ | ← →, or click | Previous/next film look | Previous/next item |
| Press ● | Enter, or click | Open gallery | Back to live view; confirms a delete |
| Hold ● (0.6 s) | Hold Enter or ● | Photo/video mode | Delete (asks first) |
| Power | P, or click the switch on top | Off/on | Off/on |

The side panel can open any photo full size or save it to the laptop. The server starts even without the board and connects when you plug it in.

## Good to know

- Photos are 2048×1536 and take about 1–1.5 s to capture and save. Videos are 640×480 at about 20 fps.
- The USB link carries about 120 KB/s, so full-size photos take 2–4 s to appear in the gallery and a 1-minute video about 2 minutes. Everything downloaded is cached in `cache/`.
- Live preview pauses in the gallery, so downloads get the whole link.

## How it fits together

```
board (firmware/)  --USB serial, 1.5 Mbaud-->  server.py + link.py  --HTTP-->  web/index.html
```

- `link.py`: the serial protocol: framed messages from the board (preview frames, status, logs, file data), JSON commands to it. It resumes interrupted downloads.
- `server.py`: bridges the link to the browser: an MJPEG stream, a state endpoint, commands and a media cache.
- `web/index.html`: the virtual camera, drawn to scale, with its screen UI at the real 320×240.

Reflash the firmware after changing it:

```bash
cd ../firmware && . ~/Development/esp-idf-v5.5.4/export.sh && idf.py -p /dev/cu.wchusbserial10 -b 921600 flash
```
