# ThumbyP8

A clean-room PICO-8-compatible fantasy console runtime targeting the
TinyCircuits Thumby Color (RP2350, 128×128 RGB565, 4ch DMA audio).

This is a *new* engine — not built on top of the Tiny Game Engine. It
will eventually replace the stock firmware as a dedicated PICO-8 device.

PICO-8 is a trademark of Lexaloffle Games. ThumbyP8 is an independent,
clean-room reimplementation of the documented PICO-8 fantasy-console API
and is not affiliated with or endorsed by Lexaloffle.

## Status

**Phase 0 — Lua VM spike.** Vendor Lua 5.4.7, build a host benchmark
harness, measure raw interpreter throughput. The single go/no-go gate
for the whole project: can a Cortex-M33 @ ≤250 MHz interpret enough
Lua per frame to run real PICO-8 carts at 30 fps?

## Layout

```
ThumbyP8/
├── lua/        Vendored Lua 5.4.7 (MIT, see lua/README)
├── src/        ThumbyP8 runtime + bench harness
├── carts/      Test .p8 carts and Phase 0 benchmark scripts
└── build/      Out-of-tree build output (created by cmake)
```

## Build (host, Phase 0)

```bash
cd ThumbyP8
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/p8bench
```

Reports raw Lua interpreter throughput on the host CPU. Later phases
cross-compile the same `p8bench` for RP2350 to get the real number.

## Phases

- **Phase 0** — Lua VM spike + bench harness *(in progress)*
- **Phase 1** — P8 memory map + 4bpp framebuffer + RGB565 expand
- **Phase 2** — Sprites, map, input
- **Phase 3** — Text, math, utility API; Celeste Classic title screen
- **Phase 4** — Audio synth (basic waveforms)
- **Phase 5** — Synth effects
- **Phase 6** — Cart picker, persistence, .p8.png decoder
- **Phase 7** — Compatibility pass

See `../CLAUDE.md` (project-root) for hardware specs and recovery info.
