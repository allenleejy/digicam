#pragma once

#include <stdbool.h>
#include "esp_err.h"

/*
 * battery.h: cell voltage through the 100k/100k divider on GPIO1.
 */

esp_err_t battery_init(void);

/* Read and smooth. Call about once a second. */
void battery_update(void);

/* False when the divider reads near zero: no cell, or running from USB on the bench. */
bool battery_present(void);
int battery_mv(void);
int battery_percent(void);
