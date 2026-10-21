# Stage 1: user parameters + the Frame component.
# Same dimensions and names as cad/instantsnap_body.scad. Model space: X across the body seen
# from the BACK, Y up, Z from the back face (0) to the front face. Lies flat, as printed.

PARAMS = [
    # body
    ("body_W", "116 mm", "Body width, seen from the back"),
    ("body_H", "66 mm", "Body height"),
    ("corner_R", "7 mm", "Outer corner radius"),
    ("wall", "2.2 mm", "Frame wall thickness"),
    ("plate_t", "1.6 mm", "Clear plate thickness, front and back"),
    ("cav", "12 mm", "Inside depth = frame thickness"),
    ("clr", "0.3 mm", "Fit clearance"),
    # fixings
    ("screw_inset", "5 mm", "Corner screw centres from the edges"),
    ("screw4_x", "17 mm", "Top-left screw x, moved in for the keychain"),
    ("boss_r", "3.2 mm", "Screw post radius"),
    ("insert_d", "3.2 mm", "M2 heat-set insert hole"),
    ("insert_len", "3.5 mm", "Insert hole depth, each face"),
    ("pilot_d", "1.8 mm", "Hole through the posts"),
    ("screw_clear", "2.4 mm", "Screw clearance in the plates"),
    ("key_d", "4.2 mm", "Keychain hole"),
    ("key_inset", "6 mm", "Keychain hole centre from the top-left edges"),
    ("key_block", "11 mm", "Solid corner around the keychain hole"),
    # parts
    ("bat_L", "35 mm", "Battery width: 103450 plus allowance"),
    ("bat_H", "52 mm", "Battery length incl. protection board"),
    ("bat_T", "10.5 mm", "Battery thickness"),
    ("bat_x", "9 mm", "Battery left edge"),
    ("bat_y", "2.6 mm", "Battery bottom edge"),
    ("board_L", "67 mm", "GOOUUU board length (MEASURE)"),
    ("board_W", "28 mm", "GOOUUU board width (MEASURE)"),
    ("board_T", "7 mm", "Board height, headers removed (MEASURE)"),
    ("board_x", "46 mm", "Board left edge"),
    ("board_y", "8.5 mm", "Board bottom edge"),
    ("usb_y", "22 mm", "Board USB-C centre height on the right wall (MEASURE)"),
    ("scr_W", "58 mm", "2.0 in screen module width (MEASURE)"),
    ("scr_H", "35 mm", "Screen module height (MEASURE)"),
    ("scr_T", "4.5 mm", "Screen module thickness (MEASURE)"),
    ("scr_aW", "40.8 mm", "Screen active area width"),
    ("scr_aH", "30.6 mm", "Screen active area height"),
    ("scr_x", "body_W - screw_inset - boss_r - 0.6 mm - scr_W", "Screen module left edge, clear of the corner post"),
    ("scr_y", "3 mm", "Screen module bottom edge"),
    ("chg_L", "18 mm", "Charge module length"),
    ("chg_W", "14 mm", "Charge module width (USB-C edge)"),
    ("chg_T", "4 mm", "Charge module thickness"),
    ("chg_x", "body_W - wall - chg_L - clr", "Charge module left edge"),
    ("chg_y", "38.5 mm", "Charge module bottom edge"),
    ("cam_S", "8.5 mm", "Camera module size (MEASURE)"),
    ("cam_T", "5.5 mm", "Camera module thickness with lens (MEASURE)"),
    ("lens_d", "8 mm", "Lens barrel diameter (MEASURE)"),
    ("lens_x", "72 mm", "Lens centre x"),
    ("lens_y", "51 mm", "Lens centre y"),
    # controls
    ("vf_W", "16 mm", "Viewfinder window width"),
    ("vf_H", "12 mm", "Viewfinder window height"),
    ("vf_x", "48 mm", "Viewfinder left edge"),
    ("vf_y", "45 mm", "Viewfinder bottom edge"),
    ("vf_wall", "1.2 mm", "Viewfinder tube wall"),
    ("vf_r", "1.6 mm", "Viewfinder window corner radius"),
    ("btn_x1", "84 mm", "First back button centre (prev)"),
    ("btn_pitch", "12 mm", "Back button spacing"),
    ("btn_y", "50 mm", "Back button centre height"),
    ("btn_S", "6 mm", "Tactile switch body"),
    ("btn_T", "5 mm", "Tactile switch height incl. plunger"),
    ("btn_body_T", "3.5 mm", "Tactile switch body height"),
    ("cap_d", "6 mm", "Button cap shaft"),
    ("cap_flange_d", "8.4 mm", "Button cap flange"),
    ("cap_flange_t", "1 mm", "Button cap flange thickness"),
    ("cap_proud", "1.2 mm", "Cap height above the back plate"),
    ("shelf_t", "1.6 mm", "Internal shelves"),
    ("btn_shelf_z", "plate_t + cap_flange_t + btn_T", "Button shelf height"),
    ("shutter_x", "100 mm", "Shutter centre x"),
    ("shutter_cap_d", "8 mm", "Shutter cap diameter"),
    ("sw_x", "82 mm", "Power switch centre x"),
    ("sw_L", "8.6 mm", "Slide switch body length"),
    ("sw_W", "3.6 mm", "Slide switch body depth"),
    ("sw_T", "3.6 mm", "Slide switch body height"),
    ("sw_knob_W", "2 mm", "Slide switch knob width"),
    ("sw_knob_T", "1.5 mm", "Slide switch knob height"),
    ("sw_travel", "2 mm", "Slide switch knob travel"),
]


