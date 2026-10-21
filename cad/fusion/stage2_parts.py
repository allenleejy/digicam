# Stage 2: clear plates, lens ring, viewfinder trims, caps, viewfinder fillets on the frame,
# and reference electronics. Needs stage 1 (parameters and Frame) in the same design.

EXTRA_PARAMS = [
    ("lens_ring_d", "19 mm", "Lens ring outer diameter"),
    ("lens_ring_h", "2.4 mm", "Lens ring height above the front plate"),
    ("vf_trim", "2.4 mm", "Viewfinder trim width around the window"),
    ("vf_trim_t", "1.2 mm", "Viewfinder trim thickness"),
    ("shutter_flange", "1.2 mm", "Shutter cap flange, beyond the shaft"),
    ("shutter_proud", "1 mm", "Shutter cap height above the top edge"),
]

SCREWS = [("screw_inset", "screw_inset"), ("body_W - screw_inset", "screw_inset"),
          ("body_W - screw_inset", "body_H - screw_inset"), ("screw4_x", "body_H - screw_inset")]
BTN_X = ["btn_x1", "btn_x1 + btn_pitch", "btn_x1 + 2*btn_pitch"]
VF_CORNERS = [("vf_x", "vf_y"), ("vf_x + vf_W", "vf_y"), ("vf_x", "vf_y + vf_H"), ("vf_x + vf_W", "vf_y + vf_H")]
FRONT_Z = "plate_t + cav"
THRU0, THRU = "-1 mm", "plate_t + 2 mm"


def outer_corners():
    return [("0 mm", "0 mm"), ("body_W", "0 mm"), ("0 mm", "body_H"), ("body_W", "body_H")]


def make_plate(name, z0, front):
    start = design.timeline.count
    c = comp_named(name)
    f = box(c, "Outline", "0 mm", "0 mm", "body_W", "body_H", z0, "plate_t", OPS.NewBodyFeatureOperation)
    body = f.bodies.item(0)
    body.name = name
    fillet_vertical(c, body, outer_corners(), "corner_R", "Corners")
    cz = f"{z0} - 1 mm"
    circles(c, "Screw holes", SCREWS, "screw_clear", cz, THRU, OPS.CutFeatureOperation)
    circles(c, "Keychain hole", [("key_inset", "body_H - key_inset")], "key_d", cz, THRU, OPS.CutFeatureOperation)
    box(c, "Viewfinder window", "vf_x", "vf_y", "vf_W", "vf_H", cz, THRU, OPS.CutFeatureOperation)
    fillet_vertical(c, body, VF_CORNERS, "vf_r", "Viewfinder window corners")
    if front:
        circles(c, "Lens hole", [("lens_x", "lens_y")], "lens_d + 0.8 mm", cz, THRU, OPS.CutFeatureOperation)
    else:
        box(c, "Screen window", "scr_x + (scr_W - scr_aW)/2 - 0.3 mm", "scr_y + (scr_H - scr_aH)/2 - 0.3 mm",
            "scr_aW + 0.6 mm", "scr_aH + 0.6 mm", cz, THRU, OPS.CutFeatureOperation)
        circles(c, "Button holes", [(x, "btn_y") for x in BTN_X], "cap_d + 0.4 mm", cz, THRU, OPS.CutFeatureOperation)
    group_timeline(start, name)
    return body


def frame_vf_fillets():
    start = design.timeline.count
    c = comp_named("Frame")
    body = c.bRepBodies.item(0)
    fillet_vertical(c, body, VF_CORNERS, "vf_r", "Viewfinder window corners")
    fillet_vertical(c, body, [("vf_x - vf_wall", "vf_y - vf_wall"), ("vf_x + vf_W + vf_wall", "vf_y - vf_wall")],
                    "vf_r + vf_wall", "Viewfinder tube corners")
    group_timeline(start, "Frame: viewfinder fillets")


