// Digicam camera 1, v3 bodies (23 Oct 2026): two styles around one inside layout.
//
//   style = "clear"  InstantSnap Novo-like: a frosted tinted shell whose rim stands proud of both
//                    faces, a raised bezel round the screen, a separate panel over the battery,
//                    an orange pill shutter and the power switch on the top left
//   style = "retro"  rangefinder-like: separate top plate, lens barrel and textured covering,
//                    each its own part so each can be its own colour
//
// What changed from v2 (the parts as they arrived, 23 Oct):
//   - the 2.0 in screen (58 x 35 mm module) behind the board
//   - no extension ribbon: the camera folds back onto the board's ESP32 module, so the lens
//     position comes from the board. A collar on the front plate centres it and holds it down
//   - buttons sit in a printed CARRIER (one pocket per switch) screwed to posts on the back
//     plate; each cap has its own guide sleeve and a recessed dish on the back
//   - the charge module sits in front of the battery, USB-C out of the left side
//
// Construction (both styles): a frame (the sides) between a back plate and a front plate,
// M2 screws into heat-set inserts. The board and screen stack on a sled that drops in on
// three screws. Everything prints flat without supports; see 04-case-design.md (v3).
//
// Coordinates: X across the body seen from the BACK (screen side), Y up, Z from the back
// face (0) to the front face (D). Print exports are mirrored back to real space (real()).
//
//   openscad -D 'style="retro"' -D 'part="frame"' -o export/v3/retro/frame.stl camera1_v3.scad
//   python3 export_v3.py        (every part of both styles, renders, and the fit check)

style = "clear";
part = "assembly";
show_parts = true;
$fn = 48;
retro = style == "retro";

// ================================================================ parts as measured (MEASURE = check before printing)
board = [62.4, 28.1];   // ESP32-S3 camera board
board_pcb = 1.6;
board_top = 3.3;        // ESP32 module above the PCB (the camera rests on it): MEASURE
board_under = 2.0;      // tallest part under the board (microSD socket): MEASURE
board_hole_in = 2.6;    // 3 mm corner holes, centre from each edge: MEASURE
board_usb_y = [14.0];   // USB-C socket centres from the board's bottom edge, at its right end (seen from the back): MEASURE
                        // list both if the two sockets are on that end, e.g. [7.5, 20.5]

// Camera, folded back onto the ESP32 module (no extension). Seen from the BACK of the camera:
lens_from_end = 14.0;   // lens centre from the board's LEFT end: MEASURE
lens_from_edge = 14.05; // lens centre from the board's BOTTOM edge: MEASURE
cam_holder = 8.5;       // square lens holder
cam_pcb_t = 1.0;
cam_h = 5.5;            // back of the camera's PCB to the front of the lens: MEASURE
cam_barrel = [8.0, 2.0];   // round part of the lens: diameter, length in front of the square holder: MEASURE
cam_tape = 0.4;         // foam tape between camera and module

scr = [58.0, 35.0];     // 2.0 in screen module, landscape
scr_t = 4.0;            // glass face to the back of the module, pins cut flush: MEASURE
scr_hole_in = 2.5;      // corner holes, centre from each edge: MEASURE
scr_active = [40.8, 30.6];      // 320 x 240 visible area
scr_active_at = [8.6, 2.2];     // visible area's corner from the module's bottom-left corner, glass facing you: MEASURE
                                // (many 2.0 in modules have the glass off-centre, away from the pins)

bat = [34, 50, 10];     // battery, standing, leads out of the top
chg = [16.5, 11.8];     // charge/boost module; its USB-C is on an 11.8 mm edge
chg_t = 4.2;
chg_usb_z = 2.6;        // USB-C socket centre above the module's back face: MEASURE

btn = 5.0;              // tactile switch, flat feet
btn_h = 6.0;            // base to top of the plunger
btn_feet = 7.6;         // feet, tip to tip: MEASURE

sw = [8.6, 3.6, 3.6];   // slide switch body: length (slide direction), height (knob side), width
sw_knob = [2.0, 1.5, 1.5];
sw_travel = 2.0;

// ================================================================ body
W = 112;
H = retro ? 58 : 68;    // retro: the shutter and switch move up into the top plate
R = retro ? 4 : 10;
wall = 2.4;
clr = 0.3;
t_back = 2.0;
t_front = retro ? 2.4 : 2.0;
tp_h = 10.2;            // retro top plate height: ceiling, sleeve, flange, switch on the frame top
bump = retro ? 0 : 0.9; // clear: the rim stands this far proud of both plates
lip = 1.4;              // clear: width of the rim beside the plates
bezel_t = 1.2;          // clear: raised bezel round the screen and buttons
face_z = retro ? 0 : -bezel_t;   // the back surface the button caps sit in

// depth: back plate | screen | sled | board | ESP32 module | camera | front plate
z_bi = t_back;                                    // inside face of the back plate
z_deck0 = z_bi + scr_t + 0.2;
z_deck1 = z_deck0 + 1.2;
z_board0 = z_deck1 + board_under;
z_board1 = z_board0 + board_pcb;
z_cam0 = z_board1 + board_top + cam_tape;         // back of the camera's PCB
z_lens = z_cam0 + cam_h;                          // front of the lens
z_bat0 = z_bi + 0.4;                              // battery on 0.4 mm of tape... and foam in front
z_chg0 = z_bat0 + bat[2] + 0.2;
lens_recess = retro ? 0.6 : 0.3;
z_fi = max(z_lens + lens_recess - t_front,        // lens just behind the front face
           z_lens - cam_barrel[1] + 0.3,          // the square holder stays behind the plate
           z_chg0 + chg_t + 0.3);
cav = z_fi - z_bi;
D = z_fi + t_front;
z_mid = z_bi + cav / 2;
echo(str("STYLE ", style, ": BODY ", W, " x ", H + (retro ? tp_h : 0), " x ", D, " mm, cavity ", cav));

// ================================================================ layout, seen from the back
post_in = 5.8;
post_r = 3.2;
posts = [[post_in, post_in], [W - post_in, post_in], [W - post_in, H - post_in], [post_in, H - post_in]];
insert_d = 3.2;         // M2 heat-set insert, 3.5 mm OD
insert_len = 3.5;

bat_xy = [post_in + post_r + 0.4, wall + 0.6];          // left strip stays free: posts and a wire lane
div_x = bat_xy[0] + bat[0] + clr;
board_x0 = div_x + 1.2 + clr;
scr_xy = [board_x0 + board[0] / 2 - scr[0] / 2, post_in + post_r + 0.2];
board_xy = [board_x0, scr_xy[1] + scr[1] / 2 - board[1] / 2];
board_holes = [for (i = [0, 1], j = [0, 1])
    [board_xy[0] + (i ? board[0] - board_hole_in : board_hole_in), board_xy[1] + (j ? board[1] - board_hole_in : board_hole_in)]];
