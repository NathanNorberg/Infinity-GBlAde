#!/usr/bin/env python3
"""Draws the interface art in the style guide's look (docs/Infinity Blade GBA Art Style Guide.md):
gold-framed bars with ornate caps, gem and shield icons, castle markers and the menu frame.

Run from the project folder:  python3 tools/make_ui_art.py [--preview out.png]

Writes graphics/hp_bar.bmp, bar_caps.bmp, shield_pip.bmp, charge_orb.bmp, markers.bmp and
menu_frame.bmp. Sizes and frame orders match what the game expects (see the style guide's
asset list).
"""

import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import art_style  # noqa: E402
from make_placeholder_art import Image  # noqa: E402

GRAPHICS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "graphics")
G = art_style.gba
R = art_style.ramp


def write_json(name, text):
    with open(os.path.join(GRAPHICS_DIR, name + ".json"), "w") as json_file:
        json_file.write(text)


def sprite_json(name, height):
    write_json(name, '{\n    "type": "sprite",\n    "height": %d\n}\n' % height)


# ---------------------------------------------------------------------------------------------
# Bars: 8x8 segments, frames 0..8 hero HP (green) filled 0..8 px, 9..17 enemy (crimson),
# 18..26 super (amber). Gold frame top and bottom, a dark well, a lit top edge on the fill.
# ---------------------------------------------------------------------------------------------

BAR_PALETTE = art_style.palette(
    ("outline", (1,)),                                    # 1 frame shadow
    ("gold", (1, 3, 4)),                                  # 2-4 frame
    G("#181018"),                                         # 5 empty well
    G("#205028"), G("#40a048"), G("#a0f0a0"),             # 6-8 hero HP
    ("crimson", (1, 3)), G("#f8a090"),                    # 9-11 enemy HP
    G("#a06010"), G("#e8a828"), G("#f8f0a0"),             # 12-14 super
)


def make_bars():
    image = Image(8, 8 * 27, BAR_PALETTE)
    fills = ((6, 7, 8), (9, 10, 11), (12, 13, 14))
    for bar, (dark, mid, light) in enumerate(fills):
        for filled in range(9):
            oy = (bar * 9 + filled) * 8
            for x in range(8):
                image.set(x, oy + 0, 4)                   # Frame top (lit)
                image.set(x, oy + 1, 1)
                image.set(x, oy + 6, 1)
                image.set(x, oy + 7, 2)                   # Frame bottom (shadow)
                for y in range(2, 6):
                    if x < filled:
                        image.set(x, oy + y, light if y == 2 else mid if y < 5 else dark)
                    else:
                        image.set(x, oy + y, 5)
    return image


def make_bar_caps():
    """Two 8x8 frames: the left cap (drawn in its right half) and the right cap (left half)."""
    image = Image(8, 16, BAR_PALETTE)
    for frame, side in ((0, -1), (1, 1)):
        oy = frame * 8
        for y in range(8):
            for x in range(8):
                local = (7 - x) if side < 0 else x                # 0 at the bar, growing outward
                if local > 3:
                    continue
                d = abs(y - 3.5)
                if d > 3.5 - local * 0.3:
                    continue
                value = 4 if y < 3 else 3 if y < 5 else 2
                image.set(x, oy + y, value)
        # A small crimson jewel at the tip of each cap.
        jx = 5 if side < 0 else 2
        image.set(jx, oy + 3, 10)
        image.set(jx, oy + 4, 9)
    return image


# ---------------------------------------------------------------------------------------------
# Shield pips (steel kite shields with gold rims) and charge orbs (blue gems in gold).
# ---------------------------------------------------------------------------------------------

ICON_PALETTE = art_style.palette(
    ("outline", (1,)),                                    # 1
    ("gold", (2, 4)),                                     # 2-3 rim
    ("steel", (1, 2, 3, 4)),                              # 4-7 shield face
    G("#282830"), G("#383848"),                           # 8-9 spent
    ("ice", (0, 2, 3, 4)),                                # 10-13 gem
)


