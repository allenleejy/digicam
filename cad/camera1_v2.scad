// Digicam camera 1, v2 test-fit body (10 Oct 2026), built around the parts as measured.
//
// Still a "clear sandwich": coloured printed frame between two clear plates. New in v2:
//   - an electronics SLED: the board screws to its front face and the screen to its back
//     face, so the whole stack is built and tested outside the case, then drops in on 3 screws
//   - the charge module sits beside the screen, its USB-C out the right-hand side
//   - wiring lanes: across the top above the viewfinder, down beside the lens, and through
//     windows in the sled; bridges hold the battery leads in the top lane
//   - nothing floats: ribs and the divider box the battery, screws hold board, screen and sled
//
// Coordinates: X across the body seen from the BACK (screen side), Y up, Z from the back
// face (0) to the front face (D). Parts export flat for printing.
//
//   openscad -D 'part="frame"' -o export/v2/frame.stl camera1_v2.scad
// Parts: assembly, exploded, frame, sled, back_plate, front_plate, caps, lens_ring, vf_frames,
//        back_plate_2d, front_plate_2d, ghost_<name>, and check views (see check_v2.py)

part = "assembly";
show_parts = true;
$fn = 40;

// ================================================================ measured parts (10 Oct)
board = [62.4, 28.1];   // ESP32-S3 camera board
board_pcb = 1.6;
board_top = 3.3;        // tallest part above the PCB (ESP32 module): MEASURE
board_under = 1.0;      // room under the board for cut header stubs
board_hole_in = 2.6;    // 3 mm corner holes: centre distance from each edge: MEASURE
board_usb_c = 14.0;     // board's USB-C: centre, measured from the board's bottom edge: MEASURE

scr = [31.3, 27.7];     // 1.14 in screen module, landscape
scr_t = 3.8;            // glass face to back of the PCB, headers off: MEASURE
scr_hole_in = 2.2;      // corner holes: centre distance from each edge: MEASURE
scr_active = [24.9, 14.0];   // 240 x 135 visible area, assumed centred on the module: CHECK

bat = [34, 50, 10];     // battery, standing, leads out of the top edge

chg = [16.5, 11.8];     // charge/boost module, USB-C on an 11.8 mm edge, facing the right wall
chg_t = 4.2;            // PCB plus USB-C socket: MEASURE

btn_body = 5.0;         // tactile switch, flat (SMD) feet
btn_h = 6.0;            // base to top of the plunger
btn_feet = 7.6;         // feet, tip to tip: MEASURE

cam = [8.5, 8.5, 5.5];  // OV3660 module on the extension ribbon: MEASURE
lens_d = 8.0;

sw = [8.6, 3.6, 3.6];   // SK02 slide switch body
sw_knob = [2.0, 1.5, 1.5];
sw_travel = 2.0;

// ================================================================ body
W = 110;
H = 64;
R = 6;
wall = 2.2;
plate_t = 1.6;
clr = 0.3;

// The depth is the stack: back plate | screen | sled | board | front plate
scr_gap = 0.2;
deck_t = 1.2;           // sled thickness
front_gap = 0.4;
z_bi = plate_t;                                        // inside face of the back plate
z_deck0 = z_bi + max(scr_gap + scr_t, 0.2 + chg_t);    // sled back face
z_deck1 = z_deck0 + deck_t;                            // sled front face
z_board0 = z_deck1 + board_under;
z_board1 = z_board0 + board_pcb;
z_fi = z_board1 + board_top + front_gap;               // inside face of the front plate
cav = z_fi - z_bi;
D = z_fi + plate_t;
echo(str("BODY ", W, " x ", H, " x ", D, " mm, cavity ", cav));

// ================================================================ layout (seen from the back)
screws = [[5, 5], [W - 5, 5], [W - 5, H - 5], [16, H - 5]];   // M2, plates to frame
boss_r = 3.2;
insert_d = 3.2;         // M2 heat-set insert
insert_len = 3.5;

key_c = [5.5, H - 5.5];
key_d = 4.2;
key_block = [10, 11];

bat_xy = [9.0, 2.7];                       // clear of the bottom-left screw post
div_x = bat_xy[0] + bat[0] + clr;          // divider rib between battery and electronics

