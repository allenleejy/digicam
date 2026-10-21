"""Export camera 1 as a STEP assembly for Fusion: every part a separate, named, coloured
solid body with real (not faceted) faces, plus reference bodies for the electronics.

    cd cad && .venv/bin/python export_step.py      ->  export/camera1_assembly.step
                                                       export/step_parts/*.step

This mirrors instantsnap_body.scad dimension for dimension. Change a number in one,
change it in the other (the parameters below use the same names).
"""

import os

import cadquery as cq

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "export")

# ---------------------------------------------------------------- parameters (same as the .scad)
W, H, R = 116.0, 66.0, 7.0
wall, plate_t, cav = 2.2, 1.6, 12.0
D = cav + 2 * plate_t
clr = 0.3

screws = [(5, 5), (W - 5, 5), (W - 5, H - 5), (17, H - 5)]
boss_r, insert_d, insert_len, screw_hole = 3.2, 3.2, 3.5, 2.4
key_c, key_d = (6, H - 6), 4.2

bat, bat_xy = (35, 52, 10.5), (9, 2.6)
chg = (18, 14, 4)
chg_xy = (W - wall - 18 - clr, 38.5)
btn, btn_body_h, btn_plunger_d = (6, 6, 5), 3.5, 3.5
sw, sw_knob, sw_travel = (8.6, 3.6, 3.6), (2.0, 1.5, 1.5), 2.0

board, board_xy, board_usb_y = (67, 28, 7.0), (46, 8.5), [22]
SCREENS = {
    "1.14": ((35, 32, 4.0), (24.91, 14.86)),
    "1.69": ((39, 31.5, 4.0), (32.63, 27.97)),
    "2.0": ((58, 35, 4.5), (40.80, 30.60)),
}
screen = "2.0"
scr, scr_active = SCREENS[screen]
scr_xy = (min(board_xy[0] + board[0] / 2 - scr[0] / 2, W - 5 - boss_r - 0.6 - scr[0]), 3.0)

cam, lens_d, lens_c = (8.5, 8.5, 5.5), 8.0, (72, 51)
vf, vf_xy = (16, 12), (48, 45)
btn_xs, btn_y = [84, 96, 108], 50
cap_d, cap_flange, cap_proud = 6.0, (8.4, 1.0), 1.2
shutter_x, shutter_cap_d, sw_x = 100, 8.0, 82

z_back_in = plate_t
z_front_in = plate_t + cav
btn_shelf_z = z_back_in + cap_flange[1] + btn[2]
shelf_t = 1.6


# ---------------------------------------------------------------- helpers
def box(x, y, z, w, h, d):
    return cq.Workplane("XY").box(w, h, d, centered=False).translate((x, y, z))


def rbox(x, y, z, w, h, d, r):
    """Box with rounded vertical edges."""
    b = box(x, y, z, w, h, d)
    return b.edges("|Z").fillet(min(r, w / 2 - 0.01, h / 2 - 0.01)) if r > 0 else b


def cyl(x, y, z, dia, h):
    return cq.Workplane("XY").circle(dia / 2).extrude(h).translate((x, y, z))


def outline(z, h):
    return rbox(0, 0, z, W, H, h, R)


