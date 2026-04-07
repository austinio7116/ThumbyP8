/*
 * ThumbyP8 — drawing primitives implementation.
 *
 * Pixel hot path is `pset_clipped()`. Everything else (lines,
 * rects, circles, sprites, map) reduces to it. We deliberately
 * keep this simple — Phase 1+2 wants visible-and-correct first,
 * then we'll inline + viper-port the hot loops in a later pass.
 */
#include "p8_draw.h"
#include <stdint.h>

/* --- low-level pixel ops with camera + clip + draw palette ----------- */

/* Returns 1 if (sx,sy) is inside the clip rect. sx/sy are screen coords. */
static inline int in_clip(const p8_machine *m, int sx, int sy) {
    int x0 = m->mem[P8_DS_CLIP_X0];
    int y0 = m->mem[P8_DS_CLIP_Y0];
    int x1 = m->mem[P8_DS_CLIP_X1];
    int y1 = m->mem[P8_DS_CLIP_Y1];
    return sx >= x0 && sy >= y0 && sx < x1 && sy < y1
        && sx >= 0 && sy >= 0 && sx < P8_SCREEN_W && sy < P8_SCREEN_H;
}

/* Plot at world (cart) coordinates with the given color. Applies
 * camera offset, clip rect, and the *draw* palette remap. */
static inline void pset_world(p8_machine *m, int x, int y, int c) {
    int sx = x - p8_camera_x(m);
    int sy = y - p8_camera_y(m);
    if (!in_clip(m, sx, sy)) return;
    /* low nibble = remapped color; high bit of pal entry = transparent
     * (only used by sprite blit, ignored here). */
    uint8_t mapped = m->mem[P8_DS_DRAW_PAL + (c & 0x0f)] & 0x0f;
    p8_fb_pset_raw(m, sx, sy, mapped);
}

/* Plot at *screen* coordinates (used by sprite blit, which already
 * baked the camera transform). Still applies clip + draw palette. */
static inline void pset_screen_remapped(p8_machine *m, int sx, int sy, int c) {
    if (!in_clip(m, sx, sy)) return;
    uint8_t mapped = m->mem[P8_DS_DRAW_PAL + (c & 0x0f)] & 0x0f;
    p8_fb_pset_raw(m, sx, sy, mapped);
}

/* --- public primitives ----------------------------------------------- */

void p8_cls(p8_machine *m, int c) {
    /* PICO-8 cls also resets the cursor; we model that. Camera is
     * unchanged. The fill ignores the draw palette (PICO-8 quirk:
     * cls writes the raw color, no pal()). */
    uint8_t cc = (uint8_t)(c & 0x0f);
    uint8_t byte = (uint8_t)(cc | (cc << 4));
    for (int i = 0; i < P8_FB_BYTES; i++) {
        m->mem[P8_FB_BASE + i] = byte;
    }
    m->mem[P8_DS_CURSOR_X] = 0;
    m->mem[P8_DS_CURSOR_Y] = 0;
}

void p8_pset(p8_machine *m, int x, int y, int c) {
    p8_set_pen(m, (uint8_t)(c & 0x0f));
    pset_world(m, x, y, c);
}

int p8_pget(const p8_machine *m, int x, int y) {
    int sx = x - p8_camera_x(m);
    int sy = y - p8_camera_y(m);
    if ((unsigned)sx >= P8_SCREEN_W || (unsigned)sy >= P8_SCREEN_H) return 0;
    return p8_fb_pget_raw(m, sx, sy);
}

void p8_color(p8_machine *m, int c) {
    p8_set_pen(m, (uint8_t)(c & 0x0f));
}

void p8_camera(p8_machine *m, int x, int y) {
    p8_set_camera(m, (int16_t)x, (int16_t)y);
}

void p8_clip(p8_machine *m, int x, int y, int w, int h, int reset) {
    if (reset) {
        m->mem[P8_DS_CLIP_X0] = 0;
        m->mem[P8_DS_CLIP_Y0] = 0;
        m->mem[P8_DS_CLIP_X1] = P8_SCREEN_W;
        m->mem[P8_DS_CLIP_Y1] = P8_SCREEN_H;
        return;
    }
    int x0 = x < 0 ? 0 : (x > 128 ? 128 : x);
    int y0 = y < 0 ? 0 : (y > 128 ? 128 : y);
    int x1 = x + w; if (x1 < 0) x1 = 0; if (x1 > 128) x1 = 128;
    int y1 = y + h; if (y1 < 0) y1 = 0; if (y1 > 128) y1 = 128;
    m->mem[P8_DS_CLIP_X0] = (uint8_t)x0;
    m->mem[P8_DS_CLIP_Y0] = (uint8_t)y0;
    m->mem[P8_DS_CLIP_X1] = (uint8_t)x1;
    m->mem[P8_DS_CLIP_Y1] = (uint8_t)y1;
}

