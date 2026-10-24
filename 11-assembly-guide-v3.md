# Assembly guide: v3 bodies (clear and retro)

**Interactive 3D version:** https://claude.ai/artifact/BogmuenUrWaN1Fk98t3Pgy (source `guide-v3/`; parts from `cad/guide_parts_v3.scad`, bundled by `cad/export_guide_v3.py`). Switch between the clear and retro bodies at its top left.

23 Oct 2026. This guide covers both camera bodies in `cad/camera1_v3.scad`, from printed parts and loose electronics to a finished camera. Where the two bodies differ, each step says so. The pictures show each step with the new parts in orange, clear body on the left and retro on the right.

**How the work is ordered:**
1. Solder every part together on the desk, with everything still loose.
2. Test it.
3. Fit the parts into the case one at a time, while they're still connected.

Fault-finding is easy on the desk and miserable inside a closed case, so don't skip the test.

**Time:**
- Printing: about 6–9 h (clear) or 9–12 h (retro).
- Soldering: about 2 h.
- Assembly: about 1 h.

**Before you start:** the case uses estimated measurements for the 2.0 in screen, where the lens sits on the board, the USB-C sockets and the slide switch. If you haven't sent those yet (list in `04-case-design.md`, v3), do it before the full print.

**Firmware:** it still drives the 1.14 in screen at 240 × 135, so the 2.0 in screen will show a small or misplaced picture until the firmware is updated for 320 × 240. Everything else in this guide works with the current firmware.

---

## 1. What you need

### Tools

| Tool | Notes |
| --- | --- |
| Soldering iron with a fine tip (1 mm conical or chisel), about 330 °C | |
| Spare or old tip for heat-set inserts, about 230 °C for PETG | A tip you don't mind wearing |
| Leaded solder 0.6–0.8 mm, flux pen, solder wick | Flux makes the small pads easy |
| Flush cutters, wire strippers for 30 and 26 AWG, tweezers | |
| Helping hands or Blu Tack | Holding parts while you solder |
| Multimeter | Polarity and continuity checks |
| Small screwdriver for M2 (hex 1.3 mm or Phillips 00) | |
| Lighter or heat gun | For heat-shrink |

### Materials

| Item | Amount |
| --- | --- |
| 30 AWG silicone wire, a light colour (white) and black | about 1 m each |
| 26 AWG silicone wire, red and black | about 0.4 m each |
| Heat-shrink, 1.5 mm and 3 mm | a few cm of each |
| Two 100 kΩ resistors | |
| Kapton tape | holds wire bundles flat |
| Foam tape: 1 mm (battery) and 0.4–0.5 mm (camera, screen pad) | |
| Thin double-sided tape (3M 9448A or tissue tape) | charge module, clear-body panel and bezel, retro coverings |
| CA glue (gel) and, optionally, a hot glue gun | small fixings, listed in section 6 |

### Screws and inserts

| | Clear | Retro |
| --- | --- | --- |
| M2 heat-set inserts, 3.5 mm OD | 11 | 16 |
| M2 × 5 (back and front plates, board to sled) | 12 | 12 |
| M2 × 5 (lens barrel, from inside the front plate) | | 3 |
| M2 × 4 (sled to frame) | 3 | 3 |
| M2 × 6 or × 5 (button carrier to the back plate) | 2 | 2 |
| M2 × 14 countersunk, or × 12 (top plate, from the top) | | 2 |

### Printed parts

All are in `cad/export/v3/<body>/`, already the right way up for printing: 0.2 mm layers, 3 walls, no supports. Colours are suggestions.

| Part | Clear | Retro |
| --- | --- | --- |
| Frame | translucent mint PETG | black |
| Back plate, front plate | translucent mint or clear PETG | black |
| Sled, button carrier | white or orange | any |
| Caps (3 buttons, shutter, switch cap) | orange, the switch cap grey | silver; switch cap black |
| Lens ring | orange | |
| Battery panel, screen bezel | frosted white or mint | |
| Top plate | | silver or cream |
| Dial, accessory shoe, shutter collar (`top_bits`) | | black |
| Lens barrel | | silver |
| Front and back coverings | | brown or tan |
| Window inserts | | black |

