#pragma once

/*
 * small.c: a photo made small for the DigiGallery frame (GET /small, 10-frame-mailbox.md).
 */

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* Make a w x h JPEG from a full-size one: scaled until it covers w x h, then the middle
 * cropped. On ESP_OK *out is a PSRAM buffer the caller frees. */
esp_err_t small_make(const uint8_t *jpg, size_t len, int w, int h, uint8_t **out, size_t *out_len);
