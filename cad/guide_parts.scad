// Parts for the 3D assembly guide (../guide), in assembly position, one file per part.
// Built on camera1_v2.scad; adds screws, inserts, foam, more detailed electronics, and wires.
//   openscad -D 'gpart="board_pcb"' -o x.stl guide_parts.scad      (export_guide.py runs them all)
//
// Wire routes follow the lanes in the case. Pin positions along the board's edges are
// illustrative: wire to the pins printed on your board, as in ../diagrams/wiring-bench.png.

include <camera1_v2.scad>
part = "none";          // silence camera1_v2's own output
gpart = "";
$fn = 24;

// ---------------------------------------------------------------- board pins (illustrative)
function pad(row, i) = [board_xy[0] + 6 + i * 2.54, row == 0 ? board_xy[1] + 1.5 : board_xy[1] + board[1] - 1.5];
module pads() { for (row = [0, 1], i = [0 : 19]) translate([pad(row, i)[0], pad(row, i)[1], z_board0 - 0.05]) cylinder(d = 1.6, h = board_pcb + 0.1); }

// ---------------------------------------------------------------- wires
// A wire through a list of [x, y, z] points
module wire(pts, d = 0.9) {
    for (i = [0 : len(pts) - 2]) hull() {
        translate(pts[i]) sphere(d = d, $fn = 10);
        translate(pts[i + 1]) sphere(d = d, $fn = 10);
    }
}
// battery leads: up out of the cell, along the top lane under the bridges, down beside the lens,
// across above the sled to the charge module's B+ / B- pads
zb = 3.6;                                   // wire height behind the sled
chg_pad = function(k) [chg_xy[0] + 0.8, chg_xy[1] + 1.6 + k * 2.8, z_deck0 - 1.15];   // B+, B-, OUT+, OUT-
function bat_lead(dx) = [
    [bat_xy[0] + bat[0] - 4 + dx, bat_xy[1] + bat[1] - 0.5, z_mid],
    [bat_xy[0] + bat[0] - 4 + dx, H - wall - 2.8 + dx * 0.5, z_mid],
    [vf_xy[0] + vf[0] + vf_wall + 1.2 + dx, H - wall - 2.8 + dx * 0.5, z_mid + 0.5],
    [vf_xy[0] + vf[0] + vf_wall + 1.2 + dx, vf_xy[1] - vf_wall - 1, z_mid + 0.5],
    [vf_xy[0] + vf[0] + vf_wall + 1.2 + dx, board_xy[1] + board[1] + 3.5 + dx, zb],
    [chg_xy[0] - 3, board_xy[1] + board[1] + 3.5 + dx, zb],
    chg_pad(dx > 0 ? 0 : 1),
];
// screen: 8 wires from the pads on its left edge, through the slot in the sled, under the board
// to the board's lower row of pins (GND, 3V3, 21, 47, EN, 41, 42, 46)
function scr_wire(k) = [
    [scr_xy[0] + 1.5, scr_xy[1] + 6 + k * 2.2, z_deck0 - 0.1],
    [scr_xy[0] + 2.1, scr_xy[1] + 6 + k * 2.2, z_deck1 + 0.5],
    [pad(0, k)[0], board_xy[1] + 4 + k * 0.3, z_deck1 + 0.5],
    [pad(0, k)[0], pad(0, k)[1], z_board0 - 0.05],
];
// buttons: one wire per button from its left foot through the shelf slot to the upper row
// (pins 2, 14, 45); the right feet share one GND wire
function btn_wire(j, i) = let(x = btn_xs[j]) [
    [x - 3.8, btn_y + 1.5, z_btn_base - 0.3],
    [x - 1.5, btn_y - btn_body / 2 - clr - 1.1, z_btn_base - 0.3],
    [x - 1.5, btn_y - btn_body / 2 - clr - 1.1, z_btn_base + shelf_t + 0.8],
    [pad(1, i)[0], board_xy[1] + board[1] + 2.5, z_board1 + 1.6],
    [pad(1, i)[0], pad(1, i)[1], z_board1 + 0.05],
];
btn_gnd = concat([for (x = btn_xs) each [[x + 3.8, btn_y - 1.5, z_btn_base - 0.3], [x + 3.8, btn_y - btn_body / 2 - clr - 1.1, z_btn_base - 0.6]]],
    [[btn_xs[2] + 1.5, btn_y - btn_body / 2 - clr - 1.1, z_btn_base + shelf_t + 0.8],
     [pad(1, 16)[0], board_xy[1] + board[1] + 2.5, z_board1 + 1.6], [pad(1, 16)[0], pad(1, 16)[1], z_board1 + 0.05]]);
