/*
 * ThumbyP8 — fantasy console machine state.
 *
 * Just the boring bits: reset to a sensible default, the canonical
 * PICO-8 16-color palette in RGB565, and the framebuffer expand
 * routine that turns 4bpp packed pixels into a linear RGB565 buffer
 * the host or LCD driver can DMA out.
 */
#include "p8_machine.h"
#include <string.h>

/* --- canonical PICO-8 palette in RGB565 ------------------------------- */
/* Reference colors (RGB888):
 *  0 black     000000   1 dark-blue 1d2b53   2 dark-purple 7e2553
 *  3 dark-green 008751  4 brown    ab5236    5 dark-grey  5f574f
 *  6 light-grey c2c3c7  7 white    fff1e8    8 red        ff004d
 *  9 orange    ffa300   10 yellow  ffec27    11 green     00e436
 * 12 blue      29adff   13 lavender 83769c   14 pink      ff77a8
 * 15 peach     ffccaa
 * Converted to RGB565: ((R>>3)<<11) | ((G>>2)<<5) | (B>>3)
 */
static const uint16_t k_p8_palette_rgb565[16] = {
    0x0000, /*  0 #000000 */
    0x194a, /*  1 #1d2b53 */
    0x792a, /*  2 #7e2553 */
    0x042a, /*  3 #008751 */
    0xaa86, /*  4 #ab5236 */
    0x5aa9, /*  5 #5f574f */
    0xc618, /*  6 #c2c3c7 */
    0xff9d, /*  7 #fff1e8 */
    0xf809, /*  8 #ff004d */
    0xfd00, /*  9 #ffa300 */
    0xff64, /* 10 #ffec27 */
    0x0726, /* 11 #00e436 */
    0x2d7f, /* 12 #29adff */
    0x83b3, /* 13 #83769c */
    0xfbb5, /* 14 #ff77a8 */
    0xfe75, /* 15 #ffccaa */
};

void p8_machine_reset(p8_machine *m) {
    memset(m->mem, 0, sizeof(m->mem));

    /* Default draw palette: identity (color N → color N). */
    for (int i = 0; i < 16; i++) {
        m->mem[P8_DS_DRAW_PAL + i]   = (uint8_t)i;
        m->mem[P8_DS_SCREEN_PAL + i] = (uint8_t)i;
    }
    /* PICO-8 default: color 0 is transparent in pal/spr terms. We
     * track that as a separate transparency bitmask in 0x5f00 high
     * bits in real PICO-8; for now we use a side flag in the draw
     * palette: high bit set means "transparent in spr". Bind that
     * default by setting bit 4 of pal entry 0. */
    m->mem[P8_DS_DRAW_PAL + 0] |= 0x10;

    /* Default clip = full screen */
    m->mem[P8_DS_CLIP_X0] = 0;
    m->mem[P8_DS_CLIP_Y0] = 0;
    m->mem[P8_DS_CLIP_X1] = 128;
    m->mem[P8_DS_CLIP_Y1] = 128;

    /* Default pen = white-ish (color 6 — light-grey is PICO-8 default) */
    m->mem[P8_DS_PEN] = 6;

    /* Camera at origin */
    p8_set_camera(m, 0, 0);

    /* Cache RGB565 palette */
    for (int i = 0; i < 16; i++) {
        m->rgb565_palette[i] = k_p8_palette_rgb565[i];
    }
}

void p8_machine_present(const p8_machine *m, uint16_t *dst) {
    /* Walk the 8 KB framebuffer two pixels at a time. The screen
     * palette indirection lets carts implement palette tricks
     * (fade-out, swap) just by editing 16 bytes of draw state. */
    const uint8_t *src = &m->mem[P8_FB_BASE];
    const uint8_t *spal = &m->mem[P8_DS_SCREEN_PAL];
    const uint16_t *pal565 = m->rgb565_palette;

    for (int i = 0; i < P8_FB_BYTES; i++) {
        uint8_t b = src[i];
        uint8_t lo = spal[b & 0x0f] & 0x0f;
        uint8_t hi = spal[(b >> 4) & 0x0f] & 0x0f;
        dst[i * 2 + 0] = pal565[lo];
        dst[i * 2 + 1] = pal565[hi];
    }
}
