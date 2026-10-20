#!/usr/bin/env python3
"""Renders preview sheets of the game's art for review before it goes in the game.

Examples (run from the project folder):
  python3 tools/preview_art.py hero enemy_idle -o preview.png         Frames of these images
  python3 tools/preview_art.py --duel area_great_hall -o duel.png    A mock duel screen
  python3 tools/preview_art.py --ramps -o ramps.png                  The style guide's color ramps

Sprites show every frame side by side on a mid-grey checker (so transparency is visible);
backgrounds show the part the 240x160 screen sees. Everything is scaled up (default x3).
"""

import argparse
import os
import sys

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import art_style  # noqa: E402

GRAPHICS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "graphics")
SCREEN = (240, 160)
BACKDROP = ((70, 74, 86), (82, 86, 98))


def load(name):
    """(RGBA image with index 0 transparent, frame height) for graphics/<name>.bmp, or for a
    path to a .bmp anywhere (for previewing art before it goes into graphics/)."""
    path = name if name.endswith(".bmp") else os.path.join(GRAPHICS_DIR, name + ".bmp")
    indexed = Image.open(path)
    rgba = indexed.convert("RGBA")
    pixels, indices = rgba.load(), indexed.load()
    for y in range(indexed.size[1]):
        for x in range(indexed.size[0]):
            if indices[x, y] == 0:
                pixels[x, y] = (0, 0, 0, 0)
    frame_height = indexed.size[1]
    json_path = path[:-4] + ".json"
    if os.path.exists(json_path):
        text = open(json_path).read()
        if '"sprite"' in text and '"height"' in text:
            frame_height = int(text.split('"height"')[1].split(":")[1].split("}")[0].strip())
        elif "regular_bg" in text:
            frame_height = 0
    return rgba, frame_height


def checker(size):
    image = Image.new("RGB", size, BACKDROP[0])
    draw = ImageDraw.Draw(image)
    for y in range(0, size[1], 8):
        for x in range(0, size[0], 8):
            if (x // 8 + y // 8) % 2:
                draw.rectangle((x, y, x + 7, y + 7), fill=BACKDROP[1])
    return image


def frames_sheet(names):
    tiles = []
    for name in names:
        image, frame_height = load(name)
        if frame_height == 0:
            # Background: the screen's view of it (the BG sits at 0,0, screen shows 8..247 x 48..207).
            tiles.append(image.crop((8, 48, 8 + SCREEN[0], 48 + SCREEN[1])))
            continue
        for top in range(0, image.size[1], frame_height):
            tiles.append(image.crop((0, top, image.size[0], top + frame_height)))
    width = sum(tile.size[0] for tile in tiles) + 4 * (len(tiles) + 1)
    height = max(tile.size[1] for tile in tiles) + 8
    sheet = checker((width, height))
    x = 4
    for tile in tiles:
        sheet.paste(tile, (x, 4), tile)
        x += tile.size[0] + 4
    return sheet


def duel_mock(background, enemy="enemy_idle"):
    """The duel screen as the game composes it: background, enemy layer, hero sprite."""
    screen = Image.new("RGB", SCREEN)
    for name, offset in ((background, (8, 48)), (enemy, (8, 48))):
        image, _ = load(name)
        crop = image.crop((offset[0], offset[1], offset[0] + SCREEN[0], offset[1] + SCREEN[1]))
        screen.paste(crop, (0, 0), crop)
    hero, frame_height = load("hero")
    frame = hero.crop((0, 0, hero.size[0], frame_height))
    screen.paste(frame, (4, 100), frame)                   # Hero at (-84, 52) from the center
    return screen


def ramps_sheet():
    names = list(art_style.RAMPS)
    swatch = 24
    sheet = Image.new("RGB", (110 + swatch * 5, swatch * len(names)), (24, 24, 30))
    draw = ImageDraw.Draw(sheet)
    for row, name in enumerate(names):
        draw.text((4, row * swatch + 7), name, fill=(220, 220, 220))
        for step, color in enumerate(art_style.ramp(name)):
            x = 110 + step * swatch
            draw.rectangle((x, row * swatch, x + swatch - 2, row * swatch + swatch - 2), fill=color)
    return sheet


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("names", nargs="*", help="Images in graphics/ (without .bmp)")
    parser.add_argument("--duel", metavar="BACKGROUND", help="Mock duel screen on this background")
    parser.add_argument("--ramps", action="store_true", help="The style guide's color ramps")
    parser.add_argument("--scale", type=int, default=3)
    parser.add_argument("-o", "--output", required=True)
    args = parser.parse_args()

    if args.ramps:
        sheet = ramps_sheet()
        args.scale = 1
    elif args.duel:
        sheet = duel_mock(args.duel)
    else:
        sheet = frames_sheet(args.names)

    sheet.resize((sheet.size[0] * args.scale, sheet.size[1] * args.scale), Image.NEAREST).save(args.output)
    print("Wrote", args.output)


if __name__ == "__main__":
    main()
