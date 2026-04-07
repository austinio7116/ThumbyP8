/*
 * ThumbyP8 — input state.
 *
 * PICO-8 has 6 buttons per player: LEFT, RIGHT, UP, DOWN, O, X
 * (indices 0..5). We track press state + previous press state so
 * btnp() can report "just pressed" with PICO-8's autorepeat semantics.
 *
 * Phase 1+2: single player, no autorepeat (btnp = "just pressed
 * this frame"). Real PICO-8 autorepeat will land in a later phase.
 */
#ifndef THUMBYP8_INPUT_H
#define THUMBYP8_INPUT_H

#include <stdint.h>

#define P8_BTN_LEFT  0
#define P8_BTN_RIGHT 1
#define P8_BTN_UP    2
#define P8_BTN_DOWN  3
#define P8_BTN_O     4
#define P8_BTN_X     5
#define P8_BTN_COUNT 6

typedef struct p8_input {
    uint8_t cur;   /* bitmask, bit i = button i pressed this frame */
    uint8_t prev;  /* bitmask, previous frame */
} p8_input;

static inline void p8_input_reset(p8_input *in) { in->cur = 0; in->prev = 0; }

/* Called by the host runner once per frame, BEFORE _update() runs. */
static inline void p8_input_begin_frame(p8_input *in, uint8_t new_state) {
    in->prev = in->cur;
    in->cur  = new_state;
}

static inline int p8_btn(const p8_input *in, int i) {
    if ((unsigned)i >= P8_BTN_COUNT) return 0;
    return (in->cur >> i) & 1;
}
static inline int p8_btnp(const p8_input *in, int i) {
    if ((unsigned)i >= P8_BTN_COUNT) return 0;
    return ((in->cur & ~in->prev) >> i) & 1;
}

#endif