# ---------------------------------------------------------------- frame (model coords: z 0..cav)
def frame():
    f = outline(0, cav).cut(rbox(wall, wall, -1, W - 2 * wall, H - 2 * wall, cav + 2, R - wall))
    for sx, sy in screws:
        f = f.union(cyl(sx, sy, 0, 2 * boss_r, cav))
    f = f.union(outline(0, cav).intersect(box(0, H - 11, -1, 11, 11, cav + 2)))          # keychain corner
    tube = rbox(vf_xy[0] - 1.2, vf_xy[1] - 1.2, 0, vf[0] + 2.4, vf[1] + 2.4, cav, 2.6) \
        .cut(rbox(vf_xy[0], vf_xy[1], -1, vf[0], vf[1], cav + 2, 1.6))
    f = f.union(tube)
    f = f.union(box(vf_xy[0] + vf[0] / 2 - 1, vf_xy[1] + vf[1], 0, 2, H - wall - vf_xy[1] - vf[1] + 0.5, cav))
    cz = cav - cam[2] - 1.2
    f = f.union(box(lens_c[0] - cam[0] / 2 - 1.6, lens_c[1] - cam[1] / 2 - 1.6, cz, cam[0] + 3.2, cam[1] + 3.2, cam[2] + 1.2))
    f = f.union(box(lens_c[0] - 1.5, lens_c[1] + cam[1] / 2, cz, 3, H - wall - lens_c[1] - cam[1] / 2 + 0.5, cam[2] + 1.2))
    f = f.union(box(btn_xs[0] - 5, btn_y - 6, btn_shelf_z - z_back_in, W - wall - btn_xs[0] + 5 + 0.5, 12, shelf_t))
    f = f.union(box(shutter_x - 5, H - wall - 1.0 - btn[2] - shelf_t, cav / 2 - 5, W - wall - shutter_x + 5 + 0.5, shelf_t, 10))
    f = f.union(box(sw_x - sw[0] / 2 - 1.2, H - wall - sw[1] - 1.2, cav / 2 - sw[2] / 2 - 1.2, sw[0] + 2.4, sw[1] + 1.7, sw[2] + 2.4))
    f = f.union(box(bat_xy[0] + bat[0] + clr, wall - 0.5, 0, 1.2, bat_xy[1] + bat[1] - wall + 0.5, cav))   # battery divider

    cuts = []
    for sx, sy in screws:
        cuts += [cyl(sx, sy, -1, insert_d, insert_len + 1), cyl(sx, sy, cav - insert_len, insert_d, insert_len + 1),
                 cyl(sx, sy, -1, 1.8, cav + 2)]
    cuts.append(cyl(key_c[0], key_c[1], -1, key_d, cav + 2))
    cuts.append(box(lens_c[0] - cam[0] / 2 - clr, lens_c[1] - cam[1] / 2 - clr, cav - cam[2] - clr,
                    cam[0] + 2 * clr, cam[1] + 2 * clr, cam[2] + clr + 1))
    cuts.append(box(lens_c[0] - 4, lens_c[1] - cam[1] / 2 - 2, cav - cam[2] - 2, 8, 3, cam[2] + 3))
    for x in btn_xs:
        cuts.append(box(x - btn[0] / 2 - clr, btn_y - btn[1] / 2 - clr, btn_shelf_z - z_back_in - btn_body_h,
                        btn[0] + 2 * clr, btn[1] + 2 * clr, btn_body_h + 0.01))
        cuts.append(box(x - 1.5, btn_y - 3.8, btn_shelf_z - z_back_in - 0.1, 3, 7.6, shelf_t + 0.2))
    cuts.append(cq.Workplane("XZ").circle((shutter_cap_d + 0.4) / 2).extrude(wall + 2)
                .translate((shutter_x, H + 1, cav / 2)))
    cuts.append(box(sw_x - sw[0] / 2 - clr, H - wall - sw[1] - clr, cav / 2 - sw[2] / 2 - clr,
                    sw[0] + 2 * clr, sw[1] + clr + 0.01, sw[2] + 2 * clr))
    cuts.append(box(sw_x - (sw_knob[0] + sw_travel) / 2 - 0.3, H - wall - 0.5, cav / 2 - sw_knob[2] / 2 - 0.3,
                    sw_knob[0] + sw_travel + 0.6, wall + 1, sw_knob[2] + 0.6))
    cuts.append(box(sw_x - 6, H - 1.2, cav / 2 - 3.5, 12, 2, 7))
    # USB-C openings in the right wall: charge module (upper) and the board's own port (lower)
    for yc, zc in [(chg_xy[1] + chg[1] / 2, cav - chg[2] / 2)] + [(y, cav - 3.2) for y in board_usb_y]:
        cuts.append(box(W - wall - 1, yc - 5, zc - 2.2, wall + 2, 10, 4.4).edges("|X").fillet(1.4))
    for c in cuts:
        f = f.cut(c)
    return f


# ---------------------------------------------------------------- clear plates, caps, trims
def plate(front):
    p = outline(0, plate_t)
    holes = [cyl(sx, sy, -1, screw_hole, plate_t + 2) for sx, sy in screws]
    holes.append(cyl(key_c[0], key_c[1], -1, key_d, plate_t + 2))
    holes.append(rbox(vf_xy[0], vf_xy[1], -1, vf[0], vf[1], plate_t + 2, 1.6))
    if front:
        holes.append(cyl(lens_c[0], lens_c[1], -1, lens_d + 0.8, plate_t + 2))
    else:
        holes.append(rbox(scr_xy[0] + (scr[0] - scr_active[0]) / 2 - 0.3, scr_xy[1] + (scr[1] - scr_active[1]) / 2 - 0.3,
                          -1, scr_active[0] + 0.6, scr_active[1] + 0.6, plate_t + 2, 0.8))
        holes += [cyl(x, btn_y, -1, cap_d + 0.4, plate_t + 2) for x in btn_xs]
    for h in holes:
        p = p.cut(h)
    return p


def button_cap():
    # shaft out through the back plate (z < plate_t), flange inside against it
    return cyl(0, 0, -cap_proud, cap_d, plate_t + cap_proud).union(cyl(0, 0, plate_t, cap_flange[0], cap_flange[1])) \
        .faces("<Z").edges().fillet(0.6)


def shutter_cap():
    c = cq.Workplane("XZ").circle(shutter_cap_d / 2).extrude(-(wall + 1.0)) \
        .union(cq.Workplane("XZ").circle(shutter_cap_d / 2 + 1.2).extrude(1.0))
    return c.faces(">Y").edges().fillet(2.0)


