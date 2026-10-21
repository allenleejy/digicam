/*
 * gfx.c: framebuffer drawing. No ESP-IDF calls (see gfx.h).
 */

#include "gfx.h"

#include <string.h>

const font_t *const SMALL = &FONT_BODY;
const font_t *const BIG = &FONT_HEAD;

static uint16_t *s_fb;

static const uint16_t *s_img;
static int s_img_w, s_img_h, s_box_x, s_box_y, s_box_w, s_box_h;

void gfx_set_framebuffer(uint16_t *fb) { s_fb = fb; }
uint16_t *gfx_framebuffer(void) { return s_fb; }

void gfx_fill(int x, int y, int w, int h, uint16_t c)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > LCD_W) w = LCD_W - x;
    if (y + h > LCD_H) h = LCD_H - y;
    if (w <= 0 || h <= 0) return;
    for (int r = 0; r < h; r++) {
        uint16_t *p = s_fb + (y + r) * LCD_W + x;
        for (int i = 0; i < w; i++) p[i] = c;
    }
}

void gfx_clear(uint16_t c)
{
    gfx_fill(0, 0, LCD_W, LCD_H, c);
}

void gfx_fill_round(int x, int y, int w, int h, int r, uint16_t c)
{
    if (r * 2 > h) r = h / 2;
    if (r * 2 > w) r = w / 2;
    for (int i = 0; i < h; i++) {
        int inset = 0;
        int dy = i < r ? r - i : (i >= h - r ? i - (h - r - 1) : 0);
        if (dy) {
            while (inset < r && (r - inset) * (r - inset) + dy * dy > r * r + r) inset++;
        }
        gfx_fill(x + inset, y + i, w - 2 * inset, 1, c);
    }
}

void gfx_frame(int x, int y, int w, int h, uint16_t c)
{
    gfx_fill(x, y, w, 1, c);
    gfx_fill(x, y + h - 1, w, 1, c);
    gfx_fill(x, y, 1, h, c);
    gfx_fill(x + w - 1, y, 1, h, c);
}

void gfx_circle(int cx, int cy, int r, uint16_t c)
{
    for (int dy = -r; dy <= r; dy++) {
        int dx = 0;
        while ((dx + 1) * (dx + 1) + dy * dy <= r * r) dx++;
        gfx_fill(cx - dx, cy + dy, 2 * dx + 1, 1, c);
    }
}

void gfx_triangle(int x, int y, int size, bool right, uint16_t c)
{
    int half = size / 2;
    for (int i = 0; i <= half; i++) {
        int len = size - 2 * i;
        int col = right ? x + i : x + half - i;
        gfx_fill(col, y + i, 1, len, c);
    }
}

/* RGB565 big-endian <-> components */
static inline uint16_t swap16(uint16_t v) { return (uint16_t)(v << 8 | v >> 8); }

void gfx_shade(int x, int y, int w, int h, int amount)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > LCD_W) w = LCD_W - x;
    if (y + h > LCD_H) h = LCD_H - y;
    int keep = 255 - amount;
    for (int r = 0; r < h; r++) {
        uint16_t *p = s_fb + (y + r) * LCD_W + x;
        for (int i = 0; i < w; i++) {
            uint16_t v = swap16(p[i]);
            int rr = (v >> 11) * keep / 255, gg = ((v >> 5) & 63) * keep / 255, bb = (v & 31) * keep / 255;
            p[i] = swap16((uint16_t)(rr << 11 | gg << 5 | bb));
        }
    }
}

void gfx_icon(const char *const rows[12], int x, int y, int scale, uint16_t c)
{
    for (int r = 0; r < 12; r++) {
        for (int i = 0; i < 12 && rows[r][i]; i++) {
            if (rows[r][i] == '#') gfx_fill(x + i * scale, y + r * scale, scale, scale, c);
        }
    }
}

int gfx_text_width(const font_t *f, const char *s)
{
    return (int)strlen(s) * f->w;
}

int gfx_text(const font_t *f, int x, int y, const char *s, uint16_t fg, int bg)
{
    for (; *s; s++, x += f->w) {
        unsigned ch = (unsigned char)*s;
        if (ch < f->first || ch > f->last) ch = '?';
        const uint8_t *g = f->bits + (ch - f->first) * f->h * f->stride;
        for (int r = 0; r < f->h; r++) {
            int py = y + r;
            if (py < 0 || py >= LCD_H) continue;
            const uint8_t *row = g + r * f->stride;
            for (int c = 0; c < f->w; c++) {
                int px = x + c;
                if (px < 0 || px >= LCD_W) continue;
                bool on = row[c >> 3] & (1 << (c & 7));
                if (on) s_fb[py * LCD_W + px] = fg;
                else if (bg >= 0) s_fb[py * LCD_W + px] = (uint16_t)bg;
            }
        }
    }
    return x;
}