// shutter (pin 48): through the slot in its ledge, down over the shelf
shutter_wire = [[shutter_x - 3.8, y_shutter_base - 0.2, z_mid], [shutter_x - 1.5, y_shutter_base - 1, z_mid + 4],
    [shutter_x - 1.5, y_shutter_base - 4, z_btn_base + shelf_t + 1.0], [pad(1, 15)[0], board_xy[1] + board[1] + 2.5, z_board1 + 1.6],
    [pad(1, 15)[0], pad(1, 15)[1], z_board1 + 0.05]];
shutter_gnd = [[shutter_x + 3.8, y_shutter_base - 0.2, z_mid], [shutter_x + 1.5, y_shutter_base - 1, z_mid + 4],
    [shutter_x + 1.5, y_shutter_base - 4, z_btn_base + shelf_t + 1.0], [btn_xs[2] + 1.5, btn_y - btn_body / 2 - clr - 1.1, z_btn_base + shelf_t + 0.8]];
// power: charge module OUT+ -> slide switch middle leg; outer leg -> board 5V; OUT- -> board GND
sw_leg = function(k) [sw_x + (k - 1) * 2.0, H - wall - sw[1] - 0.5, z_mid];
pwr_in = [chg_pad(2), [chg_xy[0] - 2, chg_pad(2)[1], zb], [chg_xy[0] - 2, board_xy[1] + board[1] + 5.5, zb],
    [chg_xy[0] - 3, board_xy[1] + board[1] + 5.5, z_btn_base + shelf_t + 2.2], [sw_x + 3, 53.5, z_btn_base + shelf_t + 2.2],
    [sw_leg(1)[0], sw_leg(1)[1] - 2, z_mid + 2], sw_leg(1)];
pwr_out = [sw_leg(0), [sw_leg(0)[0], sw_leg(0)[1] - 2, z_mid + 2], [sw_x - 3, 53.5, z_btn_base + shelf_t + 2.6],
    [pad(1, 9)[0], board_xy[1] + board[1] + 2.5, z_board1 + 1.6], [pad(1, 9)[0], pad(1, 9)[1], z_board1 + 0.05]];
gnd_out = [chg_pad(3), [chg_xy[0] - 1, chg_pad(3)[1] - 1, zb], [chg_xy[0] + 4, board_xy[1] + 17, zb],
    [chg_xy[0] + 4, board_xy[1] + 17, z_deck1 + 0.5], [pad(0, 19)[0], board_xy[1] + 4, z_deck1 + 0.5], [pad(0, 19)[0], pad(0, 19)[1], z_board0 - 0.05]];
// battery level: B+ -> 100k -> node -> 100k -> GND; node -> GPIO1 (through the right-hand window)
res_x = chg_xy[0] + 4;
sense_top = [chg_pad(0), [res_x, chg_pad(0)[1] - 1, zb], [res_x, chg_xy[1] - 3.5, zb]];
sense_mid = [[res_x, chg_xy[1] - 7.8, zb], [res_x, chg_xy[1] - 9.2, zb]];
sense_gnd = [[res_x, chg_xy[1] - 13.5, zb], [res_x, board_xy[1] + 17, zb]];
sense_gpio = [[res_x, chg_xy[1] - 8.5, zb], [res_x + 4, chg_xy[1] - 8.5, zb], [res_x + 4, chg_xy[1] - 8.5, z_deck1 + 0.5],
    [pad(0, 16)[0], board_xy[1] + 4, z_deck1 + 0.5], [pad(0, 16)[0], pad(0, 16)[1], z_board0 - 0.05]];
module resistors() for (y0 = [chg_xy[1] - 3.5, chg_xy[1] - 9.2]) translate([res_x, y0, zb]) rotate([90, 0, 0]) cylinder(d = 1.9, h = 4.3);
// camera ribbon: out of the cradle, down to the board's camera socket
fpc = [board_xy[0] + 42, board_xy[1] + 8, 15];   // socket: x, y, length
module ribbon() {
    pts = [[lens_c[0], lens_c[1] - cam[1] / 2, z_fi - 2.5], [lens_c[0], lens_c[1] - cradle[1] / 2 - 1.5, z_fi - 1.2],
           [fpc[0] + 1.2, board_xy[1] + board[1] + 1, z_fi - 1.0], [fpc[0] + 1.2, fpc[1] + fpc[2] - 2, z_board1 + 2.6]];
    for (i = [0 : len(pts) - 2]) hull() {
        translate(pts[i]) cube([8, 0.3, 0.3], center = true);
        translate(pts[i + 1]) cube([8, 0.3, 0.3], center = true);
    }
}

