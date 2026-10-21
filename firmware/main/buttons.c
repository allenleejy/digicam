/*
 * buttons.c: polls the four buttons every 10 ms. A level has to hold for 3 polls
 * (30 ms) to count, which rides out contact bounce.
 */

#include "buttons.h"
#include "pins.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/task.h"

#define POLL_MS    10
#define STABLE     3
#define REPEAT_MS  500   /* ◀ ▶ start repeating after this */
#define REPEAT_EVERY 150

static const char *TAG = "buttons";

typedef struct {
    int pin;
    btn_event_t press;
    bool repeats;
    bool down;
    int count;          /* polls the raw level has disagreed with `down` */
    int held_ms;
    int next_repeat;
    bool hold_sent;
} button_t;

static button_t s_btn[] = {
    { .pin = PIN_BTN_LEFT, .press = BTN_LEFT, .repeats = true },
    { .pin = PIN_BTN_RIGHT, .press = BTN_RIGHT, .repeats = true },
    { .pin = PIN_BTN_OK, .press = BTN_OK, .repeats = false },
    { .pin = PIN_BTN_SHUTTER, .press = BTN_SHUTTER, .repeats = false },
};
#define N_BTN (sizeof(s_btn) / sizeof(s_btn[0]))

static QueueHandle_t s_q;

static void send(btn_event_t e)
{
    xQueueSend(s_q, &e, 0);
}

static void poll_task(void *arg)
{
    for (;;) {
        for (int i = 0; i < N_BTN; i++) {
            button_t *b = &s_btn[i];
            bool raw = gpio_get_level(b->pin) == 0;
            if (raw != b->down) {
                if (++b->count < STABLE) continue;
                b->count = 0;
                b->down = raw;
                if (raw) {
                    b->held_ms = 0;
                    b->hold_sent = false;
                    b->next_repeat = REPEAT_MS;
                    if (b->press != BTN_OK) send(b->press);   /* ● waits to see if it's a hold */
                } else if (b->press == BTN_OK && !b->hold_sent) {
                    send(BTN_OK);
                }
                continue;
            }
            b->count = 0;
            if (!b->down) continue;
            b->held_ms += POLL_MS;
            if (b->press == BTN_OK && !b->hold_sent && b->held_ms >= BTN_HOLD_MS) {
                b->hold_sent = true;
                send(BTN_OK_HOLD);
            }
            if (b->repeats && b->held_ms >= b->next_repeat) {
                b->next_repeat += REPEAT_EVERY;
                send(b->press);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
    }
}

bool buttons_any_down(void)
{
    for (int i = 0; i < N_BTN; i++) if (s_btn[i].down) return true;
    return false;
}

esp_err_t buttons_init(QueueHandle_t q)
{
    s_q = q;
    uint64_t mask = 0;
    for (int i = 0; i < N_BTN; i++) mask |= 1ULL << s_btn[i].pin;
    gpio_config_t cfg = {
        .pin_bit_mask = mask, .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK) return err;
    /* A button already down at power-on (or a wiring short to GND) shouldn't fire. */
    for (int i = 0; i < N_BTN; i++) {
        s_btn[i].down = gpio_get_level(s_btn[i].pin) == 0;
        if (s_btn[i].down) {
            s_btn[i].hold_sent = true;
            ESP_LOGW(TAG, "GPIO%d reads pressed at start: check that button's wiring", s_btn[i].pin);
        }
    }
    xTaskCreatePinnedToCore(poll_task, "buttons", 3072, NULL, 6, NULL, 0);
    return ESP_OK;
}
