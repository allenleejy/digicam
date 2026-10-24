// Every piece of the v3 interactive assembly guide (guide-v3/), one per export, in model space
// (X left to right seen from the back, Y up, Z back to front), placed as assembled.
//   openscad -D 'style="retro"' -D 'gpart="frame"' -o frame.stl guide_parts_v3.scad
// export_guide_v3.py exports them all for both bodies and bundles them into guide-v3/models.json.

include <camera1_v3.scad>
part = "none";          // the included model draws nothing by itself
gpart = "frame";
$fn = 32;

module wire(pts, d = 0.9) {
    for (i = [0 : len(pts) - 2]) hull() {
        translate(pts[i]) sphere(d = d, $fn = 8);
        translate(pts[i + 1]) sphere(d = d, $fn = 8);
    }
}
module insert() { difference() { cylinder(d = 3.5, h = insert_len); translate([0, 0, -1]) cylinder(d = 2, h = insert_len + 2); } }
module screw(len_) {   // M2 button head pointing along +Z, head at z = 0
    translate([0, 0, -1.2]) cylinder(d = 3.6, h = 1.2);
    cylinder(d = 2, h = len_);
}

// ---------------------------------------------------------------- where the wires run
lane_y = bat_xy[1] + bat[1] + 1.8;                  // above the battery
z_front_w = z_chg0 + chg_t - 0.6;                   // in front of the battery, beside the charge module
z_pad = z_board1 + 0.4;                             // on top of the board's pads
row_top = board_xy[1] + board[1] - 1.27;            // the board's two pad rows
row_bot = board_xy[1] + 1.27;
function pad_x(i) = board_xy[0] + 6.2 + i * 2.54;   // pad i along a row (positions are illustrative)
chg_pad = function(k) [chg_xy[0] + chg[0] - 1.6, chg_xy[1] + 1.8 + k * 2.7, z_chg0 + 1.1];   // B+, B-, OUT+, OUT-
sw_leg = function(k) [sw_x + (k - 1) * 2.5, (retro ? H + tp_h - tp_ceiling - sw[1] : H - wall - sw[1]) - 0.2, z_mid];
sh_base = retro ? [shutter_x, H - 0.2, z_mid] : [shutter_x, shutter_base() - 1.6, z_mid];

function lead(k, dx) = [   // battery lead to B+ (k=0) or B- (k=1)
    [bat_xy[0] + bat[0] - 6 + dx, bat_xy[1] + bat[1] - 0.3, z_bat0 + bat[2] / 2],
    [bat_xy[0] + bat[0] - 6 + dx, lane_y + dx * 0.3, z_bat0 + bat[2] / 2],
    [bat_xy[0] + bat[0] - 6 + dx, lane_y + dx * 0.3, z_front_w],
    [chg_pad(k)[0] + 1.5, lane_y + dx * 0.3, z_front_w],
    [chg_pad(k)[0] + 1.5, chg_pad(k)[1], z_front_w], chg_pad(k)];
function to_board(start, lane_dy, px, row) = [   // up to the lane above the battery, across, down to a pad
    start, [start[0], lane_y + lane_dy, start[2]], [start[0], lane_y + lane_dy, z_front_w],
    [px, lane_y + lane_dy, z_front_w], [px, row + 0.6, z_front_w], [px, row, z_pad]];

module part_named(n) {
    // ---- the case
    if (n == "frame") frame();
    if (n == "sled") sled();
    if (n == "carrier") carrier();
    if (n == "back_plate") back_plate();
    if (n == "front_plate") translate([0, 0, z_fi]) front_plate();
    if (n == "caps") for (i = [0 : 2]) translate([btn_xs[i], btn_y, cap_top]) button_cap(i);
    if (n == "shutter_cap") ghost("shuttercap");
    if (n == "knob") ghost("knob");
    if (n == "lens_trim") { if (retro) translate([lens_c[0], lens_c[1], D]) lens_barrel(); else translate([lens_c[0], lens_c[1], D]) lens_ring(); }
    if (n == "panel" && !retro) translate([0, 0, 0.4]) panel();
    if (n == "bezel" && !retro) placed_bezel();
    if (n == "top_plate" && retro) top_plate();
    if (n == "top_bits" && retro) top_bits_placed();
    // plain panels here: the grain is too many triangles for the page
    if (n == "covering_front" && retro) translate([0, 0, D]) linear_extrude(bump) leatherette_2d();
    if (n == "covering_back" && retro) translate([0, 0, -bump]) linear_extrude(bump) back_leatherette_2d();
    if (n == "windows" && retro) for (w = tp_wins)
        if (w[4] == "front") zbox(w[0] + 0.15, H + w[1] + 0.15, w[2] - 0.3, w[3] - 0.3, D + 1.2 - 0.8 + 0.02, D + 1.2 - 0.02);
        else zbox(w[0] + 0.15, H + w[1] + 0.15, w[2] - 0.3, w[3] - 0.3, 0.02, 0.78);

