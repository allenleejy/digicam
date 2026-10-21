// Digicam camera 1: InstantSnap-style body, v1 concept (8 Oct 2026)
//
// A "clear sandwich": a coloured printed frame between two clear plates, so the
// battery, board and wiring show through, held by four M2 screws into heat-set inserts.
//
// Coordinates (model space): X across the body as seen from the BACK (screen side),
// Y up, Z from the back face (0) to the front face (D). Parts export flat for printing.
//
// Export a part:   openscad -D 'part="frame"' -o frame.stl instantsnap_body.scad
// Parts: assembly, exploded, frame, back_plate, front_plate, caps, lens_ring,
//        vf_frames, back_plate_2d, front_plate_2d (DXF for laser-cut acrylic)

part = "assembly";
show_parts = true;      // ghost the electronics in assembly views
$fn = 48;

// ---------------------------------------------------------------- body
W = 116;                // width
H = 66;                 // height
R = 7;                  // corner radius
wall = 2.2;             // frame wall
plate_t = 1.6;          // clear plates (1.5 mm acrylic also works)
cav = 12.0;             // inside depth = frame thickness
D = cav + 2 * plate_t;  // total thickness: 15.2 mm
clr = 0.3;              // fit clearance

// ---------------------------------------------------------------- fixings
screws = [[5, 5], [W - 5, 5], [W - 5, H - 5], [17, H - 5]];   // M2
boss_r = 3.2;
insert_d = 3.2;         // M2 heat-set insert hole
insert_len = 3.5;
screw_hole = 2.4;

key_c = [6, H - 6];     // keychain hole, top-left corner seen from the back
key_d = 4.2;

// ---------------------------------------------------------------- parts you measured
bat = [35, 52, 10.5];   // 103450 cell: 34 x 50 x 10 plus protection board and wire allowance
bat_xy = [9, 2.6];

chg = [18, 14, 4];      // charge + boost module, USB-C on its right-hand 14 mm edge
chg_xy = [W - wall - 18 - clr, 38.5];

btn = [6, 6, 5];        // 6x6 tactile switch, 5 mm tall including the plunger
btn_body_h = 3.5;
btn_plunger_d = 3.5;

sw = [8.6, 3.6, 3.6];   // SK-12D02 slide switch body (G1.5 = 1.5 mm knob)
sw_knob = [2.0, 1.5, 1.5];
sw_travel = 2.0;

// ---------------------------------------------------------------- parts still to measure (estimates)
board = [67, 28, 7.0];  // GOOUUU ESP32-S3-CAM with pin headers removed: MEASURE
board_xy = [46, 8.5];
board_usb_y = [22];     // centre heights of its USB-C port(s) on the right-hand end: MEASURE

// Screen options, landscape: module size (incl. thickness, headers removed) and active area.
// Waveshare's published sizes; generic modules differ, so MEASURE the one you buy.
// 2.0 in (320x240) matches the camera's 4:3 photos exactly; see renders/screen-compare.png.
screen = "2.0";         // [1.14, 1.69, 2.0]
scr_table = [
    ["1.14", [35, 32, 4.0], [24.91, 14.86]],
    ["1.69", [39, 31.5, 4.0], [32.63, 27.97]],
    ["2.0",  [58, 35, 4.5], [40.80, 30.60]],
];
scr_row = [for (r = scr_table) if (r[0] == screen) r][0];
scr = scr_row[1];
scr_active = scr_row[2];
// centred over the board, but kept clear of the bottom-right screw post
scr_xy = [min(board_xy[0] + board[0] / 2 - scr[0] / 2, W - 5 - boss_r - 0.6 - scr[0]), 3.0];

cam = [8.5, 8.5, 5.5];  // OV3660 module on the extension cable: MEASURE
lens_d = 8.0;
lens_c = [72, 51];

// ---------------------------------------------------------------- controls
vf = [16, 12];          // see-through viewfinder window
vf_xy = [48, 45];

btn_xs = [84, 96, 108]; // back buttons: previous, OK/gallery, next
btn_y = 50;
cap_d = 6.0;            // button cap shaft
cap_flange = [8.4, 1.0];
cap_proud = 1.2;        // how far caps stand above the back plate

