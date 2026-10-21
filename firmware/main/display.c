/*
 * display.c: ST7789 screen over SPI2, its framebuffer, and JPEG decoding for the screen.
 */

#include "display.h"
#include "pins.h"

#include <string.h>

#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "jpeg_decoder.h"

static const char *TAG = "display";

/* 1.14 in modules: the 135x240 glass sits in the middle of the controller's 240x320 RAM.
 * Landscape, header on the left as seen from the front: MV + MX, gap 40/53.
 * Turned the other way up: MV + MY, gap 40/52. */
#define DEF_SWAP   true
#define DEF_MX     true
#define DEF_MY     false
#define DEF_GAP_X  40
#define DEF_GAP_Y  53

#define BL_TIMER   LEDC_TIMER_1      /* timer 0 is the camera's clock */
#define BL_CHANNEL LEDC_CHANNEL_1

#define DEC_MAX_PX (320 * 240)       /* biggest decoded picture we keep */
#define JPEG_WORK  3100              /* decoder scratch, from esp_jpeg */

static esp_lcd_panel_handle_t s_panel;
static uint16_t *s_fb;               /* LCD_W x LCD_H, internal DMA-capable RAM */
static SemaphoreHandle_t s_sent;

static uint16_t *s_dec;              /* last decoded picture, PSRAM */
static uint8_t *s_work;

static bool on_sent(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *e, void *ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_sent, &woken);
    return woken == pdTRUE;
}

esp_err_t display_init(void)
{
    s_fb = heap_caps_malloc(LCD_W * LCD_H * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_dec = heap_caps_malloc(DEC_MAX_PX * 2, MALLOC_CAP_SPIRAM);
    s_work = heap_caps_malloc(JPEG_WORK, MALLOC_CAP_INTERNAL);
    s_sent = xSemaphoreCreateBinary();
    if (!s_fb || !s_dec || !s_work || !s_sent) return ESP_ERR_NO_MEM;
    memset(s_fb, 0, LCD_W * LCD_H * 2);
    gfx_set_framebuffer(s_fb);

    /* Backlight off until the first frame is drawn, so power-on never flashes garbage */
    ledc_timer_config_t t = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .timer_num = BL_TIMER,
        .duty_resolution = LEDC_TIMER_10_BIT, .freq_hz = 5000, .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&t));
    ledc_channel_config_t ch = {
        .gpio_num = PIN_LCD_BL, .speed_mode = LEDC_LOW_SPEED_MODE, .channel = BL_CHANNEL,
        .timer_sel = BL_TIMER, .duty = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));

    spi_bus_config_t bus = {
        .sclk_io_num = PIN_LCD_SCK, .mosi_io_num = PIN_LCD_MOSI, .miso_io_num = -1,
        .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = LCD_W * LCD_H * 2,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO), TAG, "spi bus");

    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = PIN_LCD_CS, .dc_gpio_num = PIN_LCD_DC,
        .spi_mode = 0, .pclk_hz = LCD_SPI_HZ, .trans_queue_depth = 10,
        .lcd_cmd_bits = 8, .lcd_param_bits = 8,
        .on_color_trans_done = on_sent,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &io), TAG, "panel io");

    esp_lcd_panel_dev_config_t dev = {
        .reset_gpio_num = -1,              /* RES is on the board's EN pin; this sends a soft reset */
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(io, &dev, &s_panel), TAG, "panel");
    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);
    display_tune(DEF_SWAP, DEF_MX, DEF_MY, DEF_GAP_X, DEF_GAP_Y, true);
    esp_lcd_panel_disp_on_off(s_panel, true);
    display_present();
    ESP_LOGI(TAG, "ST7789 %dx%d ready", LCD_W, LCD_H);
    return ESP_OK;
}

void display_tune(bool swap_xy, bool mirror_x, bool mirror_y, int gap_x, int gap_y, bool invert)
{
    if (!s_panel) return;
    esp_lcd_panel_swap_xy(s_panel, swap_xy);
    esp_lcd_panel_mirror(s_panel, mirror_x, mirror_y);
    esp_lcd_panel_set_gap(s_panel, gap_x, gap_y);
    esp_lcd_panel_invert_color(s_panel, invert);
    ESP_LOGI(TAG, "tune: swap %d mirror %d/%d gap %d/%d invert %d", swap_xy, mirror_x, mirror_y, gap_x, gap_y, invert);
}

void display_backlight(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BL_CHANNEL, percent * 1023 / 100);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BL_CHANNEL);
}

void display_present(void)
{
    if (!s_panel) return;
    esp_lcd_panel_draw_bitmap(s_panel, 0, 0, LCD_W, LCD_H, s_fb);
    xSemaphoreTake(s_sent, pdMS_TO_TICKS(200));   /* the framebuffer is free again */
}

/* ---------------------------------------------------------------- pictures */

bool display_decode_jpeg(const uint8_t *jpg, size_t len, int x, int y, int w, int h)
{
    esp_jpeg_image_cfg_t cfg = {
        .indata = (uint8_t *)jpg, .indata_size = len,
        .outbuf = (uint8_t *)s_dec, .outbuf_size = DEC_MAX_PX * 2,
        .out_format = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale = JPEG_IMAGE_SCALE_0,
        .flags.swap_color_bytes = 1,
        .advanced.working_buffer = s_work, .advanced.working_buffer_size = JPEG_WORK,
    };
    esp_jpeg_image_output_t info;
    if (esp_jpeg_get_image_info(&cfg, &info) != ESP_OK) return false;

    int scale = 0;
    while (scale < 3 && (info.width >> (scale + 1)) >= w && (info.height >> (scale + 1)) >= h) scale++;
    while (scale < 3 && (info.width >> scale) * (info.height >> scale) > DEC_MAX_PX) scale++;
    cfg.out_scale = (esp_jpeg_image_scale_t)scale;
    if (esp_jpeg_decode(&cfg, &info) != ESP_OK) return false;
    gfx_set_image(s_dec, info.width, info.height, x, y, w, h);
    return true;
}
