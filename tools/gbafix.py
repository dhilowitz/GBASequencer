#!/usr/bin/env python3
"""Fix up a GBA ROM header: header checksum, and the Nintendo logo if a source is given.

    gbafix.py rom.gba [--logo-from other.gba]

The logo is Nintendo's, so it isn't in this repo. Emulators don't check it. Real hardware does:
pass --logo-from with any ROM you own (or run devkitPro's gbafix) before flashing to a cartridge.
"""
import sys

path = sys.argv[1]
rom = bytearray(open(path, 'rb').read())
if '--logo-from' in sys.argv:
    src = open(sys.argv[sys.argv.index('--logo-from') + 1], 'rb').read()
    rom[0x04:0xA0] = src[0x04:0xA0]
# pad to a multiple of 4 bytes (some flash carts want it)
while len(rom) % 4:
    rom.append(0)
rom[0xBD] = (-(sum(rom[0xA0:0xBD]) + 0x19)) & 0xFF
open(path, 'wb').write(rom)