scr_holes = [for (i = [0, 1], j = [0, 1])
    [scr_xy[0] + (i ? scr[0] - scr_hole_in : scr_hole_in), scr_xy[1] + (j ? scr[1] - scr_hole_in : scr_hole_in)]];
win_xy = scr_xy + scr_active_at;
win_c = win_xy + scr_active / 2;
lens_c = [board_xy[0] + lens_from_end, board_xy[1] + lens_from_edge];

chg_xy = [wall + 0.2, 24.0];          // against the left wall, in front of the battery

// back buttons: previous, OK, next, centred over the screen
btn_y = win_xy[1] + scr_active[1] + 7.4;
btn_xs = [win_c[0] - 11, win_c[0], win_c[0] + 11];
cap_ds = [6.0, 7.0, 6.0];
dish_depth = 0.8;
dish_r = 4.6;
cap_proud = 1.2;        // above the dish floor
sleeve = [3.0, 1.2];    // guide sleeve behind the back plate: length, wall
flange = 1.0;
z_flange0 = z_bi + sleeve[0];                     // cap flange rests here
cap_top = face_z + dish_depth - cap_proud;        // z of a button cap's top
z_plunger = z_flange0 + flange + 0.1;
z_btn_base = z_plunger + btn_h;
carrier_t = 1.4;
carrier_wall = 3.0;     // pocket walls around each switch
carrier_bosses = [[btn_xs[0] - 9, btn_y], [btn_xs[2] + 9, btn_y]];
carrier_xy = [carrier_bosses[0][0] - 3.6, btn_y - 5.5];
carrier_sz = [carrier_bosses[1][0] - carrier_bosses[0][0] + 7.2, 11.0];

// shutter (clear: a pill in the frame's top wall, top left; retro: round, in the top plate)
// and the power switch beside it, with a printed cap on its knob
shutter_x = retro ? W - 16 : 22;
shutter_d = 9.0;
pill = [13.0, 7.0];     // clear shutter: length along the top, width front to back
sw_x = retro ? 20 : 41;
knob_cap = [6.0, 3.0, 3.4];   // slide switch cap: length, height, width

// clear: the panel over the battery, and the bezel round the screen and buttons
panel_rect = [10.2, 6.0, 31.8, 56.0];
bezel_rect = [44.0, 4.0, W - 4.0 - 44.0, H - 8.0];

// sled: two frame blocks under the screen, one above it
sled_pts = [[64, post_in], [94, post_in], [board_x0 + 4.6, scr_xy[1] + scr[1] + 3.0]];

// retro top plate screws (from the top, into bosses under the frame's top wall)
tp_screws = [div_x + 3.0, W - 7.0];

// ================================================================ helpers
module rrect(w, h, r) { offset(r) offset(-r) square([w, h]); }
module outline() { rrect(W, H, R); }
module plate_outline() { if (retro) outline(); else offset(-lip - 0.2) outline(); }
module stadium(sz) { hull() for (s = [-1, 1]) translate([s * (sz[0] - sz[1]) / 2, 0]) circle(d = sz[1]); }
module box(p, s) { translate(p) cube(s); }
module zbox(x, y, w, h, z0, z1) { box([x, y, z0], [w, h, z1 - z0]); }
module zcyl(c, d, z0, z1) { translate([c[0], c[1], z0]) cylinder(d = d, h = z1 - z0); }