    // ---- electronics
    if (n == "battery") ghost("battery");
    if (n == "foam") zbox(bat_xy[0] + 3, bat_xy[1] + 4, bat[0] - 6, bat[1] - 8, z_bi + 0.02, z_bat0);
    if (n == "board_pcb") difference() {
        zbox(board_xy[0], board_xy[1], board[0], board[1], z_board0, z_board1);
        for (h = board_holes) zcyl(h, 2.4, z_board0 - 1, z_board1 + 1);
    }
    if (n == "board_parts") {
        zbox(board_xy[0] + 6, board_xy[1] + 5, 18, board[1] - 10, z_board1, z_board1 + board_top);          // ESP32 module
        for (y = board_usb_y) zbox(board_xy[0] + board[0] - 7.4, board_xy[1] + y - 4.4, 7.9, 8.8, z_board1, z_board1 + 3.2);
        zbox(board_xy[0] + 8, board_xy[1] + 4, 16, board[1] - 8, z_board0 - board_under + 0.05, z_board0);  // microSD socket
        zbox(board_xy[0] + 28, board_xy[1] + 9, 6, 6, z_board1, z_board1 + 1.2);                             // the other chips
        zbox(board_xy[0] + 38, board_xy[1] + 8, 4, 4, z_board1, z_board1 + 1.0);
    }
    if (n == "board_pads") for (i = [0 : 19], y = [row_top, row_bot]) translate([pad_x(i), y, z_board1 - 0.01]) cylinder(d = 1.7, h = 0.08);
    if (n == "camera") translate([lens_c[0], lens_c[1], 0]) {
        zbox(-cam_holder / 2 - 0.5, -cam_holder / 2 - 2, cam_holder + 1, cam_holder + 2.5, z_cam0, z_cam0 + cam_pcb_t);
        zbox(-cam_holder / 2, -cam_holder / 2, cam_holder, cam_holder, z_cam0 + cam_pcb_t, z_lens - cam_barrel[1]);
        translate([0, 0, z_lens - cam_barrel[1] - 0.01]) cylinder(d = cam_barrel[0], h = cam_barrel[1] - 0.3);
    }
    if (n == "camera_glass") translate([lens_c[0], lens_c[1], z_lens - 0.32]) cylinder(d = cam_barrel[0] - 2.4, h = 0.3);
    if (n == "camera_tape") translate([lens_c[0], lens_c[1], 0]) zbox(-3.5, -3.5, 7, 7, z_board1 + board_top, z_cam0);
    if (n == "ribbon") {   // folded back from the camera onto the board's socket, beside the module
        y0 = lens_c[1] - cam_holder / 2 - 2;
        x0 = lens_c[0];
        wire([[x0, y0, z_cam0 + 0.4], [x0, y0 - 2.4, z_cam0 + 0.2], [x0, y0 - 3.6, z_cam0 - 1.4],
              [x0, y0 - 2.6, z_board1 + 1.0], [x0 + 6, y0 - 1.0, z_board1 + 0.6]], 4.0);
    }
    if (n == "screen_pcb") difference() {
        zbox(scr_xy[0], scr_xy[1], scr[0], scr[1], z_bi + 2.0, z_bi + scr_t - 0.5);
        for (h = scr_holes) zcyl(h, 2.4, z_bi, z_bi + scr_t);
    }
    if (n == "screen_glass") zbox(win_xy[0] - 2.0, win_xy[1] - 1.6, scr_active[0] + 4.0, scr_active[1] + 3.2, z_bi + 0.02, z_bi + 2.0);
    if (n == "screen_active") zbox(win_xy[0], win_xy[1], scr_active[0], scr_active[1], z_bi - 0.02, z_bi + 0.1);
    if (n == "screen_pad") zbox(scr_xy[0] + 8, scr_xy[1] + 8, scr[0] - 16, scr[1] - 16, z_bi + scr_t - 0.5, z_deck0);
    if (n == "charger_pcb") zbox(chg_xy[0], chg_xy[1], chg[0], chg[1], z_chg0, z_chg0 + 1.0);
    if (n == "charger_parts") {
        zbox(chg_xy[0] - 0.4, chg_xy[1] + chg[1] / 2 - 4.5, 7.4, 9.0, z_chg0 + 1.0, z_chg0 + chg_t);      // USB-C
        zbox(chg_xy[0] + 9, chg_xy[1] + 3, 4, 4, z_chg0 + 1.0, z_chg0 + 2.0);
        for (k = [0 : 3]) translate(chg_pad(k) + [0, 0, -0.05]) cylinder(d = 1.6, h = 0.1);
    }
    if (n == "charger_tape") zbox(chg_xy[0] + 1, chg_xy[1] + 1, chg[0] - 2, chg[1] - 2, z_bat0 + bat[2], z_chg0);
    if (n == "buttons") ghost("buttons");
    if (n == "shutter_sw") ghost("shutter");
    if (n == "slide_sw") {
        ghost("switch");
        for (k = [0 : 2]) translate(sw_leg(k)) translate([-0.3, -2.4, -0.3]) cube([0.6, 2.4, 0.6]);
    }

