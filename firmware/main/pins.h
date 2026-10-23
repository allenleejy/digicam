#pragma once

/*
 * pins.h: GPIO map for the GOOUUU ESP32-S3-CAM Core N16R8.
 * Camera and microSD pins are fixed by the board. Taken from
 * Love Letter Box firmware/camera-test/main/pins.h (now on the
 * june-2026 branch of github.com/lovingdailyreminders/development).
 * Screen, buttons and battery sense follow ../02-build-plan.md (Pin plan).
 */

/* microSD: built-in slot, SDMMC 1-bit */
#define PIN_SD_CMD    38
#define PIN_SD_CLK    39
#define PIN_SD_D0     40

/* OV3660: DVP parallel interface */
#define PIN_CAM_XCLK      15
#define PIN_CAM_SCCB_SDA   4
#define PIN_CAM_SCCB_SCL   5
#define PIN_CAM_VSYNC      6
#define PIN_CAM_HREF       7
#define PIN_CAM_PCLK      13
#define PIN_CAM_D0        11
#define PIN_CAM_D1         9
#define PIN_CAM_D2         8
#define PIN_CAM_D3        10
#define PIN_CAM_D4        12
#define PIN_CAM_D5        18
#define PIN_CAM_D6        17
#define PIN_CAM_D7        16
#define PIN_CAM_PWDN      -1
#define PIN_CAM_RESET     -1

/* Screen: ST7789 over SPI2 (through the GPIO matrix). RES goes to the board's EN pin. */
#define PIN_LCD_SCK   21
#define PIN_LCD_MOSI  47
#define PIN_LCD_CS    42
#define PIN_LCD_DC    41
#define PIN_LCD_BL    46    /* backlight, PWM */

/* Buttons: one leg to the pin, the other to GND (internal pull-ups) */
#define PIN_BTN_LEFT     2
#define PIN_BTN_OK      14
#define PIN_BTN_RIGHT   45  /* strapping pin: low at boot is the safe level, so a button to GND is fine */
#define PIN_BTN_SHUTTER 48  /* shared with the onboard RGB LED's data line */

/* Battery: cell + -> 100k -> GPIO1 -> 100k -> GND (half the cell voltage) */
#define PIN_BATT_SENSE   1