def make_shield_pips():
    image = Image(8, 16, ICON_PALETTE)
    for frame in range(2):
        oy = frame * 8
        full = frame == 0
        shape = ["01111110", "12222221", "12455421", "12456421", "01255210", "00125100", "00012000", "00000000"]
        for y, row in enumerate(shape):
            for x, cell in enumerate(row):
                if cell == "0":
                    continue
                if cell == "1":
                    color = 1
                elif cell == "2":
                    color = 3 if full and y < 2 else 2 if full else 9
                else:
                    color = (4 + int(cell) - 4) if full else 8
                    if full and cell == "6":
                        color = 7
                image.set(x, oy + y, color)
    return image


def make_charge_orbs():
    image = Image(8, 16, ICON_PALETTE)
    for frame in range(2):
        oy = frame * 8
        full = frame == 0
        for y in range(8):
            for x in range(8):
                d = math.hypot(x - 3.5, y - 3.5)
                if d > 3.6:
                    continue
                if d > 2.6:
                    color = 3 if y < 4 else 2                     # Gold setting
                elif full:
                    lit = (3.5 - x) + (3.5 - y)
                    color = 13 if lit > 2.5 else 12 if lit > 0.5 else 11 if lit > -1.5 else 10
                else:
                    color = 8
                image.set(x, oy + y, color)
        image.outline(1, (0, oy, 8, oy + 8))
    return image


# ---------------------------------------------------------------------------------------------
# Castle markers: 16x16, 11 frames (duel, duel won, gold, gold taken, chest, chest open, locked,
# exit, exit blocked, God King, elite), in gold, steel, wood and crimson.
# ---------------------------------------------------------------------------------------------

MARKER_PALETTE = art_style.palette(
    ("outline", (1,)),                                    # 1
    ("gold", (1, 3, 4)),                                  # 2-4
    ("steel", (2, 3, 4)),                                 # 5-7
    ("leather", (1, 2, 3)),                               # 8-10
    G("#3a3a46"), G("#5a5a68"),                           # 11-12 spent / blocked
    ("crimson", (2,)),                                    # 13
    G("#48c070"),                                         # 14 open exit glow
)


def make_markers():
    image = Image(16, 16 * 11, MARKER_PALETTE)

    def cell(frame):
        return frame * 16

    def line(oy, x0, y0, x1, y1, color, width=2):
        image.line(x0, oy + y0, x1, oy + y1, width, color)

    def swords(oy, dim):
        blade, edge, hilt = (11, 12, 11) if dim else (6, 7, 3)
        line(oy, 3, 3, 12, 12, blade)
        line(oy, 12, 3, 3, 12, blade)
        image.set(4, oy + 4, edge)
        image.set(11, oy + 4, edge)
        for x0, y0 in ((2, 11), (10, 11)):
            image.rect(x0, oy + y0, x0 + 4, oy + y0 + 2, hilt)

    def pouch(oy, dim):
        body, light, tie = (11, 12, 11) if dim else (9, 10, 3)
        image.rect(4, oy + 7, 12, oy + 14, body)
        image.rect(5, oy + 6, 11, oy + 7, body)
        image.rect(5, oy + 8, 7, oy + 11, light)
        image.rect(6, oy + 4, 10, oy + 6, tie)
        if not dim:
            image.set(7, oy + 10, 4)                              # Coin glint

    def chest(oy, opened, locked):
        image.rect(2, oy + 8, 14, oy + 14, 9)
        image.rect(2, oy + 8, 14, oy + 9, 3)                      # Gold band
        if opened:
            image.rect(2, oy + 3, 14, oy + 6, 8)
            image.rect(4, oy + 6, 12, oy + 8, 4)                  # Treasure glow
        else:
            image.rect(2, oy + 5, 14, oy + 8, 10)
            image.rect(2, oy + 5, 14, oy + 6, 3)
        if locked:
            image.rect(6, oy + 7, 10, oy + 12, 6)
            image.set(7, oy + 9, 1)
            image.set(8, oy + 9, 1)

    def door(oy, open_):
        frame_color, inner = (3, 14) if open_ else (12, 11)
        image.rect(4, oy + 3, 12, oy + 14, frame_color)
        image.rect(5, oy + 2, 11, oy + 3, frame_color)
        image.rect(5, oy + 4, 11, oy + 14, inner if open_ else 1)
        if open_:
            image.rect(7, oy + 6, 9, oy + 12, 4)                  # Light through the door

    swords(cell(0), False)
    swords(cell(1), True)
    pouch(cell(2), False)
    pouch(cell(3), True)
    chest(cell(4), False, False)
    chest(cell(5), True, False)
    chest(cell(6), False, True)
    door(cell(7), True)
    door(cell(8), False)
    oy = cell(9)                                                  # Crown
    image.rect(2, oy + 7, 14, oy + 13, 3)
    image.rect(2, oy + 12, 14, oy + 13, 2)
    for x in (2, 7, 12):
        image.rect(x, oy + 3, x + 2, oy + 7, 3)
        image.set(x, oy + 3, 4)
    image.rect(7, oy + 9, 9, oy + 11, 13)
    oy = cell(10)                                                 # Elite: a horned helm
    image.rect(4, oy + 5, 12, oy + 13, 6)
    image.rect(4, oy + 5, 6, oy + 13, 7)
    image.rect(5, oy + 8, 11, oy + 9, 1)
    line(oy, 4, 6, 1, 2, 3, 2)
    line(oy, 11, 6, 14, 2, 3, 2)

    for frame in range(11):
        image.outline(1, (0, cell(frame), 16, cell(frame) + 16))
    return image


