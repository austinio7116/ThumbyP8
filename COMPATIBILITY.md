# ThumbyP8 Cart Compatibility

Last updated: 2026-04-12

## Status Key
- **Playable** — loads, controls work, plays to completion or reasonable extent
- **Partial** — loads but has issues (visual glitches, some features broken, eventual crash)
- **Broken** — crashes, hangs, or unplayable
- **Impossible** — exceeds hardware memory limits

## Test Results

| Cart | Peak Heap | Status | Notes |
|------|-----------|--------|-------|
| 49232 | 168KB | Playable | Working, numbers may show as floats |
| adelie-0 | 230KB | Untested | _init OK on host |
| age_of_ants-9 | 180KB | Broken | Top-level error: attempt to index a string value |
| beam4-2 | 199KB | Untested | _init OK on host, was hanging on device |
| celeste | 146KB | Playable | Fully playable |
| cheezymaze_ex-0 | ? | Broken | _init loops/hangs (timeout on host test) |
| delunky102-0 | 117KB | Playable | Working, random levels confirmed |
| digger-2 | 115KB | Playable | Working |
| dominion_ex-4 | 112KB | Partial | Plays but eventually crashes/hangs |
| flipknight-0 | 127KB | Broken | _init error (not OOM — code issue) |
| fromrust_a-4 | 211KB | Partial | Loads, crashes during gameplay: nil concat |
| fsgupicozombiegarden121-0 | 193KB | Partial | Loads but needs mouse input (not supported) |
| hotwax-5 | 161KB | Broken | _init error (not OOM — code issue) |
| kalikan_menu-6 | 116KB | Untested | _init OK on host |
| lootslime-1 | 546KB | Impossible | Needs 546KB heap — exceeds 520KB total SRAM |
| lorez-1 | 570KB | Impossible | Needs 570KB heap — exceeds 520KB total SRAM |
| mini_pharma-1 | 140KB | Untested | _init OK on host |
| mossmoss-12 | 265KB | Broken | _init error (not OOM — code issue) |
| mot_pool-23 | 350KB | Broken | Needs 350KB heap — exceeds safe 300KB cap |
| musabanebi-0 | 95KB | Partial | Runtime: nil index field '?' (P8SCII glyph key) |
| pck404_512px_under-1 | 172KB | Partial | Playable but P8SCII control chars in strings |
| phoenix08-0 | 168KB | Untested | _init OK on host |
| pico_arcade-2 | 65KB | Untested | _init OK on host |
| pico_ball-5 | 145KB | Broken | Top-level error: attempt to get length of number |
| picovalley-2 | 128KB | Untested | _init OK on host |
| poom_0-9 | 102KB | Untested | Bootstrap cart, _init OK on host |
| praxis_fighter_x-2 | 172KB | Broken | Top-level error: nil concat (integer index) |
| province-4 | 155KB | Untested | _init OK on host |
| rtype-5 | 136KB | Partial | Runtime: nil arith field '?' (P8SCII glyph key) |
| ruwukawisa-0 | 105KB | Untested | _init OK on host |
| start_picocraft_1-3 | 124KB | Untested | _init OK on host |
| terra_1cart-43 | 144KB | Broken | _init error: nil concat field '?' |
| tinygolfpuzzles-1 | 118KB | Playable | Working |
| woodworm-0 | 123KB | Untested | _init OK on host, was crashing on old firmware |

## Memory Summary

- **Lua heap cap**: 300KB (hard limit enforced by capped allocator)
- **Safe maximum**: ~300KB (menu needs 32KB malloc headroom when open)
- **31 of 34 carts** fit within 300KB
- **1 cart** (mot_pool) needs 350KB — too tight for the menu to work
- **2 carts** (lootslime, lorez) need 500KB+ — physically impossible on 520KB SRAM

## Known Code Issues (not memory)

These carts fail with runtime errors despite fitting in memory:

- **age_of_ants-9**: top-level error indexing a string — likely translator issue
- **cheezymaze_ex-0**: _init loops forever — game logic issue or missing API
- **flipknight-0**: _init runtime error — needs investigation
- **hotwax-5**: _init runtime error — needs investigation
- **mossmoss-12**: _init runtime error — needs investigation
- **pico_ball-5**: top-level error getting length of number — translator issue
- **praxis_fighter_x-2**: top-level nil concat — likely P8SCII glyph key issue
- **terra_1cart-43**: _init nil concat — likely P8SCII glyph key issue

## Known Limitations
- Carts using mouse input (fsgupicozombiegarden) won't work — no mouse on Thumby Color
- P8SCII special characters in strings display as blocks, not glyphs
- `fillp`, `tline`, `reload` are stubs
- `cstore`/`dget`/`dset` are stubs — no persistent save data
- No multi-cart support (`load()` won't load other carts)

## Recent Fixes (2026-04-12)
- Button/arrow glyphs now correctly map to button indices 0-5
- All bitwise ops use 16.16 fixed-point via function calls
- Fixed-point results return as integer when fractional part is zero
- PXA decompressor: removed 32-iteration limit on back-ref length
- RNG seeded from hardware timer at cart launch
- In-game pause menu with volume, FPS toggle, disk/battery info
- Lobby skipped when carts exist — boots straight to picker