/* pal(c0,c1,p): remap color c0 → c1 in palette p (0=draw, 1=screen).
 * Phase 1+2 supports both. With no args (c0<0), reset palette. */
void p8_pal_set(p8_machine *m, int c0, int c1, int p) {
    int base = (p == 1) ? P8_DS_SCREEN_PAL : P8_DS_DRAW_PAL;
    /* Preserve transparency bit (high nibble) on draw palette. */
    uint8_t old = m->mem[base + (c0 & 0x0f)];
    m->mem[base + (c0 & 0x0f)] = (uint8_t)((old & 0xf0) | (c1 & 0x0f));
}

void p8_palt(p8_machine *m, int c, int t) {
    if (t) m->mem[P8_DS_DRAW_PAL + (c & 0x0f)] |= 0x10;
    else   m->mem[P8_DS_DRAW_PAL + (c & 0x0f)] &= ~0x10;
}

void p8_pal_reset(p8_machine *m) {
    for (int i = 0; i < 16; i++) {
        m->mem[P8_DS_DRAW_PAL + i]   = (uint8_t)i;
        m->mem[P8_DS_SCREEN_PAL + i] = (uint8_t)i;
    }
    m->mem[P8_DS_DRAW_PAL + 0] |= 0x10;  /* color 0 transparent */
}

/* --- line: classic Bresenham ----------------------------------------- */
void p8_line(p8_machine *m, int x0, int y0, int x1, int y1, int c) {
    p8_set_pen(m, (uint8_t)(c & 0x0f));
    int dx =  (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = -((y1 > y0) ? (y1 - y0) : (y0 - y1));
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int x = x0, y = y0;
    for (;;) {
        pset_world(m, x, y, c);
        if (x == x1 && y == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

/* --- rect: outline only ---------------------------------------------- */
void p8_rect(p8_machine *m, int x0, int y0, int x1, int y1, int c) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    for (int x = x0; x <= x1; x++) {
        pset_world(m, x, y0, c);
        pset_world(m, x, y1, c);
    }
    for (int y = y0 + 1; y < y1; y++) {
        pset_world(m, x0, y, c);
        pset_world(m, x1, y, c);
    }
}

void p8_rectfill(p8_machine *m, int x0, int y0, int x1, int y1, int c) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            pset_world(m, x, y, c);
        }
    }
}

/* --- circle: midpoint algorithm -------------------------------------- */
static inline void circ_octants(p8_machine *m, int cx, int cy, int x, int y, int c) {
    pset_world(m, cx + x, cy + y, c);
    pset_world(m, cx - x, cy + y, c);
    pset_world(m, cx + x, cy - y, c);
    pset_world(m, cx - x, cy - y, c);
    pset_world(m, cx + y, cy + x, c);
    pset_world(m, cx - y, cy + x, c);
    pset_world(m, cx + y, cy - x, c);
    pset_world(m, cx - y, cy - x, c);
}