**After printing:**
- Clear any stringing from the screw holes.
- Push each cap into its hole and sleeve. It should slide freely; if one sticks, turn a 6 or 7 mm drill bit in the hole by hand.

---

## 2. Heat-set inserts

![Step 1](guide/v3/step01.png)

1. Set the spare tip to about 230 °C.
2. Put each insert on its hole and let it sink in under its own weight plus a light push, straight down, until its top is flush.
3. While it's still soft, a flat piece of metal pressed on top keeps it square.

**Where they go:**
- **Frame corner posts:** 8, one in each end of the 4 posts.
- **Sled blocks:** 3, from the back face.
- **Retro only:**
  - 2 in the bosses under the frame's top wall (from the top);
  - 3 in the back of the lens barrel.

---

## 3. Wiring plan

![Wiring, part pin to board pad](diagrams/wiring-v3.png)

**Read this first:**
- **Board pads:** every wire ends on a board pad marked with its label (5V, GND, 3V3, EN, or a GPIO number). Board layouts vary between batches, so **find each pad by its printed label**. The diagram's order is only for clarity.
- **Wire colours:**
  - red: power +;
  - black: GND;
  - one light colour: all signals.
- **Wire gauge:** 26 AWG for the power path (battery → charge module → switch → 5V, and OUT− → GND). 30 AWG for everything else.
- **Headers:** if the board, screen or charge module still have pin headers, take them off first (`05-bench-bring-up.md`, "The pre-soldered headers").
  - Through-hole pads: push the wire through from the side away from the iron and solder on top. That's much stronger than a wire laid on the pad.

### Where each wire runs, and how long to cut it

These diagrams use the case's real positions. The lengths include 10 mm of slack, plus 25 mm for running along the board to the right pad. **Cut a little long; you can always trim.**

![Clear routes](diagrams/routes-v3-clear.png)

![Retro routes](diagrams/routes-v3-retro.png)

| Wire | Clear | Retro |
| --- | --- | --- |
| Battery + and − to the charge module's B+ and B− (the battery's own leads, trimmed) | 85 mm | 85 mm |
| Module OUT+ to the switch's middle leg (26 AWG red) | 90 mm | 70 mm |
| Switch end leg to board 5V (26 AWG red) | 70 mm | 90 mm |
| Module OUT− to board GND (26 AWG black) | 115 mm | 115 mm |
| Module B+ to the resistor at GPIO 1 (30 AWG red) | 120 mm | 120 mm |
| Shutter: 2 wires to GPIO 48 and the button GND chain | 100 mm | 60 mm |
| Buttons: 3 signals plus 1 GND, from the carrier | 40 mm each | 40 mm each |
| Screen: 8 wires, through the sled's slot | 50 mm each | 50 mm each |

---

## 4. Solder everything together on the desk

Do these in order. **Unplug USB and leave the battery disconnected** whenever you solder.

### 4.1 Battery to the charge module

Your battery's plug and the pigtail you bought are both male, so the plug comes off and the battery leads go straight to the charge module. This is normal.

1. **Cut one lead only**, close to the plug. Wrap its cut end in tape at once.
2. Cut the other lead, and tape it too. The two bare ends must never touch: even a protected LiPo can heat up fast if shorted.
3. Trim both leads to the length in section 3.
4. Slide 3 mm heat-shrink onto each lead first, strip 2 mm, and tin it.
5. Solder **red to B+** and **black to B−** on the charge module.
6. Shrink the sleeves over the joints.
7. **Check:** with the meter on DC volts, B+ to B− reads 3.6–4.2 V with + on B+.

### 4.2 Charge module to switch to board