void gfx_text_centre(const font_t *f, int cx, int y, const char *s, uint16_t fg, int bg)
{
    gfx_text(f, cx - gfx_text_width(f, s) / 2, y, s, fg, bg);
}

void gfx_set_image(const uint16_t *px, int w, int h, int bx, int by, int bw, int bh)
{
    s_img = px; s_img_w = w; s_img_h = h;
    s_box_x = bx; s_box_y = by; s_box_w = bw; s_box_h = bh;
}

void gfx_draw_image(void)
{
    if (!s_img || !s_img_w || !s_img_h) return;
    /* Nearest-neighbour resample into the box, keeping the aspect ratio */
    int w = s_box_w, h = s_box_w * s_img_h / s_img_w;
    if (h > s_box_h) { h = s_box_h; w = s_box_h * s_img_w / s_img_h; }
    int ox = s_box_x + (s_box_w - w) / 2, oy = s_box_y + (s_box_h - h) / 2;
    static uint16_t xmap[LCD_W];
    for (int i = 0; i < w && i < LCD_W; i++) xmap[i] = (uint16_t)(i * s_img_w / w);
    for (int r = 0; r < h; r++) {
        int py = oy + r;
        if (py < 0 || py >= LCD_H) continue;
        const uint16_t *src = s_img + (r * s_img_h / h) * s_img_w;
        uint16_t *dst = s_fb + py * LCD_W + ox;
        for (int i = 0; i < w; i++) dst[i] = src[xmap[i]];
    }
}

/* ---------------------------------------------------------------- icons, 12 x 12 */

const char *const ICON_CAMERA[12] = {
    "............", "....####....", "############", "#..........#", "#...####...#", "#..#....#..#",
    "#..#....#..#", "#...####...#", "#..........#", "############", "............", "............" };
const char *const ICON_VIDEO[12] = {
    "............", "............", "########....", "#......#..##", "#......#.###", "#......#####",
    "#......#####", "#......#.###", "#......#..##", "########....", "............", "............" };
const char *const ICON_GEAR[12] = {
    ".....##.....", "..#.####.#..", ".##########.", "..###..###..", ".###....###.", "####....####",
    "####....####", ".###....###.", "..###..###..", ".##########.", "..#.####.#..", ".....##....." };
const char *const ICON_TIMER[12] = {
    "....####....", "..##....##..", ".#....#...#.", ".#....#...#.", "#.....#....#", "#.....###..#",
    "#..........#", "#..........#", ".#........#.", ".#........#.", "..##....##..", "....####...." };
const char *const ICON_GRID[12] = {
    "...#...#....", "...#...#....", "...#...#....", "############", "...#...#....", "...#...#....",
    "...#...#....", "############", "...#...#....", "...#...#....", "...#...#....", "............" };
const char *const ICON_SUN[12] = {
    ".....#......", ".#...#...#..", "..#.....#...", "....###.....", "...#####....", "##.#####.##.",
    "...#####....", "....###.....", "..#.....#...", ".#...#...#..", ".....#......", "............" };
const char *const ICON_MOON[12] = {
    "....####....", "..###.......", ".###........", ".##.........", "###.........", "###.........",
    "###.........", "###.......#.", ".###.....##.", ".#####.####.", "...######...", "............" };
const char *const ICON_CARD[12] = {
    "..#######...", ".#.#.#.#.#..", "#.........#.", "#.........#.", "#.........#.", "#.........#.",
    "#.........#.", "#.........#.", "#.........#.", "#.........#.", "###########.", "............" };
const char *const ICON_INFO[12] = {
    "....####....", "..##....##..", ".#...##...#.", ".#........#.", "#....##....#", "#....##....#",
    "#....##....#", "#....##....#", ".#...##...#.", ".#........#.", "..##....##..", "....####...." };
const char *const ICON_PHONE[12] = {
    "...######...", "..#......#..", "..#.####.#..", "..#.#..#.#..", "..#.#..#.#..", "..#.#..#.#..",
    "..#.#..#.#..", "..#.####.#..", "..#......#..", "..#..##..#..", "..#......#..", "...######..." };