board_xy = [div_x + 1.2 + clr, 8.7];       // above the bottom-right screw post
board_holes = [for (i = [0, 1], j = [0, 1])
    [board_xy[0] + (i ? board[0] - board_hole_in : board_hole_in),
     board_xy[1] + (j ? board[1] - board_hole_in : board_hole_in)]];

chg_xy = [W - wall - clr - chg[0], 30.0];  // back side, right of the screen
scr_xy = [min(board_xy[0] + board[0] / 2 - scr[0] / 2, chg_xy[0] - 0.5 - scr[0]),
          board_xy[1] + board[1] / 2 - scr[1] / 2];
scr_holes = [for (i = [0, 1], j = [0, 1])
    [scr_xy[0] + (i ? scr[0] - scr_hole_in : scr_hole_in),
     scr_xy[1] + (j ? scr[1] - scr_hole_in : scr_hole_in)]];
scr_win_xy = [scr_xy[0] + (scr[0] - scr_active[0]) / 2, scr_xy[1] + (scr[1] - scr_active[1]) / 2];

vf = [16, 12];
vf_xy = [board_xy[0] + 1.2, 44.0];
vf_wall = 1.2;

lens_c = [72, 50];
cradle_wall = 1.6;
cradle = [cam[0] + 2 * (clr + cradle_wall), cam[1] + 2 * (clr + cradle_wall)];

btn_xs = [82.5, 93, 103.5];               // back buttons: previous, OK/gallery, next
btn_y = 49;
cap_d = 5.6;
cap_flange = [7.6, 1.0];
cap_proud = 1.0;
z_btn_base = z_bi + cap_flange[1] + btn_h;  // switches sit on the shelf here, plungers toward the back
shelf_t = 1.2;
pocket_d = 0.6;

shutter_x = 95;
shutter_cap_d = 8.0;
y_shutter_base = H - wall - cap_flange[1] - btn_h;   // shutter switch, plunger up
sw_x = 82.5;            // power switch, beside the shutter in its own finger recess
sw_drop = 1.6;          // the pocket runs this much lower, so the switch drops in from the front then slides up
z_mid = z_bi + cav / 2;

// sled fixings: frame blocks with M2 inserts, sled tabs on top
sled_pts = [[board_xy[0] + 8.7, 5.0], [96.5, 5.0], [board_xy[0] + 7.2, 40.0]];

// ================================================================ helpers
module rrect(w, h, r) { offset(r) offset(-r) square([w, h]); }
module outline() { rrect(W, H, R); }
module box(p, s) { translate(p) cube(s); }     // p = [x, y, z], s = [dx, dy, dz]
module zbox(x, y, w, h, z0, z1) { box([x, y, z0], [w, h, z1 - z0]); }

