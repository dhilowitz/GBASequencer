#!/bin/sh
# Build the headless test runner against mGBA 0.10.5.
#   git clone --depth 1 --branch 0.10.5 https://github.com/mgba-emu/mgba.git ~/SDKs/mgba
#   tools/harness/build.sh            (MGBA=/path/to/mgba to use another checkout)
set -e
MGBA=${MGBA:-$HOME/SDKs/mgba}
HERE=$(cd "$(dirname "$0")" && pwd)
if [ ! -f "$MGBA/build-headless/libmgba.a" ]; then
	cmake -S "$MGBA" -B "$MGBA/build-headless" -DCMAKE_BUILD_TYPE=Release -DBUILD_QT=OFF -DBUILD_SDL=OFF \
		-DBUILD_SHARED=OFF -DBUILD_STATIC=ON -DUSE_FFMPEG=OFF -DUSE_ZLIB=OFF -DUSE_PNG=OFF -DUSE_LIBZIP=OFF \
		-DUSE_MINIZIP=OFF -DUSE_SQLITE3=OFF -DUSE_ELF=OFF -DUSE_LUA=OFF -DUSE_EDITLINE=OFF -DUSE_DEBUGGERS=OFF \
		-DUSE_GDB_STUB=OFF -DM_CORE_GB=OFF -DUSE_EPOXY=OFF -DBUILD_GL=OFF -DBUILD_GLES2=OFF -DBUILD_GLES3=OFF \
		-DUSE_DISCORD_RPC=OFF -DENABLE_SCRIPTING=OFF -DUSE_LZMA=OFF
	cmake --build "$MGBA/build-headless" -j8
fi
cc -O2 -o "$HERE/harness" "$HERE/harness.c" -I"$MGBA/include" -I"$MGBA/build-headless/include" -DM_CORE_GBA \
	"$MGBA/build-headless/libmgba.a" -framework CoreFoundation -lz
echo "built $HERE/harness"