def lens_ring():
    ring = cq.Workplane("XY").add(cq.Solid.makeCone(9.5, 8.5, 2.4))
    return ring.cut(cyl(0, 0, -1, lens_d + 1.2, 5))


def vf_trim():
    return rbox(-2.4, -2.4, 0, vf[0] + 4.8, vf[1] + 4.8, 1.2, 4.0).cut(rbox(0, 0, -1, vf[0], vf[1], 3.2, 1.6))


# ---------------------------------------------------------------- reference electronics
def electronics():
    zb = z_back_in + (cav - bat[2]) / 2
    pcb_z = z_front_in - board[2]
    return {
        "REF battery 103450 2000mAh": (rbox(bat_xy[0], bat_xy[1], zb, *bat, 2), (0.88, 0.75, 0.38)),
        "REF GOOUUU ESP32-S3-CAM board": (box(board_xy[0], board_xy[1], pcb_z, board[0], board[1], 1.6)
                                          .union(box(board_xy[0] + 6, board_xy[1] + 4, pcb_z + 1.6, 25.5, 18, 3.1)),
                                          (0.16, 0.42, 0.29)),
        f"REF screen {screen} in module": (box(scr_xy[0], scr_xy[1], z_back_in, *scr), (0.11, 0.11, 0.16)),
        "REF charge-boost module": (box(chg_xy[0], chg_xy[1], z_front_in - chg[2], *chg), (0.36, 0.55, 0.94)),
        "REF OV3660 camera module": (box(lens_c[0] - cam[0] / 2, lens_c[1] - cam[1] / 2, z_front_in - cam[2], cam[0], cam[1], cam[2] - 3)
                                     .union(cyl(lens_c[0], lens_c[1], z_front_in - cam[2], lens_d, cam[2])), (0.2, 0.2, 0.2)),
        "REF back buttons x3": (box(btn_xs[0] - 3, btn_y - 3, btn_shelf_z - btn_body_h, 6, 6, btn_body_h)
                                .union(box(btn_xs[1] - 3, btn_y - 3, btn_shelf_z - btn_body_h, 6, 6, btn_body_h))
                                .union(box(btn_xs[2] - 3, btn_y - 3, btn_shelf_z - btn_body_h, 6, 6, btn_body_h)), (0.27, 0.27, 0.27)),
        "REF shutter switch": (box(shutter_x - 3, H - wall - 1.0 - btn[2], z_back_in + cav / 2 - 3, 6, btn_body_h, 6), (0.27, 0.27, 0.27)),
        "REF slide switch": (box(sw_x - sw[0] / 2, H - wall - sw[1], z_back_in + cav / 2 - sw[2] / 2, *sw), (0.53, 0.53, 0.53)),
    }


# ---------------------------------------------------------------- assemble and export
def main():
    os.makedirs(os.path.join(OUT, "step_parts"), exist_ok=True)
    lilac, orange = cq.Color(0.79, 0.72, 0.95), cq.Color(1.0, 0.54, 0.24)
    clear = cq.Color(0.75, 0.88, 1.0, 0.35)

    parts = {
        "Frame": (frame(), lilac, (0, 0, z_back_in)),
        "Back plate (clear)": (plate(False), clear, (0, 0, 0)),
        "Front plate (clear)": (plate(True), clear, (0, 0, z_front_in)),
        "Lens ring": (lens_ring(), orange, (lens_c[0], lens_c[1], D)),
        "Viewfinder trim front": (vf_trim(), orange, (vf_xy[0], vf_xy[1], D)),
        "Viewfinder trim back": (vf_trim(), orange, (vf_xy[0], vf_xy[1], -1.2)),
        "Shutter cap": (shutter_cap(), orange, (shutter_x, H - wall, z_back_in + cav / 2)),
    }
    for i, x in enumerate(btn_xs):
        parts[f"Button cap {['prev', 'ok', 'next'][i]}"] = (button_cap(), orange, (x, btn_y, 0))

    asm = cq.Assembly(name="Digicam camera 1 (InstantSnap-style) v1")
    for name, (shape, color, loc) in parts.items():
        asm.add(shape, name=name, color=color, loc=cq.Location(cq.Vector(*loc)))
        cq.exporters.export(shape, os.path.join(OUT, "step_parts", name.split(" (")[0].replace(" ", "_").lower() + ".step"))
    refs = cq.Assembly(name="Reference electronics (not printed)")
    for name, (shape, rgb) in electronics().items():
        refs.add(shape, name=name, color=cq.Color(*rgb))
    asm.add(refs, name="Reference electronics (not printed)")

    path = os.path.join(OUT, "camera1_assembly.step")
    asm.export(path)
    print(path)
    for name, (shape, _, _) in parts.items():
        print(f"  {name}: volume {shape.val().Volume():.0f} mm3, valid {shape.val().isValid()}")


if __name__ == "__main__":
    main()
