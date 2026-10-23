#pragma once

#include <stdbool.h>
#include "esp_err.h"

/*
 * phone.h: Phone mode. The camera starts its own Wi-Fi network and serves a photo
 * page (web/phone.html) at http://192.168.4.1. An iPhone joins by scanning a QR code
 * on the camera's screen, browses in Safari and saves photos to Photos: press and hold
 * a photo, or, for many at once, a free Shortcut the page sets up.
 *
 * Wi-Fi, not Bluetooth: iPhones only accept files from other devices through an app,
 * and an app on someone else's iPhone needs the paid Apple Developer Program.
 */

#define PHONE_URL "http://192.168.4.1/"

/* Start the Wi-Fi network and the web server. */
esp_err_t phone_start(const char *ssid, const char *pass, const char *owner);
void phone_stop(void);
bool phone_running(void);

/* Lowest internal free heap seen in Phone mode since it last started, in KB; -1 before then.
 * Sampled by phone_tick() (call it about twice a second) and after every request, and
 * logged every 10 s while anyone is joined. */
int phone_heap_min_kb(void);
void phone_tick(void);

/* Phones joined to the camera's Wi-Fi right now */
int phone_clients(void);

/* Called (from the web server's task) with a short message when the phone has fetched
 * a photo or deleted one, e.g. "sent DSC_0012". */
void phone_on_activity(void (*cb)(const char *msg, bool deleted));
