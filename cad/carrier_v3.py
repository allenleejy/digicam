"""The button carrier of camera1_v3.scad, built without OpenSCAD (same numbers, same shapes), plus a fit check
against a model of the 6 x 6 four-leg SMD tactile switch.

    python carrier_v3.py      ->  export/v3/retro/carrier_4leg.stl / .3mf, and prints the fit check
"""
import sys
import cadquery as cq
import numpy as np
import trimesh

# from camera1_v3.scad (the old carrier.stl gives the absolute position)
X0, BTN_Y = 24.6, 49.4                  # first switch centre
XS = [X0, X0 + 11, X0 + 22]
BOSSES = [(X0 - 9, BTN_Y), (X0 + 22 + 9, BTN_Y)]
CARRIER_XY, CARRIER_SZ = (X0 - 12.6, BTN_Y - 5.5), (47.2, 11.0)
CARRIER_T, WALL = 1.4, 3.0
BTN_W, BTN_FEET, BODY = 5.8, 8.0, 3.6
LEG_Y, LEG_W, DROP = 2.25, 0.7, 0.3
PK = 0.2
PI = BTN_W / 2 + PK
PO = PI + 1.2
SNAP_REACH, SNAP_W, SNAP_T = PK + 0.35, 3.0, 0.9
LEG_Y0 = 1.5
WIRE_X0, WIRE_W = BTN_FEET / 2 + 0.2, 1.0
# z in the model's frame: the switch base plane is 0, the floor above it (0 .. CARRIER_T), pockets below
Z_SNAP = -BODY - 0.05


def box(x0, y0, w, h, z0, z1):
    return cq.Workplane("XY").box(w, h, z1 - z0, centered=False).translate((x0, y0, z0))


def cyl(c, d, z0, z1):
    return cq.Workplane("XY").circle(d / 2).extrude(z1 - z0).translate((c[0], c[1], z0))


def wedge(x, s, yi):
    """the catch: from the wall out to snap_reach at z_snap, a lead-in running down to nothing"""
    r = SNAP_REACH
    pts = [(yi, Z_SNAP - 0.7), (yi, Z_SNAP), (yi - s * r, Z_SNAP)] if True else None
    return (cq.Workplane("YZ").polyline(pts).close().extrude(SNAP_W).translate((x - SNAP_W / 2, 0, 0)))


def carrier():
    c = box(CARRIER_XY[0], CARRIER_XY[1], CARRIER_SZ[0], CARRIER_SZ[1], 0, CARRIER_T)
    for x in XS:
        c = c.union(box(x - PO, BTN_Y - PO, 2 * PO, 2 * PO, -WALL, 0.01))
        for s in (-1, 1):
            yi = BTN_Y + s * PI
            c = c.union(box(x - SNAP_W / 2, yi - SNAP_T if s < 0 else yi, SNAP_W, SNAP_T, Z_SNAP - 0.7, -WALL + 0.01))
            c = c.union(wedge(x, s, yi))
    for x in XS:
        for s in (-1, 1):
            yi = BTN_Y + s * PI
            for dx in (-1, 1):
                c = c.cut(box(x + dx * (SNAP_W / 2 + 0.25) - 0.25, yi - 1.3 if s < 0 else yi - 0.1, 0.5, 1.4, Z_SNAP - 1, 0.01))
            c = c.cut(box(x - SNAP_W / 2, yi - 1.3 if s < 0 else yi + SNAP_T, SNAP_W, 1.3 - SNAP_T, Z_SNAP - 1, 0.01))
        c = c.cut(box(x - PI, BTN_Y - PI, 2 * PI, 2 * PI, Z_SNAP, 0.01))
        for sx in (-1, 1):
            for sy in (-1, 1):
                y0 = BTN_Y - PI if sy < 0 else BTN_Y + LEG_Y0
                c = c.cut(box(x - PO - 0.01 if sx < 0 else x + PI - 0.01, y0, PO - PI + 0.02, PI - LEG_Y0, -WALL - 1, 0.01))
                c = c.cut(box(x - BTN_FEET / 2 - 0.35 if sx < 0 else x + BTN_W / 2 - 0.05, y0,
                              BTN_FEET / 2 + 0.35 - BTN_W / 2 + 0.05, PI - LEG_Y0, -0.01, DROP + 0.15))
                c = c.cut(box(x - WIRE_X0 - WIRE_W if sx < 0 else x + WIRE_X0, y0, WIRE_W, PI - LEG_Y0, -1, CARRIER_T + 1))
    for b in BOSSES:
        c = c.cut(cyl(b, 2.4, -1, CARRIER_T + 1))
    return c


