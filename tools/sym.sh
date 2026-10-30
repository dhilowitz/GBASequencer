#!/bin/sh
# Print the address of a symbol from the last build's map file: tools/sym.sh vblank_count
awk -v s="$1" '$2 == s && $1 ~ /^0x/ { sub(/^0x0+/, "0x", $1); print $1; exit }' "$(dirname "$0")/../build/gbasequencer.map"
