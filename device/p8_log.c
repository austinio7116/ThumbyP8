/*
 * ThumbyP8 — diagnostic logging implementation.
 */
#include "p8_log.h"
#include "p8_lcd_gc9107.h"
#include "p8_flash_disk.h"
#include "p8_draw.h"
#include "p8_font.h"

#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "ff.h"

/* Approximate free heap. newlib's mallinfo returns 'fordblks' which
 * is the total free space in the currently extended malloc arena. */
static unsigned free_heap_bytes(void) {
    struct mallinfo mi = mallinfo();
    return (unsigned)mi.fordblks;
}

/* Scrollback ring of recent stage messages so we can see the history
 * of where the loader got, not just the most recent line. */
#define STAGE_LINES 6
#define STAGE_LEN   28
static char     stage_ring[STAGE_LINES][STAGE_LEN];
static int      stage_count = 0;

static void stage_push(const char *msg) {
    if (stage_count < STAGE_LINES) {
        strncpy(stage_ring[stage_count], msg, STAGE_LEN - 1);
        stage_ring[stage_count][STAGE_LEN - 1] = 0;
        stage_count++;
    } else {
        /* shift up */
        for (int i = 0; i < STAGE_LINES - 1; i++) {
            memcpy(stage_ring[i], stage_ring[i+1], STAGE_LEN);
        }
        strncpy(stage_ring[STAGE_LINES - 1], msg, STAGE_LEN - 1);
        stage_ring[STAGE_LINES - 1][STAGE_LEN - 1] = 0;
    }
}

void p8_log_stage(p8_machine *m, uint16_t *scanline, const char *msg) {
    if (!m || !scanline || !msg) return;
    stage_push(msg);

    /* Clear the bottom half and redraw the scrollback. */
    p8_rectfill(m, 0, 48, 127, 117, 0);
    for (int i = 0; i < stage_count; i++) {
        p8_font_draw(m, stage_ring[i], 2, 50 + i * 7, 10);
    }
    char heap[40];
    snprintf(heap, sizeof(heap), "heap free: %u", free_heap_bytes());
    p8_font_draw(m, heap, 2, 100, 9);
    p8_font_draw(m, "decoding takes 30-60s", 0, 110, 8);
    p8_machine_present(m, scanline);
    p8_lcd_wait_idle();
    p8_lcd_present(scanline);
}

/* Internal counter so we can prefix every log line with a sequence
 * number. */
static uint32_t log_seq = 0;

/* RAM-only event ring. Holds the last RING_LINES events as fixed
 * 56-char strings. The cart-run loop pushes "phase tags" here on
 * every frame transition; nothing touches flash so there's no
 * write amplification or audio glitching. The hardfault handler
 * drains this ring to /thumbyp8.log so the next boot can see
 * exactly what the device was doing just before it faulted. */
#define RING_LINES   32
#define RING_LINE_LEN 56
static char     ring_buf[RING_LINES][RING_LINE_LEN];
static uint32_t ring_head = 0;     /* next write position */
static uint32_t ring_count = 0;    /* number of valid entries (capped at RING_LINES) */
static uint32_t ring_seq = 0;      /* monotonic seq for entries */

void p8_log_ring(const char *msg) {
    if (!msg) return;
    ring_seq++;
    int slot = ring_head % RING_LINES;
    snprintf(ring_buf[slot], RING_LINE_LEN, "[r%lu] %s",
             (unsigned long)ring_seq, msg);
    ring_head++;
    if (ring_count < RING_LINES) ring_count++;
}

void p8_log_dump_ring(void) {
    if (ring_count == 0) return;
    FIL f;
    if (f_open(&f, "/thumbyp8.log", FA_WRITE | FA_OPEN_APPEND) != FR_OK) return;
    UINT bw;
    f_write(&f, "--- ring buffer dump ---\n", 25, &bw);
    /* Walk in chronological order: oldest entry first. */
    uint32_t start = (ring_count == RING_LINES)
                   ? (ring_head % RING_LINES) : 0;
    for (uint32_t i = 0; i < ring_count; i++) {
        int idx = (start + i) % RING_LINES;
        f_write(&f, ring_buf[idx], (UINT)strlen(ring_buf[idx]), &bw);
        f_write(&f, "\n", 1, &bw);
    }
    f_write(&f, "--- end ring ---\n", 17, &bw);
    f_close(&f);
    p8_flash_disk_flush();
}

void p8_log_to_file(const char *msg) {
    if (!msg) return;
    log_seq++;

    FIL f;
    FRESULT r = f_open(&f, "/thumbyp8.log", FA_WRITE | FA_OPEN_APPEND);
    if (r != FR_OK) {
        /* The file system may not be mounted, or the disk may be
         * full / corrupted. There's nothing we can usefully do here
         * except keep going. The on-screen error display is the
         * fallback for users. */
        return;
    }

    /* Prefix: "[seq] " so the log timeline is unambiguous. */
    char prefix[32];
    int plen = snprintf(prefix, sizeof(prefix), "[%lu] ",
                         (unsigned long)log_seq);
    UINT bw;
    if (plen > 0) f_write(&f, prefix, (UINT)plen, &bw);
    f_write(&f, msg, (UINT)strlen(msg), &bw);
    f_write(&f, "\n", 1, &bw);
    f_close(&f);

    /* Force cache → flash so a subsequent hang/crash doesn't lose
     * the line we just wrote. NOTE: this blocks IRQs for ~50 ms
     * per dirty block, so it should only be called outside the
     * audio-critical path. */
    p8_flash_disk_flush();
}
