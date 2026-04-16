/*
 * ThumbyP8 Phase 0 benchmark harness
 *
 * Measures raw Lua 5.4 interpreter throughput on the host CPU.
 * Each benchmark is a small Lua chunk run for a target wall-clock
 * duration; we report iterations/sec and ns/iter.
 *
 * Why these specific benchmarks: they isolate the kinds of work
 * a PICO-8 cart actually does most often.
 *
 *   1. empty_loop    — tightest possible interpreter overhead.
 *                      Bounds the dispatch + branch cost.
 *
 *   2. arith         — integer + float arithmetic in a loop.
 *                      Models physics / position math.
 *
 *   3. table_rw      — array-style table reads + writes.
 *                      Models particle systems / sprite lists.
 *
 *   4. fn_call       — Lua-to-Lua function call overhead.
 *                      Models per-entity update() dispatch.
 *
 *   5. string_concat — short string building.
 *                      Models score / HUD text.
 *
 *   6. trig          — math.sin / math.cos.
 *                      PICO-8 sin/cos take turns, but the
 *                      underlying libm cost dominates.
 *
 * Phase 0 PASS criterion (host): all six should report >50M iter/sec
 * on a modern x86. The number that *actually matters* is the same
 * harness run on RP2350 — that comes when we cross-compile in the
 * next step.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p8.h"

#ifdef PICO_ON_DEVICE
  #include "pico/stdlib.h"
  #include "pico/time.h"
  static uint64_t now_ns(void) { return time_us_64() * 1000ull; }
#else
  #include <time.h>
  static uint64_t now_ns(void) {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
  }
#endif

typedef struct {
    const char *name;
    const char *src;       /* Lua source defining function `bench(n)` */
    int         per_iter;  /* logical work units per `n` */
} bench_t;

static const bench_t benches[] = {
    {
        .name = "empty_loop", .per_iter = 1,
        .src  = "function bench(n) for i=1,n do end end"
    },
    {
        .name = "arith", .per_iter = 4,
        .src  =
            "function bench(n)\n"
            "  local s = 0\n"
            "  for i=1,n do s = s + i*3 - 1 end\n"
            "  return s\n"
            "end"
    },
    {
        /* Fixed 256-entry table — exercises read+write hot path
         * without growing memory. Models a sprite/particle pool. */
        .name = "table_rw", .per_iter = 2,
        .src  =
            "function bench(n)\n"
            "  local t = {}\n"
            "  for i=1,256 do t[i] = 0 end\n"
            "  local s = 0\n"
            "  for i=1,n do\n"
            "    local k = (i & 255) + 1\n"
            "    t[k] = i\n"
            "    s = s + t[k]\n"
            "  end\n"
            "  return s\n"
            "end"
    },
    {
        .name = "fn_call", .per_iter = 1,
        .src  =
            "local function f(x) return x+1 end\n"
            "function bench(n)\n"
            "  local s = 0\n"
            "  for i=1,n do s = f(s) end\n"
            "  return s\n"
            "end"
    },
    {
        /* assignment, not concat — concat would OOM the cap */
        .name = "string_assign", .per_iter = 1,
        .src  =
            "function bench(n)\n"
            "  local s = ''\n"
            "  for i=1,n do s = 'x' end\n"
            "  return s\n"
            "end"
    },
    {
        .name = "trig", .per_iter = 2,
        .src  =
            "local sin, cos = math.sin, math.cos\n"
            "function bench(n)\n"
            "  local s = 0\n"
            "  for i=1,n do s = s + sin(i) * cos(i) end\n"
            "  return s\n"
            "end"
    },
};
#define N_BENCHES (sizeof(benches) / sizeof(benches[0]))

/* Run `bench(n)` once and return wall-clock nanoseconds. */
static uint64_t time_one_run(p8_vm *vm, int n) {
    lua_getglobal(vm->L, "bench");
    lua_pushinteger(vm->L, n);
    uint64_t t0 = now_ns();
    int rc = lua_pcall(vm->L, 1, 1, 0);
    uint64_t t1 = now_ns();
    if (rc != LUA_OK) {
        fprintf(stderr, "  lua_pcall failed: %s\n",
                lua_tostring(vm->L, -1));
        lua_pop(vm->L, 1);
        return 0;
    }
    lua_pop(vm->L, 1);  /* drop result */
    return t1 - t0;
}

/* Calibrate `n` so each run takes ~target_ms, then average several
 * runs. Reports both ops/sec and ns/op. */