    // ---- inserts and screws
    if (n == "inserts") {
        for (p = posts) { translate([p[0], p[1], z_bi]) insert(); translate([p[0], p[1], z_fi - insert_len]) insert(); }
        for (q = sled_pts) translate([q[0], q[1], z_deck0 - insert_len]) insert();
        if (retro) for (x = tp_screws) translate([x, H, z_mid]) rotate([90, 0, 0]) insert();
    }
    if (n == "screws_back") for (p = posts) translate([p[0], p[1], 0]) screw(5);
    if (n == "screws_front") for (p = posts) translate([p[0], p[1], D]) mirror([0, 0, 1]) screw(5);
    if (n == "screws_sled") for (q = sled_pts) translate([q[0], q[1], z_deck1]) mirror([0, 0, 1]) screw(4);
    if (n == "screws_board") for (h = board_holes) translate([h[0], h[1], z_board1]) mirror([0, 0, 1]) screw(5);
    if (n == "screws_carrier") for (b = carrier_bosses) translate([b[0], b[1], z_btn_base + carrier_t]) mirror([0, 0, 1]) screw(6);
    if (n == "screws_top" && retro) for (x = tp_screws) translate([x, H + tp_h, z_mid]) rotate([-90, 0, 0]) screw(13);
    if (n == "screws_barrel" && retro) for (a = [90, 210, 330])
        translate([lens_c[0] + barrel_screw_r * cos(a), lens_c[1] + barrel_screw_r * sin(a), z_fi]) mirror([0, 0, 1]) screw(5);

