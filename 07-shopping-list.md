# Shopping list: parts that join everything together

12 Oct 2026, for camera 1 (v2 body). Search terms are for Lazada. Buy the small packs; the spares cover camera 2.

## Must buy

| # | Part | Search Lazada for | Qty | Notes |
| --- | --- | --- | --- | --- |
| 1 | Battery socket with wires (pigtail) | **Not settled yet (12 Oct).** The seller's photos say "2P 2.54 socket" (JST XH), but the plug measured 4.4 mm one way, which is the thin side of a JST PH2.0 plug. Measure both sides of the plug's face (the end with the two holes): 5.8 × 4.5 mm is PH2.0 (`JST PH2.0 2 pin connector with wire`); 7.3 × 5.7 mm is XH2.54 (`JST XH2.54 2 pin connector with wire`); about 4.2 × 3.3 mm is MX1.25. If unsure, buy PH2.0 and XH2.54 pairs (about $2 each) | 1 pack | You want the half that mates with the battery's plug. Check polarity with the meter before plugging in |
| 2 | Camera with a longer ribbon, or a shorter extension | See "Camera ribbon" below | 1 | Measure first |
| 3 | Silicone wire, 30 AWG | `30AWG silicone wire` | One colour is fine: you need about 0.7 m (about 20 short pieces), so the smallest reel (1–2 m) is plenty | Screen, buttons, battery level. Soft and doesn't melt when soldering |
| 4 | Silicone wire, 26 AWG | `26AWG silicone wire` | About 0.2 m (3 wires). Red and black if you can; with one colour, mark the + wire with a red pen or red heat shrink, because swapping + and − can destroy the board | Power: charge module → switch → board 5V and GND |
| 5 | Resistors, 100 kΩ | `100k resistor 1/4W` | 1 pack | Battery level: need 2 |
| 6 | M2 heat-set inserts | `M2 heat set insert brass` | 1 pack (50) | Need 11. **Get the ones about 3.5 mm across (OD 3.5, length 3–4 mm).** The case holes are 3.2 mm. If you can only find 3.2 mm-across inserts, tell me and I'll shrink the holes to 2.9 mm |
| 7 | M2 screws | `M2 screw assortment kit` (button head or pan head, black or stainless) | 1 kit with 3, 4 and 5 mm lengths | Need 14 × M2×4 and 5 × M2×3. Black button-head hex screws look best through the clear plates |
| 8 | Heat-shrink tube | `heat shrink tube 1.5mm` (or a small-size assortment) | 1 pack | Every soldered joint |
| 9 | Double-sided foam tape, 1 mm | `double sided foam tape 1mm` | 1 roll | Both faces of the battery |
| 10 | Thin double-sided tape | `3M 9448A` or `double sided tissue tape` | 1 roll | Charge module to the sled (must be thin, not foam) |

## Camera ribbon

The camera sits in its cradle at the top middle of the body (seen from the back). Its ribbon comes out of the slot under the cradle and runs down to the board's camera socket. How far that is depends on where the socket is on your board. I estimate **25–45 mm**, but measure it:

1. Print the frame, or lay the board and camera on paper at the positions in `guide/steps/step-09.png`.
2. Run a strip of paper from the bottom edge of the camera to the board's camera socket, following the route.
3. Add 10 mm of slack.

Two ways to cover that length:

| Option | Search Lazada for | Pros | Cons |
| --- | --- | --- | --- |
| **A. A camera module with a longer tail (recommended)** | `OV3660 camera 75mm` or `OV3660 40mm` (24 pin). If no OV3660 has the length you need: `OV2640 75mm 24pin` | No extension board: thinnest, nothing to come loose, cleanest look | Same lens and sensor only if it's an OV3660. OV2640 works with the firmware but is 2 MP instead of 3 MP |
| **B. A shorter extension, like the one you have** | `24pin 0.5mm FFC extension` plus `24pin 0.5mm FFC cable 50mm` (or 30 mm), **same-side contacts, often called type A** | Keeps your current camera | The adapter board (about 20 × 10 mm) needs a spot to sit, and it's one more joint to come loose |

Pick a length close to your measurement: 40 mm for up to about 35 mm, 75 mm above that. Extra ribbon has to be folded, and folds show through the clear plate.

### Listings found on Lazada (12 Oct)

**Easiest to order: keep your camera, buy a joiner board and a short ribbon.** Both listings name their options clearly.

| Buy | Listing | Choose |
| --- | --- | --- |
| Joiner board (two 24-pin sockets at opposite ends of one small board; ribbon not included) | https://www.lazada.sg/products/pdp-i13912936039.html (cheaper, same seller as the next link) or https://www.lazada.sg/products/pdp-i3614235366.html | **24Pin**, 1PC (about $3.40) or 2PCS for a spare |
| Ribbon, 24-pin, 0.5 mm, contacts on the same side at both ends ("Forward", A type) | https://www.lazada.sg/products/pdp-i3540086072.html | **5CM_24P 0.5MM** (10 pcs, about $7). Take 10CM_24P instead if your measured route is over 45 mm |

How it connects:

1. The camera's own short ribbon plugs into one side of the joiner.
2. The new ribbon goes from the other side of the joiner to the board.

Before ordering, look at the cable in your current extension: if the stiffened contact ends are on the same face at both ends, it's the same "forward" type, and that's what works with your board.

**Camera with a long ribbon built in (neater, but the listings are unclear):** these two sell an OV3660 in 21 mm and 75 mm versions, but label the choices only as "Bundle 1–4". Ask the seller in chat which bundle is the 75 mm ribbon before buying.

- https://www.lazada.sg/products/pdp-i13906025131.html (OUHOE, about $10.50)
- https://www.lazada.sg/products/pdp-i13908952492.html (NN77, about $11)

**Ribbon tips:**

- Ribbons bend fine, but never crease them flat.
- One gentle fold to change direction is fine.
- For option B, match the extension cable's contact side to your current extension. If the camera doesn't start ("camera not found" on screen), the cable is the wrong type, not broken.

## Nice to have

| Part | Search Lazada for | Why |
| --- | --- | --- |
| 8-pin plug-and-socket pair with wires, 1.0 mm | `JST SH1.0 8 pin connector with wire` | Lets you swap the 1.14 in screen for the 2.0 in later by unplugging, instead of redoing 8 joints |
| Capacitor, 470 µF 6.3 V, small | `470uF 6.3V capacitor` | Only if the camera restarts when it starts recording on battery. It goes across the board's 5V and GND |
| Kapton tape | `kapton tape 10mm` | Holds wire bundles flat, insulates bare pads |
| Solder wick and flux pen | `solder wick 2mm`, `flux pen` | For taking the pin headers off |
| Clear PETG filament, or 1.5 mm clear acrylic sheet | `clear PETG filament` / `acrylic sheet 1.5mm clear` | The clear plates. Laser-cut acrylic (DXFs in `cad/export/v2/`) is glass-clear; printed PETG is frosted |

## Already have (from the plan)

| Part |
| --- |
| ESP32-S3 camera board |
| OV3660 camera and an extension |
| 2000 mAh battery |
| Charge module |
| 1.14 in screen (2.0 in on order) |
| Buttons |
| Slide switch |
| microSD card |
| Multimeter |

Check these before ordering: female-to-female jumper wires for bench tests, leaded solder, and a USB-C cable that does data, not just charging.