// ================================================================ frame (printed standing on its back edge)
module frame() {
    difference() {
        union() {
            if (retro) translate([0, 0, z_bi]) linear_extrude(cav) difference() { outline(); offset(-wall) outline(); }
            else rim();
            for (p = posts) zcyl(p, 2 * post_r, z_bi, z_fi);
            // battery: divider, a stop above it (gaps for its leads), two fingers on the left
            zbox(div_x, wall - 0.5, 1.2, bat_xy[1] + bat[1] - wall + 0.5, z_bi, z_fi);
            zbox(bat_xy[0] + 4, bat_xy[1] + bat[1] + clr, bat[0] - 8 - 12, 1.2, z_bi, z_bat0 + bat[2]);
            for (y = [bat_xy[1] + 6, bat_xy[1] + bat[1] - 10]) zbox(bat_xy[0] - 1.6 - clr, y, 1.6, 4, z_bi, z_bat0 + bat[2]);
            // sled blocks
            for (i = [0 : 2]) let(p = sled_pts[i])
                zbox(p[0] - 3.5, i < 2 ? wall - 0.5 : p[1] - 2.4, 7, i < 2 ? p[1] + 3.2 - wall + 0.5 : 4.8, z_bi, z_deck0);
            if (!retro) {
                // shutter cradle: walls either side and a floor, hanging off the top wall
                for (x = [shutter_x - btn / 2 - clr - 1.2, shutter_x + btn / 2 + clr])
                    zbox(x, shutter_base() - 1.4, 1.2, 1.4 + 3.5, z_bi, z_fi);    // up to the switch body, clear of the cap's flange
                zbox(shutter_x - btn / 2 - clr - 1.2, shutter_base() - 1.4, btn + 2 * clr + 2.4, 1.4, z_bi, z_fi);
                // guide block under the top wall (the pill's shaft runs through it)
                zbox(shutter_x - pill[0] / 2 - 1.4, H - wall - 1.6, pill[0] + 2.8, 1.61, z_bi, z_fi);
                // power switch pocket
                zbox(sw_x - sw[0] / 2 - 1.2, H - wall - sw[1] - 1.2 - 1.6, sw[0] + 2.4, sw[1] + 1.2 + 1.6 + 0.5, z_bi, z_mid + sw[2] / 2 + 1.2);
            } else {
                // bosses under the top wall for the top plate's screws, and lugs for a strap
                for (x = tp_screws) zbox(x - 3, H - wall - 4.5, 6, 4.6, z_bi, z_fi);
                // a socket on the top wall for the shutter switch, which sits under the top plate
                for (x = [shutter_x - btn / 2 - clr - 1.2, shutter_x + btn / 2 + clr]) zbox(x, H - 0.01, 1.2, 1.8, z_bi, z_fi);
            }
        }
        // plate screws: inserts from both faces
        for (p = posts) {
            zcyl(p, insert_d, z_bi - 1, z_bi + insert_len);
            zcyl(p, insert_d, z_fi - insert_len, z_fi + 1);
            zcyl(p, 1.8, 0, D);
        }
        for (p = sled_pts) zcyl(p, insert_d, z_deck0 - insert_len, z_deck0 + 1);
        // battery leads: gap in the stop either side
        // charge module USB-C through the left wall
        port_cut([0, chg_xy[1] + chg[1] / 2, z_chg0 + chg_usb_z], -1);
        // board USB-C through the right wall
        for (y = board_usb_y) port_cut([W, board_xy[1] + y, z_board1 + 1.6], 1);
        if (!retro) {
            // shutter pill hole, and a wire slot through the cradle floor
            translate([shutter_x, H + 1, z_mid]) rotate([90, 0, 0]) linear_extrude(wall + 3.7) stadium(pill + [0.4, 0.4]);
            zbox(shutter_x - 2.5, shutter_base() - 2, 5, 3, z_mid + btn / 2 + 0.3, z_fi + 1);
            // power switch: body pocket (open to the front, slides up into place), and a slot
            // through the top wall for the cap on its knob
            zbox(sw_x - sw[0] / 2 - clr, H - wall - sw[1] - clr - 1.6, sw[0] + 2 * clr, sw[1] + clr + 1.6 + 0.01,
                 z_mid - sw[2] / 2 - clr, z_fi + 1);
            knob_slot(H - wall - 0.5, wall + 1);
        } else {
            for (x = tp_screws) {
                translate([x, H + 1, z_mid]) rotate([90, 0, 0]) cylinder(d = insert_d, h = insert_len + 1);
                translate([x, H + 1, z_mid]) rotate([90, 0, 0]) cylinder(d = 1.8, h = wall + 5);
            }
            // wires up into the top plate: shutter and power switch
            zbox(shutter_x - 3, H - wall - 1, 6, wall + 2, z_mid - 2.5, z_mid + 2.5);
            zbox(sw_x - 4, H - wall - 1, 8, wall + 2, z_mid - 2.5, z_mid + 2.5);
        }
    }
    if (retro) for (s = [-1, 1]) strap_lug(s, H - 9);
    else strap_lug(-1, H - 15);
}
// clear: the rim. Chamfered all round, standing proud of both plates, with a rebate each face
module rim() {
    c = 1.0;
    difference() {
        hull() {
            translate([0, 0, -bump]) linear_extrude(0.01) offset(-c) outline();
            translate([0, 0, -bump + c]) linear_extrude(D + 2 * bump - 2 * c) outline();
            translate([0, 0, D + bump - 0.01]) linear_extrude(0.01) offset(-c) outline();
        }
        translate([0, 0, z_bi - 0.01]) linear_extrude(cav + 0.02) offset(-wall) outline();
        translate([0, 0, -bump - 1]) linear_extrude(z_bi + bump + 1) offset(-lip) outline();
        translate([0, 0, z_fi]) linear_extrude(t_front + bump + 1) offset(-lip) outline();
    }
}
// slot for the slide switch's cap, which travels along X
module knob_slot(y0, h) {
    zbox(sw_x - (knob_cap[0] + sw_travel) / 2 - 0.3, y0, knob_cap[0] + sw_travel + 0.6, h,
         z_mid - knob_cap[2] / 2 - 0.3, z_mid + knob_cap[2] / 2 + 0.3);
}
function shutter_base() = H - wall - 1.6 - flange - 0.1 - btn_h;   // y of the shutter switch's base (clear)

// USB-C: a window for the socket and a wider rounded relief for the plug. side -1 = left wall
module port_cut(c, side) {
    x_in = side < 0 ? -1 : W - wall - 1;
    translate([x_in, c[1] - 4.8, c[2] - 1.9]) cube([wall + 2, 9.6, 3.8]);
    translate([side < 0 ? -0.6 : W - 1.4, c[1], c[2]]) rotate([0, 90, 0])
        linear_extrude(2) offset(1.2) square([6.4 - 2.4, 11.0 - 2.4], center = true);
}

// strap lug: a block on a side wall with a split-ring hole front to back.
// Its underside slopes at 50 degrees, so it prints on the standing frame without support.
module strap_lug(s, y) {
    x = s < 0 ? 0 : W;
    difference() {
        hull() {
            translate([x - s * 0.5, y - 3, z_mid - 3 - 3.4]) cube([0.01, 6, 9.4]);
            translate([x + s * 3.2, y - 2.2, z_mid - 2.2]) cube([0.01, 4.4, 4.4]);
        }
        translate([x + s * 1.9, y, z_bi]) cylinder(d = 1.8, h = cav);
    }
}

// ================================================================ sled (printed flat)
module sled() {
    difference() {
        union() {
            translate([0, 0, z_deck0]) linear_extrude(1.2) sled_2d();
            for (h = board_holes) zcyl(h, 5, z_deck1 - 0.01, z_board0);
        }
        for (h = board_holes) zcyl(h, 1.7, z_deck0 - 1, z_board0 + 1);
        for (h = scr_holes) zcyl(h, 1.7, z_deck0 - 1, z_deck1 + 1);
        for (p = sled_pts) zcyl(p, 2.4, z_deck0 - 1, z_deck1 + 1);
    }
}
module sled_2d() {
    difference() {
        union() {
            translate([board_x0 - 0.2, scr_xy[1] - 0.1]) rrect(board[0] + 0.6, scr[1] + 0.2, 1.5);
            for (i = [0 : 2]) let(p = sled_pts[i])
                translate([p[0] - 3.5, i < 2 ? wall + 0.4 : scr_xy[1] + scr[1] - 1]) rrect(7, i < 2 ? scr_xy[1] - wall : p[1] + 2.4 - scr_xy[1] - scr[1] + 1, 1.2);
        }
        // windows for the screen's and board's wires, and air behind the screen
        translate([scr_xy[0] + 6, scr_xy[1] + 6]) rrect(scr[0] / 2 - 9, scr[1] - 12, 1.5);
        translate([scr_xy[0] + scr[0] / 2 + 3, scr_xy[1] + 6]) rrect(scr[0] / 2 - 9, scr[1] - 12, 1.5);
        // slots at the screen's two short ends, where its pins are
        for (x = [scr_xy[0] + 0.8, scr_xy[0] + scr[0] - 3.8]) translate([x, scr_xy[1] + 9]) rrect(3, scr[1] - 18, 1);
    }
}

