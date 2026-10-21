/*
 * sim.c: renders every camera screen to a picture on the laptop, using the firmware's
 * own drawing code (../main/ui_draw.c, gfx.c). Run ./render.sh.
 *
 * The live picture is a real photo from the camera, converted to 320x240 RGB565 by
 * render.sh (photo.rgb565).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../main/gfx.h"
#include "../main/ui.h"
#include "qrcodegen.h"

static uint16_t fb[LCD_W * LCD_H];
static uint16_t photo[320 * 240];

static void save(const char *name)
{
    char path[128];
    snprintf(path, sizeof(path), "out/%s.ppm", name);
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6 %d %d 255\n", LCD_W, LCD_H);
    for (int i = 0; i < LCD_W * LCD_H; i++) {
        uint16_t v = (uint16_t)(fb[i] << 8 | fb[i] >> 8);
        unsigned char px[3] = { (v >> 11) * 255 / 31, ((v >> 5) & 63) * 255 / 63, (v & 31) * 255 / 31 };
        fwrite(px, 1, 3, f);
    }
    fclose(f);
}

static ui_state_t base(void)
{
    ui_state_t s;
    memset(&s, 0, sizeof(s));
    s.view = UI_PREVIEW;
    s.looks = 6; s.look = 2;
    s.sd = true; s.left = 812;
    s.batt = true; s.batt_pct = 78;
    return s;
}

static void qr(ui_state_t *s, const char *text)
{
    uint8_t tmp[qrcodegen_BUFFER_LEN_FOR_VERSION(6)], code[qrcodegen_BUFFER_LEN_FOR_VERSION(6)];
    if (!qrcodegen_encodeText(text, tmp, code, qrcodegen_Ecc_LOW, 1, 6, qrcodegen_Mask_AUTO, true)) return;
    s->qr_size = qrcodegen_getSize(code);
    for (int y = 0; y < s->qr_size; y++)
        for (int x = 0; x < s->qr_size; x++) s->qr[y * s->qr_size + x] = qrcodegen_getModule(code, x, y);
}

static void row(ui_state_t *s, const char *const *icon, const char *label, const char *value)
{
    ui_row_t *r = &s->rows[s->nrows++];
    r->icon = icon;
    snprintf(r->label, sizeof(r->label), "%s", label);
    snprintf(r->value, sizeof(r->value), "%s", value);
}

static void settings_rows(ui_state_t *s)
{
    snprintf(s->title, sizeof(s->title), "Settings");
    row(s, ICON_TIMER, "Self-timer", "Off");
    row(s, ICON_GRID, "Grid", "On");
    row(s, ICON_SUN, "Brightness", "Mid");
    row(s, ICON_MOON, "Sleep after", "2 min");
    row(s, ICON_CARD, "Format card", "");
    row(s, ICON_INFO, "About", "");
}

int main(void)
{
    gfx_set_framebuffer(fb);
    FILE *f = fopen("photo.rgb565", "rb");
    if (!f || fread(photo, 2, 320 * 240, f) != 320 * 240) { fprintf(stderr, "photo.rgb565 missing\n"); return 1; }
    fclose(f);
    gfx_set_image(photo, 320, 240, UI_IMG_X, 0, UI_IMG_W, LCD_H);

    ui_overlay_t o = { .have_image = true, .now_ms = 0 };
    ui_overlay_t bare = { .have_image = false };
    ui_state_t s;

    s = base(); s.view = UI_BOOT; snprintf(s.line1, 32, "hi, maya");
    ui_draw(&s, &bare); save("01-boot");

    s = base();
    ui_draw(&s, &o); save("02-live-photo");

    { ui_overlay_t t = o; t.toast = "Warm film"; s = base(); s.look = 3; ui_draw(&s, &t); save("03-look-changed"); }

    s = base(); s.grid = true; s.timer_s = 3;
    ui_draw(&s, &o); save("04-grid-and-timer");

    s = base(); s.timer_s = 3; s.countdown = 2;
    ui_draw(&s, &o); save("05-self-timer");

    { ui_overlay_t t = o; t.flash = true; s = base(); ui_draw(&s, &t); save("06-shutter-flash"); }

    s = base(); s.busy = true;
    ui_draw(&s, &o); save("07-saving");

    { ui_overlay_t t = o; t.toast = "DSC_0042"; s = base(); ui_draw(&s, &t); save("08-saved"); }

    s = base(); s.video = true; s.left = 41;
    ui_draw(&s, &o); save("09-live-video");

    s = base(); s.video = true; s.rec = true; s.rec_ms = 83000; s.left = 39;
    ui_draw(&s, &o); save("10-recording");

    s = base(); s.view = UI_MODE; s.mode_sel = 1;
    ui_draw(&s, &o); save("11-mode-dial");

    s = base(); s.view = UI_GALLERY; s.g_pos = 2; s.g_count = 27;
    ui_draw(&s, &o); save("12-gallery-photo");

    s = base(); s.view = UI_GALLERY; s.g_pos = 0; s.g_count = 27; s.g_video = true;
    ui_draw(&s, &o); save("13-gallery-video");

    s = base(); s.view = UI_GALLERY; s.g_pos = 2; s.g_count = 27; s.g_confirm = true; snprintf(s.g_name, 16, "DSC_0040.JPG");
    ui_draw(&s, &o); save("14-delete");

    s = base(); s.view = UI_GALLERY; s.g_count = 0;
    ui_draw(&s, &bare); save("15-gallery-empty");

    s = base(); s.view = UI_SETTINGS; settings_rows(&s); s.sel = 1;
    ui_draw(&s, &bare); save("16-settings");

    s = base(); s.view = UI_SETTINGS; settings_rows(&s); s.sel = 4; s.dialog = true;
    ui_draw(&s, &bare); save("17-format");

    s = base(); s.view = UI_SETTINGS; s.sel = -1; snprintf(s.title, 16, "About");
    row(&s, NULL, "Firmware", "0.3.0"); row(&s, NULL, "Camera", "OV3660"); row(&s, NULL, "Card free", "29.6 GB");
    row(&s, NULL, "Photos", "27"); row(&s, NULL, "Videos", "3"); row(&s, NULL, "Battery", "3.95 V");
    ui_draw(&s, &bare); save("18-about");

    s = base(); s.view = UI_MESSAGE; snprintf(s.line1, 32, "Camera not found"); snprintf(s.line2, 32, "check the ribbon cable");
    ui_draw(&s, &bare); save("19-no-camera");

    s = base(); s.view = UI_MODE; s.mode_sel = 2;
    ui_draw(&s, &o); save("21-mode-phone");

    s = base(); s.view = UI_PHONE; s.phone_step = 1; snprintf(s.ssid, 33, "digicam-maya"); snprintf(s.pass, 16, "kp7wq3ra");
    qr(&s, "WIFI:T:WPA;S:digicam-maya;P:kp7wq3ra;;");
    ui_draw(&s, &bare); save("22-phone-join");

    { ui_overlay_t t = bare; t.toast = "sent DSC_0012";
      s = base(); s.view = UI_PHONE; s.phone_step = 2; s.clients = 1; qr(&s, "http://192.168.4.1/");
      ui_draw(&s, &t); save("23-phone-open"); }

    s = base(); s.sd = false; s.batt_pct = 8;
    { ui_overlay_t t = o; t.toast = "no SD card"; ui_draw(&s, &t); } save("20-no-card-low-battery");
    return 0;
}
