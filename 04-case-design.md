# Camera 1 case design

## v3 bodies (23 Oct 2026): print one of these

**Built for the parts as they arrived on 23 Oct.**

- **Files:**
  - model: `cad/camera1_v3.scad`;
  - print files: `cad/export/v3/clear/` and `cad/export/v3/retro/`;
  - renders: `cad/renders/v3/`.
- **Rebuilding:** `python3 cad/export_v3.py` re-exports everything and runs `cad/check_v3.py`. That checks 196 pairs of parts in both styles, and none overlap.
- **History:** v2 (below) was built for the 1.14 in screen and the extension ribbon, and no longer fits.

| | Clear | Retro |
| --- | --- | --- |
| Look | InstantSnap Novo-like (restyled 23 Oct from your reference picture): a frosted tinted shell with a chamfered rim standing 0.9 mm proud of both faces, a raised bezel round the screen and buttons, a separate panel over the battery, an orange pill shutter and the power switch on the top left, a strap loop on the left side | Rangefinder: textured covering front and back, a separate top plate with shutter, dial and accessory shoe, a knurled lens barrel, strap lugs |
| Size | 112 × 68 × 22.3 mm with the rim (20.5 mm plates) | 112 × 68.2 × 22.1 mm to the front rim, 23.3 mm over the back covering (lens barrel adds 6 mm, lugs 3 mm each side) |
| Parts to print | 9 | 13 (14 with the two-colour portrait inlay) |

**Step-by-step assembly, wiring diagrams and wire lengths: `11-assembly-guide-v3.md`.**

### Retro update, 24 Oct: edges, leather, portrait, snap-in switches

![Retro back](cad/renders/v3/retro_back.png)

- **Top plate joint.** Before, the top plate's ends were rounded front to back (r 3) while the body below had square edges and rounded top corners, so the silver curved in where it met the body. Now:
  - the body's top corners are square, so its sides run straight up into the top plate;
  - the body and the top plate share one edge profile: a 2 mm fillet on the front, and on the back a 2 mm fillet that turns into a 45° chamfer (so the back plate prints face down without support);
  - the front plate now sits inside a rim on the frame that stands 1.2 mm proud of it, so the front covering is flush with the rim and with the top plate's front.
- **Leather grain.** Both coverings have a pebble-grain hide texture: Voronoi pebbles about 1.4 mm across, 0.4 mm deep, with a few long creases. It's a heightmap, `cad/art/leather.png`, made by `cad/art/make_leather.py`, on a 0.8 mm base. A 0.5 mm smooth border runs round each covering.
- **Portrait on the back covering.** The drawing is `cad/art/portrait.svg` (mm units, three layers), and the model imports it, so **edit the SVG to change the drawing** (Inkscape, Figma, Illustrator), then re-export. `cad/art/portrait_cutout.png` is the photo it was traced from, with the background removed.
  - `figure`: the silhouette, left as smooth leather (no grain), level with the grain's top.
  - `ink`: the drawing, sunk 0.6 mm into the covering.
  - `cut`: lines taken back out of the ink (hair strands, collar, sleeve seams).
  - Keep lines 0.3 mm or wider (0.4 mm safer).
  - Refined 24 Oct. The hair strands follow the photo's hair direction, measured from the picture; the fringe has pointed tufts. Each eye is an almond with the iris where the photo has it (about 0.9 mm across, toward his gaze), 15% larger for legibility. The eyebrows taper from the nose end.
  - Size and place: `portrait_h` and `portrait_at` in `camera1_v3.scad`.
- **Two ways to print the portrait:**
  1. **Two colours (AMS):** add `portrait_inlay.3mf` to `back_leatherette.3mf` as a part of the same object (Bambu Studio: right-click the covering → Add part → Load), and give the inlay a dark colour. The inlay fills the sunk drawing exactly, so it comes out flush.
  2. **One colour:** print the covering alone. Fill the sunk drawing with dark acrylic paint and wipe the face clean.
