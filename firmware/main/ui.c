/*
 * ui.c: the display task. Each pass: take a snapshot of the state (and any new
 * picture), redraw the whole frame with ui_draw(), send it to the screen.
 */

#include "ui.h"
#include "display.h"
#include "lens.h"

#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define SLOT_SIZE (256 * 1024)

static SemaphoreHandle_t s_lock, s_slot_mx;
static TaskHandle_t s_task;
static ui_state_t s_state;

static uint8_t *s_slot;
static size_t s_slot_len;
static bool s_slot_new;

static char s_toast[32];
static int64_t s_toast_until, s_flash_until;
static int s_backlight = -1;

ui_state_t *ui_lock(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    return &s_state;
}

void ui_unlock(void)
{
    xSemaphoreGive(s_lock);
    if (s_task) xTaskNotifyGive(s_task);
}

void ui_toast(const char *text, int ms)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    snprintf(s_toast, sizeof(s_toast), "%s", text);
    s_toast_until = esp_timer_get_time() + ms * 1000LL;
    ui_unlock();
}

void ui_flash(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_flash_until = esp_timer_get_time() + 90 * 1000;
    ui_unlock();
}

void ui_backlight(int percent)
{
    if (percent == s_backlight) return;
    s_backlight = percent;
    display_backlight(percent);
}

void ui_frame(const uint8_t *jpg, size_t len, bool wait)
{
    if (len > SLOT_SIZE) return;
    if (xSemaphoreTake(s_slot_mx, wait ? pdMS_TO_TICKS(500) : 0) != pdTRUE) return;
    memcpy(s_slot, jpg, len);
    s_slot_len = len;
    xSemaphoreGive(s_slot_mx);
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_slot_new = true;
    ui_unlock();
}

void ui_snapshot(uint8_t *out)
{
    memcpy(out, gfx_framebuffer(), LCD_W * LCD_H * 2);
}

static void ui_task(void *arg)
{
    ui_state_t s;
    bool have_image = false;
    ui_view_t last_view = UI_OFF;
    int last_pos = -1;
    char toast[sizeof(s_toast)];
    for (;;) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(250));   /* also redraws for timers and toasts */
        int64_t now = esp_timer_get_time();
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s = s_state;
        bool new_picture = s_slot_new;
        s_slot_new = false;
        bool toast_on = now < s_toast_until;
        if (toast_on) memcpy(toast, s_toast, sizeof(toast));
        bool flash = now < s_flash_until;
        xSemaphoreGive(s_lock);

        /* a different screen, or another photo in the gallery: drop the old picture */
        bool same_picture = (s.view == last_view) || (s.view == UI_MODE && last_view == UI_PREVIEW) ||
                            (s.view == UI_PREVIEW && last_view == UI_MODE);
        if (!same_picture || (s.view == UI_GALLERY && s.g_pos != last_pos)) have_image = false;
        last_view = s.view;
        last_pos = s.g_pos;
        if (s.view == UI_OFF && s_backlight == 0) continue;

        if (new_picture) {
            xSemaphoreTake(s_slot_mx, portMAX_DELAY);
            have_image = display_decode_jpeg(s_slot, s_slot_len, UI_IMG_X, 0, UI_IMG_W, LCD_H);
            xSemaphoreGive(s_slot_mx);
        }
        ui_overlay_t o = { .toast = toast_on ? toast : NULL, .flash = flash, .have_image = have_image,
                           .now_ms = now / 1000 };
        ui_draw(&s, &o);
        display_present();
    }
}

void ui_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    s_slot_mx = xSemaphoreCreateMutex();
    s_slot = heap_caps_malloc(SLOT_SIZE, MALLOC_CAP_SPIRAM);
    s_state.view = UI_BOOT;
    s_state.looks = lens_look_count();
    xTaskCreatePinnedToCore(ui_task, "ui", 6144, NULL, 4, &s_task, 0);
}