    // ---- wires
    if (n == "w_bat_red") wire(lead(0, 0), 1.3);
    if (n == "w_bat_black") wire(lead(1, 1.4), 1.3);
    if (n == "w_out_plus") wire([chg_pad(2), [chg_pad(2)[0] + 3, chg_pad(2)[1], z_front_w], [chg_pad(2)[0] + 3, lane_y + 1.0, z_front_w],
                                 [sw_leg(1)[0], lane_y + 1.0, z_front_w], [sw_leg(1)[0], lane_y + 1.0, z_mid], sw_leg(1) - [0, 2.4, 0]], 1.3);
    if (n == "w_5v") wire(to_board(sw_leg(2) - [0, 2.4, 0], 1.5, pad_x(0), row_top), 1.3);
    if (n == "w_gnd") wire([chg_pad(3), [chg_pad(3)[0] + 4.5, chg_pad(3)[1], z_front_w], [chg_pad(3)[0] + 4.5, lane_y + 2.0, z_front_w],
                            [pad_x(1), lane_y + 2.0, z_front_w], [pad_x(1), row_top + 0.6, z_front_w], [pad_x(1), row_top, z_pad]], 1.3);
    if (n == "w_sense") wire([chg_pad(0), [chg_pad(0)[0] + 6, chg_pad(0)[1], z_front_w], [chg_pad(0)[0] + 6, lane_y + 2.6, z_front_w],
                              [pad_x(3) - 6, lane_y + 2.6, z_front_w], [pad_x(3) - 6, row_top + 3.5, z_pad + 1.2]]);
    if (n == "resistors") {
        hull() { translate([pad_x(3) - 6, row_top + 3.5, z_pad + 1.2]) sphere(d = 1.8); translate([pad_x(3), row_top + 3.5, z_pad + 1.2]) sphere(d = 1.8); }
        wire([[pad_x(3), row_top + 3.5, z_pad + 1.2], [pad_x(3), row_top, z_pad]], 0.6);
        hull() { translate([pad_x(3), row_top + 1.5, z_pad + 2.2]) sphere(d = 1.8); translate([pad_x(5), row_top + 1.5, z_pad + 2.2]) sphere(d = 1.8); }
        wire([[pad_x(5), row_top + 1.5, z_pad + 2.2], [pad_x(5), row_top, z_pad]], 0.6);
    }
    if (n == "w_buttons") for (i = [0 : 2])
        wire([[btn_xs[i] - btn_feet / 2 + 0.6, btn_y - 3.0, z_btn_base + carrier_t + 0.4], [btn_xs[i] - btn_feet / 2 + 0.6, btn_y - 3.0, z_btn_base + carrier_t + 1.4],
              [btn_xs[i] - btn_feet / 2 + 0.6, row_top + 1.0, z_btn_base + carrier_t + 1.4], [pad_x(8 + i * 2), row_top + 1.0, z_pad + 0.6], [pad_x(8 + i * 2), row_top, z_pad]]);
    if (n == "w_button_gnd") {
        wire([for (i = [0 : 2]) [btn_xs[i] + btn_feet / 2 - 0.6, btn_y - 3.0, z_btn_base + carrier_t + 0.9]]);
        wire([[btn_xs[2] + btn_feet / 2 - 0.6, btn_y - 3.0, z_btn_base + carrier_t + 0.9], [btn_xs[2] + 6, row_top + 1.0, z_btn_base + carrier_t + 1.4],
              [pad_x(15), row_top + 1.0, z_pad + 0.6], [pad_x(15), row_top, z_pad]]);
    }
    if (n == "w_shutter") {
        if (retro) for (k = [0, 1]) wire([sh_base + [k * 1.6 - 0.8, 0, 0], sh_base + [k * 1.6 - 0.8, -4, 0],
                                          [shutter_x - 2 + k * 1.6, row_top + 1.2 + k, z_front_w], [pad_x(17 + k), row_top + 0.6, z_front_w], [pad_x(17 + k), row_top, z_pad]]);
        else for (k = [0, 1]) wire(to_board(sh_base + [k * 1.6 - 0.8, 0, 0], 3.0 + k * 0.6, pad_x(4 + k), row_top));
    }
    for (i = [0 : 7]) if (n == str("w_screen_", i)) {   // pins at the screen's right-hand end, through the sled's slot, up through pads
        y_pin = scr_xy[1] + scr[1] / 2 - 8.75 + i * 2.5;
        x_pin = scr_xy[0] + scr[0] - 2.3;
        y_row = i < 4 ? row_bot : row_top;
        x_end = pad_x(i < 4 ? 15 + i : 15 + i - 4);
        wire([[x_pin, y_pin, z_bi + scr_t - 0.6], [x_pin, y_pin, z_deck1 + 0.7], [x_end, y_pin, z_deck1 + 0.7],
              [x_end, y_row, z_deck1 + 0.7], [x_end, y_row, z_pad]], 0.8);
    }
}
part_named(gpart);