- **Switches snap into the carrier.** Each pocket has two flexible fingers, 3 mm wide and 0.9 mm thick, with slits beside them. Their catches click 0.35 mm over the edge of the switch's metal top, and a lead-in chamfer helps them on. So the carrier, the three switches and their wires are one piece you can solder and test before it goes in; then the carrier screws to the back plate's two posts as before. The catch height assumes the switch body is 3.5 mm from its base to its metal top (`btn_body`): **measure one**.
- **Printing fixes found on the way:**
  - the top plate's edges at the bed now end in 45° chamfers;
  - the dial and accessory shoe are exported upside down (peg up), so neither hangs over its thin peg;
  - the coverings are 3MF files: a clean mesh, about 6 MB each.
![Top plate joint, before and after](cad/renders/v3/retro_joint_before_after.png)
![Portrait, one colour and two colours](cad/renders/v3/retro_portrait.png)
![Portrait against the photo, before and after the 24 Oct refinement](cad/renders/v3/retro_portrait_compare.png)
![Switch snap fit, section](cad/renders/v3/carrier_snap_section.png)

- **Checks:** `check_v3.py` now measures each overlap's volume, so parts that only touch don't count. 256 pairs, none overlap.

![Clear](cad/renders/v3/clear_back.png)
![Retro](cad/renders/v3/retro_front.png)

### What changed, and why

- **2.0 in screen.** The 58 × 35 mm module sits behind the board, in a lip on the back plate that locates its glass. A thin foam pad between the screen and the sled clamps it when the case closes, so it needs no screws. The corner screw posts moved into free strips around the screen.
- **No camera extension.** The camera folds back onto the board's ESP32 module, so the board decides where the lens is.
  - A square collar on the inside of the front plate centres the lens holder and holds it down as the plate goes on.
  - Stick the camera to the module with a small square of 0.4 mm foam tape first.
  - The camera-on-module stack sets the thickness at about 20.5 mm, up from 15 mm.
- **Buttons with real seats:**
  - Each switch drops into its own pocket in a **button carrier**, a small printed tray with slots for its feet and wires. The carrier screws to two posts on the back plate.
  - Each cap runs in its own **guide sleeve**, so it can't wobble, and sits in its own **recessed dish** on the back, so the three are clearly separate.
  - The ◀ and ▶ caps carry arrows and are keyed so they can't turn. ● has a dot.
  - The shutter switch sits in a cradle under the top wall (clear), or in a socket under the top plate (retro).
- **Charge module** in front of the battery, against the left wall (seen from the back). Its USB-C comes out of the camera's right side seen from the front. A rim on the front plate holds it.
  - Clear: its LEDs show through the clear front.
  - Retro: a 2 mm peep hole in the front plate.
- **Battery** in its bay, with a lead gap at the top right. The left strip beside it is a wire lane for the battery leads and the switch wires.
- **Shutter and slide switch:**
  - Clear: both on the top left (seen from the back), above the battery. The shutter is an orange pill.
  - Retro: both in the top plate; the shutter on the right, the switch on the left.
  - Both bodies put a printed cap on the slide switch's knob, glued on with CA: the bare knob doesn't reach the outside.
- **Board USB-C** on the right side seen from the back (the left side seen from the front), as before.

### Parts, how to print them, and colour ideas

Everything prints flat without supports at 0.2 mm layers with 3 walls. The STLs are already in the right orientation.

**Clear frame (found 24 Oct, not fixed yet):** its corner posts, battery divider and sled blocks start on top of the back plate's recess, so they hang 2.9 mm above the bed. Turn on supports from the build plate only for that part. The retro frame doesn't have this problem.

