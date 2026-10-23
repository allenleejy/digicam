/*
 * phone.c: the camera's own Wi-Fi network (WPA2) and a small web server for the phone.
 *
 *   GET  /                 the photo page (web/phone.html, built into the firmware)
 *   GET  /api/list         {"camera","owner","battery","items":[{"n","s","v"}]} newest first
 *   GET  /thumb/NAME       320x240 preview JPEG
 *   GET  /file/NAME        the photo or video
 *   POST /api/delete       body: NAME
 *   GET  /api/frame        the frame mailbox, as stored (204 when there's none)
 *   POST /api/frame        body: the whole mailbox document, at most 16 KB
 *
 * All camera and SD work otherwise happens on cam_task; in Phone mode the camera is
 * idle, so the web server's task is the only one reading the card.
 */

#include "phone.h"
#include "battery.h"
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

/* Reads go into internal RAM: the SD driver can't DMA into PSRAM and falls back to one
 * 512-byte sector per transfer. The web server runs handlers one at a time on its own
 * task, so one buffer is enough. */
#define CHUNK (16 * 1024)
#define FRAME_MAX CHUNK    /* the mailbox fits the read buffer, so it's received and sent in one piece */

static uint8_t *s_chunk;
static bool s_inited, s_running;
static httpd_handle_t s_http;
static volatile int s_clients;
static void (*s_activity)(const char *, bool);
static char s_ssid[33], s_owner[24];

void phone_on_activity(void (*cb)(const char *, bool)) { s_activity = cb; }
int phone_clients(void) { return s_clients; }
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

static esp_err_t send_file(httpd_req_t *req, bool thumb)
{
    const char *name = req->uri + (thumb ? strlen("/thumb/") : strlen("/file/"));
    uint32_t size = 0;
    FILE *f = storage_open(name, thumb, &size);   /* rejects any name that isn't ours */
    if (!f) return httpd_resp_send_404(req);
    bool video = strstr(name, ".AVI") != NULL;
    httpd_resp_set_type(req, thumb || !video ? "image/jpeg" : "video/x-msvideo");
    /* names are never reused (numbers keep counting), so the phone may keep copies */
    httpd_resp_set_hdr(req, "Cache-Control", "max-age=31536000, immutable");
    int64_t t0 = esp_timer_get_time();
    esp_err_t err = ESP_OK;
    size_t n;
    while (err == ESP_OK && (n = fread(s_chunk, 1, CHUNK, f)) > 0) err = httpd_resp_send_chunk(req, (const char *)s_chunk, n);
    fclose(f);
    if (err == ESP_OK) err = httpd_resp_send_chunk(req, NULL, 0);
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
    ESP_LOGI(TAG, "frame mailbox saved: %u bytes, internal free %u KB", (unsigned)got,
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) >> 10));
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
    esp_err_t err = httpd_start(&s_http, &cfg);
    if (err != ESP_OK) return err;
    const httpd_uri_t uris[] = {
        { .uri = "/",           .method = HTTP_GET,  .handler = page },
        { .uri = "/api/list",   .method = HTTP_GET,  .handler = list },
        { .uri = "/thumb/*",    .method = HTTP_GET,  .handler = thumb },
        { .uri = "/file/*",     .method = HTTP_GET,  .handler = file },
        { .uri = "/api/delete", .method = HTTP_POST, .handler = del },
        { .uri = "/api/frame",  .method = HTTP_GET,  .handler = frame_get },
        { .uri = "/api/frame",  .method = HTTP_POST, .handler = frame_post },
    };
    for (int i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) httpd_register_uri_handler(s_http, &uris[i]);
    return ESP_OK;
}

/* ---------------------------------------------------------------- Wi-Fi */

esp_err_t phone_start(const char *ssid, const char *pass, const char *owner)
{
    if (s_running) return ESP_OK;
    if (!s_chunk) s_chunk = heap_caps_malloc(CHUNK, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
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
    ap.ap.channel = 6;
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
    ESP_LOGI(TAG, "Wi-Fi \"%s\" up, page at %s", ssid, PHONE_URL);
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
    free(s_chunk);
    s_chunk = NULL;
    ESP_LOGI(TAG, "Wi-Fi off");
}
