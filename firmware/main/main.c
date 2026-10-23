/*
 * main.c: digicam camera service.
 *
 * The camera runs on its own: screen, shutter, three back buttons, battery.
 * The laptop simulator (../host) can still drive it over the serial link; every
 * button has a matching command, and both call the same functions.
 *
 * All camera and SD work happens on one task (cam_task), so commands and button
 * presses never race the preview loop. ui.c draws the screen on the other core,
 * and tx_task streams preview frames to the laptop when it's watching.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "avi.h"
#include "battery.h"
#include "buttons.h"
#include "display.h"
#include "lens.h"
#include "link.h"
#include "personal.h"
#include "phone.h"
#include "qrcode.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "storage.h"
#include "ui.h"

static const char *TAG = "digicam";

#define FW_VERSION      "0.4.2"
#define SLOT_SIZE       (256 * 1024)     /* one preview frame on its way to the laptop */
#define THUMB_SIZE      (128 * 1024)
#define PLAY_BUF_SIZE   (256 * 1024)     /* gallery: a thumbnail or one video frame */
#define CHUNK_SIZE      (16 * 1024)
#define REC_MAX_MS      (10 * 60 * 1000) /* stop recording after 10 minutes */
#define REC_MIN_FREE    (16ULL << 20)    /* or when the card is nearly full */
#define STATUS_EVERY_MS 500
#define BOOT_SCREEN_MS  1300
#define DIM_MS          30000            /* idle: dim the screen */
#define BL_DIM          8                /* backlight % when dimmed */
#define PLAY_AFTER_MS   700              /* gallery: start a video once you stop on it */
#define BATT_STOP_MV    3350             /* stop recording cleanly before the module cuts out */
#define PHOTO_KB        240              /* typical 2048x1536 photo, for "shots left" */
#define VIDEO_KB_S      280              /* typical VGA video, for "minutes left" */

typedef enum { MODE_PHOTO, MODE_VIDEO } mode_t_;
typedef enum { VIEW_PREVIEW, VIEW_MODE, VIEW_GALLERY, VIEW_SETTINGS, VIEW_PHONE } view_t_;

static QueueHandle_t s_cmds, s_btns;
static SemaphoreHandle_t s_slot_ready;

/* State owned by cam_task */
static mode_t_ s_mode = MODE_PHOTO;
static view_t_ s_view = VIEW_PREVIEW;
static int s_look;
static bool s_power = true;
static bool s_stream;             /* laptop watching: set by "hello" / "stream" */
static bool s_cam_ok, s_lcd_ok;
static avi_writer_t *s_rec;
static int64_t s_rec_start_us;
static uint8_t *s_thumb;          /* latest preview frame, used as the photo/video thumbnail */
static size_t s_thumb_len;
static uint8_t *s_rec_thumb;
static size_t s_rec_thumb_len;
static int s_photos, s_videos;
static uint64_t s_sd_total, s_sd_free;
static float s_fps, s_tx_fps;
static uint32_t s_frames, s_tx_frames;

/* Idle */
static int64_t s_last_input_us;
static int s_idle;                /* 0 awake, 1 dimmed, 2 asleep */

/* Gallery */
static storage_item_t *s_items;
static int s_nitems, s_gi;
static bool s_confirm;
static int64_t s_item_us;
static uint8_t *s_gbuf;
static FILE *s_play;
static uint32_t s_play_us_per_frame;
static int64_t s_play_next;

/* Preview slot shared with tx_task */
static uint8_t *s_slot;
static size_t s_slot_len;
static uint16_t s_slot_w, s_slot_h;
static volatile bool s_slot_busy;

static void refresh_card_info(void)
{
    storage_space(&s_sd_total, &s_sd_free);
    storage_counts(&s_photos, &s_videos);
}

/* Replies go to the laptop. id 0 = the action came from a button, so nobody's waiting. */
static void reply(int id, bool ok, const char *extra_fmt, ...)
{
    if (id <= 0) return;
    char extra[512] = "";
    if (extra_fmt) {
        va_list ap;
        va_start(ap, extra_fmt);
        vsnprintf(extra, sizeof(extra), extra_fmt, ap);
        va_end(ap);
    }
    char buf[640];
    snprintf(buf, sizeof(buf), "{\"id\":%d,\"ok\":%s%s%s}", id, ok ? "true" : "false",
             extra[0] ? "," : "", extra);
    link_send_json('R', buf);
}

/* Tell whoever asked: the laptop gets an error reply, the screen a message. */
static void fail(int id, const char *msg)
{
    reply(id, false, "\"error\":\"%s\"", msg);
    ui_toast(msg, 1500);
}

static void send_status(void)
{
    uint32_t rec_ms = s_rec ? (uint32_t)((esp_timer_get_time() - s_rec_start_us) / 1000) : 0;
    char batt[48];
    if (battery_present()) snprintf(batt, sizeof(batt), "\"%d%% (%.2f V)\"", battery_percent(), battery_mv() / 1000.0);
    else snprintf(batt, sizeof(batt), "\"USB\"");
    char buf[600];
    snprintf(buf, sizeof(buf),
             "{\"mode\":\"%s\",\"look\":%d,\"power\":%d,\"stream\":%d,\"cam\":%d,\"lcd\":%d,"
             "\"view\":\"%s\",\"rec\":%d,\"rec_ms\":%lu,\"rec_frames\":%lu,\"rec_kb\":%lu,"
             "\"fps\":%.1f,\"tx_fps\":%.1f,\"sd\":%d,\"sd_total_mb\":%llu,\"sd_free_mb\":%llu,"
             "\"photos\":%d,\"videos\":%d,\"battery\":%s,\"battery_mv\":%d,\"psram_kb\":%u,"
             "\"internal_kb\":%u,\"phone_min_kb\":%d,\"uptime_s\":%lld}",
             s_mode == MODE_PHOTO ? "photo" : "video", s_look, s_power, s_stream, s_cam_ok, s_lcd_ok,
             (const char *[]){ "preview", "mode", "gallery", "settings", "phone" }[s_view],
             s_rec != NULL, (unsigned long)rec_ms,
             (unsigned long)(s_rec ? avi_frames(s_rec) : 0),
             (unsigned long)(s_rec ? avi_bytes(s_rec) >> 10 : 0),
             s_fps, s_tx_fps, storage_ready(), s_sd_total >> 20, s_sd_free >> 20,
             s_photos, s_videos, batt, battery_present() ? battery_mv() : 0,
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) >> 10),
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) >> 10),
             phone_heap_min_kb(), esp_timer_get_time() / 1000000);
    link_send_json('S', buf);
}