# ---------------------------------------------------------------------------------------------
# Menu frame: a 256x256 background, transparent inside, with an ornate border around the
# visible screen (columns 8-247, rows 48-207): dark stone, a gold inner molding, corner jewels.
# ---------------------------------------------------------------------------------------------

FRAME_PALETTE = art_style.palette(
    ("outline", (1,)),                                    # 1
    ("stone", (1, 2, 3)),                                 # 2-4
    ("gold", (1, 2, 3, 4)),                               # 5-8
    ("crimson", (2, 3)),                                  # 9-10 corner jewels
)


def make_menu_frame():
    """Thin enough (4 px, jewels 7 px) to clear the menus' text, which runs to 8 px from the edge."""
    image = Image(256, 256, FRAME_PALETTE)
    left, right, top, bottom = 8, 247, 48, 207
    for y in range(top, bottom + 1):
        for x in range(left, right + 1):
            inset = min(x - left, right - x, y - top, bottom - y)
            lit = min(x - left, y - top) <= min(right - x, bottom - y)   # Top and left catch the light
            if inset == 0:
                image.set(x, y, 2)
            elif inset == 1:
                image.set(x, y, 7 if lit else 6)
            elif inset == 2:
                image.set(x, y, 5)
            elif inset == 3:
                image.set(x, y, 1)
    for cx, cy in ((left + 3, top + 3), (right - 3, top + 3), (left + 3, bottom - 3), (right - 3, bottom - 3)):
        for y in range(cy - 3, cy + 4):
            for x in range(cx - 3, cx + 4):
                d = abs(x - cx) + abs(y - cy)
                if d == 3:
                    image.set(x, y, 8)
                elif d < 3:
                    image.set(x, y, 10 if (x < cx or y < cy) and d > 0 else 9)
    return image


def make_area_band():
    """The area screen's title band: a dark stone strip across the top 26 rows of the screen,
    behind the area name and status line, edged in gold so it reads as part of the HUD."""
    image = Image(256, 256, FRAME_PALETTE)
    top, bottom = 48, 48 + 26
    for y in range(top, bottom):
        for x in range(256):
            image.set(x, y, 1 if (y - top) < 20 or (x + y) % 2 else 2)      # Fades out at the bottom edge
    for x in range(256):
        image.set(x, bottom, 6)
        image.set(x, bottom + 1, 5)
    return image


# ---------------------------------------------------------------------------------------------
# Title logo: "INFINITY" over a larger "GBLADE" in beveled gold, on its own background so the
# title screen can dim the castle without dimming the name. Screen rows 6-54 (BG rows 54-102).
# ---------------------------------------------------------------------------------------------

LOGO_PALETTE = art_style.palette(
    ("outline", (0, 1)),                                  # 1-2 outline and shadow
    ("gold", (0, 1, 2, 3, 4)),                            # 3-7 letters, dark to light
    ("steel", (2, 4)),                                    # 8-9 rule highlights
)