def make_lens_ring():
    start = design.timeline.count
    c = comp_named("Lens ring")
    sk = new_sketch(c, "Lens ring")
    add_circle(sk, "lens_x", "lens_y", "lens_ring_d")
    add_circle(sk, "lens_x", "lens_y", "lens_d + 1.2 mm")
    f = extrude(c, sk, f"{FRONT_Z} + plate_t", "lens_ring_h", OPS.NewBodyFeatureOperation, "Lens ring",
                profiles=[ring_profile(sk)])
    f.bodies.item(0).name = "Lens ring"
    group_timeline(start, "Lens ring")


def make_vf_trims():
    start = design.timeline.count
    c = comp_named("Viewfinder trims")
    for label, z0 in (("front", f"{FRONT_Z} + plate_t"), ("back", "-vf_trim_t")):
        sk = new_sketch(c, f"Trim {label}")
        add_rect(sk, "vf_x - vf_trim", "vf_y - vf_trim", "vf_W + 2*vf_trim", "vf_H + 2*vf_trim")
        add_rect(sk, "vf_x", "vf_y", "vf_W", "vf_H")
        f = extrude(c, sk, z0, "vf_trim_t", OPS.NewBodyFeatureOperation, f"Trim {label}", profiles=[ring_profile(sk)])
        b = f.bodies.item(0)
        b.name = f"Viewfinder trim {label}"
        fillet_vertical(c, b, [("vf_x - vf_trim", "vf_y - vf_trim"), ("vf_x + vf_W + vf_trim", "vf_y - vf_trim"),
                               ("vf_x - vf_trim", "vf_y + vf_H + vf_trim"), ("vf_x + vf_W + vf_trim", "vf_y + vf_H + vf_trim")],
                        "vf_r + vf_trim", f"Trim {label} outer corners")
        fillet_vertical(c, b, VF_CORNERS, "vf_r", f"Trim {label} inner corners")
    group_timeline(start, "Viewfinder trims")


def make_button_caps():
    start = design.timeline.count
    c = comp_named("Button caps")
    for x, label in zip(BTN_X, ("prev", "ok", "next")):
        sk = new_sketch(c, f"Cap {label} shaft")
        add_circle(sk, x, "btn_y", "cap_d")
        f = extrude(c, sk, "-cap_proud", "plate_t + cap_proud", OPS.NewBodyFeatureOperation, f"Cap {label} shaft")
        b = f.bodies.item(0)
        b.name = f"Button cap {label}"
        sk = new_sketch(c, f"Cap {label} flange")
        add_circle(sk, x, "btn_y", "cap_flange_d")
        extrude(c, sk, "plate_t", "cap_flange_t", OPS.JoinFeatureOperation, f"Cap {label} flange", bodies=[b])
    group_timeline(start, "Button caps")


def make_shutter_cap():
    start = design.timeline.count
    c = comp_named("Shutter cap")
    up = adsk.core.Vector3D.create(0, 1, 0)
    mid = "plate_t + cav/2"
    sk = plane_sketch(c, "Shutter flange", c.xZConstructionPlane, "body_H - wall - 1 mm")
    p = P3(ev("shutter_x"), ev("body_H - wall - 1 mm"), ev(mid))
    plane_circle(sk, p, "shutter_cap_d + 2*shutter_flange", "shutter_x", mid)
    f = extrude_along(c, sk, [sk.profiles.item(0)], "1 mm", OPS.NewBodyFeatureOperation, "Shutter flange", up)
    b = f.bodies.item(0)
    b.name = "Shutter cap"
    sk = plane_sketch(c, "Shutter shaft", c.xZConstructionPlane, "body_H - wall")
    p = P3(ev("shutter_x"), ev("body_H - wall"), ev(mid))
    plane_circle(sk, p, "shutter_cap_d", "shutter_x", mid)
    extrude_along(c, sk, [sk.profiles.item(0)], "wall + shutter_proud", OPS.JoinFeatureOperation, "Shutter shaft", up,
                  bodies=[b])
    # round over the top edge: the circular edge highest in Y
    top = max((b.edges.item(i) for i in range(b.edges.count)),
              key=lambda e: e.boundingBox.minPoint.y + e.boundingBox.maxPoint.y)
    coll = adsk.core.ObjectCollection.create()
    coll.add(top)
    inp = c.features.filletFeatures.createInput()
    inp.edgeSetInputs.addConstantRadiusEdgeSet(coll, VI("shutter_cap_d/4"), False)
    c.features.filletFeatures.add(inp).name = "Domed top"
    group_timeline(start, "Shutter cap")