/* ---------------------------------------------------------------- settings (kept in flash) */

typedef struct {
    uint8_t version;
    uint8_t timer;       /* index into TIMER_S */
    uint8_t grid;
    uint8_t brightness;  /* index into BRIGHT_PCT */
    uint8_t sleep;       /* index into SLEEP_S */
} settings_t;

#define SETTINGS_VERSION 1
static settings_t s_set = { SETTINGS_VERSION, 0, 0, 1, 1 };
static const int TIMER_S[] = { 0, 3, 10 };
static const char *const TIMER_TXT[] = { "Off", "3 s", "10 s" };
static const int BRIGHT_PCT[] = { 30, 60, 100 };
static const char *const BRIGHT_TXT[] = { "Low", "Mid", "High" };
static const int SLEEP_S[] = { 60, 120, 300, 0 };
static const char *const SLEEP_TXT[] = { "1 min", "2 min", "5 min", "Never" };

static void settings_load(void)
{
    nvs_handle_t h;
    if (nvs_open("digicam", NVS_READONLY, &h) != ESP_OK) return;   /* first boot: defaults */
    settings_t t;
    size_t n = sizeof(t);
    if (nvs_get_blob(h, "cfg", &t, &n) == ESP_OK && n == sizeof(t) && t.version == SETTINGS_VERSION &&
        t.timer < 3 && t.grid < 2 && t.brightness < 3 && t.sleep < 4) {
        s_set = t;
    }
    nvs_close(h);
}

