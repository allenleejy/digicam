/*
 * phone.c: the camera's own Wi-Fi network (WPA2) and a small web server for the phone.
 *
 *   GET  /                 the photo page (web/phone.html, built into the firmware)
 *   GET  /icon-180.png     its Home Screen icon
 *   GET  /api/list         {"camera","owner","battery","items":[{"n","s","v"}]} newest first
 *   GET  /thumb/NAME       320x240 preview JPEG
 *   GET  /file/NAME        the photo or video
 *   GET  /small/NAME?w=&h= the photo made to fill w x h, for the DigiGallery frame
 *   POST /api/delete       body: NAME
 *   GET  /api/frame        the frame mailbox, as stored (204 when there's none)
 *   POST /api/frame        body: the whole mailbox document, at most 16 KB
 *
 * All camera and SD work otherwise happens on cam_task; in Phone mode the camera is
 * idle, so the web server's task is the only one reading the card.
 */

#include "phone.h"
#include "battery.h"
#include "small.h"
#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "cJSON.h"

static const char *TAG = "phone";

extern const char phone_html_start[] asm("_binary_phone_html_start");
extern const char phone_html_end[] asm("_binary_phone_html_end");
extern const uint8_t icon_png_start[] asm("_binary_icon_180_png_start");
extern const uint8_t icon_png_end[] asm("_binary_icon_180_png_end");

/* Reads go into internal RAM: the SD driver can't DMA into PSRAM and falls back to one
 * 512-byte sector per transfer. The web server runs handlers one at a time on its own
 * task, so one buffer is enough: the one storage sets aside, shared with video recording. */
#define CHUNK STORAGE_MEDIA_BUF
#define FRAME_MAX CHUNK    /* the mailbox fits the read buffer, so it's received and sent in one piece */

static uint8_t *s_chunk;
static bool s_inited, s_running;
static httpd_handle_t s_http;
static int s_heap_min = -1;
static int64_t s_heap_logged_us;
static volatile int s_clients;
static void (*s_activity)(const char *, bool);
static char s_ssid[33], s_owner[24];

void phone_on_activity(void (*cb)(const char *, bool)) { s_activity = cb; }
int phone_clients(void) { return s_clients; }
int phone_heap_min_kb(void) { return s_heap_min < 0 ? -1 : s_heap_min >> 10; }

/* Internal RAM is the scarce kind in Phone mode: keep the low-water mark, log it while busy */
static void heap_sample(const char *why)
{
    int free_b = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    if (s_heap_min < 0 || free_b < s_heap_min) s_heap_min = free_b;
    int64_t now = esp_timer_get_time();
    if (why || (s_clients > 0 && now - s_heap_logged_us > 10000000)) {
        s_heap_logged_us = now;
        ESP_LOGI(TAG, "heap%s%s: internal free %d KB, largest block %u KB, lowest %d KB, clients %d",
                 why ? " after " : "", why ? why : "", free_b >> 10,
                 (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) >> 10), s_heap_min >> 10, s_clients);
    }
}
void phone_tick(void) { if (s_running) heap_sample(NULL); }
bool phone_running(void) { return s_running; }

static void on_wifi(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (id == WIFI_EVENT_AP_STACONNECTED) s_clients++;
    else if (id == WIFI_EVENT_AP_STADISCONNECTED && s_clients > 0) s_clients--;
    ESP_LOGI(TAG, "phones connected: %d", s_clients);
}

/* ---------------------------------------------------------------- web handlers */

static esp_err_t page(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    /* EMBED_TXTFILES adds a NUL at the end: don't send it */
    return httpd_resp_send(req, phone_html_start, phone_html_end - phone_html_start - 1);
}

static esp_err_t app_icon(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/png");
    httpd_resp_set_hdr(req, "Cache-Control", "max-age=86400");
    return httpd_resp_send(req, (const char *)icon_png_start, icon_png_end - icon_png_start);
}

static esp_err_t list(httpd_req_t *req)
{
    storage_item_t *items;
    int n;
    if (storage_list(&items, &n) != ESP_OK) n = 0, items = NULL;
    size_t cap = 256 + (size_t)n * 48;
    char *buf = heap_caps_malloc(cap, MALLOC_CAP_SPIRAM);
    if (!buf) { free(items); return httpd_resp_send_500(req); }
    char batt[8] = "null";
    if (battery_present()) snprintf(batt, sizeof(batt), "%d", battery_percent());
    size_t pos = snprintf(buf, cap, "{\"camera\":\"%s\",\"owner\":\"%s\",\"battery\":%s,\"items\":[",
                          s_ssid, s_owner, batt);
    for (int i = 0; i < n; i++) {
        pos += snprintf(buf + pos, cap - pos, "%s{\"n\":\"%s\",\"s\":%lu,\"v\":%d}",
                        i ? "," : "", items[i].name, (unsigned long)items[i].size, items[i].is_video);
    }
    snprintf(buf + pos, cap - pos, "]}");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    esp_err_t err = httpd_resp_sendstr(req, buf);
    free(buf);
    free(items);
    return err;
}

