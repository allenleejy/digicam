#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "esp_err.h"

#define STORAGE_ROOT      "/sdcard"
#define STORAGE_DCIM      STORAGE_ROOT "/DCIM"
#define STORAGE_THUMBS    STORAGE_DCIM "/.thumbs"
#define STORAGE_SMALL     STORAGE_DCIM "/.small"
#define STORAGE_NAME_LEN  16   /* "DSC_0001.JPG" + NUL, with room */

typedef struct {
    char name[STORAGE_NAME_LEN];
    uint32_t size;
    bool is_video;
} storage_item_t;

/* Mount the board's microSD slot (SDMMC 1-bit), create DCIM, clean stale temp files. */
esp_err_t storage_init(void);
bool storage_ready(void);

/* One 16 KB internal-RAM buffer, set aside at start-up and shared by video recording (its
 * write buffer) and Phone mode (its read buffer), which never run at the same time. Taken
 * at start-up because once Wi-Fi has run, internal RAM is too broken up for a fresh one. */
#define STORAGE_MEDIA_BUF (16 * 1024)
uint8_t *storage_media_buf(void);

/* Erase the card (FAT, quick format). Photos and videos are gone afterwards. */
esp_err_t storage_format(void);

/* Claim the next file number. Photos and videos share one counter, kept in NVS as well as
 * read off the card, so numbers carry on after a format and a restart. */
int storage_next_number(void);

/* Save a photo and its preview-size thumbnail. Writes to a temp file, then renames,
 * so switching off mid-save never leaves a broken photo. */
esp_err_t storage_save_photo(const uint8_t *jpg, size_t len,
                             const uint8_t *thumb, size_t thumb_len,
                             char *out_name);

/* Video files are written by avi.c to STORAGE_DCIM "/.rec.tmp", then committed here. */
const char *storage_video_tmp_path(void);
esp_err_t storage_commit_video(const uint8_t *thumb, size_t thumb_len, char *out_name);

/* Newest first. Caller frees *out. */
esp_err_t storage_list(storage_item_t **out, int *count);

/* Open a photo/video (or its thumbnail) for reading. Rejects any name that is not
 * one of ours, so commands can't reach outside DCIM. */
FILE *storage_open(const char *name, bool thumb, uint32_t *size);
esp_err_t storage_delete(const char *name);   /* also its thumbnail and small copies */

/* A photo's small copy for the frame (GET /small): DCIM/.small/DSC_0001_320x256.JPG.
 * Open: NULL when it hasn't been made yet, or the name isn't one of our photos. Save goes
 * through a temp file, like photos; `jpg` may be in PSRAM. */
FILE *storage_small_open(const char *name, int w, int h, uint32_t *size);
esp_err_t storage_small_save(const char *name, int w, int h, const uint8_t *jpg, size_t len);

/* fwrite for buffers in PSRAM (camera frames). The SD driver can't DMA from PSRAM and
 * falls back to one 512-byte sector per transfer (~120 KB/s), so this copies through
 * an internal-RAM bounce buffer in 16 KB pieces. Only call from one task. */
size_t storage_fwrite(FILE *f, const void *buf, size_t len);

/* The frame mailbox: /sdcard/frame.json, written by the phone page and read by the
 * DigiGallery frame (see 10-frame-mailbox.md). Kept outside DCIM, so the photo list and
 * the name check never see it. The camera stores it but doesn't act on it. */
FILE *storage_frame_open(uint32_t *size);   /* NULL when there's no mailbox */
/* Replace the mailbox. `buf` must be in internal RAM: the SD driver can't DMA from PSRAM. */
esp_err_t storage_frame_save(const void *buf, size_t len);

void storage_space(uint64_t *total, uint64_t *free_bytes);
void storage_counts(int *photos, int *videos);