| Part | File | Print | Clear: colour | Retro: colour |
| --- | --- | --- | --- | --- |
| Frame (the sides) | `frame.stl` | Standing on its back edge | Translucent mint PETG | Black |
| Back plate | `back_plate.stl` | Outside face down | Translucent mint or clear PETG | Black |
| Front plate | `front_plate.stl` | Outside face down | Translucent mint or clear PETG | Black |
| Sled (board and screen stack) | `sled.stl` | Flat | Any; it shows through, so a colour | Any |
| Button carrier | `carrier.stl` | Floor down | Accent colour | Any |
| Caps: ◀ ● ▶, the shutter, the switch cap | `caps.stl` | As exported | Orange; switch cap grey | Silver or cream; switch cap black |
| Lens ring (clear) | `lens_ring.stl` | Flat | Accent | |
| Battery panel (clear) | `panel.stl` | Flat | Frosted white or mint | |
| Screen bezel (clear) | `bezel.stl` | Flat, outer face up | Frosted white or mint | |
| Top plate (retro) | `top_plate.stl` | Upside down, top on the bed | | Silver or cream |
| Dial, accessory shoe, shutter collar | `top_bits.stl` | As exported (dial and shoe upside down, peg up) | | Black |
| Lens barrel | `lens_barrel.stl` | Back face down | | Silver |
| Front covering | `leatherette.3mf` | Flat, grain up; 0.1–0.12 mm layers bring out the grain | | Brown or tan |
| Back covering, with the portrait | `back_leatherette.3mf` | Same | | Same as the front |
| Portrait inlay (two-colour print only) | `portrait_inlay.3mf` | A part of the back covering's object, not on its own | | Dark brown or black |
| Window inserts (viewfinder, rangefinder, eyepiece) | `window_inserts.stl` | Flat | | Black or dark grey |

**Glass-clear plates:** for the clear style, the plates can instead be laser-cut from 2 mm acrylic using `clear/*_lasercut.dxf`. They then lose the button sleeves, the carrier posts, the screen lip, the camera collar and the charger rim, so use the printed plates for the first build.

**Retro coverings:** they go on with thin double-sided tape. The front one sits inside the rim, flush with it. Both are cut around the screw heads (and the front one around the barrel), so the camera can still be opened.

### Hardware

