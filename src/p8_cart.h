/*
 * ThumbyP8 — .p8 text cart loader.
 *
 * PICO-8 carts ship in two formats:
 *   .p8       text, section markers like __lua__, __gfx__, __map__
 *   .p8.png   PNG with cart bytes steganographically encoded
 *
 * Phase 1+2 only handles .p8 text. We extract:
 *   __lua__   raw source, returned via cart->lua_source
 *   __gfx__   128 lines × 128 hex chars → 8 KB at 0x0000
 *   __gff__   sprite flags → 256 bytes at 0x3000
 *   __map__   tilemap → at 0x2000 (upper) / 0x1000 (lower)
 *
 * The Lua source is NOT yet rewritten through a PICO-8 dialect
 * pre-tokenizer (Phase 3+); test carts must use vanilla Lua 5.4
 * syntax for now.
 */
#ifndef THUMBYP8_CART_H
#define THUMBYP8_CART_H

#include "p8_machine.h"

typedef struct p8_cart {
    char *lua_source;     /* malloc'd, NUL-terminated; NULL if none */
    size_t lua_size;
} p8_cart;

/* Load a .p8 cart from a raw byte buffer. Decodes binary sections
 * directly into `m->mem`. The src buffer is not retained — the cart's
 * Lua source is malloc'd and copied. Free with p8_cart_free(). */
int  p8_cart_load_from_memory(p8_cart *cart, p8_machine *m,
                              const char *src, size_t src_len);

/* Host convenience: load from a filesystem path. Not available on
 * bare-metal device builds (no fopen). */
#ifndef PICO_ON_DEVICE
int  p8_cart_load(p8_cart *cart, p8_machine *m, const char *path);
#endif

void p8_cart_free(p8_cart *cart);

#endif