static void settings_save(void)
{
    nvs_handle_t h;
    if (nvs_open("digicam", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_blob(h, "cfg", &s_set, sizeof(s_set));
    nvs_commit(h);
    nvs_close(h);
}

static int bl_on(void)
{
    return BRIGHT_PCT[s_set.brightness];
}

/* ---------------------------------------------------------------- screen state */

static int64_t s_timer_end;       /* self-timer running until this time, else 0 */

static void ui_sync(void)
{
    ui_state_t *u = ui_lock();
    u->video = s_mode == MODE_VIDEO;
    u->rec = s_rec != NULL;
    u->rec_ms = s_rec ? (uint32_t)((esp_timer_get_time() - s_rec_start_us) / 1000) : 0;
    u->look = s_look;
    u->sd = storage_ready();
    u->left = s_mode == MODE_PHOTO ? (int)((s_sd_free >> 10) / PHOTO_KB)
                                   : (int)((s_sd_free >> 10) / VIDEO_KB_S / 60);
    u->batt = battery_present();
    u->batt_pct = battery_percent();
    u->grid = s_set.grid;
    u->timer_s = TIMER_S[s_set.timer];
    u->countdown = s_timer_end ? (int)((s_timer_end - esp_timer_get_time() + 999999) / 1000000) : 0;
    ui_unlock();
}

static void ui_set_view(ui_view_t v)
{
    ui_state_t *u = ui_lock();
    u->view = v;
    ui_unlock();
}

static void ui_set_busy(bool busy)
{
    ui_state_t *u = ui_lock();
    u->busy = busy;
    ui_unlock();
}

/* ---------------------------------------------------------------- camera actions */

static esp_err_t set_mode(mode_t_ mode)
{
    esp_err_t err = (mode == MODE_PHOTO) ? lens_set_size(LENS_PREVIEW_PHOTO, 14)
                                         : lens_set_size(LENS_PREVIEW_VIDEO, 12);
    if (err == ESP_OK) {
        s_mode = mode;
        lens_flush(2);
    }
    ui_sync();
    return err;
}

static bool set_look(int idx)
{
    int n = lens_look_count();
    idx = (idx % n + n) % n;
    if (!s_cam_ok || lens_apply_look(idx) != ESP_OK) return false;
    s_look = idx;
    ui_sync();
    ui_toast(lens_look_name(idx), 1100);
    return true;
}

static void take_photo(int id)
{
    if (!s_cam_ok || !s_power) { fail(id, "camera off"); return; }
    if (s_mode != MODE_PHOTO || s_rec) { fail(id, "not in photo mode"); return; }
    if (!storage_ready()) { fail(id, "no SD card"); return; }

    ui_flash();
    ui_set_busy(true);
    int64_t t0 = esp_timer_get_time();
    if (lens_set_size(LENS_STILL, 8) != ESP_OK) {
        ui_set_busy(false);
        fail(id, "sensor switch failed");
        return;
    }
    lens_flush(2);   /* frames queued at preview size, plus one for exposure to settle */
    camera_fb_t *fb = lens_grab();
    int64_t t_capture = esp_timer_get_time();

    char name[STORAGE_NAME_LEN] = "";
    esp_err_t err = fb ? storage_save_photo(fb->buf, fb->len, s_thumb, s_thumb_len, name) : ESP_FAIL;
    size_t size = fb ? fb->len : 0;
    int w = fb ? fb->width : 0, h = fb ? fb->height : 0;
    lens_release(fb);
    int64_t t_saved = esp_timer_get_time();

    set_mode(MODE_PHOTO);
    ui_set_busy(false);
    if (err != ESP_OK) { fail(id, "save failed"); return; }

    s_photos++;
    s_sd_free = s_sd_free > size ? s_sd_free - size : 0;
    ui_sync();
    ui_toast(name, 900);
    ESP_LOGI(TAG, "%s %dx%d %u KB, capture %lld ms, save %lld ms", name, w, h,
             (unsigned)(size >> 10), (t_capture - t0) / 1000, (t_saved - t_capture) / 1000);
    reply(id, true, "\"name\":\"%s\",\"size\":%u,\"w\":%d,\"h\":%d,\"capture_ms\":%lld,\"save_ms\":%lld",
          name, (unsigned)size, w, h, (t_capture - t0) / 1000, (t_saved - t_capture) / 1000);
}

static void rec_start(int id)
{
    if (!s_cam_ok || !s_power) { fail(id, "camera off"); return; }
    if (s_mode != MODE_VIDEO) { fail(id, "not in video mode"); return; }
    if (s_rec) { fail(id, "already recording"); return; }
    if (phone_running()) { fail(id, "Phone mode is on"); return; }   /* they share a buffer */
    if (!storage_ready() || s_sd_free < REC_MIN_FREE) { fail(id, "SD card missing or full"); return; }

    s_rec = avi_open(storage_video_tmp_path(), 640, 480);
    if (!s_rec) { fail(id, "cannot create video file"); return; }
    s_rec_start_us = esp_timer_get_time();
    s_rec_thumb_len = 0;
    ui_sync();
    ESP_LOGI(TAG, "recording started");
    reply(id, true, NULL);
}

/* id 0 = stopped by a button or the camera itself (time, space or battery limit, power off) */
static void rec_stop(int id, const char *why)
{
    if (!s_rec) { if (id) reply(id, false, "\"error\":\"not recording\""); return; }
    uint32_t ms = (uint32_t)((esp_timer_get_time() - s_rec_start_us) / 1000);
    uint32_t frames = avi_frames(s_rec);
    uint32_t bytes = avi_bytes(s_rec);
    ui_set_busy(true);
    esp_err_t err = avi_close(s_rec, ms);
    s_rec = NULL;

    char name[STORAGE_NAME_LEN] = "";
    if (err == ESP_OK && frames > 0) {
        err = storage_commit_video(s_rec_thumb, s_rec_thumb_len, name);
    } else {
        err = ESP_FAIL;
    }
    refresh_card_info();
    ui_set_busy(false);
    ui_sync();
    if (err != ESP_OK) {
        fail(id, "video save failed");
        return;
    }
    char msg[40];
    snprintf(msg, sizeof(msg), strcmp(why, "stopped") ? "%s (%s)" : "%s", name, why);
    ui_toast(msg, 1500);
    ESP_LOGI(TAG, "%s: %lu frames in %lu ms (%s)", name, (unsigned long)frames, (unsigned long)ms, why);
    if (id) {
        reply(id, true, "\"name\":\"%s\",\"frames\":%lu,\"ms\":%lu,\"size\":%lu",
              name, (unsigned long)frames, (unsigned long)ms, (unsigned long)bytes);
    } else {
        char buf[200];
        snprintf(buf, sizeof(buf), "{\"id\":0,\"event\":\"rec_stopped\",\"why\":\"%s\",\"name\":\"%s\","
                 "\"frames\":%lu,\"ms\":%lu}", why, name, (unsigned long)frames, (unsigned long)ms);
        link_send_json('R', buf);
    }
}

/* ---------------------------------------------------------------- on-camera gallery */

static void play_stop(void)
{
    if (s_play) fclose(s_play);
    s_play = NULL;
}

static void show_item(void)
{
    play_stop();
    s_confirm = false;
    s_item_us = esp_timer_get_time();
    ui_state_t *u = ui_lock();
    u->g_count = s_nitems;
    u->g_pos = s_gi;
    u->g_confirm = false;
    u->g_playing = false;
    u->g_video = s_nitems && s_items[s_gi].is_video;
    snprintf(u->g_name, sizeof(u->g_name), "%s", s_nitems ? s_items[s_gi].name : "");
    ui_unlock();
    if (!s_nitems) return;

    uint32_t size = 0;
    FILE *f = storage_open(s_items[s_gi].name, true, &size);
    if (!f) return;
    size_t n = size <= PLAY_BUF_SIZE ? fread(s_gbuf, 1, size, f) : 0;
    fclose(f);
    if (n) ui_frame(s_gbuf, n, true);
}

static void gallery_open(void)
{
    if (s_rec) { ui_toast("recording...", 1000); return; }
    free(s_items);
    s_items = NULL;
    s_nitems = 0;
    if (storage_list(&s_items, &s_nitems) != ESP_OK) { ui_toast("no SD card", 1500); return; }
    s_view = VIEW_GALLERY;
    s_gi = 0;
    ui_set_view(UI_GALLERY);
    show_item();
}

static void gallery_close(void)
{
    play_stop();
    free(s_items);
    s_items = NULL;
    s_nitems = 0;
    s_view = VIEW_PREVIEW;
    ui_set_view(UI_PREVIEW);
    lens_flush(1);   /* drop the frame that sat in the queue while browsing */
}

static void gallery_delete(void)
{
    play_stop();
    if (storage_delete(s_items[s_gi].name) != ESP_OK) {
        ui_toast("delete failed", 1500);
    } else {
        ui_toast("deleted", 900);
        memmove(&s_items[s_gi], &s_items[s_gi + 1], (s_nitems - s_gi - 1) * sizeof(storage_item_t));
        s_nitems--;
        if (s_gi >= s_nitems) s_gi = s_nitems ? s_nitems - 1 : 0;
        refresh_card_info();
        ui_sync();
    }
    show_item();
}

/* One step of video playback: open the AVI once you've stopped on it, then show
 * frames at the recorded rate, looping. */
static void gallery_step(void)
{
    if (!s_nitems || !s_items[s_gi].is_video || s_confirm) { vTaskDelay(pdMS_TO_TICKS(20)); return; }
    int64_t now = esp_timer_get_time();
    if (!s_play) {
        if (now - s_item_us < PLAY_AFTER_MS * 1000) { vTaskDelay(pdMS_TO_TICKS(20)); return; }
        uint32_t size;
        s_play = storage_open(s_items[s_gi].name, false, &size);
        if (!s_play) { vTaskDelay(pdMS_TO_TICKS(20)); return; }
        uint8_t hdr[36];
        s_play_us_per_frame = 66666;
        if (fread(hdr, 1, sizeof(hdr), s_play) == sizeof(hdr)) {
            uint32_t us = hdr[32] | hdr[33] << 8 | hdr[34] << 16 | (uint32_t)hdr[35] << 24;
            if (us >= 10000 && us <= 1000000) s_play_us_per_frame = us;
        }
        fseek(s_play, AVI_HDR_SIZE, SEEK_SET);
        s_play_next = now;
        ui_state_t *u = ui_lock();
        u->g_playing = true;
        ui_unlock();
    }
    if (now < s_play_next) { vTaskDelay(pdMS_TO_TICKS(5)); return; }

    for (int tries = 0; tries < 2; tries++) {
        uint8_t ch[8];
        if (fread(ch, 1, 8, s_play) == 8 && memcmp(ch, "00dc", 4) == 0) {
            uint32_t len = ch[4] | ch[5] << 8 | ch[6] << 16 | (uint32_t)ch[7] << 24;
            if (len > PLAY_BUF_SIZE || fread(s_gbuf, 1, len, s_play) != len) break;
            if (len & 1) fseek(s_play, 1, SEEK_CUR);
            ui_frame(s_gbuf, len, true);
            s_play_next += s_play_us_per_frame;
            if (s_play_next < now) s_play_next = now;   /* screen slower than the video: skip ahead in time */
            return;
        }
        fseek(s_play, AVI_HDR_SIZE, SEEK_SET);          /* end of the frames: loop */
    }
    play_stop();   /* not a video we can read */
    s_item_us = INT64_MAX / 2;                          /* and don't retry it */
}

/* ---------------------------------------------------------------- buttons */

static void wake(void)
{
    s_last_input_us = esp_timer_get_time();
    if (s_idle == 2) lens_flush(1);
    s_idle = 0;
    if (s_power) ui_backlight(bl_on());
}

/* ---- mode dial: hold ● in live view. ◀ ▶ choose, ● or shutter picks, hold ● cancels */

static int s_mode_sel;

static void mode_show(void)
{
    ui_state_t *u = ui_lock();
    u->view = UI_MODE;
    u->mode_sel = s_mode_sel;
    ui_unlock();
}

static void back_to_preview(void)
{
    s_view = VIEW_PREVIEW;
    ui_set_view(UI_PREVIEW);
}

/* ---- settings: ◀ ▶ move, ● changes the value or opens it, shutter goes back */

enum { ROW_TIMER, ROW_GRID, ROW_BRIGHT, ROW_SLEEP, ROW_FORMAT, ROW_ABOUT, ROW_COUNT };
static int s_sel;
static bool s_about, s_dialog;

static const char *sensor_name(void)
{
    switch (lens_sensor_pid()) {
    case 0x3660: return "OV3660";
    case 0x2642: return "OV2640";
    case 0x5640: return "OV5640";
    default:     return s_cam_ok ? "other" : "not found";
    }
}

static void settings_show(void)
{
    ui_state_t *u = ui_lock();
    u->view = UI_SETTINGS;
    u->dialog = s_dialog;
    u->nrows = 0;
#define ROW(ic, lab, ...) do { ui_row_t *r = &u->rows[u->nrows++]; r->icon = (ic); \
        snprintf(r->label, sizeof(r->label), "%s", lab); snprintf(r->value, sizeof(r->value), __VA_ARGS__); } while (0)
    if (s_about) {
        snprintf(u->title, sizeof(u->title), "About");
        ROW(NULL, "Firmware", "%s", FW_VERSION);
        ROW(NULL, "Camera", "%s", sensor_name());
        if (storage_ready()) ROW(NULL, "Card free", "%.1f GB", s_sd_free / 1e9);
        else ROW(NULL, "Card", "none");
        ROW(NULL, "Photos", "%d", s_photos);
        ROW(NULL, "Videos", "%d", s_videos);
        if (battery_present()) ROW(NULL, "Battery", "%.2f V", battery_mv() / 1000.0);
        else ROW(NULL, "Power", "USB");
        u->sel = -1;
    } else {
        snprintf(u->title, sizeof(u->title), "Settings");
        ROW(ICON_TIMER, "Self-timer", "%s", TIMER_TXT[s_set.timer]);
        ROW(ICON_GRID, "Grid", "%s", s_set.grid ? "On" : "Off");
        ROW(ICON_SUN, "Brightness", "%s", BRIGHT_TXT[s_set.brightness]);
        ROW(ICON_MOON, "Sleep after", "%s", SLEEP_TXT[s_set.sleep]);
        ROW(ICON_CARD, "Format card", "%s", "");
        ROW(ICON_INFO, "About", "%s", "");
        u->sel = s_sel;
    }
#undef ROW
    ui_unlock();
}

static void settings_open(void)
{
    s_view = VIEW_SETTINGS;
    s_sel = 0;
    s_about = s_dialog = false;
    settings_show();
}

static void settings_button(btn_event_t e)
{
    if (s_about) {                       /* any button leaves About */
        s_about = false;
        settings_show();
        return;
    }
    if (s_dialog) {                      /* "Format the card?": ● formats, anything else keeps */
        s_dialog = false;
        settings_show();
        if (e == BTN_OK) {
            ui_toast("formatting...", 10000);
            esp_err_t err = storage_format();
            refresh_card_info();
            ui_sync();
            ui_toast(err == ESP_OK ? "card formatted" : "format failed", 1500);
        }
        return;
    }
    switch (e) {
    case BTN_LEFT:  s_sel = (s_sel + ROW_COUNT - 1) % ROW_COUNT; break;
    case BTN_RIGHT: s_sel = (s_sel + 1) % ROW_COUNT; break;
    case BTN_OK:
        switch (s_sel) {
        case ROW_TIMER:  s_set.timer = (s_set.timer + 1) % 3; break;
        case ROW_GRID:   s_set.grid = !s_set.grid; break;
        case ROW_BRIGHT: s_set.brightness = (s_set.brightness + 1) % 3; ui_backlight(bl_on()); break;
        case ROW_SLEEP:  s_set.sleep = (s_set.sleep + 1) % 4; break;
        case ROW_FORMAT:
            if (storage_ready()) s_dialog = true;
            else ui_toast("no SD card", 1500);
            break;
        case ROW_ABOUT:  s_about = true; break;
        }
        settings_save();
        ui_sync();
        break;
    case BTN_SHUTTER:
        back_to_preview();
        lens_flush(1);
        return;
    default:
        break;
    }
    settings_show();
}

/* ---- phone mode: the camera's own Wi-Fi and photo page (phone.c) */

static char s_ssid[33], s_pass[16];
static int s_phone_step;
static volatile bool s_card_dirty;            /* the phone deleted something */
static uint8_t s_qr[UI_QR_MAX * UI_QR_MAX];
static int s_qr_size;

/* Wi-Fi name from the owner's name ("digicam-maya"), and a password made once and kept */
static void phone_creds(void)
{
    char nm[16];
    int k = 0;
    for (const char *p = OWNER_NAME; *p && k < 12; p++) {
        char c = *p;
        if (c >= 'A' && c <= 'Z') c += 32;
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) nm[k++] = c;
    }
    nm[k] = 0;
    if (k) {
        snprintf(s_ssid, sizeof(s_ssid), "digicam-%s", nm);
    } else {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
        snprintf(s_ssid, sizeof(s_ssid), "digicam-%02x%02x", mac[4], mac[5]);
    }
    nvs_handle_t h;
    if (nvs_open("digicam", NVS_READWRITE, &h) != ESP_OK) return;
    size_t n = sizeof(s_pass);
    if (nvs_get_str(h, "wifi_pass", s_pass, &n) != ESP_OK || strlen(s_pass) < 8) {
        static const char letters[] = "abcdefghjkmnpqrstuvwxyz23456789";   /* no 0/o, 1/l/i */
        for (int i = 0; i < 8; i++) s_pass[i] = letters[esp_random() % (sizeof(letters) - 1)];
        s_pass[8] = 0;
        nvs_set_str(h, "wifi_pass", s_pass);
        nvs_commit(h);
    }
    nvs_close(h);
}

