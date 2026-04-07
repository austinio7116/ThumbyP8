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

/* Load a .p8 file from disk. Decodes binary sections directly into
 * `m->mem`. Returns 0 on success, nonzero on error (message printed
 * to stderr). The cart's Lua source is owned by `cart` and must be
 * freed with p8_cart_free(). */
int  p8_cart_load(p8_cart *cart, p8_machine *m, const char *path);
void p8_cart_free(p8_cart *cart);

#endif
