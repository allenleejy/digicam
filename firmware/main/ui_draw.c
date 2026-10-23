/*
 * ui_draw.c: every screen, drawn from a ui_state_t. Plain C (no ESP-IDF), so ../sim
 * can render the same screens on the laptop.
 */

#include "ui.h"
#include "gfx.h"
#include "phone.h"     /* PHONE_HOST, for the address on the Phone mode screen */

#include <stdio.h>
#include <string.h>

#define BAR_L (UI_IMG_X / 2)                       /* centre of the left column */
#define BAR_R (UI_IMG_X + UI_IMG_W + UI_IMG_X / 2) /* centre of the right column */
#define IMG_CX (UI_IMG_X + UI_IMG_W / 2)

/* ---------------------------------------------------------------- small pieces */

/* the button glyphs used in hints: ◀ ▶ and ● */
static int glyph_lr(int x, int y, uint16_t c)
{
    gfx_triangle(x, y + 3, 10, false, c);
    gfx_triangle(x + 9, y + 3, 10, true, c);
    return x + 15;
}

static int glyph_ok(int x, int y, uint16_t c)
{
    gfx_circle(x + 3, y + 8, 3, c);
    return x + 9;
}

static void toast(const char *text)
{
    int w = gfx_text_width(SMALL, text) + 16;
    if (w > UI_IMG_W - 6) w = UI_IMG_W - 6;
    int x = IMG_CX - w / 2, y = LCD_H - 24;
    gfx_fill_round(x, y, w, 19, 9, C_PANEL);
    gfx_text_centre(SMALL, IMG_CX, y + 2, text, C_WHITE, -1);
}

static void battery(int cx, int y, const ui_state_t *s)
{
    if (!s->batt) {
        gfx_text_centre(SMALL, cx, y, "USB", C_GREY, -1);
        return;
    }
    int x = cx - 12;
    gfx_frame(x, y, 22, 11, C_WHITE);
    gfx_fill(x + 22, y + 3, 2, 5, C_WHITE);
    uint16_t c = s->batt_pct <= 10 ? C_RED : s->batt_pct <= 25 ? C_ORANGE : C_GREEN;
    gfx_fill(x + 2, y + 2, (18 * s->batt_pct + 50) / 100, 7, c);
    char pct[16];
    snprintf(pct, sizeof(pct), "%d", s->batt_pct);
    gfx_text_centre(SMALL, cx, y + 14, pct, C_WHITE, -1);
}

/* rule of thirds over the picture, dotted so it doesn't hide the subject */
static void grid(void)
{
    for (int k = 1; k <= 2; k++) {
        int x = UI_IMG_X + UI_IMG_W * k / 3, y = LCD_H * k / 3;
        for (int i = 0; i < LCD_H; i += 3) gfx_fill(x, i, 1, 1, C_WHITE);
        for (int i = UI_IMG_X; i < UI_IMG_X + UI_IMG_W; i += 3) gfx_fill(i, y, 1, 1, C_WHITE);
    }
}

/* a centred box with a title, a line and an action hint (delete / format) */
static void dialog(const char *title, const char *line, const char *action, uint16_t accent)
{
    int w = 208, h = 88, x = (LCD_W - w) / 2, y = (LCD_H - h) / 2;
    gfx_fill_round(x - 1, y - 1, w + 2, h + 2, 9, accent);
    gfx_fill_round(x + 1, y + 1, w - 2, h - 2, 8, C_PANEL);
    gfx_text_centre(SMALL, LCD_W / 2, y + 10, title, C_WHITE, -1);
    gfx_text_centre(SMALL, LCD_W / 2, y + 30, line, C_GREY, -1);
    /* "● delete   other: keep" */
    int tw = 9 + gfx_text_width(SMALL, action) + 24 + gfx_text_width(SMALL, "other: keep");
    int hx = LCD_W / 2 - tw / 2, hy = y + 60;
    hx = glyph_ok(hx, hy, accent);
    hx = gfx_text(SMALL, hx + 2, hy, action, C_WHITE, -1);
    gfx_text(SMALL, hx + 22, hy, "other: keep", C_GREY, -1);
}

