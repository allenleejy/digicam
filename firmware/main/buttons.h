#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/*
 * buttons.h: the shutter and the three back buttons, debounced.
 * ◀ and ▶ repeat while held (for scrolling the gallery). ● reports a press on
 * release, or a hold after 0.6 s, matching the simulator.
 */

typedef enum {
    BTN_LEFT, BTN_RIGHT, BTN_OK, BTN_OK_HOLD, BTN_SHUTTER,
} btn_event_t;

#define BTN_HOLD_MS 600

/* Starts a polling task that sends btn_event_t values to `q`. */
esp_err_t buttons_init(QueueHandle_t q);

/* True while any button is down (used to keep the screen awake). */
bool buttons_any_down(void);
