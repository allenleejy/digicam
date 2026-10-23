/*
 * storage.c: photos and videos on the built-in microSD slot.
 *
 * Layout on the card:
 *   DCIM/DSC_0001.JPG          photos, full resolution
 *   DCIM/MOV_0002.AVI          videos, MJPEG
 *   DCIM/.thumbs/DSC_0001.JPG  preview-size thumbnail for each photo/video
 *   frame.json                 the frame mailbox (outside DCIM)
 *
 * The file number is found by scanning the card at boot, so swapping cards
 * never overwrites a photo.
 */

#include "storage.h"
#include "pins.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "driver/sdmmc_host.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

static const char *TAG = "storage";

#define PHOTO_TMP  STORAGE_DCIM "/.photo.tmp"
#define VIDEO_TMP  STORAGE_DCIM "/.rec.tmp"
#define FRAME_PATH STORAGE_ROOT "/frame.json"
#define FRAME_TMP  STORAGE_ROOT "/frame.tmp"
#define FRAME_OLD  STORAGE_ROOT "/frame.old"

#define BOUNCE_SIZE (16 * 1024)

static sdmmc_card_t *s_card;
static uint8_t *s_bounce;   /* internal RAM, DMA-capable */
static uint8_t *s_media;    /* storage_media_buf() */
static bool s_ready;
static int s_next = 1;

/* "DSC_0001.JPG" or "MOV_0001.AVI" -> number, else -1 */
static int parse_name(const char *name, bool *is_video)
{
    int n;
    char ext[4];
    if (strlen(name) != 12) return -1;
    if (sscanf(name, "DSC_%4d.%3s", &n, ext) == 2 && strcmp(ext, "JPG") == 0) {
        if (is_video) *is_video = false;
        return n;
    }
    if (sscanf(name, "MOV_%4d.%3s", &n, ext) == 2 && strcmp(ext, "AVI") == 0) {
        if (is_video) *is_video = true;
        return n;
    }
    return -1;
}

static void thumb_path(const char *name, char *out, size_t len)
{
    /* MOV_0002.AVI -> .thumbs/MOV_0002.JPG */
    snprintf(out, len, STORAGE_THUMBS "/%.8s.JPG", name);
}

esp_err_t storage_init(void)
{
    s_bounce = heap_caps_malloc(BOUNCE_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_media = heap_caps_malloc(STORAGE_MEDIA_BUF, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!s_bounce || !s_media) return ESP_ERR_NO_MEM;

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.flags = SDMMC_HOST_FLAG_1BIT;
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.clk = PIN_SD_CLK;
    slot.cmd = PIN_SD_CMD;
    slot.d0 = PIN_SD_D0;
    slot.width = 1;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_vfs_fat_sdmmc_mount_config_t mount = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };

    esp_err_t err = esp_vfs_fat_sdmmc_mount(STORAGE_ROOT, &host, &slot, &mount, &s_card);
    if (err != ESP_OK && host.max_freq_khz != SDMMC_FREQ_DEFAULT) {
        ESP_LOGW(TAG, "High-speed mount failed (%s), retrying at default speed", esp_err_to_name(err));
        host.max_freq_khz = SDMMC_FREQ_DEFAULT;
        err = esp_vfs_fat_sdmmc_mount(STORAGE_ROOT, &host, &slot, &mount, &s_card);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s (card inserted and FAT32?)", esp_err_to_name(err));
        return err;
    }

    mkdir(STORAGE_DCIM, 0775);
    mkdir(STORAGE_THUMBS, 0775);
    unlink(PHOTO_TMP);
    unlink(VIDEO_TMP);
    /* A mailbox save cut off after the old one was moved aside: put the old one back */
    struct stat fst;
    if (stat(FRAME_PATH, &fst) != 0 && stat(FRAME_OLD, &fst) == 0) rename(FRAME_OLD, FRAME_PATH);
    unlink(FRAME_OLD);
    unlink(FRAME_TMP);

    int max = 0;
    DIR *d = opendir(STORAGE_DCIM);
    if (d) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            int n = parse_name(e->d_name, NULL);
            if (n > max) max = n;
        }
        closedir(d);
    }
    s_next = max + 1;
    s_ready = true;

    uint64_t total, free_bytes;
    storage_space(&total, &free_bytes);
    ESP_LOGI(TAG, "SD ready: %s, %llu MB free of %llu MB, next file %04d, bus %d kHz",
             s_card->cid.name, free_bytes >> 20, total >> 20, s_next, s_card->real_freq_khz);
    return ESP_OK;
}