/* ---------------------------------------------------------------- screens */

static void draw_boot(const ui_state_t *s)
{
    gfx_text_centre(BIG, LCD_W / 2, 34, "digicam", C_LILAC, -1);
    gfx_circle(LCD_W / 2 + 62, 40, 3, C_ORANGE);
    gfx_text_centre(SMALL, LCD_W / 2, 78, s->line1, C_WHITE, -1);
}

static void draw_message(const ui_state_t *s)
{
    gfx_icon(ICON_CAMERA, LCD_W / 2 - 12, 22, 2, C_ORANGE);
    gfx_text_centre(SMALL, LCD_W / 2, 60, s->line1, C_WHITE, -1);
    gfx_text_centre(SMALL, LCD_W / 2, 82, s->line2, C_GREY, -1);
}

static void draw_preview(const ui_state_t *s, const ui_overlay_t *o)
{
    if (s->grid && !s->busy) grid();

    /* left column: mode, one dot per look, self-timer, card warning */
    gfx_icon(s->video ? ICON_VIDEO : ICON_CAMERA, BAR_L - 6, 5, 1, s->video ? C_ORANGE : C_WHITE);
    int top = 30, step = 11;
    for (int i = 0; i < s->looks; i++) {
        if (i == s->look) gfx_circle(BAR_L, top + i * step, 3, C_LILAC);
        else gfx_circle(BAR_L, top + i * step, 1, C_DIM);
    }
    if (!s->sd) {
        gfx_text_centre(SMALL, BAR_L, LCD_H - 20, "SD", C_RED, -1);
    } else if (s->timer_s && !s->video) {
        gfx_icon(ICON_TIMER, BAR_L - 6, LCD_H - 34, 1, C_LILAC);
        char t[12];
        snprintf(t, sizeof(t), "%d", s->timer_s);
        gfx_text_centre(SMALL, BAR_L, LCD_H - 20, t, C_LILAC, -1);
    }

    /* right column: battery, then shots or minutes left */
    battery(BAR_R, 5, s);
    if (s->sd) {
        char left[16];
        if (s->video) snprintf(left, sizeof(left), "%dm", s->left > 99 ? 99 : s->left);
        else snprintf(left, sizeof(left), "%d", s->left > 999 ? 999 : s->left);
        gfx_text_centre(SMALL, BAR_R, LCD_H - 20, left, C_GREY, -1);
    }

    /* recording: blinking dot and the time */
    if (s->rec) {
        char t[12];
        snprintf(t, sizeof(t), "%02lu:%02lu", (unsigned long)(s->rec_ms / 60000), (unsigned long)(s->rec_ms / 1000 % 60));
        gfx_fill_round(UI_IMG_X + 4, 4, 64, 19, 9, C_PANEL);
        if ((o->now_ms / 500) % 2 == 0) gfx_circle(UI_IMG_X + 14, 13, 4, C_RED);
        gfx_text(SMALL, UI_IMG_X + 23, 6, t, C_WHITE, -1);
    }

    /* self-timer counting down */
    if (s->countdown > 0) {
        char n[12];
        snprintf(n, sizeof(n), "%d", s->countdown);
        gfx_circle(IMG_CX, LCD_H / 2, 26, C_PANEL);
        gfx_text_centre(BIG, IMG_CX, LCD_H / 2 - 16, n, C_ORANGE, -1);
    }
}

static void draw_mode(const ui_state_t *s)
{
    static const char *const names[UI_MODES] = { "Photo", "Video", "Phone", "Settings" };
    const char *const *icons[UI_MODES] = { ICON_CAMERA, ICON_VIDEO, ICON_PHONE, ICON_GEAR };
    gfx_shade(0, 0, LCD_W, LCD_H, 170);
    int tw = 50, th = 50, gap = 7, x0 = (LCD_W - (UI_MODES * tw + (UI_MODES - 1) * gap)) / 2, y = 18;
    for (int i = 0; i < UI_MODES; i++) {
        int x = x0 + i * (tw + gap);
        bool on = i == s->mode_sel;
        gfx_fill_round(x, y, tw, th, 8, on ? C_LILAC : C_PANEL);
        gfx_icon(icons[i], x + tw / 2 - 12, y + th / 2 - 12, 2, on ? C_PANEL : C_GREY);
    }
    gfx_text_centre(SMALL, LCD_W / 2, y + th + 8, names[s->mode_sel], C_WHITE, -1);
    int hx = LCD_W / 2 - 66, hy = 108;
    hx = glyph_lr(hx, hy, C_GREY);
    hx = gfx_text(SMALL, hx + 3, hy, "choose", C_GREY, -1);
    hx = glyph_ok(hx + 16, hy, C_GREY);
    gfx_text(SMALL, hx + 3, hy, "ok", C_GREY, -1);
}