// ---------------------------------------------------------------- fixings
module screw(len, head_down = false) {
    // M2 button head, head at z = 0, shaft along +z (or -z with head_down)
    m = head_down ? 1 : -1;
    scale([1, 1, -m]) { translate([0, 0, -1.2]) cylinder(d1 = 3.0, d2 = 3.8, h = 1.2); translate([0, 0, 0]) cylinder(d = 2, h = len); }
}
module insert() difference() { cylinder(d = 3.2, h = insert_len, $fn = 16); translate([0, 0, -1]) cylinder(d = 2, h = insert_len + 2, $fn = 12); }

// ---------------------------------------------------------------- dispatch
if (gpart == "frame") frame();
if (gpart == "sled") sled();
if (gpart == "back_plate") back_plate();
if (gpart == "front_plate") translate([0, 0, z_fi]) front_plate();
if (gpart == "lens_ring") translate([lens_c[0], lens_c[1], D]) lens_ring();
if (gpart == "vf_front") translate([vf_xy[0], vf_xy[1], D]) vf_frame();
if (gpart == "vf_back") translate([vf_xy[0], vf_xy[1], -1.2]) vf_frame();
if (gpart == "caps") for (x = btn_xs) translate([x, btn_y, z_bi + cap_flange[1]]) mirror([0, 0, 1]) button_cap();
if (gpart == "shutter_cap") translate([shutter_x, H - wall - cap_flange[1], z_mid]) rotate([-90, 0, 0]) shutter_cap();

if (gpart == "battery") zbox(bat_xy[0], bat_xy[1], bat[0], bat[1], z_mid - bat[2] / 2, z_mid + bat[2] / 2);
if (gpart == "foam") for (z = [z_bi + 0.05, z_mid + bat[2] / 2]) zbox(bat_xy[0] + 3, bat_xy[1] + 4, bat[0] - 6, bat[1] - 8, z, z + 0.9);
if (gpart == "board_pcb") difference() {
    zbox(board_xy[0], board_xy[1], board[0], board[1], z_board0, z_board1);
    for (h = board_holes) translate([h[0], h[1], z_board0 - 1]) cylinder(d = 3, h = 4);
}
if (gpart == "board_pads") pads();
if (gpart == "board_parts") {
    zbox(board_xy[0] + 5, board_xy[1] + 5, 25.5, 18, z_board1, z_board1 + 3.1);                  // ESP32-S3 module
    zbox(fpc[0], fpc[1], 3, fpc[2], z_board1, z_board1 + 2.2);                                   // camera socket
    zbox(board_xy[0] + board[0] - 7, board_xy[1] + board_usb_c - 4.5, 7.4, 9, z_board1, z_board1 + 3.2);   // USB-C
    zbox(board_xy[0] + 54, board_xy[1] + 6, 3, 4, z_board1, z_board1 + 1);                       // buttons on the board
}
if (gpart == "screen_pcb") difference() {
    zbox(scr_xy[0], scr_xy[1], scr[0], scr[1], z_deck0 - 1.6, z_deck0);
    for (h = scr_holes) translate([h[0], h[1], z_deck0 - 3]) cylinder(d = 3, h = 4);
}
if (gpart == "screen_glass") zbox(scr_win_xy[0] - 1.5, scr_win_xy[1] - 1.2, scr_active[0] + 3, scr_active[1] + 2.4, z_deck0 - scr_t, z_deck0 - 1.6);
if (gpart == "screen_active") zbox(scr_win_xy[0], scr_win_xy[1], scr_active[0], scr_active[1], z_deck0 - scr_t - 0.05, z_deck0 - scr_t + 0.1);
if (gpart == "charger_pcb") zbox(chg_xy[0], chg_xy[1], chg[0], chg[1], z_deck0 - 1.0, z_deck0);
if (gpart == "charger_parts") {
    zbox(chg_xy[0] + chg[0] - 7.3, chg_xy[1] + chg[1] / 2 - 4.5, 7.3, 9, z_deck0 - 1.0 - 3.2, z_deck0 - 1.0);   // USB-C
    zbox(chg_xy[0] + 4, chg_xy[1] + 3, 4, 4, z_deck0 - 1.0 - 1.0, z_deck0 - 1.0);                              // chip
    for (k = [0 : 3]) translate(chg_pad(k) + [0, 0, -0.05]) cylinder(d = 1.6, h = 0.2);
}
if (gpart == "camera") translate([lens_c[0], lens_c[1], 0]) {
    zbox(-cam[0] / 2, -cam[1] / 2, cam[0], cam[1], z_fi - cam[2], z_fi - 3);
    translate([0, 0, z_fi - cam[2]]) cylinder(d = lens_d, h = cam[2]);
}
if (gpart == "camera_glass") translate([lens_c[0], lens_c[1], z_fi - 0.4]) cylinder(d = lens_d - 2.4, h = 0.45);
if (gpart == "buttons") for (x = btn_xs) {
    zbox(x - btn_body / 2, btn_y - btn_body / 2, btn_body, btn_body, z_btn_base - 3.5, z_btn_base);
    translate([x, btn_y, z_bi + cap_flange[1]]) cylinder(d = 2.5, h = btn_h - 3.5);
}
if (gpart == "button_feet") for (x = btn_xs, sx = [-1, 1], sy = [-1, 1])
    zbox(x + sx * (btn_body / 2 + 0.6) - 0.6, btn_y + sy * 1.5 - 0.5, 1.2, 1.0, z_btn_base - 0.4, z_btn_base);