/* Send an open file in CHUNK pieces, and close it */
static esp_err_t send_open_file(httpd_req_t *req, FILE *f)
{
    esp_err_t err = ESP_OK;
    size_t n;
    while (err == ESP_OK && (n = fread(s_chunk, 1, CHUNK, f)) > 0) err = httpd_resp_send_chunk(req, (const char *)s_chunk, n);
    fclose(f);
    heap_sample(NULL);
    if (err == ESP_OK) err = httpd_resp_send_chunk(req, NULL, 0);
    return err;
}

/* The NAME in /thumb/NAME, /file/NAME or /small/NAME, without any ?... */
static bool uri_name(httpd_req_t *req, const char *prefix, char *name, size_t cap)
{
    const char *path = req->uri + strlen(prefix);
    size_t len = strcspn(path, "?");                /* the page may add ?retry=1 */
    if (len >= cap) return false;
    memcpy(name, path, len);
    name[len] = 0;
    return true;
}

static esp_err_t send_file(httpd_req_t *req, bool thumb)
{
    char name[STORAGE_NAME_LEN];
    if (!uri_name(req, thumb ? "/thumb/" : "/file/", name, sizeof(name))) return httpd_resp_send_404(req);
    uint32_t size = 0;
    FILE *f = storage_open(name, thumb, &size);   /* rejects any name that isn't ours */
    if (!f) return httpd_resp_send_404(req);
    bool video = strstr(name, ".AVI") != NULL;
    httpd_resp_set_type(req, thumb || !video ? "image/jpeg" : "video/x-msvideo");
    /* names are never reused (numbers keep counting), so the phone may keep copies */
    httpd_resp_set_hdr(req, "Cache-Control", "max-age=31536000, immutable");
    int64_t t0 = esp_timer_get_time();
    esp_err_t err = send_open_file(req, f);
    if (err == ESP_OK && !thumb) {
        /* time from first read to last byte handed to Wi-Fi: card and Wi-Fi together */
        float s = (esp_timer_get_time() - t0) / 1e6f;
        ESP_LOGI(TAG, "sent %s: %lu KB in %.2f s (%.0f KB/s)", name, (unsigned long)(size >> 10), s,
                 s > 0 ? size / 1024.0f / s : 0);
        if (s_activity) {
            char msg[32];
            snprintf(msg, sizeof(msg), "sent %.8s", name);
            s_activity(msg, false);
        }
    }
    return err;
}

static esp_err_t thumb(httpd_req_t *req) { return send_file(req, true); }
static esp_err_t file(httpd_req_t *req) { return send_file(req, false); }

static int query_int(const char *q, const char *key)
{
    char v[8];
    return httpd_query_key_value(q, key, v, sizeof(v)) == ESP_OK ? atoi(v) : 0;
}

/* GET /small/NAME?w=W&h=H: the real photo, scaled to cover W x H with the middle cropped,
 * for the frame (10-frame-mailbox.md). Made the first time, about a second, and kept on
 * the card for next time. Photos only. */
static esp_err_t small(httpd_req_t *req)
{
    char name[STORAGE_NAME_LEN], q[32] = "";
    uint32_t size = 0;
    if (!uri_name(req, "/small/", name, sizeof(name)) || strncmp(name, "DSC_", 4) != 0) return httpd_resp_send_404(req);
    httpd_req_get_url_query_str(req, q, sizeof(q));
    int w = query_int(q, "w"), h = query_int(q, "h");
    FILE *f = storage_open(name, false, &size);   /* checks the name: 404 for anything not ours */
    if (!f) return httpd_resp_send_404(req);
    if (w < 16 || w > 640 || h < 16 || h > 640) {
        fclose(f);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "w and h must each be 16 to 640");
    }
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Cache-Control", "max-age=31536000, immutable");

    uint32_t kept_size;
    FILE *kept = storage_small_open(name, w, h, &kept_size);
    if (kept) {
        fclose(f);
        return send_open_file(req, kept);
    }

    /* Not made yet: read the photo into PSRAM, through the internal buffer (the SD driver
     * can't DMA into PSRAM) */
    uint8_t *photo = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    size_t got = 0, n;
    while (photo && got < size && (n = fread(s_chunk, 1, size - got < CHUNK ? size - got : CHUNK, f)) > 0) {
        memcpy(photo + got, s_chunk, n);
        got += n;
    }
    fclose(f);
    uint8_t *jpg = NULL;
    size_t jpg_len = 0;
    esp_err_t err = !photo ? ESP_ERR_NO_MEM : got != size ? ESP_FAIL : small_make(photo, size, w, h, &jpg, &jpg_len);
    free(photo);
    heap_sample("GET /small");
    if (err != ESP_OK) return httpd_resp_send_500(req);
    if (storage_small_save(name, w, h, jpg, jpg_len) != ESP_OK)
        ESP_LOGW(TAG, "%s: small copy not kept", name);   /* still send it */
    err = httpd_resp_send(req, (const char *)jpg, jpg_len);
    free(jpg);
    return err;
}