def make_params():
    up = design.userParameters
    for name, value, comment in PARAMS:
        p = up.itemByName(name)
        if p:
            p.expression = value
        else:
            up.add(name, VI(value), "mm", comment)


def screw_pts():
    return [("screw_inset", "screw_inset"), ("body_W - screw_inset", "screw_inset"),
            ("body_W - screw_inset", "body_H - screw_inset"), ("screw4_x", "body_H - screw_inset")]


def make_frame():
    start = design.timeline.count
    c = comp_named("Frame")
    Z0, TH = "plate_t", "cav"
    THRU0, THRU = "plate_t - 1 mm", "cav + 2 mm"
    MID = "plate_t + cav/2"

    box(c, "Outline", "0 mm", "0 mm", "body_W", "body_H", Z0, TH, OPS.NewBodyFeatureOperation)
    body = c.bRepBodies.item(0)
    body.name = "Frame"
    fillet_vertical(c, body, [("0 mm", "0 mm"), ("body_W", "0 mm"), ("0 mm", "body_H"), ("body_W", "body_H")],
                    "corner_R", "Outer corners")
    box(c, "Inner pocket", "wall", "wall", "body_W - 2*wall", "body_H - 2*wall", THRU0, THRU, OPS.CutFeatureOperation)
    fillet_vertical(c, body, [("wall", "wall"), ("body_W - wall", "wall"), ("wall", "body_H - wall"),
                              ("body_W - wall", "body_H - wall")], "corner_R - wall", "Inner corners")

    circles(c, "Screw posts", screw_pts(), "2*boss_r", Z0, TH, OPS.JoinFeatureOperation)
    box(c, "Keychain corner", "wall", "body_H - key_block", "key_block - wall", "key_block - wall", Z0, TH,
        OPS.JoinFeatureOperation)

    # viewfinder tube and its rib to the top wall
    box(c, "Viewfinder tube", "vf_x - vf_wall", "vf_y - vf_wall", "vf_W + 2*vf_wall", "vf_H + 2*vf_wall", Z0, TH,
        OPS.JoinFeatureOperation)
    box(c, "Viewfinder rib", "vf_x + vf_W/2 - 1 mm", "vf_y + vf_H", "2 mm", "body_H - wall - vf_y - vf_H + 0.5 mm",
        Z0, TH, OPS.JoinFeatureOperation)

    # camera cradle on the front side, hung from the top wall
    CZ, CH = "plate_t + cav - cam_T - 1.2 mm", "cam_T + 1.2 mm"
    box(c, "Camera cradle", "lens_x - cam_S/2 - 1.6 mm", "lens_y - cam_S/2 - 1.6 mm", "cam_S + 3.2 mm",
        "cam_S + 3.2 mm", CZ, CH, OPS.JoinFeatureOperation)
    box(c, "Camera cradle rib", "lens_x - 1.5 mm", "lens_y + cam_S/2", "3 mm",
        "body_H - wall - lens_y - cam_S/2 + 0.5 mm", CZ, CH, OPS.JoinFeatureOperation)

    # button shelf, shutter ledge, power switch pocket walls, battery ribs
    box(c, "Button shelf", "btn_x1 - 5 mm", "btn_y - 6 mm", "body_W - wall - btn_x1 + 5.5 mm", "12 mm",
        "btn_shelf_z", "shelf_t", OPS.JoinFeatureOperation)
    box(c, "Shutter ledge", "shutter_x - 5 mm", "body_H - wall - 1 mm - btn_T - shelf_t",
        "body_W - wall - shutter_x + 5.5 mm", "shelf_t", MID + " - 5 mm", "10 mm", OPS.JoinFeatureOperation)
    box(c, "Power switch housing", "sw_x - sw_L/2 - 1.2 mm", "body_H - wall - sw_W - 1.2 mm", "sw_L + 2.4 mm",
        "sw_W + 1.7 mm", MID + " - sw_T/2 - 1.2 mm", "sw_T + 2.4 mm", OPS.JoinFeatureOperation)
    # rib along the battery's inner side, rising from the bottom wall so it's part of the frame
    box(c, "Battery divider", "bat_x + bat_L + clr", "wall - 0.5 mm", "1.2 mm", "bat_y + bat_H - wall + 0.5 mm",
        Z0, TH, OPS.JoinFeatureOperation)

    # ---- cuts
    circles(c, "Insert holes back", screw_pts(), "insert_d", THRU0, "insert_len + 1 mm", OPS.CutFeatureOperation)
    circles(c, "Insert holes front", screw_pts(), "insert_d", "plate_t + cav - insert_len", "insert_len + 1 mm",
            OPS.CutFeatureOperation)
    circles(c, "Post pilot holes", screw_pts(), "pilot_d", THRU0, THRU, OPS.CutFeatureOperation)
    circles(c, "Keychain hole", [("key_inset", "body_H - key_inset")], "key_d", THRU0, THRU, OPS.CutFeatureOperation)
    box(c, "Viewfinder window", "vf_x", "vf_y", "vf_W", "vf_H", THRU0, THRU, OPS.CutFeatureOperation)
    box(c, "Camera pocket", "lens_x - cam_S/2 - clr", "lens_y - cam_S/2 - clr", "cam_S + 2*clr", "cam_S + 2*clr",
        "plate_t + cav - cam_T - clr", "cam_T + clr + 1 mm", OPS.CutFeatureOperation)
    box(c, "Camera flex slot", "lens_x - 4 mm", "lens_y - cam_S/2 - 2 mm", "8 mm", "3 mm",
        "plate_t + cav - cam_T - 2 mm", "cam_T + 3 mm", OPS.CutFeatureOperation)
    bx = ["btn_x1", "btn_x1 + btn_pitch", "btn_x1 + 2*btn_pitch"]
    boxes(c, "Button pockets", [(x + " - btn_S/2 - clr", "btn_y - btn_S/2 - clr", "btn_S + 2*clr", "btn_S + 2*clr")
                                for x in bx], "btn_shelf_z - btn_body_T", "btn_body_T + 0.01 mm", OPS.CutFeatureOperation)
    boxes(c, "Button leg slots", [(x + " - 1.5 mm", "btn_y - 3.8 mm", "3 mm", "7.6 mm") for x in bx],
          "btn_shelf_z - 0.1 mm", "shelf_t + 0.2 mm", OPS.CutFeatureOperation)
    box(c, "Power switch pocket", "sw_x - sw_L/2 - clr", "body_H - wall - sw_W - clr", "sw_L + 2*clr",
        "sw_W + clr + 0.01 mm", MID + " - sw_T/2 - clr", "sw_T + 2*clr", OPS.CutFeatureOperation)
    box(c, "Power switch knob slot", "sw_x - (sw_knob_W + sw_travel)/2 - 0.3 mm", "body_H - wall - 0.5 mm",
        "sw_knob_W + sw_travel + 0.6 mm", "wall + 1 mm", MID + " - sw_knob_T/2 - 0.3 mm", "sw_knob_T + 0.6 mm",
        OPS.CutFeatureOperation)
    box(c, "Power switch finger recess", "sw_x - 6 mm", "body_H - 1.2 mm", "12 mm", "2 mm", MID + " - 3.5 mm",
        "7 mm", OPS.CutFeatureOperation)
    box(c, "Charge port", "body_W - wall - 1 mm", "chg_y + chg_W/2 - 5 mm", "wall + 2 mm", "10 mm",
        "plate_t + cav - chg_T/2 - 2.2 mm", "4.4 mm", OPS.CutFeatureOperation)
    box(c, "Board USB port", "body_W - wall - 1 mm", "usb_y - 5 mm", "wall + 2 mm", "10 mm",
        "plate_t + cav - 3.2 mm - 2.2 mm", "4.4 mm", OPS.CutFeatureOperation)

    # shutter hole: a round hole through the top wall, sketched on a plane just inside it
    planes = c.constructionPlanes
    pinp = planes.createInput()
    pinp.setByOffset(c.xZConstructionPlane, VI("body_H - wall - 1 mm"))
    pl = planes.add(pinp)
    pl.name = "Shutter hole plane"
    if pl.geometry.origin.y < 0:          # XZ plane's normal points -Y in this file: offset the other way
        pl.deleteMe()
        pinp = planes.createInput()
        pinp.setByOffset(c.xZConstructionPlane, VI("-(body_H - wall - 1 mm)"))
        pl = planes.add(pinp)
        pl.name = "Shutter hole plane"
    sk = new_sketch(c, "Shutter hole", pl)
    ctr = sk.modelToSketchSpace(P3(ev("shutter_x"), ev("body_H - wall - 1 mm"), ev(MID)))
    circ = sk.sketchCurves.sketchCircles.addByCenterRadius(ctr, ev("shutter_cap_d + 0.4 mm") / 2)
    sk.sketchDimensions.addDiameterDimension(circ, P3(ctr.x + 1, ctr.y + 1, 0)).parameter.expression = "shutter_cap_d + 0.4 mm"
    o = sk.originPoint
    sk.sketchDimensions.addDistanceDimension(o, circ.centerSketchPoint, HOR, P3(ctr.x / 2, ctr.y + 0.5, 0)).parameter.expression = "shutter_x"
    sk.sketchDimensions.addDistanceDimension(o, circ.centerSketchPoint, VER, P3(ctr.x + 0.5, ctr.y / 2, 0)).parameter.expression = MID
    ext = c.features.extrudeFeatures
    coll = adsk.core.ObjectCollection.create()
    coll.add(sk.profiles.item(0))
    inp = ext.createInput(coll, OPS.CutFeatureOperation)
    normal_y = sk.xDirection.crossProduct(sk.yDirection).y
    direction = (adsk.fusion.ExtentDirections.PositiveExtentDirection if normal_y > 0
                 else adsk.fusion.ExtentDirections.NegativeExtentDirection)
    inp.setOneSideExtent(adsk.fusion.DistanceExtentDefinition.create(VI("wall + 2 mm")), direction)
    ext.add(inp).name = "Shutter hole"

    group_timeline(start, "Frame")
    return body


def main():
    design.designType = adsk.fusion.DesignTypes.ParametricDesignType
    # New Fusion documents are 'Part' designs (one component); Hybrid allows a component per part.
    if design.designIntent == adsk.fusion.DesignIntentTypes.PartDesignIntentType:
        design.designIntent = adsk.fusion.DesignIntentTypes.HybridDesignIntentType
    make_params()
    body = make_frame()
    props = body.physicalProperties
    print(f"Frame: volume {props.volume * 1000:.0f} mm3, bbox "
          f"{(body.boundingBox.maxPoint.x - body.boundingBox.minPoint.x) * 10:.1f} x "
          f"{(body.boundingBox.maxPoint.y - body.boundingBox.minPoint.y) * 10:.1f} x "
          f"{(body.boundingBox.maxPoint.z - body.boundingBox.minPoint.z) * 10:.1f} mm, "
          f"{design.timeline.count} timeline items, {design.userParameters.count} parameters")