| Item | Clear | Retro |
| --- | --- | --- |
| M2 heat-set inserts (3.5 mm OD) | 11 (8 in the frame's corner posts, 3 in the sled blocks) | 16 (+2 under the top wall for the top plate, +3 in the lens barrel) |
| M2 × 5 button head | 8 (plates) + 4 (board to sled) | 8 + 4, + 3 (barrel, from inside the front plate) |
| M2 × 4 | 3 (sled to frame) | 3 |
| M2 × 6, or × 5 | 2 (button carrier to the back plate's posts) | 2 |
| M2 × 14 countersunk (flat head) | | 2 (top plate, from the top: visible, as on old cameras). Not in your kit; M2 × 12 will also do |
| Foam tape | Battery to back plate, camera to module (0.4 mm), screen to sled (0.5 mm) | Same |

### Assembly order

1. **Inserts:** heat-set them into the frame's posts (both faces) and sled blocks. For retro, also the two bosses under the top wall and the barrel.
2. **Solder and test on the desk first** (`05-bench-bring-up.md`):
   - the four buttons, on two diagonal feet each;
   - the slide switch;
   - the screen's 8 wires;
   - the battery to the charge module (below);
   - the module's OUT through the switch to the board's 5V.
3. **Board onto the sled** with 4 screws. Fold the camera onto the ESP32 module, lens facing away from the board, on a square of foam tape.
4. **Back plate:**
   - drop the caps into their sleeves from the inside, with the ◀ ▶ keys in their slots;
   - put the switches into the carrier's pockets, plungers toward the caps, wires out through the slots;
   - screw the carrier onto its two posts;
   - lay the screen in its lip, glass down, with the foam pad on its back.
5. **Frame onto the back plate**, with 4 screws from the back.
6. **Inside:**
   1. Battery (foam tape) into its bay.
   2. Charge module on its front, USB-C to the side wall.
   3. Shutter switch into its cradle or socket, and slide switch into its pocket.
   4. Feed the screen wires through the sled's slots.
   5. Lower the sled stack onto its three blocks and screw it down.
7. **Retro:** the top plate goes on with 2 screws from the top. Then screw the barrel on from inside the front plate, before the plate goes on.
8. **Front plate:**
   - Check the lens lines up with the hole: the collar slides over the camera as the plate goes on.
   - Fit 4 screws.
   - Clear: glue on the lens ring. Retro: tape on the coverings.

### Wiring changes from the plan

- **Battery plug:**
  - The pigtail you bought has the same male plug as the battery, so the two can't mate. Cut the battery's plug off and solder its leads straight to the charge module's B+ and B−. That's the normal way.
  - **Cut one lead at a time**, and cover the cut end with tape before cutting the other, so the two never touch. A shorted LiPo gets hot fast, even a protected one.
  - Strip 2 mm, tin, slide heat-shrink on first, and solder red to B+.
  - Check polarity with the meter before the module's OUT goes anywhere.
- **Camera:**
  - It plugs straight into the board, so there's no ribbon to route.
  - The folded ribbon has to bend 180°: keep the bend gentle and don't crease it.
  - The camera now sits on the warm ESP32 module. The foam tape insulates it a little. If photos get noisy after long use, a thin pad of Kapton under the camera helps.

### Measure these before printing

The model uses these values. Most are estimates, marked `MEASURE` in the file. Send the numbers and I'll update the model, re-export and re-run the fit check.

1. **2.0 in screen module:**
   - outer size (58 × 35 assumed);
   - thickness from the glass face to the back with the pins cut (4.0);
   - where the visible picture area sits, from the module's bottom-left corner with the glass facing you (8.6, 2.2 assumed: centred);
   - the hole positions.
2. **Camera on the board,** seen from the back of the camera (the board's underside facing you):
   - the lens centre from the board's left end (14.0) and from its bottom edge (14.05);
   - the height of the lens front above the board (8.7 assumed: 3.3 module + 0.4 tape + 5.5 camera);
   - the length of the round part of the lens (2.0).
3. **Board:**
   - where the USB-C socket(s) are along its end (14.0 from the bottom edge). If both sockets are on that end, give both;
   - the tallest part on the underside (2.0, the microSD socket).
4. **Slide switch:** body and knob size (8.6 × 3.6 × 3.6, knob 2.0 × 1.5).
5. **Charge module:** height of its USB-C socket's centre above the module's back (2.6).

**Quick test print first:**
1. Print the back plate, carrier and caps, about 1.5 h. That checks button feel and the screen window against the real screen.
2. Print the front plate, about 40 min. Hold it against the board stack to see the lens line up.
3. Then print the rest.

### Firmware

**The firmware still drives the 1.14 in screen (240 × 135).** The 2.0 in screen needs these changes in `display.c` and the drawing code:
- 320 × 240;
- its own orientation and offset;
- the menus laid out for the bigger screen.

The simulator already renders every screen, so the layouts can be checked on the laptop.

## v2 test-fit body (10 Oct 2026)

**Built around the parts as you measured them, with the 1.14 in screen.** Model: `cad/camera1_v2.scad`. Printable files: `cad/export/v2/`. Renders: `cad/renders/v2/`. `cad/check_v2.py` checks every pair of parts for overlaps (75 pairs, none overlap).

**Print-file fix, 11 Oct:** the model's X runs left to right seen from the back while Z runs back to front, which is a mirror image of real space. The files exported before 11 Oct (v1 and the first v2 export) would have printed as a mirror image, with the buttons and shutter on the wrong side. `camera1_v2.scad` now mirrors every print export, and `export/v2/` was re-exported. The v1 files in `export/` and the Fusion model still have the old orientation.

**Size: 110 × 64 × 15.1 mm**, down from v1's 116 × 66 × 15.2. The Novo is 16 mm thick.

### What changed from v1

| | v1 (8 Oct) | v2 |
| --- | --- | --- |
| Board and screen | Loose in the cavity, held by the plates | Screwed to a printed **sled**: board on its front face, screen on its back face. You build and test the electronics on the sled outside the case, then it drops in on 3 screws |
| Charge module | Pocket at the top right, behind the buttons | Taped on the sled beside the screen, its USB-C out the right side; visible through the back plate, LEDs included |
| Power switch | Top, middle; its pocket was closed on every side, so the switch couldn't go in | Top, beside the shutter. The pocket opens to the front; the switch drops in and slides up into its slot |
| Battery | Divider only | Divider, two ribs off the left wall, one down from the top: it can't slide. 1 mm foam on both faces stops rattle |
| Wiring | Not planned | Lanes (below) |
| Buttons | 6 × 6 through-hole | Your 5 × 5 flat-foot switches, glued in pockets in a shelf, with wire slots |

### Layout, seen from the back

| Zone | What's there |
| --- | --- |
| Left | Battery, standing, leads out of the top |
| Top-left corner | Keychain hole |
| Bottom right | Sled: screen on the back face (centred over the board), charge module to its right, board on the front face |
| Top middle | Viewfinder, then the camera (lens on the front), each in a tube through the body |
| Top right | ◀ ● ▶ on the back; shutter and power switch on the top edge |
| Right side | Two USB-C openings: charging (upper, nearer the back) and the board's own (lower, nearer the front) |

### Wiring lanes

- **Top lane:** above the viewfinder, under two printed bridges. The battery leads run here from the top of the cell.
- **Down beside the lens:** a 2.6 mm gap between the viewfinder tube and the camera tube. Battery leads and power-switch wires go down here.
- **Across above the sled:** a clear strip under the buttons, from that gap to the charge module.
- **Through the sled:** a window behind the screen, slots under both short edges of the screen (for its 8 wires, whichever side its pads are on), and a window on each side of the screen to pass wires from back to front.
- **Buttons:** a slot beside each pocket in the shelf. Over the shelf, at the front, there's room for the switch wires.

### Hardware

| What | Qty | Where |
| --- | --- | --- |
| M2 heat-set insert, 3.2 mm hole, 3.5 mm long | 11 | 8 in the corner posts (both faces), 3 in the sled blocks |
| M2 × 4 screw | 14 | 8 for the plates, 3 for the sled, 3 for the board |
| M2 × 3 screw | 5 | 4 for the screen; 1 for the board's top-right corner, which has the charge module behind it |
| 1 mm foam tape | Small piece | Both faces of the battery |
| Thin double-sided tape (not foam), or a dab of hot glue | Small piece | Charge module to the sled |
| Superglue or hot glue | | Buttons in the shelf, shutter switch on its ledge, slide switch in its pocket |

### Assembly order

1. **Inserts:** press the 8 plate inserts into the corner posts from both faces, and 3 into the blocks the sled sits on (two on the bottom wall, one under the viewfinder).
2. **Build the sled outside the case:**
   - Screw the screen to the back face (4 × M2 × 3).
   - Screw the board to the front face (M2 × 4, and M2 × 3 at the top-right corner).
   - Tape the charge module beside the screen, with its USB-C socket flush with the sled's right-hand edge, not past it. The sled drops in straight, so a socket that sticks out would catch on the wall.
   - Wire the screen to the board through the slots, and **bench test it**.
3. **Into the frame, before the sled:**
   - Glue the three buttons into the shelf with their plungers toward the back, and the shutter switch on its ledge, plunger up.
   - Drop the slide switch into its pocket from the front, slide it up until the knob comes through the top, and fix it with a dab of hot glue underneath.
   - Seat the camera in its cradle and feed the ribbon out of the slot at the bottom of the cradle.
4. **Sled in:** lower it in from the front, screen first, and screw down its 3 tabs (M2 × 4). Connect the camera ribbon.
5. **Battery:** foam on both faces, leads up and along the top lane under the bridges, down beside the lens, then across to the charge module.
6. **Wire the controls,** then tack each bundle down.
7. **Close up:**
   - Put the caps in the back plate and screw it on.
   - Screw on the front plate.
   - Glue on the lens ring and the viewfinder trims.

### Printing

| Part | File | Notes |
| --- | --- | --- |
| Frame | `frame.stl` | As exported: back face down. Everything grows from the bed except the button shelf (a 30 mm bridge: turn on supports if your bridges sag), the lane bridges and the camera ledge (both short). 0.16 mm layers |
| Sled | `sled.stl` | As exported: flat face down, spacers up. 100% infill is cheap at this size and makes the screw holes strong |
| Back and front plates | `back_plate.stl`, `front_plate.stl` | For this test fit, any filament. Later: clear PETG, or acrylic from the DXFs |
| Caps, lens ring, viewfinder trims | `caps.stl`, `lens_ring.stl`, `vf_frames.stl` | Flange down |

### Still estimates: measure before the final print

The test print will show most of these. Calipers settle them first.

| Parameter | Now | What to measure |
| --- | --- | --- |
| `board_hole_in` | 2.6 | Board's corner holes: centre to each edge |
| `board_top` | 3.3 | Board's tallest part above the PCB, headers removed |
| `board_usb_c` | 14 | Board's USB-C: centre, from the board's bottom edge; and which end it's on (assumed the right end, seen from the back) |
| `scr_t` | 3.8 | Screen: glass face to the back of its PCB |
| `scr_hole_in` | 2.2 | Screen's corner holes: centre to each edge |
| `scr_active` | 24.9 × 14.0, centred | Where the glowing area sits on the module (often not centred) |
| `chg_t` | 4.2 | Charge module, PCB plus USB-C socket; and which edge the socket is on (assumed an 11.8 mm edge) |
| `btn_h`, `btn_feet` | 6.0, 7.6 | Buttons: base to plunger top; feet tip to tip |
| `cam` | 8.5 × 8.5 × 5.5 | Camera module with lens |

**The 2.0 in screen will need a layout change:** at 58 × 35 mm it fills the whole sled back face, so the charge module has to move. The sled design stays.

**Fusion:** the parametric Fusion model is still v1. The Fusion connection was down on 10 Oct, so v2 was built in OpenSCAD. Rebuild it in Fusion once the test print has settled the layout.

---

## v1 concept (8 Oct): superseded by v2 above

v1 concept, 8 Oct 2026. Model: `cad/instantsnap_body.scad` (parametric OpenSCAD). Renders in `cad/renders/`, printable files in `cad/export/`.

**The body is a "clear sandwich" of 116 × 66 × 15.2 mm.** A coloured printed frame sits between two clear plates, so the battery, board and wiring show through, InstantSnap-transparent style. It is 0.8 mm thinner than the Novo (16 mm) but about 10% larger face-on, because the 67 mm board and 35 mm battery have to sit side by side to stay thin.

| View | File |
| --- | --- |
| Front, three-quarter | `cad/renders/front.png` |
| Back, three-quarter | `cad/renders/back.png` |
| Exploded | `cad/renders/exploded.png` |
| Frame alone, as printed | `cad/renders/frame_print.png` |

## Construction

```
back plate (clear, 1.6 mm)  ─┐
frame (coloured print, 12 mm) ├─ 4 × M2 screws per side into heat-set inserts
front plate (clear, 1.6 mm) ─┘
```

- **Frame:** printed in the accent colour. It carries everything that has to be solid: the corner screw bosses, the keychain corner, the viewfinder tube, the camera cradle, the button shelf, the shutter ledge, the power switch pocket, and battery locating ribs.
- **Clear plates:** two options, same outline:
  1. **Print them** in clear PETG on the Bambu: 100% infill, lines aligned, ironing on the top surface. Result is smoky-translucent, like frosted glass: internals visible but soft.
  2. **Laser-cut 1.5–2 mm clear acrylic** from `cad/export/*_lasercut.dxf`. Result is glass-clear, the closest to the Novo's clear edition. Any laser-cutting shop or Taobao `亚克力激光切割定制` can cut them; allow shipping time.
- **Fixing:** 8 × M2 heat-set inserts (two per corner post, one from each face) and 8 × M2 × 4 mm button-head screws. The screws are visible on the clear plates on purpose; it's part of the look.
- **Inside, nothing floats:** the battery and the board-plus-screen stack are each sandwiched between the plates (12 mm cavity, with a 1 mm foam pad on the battery). The camera module drops into a cradle on the front. The buttons sit in a shelf. The charge module and switches have pockets.

## Layout

Seen from the back (screen side). The front is the mirror image.

| Zone | What's there |
| --- | --- |
| Left half | 2000 mAh battery (35 × 52 mm), standing upright |
| Bottom right | Board (67 × 28 mm), with the 2.0 in screen (58 × 35 mm module) stacked on top of it facing the back |
| Top right | Viewfinder, camera lens (on the front), the three back buttons, the charge module behind them |
| Top edge | Shutter (right), power switch (middle) |
| Right edge | Two USB-C openings: charging (upper), and the board's own port for firmware updates (lower) |
| Top-left corner | Keychain hole, 4.2 mm, through the whole body |

## Controls

Held like a compact camera, right hand on the right side seen from the back.

| Control | Where | Does |
| --- | --- | --- |
| **Shutter** | Top edge, right: under the index finger | Photo, or start/stop video. In the gallery: back to live view |
| **◀** | Back, top right, left button | Previous film look; previous photo in the gallery |
| **●** | Back, middle button | Press: open/close the gallery. Hold: photo ↔ video; in the gallery, delete (asks first) |
| **▶** | Back, right button | Next film look; next photo |
| **Power** | Top edge, middle, in a finger recess | Slide on/off. The recess keeps it from switching on in a bag |

These map one-to-one onto the dial in the current firmware and simulator (◀ = turn left, ▶ = turn right, ● = press/hold), so no firmware logic changes. Three 6 × 6 mm buttons use the dial's three GPIOs (2, 45, 14); the shutter stays on GPIO 48.

The keychain sits at the top-left corner, opposite the shutter, so the camera hangs clear of your hand and the strap never crosses the screen.

## What each fitting does

- **Charge + boost module (your 18 × 14 × 4 mm board).** It does three jobs:
  1. **Charges the LiPo** from USB-C with the correct lithium profile (constant current, then constant voltage to 4.2 V), and stops when full.
  2. **Protects the cell:** stops charging on over-voltage and cuts the output at low voltage (about 3.0 V), so the battery is never drained to damage. Your battery's own protection board is a second safety layer.
  3. **Boosts** the cell's 3.0–4.2 V up to the steady 5 V the camera board needs.

  Its USB-C becomes the camera's charging port, and its LEDs show charging/full.
- **Viewfinder:** a straight tube through the body with a window in each plate, the Novo's see-through viewfinder. Its trim rings are separate parts, so you can print swap-in frames in other colours.
- **Lens ring:** accent-colour ring glued around the lens hole.

## Printing (Bambu Studio)

| Part | File | Material | Notes |
| --- | --- | --- | --- |
| Frame | `frame.stl` | PETG (or PLA for test fits) in the accent colour | Back face down. Tree supports for the button shelf, camera cradle and shutter ledge. 0.12–0.16 mm layers |
| Back plate | `back_plate.stl` | Clear PETG, or acrylic via DXF | Flat. 100% infill, ironing |
| Front plate | `front_plate.stl` | Same | Same |
| Button and shutter caps | `caps.stl` | Accent colour | Flange down |
| Lens ring | `lens_ring.stl` | Accent colour | |
| Viewfinder trims | `vf_frames.stl` | Accent colour, or several | |

Export any part again after editing the model:

```bash
openscad -D 'part="frame"' -o export/frame.stl instantsnap_body.scad
```

## Assembly order

1. Press the 8 heat-set inserts into the frame's corner posts (both faces).
2. Seat the buttons in the shelf, the shutter switch on its ledge, and the slide switch in its pocket. Solder their wires.
3. Fit the camera module into the cradle and run its ribbon (through the extension) down to the board.
4. Lay in the board, then the screen on top of it, then the charge module. Wire the power chain (`02-build-plan.md`, Connections and charging).
5. Battery in last, with a foam pad, plugged into the charge module.
6. Drop the button caps into the back plate, screw on the back plate, then the front plate. Glue on the lens ring and viewfinder trims.

## Editing in Fusion

**The main editable model is a parametric Fusion design** (built 9 Oct), driven by 78 named parameters. Change a number in **Modify → Change Parameters** and the whole camera rebuilds. Tested: widening the body to 124 mm and lengthening the battery rebuilt every part with no timeline errors, and restoring the values gave identical volumes.

- **Open it:** it's open in Fusion now as "Untitled". **Save it to your Fusion cloud (Cmd+S)**; it isn't saved there yet. A local archive copy is `cad/fusion/camera1_parametric.f3d` (File → Open → Open from my computer).
- **Structure:** one component per part, each in its own timeline group:

  | Component | Contents |
  | --- | --- |
  | Frame | Wall, screw posts with insert holes, keychain corner, viewfinder tube, camera cradle, button shelf, shutter ledge, power switch housing, battery divider, all openings |
  | Back plate (clear) | Screen window, button holes, viewfinder window, screw and keychain holes |
  | Front plate (clear) | Lens hole, viewfinder window, screw and keychain holes |
  | Lens ring, Viewfinder trims, Button caps, Shutter cap | Accent parts |
  | Reference electronics (not printed) | Battery, board, 2.0 in screen module and glass, charge module, camera, switches: for checking fit |

- **The parameters you'll touch most:**

  | Parameter | Controls |
  | --- | --- |
  | `body_W`, `body_H`, `corner_R` | Overall size and corner roundness |
  | `cav`, `plate_t`, `wall` | Thickness: inside depth, plate thickness, wall |
  | `scr_W`, `scr_H`, `scr_aW`, `scr_aH`, `scr_x`, `scr_y` | Screen module and its window; set from the module you buy |
  | `board_L`, `board_W`, `board_T`, `usb_y` | Board size and USB opening; set from calipers |
  | `lens_x`, `lens_y`, `vf_x`, `vf_y` | Lens and viewfinder positions |
  | `btn_x1`, `btn_pitch`, `btn_y` | Back button positions |
  | `shutter_x`, `sw_x` | Shutter and power switch positions on the top edge |
  | `key_d`, `key_inset` | Keychain hole |

- **Rebuilding from scratch:** the model is generated by the scripts in `cad/fusion/` (`stage1_params_frame.py`, `stage2_parts.py`, `stage3_appearance.py`, with shared `_helpers.py`). In a new, empty design, run them in order; ask me to, or run them from Fusion's Scripts and Add-Ins. Stage 3 alone re-applies the colourway.

**Other formats**

- `cad/export/camera1_assembly.step`: the same camera as plain solids (no history), from `cad/export_step.py`, for any CAD tool.
- `cad/instantsnap_body.scad`: the OpenSCAD original, still used for STL/DXF exports and quick renders.

These three are separate copies of the design. **From now on, treat Fusion as the main model**: make changes there and export STL/STEP from Fusion. The OpenSCAD and CadQuery files keep the 9 Oct state for reference.

## Measurements needed before printing for real

The model is parametric: change the numbers at the top of the `.scad` file and everything moves. These are estimates right now:

| Part | What to measure | Parameter |
| --- | --- | --- |
| GOOUUU board | Length × width; tallest point with pin headers removed (top and bottom); where each USB-C port, the SD slot and the camera connector sit along the edges | `board`, `board_usb_y` |
| Screen module | Board size, glass (active area) size and its offset from the board edge, thickness. **Decide the screen first** (below) | `scr`, `scr_active` |
| Camera module | Body size and thickness with lens; lens barrel diameter | `cam`, `lens_d` |
| Charge module | Which edge the USB-C is on; where the LEDs are | `chg` |
| Slide switch | Body size, knob width, knob travel | `sw`, `sw_knob`, `sw_travel` |
| Battery | Real thickness (often 10.2–10.5 mm), and the length including its protection board and wires | `bat` |

**Screen: 2.0 in, 320 × 240 IPS (ST7789), chosen 8 Oct.** Every photo, video and live-view frame the camera makes is 4:3, and this is the only size that fits the body and is 4:3 too:
- the photo fills the screen with no black bars;
- the 320 × 240 live view lands pixel-for-pixel, with no scaling;
- it's the same 320 × 240 resolution as camera 2's 2.8 in, so both cameras share one screen layout.

To-scale comparison: `cad/renders/screen-compare.png`, regenerated by `cad/screen_compare.py`.

| Screen | Pixels | Photo shown | Fills screen | Fits body |
| --- | --- | --- | --- | --- |
| 1.14 in | 240 × 135 | 19.8 × 14.9 mm | 80% (bars left/right) | Yes |
| 1.69 in | 280 × 240 | 32.6 × 24.5 mm | 87% (bars top/bottom) | Yes |
| **2.0 in** | **320 × 240** | **40.8 × 30.6 mm** | **100%** | **Yes** |
| 2.4 in | 320 × 240 | 49.0 × 36.7 mm | 100% | No: hits the viewfinder and buttons |

The model's `screen` setting switches between the first three. Sizes are Waveshare's published module dimensions; measure the module you buy, because the glass sits off-centre on many 2.0 in boards.

## Known compromises in v1

- **Footprint is about 10% larger than the Novo.** That's the price of a 67 mm dev board and a 2000 mAh cell side by side. A thinner, smaller cell (such as a 1000 mAh 603450) would buy about 3 mm of thickness, not footprint.
- **The frame needs supports.** The internal shelves sit mid-height; the cost is supports on the inside, invisible once closed.
- **The power switch knob is short** (1.5 mm). It sits in a finger recess so a fingernail reaches it. A switch with a 3 mm knob (G3) would be easier to flick.
