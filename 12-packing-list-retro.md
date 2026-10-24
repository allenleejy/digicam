# Packing list: building camera 1 (retro body) from scratch

24 Oct 2026. Everything needed to build the retro camera from loose parts, following `11-assembly-guide-v3.md`. Pack it all before you move. Quantities are what the build uses; take spares where it says so.

## Before you leave (can't be done at the new place)

- [ ] **Measure the estimated parts first**, if you'll be away from the printer. The case still uses estimates for these. Calipers and 10 minutes; send me the numbers if any differ:
  - screen module thickness;
  - screen glass position on the module;
  - screen corner-hole insets;
  - lens position on the board;
  - board and charge-module USB-C positions;
  - the switch body height (base to metal top, 3.5 mm assumed);
  - switch foot span.
- [ ] **Print the three plates** in `cad/export/v3/retro/camera1_retro_plates.3mf`, about 3 h 12 min in all:
  - black: 1 h 29 min;
  - light: 31 min;
  - brown coverings: 1 h 12 min.
- [ ] **Print spares of the tiny parts** (on the black and light plates; right-click → Add instance): one more set of button caps and shutter, and one more switch cap. They're the easiest parts to lose.
- [ ] **Firmware:** it still drives the old 1.14 in screen (240 × 135). Plan to port it to the 2.0 in (320 × 240) before the gift. Everything else works with the current firmware.
- [ ] **Laptop ready to work offline:** the repo, ESP-IDF 5.5.4 (`source ~/Development/esp-idf-v5.5.4/export.sh`), one test build of `firmware/` so its downloaded components are cached, and Python for the `host/` simulator.
- [ ] **Battery:** if its plug is still on, tape over the plug. Carry it in its own bag, away from metal and tools.

## 1. Printed parts (20 pieces)

Keep the small ones in a labelled bag.

**Black plate**

- [ ] Frame
- [ ] Back plate
- [ ] Front plate
- [ ] Sled
- [ ] Button carrier
- [ ] Dial
- [ ] Accessory shoe
- [ ] Shutter collar
- [ ] Switch cap (tiny)
- [ ] 3 window inserts: front viewfinder, rangefinder, back eyepiece (thin)

**Light plate**

- [ ] Top plate
- [ ] Lens barrel
- [ ] 3 button caps (◀ ● ▶), and the shutter cap

**Brown plate**

- [ ] Front covering
- [ ] Back covering with the portrait

## 2. Electronics

- [ ] ESP32-S3 camera board (GOOUUU N16R8), with the OV3660 camera on its ribbon
- [ ] 2.0 in screen module (ST7789, 320 × 240, 8 pins)
- [ ] LiPo battery, 2000 mAh, 34 × 50 × 10 mm
- [ ] Charge/boost module with USB-C (IP5306-type)
- [ ] Slide switch (the small one, 8.6 × 3.6 mm)
- [ ] 4 tactile switches, 5 × 5 mm, flat SMD feet: 3 back buttons and the shutter. **Take 3 spares**
- [ ] 2 resistors, 100 kΩ (plus spares)
- [ ] microSD card, formatted
- [ ] Optional: 470 µF 6.3 V capacitor, only needed if the camera restarts when recording starts on battery

## 3. Screws and inserts

Take the whole M2 assortment kit and the bag of inserts, not just the count.

| Item | Qty | Where |
| --- | --- | --- |
| M2 heat-set inserts, 3.5 mm OD | **16** | 8 frame corner posts (both ends), 3 sled blocks, 2 under the frame's top wall, 3 in the lens barrel |
| M2 × 5 | **11** | 4 back plate, 4 front plate, 3 lens barrel (from inside the front plate) |
| M2 × 4 | **7** | 4 board to sled (**not × 5**: that reaches the screen's back), 3 sled to frame |
| M2 × 6 (or × 5) | **2** | button carrier to the back plate (self-tapping into plastic) |
| M2 × 14 countersunk (or × 12) | **2** | top plate, from the top. Not in the kit; buy them if you haven't |

## 4. Wire, tape and glue

- [ ] 30 AWG silicone wire, a light colour (white) and black: about 1 m each
- [ ] 26 AWG silicone wire, red and black: about 0.4 m each
- [ ] Heat-shrink, 1.5 mm and 3 mm
- [ ] Kapton tape
- [ ] Electrical tape (to cover the battery's cut leads at once)
- [ ] Double-sided foam tape, 1 mm (battery)
- [ ] Double-sided foam tape, 0.4–0.5 mm (camera onto the ESP32 module; the pad behind the screen)
- [ ] Thin double-sided tape, 3M 9448A or tissue tape (charge module, both coverings, window inserts if you prefer tape to CA)
- [ ] CA glue, gel
- [ ] Hot glue gun and a stick or two (shutter switch, slide switch)
- [ ] Leaded solder, 0.6–0.8 mm
- [ ] Flux pen
- [ ] Solder wick (also for taking off any pin headers still on the board, screen or charge module)
- [ ] Isopropyl alcohol and cotton buds, for cleaning off flux

## 5. Tools

- [ ] Soldering iron with a fine tip (1 mm conical or chisel), about 330 °C, with its stand and tip cleaner
- [ ] A spare or old tip for the heat-set inserts. **About 200 °C**: the prints are PLA, not PETG
- [ ] A flat piece of metal (steel ruler) to press inserts square
- [ ] Flush cutters
- [ ] Wire strippers for 26 and 30 AWG
- [ ] Fine tweezers
- [ ] Helping hands or Blu Tack
- [ ] Multimeter with probes
- [ ] M2 screwdriver: hex 1.3 mm or Phillips 00, whichever your screws need (take both if unsure)
- [ ] Heat gun or lighter, for heat-shrink
- [ ] Craft knife or deburring tool, for stringing and screw holes
- [ ] 6 mm and 7 mm drill bits, turned by hand to free a sticking cap
- [ ] Small file or 400-grit sandpaper
- [ ] Calipers (and a ruler, for cutting wires to length)
- [ ] Soft cloth to build on, and a heat-proof mat
- [ ] Small parts tray, or labelled bags
- [ ] Safety glasses; a fan or open window for solder fumes

## 6. Computer, cables, power

- [ ] Laptop and its charger
- [ ] USB-C **data** cable for the board (not a charge-only one)
- [ ] A second USB-C cable and a USB charger, to test the camera's charge port
- [ ] Your phone, to test Phone mode
- [ ] Power strip, if the new place is short of sockets

## 7. Guides (offline copies)

- [ ] `11-assembly-guide-v3.md` with its pictures in `guide/v3/`
- [ ] The wiring diagram `diagrams/wiring-v3.png`, and the retro wire routes and cut lengths `diagrams/routes-v3-retro.png`
- [ ] The interactive 3D guide needs the internet: https://claude.ai/artifact/BogmuenUrWaN1Fk98t3Pgy
