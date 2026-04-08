/*
 * ThumbyP8 — Thumby Color physical button reader.
 *
 * GPIO map (active low, internal pull-ups):
 *   GP0  LEFT     GP1  UP       GP2  RIGHT     GP3  DOWN
 *   GP21 A        GP25 B        GP6  LB        GP22 RB
 *   GP26 MENU
 *
 * Mapping to PICO-8 6-bit button mask:
 *   bit 0 LEFT  ← LEFT
 *   bit 1 RIGHT ← RIGHT
 *   bit 2 UP    ← UP
 *   bit 3 DOWN  ← DOWN
 *   bit 4 O     ← B (or LB)
 *   bit 5 X     ← A (or RB)
 *
 * The Thumby Color labels A on the right and B on the left, but
 * PICO-8's idiomatic mapping is X = jump (right button), O = action
 * (left button). We follow that convention so PICO-8 muscle memory
 * works with the physical layout.
 */
#include "p8_buttons.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define BTN_LEFT_GP   0
#define BTN_UP_GP     1
#define BTN_RIGHT_GP  2
#define BTN_DOWN_GP   3
#define BTN_LB_GP     6
#define BTN_A_GP     21
#define BTN_RB_GP    22
#define BTN_B_GP     25
#define BTN_MENU_GP  26

static void init_pull_up(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

void p8_buttons_init(void) {
    init_pull_up(BTN_LEFT_GP);
    init_pull_up(BTN_UP_GP);
    init_pull_up(BTN_RIGHT_GP);
    init_pull_up(BTN_DOWN_GP);
    init_pull_up(BTN_LB_GP);
    init_pull_up(BTN_A_GP);
    init_pull_up(BTN_RB_GP);
    init_pull_up(BTN_B_GP);
    init_pull_up(BTN_MENU_GP);
}

uint8_t p8_buttons_read(void) {
    /* gpio_get returns 1 when the line is high; pressed = low. */
    uint8_t b = 0;
    if (!gpio_get(BTN_LEFT_GP))  b |= 1 << 0;
    if (!gpio_get(BTN_RIGHT_GP)) b |= 1 << 1;
    if (!gpio_get(BTN_UP_GP))    b |= 1 << 2;
    if (!gpio_get(BTN_DOWN_GP))  b |= 1 << 3;
    if (!gpio_get(BTN_B_GP) || !gpio_get(BTN_LB_GP))  b |= 1 << 4;  /* O */
    if (!gpio_get(BTN_A_GP) || !gpio_get(BTN_RB_GP))  b |= 1 << 5;  /* X */
    return b;
}

int p8_buttons_menu_pressed(void) {
    return !gpio_get(BTN_MENU_GP);
}
