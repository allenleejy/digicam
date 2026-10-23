# Reply to Allen: frame mailbox on the camera

23 Oct 2026. Everything below the line is for Allen. The full contract, kept up to date, is `10-frame-mailbox.md` in the digicam repo (https://github.com/zyrvad/digicam).

---

Hi Allen, here's the camera side of the frame mailbox. It's flashed on camera 1 and we tested it with your frame and my iPhone together today.

## Firmware

**Version 0.4.0**, with items 1, 2, 3 and 5:

| Item | State |
| --- | --- |
| 1. `GET`/`POST /api/frame` | In. `204` with no mailbox; `413` over 16 KB (the connection closes); `400` for anything that isn't a JSON object. Stored as `/sdcard/frame.json`, written safely so a power cut leaves a whole document |
| 2. The document | Exactly your section 2, keys in your order, settings before `items` |
| 3. Phone page | In, inside a redesigned page: Frame pill and sheet, frame badges on tiles, "Add to frame / On frame ✓" in the viewer, "Send to frame" in Select, saving on every change, 300 limit, photos only on the frame shown with "Take them off" |
| 4. `/small` | Not in. Your frame gets `404` and falls back to `/thumb`, as designed. After the gift |
| 5. "frame here" on the camera's screen | In, when a `DigiGallery/` client reads `/api/frame` |

## Your tests, run from a laptop on the camera's Wi-Fi

| Test | Result |
| --- | --- |
| `GET /api/frame` with no file | `204` |
| `POST` the sample document | `{"ok":true}` |
| Read back and compare | Identical |
| 20 KB body | `413` |
| `nonsense` | `400` |

Also checked:
- a restart keeps the document;
- every phone page flow, on the laptop, and the main ones on an iPhone.

## A `frame.json` the phone page wrote

From the iPhone during today's joint test:

```json
{"v":1,"rev":9,"interval":5,"transition":"slide","look":"contrast","order":"shuffle","brightness":100,"items":[{"n":"DSC_0030.JPG","s":145676},{"n":"DSC_0010.JPG","s":144739},{"n":"DSC_0009.JPG","s":135898},{"n":"DSC_0008.JPG","s":123796},{"n":"DSC_0005.JPG","s":113512}]}
```

## Internal free heap (camera log)

| When | Free | Largest block |
| --- | --- | --- |
| Phone mode, nobody joined | 38 KB | 26 KB |
| After a `POST /api/frame` | 35-36 KB | 24 KB |
| Your frame and my iPhone both joined, several minutes of syncs and saves | **lowest 34 KB** | 24 KB |

That's above your 20 KB line, so no buffers were shrunk.

The camera now logs this every 10 s while anyone is joined, and after every `POST /api/frame`:

```
phone: heap after POST /api/frame: internal free 36 KB, largest block 24 KB, lowest 34 KB, clients 1
```

## What your frame did in the joint test (from the camera's log)

- **Joining:** it joined for 1-2 s at a time, about every 35 s, and left, so the two places were never both held for long.
- **Fetching:**
  - `/small/DSC_nnnn.JPG?w=320&h=256` got `404`, then `/thumb/` worked.
  - `/api/frame` was read each time. Changes from the phone showed up on the frame.

## One change to the "leave alone" list: the Wi-Fi channel

- **Before:** the camera's network was always on channel 6.
- **Now:** when Phone mode starts, the camera scans for about a second and uses whichever of channels 1, 6 and 11 is quietest. Today it moved to channel 1 or 11, away from the busy 6.
- **Your frame:** I read `camlink.c`. `camlink_find` scans every channel for the network by name, and `camlink_join` sets no channel, so your frame shouldn't need any change. In today's test a client joined about 3 s after the camera came up on channel 11.
- **If you'd rather it stayed fixed,** say so and I'll put channel 6 back.

Name, password and the two-device limit are unchanged.

## Other camera changes you might notice

- **Transfers:**
  - Each connection can have twice as much data in flight (8 packets, not 4).
  - The camera waits 15 s, not 5, before giving up on a slow client, so a busy phone no longer gets half a photo.
  - Typical speed is still about 110-150 KB/s. The limit looks like the radio, not buffers.
- **Thumbnails:** 50-450 ms each over Wi-Fi.
- **New routes, for the page's Home Screen icon:** `/icon-180.png`, `/apple-touch-icon.png` and `/apple-touch-icon-precomposed.png`. Nothing you use changed.
- **File names:** `/thumb/` and `/file/` now ignore a `?...` after the name.
- **Still to come, after the gift:** the file-number fix. Numbering can restart at `DSC_0001` after a format and a restart. Your name-plus-size matching already copes with that.

## Pairing

- **Network name:** `digicam-2b9d` for now. If I set the owner's name before the gift it becomes `digicam-<name>`, and I'll send you the exact name.
- **Password:** unchanged.
