#!/bin/sh
# Builds the headless test runner against Homebrew's mGBA (brew install mgba).
#
# Homebrew's libmgba links ffmpeg, whose x265 dependency can fall out of step after an upgrade
# (dyld: "libx265.NNN.dylib not loaded"). The runner never records video, so a stand-in
# library with the one missing symbol is built into tools/headless/lib for run.sh to use.
set -e
cd "$(dirname "$0")"
cc -O2 -o run run.c -I/opt/homebrew/include -L/opt/homebrew/lib -lmgba

missing=$(otool -L /opt/homebrew/opt/ffmpeg/lib/libavcodec.dylib 2>/dev/null | grep -o 'libx265\.[0-9]*\.dylib' | head -1)
if [ -n "$missing" ] && [ ! -e "/opt/homebrew/opt/x265/lib/$missing" ]; then
    version=$(echo "$missing" | sed 's/libx265\.\([0-9]*\)\.dylib/\1/')
    mkdir -p lib
    echo "void* x265_api_get_$version(int depth) { (void)depth; return 0; }" > lib/stub.c
    cc -dynamiclib -o "lib/$missing" -install_name "/opt/homebrew/opt/x265/lib/$missing" lib/stub.c
    echo "Built a stand-in $missing in tools/headless/lib"
fi