static void qr_capture(esp_qrcode_handle_t q)
{
    int n = esp_qrcode_get_size(q);
    s_qr_size = n <= UI_QR_MAX ? n : 0;
    for (int y = 0; y < s_qr_size; y++)
        for (int x = 0; x < s_qr_size; x++) s_qr[y * s_qr_size + x] = esp_qrcode_get_module(q, x, y);
}

/* step 1: QR to join the Wi-Fi. step 2 (a phone has joined): QR to open the page */
static void phone_show(int step)
{
    s_phone_step = step;
    char text[96];
    if (step == 1) snprintf(text, sizeof(text), "WIFI:T:WPA;S:%s;P:%s;;", s_ssid, s_pass);
    else snprintf(text, sizeof(text), "%s", PHONE_URL);
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    cfg.display_func = qr_capture;
    cfg.max_qrcode_version = 6;
    s_qr_size = 0;
    esp_qrcode_generate(&cfg, text);
    ui_state_t *u = ui_lock();
    u->view = UI_PHONE;
    u->phone_step = step;
    u->clients = phone_clients();
    snprintf(u->ssid, sizeof(u->ssid), "%s", s_ssid);
    snprintf(u->pass, sizeof(u->pass), "%s", s_pass);
    u->qr_size = s_qr_size;
    memcpy(u->qr, s_qr, s_qr_size * s_qr_size);
    ui_unlock();
}

