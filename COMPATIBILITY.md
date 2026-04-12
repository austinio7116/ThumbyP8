# ThumbyP8 Cart Compatibility

Last updated: 2026-04-12

## Status Key
- **Playable** — loads, controls work, plays to completion or reasonable extent
- **Partial** — loads but has issues (visual glitches, some features broken, eventual crash)
- **Broken** — crashes, hangs, or unplayable
- **Impossible** — exceeds hardware memory limits

## Test Results

| Cart | Status | Notes |
|------|--------|-------|
| 24981 (Mcraft) | Impossible | OOM on device |
| 49232 | Playable | Working |
| adelie-0 | Broken | OOM in _init on host; untested on device |
| age_of_ants-9 | Broken | OOM in _update, then cascading nil errors |
| air_delivery_1-3 | Broken | Loads but sprites are corrupted |
| beam4-2 | Playable | Working |
| celeste | Playable | Fully playable |
| celeste_classic_2-5 | Broken | Loads but only shows clouds, no gameplay |
| combopool-0 | Playable | Working |
| delunky102-0 | Playable | Working (same seed every time) |
| digger-2 | Playable | Working |
| dominion_ex-4 | Playable | Working |
| fafomajoje-0 (Dungeon) | Playable | Working |
| flipknight-0 | Playable | Working |
| fromrust_a-4 | Partial | Loads, crashes during gameplay |
| fsgupicozombiegarden121-0 | Broken | Needs mouse input (not supported) |
| highstakes-2 | Playable | Working |
| hotwax-5 | Broken | _init runtime error |
| kalikan_menu-6 | Untested | _init OK on host |
| mini_pharma-1 | Untested | _init OK on host |
| mossmoss-12 | Broken | OOM in _init |
| musabanebi-0 | Partial | Runtime nil index on P8SCII glyph key |
| pck404_512px_under-1 | Partial | Playable but P8SCII chars display as blocks |
| phoenix08-0 | Untested | _init OK on host |
| pico_arcade-2 | Untested | _init OK on host |
| pico_ball-5 | Broken | Audio plays but screen stays black |
| picohot-0 | Broken | _update60 error: attempt to get length of number |
| picovalley-2 | Untested | _init OK on host |
| poom_0-9 | Broken | Colourmap wrong, hangs at level select |
| porklike-2 | Playable | Working (fixed by P8SCII identifier support) |
| praxis_fighter_x-2 | Broken | OOM during translation on host; untested on device |
| province-4 | Untested | _init OK on host |
| rtype-5 | Untested | _init OK on host |
| ruwukawisa-0 | Untested | _init OK on host |
| slipways-1 | Broken | OOM during translation on host; hangs on load on device |
| start_picocraft_1-3 | Untested | _init OK on host |
| terra_1cart-43 | Broken | Hangs during translation on host |
| tinygolfpuzzles-1 | Playable | Working |
| woodworm-0 | Untested | _init OK on host |

## Summary

- **Playable**: 14 carts
- **Partial**: 3 carts
- **Broken**: 12 carts
- **Impossible**: 1 cart
- **Untested on device**: 9 carts

## Known Limitations
- Carts using mouse input (fsgupicozombiegarden) won't work — no mouse on Thumby Color
- P8SCII special characters in strings display as blocks, not glyphs
- `fillp`, `tline`, `reload` are stubs — affects visual fidelity in many carts
- `cartdata`/`dget`/`dset` are stubs — no persistent save data
- No multi-cart support (`load()` won't load other carts)

## Recent Fixes (2026-04-12)
- Audio: fixed severe distortion from rogue *4 gain multiplier in master volume
- P8SCII bytes (>=0x80) now work as Lua identifier characters (fixes porklike, others)
- Host pipeline unified with device (shrinko8 + full PICO-8 dialect translation)
- API: add(tbl,val,index), count(tbl,val), multi-byte peek/poke
- Button/arrow glyphs correctly map to button indices 0-5
- All bitwise ops use 16.16 fixed-point via function calls
- PXA decompressor: removed 32-iteration limit on back-ref length
- RNG seeded from hardware timer at cart launch
- In-game pause menu with volume, FPS toggle, disk/battery info
- Lobby skipped when carts exist — boots straight to picker