static esp_err_t del(httpd_req_t *req)
{
    char name[STORAGE_NAME_LEN] = "";
    int len = req->content_len < sizeof(name) - 1 ? req->content_len : sizeof(name) - 1;
    if (len <= 0 || httpd_req_recv(req, name, len) != len) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
    name[len] = 0;
    bool ok = storage_delete(name) == ESP_OK;   /* storage_delete checks the name too */
    if (ok && s_activity) {
        char msg[32];
        snprintf(msg, sizeof(msg), "del %.8s", name);
        s_activity(msg, true);
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

/* ---------------------------------------------------------------- frame mailbox */

static bool from_frame(httpd_req_t *req)
{
    char ua[16] = "";
    httpd_req_get_hdr_value_str(req, "User-Agent", ua, sizeof(ua));   /* truncated is fine */
    return strncmp(ua, "DigiGallery/", 12) == 0;
}

static esp_err_t frame_get(httpd_req_t *req)
{
    if (from_frame(req) && s_activity) s_activity("frame here", false);
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    uint32_t size = 0;
    FILE *f = storage_frame_open(&size);
    if (!f) {
        httpd_resp_set_status(req, "204 No Content");
        return httpd_resp_send(req, NULL, 0);
    }
    size_t n = fread(s_chunk, 1, CHUNK, f);
    fclose(f);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, (const char *)s_chunk, n);
}

static esp_err_t frame_post(httpd_req_t *req)
{
    if (req->content_len > FRAME_MAX) {
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "The frame document is limited to 16 KB");
        return ESP_FAIL;    /* closes the connection rather than reading the rest */
    }
    size_t got = 0;
    while (got < req->content_len) {
        int r = httpd_req_recv(req, (char *)s_chunk + got, req->content_len - got);
        if (r == HTTPD_SOCK_ERR_TIMEOUT) continue;
        if (r <= 0) return ESP_FAIL;
        got += r;
    }
    cJSON *doc = cJSON_ParseWithLength((const char *)s_chunk, got);
    bool ok = cJSON_IsObject(doc);
    cJSON_Delete(doc);
    if (!ok) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Not a JSON object");
    if (storage_frame_save(s_chunk, got) != ESP_OK) return httpd_resp_send_500(req);
    ESP_LOGI(TAG, "frame mailbox saved: %u bytes", (unsigned)got);
    heap_sample("POST /api/frame");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* cJSON's nodes for a 300-photo mailbox take about 50 KB: keep them out of internal RAM */
static void *json_malloc(size_t n) { return heap_caps_malloc(n, MALLOC_CAP_SPIRAM); }

static esp_err_t start_http(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;
    cfg.stack_size = 8192;
    cfg.max_open_sockets = 7;      /* Safari opens several connections for the thumbnails */
    cfg.lru_purge_enable = true;
    cfg.max_uri_handlers = 12;     /* the default 8 leaves too little room */
    /* A phone busy with other requests can stall a download for a few seconds; giving up
     * after the default 5 s left it with half a photo */
    cfg.send_wait_timeout = 15;
    esp_err_t err = httpd_start(&s_http, &cfg);
    if (err != ESP_OK) return err;
    const httpd_uri_t uris[] = {
        { .uri = "/",           .method = HTTP_GET,  .handler = page },
        { .uri = "/api/list",   .method = HTTP_GET,  .handler = list },
        { .uri = "/icon-180.png", .method = HTTP_GET, .handler = app_icon },
        { .uri = "/apple-touch-icon.png", .method = HTTP_GET, .handler = app_icon },   /* iOS asks for these too */
        { .uri = "/apple-touch-icon-precomposed.png", .method = HTTP_GET, .handler = app_icon },
        { .uri = "/thumb/*",    .method = HTTP_GET,  .handler = thumb },
        { .uri = "/file/*",     .method = HTTP_GET,  .handler = file },
        { .uri = "/small/*",    .method = HTTP_GET,  .handler = small },
        { .uri = "/api/delete", .method = HTTP_POST, .handler = del },
        { .uri = "/api/frame",  .method = HTTP_GET,  .handler = frame_get },
        { .uri = "/api/frame",  .method = HTTP_POST, .handler = frame_post },
    };
    for (int i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) httpd_register_uri_handler(s_http, &uris[i]);
    return ESP_OK;
}

/* ---------------------------------------------------------------- Wi-Fi */

/* The quietest of channels 1, 6 and 11 (the three that don't overlap). Every network heard
 * counts against the channels it overlaps, more for a strong or close one. Phones and the
 * frame find the camera by its name on any channel. Takes about a second; 6 if it fails. */
static uint8_t pick_channel(void)
{
    static const uint8_t cand[] = { 6, 1, 11 };   /* 6 first: it wins ties */
    int load[3] = { 0 };
    uint8_t best = 6;
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK || esp_wifi_start() != ESP_OK) return best;
    wifi_scan_config_t sc = { .show_hidden = true, .scan_time.active = { .min = 40, .max = 80 } };
    if (esp_wifi_scan_start(&sc, true) == ESP_OK) {
        uint16_t n = 40;
        wifi_ap_record_t *aps = heap_caps_malloc(n * sizeof(*aps), MALLOC_CAP_SPIRAM);
        if (aps && esp_wifi_scan_get_ap_records(&n, aps) == ESP_OK) {
            for (int i = 0; i < n; i++) {
                for (int k = 0; k < 3; k++) {
                    int d = abs((int)aps[i].primary - cand[k]);
                    if (d < 5) load[k] += (aps[i].rssi + 100 > 0 ? aps[i].rssi + 100 : 1) * (5 - d);
                }
            }
            int b = 0;
            for (int k = 1; k < 3; k++) if (load[k] < load[b]) b = k;
            best = cand[b];
            ESP_LOGI(TAG, "%u networks nearby; load on 1/6/11: %d/%d/%d; using channel %u",
                     n, load[1], load[0], load[2], best);
        }
        free(aps);
        esp_wifi_clear_ap_list();
    }
    esp_wifi_stop();
    return best;
}