/* from the web server's task */
static void on_phone_activity(const char *msg, bool deleted)
{
    ui_toast(msg, 3000);
    if (deleted) s_card_dirty = true;
    s_last_input_us = esp_timer_get_time();
}

static void phone_open(void)
{
    phone_creds();
    if (phone_start(s_ssid, s_pass, OWNER_NAME) != ESP_OK) {
        back_to_preview();
        ui_toast("Wi-Fi didn't start", 2000);
        return;
    }
    s_view = VIEW_PHONE;
    phone_show(1);
}

static void phone_close(void)
{
    phone_stop();
    refresh_card_info();
    ui_sync();
    back_to_preview();
    lens_flush(1);
}

/* ---- self-timer */

static void timer_cancel(const char *why)
{
    s_timer_end = 0;
    ui_sync();
    if (why) ui_toast(why, 1000);
}

static void on_button(btn_event_t e)
{
    static const char *names[] = { "left", "right", "ok", "hold", "shutter" };
    ESP_LOGI(TAG, "button: %s", names[e]);
    bool was_asleep = s_idle == 2;
    wake();
    if (was_asleep || !s_power) return;   /* a press on a dark screen only wakes it */

    if (s_view == VIEW_SETTINGS) { settings_button(e); return; }
    if (s_view == VIEW_PHONE) {
        if (e == BTN_SHUTTER || e == BTN_OK_HOLD) phone_close();
        else if (e == BTN_OK) phone_show(s_phone_step == 1 ? 2 : 1);   /* switch QR codes by hand */
        return;
    }
    if (!s_cam_ok) return;

    if (s_view == VIEW_MODE) {
        switch (e) {
        case BTN_LEFT:  s_mode_sel = (s_mode_sel + UI_MODES - 1) % UI_MODES; mode_show(); break;
        case BTN_RIGHT: s_mode_sel = (s_mode_sel + 1) % UI_MODES; mode_show(); break;
        case BTN_OK_HOLD: back_to_preview(); break;
        case BTN_OK:
        case BTN_SHUTTER:
            if (s_mode_sel == 3) { settings_open(); break; }
            if (s_mode_sel == 2) { phone_open(); break; }
            back_to_preview();
            mode_t_ want = s_mode_sel == 1 ? MODE_VIDEO : MODE_PHOTO;
            if (want != s_mode) {
                set_mode(want);
                ui_toast(want == MODE_VIDEO ? "video" : "photo", 900);
            }
            break;
        }
        return;
    }

    if (s_view == VIEW_GALLERY) {
        if (s_confirm) {
            if (e == BTN_OK) { gallery_delete(); return; }
            s_confirm = false;                          /* any other button keeps it */
            ui_state_t *u = ui_lock();
            u->g_confirm = false;
            ui_unlock();
            s_item_us = esp_timer_get_time();           /* a video starts playing again */
            return;
        }
        switch (e) {
        case BTN_LEFT:
        case BTN_RIGHT:
            if (!s_nitems) return;
            s_gi = (s_gi + (e == BTN_RIGHT ? 1 : -1) + s_nitems) % s_nitems;
            show_item();
            break;
        case BTN_OK_HOLD: {
            if (!s_nitems) return;
            play_stop();
            s_confirm = true;
            ui_state_t *u = ui_lock();
            u->g_confirm = true;
            u->g_playing = false;
            ui_unlock();
            break;
        }
        case BTN_OK:
        case BTN_SHUTTER:
            gallery_close();
            break;
        }
        return;
    }

    /* live view */
    if (s_timer_end) { timer_cancel("timer cancelled"); return; }   /* any button stops the countdown */
    switch (e) {
    case BTN_LEFT:  set_look(s_look - 1); break;
    case BTN_RIGHT: set_look(s_look + 1); break;
    case BTN_OK:    gallery_open(); break;
    case BTN_OK_HOLD:
        if (s_rec) { ui_toast("recording...", 1000); break; }
        s_view = VIEW_MODE;
        s_mode_sel = s_mode == MODE_VIDEO ? 1 : 0;
        mode_show();
        break;
    case BTN_SHUTTER:
        if (s_mode == MODE_PHOTO) {
            if (TIMER_S[s_set.timer] && storage_ready()) {
                s_timer_end = esp_timer_get_time() + TIMER_S[s_set.timer] * 1000000LL;
                ui_sync();
            } else {
                take_photo(0);
            }
        }
        else if (s_rec) rec_stop(0, "stopped");
        else rec_start(0);
        break;
    }
}

