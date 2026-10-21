#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * Serial link to the laptop simulator over the board's CH340 USB port (UART0).
 *
 * Device -> laptop frames:  A5 5A | type u8 | len u32 LE | payload | crc32 u32 LE
 *   'F' preview frame (JPEG)
 *   'S' status (JSON)
 *   'R' command reply (JSON)
 *   'D' file data: req id u32, offset u32, total u32, then bytes
 *   'L' log line (text)
 *
 * Laptop -> device: one JSON command per line, e.g. {"id":3,"cmd":"photo"}
 */

/* 1.5 Mbaud: the fastest rate the CH340 carried without errors on a Mac (2 Mbaud dropped bytes). */
#define LINK_BAUD 1500000

typedef void (*link_line_cb_t)(const char *line);

esp_err_t link_init(link_line_cb_t on_line);

/* Blocking send of one frame. `head` is optional bytes sent before `body`. */
void link_send(char type, const void *head, size_t head_len, const void *body, size_t body_len);
void link_send_json(char type, const char *json);

/* Change the link speed after the reply to the current command has gone out. */
void link_set_baud(uint32_t baud);