// ================================================================ back plate (printed outer face down)
module back_plate() {
    difference() {
        union() {
            linear_extrude(t_back) plate_outline();
            // guide sleeves for the button caps, and the posts the carrier screws to
            for (i = [0 : 2]) {
                zcyl([btn_xs[i], btn_y], cap_ds[i] + 0.4 + 2 * sleeve[1], t_back - 0.01, z_flange0);
            }
            for (b = carrier_bosses) zcyl(b, 6.0, t_back - 0.01, z_btn_base);
            // a lip that locates the screen's glass in its window
            translate([0, 0, t_back - 0.01]) linear_extrude(1.2) difference() {
                translate(scr_xy - [1.4, 1.4]) rrect(scr[0] + 2.8, scr[1] + 2.8, 1.4);
                translate(scr_xy - [0.25, 0.25]) square(scr + [0.5, 0.5]);
                for (p = posts) translate(p) circle(r = post_r + 0.4);
                for (i = [0 : 2]) let(p = sled_pts[i])
                    translate([p[0] - 3.9, i < 2 ? 0 : p[1] - 2.8]) square([7.8, i < 2 ? p[1] + 3.6 : 5.6]);
            }
        }
        for (p = posts) translate([p[0], p[1], -1]) cylinder(d = 2.4, h = t_back + 2);
        // screen window, chamfered toward the outside
        hull() {
            translate([win_xy[0] - 0.3, win_xy[1] - 0.3, 0.8]) linear_extrude(t_back) rrect(scr_active[0] + 0.6, scr_active[1] + 0.6, 1.0);
            translate([win_xy[0] - 1.1, win_xy[1] - 1.1, -0.01]) linear_extrude(0.01) rrect(scr_active[0] + 2.2, scr_active[1] + 2.2, 1.8);
        }
        for (i = [0 : 2]) {
            c = [btn_xs[i], btn_y];
            translate([c[0], c[1], -1]) cylinder(d = cap_ds[i] + 0.4, h = z_flange0 + 2);
            if (retro) translate([c[0], c[1], -0.01]) cylinder(r = dish_r, h = dish_depth + 0.01);   // dish (clear: in the bezel)
            // key slot down the sleeve, so the arrow caps can't turn
            if (i != 1) zbox(c[0] - 0.6, c[1] + cap_ds[i] / 2 - 0.5, 1.2, 1.4, t_back, z_flange0 + 1);
        }
        for (b = carrier_bosses) zcyl(b, 1.7, t_back + 1.0, z_btn_base + 1);
        // clear: the battery panel drops in from outside onto a 0.6 mm ledge
        if (!retro) {
            translate([0, 0, -1]) linear_extrude(t_back - 0.6 + 1) offset(0.15) panel_2d();
            translate([0, 0, -1]) linear_extrude(t_back + 2) offset(-0.8) panel_2d();
        }
    }
}
module panel_2d() { translate([panel_rect[0], panel_rect[1]]) rrect(panel_rect[2], panel_rect[3], 3); }

// clear: the panel over the battery (recessed 0.4 mm) and the raised bezel. Both print flat,
// outer face up, and fix on with thin double-sided tape or a few dots of glue.
module panel() { linear_extrude(1.0) panel_2d(); }
module bezel_2d() {
    difference() {
        translate([bezel_rect[0], bezel_rect[1]]) rrect(bezel_rect[2], bezel_rect[3], 4);
        for (p = posts) if (p[0] > W / 2) translate(p) circle(r = 2.8);
    }
}
module bezel() {   // outer face on top (z = bezel_t), as printed
    difference() {
        linear_extrude(bezel_t) bezel_2d();
        hull() {
            translate([win_xy[0] - 0.3, win_xy[1] - 0.3, -0.01]) linear_extrude(0.01) rrect(scr_active[0] + 0.6, scr_active[1] + 0.6, 1.0);
            translate([win_xy[0] - 1.3, win_xy[1] - 1.3, bezel_t]) linear_extrude(0.02) rrect(scr_active[0] + 2.6, scr_active[1] + 2.6, 2.0);
        }
        for (i = [0 : 2]) {
            translate([btn_xs[i], btn_y, -1]) cylinder(d = cap_ds[i] + 0.4, h = bezel_t + 2);
            translate([btn_xs[i], btn_y, bezel_t - dish_depth]) cylinder(r = dish_r, h = dish_depth + 0.01);
        }
    }
}

// ================================================================ button carrier (printed floor down)
// The three switches drop into its pockets, plungers toward the back plate; their wires leave
// through the slots. It screws to the two posts on the back plate (M2 x 6, self-tapping).
module carrier() {
    difference() {
        union() {
            zbox(carrier_xy[0], carrier_xy[1], carrier_sz[0], carrier_sz[1], z_btn_base, z_btn_base + carrier_t);
            for (x = btn_xs) zbox(x - btn / 2 - clr - 1.2, btn_y - btn / 2 - clr - 1.2, btn + 2 * clr + 2.4, btn + 2 * clr + 2.4,
                                  z_btn_base - carrier_wall, z_btn_base + 0.01);
        }
        for (x = btn_xs) {
            // the switch body, and its feet out to each side
            zbox(x - btn / 2 - clr, btn_y - btn / 2 - clr, btn + 2 * clr, btn + 2 * clr, z_btn_base - carrier_wall - 1, z_btn_base + 0.01);
            zbox(x - btn_feet / 2 - clr, btn_y - 1.6, btn_feet + 2 * clr, 3.2, z_btn_base - 1.2, z_btn_base + 0.01);
            // wires out through the floor, beside the feet
            for (s = [-1, 1]) zbox(x + s * (btn_feet / 2 - 0.6) - 1.2, btn_y - 1.6 - 1.4, 2.4, 1.4, z_btn_base - 1, z_btn_base + carrier_t + 1);
        }
        for (b = carrier_bosses) zcyl(b, 2.4, z_btn_base - 1, z_btn_base + carrier_t + 1);
    }
}

// ================================================================ front plate (printed outer face down)
module front_plate() {
    difference() {
        union() {
            linear_extrude(t_front) plate_outline();
            // camera collar: centres the lens holder and keeps it on the module
            translate([0, 0, -2.0]) linear_extrude(2.01) difference() {
                translate(lens_c) square(cam_holder + 2 * clr + 2.4, center = true);
                translate(lens_c) square(cam_holder + 2 * clr, center = true);
            }
            // charge module: a rim on three sides holds it against the battery
            translate([0, 0, -2.5]) linear_extrude(2.51) difference() {
                translate([chg_xy[0] + 1.0, chg_xy[1] - clr - 1.2]) square([chg[0] + clr + 1.2 - 1.0, chg[1] + 2 * clr + 2.4]);
                translate([chg_xy[0] - 1, chg_xy[1] - clr]) square([chg[0] + clr + 1, chg[1] + 2 * clr]);
            }
        }
        for (p = posts) translate([p[0], p[1], -1]) cylinder(d = 2.4, h = t_front + 2);
        translate([lens_c[0], lens_c[1], -3]) cylinder(d = cam_barrel[0] + 0.6, h = t_front + 4);
        if (!retro) translate([lens_c[0], lens_c[1], t_front - 0.6]) cylinder(d1 = cam_barrel[0] + 0.6, d2 = cam_barrel[0] + 2.0, h = 0.61);
        if (retro) {
            for (a = [90, 210, 330]) translate([lens_c[0] + barrel_screw_r * cos(a), lens_c[1] + barrel_screw_r * sin(a), -1]) cylinder(d = 2.4, h = t_front + 2);
            // charge LEDs: a peep hole over the module
            translate([chg_xy[0] + chg[0] - 4, chg_xy[1] + chg[1] / 2, -1]) cylinder(d = 2.0, h = t_front + 2);
        }
    }
}

