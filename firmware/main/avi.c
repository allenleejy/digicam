/*
 * avi.c: minimal MJPEG AVI writer (RIFF AVI with one video stream and an idx1 index).
 *
 * File layout:
 *   RIFF 'AVI '
 *     LIST 'hdrl'  avih, LIST 'strl' (strh, strf)       fixed 212 bytes, patched on close
 *     LIST 'movi'  '00dc' chunk per frame
 *     idx1         one 16-byte entry per frame
 *
 * The index lives in PSRAM while recording (16 bytes per frame).
 */

#include "avi.h"
#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "avi";

#define HDR_SIZE      AVI_HDR_SIZE        /* everything before the first frame chunk */
#define MOVI_FOURCC   220                 /* file offset of 'movi'; idx1 offsets count from here */
#define WRITE_BUF     STORAGE_MEDIA_BUF

typedef struct {
    uint32_t offset;
    uint32_t size;
} idx_entry_t;

struct avi_writer {
    FILE *f;
    char *buf;
    int width, height;
    uint32_t frames;
    uint32_t movi_bytes;    /* bytes of frame chunks written after 'movi' */
    uint32_t max_frame;
    idx_entry_t *idx;
    uint32_t idx_cap;
};

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24;
}

static void put16(uint8_t *p, uint16_t v)
{
    p[0] = v; p[1] = v >> 8;
}

static void build_header(const avi_writer_t *w, uint8_t *h, uint32_t fps_milli, uint32_t riff_size)
{
    memset(h, 0, HDR_SIZE);
    uint32_t us_per_frame = fps_milli ? (uint32_t)(1000000000ULL / fps_milli) : 66666;

    memcpy(h + 0, "RIFF", 4);     put32(h + 4, riff_size);     memcpy(h + 8, "AVI ", 4);
    memcpy(h + 12, "LIST", 4);    put32(h + 16, 192);          memcpy(h + 20, "hdrl", 4);

    memcpy(h + 24, "avih", 4);    put32(h + 28, 56);
    uint8_t *a = h + 32;
    put32(a + 0, us_per_frame);
    put32(a + 4, w->max_frame * (fps_milli / 1000 + 1));   /* max bytes/sec, approximate */
    put32(a + 12, 0x10);                                   /* AVIF_HASINDEX */
    put32(a + 16, w->frames);
    put32(a + 24, 1);                                      /* streams */
    put32(a + 28, w->max_frame);
    put32(a + 32, w->width);
    put32(a + 36, w->height);

    memcpy(h + 88, "LIST", 4);    put32(h + 92, 116);          memcpy(h + 96, "strl", 4);

    memcpy(h + 100, "strh", 4);   put32(h + 104, 56);
    uint8_t *s = h + 108;
    memcpy(s + 0, "vids", 4);
    memcpy(s + 4, "MJPG", 4);
    put32(s + 20, 1000);                                   /* scale */
    put32(s + 24, fps_milli);                              /* rate: fps = rate / scale */
    put32(s + 32, w->frames);                              /* length */
    put32(s + 36, w->max_frame);
    put32(s + 40, 0xFFFFFFFF);                             /* quality: default */
    put16(s + 52, w->width);
    put16(s + 54, w->height);

    memcpy(h + 164, "strf", 4);   put32(h + 168, 40);
    uint8_t *b = h + 172;
    put32(b + 0, 40);
    put32(b + 4, w->width);
    put32(b + 8, w->height);
    put16(b + 12, 1);
    put16(b + 14, 24);
    memcpy(b + 16, "MJPG", 4);
    put32(b + 20, w->width * w->height * 3);

    memcpy(h + 212, "LIST", 4);   put32(h + 216, 4 + w->movi_bytes);   memcpy(h + 220, "movi", 4);
}