1. **OUT+ to the switch:** 26 AWG red, from the module's OUT+ to the slide switch's **middle** leg.
2. **Switch to the board:** 26 AWG red, from **one end** leg of the switch to the board's **5V** pad. The other end leg stays empty.
3. **Ground:** 26 AWG black, from the module's OUT− to a **GND** pad on the board.
4. **Insulate:** heat-shrink every joint, and fold the switch's empty leg flat so it can't touch anything.
5. **Switch cap:** glue it onto the switch's knob now with a small drop of CA, keeping glue out of the switch. It's much harder to do later.

### 4.3 Battery-level resistors

1. Solder one 100 kΩ resistor to the board's **GPIO 1** pad and the other between **GPIO 1** and a **GND** pad, keeping the leads short.
2. Run 30 AWG red from the module's **B+** to the free end of the first resistor.
3. Sleeve both resistors in heat-shrink so no bare lead can touch the board.

### 4.4 Screen (8 wires)

1. Cut eight 50 mm lengths of 30 AWG: red for VCC, black for GND, the light colour for the rest.
2. Solder them to the screen's pads: VCC, GND, SCL, SDA, RES, DC, CS, BLK.
3. **Don't solder the board end yet.** These wires go through the sled's slot first (section 5, step 6).
4. Twist the bundle loosely.

### 4.5 Buttons and shutter (the flat-foot switches)

Each 5 × 5 switch has four feet. Feet on the same long side are already joined inside, so **use two diagonal feet**: then the pair is always switched, whichever way round the switch sits.

1. Tin the two diagonal feet you'll use on each switch.
2. **Threading:**
   - **Back buttons:** thread each switch's two wires through the slots in the button carrier's floor first (the slots beside each pocket). Then solder them to the feet.
   - **Clear shutter:** thread its two wires through the slot in the shutter cradle's floor.
   - **Retro shutter:** thread them through the hole in the frame's top wall under the top plate.
3. **Signal wires:** left button to **GPIO 2**, OK to **GPIO 14**, right to **GPIO 45**, shutter to **GPIO 48**.
4. **GND chain:** link the other foot of all four switches with short black jumpers, then run one black wire to a **GND** pad.
5. Sleeve the joints, or put a dab of hot glue over them once tested.

### 4.6 Test on the desk

Leave the screen's board end unsoldered for now, but plug the board into the laptop with the simulator running (`host/`).

1. **Board power:** the board starts, and the simulator shows "Camera connected".
2. **Buttons:** press each one. The camera log shows `button: left`, `button: ok`, `button: right` and `button: shutter`. Holding ● shows `button: hold`.
3. **Battery level:** the simulator's status shows a battery reading instead of "USB" once the battery is connected.
4. **Battery power:** unplug the laptop and slide the switch on. The board starts from the battery.
5. Switch off. Any fault is far easier to fix now.

---

## 5. Build the camera

The electronics are now one connected bundle. Each step below puts one part of it into the case. Lay the back plate face down on a soft cloth and build upward.

### Step 2: buttons into the back plate

![Step 2](guide/v3/step02.png)

1. **Caps:** from the inside, drop the three button caps into their sleeves.
   - The ◀ and ▶ caps have a small key rib. Line it up with the slot in the sleeve; then the arrows can't turn.
   - The caps' flanges rest on the sleeves.
2. **Switches:** push each switch into its pocket in the carrier, **plunger toward the caps**, with its wires through the slots, until the two fingers click over its metal top.
   - The carrier, switches and wires are now one piece: solder and test them before fitting.
   - If the fingers don't catch (your switch body is shorter than 3.5 mm), a drop of CA on two corners holds it.
3. **Carrier:** lay it on the two posts and fit the 2 screws, M2 × 6 or × 5, straight into the plastic. Stop when snug.
4. **Check:** press each cap from the outside. Each should click once, with no wobble.

### Step 3: screen into its lip

![Step 3](guide/v3/step03.png)

