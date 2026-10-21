# Camera 1 case design: InstantSnap-style, clear sandwich

## v2 test-fit body (10 Oct 2026): print this one

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