avi_writer_t *avi_open(const char *path, int width, int height)
{
    avi_writer_t *w = calloc(1, sizeof(*w));
    if (!w) return NULL;
    w->width = width;
    w->height = height;
    w->idx_cap = 4096;
    w->idx = heap_caps_malloc(w->idx_cap * sizeof(idx_entry_t), MALLOC_CAP_SPIRAM);
    /* stdio buffer in internal RAM so flushes can DMA to the card: the one set aside at
     * start-up, since after Phone mode a fresh 16-32 KB block often can't be found */
    w->buf = (char *)storage_media_buf();
    w->f = fopen(path, "wb");
    if (!w->idx || !w->buf || !w->f) {
        ESP_LOGE(TAG, "open %s failed", path);
        if (w->f) fclose(w->f);
        free(w->idx);
        free(w);
        return NULL;
    }
    setvbuf(w->f, w->buf, _IOFBF, WRITE_BUF);

    uint8_t h[HDR_SIZE];
    build_header(w, h, 15000, 0);
    fwrite(h, 1, HDR_SIZE, w->f);
    return w;
}

esp_err_t avi_add_frame(avi_writer_t *w, const uint8_t *jpg, size_t len)
{
    if (w->frames == w->idx_cap) {
        idx_entry_t *bigger = heap_caps_realloc(w->idx, 2 * w->idx_cap * sizeof(idx_entry_t), MALLOC_CAP_SPIRAM);
        if (!bigger) return ESP_ERR_NO_MEM;
        w->idx = bigger;
        w->idx_cap *= 2;
    }

    uint8_t chunk[8];
    memcpy(chunk, "00dc", 4);
    put32(chunk + 4, len);
    uint32_t pad = len & 1;
    static const uint8_t zero = 0;

    if (fwrite(chunk, 1, 8, w->f) != 8 ||
        storage_fwrite(w->f, jpg, len) != len ||
        (pad && fwrite(&zero, 1, 1, w->f) != 1)) {
        return ESP_FAIL;
    }

    w->idx[w->frames].offset = 4 + w->movi_bytes;   /* relative to the 'movi' fourcc */
    w->idx[w->frames].size = len;
    w->frames++;
    w->movi_bytes += 8 + len + pad;
    if (len > w->max_frame) w->max_frame = len;
    return ESP_OK;
}

uint32_t avi_frames(const avi_writer_t *w)
{
    return w->frames;
}

uint32_t avi_bytes(const avi_writer_t *w)
{
    return HDR_SIZE + w->movi_bytes;
}

esp_err_t avi_close(avi_writer_t *w, uint32_t duration_ms)
{
    esp_err_t ret = ESP_OK;

    uint8_t hdr[8];
    memcpy(hdr, "idx1", 4);
    put32(hdr + 4, w->frames * 16);
    fwrite(hdr, 1, 8, w->f);
    for (uint32_t i = 0; i < w->frames; i++) {
        uint8_t e[16];
        memcpy(e, "00dc", 4);
        put32(e + 4, 0x10);                  /* AVIIF_KEYFRAME: every MJPEG frame is one */
        put32(e + 8, w->idx[i].offset);
        put32(e + 12, w->idx[i].size);
        if (fwrite(e, 1, 16, w->f) != 16) { ret = ESP_FAIL; break; }
    }

    uint32_t fps_milli = duration_ms ? (uint32_t)((uint64_t)w->frames * 1000000ULL / duration_ms) : 15000;
    if (fps_milli == 0) fps_milli = 1000;
    uint32_t riff_size = HDR_SIZE - 8 + w->movi_bytes + 8 + w->frames * 16;

    uint8_t h[HDR_SIZE];
    build_header(w, h, fps_milli, riff_size);
    fflush(w->f);
    if (fseek(w->f, 0, SEEK_SET) != 0 || fwrite(h, 1, HDR_SIZE, w->f) != HDR_SIZE) ret = ESP_FAIL;
    fflush(w->f);
    fsync(fileno(w->f));
    fclose(w->f);

    ESP_LOGI(TAG, "closed: %lu frames, %.1f fps, %lu KB",
             (unsigned long)w->frames, fps_milli / 1000.0, (unsigned long)(riff_size >> 10));
    free(w->idx);
    free(w);
    return ret;
}
