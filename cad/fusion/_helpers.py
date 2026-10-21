# Shared helpers for the Fusion build scripts (pasted at the top of each stage script).
import adsk.core, adsk.fusion

app = adsk.core.Application.get()
design = adsk.fusion.Design.cast(app.activeProduct)
um = design.unitsManager
P3 = adsk.core.Point3D.create
VI = adsk.core.ValueInput.createByString
OPS = adsk.fusion.FeatureOperations
HOR = adsk.fusion.DimensionOrientations.HorizontalDimensionOrientation
VER = adsk.fusion.DimensionOrientations.VerticalDimensionOrientation


def ev(expr):
    """Expression in mm -> internal cm."""
    return um.evaluateExpression(expr, 'mm')


def comp_named(name):
    root = design.rootComponent
    for i in range(root.occurrences.count):
        if root.occurrences.item(i).component.name == name:
            return root.occurrences.item(i).component
    occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
    occ.component.name = name
    return occ.component


_placed = {}   # per sketch: {("x"|"y", expression): SketchPoint already dimensioned}


def _pos_dims(sk, pt, x, y, X, Y):
    """Fix a point's position from the sketch origin. A point sharing an x or y expression with one
    already placed is aligned to it instead of getting a second dimension: cleaner, and it avoids
    Fusion's solver rejecting duplicate dimensions as over-constrained."""
    dims, o, gc = sk.sketchDimensions, sk.originPoint, sk.geometricConstraints
    reg = _placed.setdefault(id(sk), {})
    if abs(X) < 1e-7 and abs(Y) < 1e-7:
        gc.addCoincident(pt, o)
        return
    if abs(X) < 1e-7:
        gc.addVerticalPoints(o, pt)
    elif ("x", x) in reg:
        gc.addVerticalPoints(reg[("x", x)], pt)
    else:
        dims.addDistanceDimension(o, pt, HOR, P3(X / 2, Y - 0.4, 0)).parameter.expression = x
        reg[("x", x)] = pt
    if abs(Y) < 1e-7:
        gc.addHorizontalPoints(o, pt)
    elif ("y", y) in reg:
        gc.addHorizontalPoints(reg[("y", y)], pt)
    else:
        dims.addDistanceDimension(o, pt, VER, P3(X - 0.4, Y / 2, 0)).parameter.expression = y
        reg[("y", y)] = pt


def add_rect(sk, x, y, w, h):
    X, Y, Wd, Hd = ev(x), ev(y), ev(w), ev(h)
    lines = sk.sketchCurves.sketchLines.addTwoPointRectangle(P3(X, Y, 0), P3(X + Wd, Y + Hd, 0))
    pts = {}
    for i in range(lines.count):
        for sp in (lines.item(i).startSketchPoint, lines.item(i).endSketchPoint):
            pts[(round(sp.geometry.x, 5), round(sp.geometry.y, 5))] = sp
    bl = pts[(round(X, 5), round(Y, 5))]
    br = pts[(round(X + Wd, 5), round(Y, 5))]
    tl = pts[(round(X, 5), round(Y + Hd, 5))]
    dims = sk.sketchDimensions
    dims.addDistanceDimension(bl, br, HOR, P3(X + Wd / 2, Y - 0.2, 0)).parameter.expression = w
    dims.addDistanceDimension(bl, tl, VER, P3(X - 0.2, Y + Hd / 2, 0)).parameter.expression = h
    _pos_dims(sk, bl, x, y, X, Y)


def add_circle(sk, cx, cy, d):
    CX, CY, Dd = ev(cx), ev(cy), ev(d)
    c = sk.sketchCurves.sketchCircles.addByCenterRadius(P3(CX, CY, 0), Dd / 2)
    sk.sketchDimensions.addDiameterDimension(c, P3(CX + Dd, CY + Dd, 0)).parameter.expression = d
    _pos_dims(sk, c.centerSketchPoint, cx, cy, CX, CY)
    return c


def new_sketch(comp, name, plane=None):
    sk = comp.sketches.add(plane or comp.xYConstructionPlane)
    sk.name = f"{name} sketch"   # sketch and feature can't share a name, or Fusion appends "(1)"
    sk.isComputeDeferred = False
    _placed[id(sk)] = {}   # Python reuses ids of freed objects: start this sketch's record fresh
    return sk


def extrude(comp, sk, start, dist, op, name, profiles=None, bodies=None):
    coll = adsk.core.ObjectCollection.create()
    for p in (profiles if profiles is not None else [sk.profiles.item(i) for i in range(sk.profiles.count)]):
        coll.add(p)
    ext = comp.features.extrudeFeatures
    inp = ext.createInput(coll, op)
    inp.startExtent = adsk.fusion.OffsetStartDefinition.create(VI(start))
    inp.setOneSideExtent(adsk.fusion.DistanceExtentDefinition.create(VI(dist)),
                         adsk.fusion.ExtentDirections.PositiveExtentDirection)
    if bodies:
        inp.participantBodies = bodies
    f = ext.add(inp)
    f.name = name
    return f


