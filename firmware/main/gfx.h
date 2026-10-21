#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "font_spleen.h"

/*
 * gfx.h: drawing into the screen's framebuffer. Plain C with no ESP-IDF calls, so the
 * same code also builds on the laptop (../sim) to render pictures of every screen.
 */

#define LCD_W 240
#define LCD_H 135

/* Colours are RGB565 stored big-endian, the byte order the screen wants. */
#define RGB(r, g, b) ((uint16_t)(((((r) & 0xF8) | ((g) >> 5)) & 0xFF) | \
                                 (((((g) & 0x1C) << 3) | ((b) >> 3)) << 8)))
#define C_BLACK  RGB(0, 0, 0)
#define C_WHITE  RGB(255, 255, 255)
#define C_GREY   RGB(150, 146, 160)
#define C_DIM    RGB(88, 84, 98)
#define C_DARK   RGB(34, 32, 40)
#define C_PANEL  RGB(22, 20, 28)
#define C_RED    RGB(235, 60, 60)
#define C_ORANGE RGB(255, 138, 61)
#define C_LILAC  RGB(201, 184, 242)
#define C_GREEN  RGB(90, 210, 120)

extern const font_t *const SMALL;   /* Spleen 8x16 */
extern const font_t *const BIG;     /* Spleen 16x32 */

/* The framebuffer, LCD_W x LCD_H. The firmware hands in DMA-capable memory. */
void gfx_set_framebuffer(uint16_t *fb);
uint16_t *gfx_framebuffer(void);

void gfx_clear(uint16_t c);
void gfx_fill(int x, int y, int w, int h, uint16_t c);
void gfx_fill_round(int x, int y, int w, int h, int r, uint16_t c);
void gfx_frame(int x, int y, int w, int h, uint16_t c);
void gfx_circle(int cx, int cy, int r, uint16_t c);
void gfx_triangle(int x, int y, int size, bool pointing_right, uint16_t c);
/* Darken a box (for panels over the live picture). amount 0..255 */
void gfx_shade(int x, int y, int w, int h, int amount);
/* 1-bit icon, 12x12, drawn from rows of '#' and '.', at 1x or larger */
void gfx_icon(const char *const rows[12], int x, int y, int scale, uint16_t c);
int  gfx_text_width(const font_t *f, const char *s);
/* bg < 0 draws the text with no background. Returns the x after the text. */
int  gfx_text(const font_t *f, int x, int y, const char *s, uint16_t fg, int bg);
void gfx_text_centre(const font_t *f, int cx, int y, const char *s, uint16_t fg, int bg);

/* The picture last decoded for the screen (RGB565 big-endian), fitted into a box,
 * same aspect ratio, centred. The firmware decodes JPEGs into it; the laptop
 * preview loads a raw file. */
void gfx_set_image(const uint16_t *px, int w, int h, int box_x, int box_y, int box_w, int box_h);
void gfx_draw_image(void);

/* Icons */
extern const char *const ICON_CAMERA[12], *const ICON_VIDEO[12], *const ICON_GEAR[12],
    *const ICON_TIMER[12], *const ICON_GRID[12], *const ICON_SUN[12], *const ICON_MOON[12],
    *const ICON_CARD[12], *const ICON_INFO[12], *const ICON_PHONE[12];