// ================================================================ caps and small parts
// back button cap, standing on its top face (printing orientation); shaft, key, flange
module button_cap(i) {
    d = cap_ds[i];
    len = z_flange0 - cap_top;   // top to flange
    difference() {
        union() {
            cylinder(d = d, h = len);
            translate([0, 0, len - 0.01]) cylinder(d = d + 2.4, h = flange);
            if (i != 1) translate([-0.5, d / 2 - 0.6, z_bi + 0.2 - cap_top]) cube([1.0, 1.2, len - (z_bi + 0.2 - cap_top) + 0.01]);
        }
        // arrows on the side caps, a dot on OK (debossed into the top)
        if (i != 1) translate([0, 0, -0.01]) linear_extrude(0.5)
            rotate(i == 0 ? 180 : 0) polygon([[1.6, 0], [-1.0, 1.6], [-1.0, -1.6]]);
        else translate([0, 0, -0.01]) cylinder(d = 2.2, h = 0.5);
    }
}
// shutter cap, standing on its top face: domed top, shaft, flange
function shutter_shaft() = retro ? 1.8 + tp_ceiling + 1.0 : 1.6 + wall + 1.6;   // top of the shaft to the flange
module shutter_cap() {   // its top at z = 0, shaft toward +z
    shaft = shutter_shaft();
    if (retro) {
        scale([1, 1, 0.4]) difference() { sphere(d = shutter_d); translate([0, 0, 5]) cube(10, center = true); }
        cylinder(d = shutter_d, h = shaft);
        translate([0, 0, shaft - 0.01]) cylinder(d = shutter_d + 2.4, h = flange);
    } else {   // the pill: a softly domed top
        hull() for (s = [-1, 1]) translate([s * (pill[0] - pill[1]) / 2, 0, 0])
            scale([1, 1, 0.3]) difference() { sphere(d = pill[1]); translate([0, 0, 5]) cube(10, center = true); }
        linear_extrude(shaft) stadium(pill);
        translate([0, 0, shaft - 0.01]) linear_extrude(flange) stadium(pill + [2.4, 2.4]);
    }
}
// cap glued onto the slide switch's knob (the knob alone doesn't reach the outside)
module knob_cap_part() {   // standing on its top face
    difference() {
        translate([-knob_cap[0] / 2, -knob_cap[2] / 2, 0]) cube([knob_cap[0], knob_cap[2], knob_cap[1]]);
        translate([-sw_knob[0] / 2 - 0.1, -sw_knob[2] / 2 - 0.1, knob_cap[1] - 1.4]) cube([sw_knob[0] + 0.2, sw_knob[2] + 0.2, 2]);
        for (i = [-1, 0, 1]) translate([i * 1.4 - 0.3, -knob_cap[2], -0.01]) cube([0.6, 2 * knob_cap[2], 0.4]);   // grip ridges
    }
}
module lens_ring() {   // clear style
    difference() {
        cylinder(d1 = 19, d2 = 17, h = 2.4);
        translate([0, 0, -1]) cylinder(d = cam_barrel[0] + 1.0, h = 5);
    }
}

// ================================================================ retro parts
// lens barrel: stands on its back face; three M2 inserts from the back, screws through the front plate
barrel_d = 30;
barrel_screw_r = 11.2;
module lens_barrel() {
    difference() {
        union() {
            cylinder(d = barrel_d, h = 1.6);                                     // base ring
            cylinder(d = barrel_d - 2, h = 5.0);                                 // body
            translate([0, 0, 1.9]) knurl(barrel_d + 0.6, 2.4, 64);               // focus grip
            translate([0, 0, 5.0]) cylinder(d1 = barrel_d - 2, d2 = barrel_d - 4.4, h = 1.2);   // front bevel
        }
        // a cone opening at about 48 degrees from the lens, so the barrel stays out of the picture
        translate([0, 0, -0.01]) cylinder(r1 = 4.6, r2 = 11.5, h = 6.22);
        for (a = [90, 210, 330]) translate([barrel_screw_r * cos(a), barrel_screw_r * sin(a), -1]) cylinder(d = insert_d, h = insert_len + 1);
        translate([-0.4, barrel_d / 2 - 3.6, 5.6]) cube([0.8, 1.6, 1]);       // index mark
    }
}
module knurl(d, h, n) {
    linear_extrude(h) for (i = [0 : n - 1]) rotate(i * 360 / n) translate([d / 2 - 1.0, -0.45]) square([1.0, 0.9]);
    cylinder(d = d - 1.6, h = h);
}

// leatherette: one textured panel over the front plate, cut around the barrel and screw heads
module leatherette_2d() {
    difference() {
        offset(-1.2) outline();
        translate(lens_c) circle(d = barrel_d + 1.0);
        for (p = posts) translate(p) circle(r = 3.0);
        translate([chg_xy[0] + chg[0] - 4, chg_xy[1] + chg[1] / 2]) circle(d = 2.6);
    }
}
module leatherette() { grain() leatherette_2d(); }
// the back's covering: the part left of the screen and buttons
module back_leatherette_2d() {
    difference() {
        intersection() { offset(-1.2) outline(); square([win_xy[0] - 4.2, H]); }
        for (p = posts) translate(p) circle(r = 3.0);
    }
}
module back_leatherette() { grain() back_leatherette_2d(); }
// a flat panel with pebbled grain (small raised diamonds) on top, clipped to its outline
module grain() {
    linear_extrude(0.8) children();
    translate([0, 0, 0.79]) linear_extrude(0.4) intersection() {
        offset(-0.6) children();
        for (i = [0 : 2 : W], j = [0 : 2 : H]) translate([i + (j % 4 == 0 ? 0 : 1), j]) rotate(45) square(1.1, center = true);
    }
}