/* ---------------------------------------------------------------- laptop commands */

static void send_list(int id)
{
    storage_item_t *items;
    int n;
    if (storage_list(&items, &n) != ESP_OK) { reply(id, false, "\"error\":\"no SD card\""); return; }

    size_t cap = 64 + (size_t)n * 48;
    char *buf = heap_caps_malloc(cap, MALLOC_CAP_SPIRAM);
    if (!buf) { free(items); reply(id, false, "\"error\":\"out of memory\""); return; }
    size_t pos = snprintf(buf, cap, "{\"id\":%d,\"ok\":true,\"items\":[", id);
    for (int i = 0; i < n; i++) {
        pos += snprintf(buf + pos, cap - pos, "%s{\"n\":\"%s\",\"s\":%lu,\"v\":%d}",
                        i ? "," : "", items[i].name, (unsigned long)items[i].size, items[i].is_video);
    }
    snprintf(buf + pos, cap - pos, "]}");
    link_send_json('R', buf);
    free(buf);
    free(items);
}

static void send_file(int id, const char *name, bool thumb, uint32_t from)
{
    uint32_t size = 0;
    FILE *f = storage_open(name, thumb, &size);
    if (!f) { reply(id, false, "\"error\":\"not found\""); return; }
    if (from > size) from = size;
    reply(id, true, "\"size\":%lu,\"offset\":%lu", (unsigned long)size, (unsigned long)from);

    uint8_t *chunk = heap_caps_malloc(CHUNK_SIZE, MALLOC_CAP_SPIRAM);
    uint32_t off = from;
    fseek(f, from, SEEK_SET);
    while (chunk && off < size) {
        /* A new command cancels the transfer, so a big video never makes the camera
         * unresponsive. The laptop resumes from `offset` if it still wants the file. */
        if (uxQueueMessagesWaiting(s_cmds) > 0) {
            ESP_LOGI(TAG, "%s: transfer cancelled at %lu of %lu bytes", name, (unsigned long)off, (unsigned long)size);
            break;
        }
        size_t n = fread(chunk, 1, CHUNK_SIZE, f);
        if (n == 0) break;
        uint32_t head[3] = { (uint32_t)id, off, size };
        link_send('D', head, sizeof(head), chunk, n);
        off += n;
    }
    free(chunk);
    fclose(f);
}

/* The screen as raw RGB565 (big-endian), for checking the UI without looking at it */
static void send_screen(int id)
{
    uint32_t size = LCD_W * LCD_H * 2;
    uint8_t *buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    if (!buf) { reply(id, false, "\"error\":\"out of memory\""); return; }
    ui_snapshot(buf);
    reply(id, true, "\"size\":%lu,\"offset\":0,\"w\":%d,\"h\":%d", (unsigned long)size, LCD_W, LCD_H);
    for (uint32_t off = 0; off < size; off += CHUNK_SIZE) {
        uint32_t n = size - off < CHUNK_SIZE ? size - off : CHUNK_SIZE;
        uint32_t head[3] = { (uint32_t)id, off, size };
        link_send('D', head, sizeof(head), buf + off, n);
    }
    free(buf);
}

static void set_power(bool on)
{
    if (!on && s_rec) rec_stop(0, "power off");
    if (!on && s_view == VIEW_GALLERY) gallery_close();
    if (!on && s_view == VIEW_PHONE) phone_close();
    if (!on) { s_view = VIEW_PREVIEW; s_timer_end = 0; }
    s_power = on;
    ui_set_view(on ? UI_PREVIEW : UI_OFF);
    ui_backlight(on ? bl_on() : 0);
    if (on) wake();
    ESP_LOGI(TAG, "power %s", on ? "on" : "off");
}

static bool num_is(cJSON *j, const char *key, int value)
{
    cJSON *v = cJSON_GetObjectItem(j, key);
    return cJSON_IsTrue(v) || (cJSON_IsNumber(v) && v->valueint == value);
}