void p8_circ(p8_machine *m, int cx, int cy, int r, int c) {
    if (r < 0) return;
    if (r == 0) { p8_pset(m, cx, cy, c); return; }
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        circ_octants(m, cx, cy, x, y, c);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

void p8_circfill(p8_machine *m, int cx, int cy, int r, int c) {
    if (r < 0) return;
    /* Filled-circle scanline approach: for each y in [-r,r], compute
     * x extent and fill. */
    for (int dy = -r; dy <= r; dy++) {
        /* x extent where dx*dx + dy*dy <= r*r */
        int rr = r * r - dy * dy;
        if (rr < 0) continue;
        int dx = 0;
        while ((dx + 1) * (dx + 1) <= rr) dx++;
        for (int x = cx - dx; x <= cx + dx; x++) {
            pset_world(m, x, cy + dy, c);
        }
    }
}

/* --- sprite blit ----------------------------------------------------- */
/* Pulls 8×8 cells from the sprite sheet at (n*8 % 128, n/16 * 8). */
void p8_spr(p8_machine *m, int n, int x, int y, int w, int h, int flip_x, int flip_y) {
    if (w <= 0 || h <= 0) return;
    int sheet_x = (n & 0x0f) * 8;
    int sheet_y = (n >> 4) * 8;
    int pw = w * 8;
    int ph = h * 8;
    int dx0 = x - p8_camera_x(m);
    int dy0 = y - p8_camera_y(m);

    for (int py = 0; py < ph; py++) {
        int sy = sheet_y + (flip_y ? (ph - 1 - py) : py);
        for (int px = 0; px < pw; px++) {
            int sx = sheet_x + (flip_x ? (pw - 1 - px) : px);
            uint8_t col = p8_sget(m, sx, sy);
            uint8_t pal_entry = m->mem[P8_DS_DRAW_PAL + (col & 0x0f)];
            if (pal_entry & 0x10) continue;  /* transparent */
            uint8_t mapped = pal_entry & 0x0f;
            int dx = dx0 + px;
            int dy = dy0 + py;
            if (in_clip(m, dx, dy)) {
                p8_fb_pset_raw(m, dx, dy, mapped);
            }
        }
    }
}

void p8_sspr(p8_machine *m,
             int sx, int sy, int sw, int sh,
             int dx, int dy, int dw, int dh,
             int flip_x, int flip_y) {
    if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) return;
    int dx0 = dx - p8_camera_x(m);
    int dy0 = dy - p8_camera_y(m);

    /* Nearest-neighbor scaling: for each destination pixel, sample
     * the source. Cheap and correct; we'll DDA-optimize later. */
    for (int py = 0; py < dh; py++) {
        int src_y = sy + (py * sh) / dh;
        if (flip_y) src_y = sy + sh - 1 - ((py * sh) / dh);
        for (int px = 0; px < dw; px++) {
            int src_x = sx + (px * sw) / dw;
            if (flip_x) src_x = sx + sw - 1 - ((px * sw) / dw);
            uint8_t col = p8_sget(m, src_x, src_y);
            uint8_t pal_entry = m->mem[P8_DS_DRAW_PAL + (col & 0x0f)];
            if (pal_entry & 0x10) continue;
            uint8_t mapped = pal_entry & 0x0f;
            int ddx = dx0 + px;
            int ddy = dy0 + py;
            if (in_clip(m, ddx, ddy)) {
                p8_fb_pset_raw(m, ddx, ddy, mapped);
            }
        }
    }
}

/* --- map ------------------------------------------------------------- */
/* PICO-8 map layout: 128 cells wide × 64 cells tall. Lower 32 rows
 * live at 0x1000–0x1fff (overlapping the lower half of gfx); upper
 * 32 rows live at 0x2000–0x2fff. We model this by offset arithmetic. */
static inline int map_addr(int x, int y) {
    if ((unsigned)x >= 128 || (unsigned)y >= 64) return -1;
    if (y < 32) return 0x2000 + y * 128 + x;  /* upper rows */
    /* lower rows live in shared region, but PICO-8 indexes them after
     * the upper rows in cart layout. We mirror real PICO-8: rows
     * 32..63 live at 0x1000–0x1fff. */
    return 0x1000 + (y - 32) * 128 + x;
}

int p8_mget(const p8_machine *m, int x, int y) {
    int a = map_addr(x, y);
    return a < 0 ? 0 : m->mem[a];
}
void p8_mset(p8_machine *m, int x, int y, int v) {
    int a = map_addr(x, y);
    if (a >= 0) m->mem[a] = (uint8_t)(v & 0xff);
}

void p8_map(p8_machine *m, int cx, int cy, int sx, int sy, int cw, int ch, int layer) {
    for (int j = 0; j < ch; j++) {
        for (int i = 0; i < cw; i++) {
            int n = p8_mget(m, cx + i, cy + j);
            if (n == 0) continue;
            if (layer != 0) {
                int flags = m->mem[P8_GFF_BASE + n];
                if ((flags & layer) == 0) continue;
            }
            p8_spr(m, n, sx + i * 8, sy + j * 8, 1, 1, 0, 0);
        }
    }
}

/* --- sprite flags ---------------------------------------------------- */
int p8_fget(const p8_machine *m, int n, int f) {
    if ((unsigned)n >= 256) return 0;
    uint8_t flags = m->mem[P8_GFF_BASE + n];
    if (f < 0) return flags;
    return (flags >> f) & 1;
}
void p8_fset(p8_machine *m, int n, int f, int v) {
    if ((unsigned)n >= 256) return;
    if (f < 0) {
        m->mem[P8_GFF_BASE + n] = (uint8_t)(v & 0xff);
    } else {
        uint8_t b = m->mem[P8_GFF_BASE + n];
        if (v) b |= (1 << f);
        else   b &= ~(1 << f);
        m->mem[P8_GFF_BASE + n] = b;
    }
}