// ================================================================ frame
module frame() {
    difference() {
        union() {
            // perimeter wall
            translate([0, 0, z_bi]) linear_extrude(cav) difference() { outline(); offset(-wall) outline(); }
            for (s = screws) translate([s[0], s[1], z_bi]) cylinder(r = boss_r, h = cav);
            // keychain corner
            translate([0, 0, z_bi]) linear_extrude(cav) intersection() {
                outline();
                translate([0, H - key_block[1]]) square(key_block);
            }
            // battery: two ribs off the left wall, one down from the top wall, and the divider
            for (y = [18, 38]) zbox(wall - 0.5, y, bat_xy[0] - clr - wall + 0.5, 1.2, z_bi, z_fi);
            zbox(24, bat_xy[1] + bat[1] + clr, 1.2, H - wall - bat_xy[1] - bat[1] - clr + 0.5, z_bi, z_fi);
            zbox(div_x, wall - 0.5, 1.2, vf_xy[1] - vf_wall - wall + 0.5 + 0.3, z_bi, z_fi);
            zbox(div_x, vf_xy[1] - vf_wall, vf_xy[0] - vf_wall - div_x + 0.5, bat_xy[1] + bat[1] + clr - vf_xy[1] + vf_wall, z_bi, z_fi);
            // viewfinder tube
            translate([vf_xy[0] - vf_wall, vf_xy[1] - vf_wall, z_bi]) linear_extrude(cav) difference() {
                rrect(vf[0] + 2 * vf_wall, vf[1] + 2 * vf_wall, 2.6);
                translate([vf_wall, vf_wall]) rrect(vf[0], vf[1], 1.6);
            }
            // camera cradle: a tube through the body, the camera seated at the front
            translate([lens_c[0] - cradle[0] / 2, lens_c[1] - cradle[1] / 2, z_bi])
                linear_extrude(cav) difference() { square(cradle); translate([cradle_wall, cradle_wall]) square(cradle - 2 * [cradle_wall, cradle_wall]); }
            // ledge the camera rests on: a 1.3 mm lip, small enough to print without support
            translate([lens_c[0], lens_c[1], z_fi - cam[2] - 1.0]) difference() {
                translate([-cradle[0] / 2 + 0.01, -cradle[1] / 2 + 0.01, 0]) cube([cradle[0] - 0.02, cradle[1] - 0.02, 1.0]);
                translate([-cam[0] / 2 + 1, -cam[1] / 2 + 1, -1]) cube([cam[0] - 2, cam[1] - 2, 3]);
            }
            // back-button shelf, between the cradle and the right wall
            zbox(lens_c[0] + cradle[0] / 2 - 0.1, 44.0, W - wall - lens_c[0] - cradle[0] / 2 + 0.6, 8.2,
                 z_btn_base - pocket_d, z_btn_base - pocket_d + shelf_t);
            // shutter ledge, tied into the top-right screw post; full depth so it prints from the bed
            zbox(shutter_x - 6, y_shutter_base - 1.4, W - 5 - shutter_x + 6, 1.4, z_bi, z_mid + 5);
            // power switch pocket walls
            zbox(sw_x - sw[0] / 2 - 1.2, H - wall - sw[1] - 1.2 - sw_drop, sw[0] + 2.4, sw[1] + 1.2 + sw_drop + 0.5,
                 z_bi, z_mid + sw[2] / 2 + 1.2);
            // sled blocks: two off the bottom wall, one under the viewfinder tube
            for (i = [0 : 2]) let(p = sled_pts[i])
                zbox(p[0] - 3.5, i < 2 ? wall - 0.5 : p[1] - 2.6, 7, i < 2 ? p[1] + 2.8 - wall + 0.5 : vf_xy[1] - vf_wall - p[1] + 2.6 + 0.3,
                     z_bi, z_deck0);
            // bridges over the top wiring lane (battery leads run underneath)
            for (x = [vf_xy[0] + 3, vf_xy[0] + vf[0] - 3])
                zbox(x - 1, vf_xy[1] + vf[1] + vf_wall - 0.3, 2, H - wall - vf_xy[1] - vf[1] - vf_wall + 0.8, z_fi - 3.2, z_fi - 1.6);
        }
        // plate screws: inserts from both faces, clearance between
        for (s = screws) translate([s[0], s[1], 0]) {
            translate([0, 0, z_bi - 1]) cylinder(d = insert_d, h = insert_len + 1);
            translate([0, 0, z_fi - insert_len]) cylinder(d = insert_d, h = insert_len + 1);
            cylinder(d = 1.8, h = D);
        }
        // sled inserts, from the top of each block
        for (p = sled_pts) translate([p[0], p[1], z_deck0 - insert_len]) cylinder(d = insert_d, h = insert_len + 1);
        translate([key_c[0], key_c[1], -1]) cylinder(d = key_d, h = D + 2);
        // camera pocket, open to the front plate; ribbon slot out of the bottom of the cradle
        translate([lens_c[0] - cam[0] / 2 - clr, lens_c[1] - cam[1] / 2 - clr, z_fi - cam[2] - clr])
            cube([cam[0] + 2 * clr, cam[1] + 2 * clr, cam[2] + clr + 1]);
        zbox(lens_c[0] - 4.5, lens_c[1] - cradle[1] / 2 - 1, 9, cradle_wall + 2, z_fi - cam[2] - 1, z_fi + 1);
        // button pockets in the shelf (body and feet), with a slot for the wires
        for (x = btn_xs) {
            zbox(x - btn_feet / 2 - clr, btn_y - btn_body / 2 - clr, btn_feet + 2 * clr, btn_body + 2 * clr,
                 z_btn_base - pocket_d - 0.01, z_btn_base);
            zbox(x - 2.5, btn_y - btn_body / 2 - clr - 2.2 + 0.01, 5, 2.2, z_btn_base - pocket_d - 1, z_btn_base + shelf_t);
        }
        // shutter: cap hole through the top wall, wire slot in the ledge
        translate([shutter_x, H + 1, z_mid]) rotate([90, 0, 0]) cylinder(d = shutter_cap_d + 0.4, h = wall + 2);
        zbox(shutter_x - 2.5, y_shutter_base - 2, 5, 3, z_mid + btn_body / 2 + 0.3, z_mid + 5.1);
        // power switch: body pocket, knob slot, finger recess
        zbox(sw_x - sw[0] / 2 - clr, H - wall - sw[1] - clr - sw_drop, sw[0] + 2 * clr, sw[1] + clr + sw_drop + 0.01,
             z_mid - sw[2] / 2 - clr, z_fi + 1);   // open to the front
        zbox(sw_x - (sw_knob[0] + sw_travel) / 2 - 0.3, H - wall - 0.5, sw_knob[0] + sw_travel + 0.6, wall + 1,
             z_mid - sw_knob[2] / 2 - 0.3, z_mid + sw_knob[2] / 2 + 0.3);
        zbox(sw_x - 6, H - 1.2, 12, 2, z_mid - 3.5, z_mid + 3.5);
        zbox(sw_x - 4, H - wall - sw[1] - sw_drop - 3.5, 8, 3.6, z_mid - 1.5, z_fi + 1);   // its legs and wires, out the bottom
        // right wall: the board's USB-C (front half) and the charge module's (back half)
        port_cut(board_xy[1] + board_usb_c, z_board1 + 1.6);
        port_cut(chg_xy[1] + chg[1] / 2, z_deck0 - 1.0 - 1.6);
    }
}
// USB-C: a tight window for the socket, and a wider rounded relief outside for the plug
module port_cut(y, z) {
    translate([W - wall - 1, y - 4.8, z - 1.9]) cube([wall + 2, 9.6, 3.8]);
    translate([W - 1.4, y, z]) rotate([0, 90, 0]) linear_extrude(2) offset(1.2) square([6.0 - 2.4, 10.5 - 2.4], center = true);
}