shutter_x = 100;        // top edge, right-hand side seen from the back
shutter_cap_d = 8.0;
sw_x = 82;              // power switch, top edge

// z levels
z_back_in = plate_t;            // inner face of the back plate
z_front_in = plate_t + cav;     // inner face of the front plate
btn_shelf_z = z_back_in + cap_flange[1] + btn[2];   // buttons press against caps from this shelf
shelf_t = 1.6;

// ================================================================ helpers
module rrect(w, h, r) { offset(r) offset(-r) square([w, h]); }
module outline() { rrect(W, H, R); }
module rbox(size, r = 1) { linear_extrude(size[2]) rrect(size[0], size[1], r); }

// ================================================================ frame (printed, coloured)
module frame() {
    difference() {
        union() {
            // perimeter wall
            linear_extrude(cav) difference() { outline(); offset(-wall) outline(); }
            // corner screw bosses
            for (s = screws) translate(s) cylinder(r = boss_r, h = cav);
            // solid corner for the keychain hole
            linear_extrude(cav) intersection() {
                outline();
                translate([0, H - 11]) square([11, 11]);
            }
            // viewfinder tube
            translate([vf_xy[0] - 1.2, vf_xy[1] - 1.2, 0])
                linear_extrude(cav) difference() {
                    rrect(vf[0] + 2.4, vf[1] + 2.4, 2.6);
                    translate([1.2, 1.2]) rrect(vf[0], vf[1], 1.6);
                }
            // rib tying the viewfinder tube to the top wall
            translate([vf_xy[0] + vf[0] / 2 - 1, vf_xy[1] + vf[1], 0]) cube([2, H - wall - vf_xy[1] - vf[1] + 0.5, cav]);
            // camera cradle on the front side, hung from the top wall
            translate([lens_c[0] - cam[0] / 2 - 1.6, lens_c[1] - cam[1] / 2 - 1.6, cav - cam[2] - 1.2])
                cube([cam[0] + 3.2, cam[1] + 3.2, cam[2] + 1.2]);
            translate([lens_c[0] - 1.5, lens_c[1] + cam[1] / 2, cav - cam[2] - 1.2])
                cube([3, H - wall - lens_c[1] - cam[1] / 2 + 0.5, cam[2] + 1.2]);
            // shelf the three back buttons push against
            translate([btn_xs[0] - 5, btn_y - 6, btn_shelf_z - z_back_in])
                cube([W - wall - btn_xs[0] + 5 + 0.5, 12, shelf_t]);
            // ledge under the shutter switch
            translate([shutter_x - 5, H - wall - 1.0 - btn[2] - shelf_t, cav / 2 - 5])
                cube([W - wall - shutter_x + 5 + 0.5, shelf_t, 10]);
            // power switch pocket walls
            translate([sw_x - sw[0] / 2 - 1.2, H - wall - sw[1] - 1.2, cav / 2 - sw[2] / 2 - 1.2])
                cube([sw[0] + 2.4, sw[1] + 1.2 + 0.5, sw[2] + 2.4]);
            // battery divider: rib along the battery's inner side, rising from the bottom wall
            // (two free-standing ribs here before 9 Oct printed as loose pieces)
            translate([bat_xy[0] + bat[0] + clr, wall - 0.5, 0]) cube([1.2, bat_xy[1] + bat[1] - wall + 0.5, cav]);
        }
        // screw inserts from both faces, and clearance through the middle
        for (s = screws) translate(s) {
            translate([0, 0, -1]) cylinder(d = insert_d, h = insert_len + 1);
            translate([0, 0, cav - insert_len]) cylinder(d = insert_d, h = insert_len + 1);
            translate([0, 0, -1]) cylinder(d = 1.8, h = cav + 2);
        }
        // keychain hole
        translate([key_c[0], key_c[1], -1]) cylinder(d = key_d, h = cav + 2);
        // camera pocket, open to the front plate
        translate([lens_c[0] - cam[0] / 2 - clr, lens_c[1] - cam[1] / 2 - clr, cav - cam[2] - clr])
            cube([cam[0] + 2 * clr, cam[1] + 2 * clr, cam[2] + clr + 1]);
        // flex cable slot out of the cradle, downward toward the board
        translate([lens_c[0] - 4, lens_c[1] - cam[1] / 2 - 2, cav - cam[2] - 2]) cube([8, 3, cam[2] + 3]);
        // button pockets: switch bodies sit in the shelf so their plungers face the back plate
        for (x = btn_xs) translate([x - btn[0] / 2 - clr, btn_y - btn[1] / 2 - clr, btn_shelf_z - z_back_in - btn_body_h])
            cube([btn[0] + 2 * clr, btn[1] + 2 * clr, btn_body_h + 0.01]);
        for (x = btn_xs) translate([x - 1.5, btn_y - 3.8, btn_shelf_z - z_back_in - 0.1]) cube([3, 7.6, shelf_t + 0.2]);   // legs and wires
        // shutter: cap hole through the top wall
        translate([shutter_x, H + 1, cav / 2]) rotate([90, 0, 0]) cylinder(d = shutter_cap_d + 0.4, h = wall + 2);
        // power switch: body pocket, knob slot, and a finger recess so the knob can't catch in a bag
        translate([sw_x - sw[0] / 2 - clr, H - wall - sw[1] - clr, cav / 2 - sw[2] / 2 - clr])
            cube([sw[0] + 2 * clr, sw[1] + clr + 0.01, sw[2] + 2 * clr]);
        translate([sw_x - (sw_knob[0] + sw_travel) / 2 - 0.3, H - wall - 0.5, cav / 2 - sw_knob[2] / 2 - 0.3])
            cube([sw_knob[0] + sw_travel + 0.6, wall + 1, sw_knob[2] + 0.6]);
        translate([sw_x - 6, H - 1.2, cav / 2 - 3.5]) cube([12, 2, 7]);
        // right wall: charge port (module) and the board's own USB-C (firmware updates)
        translate([W - wall - 1, chg_xy[1] + chg[1] / 2 - 5, cav - chg[2] / 2 - 2.2]) port_cut();
        for (y = board_usb_y) translate([W - wall - 1, y - 5, cav - 3.2 - 2.2]) port_cut();
    }
}
module port_cut() {
    // USB-C plug overmould is about 12.5 x 6.5 mm
    rotate([0, 90, 0]) translate([-4.4, 0, 0]) linear_extrude(wall + 2) translate([0, 0]) rrect(4.4, 10, 1.4);
}