/* Phone mode: QR code on the left, what to do on the right */
static void draw_phone(const ui_state_t *s, const ui_overlay_t *o)
{
    gfx_clear(C_PANEL);
    int box = 0;
    if (s->qr_size) {
        int quiet = 2, mod = (LCD_H - 16) / (s->qr_size + 2 * quiet);
        if (mod > 3) mod = 3;
        if (mod < 1) mod = 1;
        box = (s->qr_size + 2 * quiet) * mod;
        int x0 = 8, y0 = (LCD_H - box) / 2;
        gfx_fill_round(x0, y0, box, box, 4, C_WHITE);
        for (int y = 0; y < s->qr_size; y++)
            for (int x = 0; x < s->qr_size; x++)
                if (s->qr[y * s->qr_size + x]) gfx_fill(x0 + (quiet + x) * mod, y0 + (quiet + y) * mod, mod, mod, C_BLACK);
    }
    int tx = 8 + box + 10, ty = 8;
    char line[40];
    if (s->phone_step <= 1) {
        gfx_text(SMALL, tx, ty, "1 Scan to join", C_LILAC, -1);
        gfx_text(SMALL, tx, ty + 22, "Wi-Fi", C_GREY, -1);
        gfx_text(SMALL, tx, ty + 38, s->ssid, C_WHITE, -1);
        gfx_text(SMALL, tx, ty + 58, "password", C_GREY, -1);
        gfx_text(SMALL, tx, ty + 74, s->pass, C_WHITE, -1);
    } else {
        gfx_text(SMALL, tx, ty, "2 Scan to see", C_LILAC, -1);
        gfx_text(SMALL, tx, ty + 16, "  the photos", C_LILAC, -1);
        gfx_text(SMALL, tx, ty + 38, "or open", C_GREY, -1);
        gfx_text(SMALL, tx, ty + 54, PHONE_HOST ".local", C_WHITE, -1);
        snprintf(line, sizeof(line), s->clients == 1 ? "1 phone on" : "%d phones on", s->clients);
        gfx_circle(tx + 3, ty + 82, 3, C_GREEN);
        gfx_text(SMALL, tx + 10, ty + 74, line, C_GREEN, -1);
    }
    if (o->toast) gfx_text(SMALL, tx, LCD_H - 34, o->toast, C_ORANGE, -1);
    gfx_text(SMALL, tx, LCD_H - 18, "shutter: stop", C_DIM, -1);
}

static void draw_gallery(const ui_state_t *s)
{
    if (s->g_count == 0) {
        gfx_icon(ICON_CAMERA, IMG_CX - 12, 30, 2, C_DIM);
        gfx_text_centre(SMALL, IMG_CX, 64, "No photos yet", C_WHITE, -1);
        gfx_text_centre(SMALL, IMG_CX, 84, "press the shutter", C_GREY, -1);
        return;
    }
    char pos[16], count[16];
    snprintf(pos, sizeof(pos), "%d", s->g_pos + 1);
    snprintf(count, sizeof(count), s->g_count < 100 ? "/%d" : "%d", s->g_count);
    gfx_text_centre(SMALL, BAR_L, 4, pos, C_WHITE, -1);
    gfx_text_centre(SMALL, BAR_L, 20, count, C_GREY, -1);
    gfx_triangle(BAR_L - 4, LCD_H / 2 - 6, 12, false, C_DIM);
    gfx_triangle(BAR_R - 2, LCD_H / 2 - 6, 12, true, C_DIM);
    if (s->g_video) {
        gfx_icon(ICON_VIDEO, BAR_R - 6, 6, 1, C_ORANGE);
        if (!s->g_playing) {
            gfx_circle(IMG_CX, LCD_H / 2, 17, C_PANEL);
            gfx_triangle(IMG_CX - 4, LCD_H / 2 - 9, 18, true, C_WHITE);
        }
    }
    if (s->g_confirm) dialog("Delete this?", s->g_name, "delete", C_RED);
}