// dark inserts for the top plate's windows (front viewfinder, rangefinder, back eyepiece)
tp_wins = [[14, 2.6, 15, 5.4, "front"], [W - 34, 3.2, 7, 4.2, "front"], [14, 2.6, 11, 5.4, "back"]];
module window_inserts() {
    for (i = [0 : 2]) let(w = tp_wins[i]) translate([i * 20, 0, 0]) cube([w[2] - 0.3, w[3] - 0.3, 0.8]);
}

// top plate: a hollow cap on the frame's top wall, holding the shutter and the power switch.
// Prints upside down (its top on the bed); the dial, shoe and knob are separate small parts.
tp_ceiling = 2.0;
module top_plate_2d() { offset(r = 3) offset(delta = -3) square([W, D + 1.2]); }   // front flush with the covering
module top_plate() {
    y0 = H;
    y1 = H + tp_h;
    intersection() {
        top_plate_body(y0, y1);
        translate([0, y0 - 10, -1]) linear_extrude(D + 4) rrect(W, tp_h + 10, R + 1);   // round the top corners
    }
}
module top_plate_body(y0, y1) {
    difference() {
        union() {
            // shell: walls and ceiling (2D outline is X across, Z front-back)
            translate([0, y1, 0]) rotate([90, 0, 0]) difference() {
                linear_extrude(tp_h) top_plate_2d();
                translate([0, 0, tp_ceiling]) linear_extrude(tp_h) offset(-2.0) top_plate_2d();
            }
            // shutter: guide sleeve under the ceiling, and a cradle the switch sits in
            translate([shutter_x, y1 - tp_ceiling + 0.01, z_mid]) rotate([90, 0, 0]) cylinder(d = shutter_d + 0.4 + 2.4, h = 1.0);
            // power switch pocket, hanging from the ceiling
            zbox(sw_x - sw[0] / 2 - clr - 1.2, y1 - tp_ceiling - sw[1] - 0.6, sw[0] + 2 * clr + 2.4, sw[1] + 0.61,
                 z_mid - sw[2] / 2 - clr - 1.2, z_mid + sw[2] / 2 + clr + 1.2);
            // screw bosses down to the frame
            for (x = tp_screws) translate([x, y1 - 0.01, z_mid]) rotate([90, 0, 0]) cylinder(d = 5.4, h = tp_h - 0.2);
        }
        // shutter cap hole
        translate([shutter_x, y1 + 1, z_mid]) rotate([90, 0, 0]) cylinder(d = shutter_d + 0.4, h = tp_ceiling + 2.2);
        // switch: body pocket and knob slot
        zbox(sw_x - sw[0] / 2 - clr, y1 - tp_ceiling - sw[1] - 1, sw[0] + 2 * clr, sw[1] + 1.01, z_mid - sw[2] / 2 - clr, z_mid + sw[2] / 2 + clr);
        knob_slot(y1 - tp_ceiling - 0.5, tp_ceiling + 1);
        // countersunk screws from the top
        for (x = tp_screws) {
            translate([x, y1 + 1, z_mid]) rotate([90, 0, 0]) cylinder(d = 2.4, h = tp_h + 2);
            translate([x, y1 + 0.01, z_mid]) rotate([90, 0, 0]) cylinder(d1 = 4.4, d2 = 2.4, h = 1.0);
        }
        // front: viewfinder and rangefinder windows (recesses for dark inserts or paint)
        win_depth = 0.8;
        for (w = tp_wins) if (w[4] == "front") zbox(w[0], y0 + w[1], w[2], w[3], D + 1.2 - win_depth, D + 3);
                          else zbox(w[0], y0 + w[1], w[2], w[3], -1, win_depth);
        // holes for the decorations
        translate([W / 2 + 2, y1 + 1, z_mid]) rotate([90, 0, 0]) cylinder(d = 3.0, h = tp_ceiling + 2);      // shoe peg
        translate([shutter_x - 15, y1 + 1, z_mid]) rotate([90, 0, 0]) cylinder(d = 3.0, h = tp_ceiling + 2);  // dial peg
    }
}
// decorations, each standing on its base with a peg underneath
module dial() {   // the "shutter speed dial" beside the shutter
    translate([0, 0, 0]) cylinder(d = 2.8, h = 2.0);
    translate([0, 0, 2.0]) { knurl(13, 3.0, 40); translate([0, 0, 3.0]) cylinder(d = 11, h = 0.8); }
}
module shoe() {   // accessory shoe
    translate([0, 0, 0]) cylinder(d = 2.8, h = 2.0);
    translate([-9, -6, 2.0]) difference() {
        cube([18, 12, 2.4]);
        translate([2, -1, 1.0]) cube([14, 14, 2]);
    }
    translate([-9, -6, 2.0 + 2.4 - 0.6]) for (s = [0, 1]) translate([s * 14.6, 0, 0]) cube([3.4, 12, 0.6]);
}
module shutter_collar() {   // ring round the shutter button
    difference() { cylinder(d = shutter_d + 5, h = 1.2); translate([0, 0, -1]) cylinder(d = shutter_d + 0.6, h = 3); }
}