// ================================================================ sled (printed back face down)
module sled() {
    difference() {
        union() {
            translate([0, 0, z_deck0]) linear_extrude(deck_t) sled_2d();
            // spacers the board's corners sit on
            for (h = board_holes) translate([h[0], h[1], z_deck1 - 0.01]) cylinder(d = 5, h = board_under + 0.01);
        }
        for (h = board_holes) translate([h[0], h[1], z_deck0 - 1]) cylinder(d = 1.7, h = deck_t + board_under + 2);
        for (h = scr_holes) translate([h[0], h[1], z_deck0 - 1]) cylinder(d = 1.7, h = deck_t + 2);
        for (p = sled_pts) translate([p[0], p[1], z_deck0 - 1]) cylinder(d = 2.4, h = deck_t + 2);
    }
}
module sled_2d() {
    difference() {
        union() {
            // main deck under the board, plus the extension the charge module tapes to
            translate([board_xy[0], board_xy[1] - 0.3]) rrect(W - wall - clr - board_xy[0], board[1] + 0.6, 1.5);
            translate([chg_xy[0] - 0.4, board_xy[1] + board[1] - 2]) rrect(W - wall - clr - chg_xy[0] + 0.4, chg_xy[1] + chg[1] + 0.3 - board_xy[1] - board[1] + 2, 1.5);
            // tabs over the frame blocks
            for (i = [0 : 2]) let(p = sled_pts[i])
                translate([p[0] - 3.5, i < 2 ? wall + 0.4 : board_xy[1] + board[1] - 2]) rrect(7, i < 2 ? board_xy[1] - wall : p[1] + 2.4 - board_xy[1] - board[1] + 2, 1.2);
        }
        // window behind the screen (its components and wires), slots under its two short edges
        translate([scr_xy[0] + 5, scr_xy[1] + 5]) rrect(scr[0] - 10, scr[1] - 10, 1.5);
        for (x = [scr_xy[0] + 0.6, scr_xy[0] + scr[0] - 3.6]) translate([x, scr_xy[1] + 5]) rrect(3, scr[1] - 10, 1);
        // wiring windows either side of the screen
        translate([board_xy[0] + 5.5, board_xy[1] + 6]) rrect(scr_xy[0] - board_xy[0] - 7, board[1] - 12, 1.5);
        translate([chg_xy[0] + 2, board_xy[1] + 6]) rrect(W - wall - chg_xy[0] - 9.5, chg_xy[1] - board_xy[1] - 7.5, 1.5);
    }
}