static void draw_settings(const ui_state_t *s)
{
    gfx_clear(C_PANEL);
    gfx_text(SMALL, 8, 2, s->title, C_LILAC, -1);
    gfx_fill(0, 19, LCD_W, 1, C_DARK);

    const int visible = 5, row_h = 19, y0 = 22;
    int first = s->sel - 2;
    if (first > s->nrows - visible) first = s->nrows - visible;
    if (first < 0) first = 0;
    for (int k = 0; k < visible && first + k < s->nrows; k++) {
        const ui_row_t *r = &s->rows[first + k];
        int y = y0 + k * row_h;
        bool on = first + k == s->sel;
        if (on) gfx_fill_round(4, y, LCD_W - 8, row_h - 1, 6, C_LILAC);
        uint16_t fg = on ? C_PANEL : C_WHITE, fg2 = on ? C_PANEL : C_GREY;
        if (r->icon) gfx_icon(r->icon, 10, y + 3, 1, on ? C_PANEL : C_LILAC);
        gfx_text(SMALL, r->icon ? 28 : 12, y + 1, r->label, fg, -1);
        gfx_text(SMALL, LCD_W - 12 - gfx_text_width(SMALL, r->value), y + 1, r->value, fg2, -1);
    }
    /* scroll position */
    if (s->nrows > visible) {
        int track = visible * row_h, bar = track * visible / s->nrows;
        int by = y0 + (track - bar) * first / (s->nrows - visible);
        gfx_fill(LCD_W - 3, by, 2, bar, C_DIM);
    }
    /* "◀▶ move   ● set   shutter: back" (About: any button goes back) */
    gfx_fill(0, LCD_H - 18, LCD_W, 1, C_DARK);
    int hx = 8, hy = LCD_H - 16;
    if (s->sel < 0) {
        gfx_text_centre(SMALL, LCD_W / 2, hy, "any button: back", C_GREY, -1);
        return;
    }
    hx = glyph_lr(hx, hy, C_GREY);
    hx = gfx_text(SMALL, hx + 3, hy, "move", C_GREY, -1);
    hx = glyph_ok(hx + 14, hy, C_GREY);
    hx = gfx_text(SMALL, hx + 3, hy, "set", C_GREY, -1);
    gfx_text(SMALL, hx + 14, hy, "shutter: back", C_GREY, -1);

    if (s->dialog) dialog("Format the card?", "erases every photo", "format", C_RED);
}

void ui_draw(const ui_state_t *s, const ui_overlay_t *o)
{
    gfx_clear(C_BLACK);
    bool picture = s->view == UI_PREVIEW || s->view == UI_MODE || s->view == UI_GALLERY;
    if (picture && o->have_image) gfx_draw_image();
    if (picture && o->flash) gfx_fill(UI_IMG_X, 0, UI_IMG_W, LCD_H, C_WHITE);

    switch (s->view) {
    case UI_BOOT:     draw_boot(s); break;
    case UI_MESSAGE:  draw_message(s); break;
    case UI_PREVIEW:  draw_preview(s, o); if (s->busy) toast("saving..."); break;
    case UI_MODE:     draw_mode(s); break;
    case UI_GALLERY:  draw_gallery(s); break;
    case UI_SETTINGS: draw_settings(s); break;
    case UI_PHONE:    draw_phone(s, o); return;   /* draws its own messages */
    case UI_OFF:      break;
    }
    bool toast_ok = (s->view == UI_PREVIEW && !s->busy && !s->countdown) ||
                    (s->view == UI_GALLERY && !s->g_confirm) || (s->view == UI_SETTINGS && !s->dialog);
    if (o->toast && toast_ok) toast(o->toast);
}
