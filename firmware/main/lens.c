/*
 * lens.c: OV3660 camera on the GOOUUU board.
 *
 * The driver sizes its JPEG frame buffers from the init frame size
 * (width * height / 5), so we init at QXGA and switch down for preview.
 * Config otherwise matches lovebox camera-test, which is proven on this board.
 */

#include "lens.h"
#include "pins.h"

#include "esp_log.h"

static const char *TAG = "lens";

typedef struct {
    const char *name;
    int effect;      /* 0 none, 2 grayscale, 6 sepia */
    int wb_mode;     /* 0 auto, 1 sunny, 2 cloudy, 3 office, 4 home */
    int saturation;  /* -2..2 */
    int contrast;    /* -2..2 */
    int brightness;  /* -2..2 */
} look_t;

/* The starting set from 02-build-plan.md, Firmware spec. */
static const look_t LOOKS[] = {
    { "Natural",   0, 0,  0,  0, 0 },
    { "Warm film", 0, 2,  1,  0, 0 },
    { "Faded",     0, 0, -1, -2, 1 },
    { "Mono",      2, 0,  0,  2, 0 },
    { "Sepia",     6, 0,  0,  0, 0 },
    { "Cool",      0, 3, -1,  0, 0 },
};
#define LOOK_COUNT (int)(sizeof(LOOKS) / sizeof(LOOKS[0]))

esp_err_t lens_init(void)
{
    camera_config_t config = {
        .pin_pwdn     = PIN_CAM_PWDN,
        .pin_reset    = PIN_CAM_RESET,
        .pin_xclk     = PIN_CAM_XCLK,
        .pin_sccb_sda = PIN_CAM_SCCB_SDA,
        .pin_sccb_scl = PIN_CAM_SCCB_SCL,
        .pin_d0       = PIN_CAM_D0,
        .pin_d1       = PIN_CAM_D1,
        .pin_d2       = PIN_CAM_D2,
        .pin_d3       = PIN_CAM_D3,
        .pin_d4       = PIN_CAM_D4,
        .pin_d5       = PIN_CAM_D5,
        .pin_d6       = PIN_CAM_D6,
        .pin_d7       = PIN_CAM_D7,
        .pin_vsync    = PIN_CAM_VSYNC,
        .pin_href     = PIN_CAM_HREF,
        .pin_pclk     = PIN_CAM_PCLK,
        .xclk_freq_hz = 20000000,
        .ledc_timer   = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size   = LENS_STILL,
        .jpeg_quality = 10,
        .fb_count     = 2,
        .fb_location  = CAMERA_FB_IN_PSRAM,
        .grab_mode    = CAMERA_GRAB_LATEST,
    };

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_camera_init failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "OV sensor PID 0x%04X", lens_sensor_pid());

    sensor_t *s = esp_camera_sensor_get();
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_exposure_ctrl(s, 1);
    s->set_gain_ctrl(s, 1);
    lens_apply_look(0);
    return lens_set_size(LENS_PREVIEW_PHOTO, 14);
}

esp_err_t lens_set_size(framesize_t size, int quality)
{
    sensor_t *s = esp_camera_sensor_get();
    if (!s) return ESP_FAIL;
    if (s->set_quality(s, quality) != 0) return ESP_FAIL;
    if (s->status.framesize != size && s->set_framesize(s, size) != 0) {
        ESP_LOGE(TAG, "set_framesize(%d) failed", size);
        return ESP_FAIL;
    }
    return ESP_OK;
}

camera_fb_t *lens_grab(void)
{
    return esp_camera_fb_get();
}

void lens_release(camera_fb_t *fb)
{
    if (fb) esp_camera_fb_return(fb);
}

void lens_flush(int frames)
{
    for (int i = 0; i < frames; i++) {
        lens_release(lens_grab());
    }
}

int lens_look_count(void)
{
    return LOOK_COUNT;
}

const char *lens_look_name(int index)
{
    return (index >= 0 && index < LOOK_COUNT) ? LOOKS[index].name : "?";
}

esp_err_t lens_apply_look(int index)
{
    if (index < 0 || index >= LOOK_COUNT) return ESP_ERR_INVALID_ARG;
    sensor_t *s = esp_camera_sensor_get();
    if (!s) return ESP_FAIL;
    const look_t *l = &LOOKS[index];
    s->set_special_effect(s, l->effect);
    s->set_wb_mode(s, l->wb_mode);
    s->set_saturation(s, l->saturation);
    s->set_contrast(s, l->contrast);
    s->set_brightness(s, l->brightness);
    return ESP_OK;
}

int lens_sensor_pid(void)
{
    sensor_t *s = esp_camera_sensor_get();
    return s ? (int)s->id.PID : 0;
}