1. Peel the screen's protective film.
2. Lay the screen **glass down** in the lip around the window, with its pin end toward the board's slot side (the right in the step picture).
3. Stick a 0.5 mm foam pad to the back of the screen. When the case closes, the sled presses on this pad and holds the screen in place, so no screws or glue are needed.

### Step 4: frame onto the back plate

![Step 4](guide/v3/step04.png)

1. Lower the frame onto the back plate.
   - **Clear:** the plate sits inside the rim's recess.
2. Turn it over, holding it together, and fit the 4 corner screws (M2 × 5) through the back plate into the frame's inserts.

### Step 5: battery, charge module and top controls

![Step 5](guide/v3/step05.png)

1. **Battery:** two strips of 1 mm foam tape on its back, pressed into its bay. Its leads go to the top-right corner of the bay, where there's a gap above the battery.
2. **Charge module:**
   1. Thin double-sided tape on its back.
   2. Before pressing it down, slide it against the side wall so its USB-C lines up with the hole. **Test with a cable** that the plug goes fully in.
   3. Press it onto the battery's front face.
3. **Clear only:**
   - **Shutter switch:** into its cradle under the top wall, plunger up, wires down through the floor slot. One dot of hot glue on the cradle floor keeps it from rocking.
   - **Slide switch:** in from the front, then slide it up into its pocket so the switch cap pokes through the slot in the top wall. A dot of hot glue on its side holds it.
4. **Wires:** lay the power wires along the lane above the battery, and hold them with a strip of Kapton.

### Step 6: board, camera and sled

![Step 6](guide/v3/step06.png)

1. **Screen wires:** thread all 8 up through the sled's slot at the screen's pin end.
2. **Board:** set it on the sled's four spacers, component side up.
3. **Solder the screen wires:** feed each one up through its board pad from underneath (by label: VCC→3V3, GND→GND, SCL→21, SDA→47, RES→EN, DC→41, CS→42, BLK→46) and solder on top.
4. **Screw the board down:** 4 screws (M2 × 5) into the spacers.
5. **Camera:**
   1. Stick a square of 0.4 mm foam tape on the ESP32 module's metal shield.
   2. Fold the camera over onto it, **lens facing up, away from the board**.
   3. Bend the ribbon in a gentle curve, and don't crease it.
   4. Press the camera down so it sits square.

### Step 7: sled into the frame

![Step 7](guide/v3/step07.png)

1. Lower the sled onto its three blocks. The screen's wires fold under it, and its foam pad lands on the screen.
2. Fit 3 screws (M2 × 4).
3. Tuck the button and shutter wires along the board's edge and tape them flat.
4. **Power test, front still open:**
   1. Switch on. The camera starts.
   2. The screen lights up.
   3. Every button works.
   4. A USB-C cable in the charge port lights the charge LED.
   5. Switch off.

### Step 8 (retro only): top plate

![Step 8](guide/v3/step08.png)

1. **Shutter switch:** into the socket on top of the frame, plunger up, wires down through the hole.
2. **Slide switch:** press it up into its pocket under the top plate's ceiling, so the switch cap comes through the slot. A dot of hot glue holds it.
3. **Shutter cap:** drop it into its hole from inside the top plate, flange down.
4. **Fit the top plate:** set it on the frame and fit 2 countersunk screws from the top. They show, as they do on old cameras.
5. **Decorations:** glue the dial, accessory shoe and shutter collar into their peg holes with CA.
6. **Window inserts:** glue them into the window recesses: two on the front, the eyepiece on the back.

### Step 9: front plate

![Step 9](guide/v3/step09.png)

1. **Lens surround:**
   - **Retro:** first screw the lens barrel to the front plate from inside, with 3 screws (M2 × 5) into the barrel's inserts.
   - **Clear:** glue the lens ring around the lens hole, with a thin ring of CA or a ring of double-sided tape.
