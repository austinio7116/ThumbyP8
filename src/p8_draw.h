/*
 * ThumbyP8 — drawing primitives
 *
 * Implements the subset of PICO-8 draw calls Phase 1+2 needs:
 *
 *    cls(c)                       clear screen
 *    pset(x,y,c) / pget(x,y)      single pixel
 *    line(x0,y0,x1,y1,c)
 *    rect(x0,y0,x1,y1,c)
 *    rectfill(x0,y0,x1,y1,c)
 *    circ(x,y,r,c)
 *    circfill(x,y,r,c)
 *    spr(n,x,y,w,h,flip_x,flip_y)
 *    sspr(sx,sy,sw,sh,dx,dy,dw,dh,flip_x,flip_y)
 *    map(cx,cy,sx,sy,cw,ch,layer)
 *
 * Coordinates are signed (PICO-8 carts pass negatives all the time).
 * The camera offset is subtracted from input coords; clipping is
 * enforced against the draw-state clip rect.
 *
 * Color handling: every primitive routes through p8_draw_remap()
 * so the draw palette (pal()) and transparency (palt()) Just Work.
 */
#ifndef THUMBYP8_DRAW_H
#define THUMBYP8_DRAW_H

#include "p8_machine.h"

void p8_cls(p8_machine *m, int c);
void p8_pset(p8_machine *m, int x, int y, int c);
int  p8_pget(const p8_machine *m, int x, int y);

void p8_line(p8_machine *m, int x0, int y0, int x1, int y1, int c);
void p8_rect(p8_machine *m, int x0, int y0, int x1, int y1, int c);
void p8_rectfill(p8_machine *m, int x0, int y0, int x1, int y1, int c);
void p8_circ(p8_machine *m, int x, int y, int r, int c);
void p8_circfill(p8_machine *m, int x, int y, int r, int c);

void p8_clip(p8_machine *m, int x, int y, int w, int h, int reset);
void p8_camera(p8_machine *m, int x, int y);
void p8_pal_set(p8_machine *m, int c0, int c1, int p);
void p8_palt(p8_machine *m, int c, int t);
void p8_pal_reset(p8_machine *m);
void p8_color(p8_machine *m, int c);

/* Sprite blit. n is sprite index (0..255 for first page, etc.).
 * w/h are in cells (8 px each); flip_x/flip_y are 0/1. */
void p8_spr(p8_machine *m, int n, int x, int y, int w, int h, int flip_x, int flip_y);

/* Stretched sprite blit (nearest-neighbor). */
void p8_sspr(p8_machine *m,
             int sx, int sy, int sw, int sh,
             int dx, int dy, int dw, int dh,
             int flip_x, int flip_y);

/* Tilemap render. cx,cy,cw,ch in tile cells; sx,sy in pixels; layer
 * is the gff (sprite-flag) bitmask: only sprites whose flags &
 * layer != 0 are drawn. layer==0 draws everything. */
void p8_map(p8_machine *m, int cx, int cy, int sx, int sy, int cw, int ch, int layer);

/* Sprite-flag (gff) accessors. f=-1 returns/sets all 8 bits. */
int  p8_fget(const p8_machine *m, int n, int f);
void p8_fset(p8_machine *m, int n, int f, int v);

/* Tilemap accessors. */
int  p8_mget(const p8_machine *m, int x, int y);
void p8_mset(p8_machine *m, int x, int y, int v);

#endif
