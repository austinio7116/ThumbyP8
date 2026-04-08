/*
 * ThumbyP8 — minimal GC9107 LCD driver for Thumby Color (RP2350).
 *
 * 128×128 RGB565 panel, 4-wire SPI on spi0, 80 MHz.
 * Pin map (from board reference):
 *   GP18 = SCK
 *   GP19 = SDA (MOSI)
 *   GP17 = CS  (manual GPIO)
 *   GP16 = DC
 *   GP4  = RST
 *   GP7  = BL  (driven high = full brightness)
 *
 * The init sequence is the documented startup register write
 * order for the GC9107 panel — these are device-mandated values,
 * not creative work, and they match the published datasheet flow.
 */
#ifndef THUMBYP8_LCD_GC9107_H
#define THUMBYP8_LCD_GC9107_H

#include <stdint.h>

void p8_lcd_init(void);

/* Push a 128×128 RGB565 framebuffer to the panel. Blocks until the
 * previous DMA finishes, then kicks a new DMA transfer. */
void p8_lcd_present(const uint16_t *fb_rgb565);

/* Wait for any in-flight DMA push to complete. Call before reusing
 * the framebuffer for the next frame. */
void p8_lcd_wait_idle(void);

#endif