2. **Close the case:**
   1. Lower the front plate.
   2. The square collar on its inside slides over the camera and centres the lens in the hole. **Look through the hole as it closes.** If the collar lands on the camera's edge, lift and nudge the camera.
   3. Fit the 4 corner screws (M2 × 5).

### Step 10: finishing

![Step 10](guide/v3/step10.png)

- **Clear:**
  1. Press the battery panel into its opening in the back plate from outside, onto the ledge, with 4 dots of CA or thin tape.
  2. Stick the screen bezel onto the back with thin double-sided tape, so the caps come through its holes and the screen shows through its window.
- **Retro:** stick on the front and back coverings with thin double-sided tape. The front one drops into the rim. They're cut around the screws, so the camera still opens.
  - One-colour print: before fitting the back covering, fill the sunk portrait with dark acrylic paint and wipe the face clean with a damp cloth.

---

## 6. What holds each part

| Part | Held by | Glue? |
| --- | --- | --- |
| Screen | The back plate's lip, and the sled pressing on its foam pad | No |
| Board | 4 screws into the sled's spacers | No |
| Camera | 0.4 mm foam tape on the ESP32 module, and the front plate's collar | No glue near the lens or ribbon |
| Battery | 1 mm foam tape on the back plate, the bay ribs, and the charge module in front | No |
| Charge module | Thin tape on the battery, the front plate's rim, the side wall at the USB-C | No |
| Back buttons | Pockets in the carrier, carrier screwed to the back plate | A drop of CA only if loose |
| Shutter switch | Cradle (clear) or socket (retro), held in by its cap | 1 dot of hot glue |
| Slide switch | Its pocket | 1 dot of hot glue; cap CA'd onto the knob |
| Button caps | Their sleeves, held in by their flanges | No |
| Wires | Kapton tape along the lanes; heat-shrink on every joint | Optional hot glue over the board joints, as strain relief, after testing |
| Lens ring (clear) | | CA or double-sided tape |
| Panel and bezel (clear) | | CA dots or thin tape |
| Coverings, dial, shoe, collar, window inserts (retro) | | Thin tape (coverings), CA (the rest) |

**Glue on PETG:** CA grips PETG but stays a little brittle, which is fine for small trim. **Never put glue on the screen glass, the lens or the camera ribbon.**

---

## 7. Final checks

- [ ] Every button clicks once and does its job; holding ● opens the mode dial.
- [ ] Photo: the shutter takes one, and it shows in the gallery.
- [ ] Phone mode starts, and the phone page works.
- [ ] Charging: the charge LED lights with a cable in, and the camera stays on while charging.
- [ ] Nothing rattles when you shake it gently. If something does, it's usually a cap (normal, slight) or a loose wire bundle (tape it).
- [ ] After 10 minutes of live view the case is warm, not hot.

## 8. If something's wrong

| Problem | Likely cause |
| --- | --- |
| Nothing at all with the switch on | Switch wired to the wrong end leg, or the battery flat: charge it 30 min |
| The camera restarts when recording starts | The module sags under load: add a 470 µF capacitor across 5V and GND at the board |
| Screen white or black | One of SCL, SDA, DC or CS loose. Recheck those four joints |
| Can't flash with the screen connected | The screen's BLK holds GPIO 46 high at boot: disconnect BLK while flashing |
| A button does nothing | Two feet on the same side were used instead of diagonal ones, or a GND chain wire is off |
| A button is always "pressed" | The switch was pushed too deep, or a foot touches GND: reseat it |
| Lens off-centre in its hole | The camera moved on its tape: open the front, press it into the collar |
| The front plate won't close flat | A wire bundle is over the camera or the charge module: re-tuck it |

## 9. Taking it apart

The steps in reverse:
1. Remove the coverings (retro) or the panel and bezel (clear) if needed.
2. Undo the front plate's 4 screws.
3. Undo the sled's 3 screws and lift out the stack.
4. Retro: undo the top plate's 2 screws.

Every electronic part comes out without cutting anything, because nothing on the electronics is glued except the switch dots.