LOGO_GLYPHS = {
    "I": ["#####", "..#..", "..#..", "..#..", "..#..", "..#..", "#####"],
    "N": ["#...#", "##..#", "#.#.#", "#.#.#", "#..##", "#...#", "#...#"],
    "F": ["#####", "#....", "#....", "####.", "#....", "#....", "#...."],
    "T": ["#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."],
    "Y": ["#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."],
    "G": [".####", "#....", "#....", "#.###", "#...#", "#...#", ".###."],
    "B": ["####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."],
    "L": ["#....", "#....", "#....", "#....", "#....", "#....", "#####"],
    "A": [".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"],
    "D": ["####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."],
    "E": ["#####", "#....", "#....", "####.", "#....", "#....", "#####"],
}


def draw_logo_word(image, word, top, scale, spacing):
    width = len(word) * (5 * scale + spacing) - spacing
    left = 128 - width // 2
    height = 7 * scale
    for index, letter in enumerate(word):
        glyph = LOGO_GLYPHS[letter]
        x0 = left + index * (5 * scale + spacing)
        for gy, row in enumerate(glyph):
            for gx, cell in enumerate(row):
                if cell != "#":
                    continue
                for dy in range(scale):
                    for dx in range(scale):
                        y = top + gy * scale + dy
                        t = (y - top) / height                       # Gold runs light at the top
                        color = 7 if t < 0.15 else 6 if t < 0.45 else 5 if t < 0.75 else 4
                        if dy == scale - 1 and gy < 6 and glyph[gy + 1][gx] != "#":
                            color = 3                                # Shaded underside of each stroke
                        image.set(x0 + gx * scale + dx, y, color)
    return left, left + width


def make_title_logo():
    image = Image(256, 256, LOGO_PALETTE)
    small_left, small_right = draw_logo_word(image, "INFINITY", 56, 2, 2)
    draw_logo_word(image, "GBLADE", 76, 3, 3)

    # Drop shadow down-right, then a 1 px outline around everything.
    letters = [(x, y) for y in range(50, 106) for x in range(256) if image.get(x, y) >= 3]
    for x, y in letters:
        if image.get(x + 1, y + 1) == 0:
            image.set(x + 1, y + 1, 2)
    image.outline(1, (0, 48, 256, 110))

    # Gold rules either side of "INFINITY", ending in a small diamond.
    for x0, x1 in ((small_left - 32, small_left - 6), (small_right + 6, small_right + 32)):
        for x in range(x0, x1):
            image.set(x, 62, 6)
            image.set(x, 63, 4)
        for tip in (x0, x1):
            for d in range(-2, 3):
                image.set(tip + d, 62 + (2 - abs(d)) - 2, 7)
                image.set(tip + d, 63 - (2 - abs(d)) + 2, 4)
    return image


def main():
    outputs = [
        ("hp_bar", make_bars(), 8),
        ("bar_caps", make_bar_caps(), 8),
        ("shield_pip", make_shield_pips(), 8),
        ("charge_orb", make_charge_orbs(), 8),
        ("markers", make_markers(), 16),
    ]
    for name, image, height in outputs:
        image.save_bmp(os.path.join(GRAPHICS_DIR, name + ".bmp"))
        sprite_json(name, height)

    make_menu_frame().save_bmp(os.path.join(GRAPHICS_DIR, "menu_frame.bmp"))
    write_json("menu_frame", '{\n    "type": "regular_bg",\n    "bpp_mode": "bpp_4"\n}\n')
    make_title_logo().save_bmp(os.path.join(GRAPHICS_DIR, "title_logo.bmp"))
    write_json("title_logo", '{\n    "type": "regular_bg",\n    "bpp_mode": "bpp_4"\n}\n')
    make_area_band().save_bmp(os.path.join(GRAPHICS_DIR, "area_band.bmp"))
    write_json("area_band", '{\n    "type": "regular_bg",\n    "bpp_mode": "bpp_4"\n}\n')
    print("Wrote the HUD, markers, menu frame, area band and title logo to graphics/")

    if "--preview" in sys.argv:
        import subprocess
        output = sys.argv[sys.argv.index("--preview") + 1]
        tools = os.path.dirname(os.path.abspath(__file__))
        subprocess.run([sys.executable, os.path.join(tools, "preview_art.py"), "bar_caps", "shield_pip", "charge_orb",
                        "markers", "-o", output, "--scale", "4"], check=True)


if __name__ == "__main__":
    main()