def ring_profile(sk):
    """The profile with a hole in it (between two nested shapes)."""
    for i in range(sk.profiles.count):
        if sk.profiles.item(i).profileLoops.count == 2:
            return sk.profiles.item(i)
    raise RuntimeError(f"{sk.name}: no ring profile")


def plane_sketch(comp, name, base, offset):
    """Sketch on a plane offset from one of the component's origin planes, oriented so the
    offset runs in the +X/+Y/+Z direction whatever the plane's own normal is."""
    planes = comp.constructionPlanes
    for sign in ("", "-"):
        inp = planes.createInput()
        inp.setByOffset(base, VI(f"{sign}({offset})"))
        pl = planes.add(inp)
        o = pl.geometry.origin
        if o.x + o.y + o.z >= -1e-9 or ev(offset) <= 0:
            pl.name = name + " plane"
            return new_sketch(comp, name, pl)
        pl.deleteMe()
    raise RuntimeError(f"{name}: could not place the plane")


def plane_circle(sk, model_pt, d, h_expr, v_expr):
    """Circle on a non-XY sketch at a model-space point, dimensioned from the sketch origin."""
    ctr = sk.modelToSketchSpace(model_pt)
    c = sk.sketchCurves.sketchCircles.addByCenterRadius(ctr, ev(d) / 2)
    dims, o = sk.sketchDimensions, sk.originPoint
    dims.addDiameterDimension(c, P3(ctr.x + 1, ctr.y + 1, 0)).parameter.expression = d
    dims.addDistanceDimension(o, c.centerSketchPoint, HOR, P3(ctr.x / 2, ctr.y + 0.5, 0)).parameter.expression = h_expr
    dims.addDistanceDimension(o, c.centerSketchPoint, VER, P3(ctr.x + 0.5, ctr.y / 2, 0)).parameter.expression = v_expr
    return c


def extrude_along(comp, sk, profiles, dist, op, name, model_dir, bodies=None):
    """Extrude a non-XY sketch by `dist` in the model direction `model_dir` (a Vector3D)."""
    coll = adsk.core.ObjectCollection.create()
    for p in profiles:
        coll.add(p)
    ext = comp.features.extrudeFeatures
    inp = ext.createInput(coll, op)
    n = sk.xDirection.crossProduct(sk.yDirection)
    d = (adsk.fusion.ExtentDirections.PositiveExtentDirection if n.dotProduct(model_dir) > 0
         else adsk.fusion.ExtentDirections.NegativeExtentDirection)
    inp.setOneSideExtent(adsk.fusion.DistanceExtentDefinition.create(VI(dist)), d)
    if bodies:
        inp.participantBodies = bodies
    f = ext.add(inp)
    f.name = name
    return f


def box(comp, name, x, y, w, h, z0, dz, op):
    sk = new_sketch(comp, name)
    add_rect(sk, x, y, w, h)
    return extrude(comp, sk, z0, dz, op, name)


def boxes(comp, name, rects, z0, dz, op):
    sk = new_sketch(comp, name)
    for r in rects:
        add_rect(sk, *r)
    return extrude(comp, sk, z0, dz, op, name)


def circles(comp, name, centres, d, z0, dz, op):
    sk = new_sketch(comp, name)
    for cx, cy in centres:
        add_circle(sk, cx, cy, d)
    return extrude(comp, sk, z0, dz, op, name)


def fillet_vertical(comp, body, corners, radius, name):
    """Fillet the vertical (Z) edges that pass through the given (x, y) points, in mm expressions."""
    pts = [(ev(x), ev(y)) for x, y in corners]
    coll = adsk.core.ObjectCollection.create()
    for i in range(body.edges.count):
        e = body.edges.item(i)
        s, t = e.startVertex.geometry, e.endVertex.geometry
        if abs(s.x - t.x) < 1e-6 and abs(s.y - t.y) < 1e-6 and abs(s.z - t.z) > 1e-4:
            if any(abs(s.x - px) < 1e-4 and abs(s.y - py) < 1e-4 for px, py in pts):
                coll.add(e)
    if coll.count == 0:
        raise RuntimeError(f"{name}: no edges found at {corners}")
    fil = comp.features.filletFeatures
    inp = fil.createInput()
    inp.edgeSetInputs.addConstantRadiusEdgeSet(coll, VI(radius), False)
    f = fil.add(inp)
    f.name = name
    return f


def group_timeline(start_index, name):
    tl = design.timeline
    if tl.count - 1 > start_index:
        tl.timelineGroups.add(start_index, tl.count - 1).name = name
