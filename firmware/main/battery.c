/*
 * battery.c: ADC1 channel 0 (GPIO1) with the chip's calibration, averaged and smoothed.
 * A LiPo's voltage under load is only a rough guide to charge, so the percentage
 * comes from a typical discharge curve and is shown in steps.
 */

#include "battery.h"
#include "pins.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "battery";

#define DIVIDER    2        /* 100k / 100k */
#define SAMPLES    16
#define PRESENT_MV 2500     /* below this there's no cell on the divider */

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;
static float s_mv = -1;

/* Resting LiPo voltage (mV) at 100, 90 ... 0 % */
static const int CURVE[] = { 4150, 4050, 3970, 3900, 3840, 3790, 3750, 3710, 3670, 3600, 3400 };

esp_err_t battery_init(void)
{
    adc_oneshot_unit_init_cfg_t unit = { .unit_id = ADC_UNIT_1 };
    esp_err_t err = adc_oneshot_new_unit(&unit, &s_adc);
    if (err != ESP_OK) return err;
    adc_oneshot_chan_cfg_t ch = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT };
    adc_oneshot_config_channel(s_adc, ADC_CHANNEL_0, &ch);   /* GPIO1 */
    adc_cali_curve_fitting_config_t cal = {
        .unit_id = ADC_UNIT_1, .chan = ADC_CHANNEL_0, .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cal, &s_cali) != ESP_OK) {
        ESP_LOGW(TAG, "no ADC calibration in eFuse; readings will be rougher");
        s_cali = NULL;
    }
    battery_update();
    return ESP_OK;
}

void battery_update(void)
{
    int sum = 0, n = 0;
    for (int i = 0; i < SAMPLES; i++) {
        int raw, mv;
        if (adc_oneshot_read(s_adc, ADC_CHANNEL_0, &raw) != ESP_OK) continue;
        if (s_cali && adc_cali_raw_to_voltage(s_cali, raw, &mv) == ESP_OK) sum += mv;
        else sum += raw * 3100 / 4095;
        n++;
    }
    if (!n) return;
    float mv = (float)sum / n * DIVIDER;
    s_mv = (s_mv < 0 || mv < PRESENT_MV || s_mv < PRESENT_MV) ? mv : s_mv * 0.8f + mv * 0.2f;
}

bool battery_present(void)
{
    return s_mv >= PRESENT_MV;
}

int battery_mv(void)
{
    return (int)s_mv;
}

int battery_percent(void)
{
    int mv = (int)s_mv;
    if (mv >= CURVE[0]) return 100;
    for (int i = 1; i < 11; i++) {
        if (mv >= CURVE[i]) {
            int pct = 100 - 10 * i + 10 * (mv - CURVE[i]) / (CURVE[i - 1] - CURVE[i]);
            return pct;
        }
    }
    return 0;
}
