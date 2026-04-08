/*
 * ThumbyP8 — Thumby Color physical button reader.
 */
#ifndef THUMBYP8_BUTTONS_H
#define THUMBYP8_BUTTONS_H

#include <stdint.h>

void    p8_buttons_init(void);
uint8_t p8_buttons_read(void);   /* PICO-8 6-bit mask: LRUDOX */
int     p8_buttons_menu_pressed(void);

#endif