static void handle_command(const char *line)
{
    cJSON *j = cJSON_Parse(line);
    if (!j) return;
    int id = cJSON_GetNumberValue(cJSON_GetObjectItem(j, "id"));
    const char *cmd = cJSON_GetStringValue(cJSON_GetObjectItem(j, "cmd"));
    if (!cmd) { cJSON_Delete(j); return; }
    /* The laptop being used counts as activity; a "btn" press wakes the camera the way a real one does */
    if (strcmp(cmd, "btn") && strcmp(cmd, "shot")) wake();

    if (strcmp(cmd, "hello") == 0) {
        s_stream = true;
        char looks[256] = "";
        size_t pos = 0;
        for (int i = 0; i < lens_look_count(); i++) {
            pos += snprintf(looks + pos, sizeof(looks) - pos, "%s\"%s\"", i ? "," : "", lens_look_name(i));
        }
        reply(id, true, "\"fw\":\"%s\",\"sensor\":\"0x%04X\",\"cam\":%d,\"sd\":%d,\"lcd\":%d,\"looks\":[%s]",
              FW_VERSION, lens_sensor_pid(), s_cam_ok, storage_ready(), s_lcd_ok, looks);
        send_status();
    } else if (strcmp(cmd, "status") == 0) {
        reply(id, true, NULL);
        send_status();
    } else if (strcmp(cmd, "stream") == 0) {
        s_stream = cJSON_IsTrue(cJSON_GetObjectItem(j, "on"));
        reply(id, true, NULL);
    } else if (strcmp(cmd, "power") == 0) {
        set_power(cJSON_IsTrue(cJSON_GetObjectItem(j, "on")));
        reply(id, true, NULL);
    } else if (strcmp(cmd, "mode") == 0) {
        const char *m = cJSON_GetStringValue(cJSON_GetObjectItem(j, "mode"));
        if (s_rec) {
            reply(id, false, "\"error\":\"stop recording first\"");
        } else if (m && (strcmp(m, "photo") == 0 || strcmp(m, "video") == 0)) {
            bool ok = s_cam_ok && set_mode(strcmp(m, "photo") == 0 ? MODE_PHOTO : MODE_VIDEO) == ESP_OK;
            reply(id, ok, NULL);
        } else {
            reply(id, false, "\"error\":\"bad mode\"");
        }
    } else if (strcmp(cmd, "look") == 0) {
        bool ok = set_look(cJSON_GetNumberValue(cJSON_GetObjectItem(j, "index")));
        reply(id, ok, "\"look\":%d", s_look);
    } else if (strcmp(cmd, "photo") == 0) {
        take_photo(id);
    } else if (strcmp(cmd, "rec_start") == 0) {
        rec_start(id);
    } else if (strcmp(cmd, "rec_stop") == 0) {
        rec_stop(id, "stopped");
    } else if (strcmp(cmd, "list") == 0) {
        send_list(id);
    } else if (strcmp(cmd, "get") == 0) {
        const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(j, "name"));
        cJSON *from = cJSON_GetObjectItem(j, "offset");
        send_file(id, name ? name : "", cJSON_IsTrue(cJSON_GetObjectItem(j, "thumb")),
                  cJSON_IsNumber(from) ? (uint32_t)from->valuedouble : 0);
    } else if (strcmp(cmd, "delete") == 0) {
        const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(j, "name"));
        bool ok = name && storage_delete(name) == ESP_OK;
        if (ok) { refresh_card_info(); ui_sync(); }
        reply(id, ok, NULL);
    } else if (strcmp(cmd, "btn") == 0) {
        /* Press a button from the laptop: {"cmd":"btn","b":"left|right|ok|hold|shutter"} */
        static const char *names[] = { "left", "right", "ok", "hold", "shutter" };
        static const btn_event_t events[] = { BTN_LEFT, BTN_RIGHT, BTN_OK, BTN_OK_HOLD, BTN_SHUTTER };
        const char *b = cJSON_GetStringValue(cJSON_GetObjectItem(j, "b"));
        int e = -1;
        for (int i = 0; b && i < 5; i++) if (strcmp(b, names[i]) == 0) e = i;
        reply(id, e >= 0, NULL);
        if (e >= 0) on_button(events[e]);
    } else if (strcmp(cmd, "shot") == 0) {
        send_screen(id);
    } else if (strcmp(cmd, "lcd") == 0) {
        /* Line up a new screen module: {"cmd":"lcd","swap":1,"mx":1,"my":0,"gx":40,"gy":53,"inv":1} */
        display_tune(num_is(j, "swap", 1), num_is(j, "mx", 1), num_is(j, "my", 1),
                     cJSON_GetNumberValue(cJSON_GetObjectItem(j, "gx")),
                     cJSON_GetNumberValue(cJSON_GetObjectItem(j, "gy")), num_is(j, "inv", 1));
        reply(id, true, NULL);
    } else if (strcmp(cmd, "baud") == 0) {
        int rate = cJSON_GetNumberValue(cJSON_GetObjectItem(j, "rate"));
        bool ok = rate >= 115200 && rate <= 3000000;
        reply(id, ok, NULL);
        if (ok) link_set_baud(rate);
    } else if (strcmp(cmd, "reboot") == 0) {
        reply(id, true, NULL);
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_restart();
    } else {
        reply(id, false, "\"error\":\"unknown command\"");
    }
    cJSON_Delete(j);
}

static void on_line(const char *line)
{
    char *copy = strdup(line);
    if (copy && xQueueSend(s_cmds, &copy, 0) != pdTRUE) free(copy);
}

/* ---------------------------------------------------------------- tasks */

static void tx_task(void *arg)
{
    int64_t t_mark = esp_timer_get_time();
    uint32_t mark_frames = 0;
    for (;;) {
        if (xSemaphoreTake(s_slot_ready, pdMS_TO_TICKS(1000)) == pdTRUE) {
            uint16_t head[2] = { s_slot_w, s_slot_h };
            link_send('F', head, sizeof(head), s_slot, s_slot_len);
            s_tx_frames++;
            s_slot_busy = false;
        }
        int64_t now = esp_timer_get_time();
        if (now - t_mark >= 1000000) {
            s_tx_fps = (s_tx_frames - mark_frames) * 1e6f / (now - t_mark);
            mark_frames = s_tx_frames;
            t_mark = now;
        }
    }
}

/* Once a second: battery, idle dimming and sleep */
static void housekeeping(int64_t now)
{
    battery_update();
    bool batt = battery_present();
    if (s_rec && batt && battery_mv() < BATT_STOP_MV) rec_stop(0, "battery low");

    static int64_t t_warned;
    if (batt && battery_percent() <= 10 && now - t_warned > 60 * 1000000LL) {
        ui_toast("battery low", 2000);
        t_warned = now;
    }

    /* Stay awake while recording, counting down, or while the laptop is watching the stream */
    if (s_rec || s_timer_end || s_stream || buttons_any_down() || phone_clients() > 0) s_last_input_us = now;
    int64_t idle_ms = (now - s_last_input_us) / 1000;
    int64_t sleep_ms = SLEEP_S[s_set.sleep] * 1000LL;   /* 0: never */
    if (s_idle < 2 && sleep_ms && idle_ms >= sleep_ms) {
        if (s_view == VIEW_PHONE) phone_close();   /* nobody connected: Wi-Fi off before sleeping */
        s_idle = 2;
        play_stop();
        ui_backlight(0);
        ESP_LOGI(TAG, "idle: screen off, camera paused");
    } else if (s_idle < 1 && idle_ms >= DIM_MS) {
        s_idle = 1;
        ui_backlight(BL_DIM);
    }
    ui_sync();
}