// ================================================================ electronics (ghosts, for fit)
e = 0.02;
module ghost(name) {
    if (name == "battery") zbox(bat_xy[0] + e, bat_xy[1] + e, bat[0] - 2 * e, bat[1] - 2 * e, z_bat0, z_bat0 + bat[2]);
    if (name == "board") {
        zbox(board_xy[0] + e, board_xy[1] + e, board[0] - 2 * e, board[1] - 2 * e, z_board0 + e, z_board1);
        zbox(board_xy[0] + 6, board_xy[1] + 5, board[0] - 12, board[1] - 10, z_board1, z_board1 + board_top);   // ESP32 module and parts
        zbox(board_xy[0] + 8, board_xy[1] + 4, 16, board[1] - 8, z_board0 - board_under + e, z_board0);          // microSD socket underneath
        for (y = board_usb_y) zbox(board_xy[0] + board[0] - 7.4, board_xy[1] + y - 4.4, 7.4 + 0.5, 8.8, z_board1, z_board1 + 3.2);
    }
    if (name == "camera") translate([lens_c[0], lens_c[1], 0]) {
        zbox(-cam_holder / 2 - 0.5 + e, -cam_holder / 2 - 2 + e, cam_holder + 1 - 2 * e, cam_holder + 2.5 - 2 * e, z_cam0, z_cam0 + cam_pcb_t);
        zbox(-cam_holder / 2 + e, -cam_holder / 2 + e, cam_holder - 2 * e, cam_holder - 2 * e, z_cam0 + cam_pcb_t, z_lens - cam_barrel[1]);
        translate([0, 0, z_lens - cam_barrel[1] - e]) cylinder(d = cam_barrel[0] - e, h = cam_barrel[1]);
    }
    if (name == "screen") zbox(scr_xy[0] + e, scr_xy[1] + e, scr[0] - 2 * e, scr[1] - 2 * e, z_bi + e, z_bi + scr_t);
    if (name == "charger") zbox(chg_xy[0] + e, chg_xy[1] + e, chg[0] - 2 * e, chg[1] - 2 * e, z_chg0, z_chg0 + chg_t);
    if (name == "buttons") for (x = btn_xs) {
        zbox(x - btn / 2 + e, btn_y - btn / 2 + e, btn - 2 * e, btn - 2 * e, z_btn_base - 3.5, z_btn_base - e);
        zbox(x - btn_feet / 2 + e, btn_y - 1.5, btn_feet - 2 * e, 3, z_btn_base - 0.6, z_btn_base - e);
        translate([x, btn_y, z_plunger + e]) cylinder(d = 2.5, h = btn_h - 3.5 - e);
    }
    if (name == "shutter") {
        yb = retro ? H + 0.01 : shutter_base();
        zbox(shutter_x - btn / 2 + e, yb + e, btn - 2 * e, 3.5, z_mid - btn / 2 + e, z_mid + btn / 2 - e);
        translate([shutter_x, yb + 3.5 - e, z_mid]) rotate([-90, 0, 0]) cylinder(d = 2.5, h = btn_h - 3.5);
    }
    if (name == "switch") {
        yt = retro ? H + tp_h - tp_ceiling : H - wall;
        zbox(sw_x - sw[0] / 2 + e, yt - sw[1] + e, sw[0] - 2 * e, sw[1] - 2 * e, z_mid - sw[2] / 2 + e, z_mid + sw[2] / 2 - e);
    }
    if (name == "knob") {
        sw_top = retro ? H + tp_h - tp_ceiling : H - wall;
        translate([sw_x, sw_top + 0.1 + knob_cap[1], z_mid]) rotate([90, 0, 0]) knob_cap_part();
    }
    if (name == "caps" || name == "btncaps") for (i = [0 : 2]) translate([btn_xs[i], btn_y, cap_top]) button_cap(i);
    if (name == "caps" || name == "shuttercap") {

        if (retro) translate([shutter_x, H + tp_h + 1.8, z_mid]) rotate([90, 0, 0]) shutter_cap();
        else translate([shutter_x, H + 1.6, z_mid]) rotate([90, 0, 0]) shutter_cap();
    }
}
ghost_names = ["battery", "board", "screen", "charger", "camera", "buttons", "shutter", "switch"];
module electronics() {
    cols = ["#e0c060", "#2a6b4a", "#1a1a22", "#5b8def", "#333", "#444", "#444", "#888"];
    for (i = [0 : len(ghost_names) - 1]) color(cols[i]) ghost(ghost_names[i]);
}

// ================================================================ colours and assembly
body_col = retro ? "#2b2a2e" : "#a9dcc2";   // clear: mint, printed in translucent PETG
inner_col = "#f2efe8";
accent = "#ff8a3d";
silver = "#d8d6d0";
leather_col = "#7a4a2e";
clear_solid = false;
module clear_part() { if (clear_solid) color([0.75, 0.88, 1.0, 0.3]) children(); else %children(); }

module assembly(gap = 0) {
    if (retro) {
        translate([0, 0, -gap]) color(body_col) back_plate();
        translate([0, 0, z_fi + gap]) color(body_col) front_plate();
        translate([0, 0, D + 1.5 * gap]) color(leather_col) leatherette();
        translate([0, 0, -1.5 * gap]) mirror([0, 0, 1]) color(leather_col) back_leatherette();
        translate([0, gap, 0]) color("#1c1b20") for (w = tp_wins)
            if (w[4] == "front") zbox(w[0] + 0.15, H + w[1] + 0.15, w[2] - 0.3, w[3] - 0.3, D + 1.2 - 0.8 + 0.02, D + 1.2 - 0.02);
            else zbox(w[0] + 0.15, H + w[1] + 0.15, w[2] - 0.3, w[3] - 0.3, 0.02, 0.78);
        translate([lens_c[0], lens_c[1], D + 2.2 * gap]) color(silver) lens_barrel();
        translate([0, gap, 0]) color(silver) top_plate();
        translate([0, 1.6 * gap, 0]) color("#3a393e") top_bits_placed();
    } else {
        translate([0, 0, -gap]) clear_part() back_plate();
        translate([0, 0, z_fi + gap]) clear_part() front_plate();
        translate([lens_c[0], lens_c[1], D + 2 * gap]) color(accent) lens_ring();
        translate([0, 0, 0.4 - gap * 1.6]) color("#e6f2ec") panel();
        translate([0, 0, -gap * 2.2]) color("#e6f2ec") placed_bezel();
    }
    color(body_col, retro ? 1 : 0.85) frame();
    translate([0, 0, gap * 0.4]) color(inner_col) sled();
    translate([0, 0, -gap * 0.5]) color(retro ? silver : accent) carrier();
    translate([0, 0, -2.6 * gap]) color(retro ? silver : accent) ghost("caps");
    translate([0, 2 * gap, 0]) color(retro ? "#3a393e" : "#9a9aa0") ghost("knob");
    if (show_parts) translate([0, 0, gap * 0.4]) electronics();
}
module placed_bezel() { translate([0, 0, 0]) mirror([0, 0, 1]) bezel(); }   // outer face at z = -bezel_t
module top_bits_placed() {
    y1 = H + tp_h;
    translate([shutter_x - 15, y1 - 2.0, z_mid]) rotate([-90, 0, 0]) dial();
    translate([W / 2 + 2, y1 - 2.0, z_mid]) rotate([-90, 0, 0]) rotate(90) shoe();
    translate([shutter_x, y1, z_mid]) rotate([-90, 0, 0]) shutter_collar();
}
module view(gap = 0) { rotate([90, 0, 0]) translate([W, 0, 0]) mirror([1, 0, 0]) assembly(gap); }
module real() { translate([W, 0, 0]) mirror([1, 0, 0]) children(); }

// check_<a>__<b>: overlap of two parts (should be empty). See check_v3.py.
module solid(name) {
    if (name == "frame") frame();
    else if (name == "sled") sled();
    else if (name == "carrier") carrier();
    else if (name == "back_plate") back_plate();
    else if (name == "front_plate") translate([0, 0, z_fi]) front_plate();
    else if (name == "top_plate") top_plate();
    else if (name == "panel") { if (!retro) translate([0, 0, 0.4]) panel(); }
    else if (name == "bezel") { if (!retro) placed_bezel(); }
    else ghost(name);
}
check_a = "";
check_b = "";