def make_reference():
    start = design.timeline.count
    c = comp_named("Reference electronics (not printed)")
    NB = OPS.NewBodyFeatureOperation

    def ref(name, x, y, w, h, z0, dz):
        f = box(c, name, x, y, w, h, z0, dz, NB)
        f.bodies.item(0).name = name
        return f.bodies.item(0)

    ref("Battery 103450 2000mAh", "bat_x", "bat_y", "bat_L", "bat_H", "plate_t + (cav - bat_T)/2", "bat_T")
    pcb = ref("GOOUUU ESP32-S3-CAM board", "board_x", "board_y", "board_L", "board_W", "plate_t + cav - board_T", "1.6 mm")
    sk = new_sketch(c, "ESP32-S3 module")
    add_rect(sk, "board_x + 6 mm", "board_y + 4 mm", "25.5 mm", "18 mm")
    extrude(c, sk, "plate_t + cav - board_T + 1.6 mm", "3.1 mm", OPS.JoinFeatureOperation, "ESP32-S3 module", bodies=[pcb])
    ref("Screen module 2.0in", "scr_x", "scr_y", "scr_W", "scr_H", "plate_t", "scr_T")
    ref("Screen active area", "scr_x + (scr_W - scr_aW)/2", "scr_y + (scr_H - scr_aH)/2", "scr_aW", "scr_aH",
        "plate_t - 0.2 mm", "0.2 mm")
    ref("Charge-boost module", "chg_x", "chg_y", "chg_L", "chg_W", "plate_t + cav - chg_T", "chg_T")
    cam = ref("OV3660 camera module", "lens_x - cam_S/2", "lens_y - cam_S/2", "cam_S", "cam_S",
              "plate_t + cav - cam_T", "cam_T - 3 mm")
    sk = new_sketch(c, "Lens barrel")
    add_circle(sk, "lens_x", "lens_y", "lens_d")
    extrude(c, sk, "plate_t + cav - cam_T", "cam_T", OPS.JoinFeatureOperation, "Lens barrel", bodies=[cam])
    for x, label in zip(BTN_X, ("prev", "ok", "next")):
        ref(f"Button switch {label}", f"{x} - btn_S/2", "btn_y - btn_S/2", "btn_S", "btn_S",
            "btn_shelf_z - btn_body_T", "btn_body_T")
    ref("Shutter switch", "shutter_x - btn_S/2", "body_H - wall - 1 mm - btn_T", "btn_S", "btn_body_T",
        "plate_t + cav/2 - btn_S/2", "btn_S")
    ref("Slide switch", "sw_x - sw_L/2", "body_H - wall - sw_W", "sw_L", "sw_W", "plate_t + cav/2 - sw_T/2", "sw_T")
    group_timeline(start, "Reference electronics")


def main():
    up = design.userParameters
    for name, value, comment in EXTRA_PARAMS:
        p = up.itemByName(name)
        if p:
            p.expression = value
        else:
            up.add(name, VI(value), "mm", comment)
    frame_vf_fillets()
    make_plate("Back plate (clear)", "0 mm", False)
    make_plate("Front plate (clear)", FRONT_Z, True)
    make_lens_ring()
    make_vf_trims()
    make_button_caps()
    make_shutter_cap()
    make_reference()
    root = design.rootComponent
    out = []
    for i in range(root.occurrences.count):
        comp = root.occurrences.item(i).component
        vol = sum(comp.bRepBodies.item(j).physicalProperties.volume for j in range(comp.bRepBodies.count)) * 1000
        out.append(f"{comp.name}: {comp.bRepBodies.count} bodies, {vol:.0f} mm3")
    print("\n".join(out))
    print(f"timeline {design.timeline.count} items, {up.count} parameters")