esp_err_t storage_format(void)
{
    if (!s_card) return ESP_ERR_INVALID_STATE;
    esp_err_t err = esp_vfs_fat_sdcard_format(STORAGE_ROOT, s_card);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "format failed: %s", esp_err_to_name(err));
        return err;
    }
    mkdir(STORAGE_DCIM, 0775);
    mkdir(STORAGE_THUMBS, 0775);
    /* File numbers carry on from where they were, so a photo already copied to a phone
     * never shares a name with a new one. */
    ESP_LOGI(TAG, "card formatted");
    return ESP_OK;
}

uint8_t *storage_media_buf(void)
{
    return s_media;
}

bool storage_ready(void)
{
    return s_ready;
}

int storage_next_number(void)
{
    int n = s_next++;
    if (s_next > 9999) s_next = 1;
    return n;
}

size_t storage_fwrite(FILE *f, const void *buf, size_t len)
{
    const uint8_t *p = buf;
    size_t done = 0;
    while (done < len) {
        size_t n = len - done < BOUNCE_SIZE ? len - done : BOUNCE_SIZE;
        memcpy(s_bounce, p + done, n);
        if (fwrite(s_bounce, 1, n, f) != n) break;
        done += n;
    }
    return done;
}

static esp_err_t write_file(const char *path, const uint8_t *buf, size_t len, bool sync)
{
    int64_t t0 = esp_timer_get_time();
    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "open %s failed: %s", path, strerror(errno));
        return ESP_FAIL;
    }
    int64_t t1 = esp_timer_get_time();
    size_t n = storage_fwrite(f, buf, len);
    int64_t t2 = esp_timer_get_time();
    fflush(f);
    if (sync) fsync(fileno(f));
    int64_t t3 = esp_timer_get_time();
    fclose(f);
    ESP_LOGI(TAG, "write %s %u B: open %lld ms, write %lld ms, sync %lld ms, close %lld ms", path,
             (unsigned)len, (t1 - t0) / 1000, (t2 - t1) / 1000, (t3 - t2) / 1000,
             (esp_timer_get_time() - t3) / 1000);
    if (n != len) {
        ESP_LOGE(TAG, "short write to %s (%u of %u)", path, (unsigned)n, (unsigned)len);
        unlink(path);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t storage_save_photo(const uint8_t *jpg, size_t len,
                             const uint8_t *thumb, size_t thumb_len,
                             char *out_name)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    if (write_file(PHOTO_TMP, jpg, len, true) != ESP_OK) return ESP_FAIL;

    char name[STORAGE_NAME_LEN], path[64];
    snprintf(name, sizeof(name), "DSC_%04d.JPG", storage_next_number());
    snprintf(path, sizeof(path), STORAGE_DCIM "/%s", name);
    if (rename(PHOTO_TMP, path) != 0) {
        ESP_LOGE(TAG, "rename to %s failed: %s", path, strerror(errno));
        unlink(PHOTO_TMP);
        return ESP_FAIL;
    }
    if (thumb && thumb_len) {
        thumb_path(name, path, sizeof(path));
        write_file(path, thumb, thumb_len, false);   /* a missing thumbnail is not fatal */
    }
    strcpy(out_name, name);
    return ESP_OK;
}

const char *storage_video_tmp_path(void)
{
    return VIDEO_TMP;
}

esp_err_t storage_commit_video(const uint8_t *thumb, size_t thumb_len, char *out_name)
{
    char name[STORAGE_NAME_LEN], path[64];
    snprintf(name, sizeof(name), "MOV_%04d.AVI", storage_next_number());
    snprintf(path, sizeof(path), STORAGE_DCIM "/%s", name);
    if (rename(VIDEO_TMP, path) != 0) {
        ESP_LOGE(TAG, "rename to %s failed: %s", path, strerror(errno));
        return ESP_FAIL;
    }
    if (thumb && thumb_len) {
        thumb_path(name, path, sizeof(path));
        write_file(path, thumb, thumb_len, false);
    }
    strcpy(out_name, name);
    return ESP_OK;
}

static int item_cmp(const void *a, const void *b)
{
    /* newest (highest number) first */
    return parse_name(((const storage_item_t *)b)->name, NULL) -
           parse_name(((const storage_item_t *)a)->name, NULL);
}