// ================================================================ assembly guide steps
// part="guide", step=N: the build as it looks on the desk at step N, in real space with the
// back plate face down and the front facing up. That step's parts are in colour, earlier ones grey.
step = 1;
new_col = "#ff8a3d";
old_col = "#c8c6cc";
module show(first, also = -1) {   // draw children from step `first` on; new at `first` (and `also`)
    if (step >= first) color(step == first || step == also ? new_col : old_col) children();
}
module guide() real() {
    lift = 0;
    // 1: the frame, with its heat-set inserts in gold
    if (step == 1 || step >= 4) show(1, 4) frame();
    if (step == 1) color("#d4a017") {
        for (p = posts) { zcyl(p, 3.5, z_bi, z_bi + insert_len); zcyl(p, 3.5, z_fi - insert_len, z_fi); }
        for (q = sled_pts) zcyl(q, 3.5, z_deck0 - insert_len, z_deck0);
        if (retro) for (x = tp_screws) translate([x, H, z_mid]) rotate([90, 0, 0]) cylinder(d = 3.5, h = insert_len);
    }
    // 2: back plate with the button caps, switches and carrier
    if (step >= 2) {
        show(2) back_plate();
        show(2) ghost("btncaps");
        show(2) translate([0, 0, step == 2 ? 6 : 0]) { ghost("buttons"); carrier(); }
    }
    // 3: the screen, glass down in its lip
    if (step >= 3) show(3) ghost("screen");
    // 5: battery, charge module, shutter and power switches
    if (step >= 5) {
        show(5) ghost("battery");
        show(5) ghost("charger");
        if (!retro) { show(5) ghost("shutter"); show(5) ghost("shuttercap"); show(5) ghost("switch"); show(5) ghost("knob"); }
    }
    // 6: board on the sled, camera folded onto the ESP32 module (shown on its own)
    // 7: the stack screwed down in the frame
    if (step == 6 || step >= 7) {
        show(6, 7) translate([0, 0, step == 6 ? 22 : 0]) { sled(); ghost("board"); ghost("camera"); }
    }
    // 8 (retro): shutter, switch, top plate
    if (retro && step >= 8) {
        show(8) { ghost("shutter"); ghost("shuttercap"); ghost("switch"); ghost("knob"); }
        show(8) translate([0, step == 8 ? 8 : 0, 0]) { top_plate(); top_bits_placed(); }
    }
    // 9: front plate (and the barrel or lens ring)
    if (step >= 9) {
        show(9) translate([0, 0, z_fi + (step == 9 ? 8 : 0)]) front_plate();
        if (retro) show(9) translate([lens_c[0], lens_c[1], D + (step == 9 ? 8 : 0)]) lens_barrel();
        else show(9) translate([lens_c[0], lens_c[1], D + (step == 9 ? 8 : 0)]) lens_ring();
    }
    // 10: finishing: the coverings (retro) or the battery panel and bezel (clear)
    if (step >= 10) {
        if (retro) {
            show(10) translate([0, 0, D + 4]) leatherette();
            show(10) translate([0, 0, -4]) mirror([0, 0, 1]) back_leatherette();
        } else {
            show(10) translate([0, 0, 0.4 - 6]) panel();
            show(10) translate([0, 0, -6]) placed_bezel();
        }
    }
}

// ================================================================ parts
if (part == "assembly") view(0);
else if (part == "exploded") view(14);
else if (part == "frame") real() translate([0, 0, retro ? -z_bi : bump]) frame();
else if (part == "sled") real() translate([0, 0, -z_deck0]) sled();
else if (part == "carrier") real() translate([0, 0, z_btn_base + carrier_t]) mirror([0, 0, 1]) carrier();
else if (part == "back_plate") real() back_plate();
else if (part == "front_plate") real() translate([0, 0, t_front]) mirror([0, 0, 1]) front_plate();
else if (part == "back_plate_2d") real() projection(cut = true) translate([0, 0, -1.5]) back_plate();
else if (part == "front_plate_2d") real() projection(cut = true) translate([0, 0, -1]) front_plate();
else if (part == "caps") {
    for (i = [0 : 2]) translate([i * 12, 0, 0]) button_cap(i);
    translate([retro ? 40 : 44, 0, shutter_shaft() + flange]) mirror([0, 0, 1]) shutter_cap();
    translate([60, 0, 0]) knob_cap_part();
}
else if (part == "panel") real() panel();
else if (part == "bezel") bezel();   // placed flipped, so already the right way round
else if (part == "lens_ring") lens_ring();
else if (part == "lens_barrel") lens_barrel();
else if (part == "leatherette") real() leatherette();
else if (part == "back_leatherette") back_leatherette();   // back covering: its own outline is already the right way round
else if (part == "window_inserts") window_inserts();
else if (part == "top_plate") real() rotate([-90, 0, 0]) translate([0, -H - tp_h, 0]) top_plate();
else if (part == "top_bits") { dial(); translate([20, 0, 0]) shoe(); translate([40, 0, 0]) shutter_collar(); }
else if (part == "guide") guide();
else if (part == "coords") {   // positions for the wiring diagrams (wiring_v3.py), seen from the back
    echo(str("COORDS {", "\"W\":", W, ",\"H\":", H, ",\"wall\":", wall, ",\"R\":", R,
        ",\"battery\":", [bat_xy[0], bat_xy[1], bat[0], bat[1]], ",\"charger\":", [chg_xy[0], chg_xy[1], chg[0], chg[1]],
        ",\"board\":", [board_xy[0], board_xy[1], board[0], board[1]], ",\"screen\":", [scr_xy[0], scr_xy[1], scr[0], scr[1]],
        ",\"lens\":", lens_c, ",\"buttons\":", [for (x = btn_xs) [x, btn_y]], ",\"carrier\":", [carrier_xy[0], carrier_xy[1], carrier_sz[0], carrier_sz[1]],
        ",\"shutter\":", [shutter_x, retro ? H : shutter_base()], ",\"switch\":", [sw_x, retro ? H + tp_h - tp_ceiling - sw[1] : H - wall - sw[1]],
        ",\"div_x\":", div_x, ",\"usb\":", [for (y = board_usb_y) board_xy[1] + y], ",\"posts\":", posts, ",\"tp_h\":", retro ? tp_h : 0, "}"));
    cube(1);
}
else if (part == "check") intersection() { solid(check_a); solid(check_b); }
else if (part == "inside") rotate([90, 0, 0]) translate([W, 0, 0]) mirror([1, 0, 0]) {
    color(body_col) frame(); color(inner_col) sled(); color(accent) carrier(); electronics();
}