static void run_bench(const bench_t *b, int target_ms) {
    p8_vm vm;
    if (p8_vm_init(&vm, 0) != 0) {
        printf("  %-15s FAILED to init VM\n", b->name);
        return;
    }

    if (p8_vm_do_string(&vm, b->src, b->name) != LUA_OK) {
        printf("  %-15s LOAD ERROR: %s\n",
               b->name, p8_vm_last_error_msg(&vm));
        p8_vm_free(&vm);
        return;
    }

    /* Calibration: start at n=1000, double until we cross 20ms. */
    int n = 1000;
    uint64_t ns;
    for (;;) {
        ns = time_one_run(&vm, n);
        if (ns == 0) { p8_vm_free(&vm); return; }
        if (ns >= 20ull * 1000000ull) break;
        if (n >= (1 << 24)) break;
        n *= 2;
    }

    /* Scale n so one run is ~target_ms. */
    if (ns > 0) {
        double scale = (double)(target_ms * 1000000.0) / (double)ns;
        if (scale < 1.0) scale = 1.0;
        n = (int)((double)n * scale);
        if (n < 1) n = 1;
    }

    /* Average over 5 runs, report best (least noise). */
    uint64_t best = ~0ull;
    for (int i = 0; i < 5; i++) {
        uint64_t r = time_one_run(&vm, n);
        if (r && r < best) best = r;
    }

    double ms      = best / 1.0e6;
    double ops     = (double)n * (double)b->per_iter;
    double ops_sec = ops / (best / 1.0e9);
    double ns_op   = (double)best / ops;

    printf("  %-15s n=%-9d %7.2f ms   %10.2f Mops/s   %7.1f ns/op\n",
           b->name, n, ms, ops_sec / 1.0e6, ns_op);

    printf("                  Lua heap: in_use=%zu peak=%zu cap=%zu\n",
           vm.bytes_in_use, vm.bytes_peak, vm.bytes_cap);

    p8_vm_free(&vm);
}

#ifdef PICO_ON_DEVICE
static void run_all_benches(int target_ms);


#include "pico/stdio_usb.h"
#include "hardware/clocks.h"
int main(void) {
    /* Overclock to 250 MHz BEFORE stdio_init_all so the USB PLL
     * comes up with the right divider. The Tiny Game Engine runs
     * the same chip at 250 MHz in production, so this is known-good. */
    set_sys_clock_khz(250000, true);

    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Wait (up to 30s) for the host to actually open the CDC port.
     * Bytes printed before the host attaches are dropped by TinyUSB. */
    for (int i = 0; i < 300 && !stdio_usb_connected(); i++) {
        sleep_ms(100);
    }
    sleep_ms(500);   /* small grace period */

    int run = 0;
    while (1) {
        printf("\n\n=== ThumbyP8 device bench  run #%d ===\n", ++run);
        printf("sys_clk = %u kHz   lua_Number = %s\n",
               (unsigned)clock_get_hz(clk_sys) / 1000,
               "int32 fixed-point 16.16");
        run_all_benches(100);
        printf("=== run #%d complete — sleeping 5s ===\n", run);
        for (int i = 0; i < 50; i++) sleep_ms(100);
    }
    return 0;
}
static void run_all_benches(int target_ms) {
#else
int main(int argc, char **argv) {
    int target_ms = 100;
    if (argc > 1) target_ms = atoi(argv[1]);
    if (target_ms < 10) target_ms = 10;
#endif

    printf("ThumbyP8 — Phase 0 Lua VM benchmark\n");
    printf("Lua version: %s\n", LUA_RELEASE);
    printf("Heap cap:    %d KB\n", P8_LUA_HEAP_CAP / 1024);
    printf("Target run:  %d ms\n", target_ms);
    printf("\n");

    /* Sanity: VM init and trivial eval before the bench loop. */
    {
        p8_vm vm;
        if (p8_vm_init(&vm, 0) != 0) {
            fprintf(stderr, "FATAL: p8_vm_init failed\n");
            goto smoke_done;
        }
        if (p8_vm_do_string(&vm, "return 1+2", "smoke") != LUA_OK) {
            fprintf(stderr, "FATAL: smoke test failed: %s\n",
                    p8_vm_last_error_msg(&vm));
            goto smoke_done;
        }
        printf("Smoke test: VM init OK, '1+2' evaluated OK.\n\n");
smoke_done:
        p8_vm_free(&vm);
    }

    printf("Benchmarks (best of 5):\n");
    for (size_t i = 0; i < N_BENCHES; i++) {
        run_bench(&benches[i], target_ms);
    }

    printf("\nPhase 0 bench complete.\n");
#ifdef PICO_ON_DEVICE
}
#else
    return 0;
}
#endif
