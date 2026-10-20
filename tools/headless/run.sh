#!/bin/sh
# Plays a script against the ROM headlessly and converts the screenshots to PNG.
#   tools/headless/run.sh <script> [output folder]      (default output: tools/headless/shots)
# Script format: see run.c.
set -e
here="$(cd "$(dirname "$0")" && pwd)"
out="${2:-$here/shots}"
mkdir -p "$out"
DYLD_FALLBACK_LIBRARY_PATH="$here/lib" "$here/run" "$here/../../infinity_gblade.gba" "$1" "$out" | grep -v "^GBA" || true
python3 - "$out" <<'PY'
import glob, os, sys
from PIL import Image
for path in glob.glob(os.path.join(sys.argv[1], "*.ppm")):
    Image.open(path).save(path[:-4] + ".png")
    os.remove(path)
PY