if (gpart == "shutter_sw") {
    zbox(shutter_x - btn_body / 2, y_shutter_base, btn_body, 3.5, z_mid - btn_body / 2, z_mid + btn_body / 2);
    translate([shutter_x, y_shutter_base + 3.5, z_mid]) rotate([-90, 0, 0]) cylinder(d = 2.5, h = btn_h - 3.5);
}
if (gpart == "slide_sw") {
    zbox(sw_x - sw[0] / 2, H - wall - sw[1], sw[0], sw[1], z_mid - sw[2] / 2, z_mid + sw[2] / 2);
    zbox(sw_x - sw_knob[0] / 2 - 0.5, H - wall, sw_knob[0], sw_knob[1] + wall - 0.4, z_mid - sw_knob[2] / 2, z_mid + sw_knob[2] / 2);
    for (k = [0 : 2]) translate(sw_leg(k) - [0.3, 2.5, 0.3]) cube([0.6, 2.5, 0.6]);
}
if (gpart == "inserts_back") for (s = screws) translate([s[0], s[1], z_bi]) insert();
if (gpart == "inserts_front") for (s = screws) translate([s[0], s[1], z_fi - insert_len]) insert();
if (gpart == "inserts_sled") for (p = sled_pts) translate([p[0], p[1], z_deck0 - insert_len]) insert();
if (gpart == "screws_back") for (s = screws) translate([s[0], s[1], 0]) screw(4);
if (gpart == "screws_front") for (s = screws) translate([s[0], s[1], D]) screw(4, true);
if (gpart == "screws_sled") for (p = sled_pts) translate([p[0], p[1], z_deck1]) screw(4, true);
if (gpart == "screws_board") for (h = board_holes) translate([h[0], h[1], z_board1]) screw(h[0] > W / 2 && h[1] > board_xy[1] + board[1] / 2 ? 3 : 4, true);
if (gpart == "screws_screen") for (h = scr_holes) translate([h[0], h[1], z_deck0 - 1.6]) screw(3);
if (gpart == "tape") zbox(chg_xy[0] + 2, chg_xy[1] + 1.5, chg[0] - 9, chg[1] - 3, z_deck0 - 0.01, z_deck0 + 0.01 + 0);

if (gpart == "w_screen_0") wire(scr_wire(0), 0.8);
if (gpart == "w_screen_1") wire(scr_wire(1), 0.8);
if (gpart == "w_screen_2") wire(scr_wire(2), 0.8);
if (gpart == "w_screen_3") wire(scr_wire(3), 0.8);
if (gpart == "w_screen_4") wire(scr_wire(4), 0.8);
if (gpart == "w_screen_5") wire(scr_wire(5), 0.8);
if (gpart == "w_screen_6") wire(scr_wire(6), 0.8);
if (gpart == "w_screen_7") wire(scr_wire(7), 0.8);
if (gpart == "w_bat_red") wire(bat_lead(1.4), 1.2);
if (gpart == "w_bat_black") wire(bat_lead(0), 1.2);
if (gpart == "w_buttons") { wire(btn_wire(0, 12)); wire(btn_wire(1, 13)); wire(btn_wire(2, 14)); wire(shutter_wire); }
if (gpart == "w_button_gnd") { wire(btn_gnd); wire(shutter_gnd); }
if (gpart == "w_power") { wire(pwr_in, 1.2); wire(pwr_out, 1.2); }
if (gpart == "w_gnd") wire(gnd_out, 1.2);
if (gpart == "w_sense") { wire(sense_top); wire(sense_mid); wire(sense_gnd); wire(sense_gpio); }
if (gpart == "resistors") resistors();
if (gpart == "ribbon") ribbon();
