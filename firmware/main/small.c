/*
 * small.c: a photo made small for the DigiGallery frame. See small.h.
 *
 * Decode at the biggest reduction that still covers the size (1/4 of a 2048x1536 photo for
 * 320x256), crop the middle to the size's shape and shrink it with a box filter, then encode
 * a baseline JPEG. Everything big lives in PSRAM: Phone mode has little internal RAM.
 */

#include "small.h"

#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "jpeg_decoder.h"

static const char *TAG = "small";

#define WORK_SIZE   6144        /* decoder scratch: 3100 is enough for 4:2:2, 4:2:0 needs more */
#define QUALITY     80
#define QUALITY_LOW 60
#define SIZE_AIM    (24 * 1024) /* the frame takes one picture of up to 32 KB at a time */

static void *big(size_t n)
{
    return heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

/* Shrink the middle of src (sw x sh, 3 bytes a pixel) into dst (w x h): each output pixel is
 * the average of the source pixels under it, at least one */
static void cover(const uint8_t *src, int sw, int sh, uint8_t *dst, int w, int h)
{
    int cw = sw, ch = sh;
    if ((long)sw * h > (long)sh * w) cw = sh * w / h;   /* wider than wanted: crop the sides */
    else ch = sw * h / w;                               /* taller: crop top and bottom */
    int ox = (sw - cw) / 2, oy = (sh - ch) / 2;

    for (int y = 0; y < h; y++) {
        int y0 = oy + y * ch / h, y1 = oy + (y + 1) * ch / h;
        if (y1 <= y0) y1 = y0 + 1;
        for (int x = 0; x < w; x++) {
            int x0 = ox + x * cw / w, x1 = ox + (x + 1) * cw / w;
            if (x1 <= x0) x1 = x0 + 1;
            uint32_t s0 = 0, s1 = 0, s2 = 0, n = 0;
            for (int yy = y0; yy < y1; yy++) {
                const uint8_t *p = src + ((size_t)yy * sw + x0) * 3;
                for (int xx = x0; xx < x1; xx++, p += 3) {
                    s0 += p[0];
                    s1 += p[1];
                    s2 += p[2];
                    n++;
                }
            }
            uint8_t *d = dst + ((size_t)y * w + x) * 3;
            d[0] = s0 / n;
            d[1] = s1 / n;
            d[2] = s2 / n;
        }
    }
}

esp_err_t small_make(const uint8_t *jpg, size_t len, int w, int h, uint8_t **out, size_t *out_len)
{
    int64_t t0 = esp_timer_get_time();
    *out = NULL;
    *out_len = 0;

    uint8_t *work = heap_caps_malloc(WORK_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!work) work = big(WORK_SIZE);
    /* swap_color_bytes: the decoder gives R,G,B; fmt2jpg's RGB888 is B,G,R, like the camera's */
    esp_jpeg_image_cfg_t cfg = {
        .indata = (uint8_t *)jpg, .indata_size = len,
        .out_format = JPEG_IMAGE_FORMAT_RGB888, .out_scale = JPEG_IMAGE_SCALE_0,
        .flags.swap_color_bytes = 1,
        .advanced.working_buffer = work, .advanced.working_buffer_size = WORK_SIZE,
    };
    esp_jpeg_image_output_t info;
    uint8_t *dec = NULL, *px = NULL;
    esp_err_t err = work ? esp_jpeg_get_image_info(&cfg, &info) : ESP_ERR_NO_MEM;
    if (err != ESP_OK) goto done;

    int scale = 0;
    while (scale < 3 && (info.width >> (scale + 1)) >= w && (info.height >> (scale + 1)) >= h) scale++;
    cfg.out_scale = (esp_jpeg_image_scale_t)scale;
    esp_jpeg_get_image_info(&cfg, &info);   /* the output size at that scale */
    cfg.outbuf = dec = big(info.output_len);
    cfg.outbuf_size = info.output_len;
    px = big((size_t)w * h * 3);
    if (!dec || !px) {
        err = ESP_ERR_NO_MEM;
        goto done;
    }
    err = esp_jpeg_decode(&cfg, &info);
    if (err != ESP_OK) goto done;
    int64_t t1 = esp_timer_get_time();

    free(work);
    work = NULL;
    cover(dec, info.width, info.height, px, w, h);
    free(dec);
    dec = NULL;
    int64_t t2 = esp_timer_get_time();

    int q = QUALITY;
    if (!fmt2jpg(px, (size_t)w * h * 3, w, h, PIXFORMAT_RGB888, q, out, out_len)) {
        err = ESP_FAIL;
        goto done;
    }
    if (*out_len > SIZE_AIM) {   /* a busy picture: smaller, so it fits the frame's room */
        uint8_t *again;
        size_t again_len;
        if (fmt2jpg(px, (size_t)w * h * 3, w, h, PIXFORMAT_RGB888, QUALITY_LOW, &again, &again_len)) {
            free(*out);
            *out = again;
            *out_len = again_len;
            q = QUALITY_LOW;
        }
    }
    ESP_LOGI(TAG, "%dx%d at 1/%d to %dx%d, %u bytes at quality %d: decode %d ms, shrink %d ms, encode %d ms",
             info.width << scale, info.height << scale, 1 << scale, w, h, (unsigned)*out_len, q,
             (int)((t1 - t0) / 1000), (int)((t2 - t1) / 1000), (int)((esp_timer_get_time() - t2) / 1000));

done:
    if (err != ESP_OK) ESP_LOGW(TAG, "couldn't make %dx%d: %s", w, h, esp_err_to_name(err));
    free(work);
    free(dec);
    free(px);
    return err;
}
