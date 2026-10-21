#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "gfx.h"

/*
 * display.h: the camera's screen hardware. A 1.14 in 240x135 ST7789 for now; the
 * 2.0 in 320x240 uses the same driver with a different size and gap.
 * Drawing is in gfx.h; display_present() sends the framebuffer (65 KB, about 13 ms
 * at 40 MHz).
 */

#define LCD_SPI_HZ (40 * 1000 * 1000)

esp_err_t display_init(void);
void display_backlight(int percent);
void display_present(void);

/* Orientation and RAM offset, adjustable at run time (link command "lcd") so a new
 * module can be lined up without reflashing. */
void display_tune(bool swap_xy, bool mirror_x, bool mirror_y, int gap_x, int gap_y, bool invert);

/* Decode a JPEG for the screen and hand it to gfx_set_image(), fitted into the box.
 * Decodes at the smallest 1/1, 1/2, 1/4 or 1/8 scale that still covers the box.
 * Returns false if the JPEG can't be decoded. */
bool display_decode_jpeg(const uint8_t *jpg, size_t len, int x, int y, int w, int h);