def tomesh(wp):
    v, f = wp.val().tessellate(0.02, 0.1)
    return trimesh.Trimesh([(p.x, p.y, p.z) for p in v], f)


def switch(x, leg_y=LEG_Y, leg_w=LEG_W, body=BTN_W, feet=BTN_FEET, foot_len=0.65):
    """the switch as a solid: body from the base plane down, four legs, the feet 0.3 below the base"""
    s = box(x - body / 2, BTN_Y - body / 2, body, body, -BODY, 0)
    s = s.union(cyl((x, BTN_Y), 3.4, -6.0, -BODY))
    for sx in (-1, 1):
        for sy in (-1, 1):
            xa = x + sx * body / 2 if sx > 0 else x - feet / 2
            xb = x + feet / 2 if sx > 0 else x - body / 2
            s = s.union(box(xa, BTN_Y + sy * leg_y - leg_w / 2, xb - xa, leg_w, 0, DROP))
    return s


def overlap(a, b):
    r = a.val().intersect(b.val())
    return r.Volume() if r.Solids() else 0.0


def write_3mf(path, m):
    import zipfile
    v = "".join(f'<vertex x="{a:.4f}" y="{b:.4f}" z="{c:.4f}"/>' for a, b, c in m.vertices)
    t = "".join(f'<triangle v1="{a}" v2="{b}" v3="{c}"/>' for a, b, c in m.faces)
    model = ('<?xml version="1.0" encoding="UTF-8"?><model unit="millimeter" xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02">'
             f'<resources><object id="1" type="model"><mesh><vertices>{v}</vertices><triangles>{t}</triangles></mesh></object></resources>'
             '<build><item objectid="1"/></build></model>')
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", '<?xml version="1.0"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/></Types>')
        z.writestr("_rels/.rels", '<?xml version="1.0"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Target="/3D/3dmodel.model" Id="rel0" Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/></Relationships>')
        z.writestr("3D/3dmodel.model", model)


if __name__ == "__main__":
    c = carrier()
    sw = [switch(x) for x in XS]
    ov = [overlap(c, s) for s in sw]
    print("new carrier vs switch (body + 4 legs), overlap mm^3:", [round(v, 3) for v in ov])
    # legs at the other plausible reading of the drawing (4.5 between the inner edges) and the extremes
    for name, kw in [("legs 2.6 out", dict(leg_y=2.6)), ("legs 1.9 out", dict(leg_y=1.9)),
                     ("feet 8.3 tip to tip", dict(feet=8.3)), ("body 5.9", dict(body=5.9))]:
        print(f"  {name}:", round(sum(overlap(c, switch(x, **kw)) for x in XS), 3))
    m = tomesh(c)
    # print orientation: floor down, so z -> CARRIER_T - z
    m.apply_transform(np.diag([1, 1, -1, 1]))
    m.apply_translation([0, 0, CARRIER_T])
    m.merge_vertices()
    print("bounds", m.bounds.round(2).tolist(), "volume", round(m.volume, 1), "watertight", m.is_watertight)
    out = sys.argv[1] if len(sys.argv) > 1 else "export/v3/retro/carrier_4leg"
    m.export(out + ".stl")
    write_3mf(out + ".3mf", m)