static void cam_task(void *arg)
{
    int64_t t_status = 0, t_mark = esp_timer_get_time(), t_house = 0, t_rec_ui = 0;
    uint32_t mark_frames = 0;
    for (;;) {
        char *line;
        while (xQueueReceive(s_cmds, &line, 0) == pdTRUE) {
            handle_command(line);
            free(line);
        }
        btn_event_t e;
        while (xQueueReceive(s_btns, &e, 0) == pdTRUE) on_button(e);

        int64_t now = esp_timer_get_time();
        if (now - t_status >= STATUS_EVERY_MS * 1000) {
            if (now - t_mark >= 1000000) {
                s_fps = (s_frames - mark_frames) * 1e6f / (now - t_mark);
                mark_frames = s_frames;
                t_mark = now;
            }
            if (s_stream) send_status();
            t_status = now;
        }
        if (now - t_house >= 1000000) {
            housekeeping(now);
            t_house = now;
        }
        if ((s_rec || s_timer_end) && now - t_rec_ui >= 200000) {
            ui_sync();   /* recording time, or the self-timer's count */
            t_rec_ui = now;
        }
        if (s_timer_end && now >= s_timer_end) {
            timer_cancel(NULL);
            take_photo(0);
        }

        if (!s_power || !s_cam_ok || s_idle == 2) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        if (s_view == VIEW_GALLERY) {
            gallery_step();
            continue;
        }
        if (s_view == VIEW_PHONE) {
            static int64_t t_phone;
            if (now - t_phone >= 500000) {
                t_phone = now;
                phone_tick();
                int step = phone_clients() > 0 ? 2 : 1;
                if (step != s_phone_step && (step == 2 || s_phone_step == 2)) phone_show(step);
                ui_state_t *u = ui_lock();
                u->clients = phone_clients();
                ui_unlock();
                if (s_card_dirty) { s_card_dirty = false; refresh_card_info(); }
            }
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        if (s_view == VIEW_SETTINGS) {   /* no live view behind the menu */
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }

        camera_fb_t *fb = lens_grab();
        if (!fb) continue;
        s_frames++;

        if (s_rec) {
            if (s_rec_thumb_len == 0 && fb->len <= THUMB_SIZE) {
                memcpy(s_rec_thumb, fb->buf, fb->len);
                s_rec_thumb_len = fb->len;
            }
            if (avi_add_frame(s_rec, fb->buf, fb->len) != ESP_OK) {
                lens_release(fb);
                rec_stop(0, "write error");
                continue;
            }
        } else if (s_mode == MODE_PHOTO && fb->len <= THUMB_SIZE) {
            memcpy(s_thumb, fb->buf, fb->len);
            s_thumb_len = fb->len;
        }

        if (s_lcd_ok) ui_frame(fb->buf, fb->len, false);
        if (s_stream && !s_slot_busy && fb->len <= SLOT_SIZE) {
            memcpy(s_slot, fb->buf, fb->len);
            s_slot_len = fb->len;
            s_slot_w = fb->width;
            s_slot_h = fb->height;
            s_slot_busy = true;
            xSemaphoreGive(s_slot_ready);
        }
        lens_release(fb);

        if (s_rec) {
            uint32_t ms = (uint32_t)((esp_timer_get_time() - s_rec_start_us) / 1000);
            if (ms >= REC_MAX_MS) rec_stop(0, "time limit");
            else if (s_sd_free < REC_MIN_FREE + avi_bytes(s_rec)) rec_stop(0, "card full");
        }
    }
}

void app_main(void)
{
    s_cmds = xQueueCreate(8, sizeof(char *));
    s_btns = xQueueCreate(16, sizeof(btn_event_t));
    s_slot_ready = xSemaphoreCreateBinary();
    s_slot = heap_caps_malloc(SLOT_SIZE, MALLOC_CAP_SPIRAM);
    s_thumb = heap_caps_malloc(THUMB_SIZE, MALLOC_CAP_SPIRAM);
    s_rec_thumb = heap_caps_malloc(THUMB_SIZE, MALLOC_CAP_SPIRAM);
    s_gbuf = heap_caps_malloc(PLAY_BUF_SIZE, MALLOC_CAP_SPIRAM);

    link_init(on_line);
    ESP_LOGI(TAG, "digicam firmware %s starting", FW_VERSION);

    esp_err_t nvs = nvs_flash_init();
    if (nvs == ESP_ERR_NVS_NO_FREE_PAGES || nvs == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    settings_load();
    phone_on_activity(on_phone_activity);

    /* Screen first, so there's something to look at while the rest starts */
    s_lcd_ok = display_init() == ESP_OK;
    if (!s_lcd_ok) ESP_LOGE(TAG, "screen failed to start");
    ui_init();
    ui_state_t *u = ui_lock();
    u->view = UI_BOOT;
    if (OWNER_NAME[0]) snprintf(u->line1, sizeof(u->line1), "hi, %s", OWNER_NAME);
    else snprintf(u->line1, sizeof(u->line1), "ready when you are");
    ui_unlock();
    ui_backlight(bl_on());
    int64_t t_boot = esp_timer_get_time();

    battery_init();
    buttons_init(s_btns);
    if (storage_init() == ESP_OK) refresh_card_info();
    s_cam_ok = lens_init() == ESP_OK;
    if (!s_cam_ok) ESP_LOGE(TAG, "camera failed to start: check the ribbon cable and extension");
    ui_sync();

    int64_t shown = (esp_timer_get_time() - t_boot) / 1000;
    if (shown < BOOT_SCREEN_MS) vTaskDelay(pdMS_TO_TICKS(BOOT_SCREEN_MS - shown));
    u = ui_lock();
    if (s_cam_ok) {
        u->view = UI_PREVIEW;
    } else {
        u->view = UI_MESSAGE;
        snprintf(u->line1, sizeof(u->line1), "camera not found");
        snprintf(u->line2, sizeof(u->line2), "check the ribbon cable");
    }
    ui_unlock();
    if (s_cam_ok && !storage_ready()) ui_toast("no SD card", 3000);
    s_last_input_us = esp_timer_get_time();

    link_send_json('R', "{\"id\":0,\"event\":\"boot\"}");
    xTaskCreatePinnedToCore(tx_task, "tx", 4096, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(cam_task, "cam", 8192, NULL, 5, NULL, 1);
}
