#!/usr/bin/env python3
"""Converts a PNG (or any image Pillow reads) into a game-ready 16-color BMP plus its Butano
JSON in graphics/, so hand-drawn sketches or licensed asset packs can drop into the game.

Examples (run from the project folder):
  python3 tools/png_to_gba.py sketch.png hero --type sprite --frame-height 64
  python3 tools/png_to_gba.py hall.png area_great_hall --type bg
  python3 tools/png_to_gba.py knight.png enemy_idle --type bg --palette-from graphics/enemy_idle.bmp

Rules it enforces (see docs/Infinity Blade GBA Art Style Guide.md):
  - 16 colors at most: transparent index 0 plus 15 colors, snapped to the GBA's 15-bit color.
  - Transparency comes from alpha below 128, or from pure magenta (#ff00ff).
  - Sprites: frames stacked vertically; width and frame height must be 8, 16, 32 or 64.
  - Backgrounds: 256x256 (the size every background in the game uses).
  - --palette-from maps colors onto an existing image's palette instead of choosing new ones,
    which keeps palette swaps (Brute, Duelist, God King) working for enemy poses.
"""

import argparse
import os
import sys

from PIL import Image as PilImage

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from make_placeholder_art import Image  # noqa: E402  (shared 4bpp BMP writer)

GRAPHICS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "graphics")
SPRITE_SIZES = (8, 16, 32, 64)


def snap(color):
    return tuple((channel // 8) * 8 for channel in color[:3])


def read_bmp_palette(path):
    with PilImage.open(path) as image:
        flat = image.getpalette()[:48]
    return [tuple(flat[i:i + 3]) for i in range(0, len(flat), 3)]


def nearest(color, palette):
    # Compare in snapped (15-bit) space; on a tie the lower index wins, so unused filler
    # entries at the end of a palette never steal pixels.
    best, best_distance = 1, None
    for index in range(1, len(palette)):
        candidate = snap(palette[index])
        distance = sum((a - b) ** 2 for a, b in zip(color, candidate))
        if best_distance is None or distance < best_distance:
            best, best_distance = index, distance
    return best


def convert(source, name, kind, frame_height, palette_from, out_dir=GRAPHICS_DIR):
    image = PilImage.open(source).convert("RGBA")
    width, height = image.size

    if kind == "sprite":
        if width not in SPRITE_SIZES or frame_height not in SPRITE_SIZES or height % frame_height:
            raise SystemExit("Sprite sheets need width and frame height of 8/16/32/64, and whole frames "
                             "(got %dx%d, frame height %d)" % (width, height, frame_height))
    elif (width, height) != (256, 256):
        raise SystemExit("Backgrounds must be 256x256 (got %dx%d)" % (width, height))

    pixels = image.load()
    opaque = [[pixels[x, y][3] >= 128 and pixels[x, y][:3] != (255, 0, 255) for x in range(width)]
              for y in range(height)]

    if palette_from:
        palette = read_bmp_palette(palette_from)
    else:
        # Median-cut the opaque pixels down to 15 colors.
        rgb = PilImage.new("RGB", (width, height))
        rgb_pixels = rgb.load()
        for y in range(height):
            for x in range(width):
                rgb_pixels[x, y] = snap(pixels[x, y]) if opaque[y][x] else (0, 0, 0)
        opaque_only = [snap(pixels[x, y]) for y in range(height) for x in range(width) if opaque[y][x]]
        sample = PilImage.new("RGB", (len(opaque_only), 1))
        sample.putdata(opaque_only)
        quantized = sample.quantize(colors=15, method=PilImage.Quantize.MEDIANCUT)
        flat = quantized.getpalette()[:45]
        palette = [(255, 0, 255)] + [snap(tuple(flat[i:i + 3])) for i in range(0, len(flat), 3)]

    out = Image(width, height, palette[:16])
    for y in range(height):
        for x in range(width):
            if opaque[y][x]:
                out.set(x, y, nearest(snap(pixels[x, y]), out.palette))

    out.save_bmp(os.path.join(out_dir, name + ".bmp"))

    with open(os.path.join(out_dir, name + ".json"), "w") as json_file:
        if kind == "sprite":
            json_file.write('{\n    "type": "sprite",\n    "height": %d\n}\n' % frame_height)
        else:
            json_file.write('{\n    "type": "regular_bg",\n    "bpp_mode": "bpp_4"\n}\n')

    used = len({out.pixels[y][x] for y in range(height) for x in range(width)} - {0})
    print("Wrote %s.bmp (%dx%d, %d colors used)" % (os.path.join(out_dir, name), width, height, used))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source")
    parser.add_argument("name", help="Output name in graphics/, e.g. hero or area_great_hall")
    parser.add_argument("--type", choices=("sprite", "bg"), required=True)
    parser.add_argument("--frame-height", type=int, default=0, help="Sprites: height of one frame")
    parser.add_argument("--palette-from", help="Map onto this existing BMP's palette")
    parser.add_argument("--out-dir", default=GRAPHICS_DIR, help="Where to write (default graphics/)")
    args = parser.parse_args()

    frame_height = args.frame_height or (PilImage.open(args.source).size[1] if args.type == "sprite" else 0)
    convert(args.source, args.name, args.type, frame_height, args.palette_from, args.out_dir)


if __name__ == "__main__":
    main()
