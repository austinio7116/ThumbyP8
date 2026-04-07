/*
 * ThumbyP8 — fantasy console "machine" state
 *
 * Mirrors the documented PICO-8 memory map. Single 64 KB byte
 * array; everything else is offsets into it. This is deliberate:
 * peek/poke, the framebuffer, sprite sheet, map, and the draw
 * state are all just regions of `mem`.
 *
 *  0x0000–0x0fff   sprite sheet (gfx) — 128×128 4bpp = 8 KB
 *  0x1000–0x1fff   shared lower-half map (overlaps gfx)
 *  0x2000–0x2fff   upper-half map
 *  0x3000–0x30ff   sprite flags (gff) — 256 bytes
 *  0x3100–0x31ff   music — 256 bytes (Phase 4)
 *  0x3200–0x42ff   sfx — 4352 bytes  (Phase 4)
 *  0x4300–0x5dff   user data
 *  0x5e00–0x5eff   persistent cart data (Phase 6)
 *  0x5f00–0x5fff   draw state / hardware regs
 *  0x6000–0x7fff   framebuffer — 128×128 4bpp = 8 KB
 *  0x8000–0xffff   general RAM
 *
 * Phase 1+2 only touches gfx, gff, map, draw state, and framebuffer.
 */
#ifndef THUMBYP8_MACHINE_H
#define THUMBYP8_MACHINE_H

#include <stdint.h>
#include <stddef.h>

#define P8_MEM_SIZE      0x10000
#define P8_GFX_BASE      0x0000
#define P8_MAP_BASE      0x1000   /* lower half (shared with gfx 0x1000–0x1fff) */
#define P8_GFF_BASE      0x3000   /* sprite flags */
#define P8_DRAWSTATE     0x5f00
#define P8_FB_BASE       0x6000

#define P8_SCREEN_W      128
#define P8_SCREEN_H      128
#define P8_FB_BYTES      (128 * 128 / 2)   /* 4bpp */

#define P8_MAP_W         128
#define P8_MAP_H         64

/* Draw-state offsets within 0x5f00 (subset we actually use) */
#define P8_DS_DRAW_PAL   (P8_DRAWSTATE + 0x00)  /* 16 bytes: draw palette remap */
#define P8_DS_SCREEN_PAL (P8_DRAWSTATE + 0x10)  /* 16 bytes: screen palette */
#define P8_DS_CLIP_X0    (P8_DRAWSTATE + 0x20)
#define P8_DS_CLIP_Y0    (P8_DRAWSTATE + 0x21)
#define P8_DS_CLIP_X1    (P8_DRAWSTATE + 0x22)
#define P8_DS_CLIP_Y1    (P8_DRAWSTATE + 0x23)
#define P8_DS_PEN        (P8_DRAWSTATE + 0x25)
#define P8_DS_CURSOR_X   (P8_DRAWSTATE + 0x26)
#define P8_DS_CURSOR_Y   (P8_DRAWSTATE + 0x27)
#define P8_DS_CAMERA_X   (P8_DRAWSTATE + 0x28)  /* int16 LE */
#define P8_DS_CAMERA_Y   (P8_DRAWSTATE + 0x2a)  /* int16 LE */

typedef struct p8_machine {
    uint8_t mem[P8_MEM_SIZE];

    /* PICO-8 16-color palette as RGB565, mirroring the default palette.
     * Indexed by the *screen* palette mapping (DS_SCREEN_PAL) at present
     * time, not the raw color index — see p8_machine_present(). */
    uint16_t rgb565_palette[16];
} p8_machine;

void p8_machine_reset(p8_machine *m);

/* --- draw state convenience accessors ---------------------------------- */
static inline int16_t p8_camera_x(const p8_machine *m) {
    return (int16_t)(m->mem[P8_DS_CAMERA_X] | (m->mem[P8_DS_CAMERA_X + 1] << 8));
}
static inline int16_t p8_camera_y(const p8_machine *m) {
    return (int16_t)(m->mem[P8_DS_CAMERA_Y] | (m->mem[P8_DS_CAMERA_Y + 1] << 8));
}
static inline void p8_set_camera(p8_machine *m, int16_t x, int16_t y) {
    m->mem[P8_DS_CAMERA_X]     = (uint8_t)(x & 0xff);
    m->mem[P8_DS_CAMERA_X + 1] = (uint8_t)((x >> 8) & 0xff);
    m->mem[P8_DS_CAMERA_Y]     = (uint8_t)(y & 0xff);
    m->mem[P8_DS_CAMERA_Y + 1] = (uint8_t)((y >> 8) & 0xff);
}

static inline uint8_t p8_pen(const p8_machine *m) { return m->mem[P8_DS_PEN]; }
static inline void    p8_set_pen(p8_machine *m, uint8_t c) { m->mem[P8_DS_PEN] = c & 0x0f; }

/* Apply the draw palette remap (used by drawing primitives). */
static inline uint8_t p8_draw_remap(const p8_machine *m, uint8_t c) {
    return m->mem[P8_DS_DRAW_PAL + (c & 0x0f)] & 0x0f;
}

/* --- framebuffer pixel access ----------------------------------------- */
/* Both treat (x,y) as raw screen coordinates AFTER camera/clip handling. */
static inline void p8_fb_pset_raw(p8_machine *m, int x, int y, uint8_t c) {
    /* assumes 0 <= x < 128, 0 <= y < 128 */
    int addr = P8_FB_BASE + (y << 6) + (x >> 1);
    uint8_t b = m->mem[addr];
    if (x & 1) {
        m->mem[addr] = (b & 0x0f) | ((c & 0x0f) << 4);
    } else {
        m->mem[addr] = (b & 0xf0) | (c & 0x0f);
    }
}
static inline uint8_t p8_fb_pget_raw(const p8_machine *m, int x, int y) {
    int addr = P8_FB_BASE + (y << 6) + (x >> 1);
    uint8_t b = m->mem[addr];
    return (x & 1) ? (b >> 4) : (b & 0x0f);
}

/* --- sprite sheet pixel access (4bpp, same layout as fb) -------------- */
static inline uint8_t p8_sget(const p8_machine *m, int x, int y) {
    if ((unsigned)x >= 128 || (unsigned)y >= 128) return 0;
    int addr = P8_GFX_BASE + (y << 6) + (x >> 1);
    uint8_t b = m->mem[addr];
    return (x & 1) ? (b >> 4) : (b & 0x0f);
}
static inline void p8_sset(p8_machine *m, int x, int y, uint8_t c) {
    if ((unsigned)x >= 128 || (unsigned)y >= 128) return;
    int addr = P8_GFX_BASE + (y << 6) + (x >> 1);
    uint8_t b = m->mem[addr];
    if (x & 1) m->mem[addr] = (b & 0x0f) | ((c & 0x0f) << 4);
    else       m->mem[addr] = (b & 0xf0) | (c & 0x0f);
}

/* --- expand 4bpp framebuffer into RGB565 scanline buffer -------------- */
/* dst must hold 128*128 uint16_t. */
void p8_machine_present(const p8_machine *m, uint16_t *dst);

#endif /* THUMBYP8_MACHINE_H */
