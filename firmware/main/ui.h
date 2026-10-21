#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * ui.h: what the screen shows. cam_task (main.c) owns the camera logic and fills in a
 * ui_state_t; a display task on the other core draws it, so decoding and sending
 * frames to the screen never slows the camera.
 *
 * Layout on the 240x135 screen: the 4:3 picture fills the middle 180x135, with a
 * 30 px column each side for status (mode and look on the left, battery and shots
 * left on the right). Menus cover the whole screen.
 *
 * Drawing (ui_draw.c) is plain C and also builds on the laptop: ../sim renders every
 * screen to a picture.
 */

typedef enum {
    UI_BOOT,        /* name, while the camera starts */
    UI_PREVIEW,     /* live view */
    UI_MODE,        /* mode dial over the live view: photo / video / settings */
    UI_GALLERY,
    UI_SETTINGS,    /* also the About page, with a different title */
    UI_PHONE,       /* Wi-Fi for the phone: QR codes to join and open the photo page */
    UI_MESSAGE,     /* camera not found, and so on */
    UI_OFF,
} ui_view_t;

#define UI_ROWS 8
#define UI_MODES 4          /* photo, video, phone, settings */
#define UI_QR_MAX 45

typedef struct {
    const char *const *icon;    /* 12x12 icon from gfx.h, or NULL */
    char label[16];
    char value[14];
} ui_row_t;

typedef struct {
    ui_view_t view;
    /* live view */
    bool video;
    bool rec;
    uint32_t rec_ms;
    int look, looks;
    bool sd;
    int left;                  /* photos left, or minutes of video left */
    bool busy;                 /* saving: picture frozen */
    bool grid;                 /* rule-of-thirds lines */
    int timer_s;               /* self-timer setting, shown in the left column */
    int countdown;             /* self-timer running: seconds to go, else 0 */
    /* battery */
    bool batt;
    int batt_pct;
    /* mode dial */
    int mode_sel;
    /* gallery */
    int g_pos, g_count;
    bool g_video, g_playing, g_confirm;
    char g_name[16];
    /* settings and About */
    char title[16];
    ui_row_t rows[UI_ROWS];
    int nrows, sel;
    bool dialog;               /* "Format card?" */
    /* phone mode */
    int phone_step;            /* 1: scan to join the Wi-Fi, 2: scan to open the photos */
    int clients;
    char ssid[33], pass[16];
    int qr_size;               /* modules per side; 0: none */
    uint8_t qr[UI_QR_MAX * UI_QR_MAX];   /* 1 = dark */
    /* boot and message screens */
    char line1[32], line2[32];
} ui_state_t;

/* ---------------------------------------------------------------- firmware side (ui.c) */

void ui_init(void);

/* Lock, edit, unlock. Unlock asks for a redraw. */
ui_state_t *ui_lock(void);
void ui_unlock(void);

void ui_toast(const char *text, int ms);
void ui_flash(void);                 /* white flash for the shutter */
void ui_backlight(int percent);

/* Hand the screen a new picture (live view, thumbnail or video frame). With wait=false
 * the frame is dropped if the screen is still drawing the last one (live view);
 * with wait=true it waits its turn (gallery). */
void ui_frame(const uint8_t *jpg, size_t len, bool wait);

/* Copy of the screen as RGB565 big-endian, LCD_W x LCD_H. For the laptop's "shot" command. */
void ui_snapshot(uint8_t *out);

/* ---------------------------------------------------------------- drawing (ui_draw.c) */

typedef struct {
    const char *toast;         /* NULL: none */
    bool flash;                /* shutter flash over the picture */
    bool have_image;           /* a picture is ready to draw */
    int64_t now_ms;            /* for blinking */
} ui_overlay_t;

/* The picture box: the 4:3 live view or photo, between the two status columns */
#define UI_IMG_X 30
#define UI_IMG_W 180

void ui_draw(const ui_state_t *s, const ui_overlay_t *o);
