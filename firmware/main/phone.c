/*
 * phone.c: the camera's own Wi-Fi network (WPA2) and a small web server for the phone.
 *
 *   GET  /                 the photo page (web/phone.html, built into the firmware)
 *   GET  /api/list         {"camera","owner","battery","items":[{"n","s","v"}]} newest first
 *   GET  /thumb/NAME       320x240 preview JPEG
 *   GET  /file/NAME        the photo or video
 *   POST /api/delete       body: NAME
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
#include "esp_wifi.h"

static const char *TAG = "phone";

extern const char phone_html_start[] asm("_binary_phone_html_start");
extern const char phone_html_end[] asm("_binary_phone_html_end");

#define CHUNK (16 * 1024)

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
    char *chunk = heap_caps_malloc(CHUNK, MALLOC_CAP_SPIRAM);
    esp_err_t err = chunk ? ESP_OK : ESP_ERR_NO_MEM;
    size_t n;
    while (err == ESP_OK && (n = fread(chunk, 1, CHUNK, f)) > 0) err = httpd_resp_send_chunk(req, chunk, n);
    fclose(f);
    free(chunk);
    if (err == ESP_OK) err = httpd_resp_send_chunk(req, NULL, 0);
    if (err == ESP_OK && !thumb && s_activity) {
        char msg[32];
        snprintf(msg, sizeof(msg), "sent %.8s", name);
        s_activity(msg, false);
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

static esp_err_t start_http(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;
    cfg.stack_size = 8192;
    cfg.max_open_sockets = 7;      /* Safari opens several connections for the thumbnails */
    cfg.lru_purge_enable = true;
    esp_err_t err = httpd_start(&s_http, &cfg);
    if (err != ESP_OK) return err;
    const httpd_uri_t uris[] = {
        { .uri = "/",           .method = HTTP_GET,  .handler = page },
        { .uri = "/api/list",   .method = HTTP_GET,  .handler = list },
        { .uri = "/thumb/*",    .method = HTTP_GET,  .handler = thumb },
        { .uri = "/file/*",     .method = HTTP_GET,  .handler = file },
        { .uri = "/api/delete", .method = HTTP_POST, .handler = del },
    };
    for (int i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) httpd_register_uri_handler(s_http, &uris[i]);
    return ESP_OK;
}

/* ---------------------------------------------------------------- Wi-Fi */

esp_err_t phone_start(const char *ssid, const char *pass, const char *owner)
{
    if (s_running) return ESP_OK;
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
    ESP_LOGI(TAG, "Wi-Fi off");
}
