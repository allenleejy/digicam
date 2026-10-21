/*
 * link.c: framed serial link on UART0.
 *
 * After boot the app switches UART0 to LINK_BAUD and routes ESP_LOG output
 * into 'L' frames, so log text never lands in the middle of a binary frame.
 */

#include "link.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_rom_crc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define UART      UART_NUM_0
#define RX_BUF    4096
#define LINK_LINE_MAX  1024

static SemaphoreHandle_t s_tx_lock;
static link_line_cb_t s_on_line;

void link_send(char type, const void *head, size_t head_len, const void *body, size_t body_len)
{
    uint32_t len = head_len + body_len;
    uint8_t hdr[7] = { 0xA5, 0x5A, (uint8_t)type, len, len >> 8, len >> 16, len >> 24 };
    uint32_t crc = 0;
    if (head_len) crc = esp_rom_crc32_le(crc, head, head_len);
    if (body_len) crc = esp_rom_crc32_le(crc, body, body_len);
    uint8_t tail[4] = { crc, crc >> 8, crc >> 16, crc >> 24 };

    xSemaphoreTake(s_tx_lock, portMAX_DELAY);
    uart_write_bytes(UART, hdr, sizeof(hdr));
    if (head_len) uart_write_bytes(UART, head, head_len);
    if (body_len) uart_write_bytes(UART, body, body_len);
    uart_write_bytes(UART, tail, sizeof(tail));
    xSemaphoreGive(s_tx_lock);
}

void link_send_json(char type, const char *json)
{
    link_send(type, NULL, 0, json, strlen(json));
}

void link_set_baud(uint32_t baud)
{
    xSemaphoreTake(s_tx_lock, portMAX_DELAY);
    uart_wait_tx_done(UART, pdMS_TO_TICKS(500));
    uart_set_baudrate(UART, baud);
    xSemaphoreGive(s_tx_lock);
}

static int log_to_link(const char *fmt, va_list args)
{
    char line[256];
    int n = vsnprintf(line, sizeof(line), fmt, args);
    if (n <= 0) return n;
    size_t len = strnlen(line, sizeof(line));
    while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) len--;
    /* Drop logs raised while this task is mid-frame; nesting would corrupt the frame. */
    if (len && s_tx_lock && !xPortInIsrContext() &&
        xSemaphoreGetMutexHolder(s_tx_lock) != xTaskGetCurrentTaskHandle()) {
        link_send('L', NULL, 0, line, len);
    }
    return n;
}

static void rx_task(void *arg)
{
    static char line[LINK_LINE_MAX];
    size_t pos = 0;
    uint8_t buf[256];
    for (;;) {
        int n = uart_read_bytes(UART, buf, sizeof(buf), pdMS_TO_TICKS(50));
        for (int i = 0; i < n; i++) {
            char c = buf[i];
            if (c == '\n' || c == '\r') {
                if (pos) {
                    line[pos] = 0;
                    if (line[0] == '{' && s_on_line) s_on_line(line);
                    pos = 0;
                }
            } else if (pos < LINK_LINE_MAX - 1) {
                line[pos++] = c;
            } else {
                pos = 0;   /* overlong line: drop it */
            }
        }
    }
}

esp_err_t link_init(link_line_cb_t on_line)
{
    s_on_line = on_line;
    s_tx_lock = xSemaphoreCreateMutex();

    esp_err_t err = uart_driver_install(UART, RX_BUF, 0, 0, NULL, 0);
    if (err != ESP_OK) return err;
    uart_wait_tx_idle_polling(UART);   /* let boot output finish at 115200 */
    uart_set_baudrate(UART, LINK_BAUD);

    esp_log_set_vprintf(log_to_link);
    xTaskCreatePinnedToCore(rx_task, "link_rx", 4096, NULL, 6, NULL, 0);
    return ESP_OK;
}