// ================================================================ clear plates
module plate_2d(front) {
    difference() {
        outline();
        for (s = screws) translate(s) circle(d = screw_hole);
        translate(key_c) circle(d = key_d);
        translate(vf_xy) rrect(vf[0], vf[1], 1.6);
        if (front) {
            // front is seen mirrored: lens at the same model X
            translate(lens_c) circle(d = lens_d + 0.8);
        } else {
            translate([scr_xy[0] + (scr[0] - scr_active[0]) / 2 - 0.3, scr_xy[1] + (scr[1] - scr_active[1]) / 2 - 0.3])
                rrect(scr_active[0] + 0.6, scr_active[1] + 0.6, 0.8);
            for (x = btn_xs) translate([x, btn_y]) circle(d = cap_d + 0.4);
        }
    }
}
module back_plate()  { linear_extrude(plate_t) plate_2d(false); }
module front_plate() { linear_extrude(plate_t) plate_2d(true); }

// ================================================================ small printed parts
module button_cap() {
    // shaft through the back plate, flange inside so it can't fall out
    cylinder(d = cap_flange[0], h = cap_flange[1]);
    translate([0, 0, cap_flange[1] - 0.01]) cylinder(d = cap_d, h = plate_t + cap_proud);
}
module shutter_cap() {
    cylinder(d = shutter_cap_d + 2.4, h = 1.0);
    translate([0, 0, 0.99]) cylinder(d = shutter_cap_d, h = wall + 1.0);
    translate([0, 0, wall + 1.99]) scale([1, 1, 0.35]) sphere(d = shutter_cap_d);   // domed top
}
module lens_ring() {
    difference() {
        cylinder(d1 = 19, d2 = 17, h = 2.4);
        translate([0, 0, -1]) cylinder(d = lens_d + 1.2, h = 5);
    }
}
module vf_frame() {
    // swappable trim around the viewfinder window
    linear_extrude(1.2) difference() {
        offset(2.4) rrect(vf[0], vf[1], 1.6);
        rrect(vf[0], vf[1], 1.6);
    }
}