// ================================================================ plates
module plate_2d(front) {
    difference() {
        outline();
        for (s = screws) translate(s) circle(d = 2.4);
        translate(key_c) circle(d = key_d);
        translate(vf_xy) rrect(vf[0], vf[1], 1.6);
        if (front) {
            translate(lens_c) circle(d = lens_d + 0.8);
        } else {
            translate(scr_win_xy - [0.3, 0.3]) rrect(scr_active[0] + 0.6, scr_active[1] + 0.6, 0.8);
            for (x = btn_xs) translate([x, btn_y]) circle(d = cap_d + 0.4);
        }
    }
}
module back_plate()  { linear_extrude(plate_t) plate_2d(false); }
module front_plate() { linear_extrude(plate_t) plate_2d(true); }

// ================================================================ small printed parts
module button_cap() {
    cylinder(d = cap_flange[0], h = cap_flange[1]);
    translate([0, 0, cap_flange[1] - 0.01]) cylinder(d = cap_d, h = plate_t + cap_proud);
}
module shutter_cap() {
    cylinder(d = shutter_cap_d + 2.4, h = cap_flange[1]);
    translate([0, 0, cap_flange[1] - 0.01]) cylinder(d = shutter_cap_d, h = wall + 1.0);
    translate([0, 0, cap_flange[1] + wall + 0.99]) scale([1, 1, 0.35]) sphere(d = shutter_cap_d);
}
module lens_ring() {
    difference() {
        cylinder(d1 = 19, d2 = 17, h = 2.4);
        translate([0, 0, -1]) cylinder(d = lens_d + 1.2, h = 5);
    }
}
module vf_frame() {
    linear_extrude(1.2) difference() { offset(2.4) rrect(vf[0], vf[1], 1.6); rrect(vf[0], vf[1], 1.6); }
}

// ================================================================ electronics (ghosts, for fit)
e = 0.02;   // ghosts shrink a hair, so parts that only touch don't count as collisions
module ghost(name) {
    if (name == "battery") zbox(bat_xy[0] + e, bat_xy[1] + e, bat[0] - 2 * e, bat[1] - 2 * e, z_mid - bat[2] / 2, z_mid + bat[2] / 2);
    if (name == "board") {
        zbox(board_xy[0] + e, board_xy[1] + e, board[0] - 2 * e, board[1] - 2 * e, z_board0 + e, z_board1);
        // parts on top, kept clear of the corner screw heads
        zbox(board_xy[0] + 6, board_xy[1] + 5, board[0] - 12, board[1] - 10, z_board1, z_board1 + board_top);
    }
    if (name == "screen") zbox(scr_xy[0] + e, scr_xy[1] + e, scr[0] - 2 * e, scr[1] - 2 * e, z_deck0 - scr_t, z_deck0 - e);
    if (name == "charger") {
        zbox(chg_xy[0] + e, chg_xy[1] + e, chg[0] - 2 * e, chg[1] - 2 * e, z_deck0 - chg_t, z_deck0 - e);
    }
    if (name == "camera") translate([lens_c[0], lens_c[1], 0]) {
        zbox(-cam[0] / 2 + e, -cam[1] / 2 + e, cam[0] - 2 * e, cam[1] - 2 * e, z_fi - cam[2], z_fi - 3);
        translate([0, 0, z_fi - cam[2]]) cylinder(d = lens_d - e, h = cam[2] - e);
    }
    if (name == "buttons") for (x = btn_xs) {
        zbox(x - btn_body / 2, btn_y - btn_body / 2, btn_body, btn_body, z_btn_base - 3.5, z_btn_base - e);
        zbox(x - btn_feet / 2, btn_y - 1.5, btn_feet, 3, z_btn_base - 0.6, z_btn_base - e);
        translate([x, btn_y, z_bi + cap_flange[1] + e]) cylinder(d = 2.5, h = btn_h - 3.5);
    }
    if (name == "shutter") zbox(shutter_x - btn_body / 2, y_shutter_base + e, btn_body, btn_h - 2 * e, z_mid - btn_body / 2, z_mid + btn_body / 2);
    if (name == "switch") zbox(sw_x - sw[0] / 2, H - wall - sw[1], sw[0], sw[1] - e, z_mid - sw[2] / 2, z_mid + sw[2] / 2);
    if (name == "caps") {
        for (x = btn_xs) translate([x, btn_y, z_bi + cap_flange[1] + e]) mirror([0, 0, 1]) button_cap();
        translate([shutter_x, H - wall - cap_flange[1] + e, z_mid]) rotate([-90, 0, 0]) shutter_cap();
    }
}
ghost_names = ["battery", "board", "screen", "charger", "camera", "buttons", "shutter", "switch"];
module electronics() {
    cols = ["#e0c060", "#2a6b4a", "#1a1a22", "#5b8def", "#333", "#444", "#444", "#888"];
    for (i = [0 : len(ghost_names) - 1]) color(cols[i]) ghost(ghost_names[i]);
}