esp_err_t storage_list(storage_item_t **out, int *count)
{
    *out = NULL;
    *count = 0;
    if (!s_ready) return ESP_ERR_INVALID_STATE;

    DIR *d = opendir(STORAGE_DCIM);
    if (!d) return ESP_FAIL;

    int cap = 64, n = 0;
    storage_item_t *items = heap_caps_malloc(cap * sizeof(*items), MALLOC_CAP_SPIRAM);
    struct dirent *e;
    char path[300];
    while (items && (e = readdir(d)) != NULL) {
        bool is_video;
        if (parse_name(e->d_name, &is_video) < 0) continue;
        if (n == cap) {
            cap *= 2;
            storage_item_t *bigger = heap_caps_realloc(items, cap * sizeof(*items), MALLOC_CAP_SPIRAM);
            if (!bigger) break;
            items = bigger;
        }
        struct stat st;
        snprintf(path, sizeof(path), STORAGE_DCIM "/%s", e->d_name);
        items[n].size = (stat(path, &st) == 0) ? (uint32_t)st.st_size : 0;
        items[n].is_video = is_video;
        strlcpy(items[n].name, e->d_name, sizeof(items[n].name));
        n++;
    }
    closedir(d);
    if (!items) return ESP_ERR_NO_MEM;

    qsort(items, n, sizeof(*items), item_cmp);
    *out = items;
    *count = n;
    return ESP_OK;
}

FILE *storage_open(const char *name, bool thumb, uint32_t *size)
{
    if (strcmp(name, "frame.json") == 0) return thumb ? NULL : storage_frame_open(size);   /* for the USB link */
    if (!s_ready || parse_name(name, NULL) < 0) return NULL;
    char path[64];
    if (thumb) {
        thumb_path(name, path, sizeof(path));
    } else {
        snprintf(path, sizeof(path), STORAGE_DCIM "/%s", name);
    }
    struct stat st;
    if (stat(path, &st) != 0) return NULL;
    *size = (uint32_t)st.st_size;
    return fopen(path, "rb");
}

esp_err_t storage_delete(const char *name)
{
    if (!s_ready || parse_name(name, NULL) < 0) return ESP_ERR_INVALID_ARG;
    char path[64];
    snprintf(path, sizeof(path), STORAGE_DCIM "/%s", name);
    if (unlink(path) != 0) return ESP_ERR_NOT_FOUND;
    thumb_path(name, path, sizeof(path));
    unlink(path);
    return ESP_OK;
}

FILE *storage_frame_open(uint32_t *size)
{
    struct stat st;
    if (!s_ready || stat(FRAME_PATH, &st) != 0) return NULL;
    *size = (uint32_t)st.st_size;
    return fopen(FRAME_PATH, "rb");
}

esp_err_t storage_frame_save(const void *buf, size_t len)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    FILE *f = fopen(FRAME_TMP, "wb");
    if (!f) return ESP_FAIL;
    bool ok = fwrite(buf, 1, len, f) == len && fflush(f) == 0 && fsync(fileno(f)) == 0;
    fclose(f);
    if (!ok) {
        unlink(FRAME_TMP);
        return ESP_FAIL;
    }
    /* FAT can't rename over a file, so the old mailbox moves aside first. A power cut at
     * any point leaves a whole document: storage_init puts the old one back if needed. */
    unlink(FRAME_OLD);
    rename(FRAME_PATH, FRAME_OLD);                  /* fails harmlessly the first time */
    if (rename(FRAME_TMP, FRAME_PATH) != 0) {
        ESP_LOGE(TAG, "rename to %s failed: %s", FRAME_PATH, strerror(errno));
        rename(FRAME_OLD, FRAME_PATH);
        unlink(FRAME_TMP);
        return ESP_FAIL;
    }
    unlink(FRAME_OLD);
    return ESP_OK;
}

void storage_space(uint64_t *total, uint64_t *free_bytes)
{
    *total = 0;
    *free_bytes = 0;
    if (s_card) esp_vfs_fat_info(STORAGE_ROOT, total, free_bytes);
}

void storage_counts(int *photos, int *videos)
{
    *photos = 0;
    *videos = 0;
    DIR *d = opendir(STORAGE_DCIM);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        bool is_video;
        if (parse_name(e->d_name, &is_video) < 0) continue;
        if (is_video) (*videos)++; else (*photos)++;
    }
    closedir(d);
}
