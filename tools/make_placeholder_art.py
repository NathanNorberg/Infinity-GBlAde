#!/usr/bin/env python3
"""Generates the milestone 1 placeholder art as 16-color (4bpp) BMPs in graphics/.

Run from the project folder:  python3 tools/make_placeholder_art.py
Optional:  --preview out.png  writes a 240x160 mock-up of the duel screen (needs Pillow).

Palette index 0 is transparent everywhere. Final art in milestone 5 keeps the same
image sizes so the game code doesn't change.
"""

import os
import struct
import sys

GRAPHICS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "graphics")


class Image:
    def __init__(self, width, height, palette):
        assert len(palette) <= 16
        self.width = width
        self.height = height
        self.palette = list(palette) + [(0, 0, 0)] * (16 - len(palette))
        self.pixels = [[0] * width for _ in range(height)]

    def set(self, x, y, c):
        if 0 <= x < self.width and 0 <= y < self.height:
            self.pixels[y][x] = c

    def get(self, x, y):
        if 0 <= x < self.width and 0 <= y < self.height:
            return self.pixels[y][x]
        return 0

    def rect(self, x0, y0, x1, y1, c):
        """Fills [x0, x1) x [y0, y1)."""
        for y in range(y0, y1):
            for x in range(x0, x1):
                self.set(x, y, c)

    def line(self, x0, y0, x1, y1, thickness, c):
        steps = max(round(abs(x1 - x0)), round(abs(y1 - y0)), 1)
        r = thickness // 2
        for i in range(steps + 1):
            x = round(x0 + (x1 - x0) * i / steps)
            y = round(y0 + (y1 - y0) * i / steps)
            self.rect(x - r, y - r, x - r + thickness, y - r + thickness, c)

    def outline(self, c, region=None):
        """Draws color c on transparent pixels that touch a filled pixel."""
        x0, y0, x1, y1 = region or (0, 0, self.width, self.height)
        marks = []
        for y in range(y0, y1):
            for x in range(x0, x1):
                if self.pixels[y][x] == 0:
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        nx, ny = x + dx, y + dy
                        if x0 <= nx < x1 and y0 <= ny < y1 and self.pixels[ny][nx] not in (0, c):
                            marks.append((x, y))
                            break
        for x, y in marks:
            self.pixels[y][x] = c

    def save_bmp(self, path):
        row_size = ((self.width * 4 + 31) // 32) * 4
        pixel_data = bytearray()
        for y in range(self.height - 1, -1, -1):
            row = bytearray()
            for x in range(0, self.width, 2):
                hi = self.pixels[y][x]
                lo = self.pixels[y][x + 1] if x + 1 < self.width else 0
                row.append((hi << 4) | lo)
            row.extend(b"\0" * (row_size - len(row)))
            pixel_data.extend(row)
        palette_data = bytearray()
        for r, g, b in self.palette:
            palette_data.extend((b, g, r, 0))
        offset = 14 + 40 + len(palette_data)
        with open(path, "wb") as f:
            f.write(struct.pack("<2sIHHI", b"BM", offset + len(pixel_data), 0, 0, offset))
            f.write(struct.pack("<IiiHHIIiiII", 40, self.width, self.height, 1, 4, 0,
                                len(pixel_data), 2835, 2835, 16, 16))
            f.write(palette_data)
            f.write(pixel_data)

    def rgb(self, x, y):
        return self.palette[self.pixels[y][x]]


# ---------------------------------------------------------------------------------------------
# Arena: 256x256 regular BG. With the BG at position (0, 0) the screen shows image
# columns 8..247 and rows 48..207. Wall on top, trim at the horizon, floor below.
# ---------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------
# Castle area screens: 256x256 regular BGs, one per area (gate, garden, barracks, great hall,
# throne approach, throne room). Wall or sky on top, floor below a horizon at image row 120
# (screen y -8), and a few shapes that say which area it is. Hotspot markers sit on top.
# ---------------------------------------------------------------------------------------------

AREA_HORIZON = 120


def area_base(palette, wall_colors, floor_colors, seam):
    """Fills a wall gradient above the horizon and floor bands below, with stone seams."""
    img = Image(256, 256, palette)
    for y in range(AREA_HORIZON):
        img.rect(0, y, 256, y + 1, wall_colors[min(len(wall_colors) - 1, y * len(wall_colors) // AREA_HORIZON)])
    for y in range(AREA_HORIZON, 256):
        band = min(len(floor_colors) - 1, (y - AREA_HORIZON) // 12)
        img.rect(0, y, 256, y + 1, floor_colors[band])
    if seam:
        for y in range(AREA_HORIZON + 11, 256, 12):
            img.rect(0, y, 256, y + 1, seam)
    return img


def make_area_gate():
    palette = [(255, 0, 255), (10, 12, 30), (18, 22, 48), (28, 32, 66), (240, 240, 200),
               (50, 52, 70), (70, 72, 92), (34, 34, 48), (60, 56, 52), (84, 78, 70), (104, 96, 84),
               (40, 36, 34), (14, 14, 20)]
    img = area_base(palette, [1, 2, 3], [8, 9, 10, 10], 11)
    for x, y in ((20, 12), (60, 30), (110, 8), (170, 24), (220, 14), (240, 44), (88, 50), (200, 60)):
        img.rect(x, y, x + 1, y + 1, 4)                                      # Stars
    img.rect(0, 56, 256, AREA_HORIZON, 6)                                    # Castle wall
    for x in range(0, 256, 16):
        img.rect(x, 48, x + 10, 56, 6)                                       # Battlements
    for y in range(64, AREA_HORIZON, 12):
        img.rect(0, y, 256, y + 1, 7)
    img.rect(100, 70, 156, AREA_HORIZON, 12)                                 # Gate arch
    img.rect(108, 64, 148, 70, 12)
    img.rect(116, 60, 140, 64, 12)
    return img


def make_area_garden():
    palette = [(255, 0, 255), (70, 110, 170), (100, 140, 190), (140, 176, 210),
               (24, 70, 30), (36, 96, 40), (50, 120, 54), (70, 140, 60), (90, 156, 70),
               (110, 170, 84), (150, 130, 90), (120, 100, 70), (30, 60, 26)]
    img = area_base(palette, [1, 2, 3], [7, 8, 9, 9], 0)
    for x in range(0, 256, 40):
        img.rect(x, 84, x + 34, AREA_HORIZON, 5)                             # Hedges
        img.rect(x, 84, x + 34, 88, 6)
        img.rect(x + 34, 84, x + 40, AREA_HORIZON, 12)
    for y in range(AREA_HORIZON, 256):                                      # Gravel path, widening
        half = 10 + (y - AREA_HORIZON) // 3
        img.rect(128 - half, y, 128 + half, y + 1, 10 if (y // 6) % 2 else 11)
    return img


def make_area_barracks():
    palette = [(255, 0, 255), (60, 38, 26), (80, 52, 34), (100, 66, 42), (40, 26, 18),
               (110, 86, 60), (90, 70, 50), (70, 56, 40), (52, 42, 30), (150, 150, 156), (96, 60, 36), (30, 20, 14)]
    img = area_base(palette, [3, 2, 1], [5, 6, 7, 8], 0)
    for x in range(0, 256, 24):
        img.rect(x, 0, x + 1, AREA_HORIZON, 4)                               # Wall planks
    for x in (40, 200):
        img.rect(x, 60, x + 4, AREA_HORIZON, 10)                             # Weapon racks
        img.rect(x + 30, 60, x + 34, AREA_HORIZON, 10)
        img.rect(x, 70, x + 34, 72, 10)
        for spear in range(x + 8, x + 30, 7):
            img.rect(spear, 50, spear + 2, AREA_HORIZON - 4, 9)
    return img


def make_area_great_hall():
    palette = [(255, 0, 255), (40, 40, 56), (54, 54, 74), (66, 66, 90), (90, 90, 116),
               (150, 30, 40), (110, 20, 30), (200, 160, 60), (70, 64, 80), (90, 84, 100), (110, 104, 120),
               (50, 46, 60), (24, 24, 34)]
    img = area_base(palette, [1, 2, 3], [8, 9, 10, 10], 11)
    for x in range(16, 256, 56):
        img.rect(x, 0, x + 18, AREA_HORIZON, 4)                              # Pillars
        img.rect(x + 14, 0, x + 18, AREA_HORIZON, 12)
        img.rect(x + 24, 20, x + 40, 70, 5)                                  # Banners
        img.rect(x + 24, 20, x + 40, 24, 7)
        img.rect(x + 28, 70, x + 36, 76, 6)
    return img


def make_area_throne_approach():
    palette = [(255, 0, 255), (20, 18, 30), (30, 26, 44), (40, 36, 58), (70, 64, 90),
               (140, 20, 30), (170, 34, 44), (200, 160, 60), (46, 42, 56), (60, 56, 72), (76, 70, 88),
               (34, 30, 44), (14, 12, 20)]
    img = area_base(palette, [1, 2, 3], [8, 9, 10, 10], 11)
    for x in (24, 72, 168, 216):
        img.rect(x, 0, x + 16, AREA_HORIZON, 4)
        img.rect(x + 12, 0, x + 16, AREA_HORIZON, 12)
    for y in range(AREA_HORIZON, 256):                                      # Red carpet to the throne
        half = 14 + (y - AREA_HORIZON) // 4
        img.rect(128 - half, y, 128 + half, y + 1, 5)
        img.rect(128 - half, y, 128 - half + 2, y + 1, 7)
        img.rect(128 + half - 2, y, 128 + half, y + 1, 7)
    img.rect(112, 40, 144, AREA_HORIZON, 12)                                 # Doorway ahead
    return img


def make_area_throne_room():
    palette = [(255, 0, 255), (30, 14, 40), (44, 22, 58), (60, 30, 76), (250, 200, 60),
               (140, 20, 30), (170, 34, 44), (200, 150, 40), (50, 30, 60), (66, 40, 78), (84, 52, 96),
               (36, 20, 46), (14, 8, 20)]
    img = area_base(palette, [1, 2, 3], [8, 9, 10, 10], 11)
    img.rect(104, 30, 152, AREA_HORIZON, 12)                                 # Throne silhouette
    img.rect(96, 76, 160, AREA_HORIZON, 12)
    img.rect(104, 30, 152, 34, 4)
    for x in (108, 124, 140):
        img.rect(x, 22, x + 6, 30, 4)                                        # Crown points
    for y in range(AREA_HORIZON, 256):
        half = 18 + (y - AREA_HORIZON) // 4
        img.rect(128 - half, y, 128 + half, y + 1, 5)
    return img


AREA_MAKERS = [
    ("area_gate", make_area_gate),
    ("area_garden", make_area_garden),
    ("area_barracks", make_area_barracks),
    ("area_great_hall", make_area_great_hall),
    ("area_throne_approach", make_area_throne_approach),
    ("area_throne_room", make_area_throne_room),
]


# ---------------------------------------------------------------------------------------------
# Hotspot markers: 16x16 sprite, 11 frames stacked vertically:
# 0 duel, 1 duel won, 2 gold pouch, 3 pouch taken, 4 chest, 5 chest opened, 6 locked chest,
# 7 exit, 8 exit blocked, 9 God King (crown), 10 elite guard (skull).
# ---------------------------------------------------------------------------------------------

MARKER_PALETTE = [
    (255, 0, 255),    # 0 transparent
    (8, 8, 12),       # 1 outline
    (200, 210, 224),  # 2 steel
    (250, 200, 60),   # 3 gold
    (130, 90, 40),    # 4 wood / leather
    (90, 60, 30),     # 5 dark wood
    (90, 90, 100),    # 6 dim (taken / blocked)
    (60, 200, 90),    # 7 exit green
    (230, 60, 50),    # 8 red
    (240, 240, 230),  # 9 bone
]


def make_markers():
    img = Image(16, 16 * 11, MARKER_PALETTE)

    def cell(frame):
        return frame * 16

    def swords(oy, color):
        img.line(3, oy + 3, 12, oy + 12, 2, color)
        img.line(12, oy + 3, 3, oy + 12, 2, color)
        img.rect(2, oy + 11, 6, oy + 13, 4 if color == 2 else color)
        img.rect(10, oy + 11, 14, oy + 13, 4 if color == 2 else color)

    def pouch(oy, color, tie):
        img.rect(4, oy + 6, 12, oy + 14, color)
        img.rect(5, oy + 5, 11, oy + 6, color)
        img.rect(6, oy + 3, 10, oy + 5, tie)

    def chest(oy, body, lid_open, lock):
        img.rect(2, oy + 8, 14, oy + 14, body)
        if lid_open:
            img.rect(2, oy + 3, 14, oy + 6, body)
            img.rect(4, oy + 6, 12, oy + 8, 3)                               # Gold inside
        else:
            img.rect(2, oy + 5, 14, oy + 8, body)
        img.rect(2, oy + 8, 14, oy + 9, 3)
        if lock:
            img.rect(6, oy + 7, 10, oy + 12, 2)
            img.rect(7, oy + 9, 9, oy + 10, 1)

    def door(oy, color):
        img.rect(4, oy + 3, 12, oy + 14, color)
        img.rect(5, oy + 2, 11, oy + 3, color)
        img.rect(6, oy + 7, 10, oy + 8, 1)                                   # Arrow-ish bar
        img.rect(8, oy + 5, 9, oy + 10, 1)

    swords(cell(0), 2)
    swords(cell(1), 6)
    pouch(cell(2), 4, 3)
    pouch(cell(3), 6, 6)
    chest(cell(4), 5, False, False)
    chest(cell(5), 5, True, False)
    chest(cell(6), 5, False, True)
    door(cell(7), 7)
    door(cell(8), 6)
    oy = cell(9)                                                             # Crown
    img.rect(2, oy + 7, 14, oy + 13, 3)
    for x in (2, 7, 12):
        img.rect(x, oy + 3, x + 2, oy + 7, 3)
    img.rect(7, oy + 9, 9, oy + 11, 8)
    oy = cell(10)                                                            # Skull
    img.rect(4, oy + 3, 12, oy + 11, 9)
    img.rect(6, oy + 11, 10, oy + 14, 9)
    img.rect(5, oy + 6, 7, oy + 8, 1)
    img.rect(9, oy + 6, 11, oy + 8, 1)

    for frame in range(11):
        img.outline(1, (0, cell(frame), 16, cell(frame) + 16))
    return img


def make_arena():
    palette = [
        (255, 0, 255),    # 0 transparent (unused)
        (28, 30, 44),     # 1 wall darkest
        (38, 40, 58),     # 2 wall dark
        (50, 52, 72),     # 3 wall mid
        (62, 64, 86),     # 4 wall light
        (18, 18, 28),     # 5 mortar
        (58, 44, 34),     # 6 floor far
        (70, 54, 40),     # 7
        (82, 64, 46),     # 8
        (96, 74, 52),     # 9
        (110, 86, 60),    # 10
        (124, 98, 68),    # 11
        (138, 110, 76),   # 12 floor near
        (40, 30, 24),     # 13 floor seam
        (92, 84, 70),     # 14 trim
        (60, 54, 44),     # 15 trim shadow
    ]
    img = Image(256, 256, palette)

    wall_bottom = 144
    floor_top = 152

    # Wall: 32x16 bricks, rows offset by half a brick, darker toward the top.
    for brick_row in range(wall_bottom // 16):
        y0 = brick_row * 16
        shade = 1 + min(3, brick_row * 4 // (wall_bottom // 16))
        img.rect(0, y0, 256, y0 + 16, shade)
        img.rect(0, y0 + 15, 256, y0 + 16, 5)
        offset = 16 if brick_row % 2 else 0
        for x in range(offset, 256 + 32, 32):
            img.rect((x - 1) % 256, y0, (x - 1) % 256 + 1, y0 + 15, 5)

    # Trim at the base of the wall.
    img.rect(0, wall_bottom, 256, floor_top - 2, 14)
    img.rect(0, floor_top - 2, 256, floor_top, 15)

    # Floor: bands that lighten toward the viewer, with stone seams.
    band_colors = [6, 7, 8, 9, 10, 11, 12]
    band_height = 8
    for y in range(floor_top, 256):
        band = min(len(band_colors) - 1, (y - floor_top) // band_height)
        img.rect(0, y, 256, y + 1, band_colors[band])
    for y in range(floor_top + 15, 256, 16):
        img.rect(0, y, 256, y + 1, 13)
    for x in range(15, 256, 32):
        img.rect(x, floor_top, x + 1, 256, 13)

    return img


# ---------------------------------------------------------------------------------------------
# Enemy: 256x256 regular BG with an armored figure about 112 px tall, centered on
# column 128. Rows 72..184 map to screen y -56..56 with the BG at (0, 0).
# ---------------------------------------------------------------------------------------------

ENEMY_PALETTE = [
    (255, 0, 255),    # 0 transparent
    (10, 8, 12),      # 1 outline
    (56, 52, 64),     # 2 armor dark
    (92, 88, 104),    # 3 armor mid
    (140, 136, 152),  # 4 armor light
    (230, 40, 30),    # 5 visor glow
    (176, 188, 200),  # 6 blade
    (236, 244, 252),  # 7 blade edge
    (96, 60, 36),     # 8 leather
    (200, 160, 60),   # 9 gold trim
]


# The enemy is drawn in a three-quarter view, turned toward the hero in the lower left:
# front details (visor, chest plate, buckle) shift to screen left by ENEMY_TURN pixels,
# his left side (screen right) shows in shadow, and the far shoulder is narrower.
ENEMY_TURN = 3


# Palette swaps for the other enemy types; same color slots as ENEMY_PALETTE.
ENEMY_TYPE_PALETTES = {
    "brute": [
        (255, 0, 255),    # 0 transparent
        (12, 6, 4),       # 1 outline
        (74, 46, 26),     # 2 armor dark (bronze)
        (128, 86, 46),    # 3 armor mid
        (188, 140, 78),   # 4 armor light
        (255, 120, 20),   # 5 visor glow (orange)
        (150, 150, 156),  # 6 blade (dull iron)
        (210, 210, 214),  # 7 blade edge
        (70, 40, 26),     # 8 leather
        (120, 120, 128),  # 9 trim (iron)
    ],
    "duelist": [
        (255, 0, 255),    # 0 transparent
        (6, 6, 14),       # 1 outline
        (28, 34, 70),     # 2 armor dark (navy)
        (52, 66, 124),    # 3 armor mid
        (104, 124, 196),  # 4 armor light
        (60, 220, 255),   # 5 visor glow (cyan)
        (196, 208, 224),  # 6 blade
        (250, 252, 255),  # 7 blade edge
        (60, 34, 50),     # 8 leather
        (230, 190, 70),   # 9 trim (gold)
    ],
    "god_king": [
        (255, 0, 255),    # 0 transparent
        (8, 4, 12),       # 1 outline
        (26, 14, 38),     # 2 armor dark (royal black-purple)
        (64, 32, 86),     # 3 armor mid
        (130, 84, 164),   # 4 armor light
        (250, 250, 255),  # 5 visor glow (white)
        (240, 206, 120),  # 6 blade (gilded)
        (255, 250, 214),  # 7 blade edge
        (92, 20, 32),     # 8 leather (crimson)
        (250, 200, 60),   # 9 trim (gold)
    ],
}


def make_enemy_type_palettes():
    """Returns [(name, image)]: tiny images that only carry a palette (Butano bg_palette assets)."""
    return [("enemy_palette_" + name, Image(8, 8, palette)) for name, palette in ENEMY_TYPE_PALETTES.items()]


def draw_enemy_body(img, cx):
    t = ENEMY_TURN

    # Legs and boots; boots point toward the hero.
    img.rect(cx - 20, 146, cx - 4, 184, 2)
    img.rect(cx + 4, 146, cx + 20, 184, 2)
    img.rect(cx - 20, 146, cx - 16, 176, 3)
    img.rect(cx + 4, 146, cx + 8, 176, 3)
    img.rect(cx - 22 - t, 176, cx - 2, 184, 8)
    img.rect(cx + 2 - t, 176, cx + 22 - t // 2, 184, 8)

    # Torso: lit front, shadowed side on the right, chest plate shifted toward the hero.
    img.rect(cx - 24, 96, cx + 24, 140, 3)
    img.rect(cx - 24, 96, cx - 18, 140, 4)
    img.rect(cx + 24 - 2 * t, 96, cx + 24, 140, 2)
    img.rect(cx - 12 - t, 102, cx + 12 - t, 128, 2)
    img.rect(cx - 1 - t, 102, cx + 1 - t, 128, 9)

    # Belt.
    img.rect(cx - 24, 140, cx + 24, 146, 8)
    img.rect(cx - 4 - t, 140, cx + 4 - t, 146, 9)

    # Shoulder pads: far one (screen left) narrower, near one (screen right) wider.
    img.rect(cx - 36 + t // 2, 94, cx - 20, 110, 4)
    img.rect(cx + 20, 94, cx + 36 + t // 2, 110, 4)
    img.rect(cx - 36 + t // 2, 108, cx - 20, 110, 9)
    img.rect(cx + 20, 108, cx + 36 + t // 2, 110, 9)

    # Helmet: shadowed side on the right, visor slit and crest turned toward the hero.
    img.rect(cx - 12, 72, cx + 12, 96, 3)
    img.rect(cx - 12, 72, cx - 8, 96, 4)
    img.rect(cx + 12 - t, 72, cx + 12, 96, 2)
    img.rect(cx - 12, 82, cx + 9 - 2 * t, 86, 5)
    img.rect(cx - 2 - t, 66, cx + 2 - t, 72, 9)


def draw_enemy_idle(img, cx):
    draw_enemy_body(img, cx)

    # Left arm hangs at the side.
    img.rect(cx - 34, 110, cx - 24, 140, 2)
    img.rect(cx - 34, 140, cx - 24, 148, 8)

    # Right arm bent forward, sword held upright in guard.
    img.rect(cx + 24, 110, cx + 34, 132, 2)
    img.rect(cx + 26, 132, cx + 38, 140, 8)
    img.rect(cx + 30, 140, cx + 34, 152, 8)          # grip below the hand
    img.rect(cx + 22, 128, cx + 42, 132, 9)          # crossguard
    img.rect(cx + 30, 64, cx + 34, 128, 6)           # blade
    img.rect(cx + 30, 64, cx + 31, 128, 7)
    img.rect(cx + 31, 60, cx + 33, 64, 6)            # tip


# Two-handed sword poses in screen coordinates (origin at the screen center, which is
# image (128, 128) with the BG at (0, 0)): (hand, tip). Names match graphics/enemy_<name>.bmp
# and the order of attack_id in src/attack_data.h. "wu" = wind-up, "st" = strike follow-through.
ENEMY_SHOULDERS = ((-28, -26), (28, -26))

ENEMY_SWORD_POSES = [
    ("wu_slash_l", (-36, -50), (-66, -78)),    # Diagonal / : raised high left
    ("st_slash_l", (30, 20), (62, 52)),
    ("wu_slash_r", (36, -50), (66, -78)),      # Diagonal \ : raised high right
    ("st_slash_r", (-30, 20), (-62, 52)),
    ("wu_horiz_l", (-46, -10), (-102, -16)),   # Horizontal from the left: pulled back left
    ("st_horiz_l", (46, -6), (102, 0)),
    ("wu_horiz_r", (46, -10), (102, -16)),     # Horizontal from the right
    ("st_horiz_r", (-46, -6), (-102, 0)),
    ("wu_overhead", (0, -66), (0, -112)),      # Overhead: raised straight up
    ("st_overhead", (0, 8), (0, 62)),
    ("wu_sweep", (-38, 34), (-94, 54)),        # Low sweep: dropped low left
    ("st_sweep", (38, 34), (94, 54)),
    ("wu_thrust", (32, 8), (-2, 14)),          # Thrust: pulled back at the waist
    ("st_thrust", (0, -2), (0, 34)),
    ("wu_rise_l", (-32, 24), (-40, 80)),       # Rising /: dropped low left, blade pointing straight down
    ("st_rise_l", (34, -50), (66, -78)),       # ...finishes high right
    ("wu_rise_r", (32, 24), (40, 80)),         # Rising \: dropped low right
    ("st_rise_r", (-34, -50), (-66, -78)),
]


def draw_enemy_sword_pose(img, hand, tip):
    def to_image(point):
        return point[0] + 128, point[1] + 128

    hx, hy = to_image(hand)
    tx, ty = to_image(tip)
    dx, dy = tx - hx, ty - hy
    length = max(1.0, (dx * dx + dy * dy) ** 0.5)
    ux, uy = dx / length, dy / length

    # Arms from both shoulders to the hands.
    for shoulder in ENEMY_SHOULDERS:
        sx, sy = to_image(shoulder)
        img.line(sx, sy + 6, hx, hy, 9, 2)

    # Blade, edge highlight, crossguard, grip, then the gauntlets on top.
    img.line(hx + ux * 6, hy + uy * 6, tx, ty, 4, 6)
    img.line(hx + ux * 6 - uy, hy + uy * 6 + ux, tx - uy, ty + ux, 1, 7)
    img.line(hx + ux * 5 - uy * 9, hy + uy * 5 + ux * 9, hx + ux * 5 + uy * 9, hy + uy * 5 - ux * 9, 3, 9)
    img.line(hx, hy, hx - ux * 8, hy - uy * 8, 3, 8)
    img.rect(round(hx) - 5, round(hy) - 5, round(hx) + 5, round(hy) + 5, 8)


def make_enemy_poses():
    """Returns [(name, image)]: idle first, then a wind-up and a strike pose per attack."""
    idle = Image(256, 256, ENEMY_PALETTE)
    draw_enemy_idle(idle, 128)
    idle.outline(1)
    poses = [("idle", idle)]

    for name, hand, tip in ENEMY_SWORD_POSES:
        img = Image(256, 256, ENEMY_PALETTE)
        draw_enemy_body(img, 128)
        draw_enemy_sword_pose(img, hand, tip)
        img.outline(1)
        poses.append((name, img))

    return poses


# ---------------------------------------------------------------------------------------------
# Telegraph: 8x8 sprite, 2 frames stacked vertically: normal (yellow) and unblockable (red).
# ---------------------------------------------------------------------------------------------

TELEGRAPH_PALETTE = [
    (255, 0, 255),    # 0 transparent
    (255, 255, 255),  # 1 core
    (255, 220, 60),   # 2 normal glow
    (200, 120, 20),   # 3 normal rim
    (255, 70, 50),    # 4 unblockable glow
    (150, 10, 10),    # 5 unblockable rim
]


def make_telegraph():
    img = Image(8, 16, TELEGRAPH_PALETTE)
    for frame, (glow, rim) in enumerate(((2, 3), (4, 5))):
        oy = frame * 8
        img.rect(2, oy + 1, 6, oy + 7, rim)
        img.rect(1, oy + 2, 7, oy + 6, rim)
        img.rect(2, oy + 2, 6, oy + 6, glow)
        img.rect(3, oy + 3, 5, oy + 5, 1)
    return img


# ---------------------------------------------------------------------------------------------
# Hero: 64x64 sprite seen from behind, 5 frames stacked vertically: idle, dodge, duck, block, parry.
# Dodge leans right; the game flips it horizontally for a left dodge.
# ---------------------------------------------------------------------------------------------

HERO_PALETTE = [
    (255, 0, 255),    # 0 transparent
    (6, 6, 10),       # 1 outline
    (30, 34, 48),     # 2 body dark
    (52, 58, 80),     # 3 body mid
    (90, 100, 132),   # 4 rim light
    (70, 40, 24),     # 5 hair
    (150, 30, 30),    # 6 cape
    (100, 18, 20),    # 7 cape shadow
    (190, 200, 214),  # 8 blade
    (245, 250, 255),  # 9 blade edge
    (200, 160, 60),   # 10 hilt gold
    (96, 60, 36),     # 11 grip
]


def draw_hero(img, ox, oy, lean=0, drop=0, block=False, parry=False):
    """Draws the hero into the 64x64 cell at (ox, oy), clipped to the cell.

    lean shears the upper body right, drop lowers the whole figure, block raises
    a round shield on the left arm, and parry swings the sword across to the upper left
    (flipped in game for a right parry).
    """

    def sx(x, y):
        # Shear: the higher the pixel, the further it shifts by lean.
        return ox + x + round(lean * (63 - y) / 63)

    def plot(x, y, c):
        y += drop
        if 0 <= y < 64:
            img.set(sx(x, y), oy + y, c)

    def rect(x0, y0, x1, y1, c):
        for y in range(y0, y1):
            for x in range(x0, x1):
                plot(x, y, c)

    def ellipse(cx, cy, rx, ry, c):
        for y in range(cy - ry, cy + ry + 1):
            for x in range(cx - rx, cx + rx + 1):
                dx = (x - cx) / rx
                dy = (y - cy) / ry
                if dx * dx + dy * dy <= 1.0:
                    plot(x, y, c)

    def line(x0, y0, x1, y1, thickness, c):
        steps = max(abs(x1 - x0), abs(y1 - y0), 1)
        r = thickness // 2
        for i in range(steps + 1):
            x = round(x0 + (x1 - x0) * i / steps)
            y = round(y0 + (y1 - y0) * i / steps)
            rect(x - r, y - r, x - r + thickness, y - r + thickness, c)

    # Sword in the right hand, raised up and out to the right.
    if not parry:
        line(49, 40, 60, 6, 3, 8)
        line(48, 40, 59, 6, 1, 9)
        line(44, 40, 54, 44, 2, 10)    # crossguard
        line(50, 42, 47, 50, 2, 11)    # grip

    # Cape and torso.
    rect(14, 26, 50, 64, 6)
    rect(14, 26, 20, 64, 7)
    rect(18, 22, 46, 30, 3)            # shoulders
    rect(16, 22, 20, 30, 4)

    # Arms.
    if block:
        rect(10, 22, 18, 40, 2)        # shield arm raised
    elif not parry:
        rect(8, 28, 16, 52, 2)
        rect(8, 28, 10, 52, 4)

    if parry:
        line(44, 28, 22, 26, 6, 2)     # sword arm swung across the body
        line(12, 30, 20, 26, 6, 2)     # off hand joins the grip
    else:
        line(46, 28, 50, 44, 6, 2)     # sword arm

    # Head from behind: hair.
    ellipse(32, 15, 8, 9, 5)
    rect(28, 22, 36, 26, 2)            # neck

    # Parry: blade swung up and across to meet the incoming swing.
    if parry:
        line(18, 24, 2, 2, 3, 8)
        line(17, 24, 1, 2, 1, 9)
        line(14, 30, 24, 20, 2, 10)    # crossguard
        line(20, 26, 24, 31, 2, 11)    # grip

    # Round shield raised beside the head.
    if block:
        ellipse(15, 20, 13, 14, 10)
        ellipse(15, 20, 10, 11, 3)
        ellipse(15, 20, 3, 3, 4)


HERO_FRAMES = 5                        # idle, dodge, duck, block, parry


def make_hero():
    img = Image(64, 64 * HERO_FRAMES, HERO_PALETTE)
    draw_hero(img, 0, 0)               # idle
    draw_hero(img, 0, 64, lean=8)      # dodge (leans right)
    draw_hero(img, 0, 128, drop=14)    # duck
    draw_hero(img, 0, 192, block=True) # block
    draw_hero(img, 0, 256, parry=True) # parry (toward the upper left)
    for frame in range(HERO_FRAMES):
        img.outline(1, (0, frame * 64, 64, frame * 64 + 64))
    return img


# ---------------------------------------------------------------------------------------------
# Shield pip: 8x8 HUD sprite, 2 frames stacked vertically: full and empty.
# ---------------------------------------------------------------------------------------------

PIP_PALETTE = [
    (255, 0, 255),    # 0 transparent
    (8, 10, 20),      # 1 outline
    (70, 130, 220),   # 2 full fill
    (180, 220, 255),  # 3 full highlight
    (44, 46, 58),     # 4 empty fill
]


# ---------------------------------------------------------------------------------------------
# HP bar segment: 8x8 HUD sprite. Frames 0..8 are the player bar (green) filled 0..8 px from
# the left; frames 9..17 are the enemy bar (red) and 18..26 the super meter (gold), filled
# the same way.
# ---------------------------------------------------------------------------------------------

BAR_PALETTE = [
    (255, 0, 255),    # 0 transparent
    (8, 10, 20),      # 1 outline
    (40, 30, 36),     # 2 empty
    (60, 190, 80),    # 3 player fill
    (150, 240, 150),  # 4 player highlight
    (200, 40, 40),    # 5 enemy fill
    (255, 140, 120),  # 6 enemy highlight
    (230, 170, 30),   # 7 super fill
    (255, 240, 140),  # 8 super highlight
]

BAR_SEGMENT_FILLS = 9


def make_hp_bar():
    bars = ((3, 4), (5, 6), (7, 8))
    img = Image(8, 8 * BAR_SEGMENT_FILLS * len(bars), BAR_PALETTE)
    for bar, (fill, shine) in enumerate(bars):
        for filled in range(BAR_SEGMENT_FILLS):
            oy = (bar * BAR_SEGMENT_FILLS + filled) * 8
            img.rect(0, oy, 8, oy + 1, 1)
            img.rect(0, oy + 7, 8, oy + 8, 1)
            img.rect(0, oy + 1, 8, oy + 7, 2)
            img.rect(0, oy + 1, filled, oy + 7, fill)
            img.rect(0, oy + 1, filled, oy + 2, shine)
    return img


# ---------------------------------------------------------------------------------------------
# Input arrows: 16x16 HUD sprite for spell patterns and combos. 12 frames stacked vertically:
# up, down, left, right dim (0..3), lit (4..7) and mistake red (8..11).
# ---------------------------------------------------------------------------------------------

ARROW_PALETTE = [
    (255, 0, 255),    # 0 transparent
    (8, 8, 16),       # 1 outline
    (70, 76, 100),    # 2 dim fill
    (110, 118, 150),  # 3 dim highlight
    (240, 190, 40),   # 4 lit fill (gold)
    (255, 250, 190),  # 5 lit highlight
    (220, 40, 40),    # 6 mistake fill
    (255, 150, 140),  # 7 mistake highlight
]


def arrow_up_mask():
    """16x16 grid of booleans for an up arrow: triangle head over a stem."""
    mask = [[False] * 16 for _ in range(16)]
    for y in range(2, 9):
        half = y - 2
        for x in range(8 - half - 1, 8 + half + 1):
            mask[y][x] = True
    for y in range(9, 14):
        for x in range(6, 10):
            mask[y][x] = True
    return mask


def make_arrows():
    up = arrow_up_mask()
    rotations = {
        "up": lambda x, y: up[y][x],
        "down": lambda x, y: up[15 - y][x],
        "left": lambda x, y: up[x][y],
        "right": lambda x, y: up[15 - x][y],
    }
    img = Image(16, 16 * 12, ARROW_PALETTE)
    for state, (fill, shine) in enumerate(((2, 3), (4, 5), (6, 7))):
        for index, direction in enumerate(("up", "down", "left", "right")):
            oy = (state * 4 + index) * 16
            inside = rotations[direction]
            for y in range(16):
                for x in range(16):
                    if inside(x, y):
                        img.set(x, oy + y, fill)
            # Highlight the upper-left edge of the shape.
            for y in range(16):
                for x in range(16):
                    if inside(x, y) and (x == 0 or not inside(x - 1, y) or y == 0 or not inside(x, y - 1)):
                        img.set(x, oy + y, shine)
            img.outline(1, (0, oy, 16, oy + 16))
    return img


# ---------------------------------------------------------------------------------------------
# Magic charge orb: 8x8 HUD sprite, 2 frames: full and empty.
# ---------------------------------------------------------------------------------------------

ORB_PALETTE = [
    (255, 0, 255),    # 0 transparent
    (6, 8, 20),       # 1 outline
    (40, 120, 230),   # 2 full fill
    (170, 230, 255),  # 3 full highlight
    (40, 42, 56),     # 4 empty fill
]


def make_charge_orb():
    img = Image(8, 16, ORB_PALETTE)
    for frame, (fill, shine) in enumerate(((2, 3), (4, 4))):
        oy = frame * 8
        img.rect(2, oy + 0, 6, oy + 8, 1)
        img.rect(0, oy + 2, 8, oy + 6, 1)
        img.rect(1, oy + 1, 7, oy + 7, 1)
        img.rect(2, oy + 1, 6, oy + 7, fill)
        img.rect(1, oy + 2, 7, oy + 6, fill)
        img.rect(2, oy + 2, 4, oy + 4, shine)
    return img


# ---------------------------------------------------------------------------------------------
# Impact sparks: 16x16 sprite, 8 frames stacked vertically: a 4-frame gold burst (parries,
# combo finishers) then a 4-frame blue burst (blocks). Each burst flashes, spreads and fades.
# ---------------------------------------------------------------------------------------------

SPARK_PALETTE = [
    (255, 0, 255),    # 0 transparent
    (255, 255, 255),  # 1 white core
    (255, 230, 120),  # 2 gold
    (240, 150, 40),   # 3 orange
    (190, 240, 255),  # 4 pale blue
    (80, 170, 255),   # 5 blue
    (40, 90, 200),    # 6 deep blue
]


def make_sparks():
    img = Image(16, 16 * 8, SPARK_PALETTE)
    for burst, (core, mid, edge) in enumerate(((1, 2, 3), (1, 4, 5))):
        for step in range(4):
            oy = (burst * 4 + step) * 16
            cx, cy = 8, oy + 8
            reach = 2 + step * 2
            color_ray = mid if step < 2 else edge
            # Eight rays that lengthen, then break into dots as the burst fades.
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (-1, -1), (1, -1), (-1, 1)):
                length = reach if dx == 0 or dy == 0 else reach - 2
                for r in range(1, max(2, length)):
                    if step == 3 and r < length - 2:
                        continue
                    img.set(cx + dx * r, cy + dy * r, color_ray)
            if step < 2:
                img.rect(cx - 1 - step, cy - 1 - step, cx + 1 + step, cy + 1 + step, mid)
                img.rect(cx - 1, cy - 1, cx + 1, cy + 1, core)
    return img


# ---------------------------------------------------------------------------------------------
# Damage numbers: 8x8 sprite, 11 glyphs (0-9 and +) in 4 colors, stacked vertically:
# frames 0..10 white (hits on the enemy), 11..21 gold (finishers, super, spells, burn),
# 22..32 red (damage to the hero), 33..43 green (healing). Bold 5x7 glyphs with an outline.
# ---------------------------------------------------------------------------------------------

DIGIT_PALETTE = [
    (255, 0, 255),    # 0 transparent
    (10, 6, 10),      # 1 outline
    (255, 255, 255),  # 2 white
    (200, 200, 216),  # 3 white shade
    (255, 220, 80),   # 4 gold
    (220, 150, 30),   # 5 gold shade
    (255, 80, 70),    # 6 red
    (190, 30, 30),    # 7 red shade
    (120, 240, 120),  # 8 green
    (40, 170, 60),    # 9 green shade
]

DIGIT_GLYPHS = [
    ["01110", "10001", "10011", "10101", "11001", "10001", "01110"],  # 0
    ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],  # 1
    ["01110", "10001", "00001", "00010", "00100", "01000", "11111"],  # 2
    ["11110", "00001", "00001", "01110", "00001", "00001", "11110"],  # 3
    ["00010", "00110", "01010", "10010", "11111", "00010", "00010"],  # 4
    ["11111", "10000", "11110", "00001", "00001", "10001", "01110"],  # 5
    ["00110", "01000", "10000", "11110", "10001", "10001", "01110"],  # 6
    ["11111", "00001", "00010", "00100", "01000", "01000", "01000"],  # 7
    ["01110", "10001", "10001", "01110", "10001", "10001", "01110"],  # 8
    ["01110", "10001", "10001", "01111", "00001", "00010", "01100"],  # 9
    ["00000", "00100", "00100", "11111", "00100", "00100", "00000"],  # +
]


def make_digits():
    colors = ((2, 3), (4, 5), (6, 7), (8, 9))
    img = Image(8, 8 * len(DIGIT_GLYPHS) * len(colors), DIGIT_PALETTE)
    for color_index, (fill, shade) in enumerate(colors):
        for glyph_index, glyph in enumerate(DIGIT_GLYPHS):
            oy = (color_index * len(DIGIT_GLYPHS) + glyph_index) * 8
            for y, row in enumerate(glyph):
                for x, bit in enumerate(row):
                    if bit == "1":
                        # Lower rows a shade darker for a little depth.
                        img.set(1 + x, oy + y, shade if y >= 5 else fill)
            img.outline(1, (0, oy, 8, oy + 8))
    return img


def make_shield_pip():
    img = Image(8, 16, PIP_PALETTE)
    for frame, (fill, shine) in enumerate(((2, 3), (4, 4))):
        oy = frame * 8
        # Shield shape: flat top, pointed bottom.
        img.rect(1, oy + 0, 7, oy + 5, 1)
        img.rect(2, oy + 5, 6, oy + 7, 1)
        img.rect(3, oy + 7, 5, oy + 8, 1)
        img.rect(2, oy + 1, 6, oy + 5, fill)
        img.rect(3, oy + 5, 5, oy + 6, fill)
        img.rect(2, oy + 1, 3, oy + 3, shine)
    return img


def write_preview(path, arena, enemy_poses, hero):
    """Writes the duel screen mock-up to path, plus _hero and _enemy contact sheets beside it."""
    from PIL import Image as PilImage

    def screen(enemy):
        out = PilImage.new("RGB", (240, 160))
        px = out.load()
        for y in range(160):
            for x in range(240):
                px[x, y] = arena.rgb(x + 8, y + 48)
                if enemy.get(x + 8, y + 48):
                    px[x, y] = enemy.rgb(x + 8, y + 48)
        # Hero sprite centered at screen (-84, 52) -> top-left (4, 100).
        for y in range(64):
            for x in range(64):
                sy = 100 + y
                if sy < 160 and hero.get(x, y):
                    px[4 + x, sy] = hero.rgb(x, y)
        return out

    screen(enemy_poses[0][1]).resize((480, 320), PilImage.NEAREST).save(path)

    sheet = PilImage.new("RGB", (64, hero.height))
    sp = sheet.load()
    for y in range(hero.height):
        for x in range(64):
            sp[x, y] = hero.rgb(x, y) if hero.get(x, y) else (60, 60, 60)
    sheet.resize((256, hero.height * 4), PilImage.NEAREST).save(path.replace(".png", "_hero.png"))

    # Enemy poses: idle alone, then wind-up / strike pairs side by side.
    cols = 4
    rows = (len(enemy_poses) + cols - 1) // cols
    contact = PilImage.new("RGB", (240 * cols, 160 * rows), (0, 0, 0))
    for i, (_, img) in enumerate(enemy_poses):
        contact.paste(screen(img), ((i % cols) * 240, (i // cols) * 160))
    contact.save(path.replace(".png", "_enemy.png"))


def main():
    arena = make_arena()
    enemy_poses = make_enemy_poses()
    hero = make_hero()
    telegraph = make_telegraph()
    shield_pip = make_shield_pip()
    hp_bar = make_hp_bar()
    arrows = make_arrows()
    charge_orb = make_charge_orb()

    # The arena and area backgrounds are drawn by tools/make_background_art.py; the HP bars,
    # shield pips, charge orbs and markers by tools/make_ui_art.py.
    make_sparks().save_bmp(os.path.join(GRAPHICS_DIR, "sparks.bmp"))
    make_digits().save_bmp(os.path.join(GRAPHICS_DIR, "digits.bmp"))
    # The enemy's poses and type palettes are drawn by tools/make_enemy_art.py.
    # The final hero is drawn by tools/make_hero_art.py; the placeholder only feeds the preview.
    telegraph.save_bmp(os.path.join(GRAPHICS_DIR, "telegraph.bmp"))
    arrows.save_bmp(os.path.join(GRAPHICS_DIR, "arrows.bmp"))
    print("Wrote arena, %d enemy poses, hero, telegraph and HUD sprites to graphics/" % len(enemy_poses))

    if "--preview" in sys.argv:
        write_preview(sys.argv[sys.argv.index("--preview") + 1], arena, enemy_poses, hero)
        print("Wrote preview")


if __name__ == "__main__":
    main()