// ================================================================ assembly and views
frame_col = "#c9b8f2";
sled_col = "#f2efe8";
accent = "#ff8a3d";
clear_solid = false;
module clear_part() { if (clear_solid) color([0.75, 0.88, 1.0, 0.28]) children(); else %children(); }

module assembly(gap = 0) {
    translate([0, 0, -gap]) clear_part() back_plate();
    color(frame_col) frame();
    translate([0, 0, gap * 0.5]) color(sled_col) sled();
    translate([0, 0, z_fi + gap]) clear_part() front_plate();
    translate([lens_c[0], lens_c[1], D + 2 * gap]) color(accent) lens_ring();
    translate([vf_xy[0], vf_xy[1], D + 2 * gap]) color(accent) vf_frame();
    translate([vf_xy[0], vf_xy[1], -1.2 - 2 * gap]) color(accent) vf_frame();
    translate([0, 0, -2 * gap]) color(accent) ghost("caps");
    if (show_parts) translate([0, 0, gap * 0.5]) electronics();
}
module view(gap = 0) { rotate([90, 0, 0]) translate([W, 0, 0]) mirror([1, 0, 0]) assembly(gap); }

module real() { translate([W, 0, 0]) mirror([1, 0, 0]) children(); }

// check_<a>__<b>: the overlap of two parts (should be empty). See check_v2.py.
module solid(name) {
    if (name == "frame") frame();
    else if (name == "sled") sled();
    else if (name == "back_plate") back_plate();
    else if (name == "front_plate") translate([0, 0, z_fi]) front_plate();
    else ghost(name);
}
check_a = "";
check_b = "";

if (part == "assembly") view(0);
else if (part == "exploded") view(12);
// Print exports are mirrored in X. The model's X runs left to right seen from the BACK
// while Z runs back to front, which is a mirror image of real space; without this the
// printed camera would have its buttons and shutter on the wrong side.
else if (part == "frame") real() translate([0, 0, -z_bi]) frame();
else if (part == "sled") real() translate([0, 0, -z_deck0]) sled();
else if (part == "back_plate") real() back_plate();
else if (part == "front_plate") real() front_plate();
else if (part == "back_plate_2d") real() plate_2d(false);
else if (part == "front_plate_2d") real() plate_2d(true);
else if (part == "caps") {
    for (i = [0 : 2]) translate([i * 11, 0, 0]) button_cap();
    translate([38, 0, 0]) shutter_cap();
}
else if (part == "lens_ring") lens_ring();
else if (part == "vf_frames") { vf_frame(); translate([0, 20, 0]) vf_frame(); }
else if (part == "check") intersection() { solid(check_a); solid(check_b); }
else if (part == "inside") {   // frame, sled and parts, no plates: for the fit renders
    rotate([90, 0, 0]) translate([W, 0, 0]) mirror([1, 0, 0]) {
        color(frame_col) frame(); color(sled_col) sled(); electronics();
    }
}
else if (part == "inside_noboard") {   // the same, with the board lifted off to show the sled
    rotate([90, 0, 0]) translate([W, 0, 0]) mirror([1, 0, 0]) {
        color(frame_col) frame(); color(sled_col) sled();
        for (n = ["battery", "screen", "charger", "camera", "buttons", "shutter", "switch"]) color("#555") ghost(n);
    }
}
