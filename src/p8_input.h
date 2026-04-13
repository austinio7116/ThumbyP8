/*
 * ThumbyP8 — input state.
 *
 * PICO-8 has 6 buttons per player: LEFT, RIGHT, UP, DOWN, O, X
 * (indices 0..5).
 *
 * btnp() autorepeat semantics (matching PICO-8):
 *   - Fires on the frame a button becomes pressed (0 → 1 transition)
 *   - After 15 frames of continuous hold, fires again
 *   - Then every 4 frames while still held
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

#define P8_BTN_REPEAT_DELAY 15   /* frames before autorepeat starts */
#define P8_BTN_REPEAT_RATE   4   /* frames between subsequent repeats */

typedef struct p8_input {
    uint8_t cur;   /* bitmask, bit i = button i pressed this frame */
    uint8_t prev;  /* bitmask, previous frame */
    /* Per-button hold counter. 0 when released, 1+ = frames held. */
    uint8_t hold[P8_BTN_COUNT];
    /* Per-button btnp flag for this frame (bitmask). Computed by
     * begin_frame based on edge + autorepeat logic. */
    uint8_t p_flag;
} p8_input;

/* Reset input state. All buttons start unpressed; but if a button
 * is already held on reset, we mark it as "long held" so btnp doesn't
 * immediately fire (prevents phantom presses on scene transitions). */
static inline void p8_input_reset(p8_input *in) {
    in->cur = 0xff;
    in->prev = 0xff;
    in->p_flag = 0;
    for (int i = 0; i < P8_BTN_COUNT; i++) {
        in->hold[i] = P8_BTN_REPEAT_DELAY + 1;  /* already past initial delay */
    }
}

/* Called by the host runner once per frame, BEFORE _update() runs. */
static inline void p8_input_begin_frame(p8_input *in, uint8_t new_state) {
    in->prev = in->cur;
    in->cur  = new_state;
    in->p_flag = 0;
    for (int i = 0; i < P8_BTN_COUNT; i++) {
        int pressed = (new_state >> i) & 1;
        if (!pressed) {
            in->hold[i] = 0;
            continue;
        }
        /* pressed this frame */
        in->hold[i]++;
        if (in->hold[i] == 1) {
            /* Initial press — fire btnp */
            in->p_flag |= (1 << i);
        } else if (in->hold[i] > P8_BTN_REPEAT_DELAY) {
            /* In autorepeat phase. Fire every REPEAT_RATE frames
             * starting one frame after the delay. */
            int since_delay = in->hold[i] - P8_BTN_REPEAT_DELAY - 1;
            if ((since_delay % P8_BTN_REPEAT_RATE) == 0) {
                in->p_flag |= (1 << i);
            }
        }
    }
}

static inline int p8_btn(const p8_input *in, int i) {
    if ((unsigned)i >= P8_BTN_COUNT) return 0;
    return (in->cur >> i) & 1;
}
static inline int p8_btnp(const p8_input *in, int i) {
    if ((unsigned)i >= P8_BTN_COUNT) return 0;
    return (in->p_flag >> i) & 1;
}

#endif