esp_err_t phone_start(const char *ssid, const char *pass, const char *owner)
{
    if (s_running) return ESP_OK;
    s_chunk = storage_media_buf();
    if (!s_chunk) return ESP_ERR_NO_MEM;
    snprintf(s_ssid, sizeof(s_ssid), "%s", ssid);
    snprintf(s_owner, sizeof(s_owner), "%s", owner);
    if (!s_inited) {
        ESP_ERROR_CHECK(esp_netif_init());
        esp_err_t e = esp_event_loop_create_default();
        if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) return e;
        esp_netif_create_default_wifi_ap();
        wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&init));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi, NULL));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        /* For every cJSON user, including the USB link's commands: free() handles both heaps */
        cJSON_Hooks hooks = { .malloc_fn = json_malloc, .free_fn = free };
        cJSON_InitHooks(&hooks);
        s_inited = true;
    }
    wifi_config_t ap = { 0 };
    snprintf((char *)ap.ap.ssid, sizeof(ap.ap.ssid), "%s", ssid);
    snprintf((char *)ap.ap.password, sizeof(ap.ap.password), "%s", pass);
    ap.ap.ssid_len = strlen(ssid);
    ap.ap.channel = pick_channel();
    ap.ap.max_connection = 2;
    ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap.ap.pmf_cfg.required = false;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    esp_err_t err = esp_wifi_start();
    if (err != ESP_OK) return err;
    s_clients = 0;
    err = start_http();
    if (err != ESP_OK) { esp_wifi_stop(); return err; }
    s_running = true;
    s_heap_min = -1;
    ESP_LOGI(TAG, "Wi-Fi \"%s\" up, page at %s", ssid, PHONE_URL);
    heap_sample("start");
    return ESP_OK;
}

void phone_stop(void)
{
    if (!s_running) return;
    httpd_stop(s_http);
    s_http = NULL;
    esp_wifi_stop();
    s_clients = 0;
    s_running = false;
    s_chunk = NULL;
    ESP_LOGI(TAG, "Wi-Fi off");
}
