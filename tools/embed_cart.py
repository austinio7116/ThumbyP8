#!/usr/bin/env python3
"""
embed_cart.py — bake a .p8 text cart into a C header.

Usage: embed_cart.py input.p8 output.h symbol_name

Emits a header with `static const char SYMBOL[]` containing the cart
bytes plus a length constant. Used by the device build to ship a
cart inside flash without needing a filesystem.
"""
import sys
from pathlib import Path

def main():
    if len(sys.argv) != 4:
        print("usage: embed_cart.py input.p8 output.h symbol", file=sys.stderr)
        sys.exit(1)
    src  = Path(sys.argv[1])
    dst  = Path(sys.argv[2])
    sym  = sys.argv[3]

    data = src.read_bytes()
    out = []
    out.append(f"/* Auto-generated from {src.name} — do not edit by hand. */")
    out.append(f"#ifndef THUMBYP8_EMBED_{sym.upper()}_H")
    out.append(f"#define THUMBYP8_EMBED_{sym.upper()}_H")
    out.append("")
    out.append(f"#include <stddef.h>")
    out.append("")
    out.append(f"static const unsigned int {sym}_len = {len(data)};")
    out.append(f"static const unsigned char {sym}[] = {{")
    line = []
    for i, b in enumerate(data):
        line.append(f"0x{b:02x}")
        if len(line) == 16:
            out.append("    " + ",".join(line) + ",")
            line = []
    if line:
        out.append("    " + ",".join(line) + ",")
    out.append("};")
    out.append("")
    out.append("#endif")
    dst.write_text("\n".join(out) + "\n")
    print(f"wrote {dst} ({len(data)} bytes)", file=sys.stderr)

if __name__ == "__main__":
    main()
