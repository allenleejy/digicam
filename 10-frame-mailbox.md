# DigiGallery frame mailbox

How the phone page decides what **DigiGallery**, Allen's desk photo frame, shows: which photos are on it, and how it shows them. The camera keeps a small "frame mailbox" that the phone page writes and the frame reads.

- **History:** this replaces the 21 Oct `/small` connector request. Its file-number fix moves to after the gift.
- **Upkeep:** keep this doc up to date. If the contract changes in a way an old frame wouldn't understand, raise `v`.

| Item | Status (23 Oct 2026) |
| --- | --- |
| 1. `GET` and `POST /api/frame` | In firmware 0.4.0, flashed 23 Oct; tried over Wi-Fi with the frame and an iPhone the same day |
| 2. The document (below) | Written by the phone page exactly as specified |
| 3. Phone page: Frame pill and sheet, badges, viewer button, Send to frame | In the new-design page (23 Oct), tested on the laptop (`host/server.py` at `/phone/`). Main flows tried on an iPhone, 23 Oct |
| 4. `/small` (optional) | In firmware 0.4.1 (23 Oct, written by Allen's session): `small.c`, cached in `DCIM/.small/` |
| 5. "frame here" on the camera's screen (optional) | Built: shown when a `DigiGallery/` client reads `/api/frame` |

## The frame (nothing to change on the camera for this)

- **Hardware:** ESP32-C3 with a 160 × 128 screen and about 300 photos' worth of flash. Sits on a desk, on USB power.
- **Joining:** in Phone mode it joins the camera's Wi-Fi as an ordinary client, using one of the two places for a few seconds at a time. It finds the camera at the gateway, 192.168.4.1, and sends `User-Agent: DigiGallery/<version>`.
- **Each check:**
  1. It reads `GET /api/frame`.
  2. It fetches `GET /thumb/NAME` for any chosen photo it doesn't have yet.
  3. It deletes the photos that are no longer chosen, and leaves.
- **How often:** every 30 s while the mailbox keeps changing. After about 3 minutes with no change it stops until Phone mode starts again, so the camera can still sleep.
- **With older firmware:** 0.3.0 has no `/api/frame` and answers 404, so the frame takes every new photo instead.
- **What it never does:** call `/api/delete` or `/file/`, or touch videos.

## Camera routes

- **`GET /api/frame`:** `200`, `application/json`, `Cache-Control: no-store`, with the stored bytes exactly. `204` with no body when there's no mailbox.
- **`POST /api/frame`:** the body is the whole document, at most 16 KB.
  - Larger: `413`, and the connection closes.
  - Not JSON, or not an object at the top level (checked with cJSON): `400`.
  - Otherwise `{"ok":true}`.

**How it's stored:**
- **Where:** `/sdcard/frame.json`, outside `DCIM`, so the photo list and the name check never see it. Formatting the card removes it; the frame then keeps what it has.
- **Writing:** to `frame.tmp`, synced to the card, then swapped in, with the old copy moved aside to `frame.old` first.
- **After a power cut:** if one hits mid-swap, start-up puts `frame.old` back, so there's always a whole document.

**Memory:**
- The body is received into Phone mode's 16 KB internal read buffer, which is also the size limit, and written to the card from there.
- cJSON's nodes are allocated in PSRAM. That's about 50 KB for 300 photos, so internal RAM isn't touched (about 38 KB free in Phone mode).
- Each save logs the internal free heap.

**Routes:** `cfg.max_uri_handlers = 12`.

## The document

The phone page writes exactly this shape. **Keys come in this order, with the settings before `"items"`: the frame reads the document as it streams in.**

```json
{"v":1,"rev":7,"interval":10,"transition":"fade","look":"vivid","order":"newest","brightness":100,
 "items":[{"n":"DSC_0012.JPG","s":245123},{"n":"DSC_0009.JPG","s":231877}]}
```

| Key | Meaning | Values (default in bold) |
| --- | --- | --- |
| `v` | Document version | **1** |
| `rev` | Up by 1 on every save; the frame skips its work when it hasn't changed | integer, from **1** |
| `interval` | Seconds each photo stays on screen | 5, **10**, 30, 60, 300 |
| `transition` | How one photo gives way to the next | `"cut"`, **`"fade"`**, `"slide"` |
| `look` | The frame's colour treatment | `"natural"`, `"contrast"`, **`"vivid"`**, `"warm"` |
| `order` | Slideshow order | **`"newest"`**, `"oldest"`, `"shuffle"` |
| `brightness` | Backlight, percent | 30, 60, **100** |
| `items` | The photos on the frame, newest first: `DSC_` only, `n` and `s` exactly as `/api/list` gives them, at most 300 | **empty** |

The frame shows exactly the photos in `items`. With no document, the frame uses the defaults and shows nothing new.

## The phone page (`firmware/web/phone.html`)

- **Loading:** it reads `/api/frame` alongside `/api/list`. A `204` means the defaults with no photos. A 404 (older firmware) hides the frame features.
  - It saves only after it has read the camera's copy, so a page that couldn't read it never overwrites it.
- **Frame pill** beside Select, opening a sheet with:
  - "N photos on the frame (up to 300)";
  - Change every, Transition, Look, Order and Brightness;
  - "Take all photos off the frame", which asks first.
- **Badge:** a small picture badge on each tile that's on the frame.
- **Viewer:** a button reading "Add to frame", or "On frame ✓" to take the photo off again. Videos don't get it.
- **Select mode:** **Send to frame** beside Save. It adds the picked photos and skips videos.
- **Saving:**
  - Every change POSTs the whole document with `rev` + 1, one save at a time. A change made during a save follows straight after.
  - Success: "Saved. The frame picks it up within a minute while Phone mode is on."
  - Failure: "Couldn't save to the camera". The page keeps its state, so the next change saves everything.
- **Deleting** a photo that's on the frame also takes it off the frame list.
- **Photos deleted on the camera itself** stay on the list. The sheet shows "N more only on the frame" with a "Take them off" link.
- **More than 300:** refused with a toast.

## Item 4: `GET /small/NAME?w=W&h=H` (firmware 0.4.1)

The real photo, shrunk for a small screen. The thumbnails are the preview frame from just before the shot, so they're soft and dull.

- **Accepts:** `DSC_nnnn.JPG` only, with the `/thumb` name check; anything else is 404. `w` and `h` must be 16 to 640, or 400.
- **Returns:** exactly W × H, scaled to cover and centre-cropped. The frame asks for 320 × 256 and halves it while decoding.
- **How:**
  1. Read the photo into PSRAM.
  2. Decode it with `esp_jpeg` at 1/4 for 320 × 256.
  3. Shrink with a box filter.
  4. Encode with `fmt2jpg()`: baseline, quality about 80.
- **Cache:** saved as `DCIM/.small/DSC_nnnn_WxH.JPG` through a temp file. `storage_delete` removes the copies.
- **Errors:** 500 on a decode or encode failure. The frame falls back to `/thumb` on any error.
- **As built:** `small.c` decodes at the biggest reduction that still covers W × H, box-filters the middle, and encodes at quality 80; a picture over 24 KB is encoded again at 60, because the frame takes one picture of up to 32 KB at a time. The first request for a photo takes about a second (logged as `small: ... decode N ms, shrink N ms, encode N ms`); after that the copy on the card is sent. `GET /small` logs the internal free heap.

## The file-number fix (firmware 0.4.1)

Numbering used to start again at `DSC_0001` after a format and a restart, which could confuse the phone page's "saved" ticks and Safari's cache (names are served as `immutable`). Since 0.4.1 the next number is also kept in NVS (`digicam` / `next_num`), and the camera uses the higher of that and the card's highest file. The mailbox and the frame know a photo by name and size, so they were never confused by it.

## Tests

From a laptop joined to the camera's Wi-Fi:

```bash
curl -i http://192.168.4.1/api/frame                     # 204 on a card with no frame.json
printf '{"v":1,"rev":1,"interval":10,"transition":"fade","look":"vivid","order":"newest","brightness":100,"items":[]}' > f.json
curl -s -X POST --data-binary @f.json http://192.168.4.1/api/frame   # {"ok":true}
curl -s http://192.168.4.1/api/frame -o g.json && cmp f.json g.json  # identical
head -c 20000 /dev/zero | tr '\0' 'a' > big.txt
curl -s -o /dev/null -w "%{http_code}\n" -X POST --data-binary @big.txt http://192.168.4.1/api/frame    # 413
curl -s -o /dev/null -w "%{http_code}\n" -X POST --data-binary 'nonsense' http://192.168.4.1/api/frame  # 400
```

Then:
- **Persistence:**
  - Restart the camera: `GET /api/frame` returns the same document.
  - Format the card: `204` again.
- **On an iPhone:**
  - add a photo from the viewer and see the badge;
  - pick three in Select mode and use Send to frame;
  - change each setting, reload, and check everything was kept;
  - delete a photo that's on the frame and check it leaves the frame list;
  - take all photos off the frame.
- **Nothing broken:** everything that worked before still works (list, thumbnails, opening a photo, Save to Photos, Delete, the Shortcut).

**Already passed on the laptop** (`host/server.py` stands in for the camera's routes at `/phone/api/frame`):
- the curl tests above;
- every phone page flow listed here, with a stand-in photo list.

## Results to send back to Allen

| | |
| --- | --- |
| Firmware version and items | 0.4.0: items 1, 2, 3 and 5. 0.4.1: item 4 too, and the file-number fix |
| A `frame.json` the page wrote | `{"v":1,"rev":5,"interval":5,"transition":"cut","look":"natural","order":"newest","brightness":100,"items":[{"n":"DSC_0010.JPG","s":144739},{"n":"DSC_0009.JPG","s":135898},{"n":"DSC_0008.JPG","s":123796},{"n":"DSC_0005.JPG","s":113512}]}` (from an iPhone, 23 Oct, read off the camera over USB) |
| Internal free heap after a `POST /api/frame` | (camera log: "phone: heap after POST /api/frame: internal free N KB, largest block N KB, lowest N KB") |
| Internal free heap in Phone mode, nobody joined | 38 KB, largest block 26 KB (0.4.0, 23 Oct) |
| Internal free heap, iPhone and frame both joined | Lowest 34 KB, largest block 24 KB (23 Oct joint test, current build) |
| Contract changes | None to section 2. The Wi-Fi channel is no longer fixed at 6: Phone mode picks the quietest of 1, 6 and 11 at start (the frame finds the camera by name on any channel) |
| Wi-Fi name | `digicam-2b9d` now. Becomes `digicam-<name>` if `OWNER_NAME` is set in `personal.h`; the password stays the same |
