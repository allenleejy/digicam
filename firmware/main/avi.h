#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* Bytes before the first frame chunk. Each frame is then "00dc", length u32, JPEG, pad to even. */
#define AVI_HDR_SIZE 224

typedef struct avi_writer avi_writer_t;

/* Start an MJPEG AVI at `path`. Frames are JPEGs straight from the sensor. */
avi_writer_t *avi_open(const char *path, int width, int height);
esp_err_t avi_add_frame(avi_writer_t *w, const uint8_t *jpg, size_t len);
uint32_t avi_frames(const avi_writer_t *w);
uint32_t avi_bytes(const avi_writer_t *w);

/* Write the index and patch the header with the real frame rate. Frees `w`. */
esp_err_t avi_close(avi_writer_t *w, uint32_t duration_ms);