// ================================================================ electronics (ghosts, for checking fit)
module electronics() {
    color("#e0c060") translate([bat_xy[0], bat_xy[1], z_back_in + (cav - bat[2]) / 2]) rbox(bat, 2);
    color("#2a6b4a") translate([board_xy[0], board_xy[1], z_front_in - board[2]]) {
        cube([board[0], board[1], 1.6]);
        translate([6, 4, 1.6]) color("#b8b8c0") cube([25.5, 18, 3.1]);        // ESP32-S3 module
    }
    color("#1a1a22") translate([scr_xy[0], scr_xy[1], z_back_in]) cube(scr);
    color("#5b8def") translate([chg_xy[0], chg_xy[1], z_front_in - chg[2]]) cube(chg);
    color("#333") translate([lens_c[0] - cam[0] / 2, lens_c[1] - cam[1] / 2, z_front_in - cam[2]]) {
        cube([cam[0], cam[1], cam[2] - 3]);
        translate([cam[0] / 2, cam[1] / 2, 0]) cylinder(d = lens_d, h = cam[2]);
    }
    for (x = btn_xs) color("#444") translate([x - 3, btn_y - 3, btn_shelf_z - btn_body_h]) {
        cube([6, 6, btn_body_h]);
        translate([3, 3, -1.5]) cylinder(d = btn_plunger_d, h = 1.5);
    }
    color("#444") translate([shutter_x - 3, H - wall - 1.0 - btn[2], z_back_in + cav / 2 - 3]) cube([6, btn_body_h, 6]);
    color("#888") translate([sw_x - sw[0] / 2, H - wall - sw[1], z_back_in + cav / 2 - sw[2] / 2]) cube(sw);
}

// ================================================================ assembly
frame_col = "#c9b8f2";
accent = "#ff8a3d";
clear = [0.75, 0.88, 1.0, 0.28];

// OpenSCAD's preview can't layer see-through solids reliably, so in assembly views the clear
// plates are drawn as background (%) ghosts. Set clear_solid=true to see them as solids.
clear_solid = false;
module clear_part() { if (clear_solid) color(clear) children(); else %children(); }

module assembly(gap = 0) {
    translate([0, 0, -gap]) clear_part() back_plate();
    translate([0, 0, z_back_in]) color(frame_col) frame();
    translate([0, 0, z_front_in + gap]) clear_part() front_plate();
    translate([lens_c[0], lens_c[1], D + 2 * gap]) color(accent) lens_ring();
    translate([vf_xy[0], vf_xy[1], D + 2 * gap]) color(accent) vf_frame();
    translate([vf_xy[0], vf_xy[1], -1.2 - 2 * gap]) color(accent) vf_frame();
    for (x = btn_xs) translate([x, btn_y, -cap_proud - 2 * gap + 0.0]) color(accent)
        translate([0, 0, plate_t + cap_proud + cap_flange[1]]) mirror([0, 0, 1]) button_cap();
    translate([shutter_x, H - wall + 1.0 + gap, z_back_in + cav / 2]) rotate([-90, 0, 0]) translate([0, 0, -1.0]) color(accent) shutter_cap();
    if (show_parts) electronics();
}

// Render views: stand the camera up (model Y -> world Z, front faces -Y). Mirroring X keeps
// model X running left-to-right when you look at the back, as defined above.
module view(gap = 0) { rotate([90, 0, 0]) translate([W, 0, 0]) mirror([1, 0, 0]) assembly(gap); }

if (part == "assembly") view(0);
else if (part == "exploded") view(14);
else if (part == "frame") frame();
else if (part == "back_plate") back_plate();
else if (part == "front_plate") front_plate();
else if (part == "back_plate_2d") plate_2d(false);
else if (part == "front_plate_2d") plate_2d(true);
else if (part == "caps") {
    for (i = [0 : 2]) translate([i * 12, 0, 0]) button_cap();
    translate([40, 0, 0]) shutter_cap();
}
else if (part == "lens_ring") lens_ring();
else if (part == "vf_frames") { vf_frame(); translate([0, 22, 0]) vf_frame(); }
