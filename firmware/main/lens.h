#pragma once

#include "esp_err.h"
#include "esp_camera.h"

/* Preview sizes per mode. Stills are always the sensor's full 2048x1536. */
#define LENS_PREVIEW_PHOTO  FRAMESIZE_QVGA   /* 320x240 */
#define LENS_PREVIEW_VIDEO  FRAMESIZE_VGA    /* 640x480, also the recorded video size */
#define LENS_STILL          FRAMESIZE_QXGA   /* 2048x1536 */

/* Init at full resolution so the frame buffers fit a still, then drop to preview. */
esp_err_t lens_init(void);

/* Switch the running sensor to a frame size and JPEG quality (lower = better). */
esp_err_t lens_set_size(framesize_t size, int quality);

camera_fb_t *lens_grab(void);
void lens_release(camera_fb_t *fb);

/* Throw away frames still queued at the old size after a switch. */
void lens_flush(int frames);

/* Film looks: presets of the sensor's own colour settings, so they cost no CPU. */
int lens_look_count(void);
const char *lens_look_name(int index);
esp_err_t lens_apply_look(int index);

int lens_sensor_pid(void);
