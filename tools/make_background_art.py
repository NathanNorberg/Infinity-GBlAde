#!/usr/bin/env python3
"""Draws the backgrounds (graphics/area_*.bmp and graphics/title.bmp) in the style guide's look
(docs/Infinity Blade GBA Art Style Guide.md): dark, desaturated stone with cool shadows and warm
highlights, small warm accents (torches, candles, gold) and moonlit stained colors.

Run from the project folder:  python3 tools/make_background_art.py [--preview out.png]

Each area has one 256x256 background used both behind its duels and as its area screen. With the
background at (0, 0) the screen shows columns 8-247 and rows 48-207. The wall meets the floor at
row 144 (screen y 96); the enemy stands on the floor around rows 150-194.
"""

import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import art_style  # noqa: E402
from make_placeholder_art import Image  # noqa: E402

GRAPHICS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "graphics")
HORIZON = 144
VANISH = (128, 70)                         # Floor lines converge here


class Scene:
    """A 256x256 background with named color ramps mapped onto one 16-color palette."""

    def __init__(self, ramps):
        colors = [(255, 0, 255)]
        self.ramps = {}
        for name, values in ramps:
            start = len(colors)
            colors.extend(values)
            self.ramps[name] = list(range(start, start + len(values)))
        if len(colors) > 16:
            raise ValueError("Background palette has %d colors" % len(colors))
        self.image = Image(256, 256, colors)
        self.random = random.Random(7)

    def color(self, ramp, value):
        """value 0..1 along a ramp (dark to light)."""
        indices = self.ramps[ramp]
        return indices[max(0, min(len(indices) - 1, int(value * len(indices))))]

    def put(self, x, y, ramp, value):
        self.image.set(int(x), int(y), self.color(ramp, value))

    def dither(self, x, y, ramp, value):
        """Checker dither between two neighboring ramp steps for smooth large gradients."""
        steps = len(self.ramps[ramp])
        scaled = value * steps
        low = int(scaled)
        fraction = scaled - low
        threshold = 0.25 if (x + y) % 2 == 0 else 0.75
        self.put(x, y, ramp, (low + (1 if fraction > threshold else 0)) / steps)

    # Building blocks --------------------------------------------------------------------------

    def sky(self, top, bottom, ramp, stars=0, moon=None):
        for y in range(top, bottom):
            for x in range(256):
                self.dither(x, y, ramp, 0.15 + 0.5 * (y - top) / max(1, bottom - top))
        for _ in range(stars):
            self.put(self.random.randrange(256), self.random.randrange(top, bottom - 10), ramp, 0.99)
        if moon:
            mx, my, radius = moon
            for y in range(my - radius - 3, my + radius + 4):
                for x in range(mx - radius - 3, mx + radius + 4):
                    d = math.hypot(x - mx, y - my)
                    if d <= radius:
                        self.put(x, y, "moon", 0.9 if (x - mx) < radius * 0.4 else 0.5)
                    elif d <= radius + 3 and (x + y) % 2 == 0:
                        self.put(x, y, ramp, 0.8)                     # Glow halo

    def bricks(self, x0, y0, x1, y1, ramp, width=16, height=8, base=0.4, top_light=0.15):
        """Stone blocks with mortar lines, each block a little lighter or darker, lit from above."""
        for row, y in enumerate(range(y0, y1, height)):
            offset = (width // 2) if row % 2 else 0
            for bx in range(x0 - offset, x1, width):
                variation = self.random.uniform(-0.08, 0.08)
                for yy in range(y, min(y + height, y1)):
                    for xx in range(max(bx, x0), min(bx + width, x1)):
                        if yy == y + height - 1 or xx == bx + width - 1:
                            self.put(xx, yy, ramp, base - 0.25)           # Mortar
                        else:
                            light = top_light * (1 - (yy - y0) / max(1, y1 - y0))
                            edge = 0.1 if yy == y else 0.0                # Lit top edge of a block
                            self.put(xx, yy, ramp, base + variation + light + edge)

    def pillar(self, x, y0, y1, width, ramp, base=0.45):
        """A round pillar: cylindrical shading lit from the left, with a capital and a base."""
        for y in range(y0, y1):
            for xx in range(x - width // 2, x + width // 2 + 1):
                across = (xx - x) / (width / 2)
                self.put(xx, y, ramp, base + 0.3 * -across - 0.2 * across * across)
        for y in list(range(y0, y0 + 5)) + list(range(y1 - 6, y1)):
            for xx in range(x - width // 2 - 3, x + width // 2 + 4):
                across = (xx - x) / (width / 2 + 3)
                self.put(xx, y, ramp, base + 0.15 + 0.25 * -across)

    def arch_window(self, cx, top, width, height, glass_ramps, frame_ramp):
        """A pointed gothic window with stained glass panes in a stone frame."""
        half = width / 2
        for y in range(top, top + height):
            t = (y - top) / height
            # Pointed top: width grows like a gothic arch over the first third.
            w = half * min(1.0, math.sqrt(t / 0.33)) if t < 0.33 else half
            for x in range(int(cx - w - 2), int(cx + w + 3)):
                if abs(x - cx) > w:
                    self.put(x, y, frame_ramp, 0.55)                     # Frame
                    continue
                pane = (int((x - cx + half) // 6) + int(y // 8)) % len(glass_ramps)
                lead = (x - cx + half) % 6 == 0 or y % 8 == 0
                if lead:
                    self.put(x, y, frame_ramp, 0.05)                     # Lead lines
                else:
                    light = 0.45 + 0.4 * (1 - t) + (0.1 if (x + y) % 3 == 0 else 0.0)
                    self.put(x, y, glass_ramps[pane], light)
        for x in range(int(cx - half - 3), int(cx + half + 4)):
            self.put(x, top + height, frame_ramp, 0.65)                   # Sill

    def floor(self, ramp, y0=HORIZON, tiles=True, checker=False, base=0.35):
        """Floor in perspective: tile rows widen toward the viewer, joints converge on VANISH."""
        rows = []
        y, step = y0, 3.0
        while y < 256:
            rows.append(int(y))
            y += step
            step *= 1.28
        for yy in range(y0, 256):
            depth = (yy - y0) / (256 - y0)
            row_index = sum(1 for r in rows if r <= yy) - 1
            for x in range(256):
                # Column of the tile this pixel is in, measured along lines from the vanishing point.
                spread = (x - VANISH[0]) / max(1.0, yy - VANISH[1])
                column = int(math.floor(spread * 2.2))
                value = base + 0.3 * depth
                if checker and (column + row_index) % 2:
                    value -= 0.18
                if tiles and (yy in rows or abs(spread * 2.2 - round(spread * 2.2)) < 0.035 * (1 + depth)):
                    value -= 0.22                                         # Joints
                self.put(x, yy, ramp, value)

    def banner(self, x, y, width, length, cloth, trim):
        for yy in range(y, y + length):
            for xx in range(x, x + width):
                across = (xx - x) / max(1, width - 1)
                fold = math.sin((xx - x) * 0.9)
                self.put(xx, yy, cloth, 0.55 - 0.35 * across + 0.12 * fold)
            self.put(x, yy, trim, 0.7)
            self.put(x + width - 1, yy, trim, 0.5)
        for i in range(width // 2 + 1):                                  # Swallowtail
            for yy in range(y + length, y + length + width // 2 - i):
                self.put(x + i, yy, cloth, 0.35)
                self.put(x + width - 1 - i, yy, cloth, 0.3)
        for xx in range(x - 2, x + width + 2):
            self.put(xx, y, trim, 0.8)                                    # Rod

    def flame(self, x, y, size=1.0):
        """A torch or candle flame with a soft dithered glow."""
        for yy in range(int(y - 10 * size), int(y + 4 * size)):
            for xx in range(int(x - 7 * size), int(x + 8 * size)):
                d = math.hypot((xx - x) / size, (yy - y) / size)
                if d < 7 and (xx + yy) % 2 == 0 and self.image.get(xx, yy) not in self.ramps["fire"]:
                    self.put(xx, yy, "fire", 0.05)                        # Glow on the wall
        for yy in range(int(y - 6 * size), int(y + 2 * size)):
            t = (yy - (y - 6 * size)) / (8 * size)
            half = 2.5 * size * math.sin(t * math.pi)
            for xx in range(int(x - half), int(x + half) + 1):
                self.put(xx, yy, "fire", 0.4 + 0.6 * (1 - abs(xx - x) / max(0.5, half)) * (1 - t * 0.5))

    def save(self, name):
        self.image.save_bmp(os.path.join(GRAPHICS_DIR, name + ".bmp"))
        with open(os.path.join(GRAPHICS_DIR, name + ".json"), "w") as json_file:
            json_file.write('{\n    "type": "regular_bg",\n    "bpp_mode": "bpp_4"\n}\n')


R = art_style.ramp


# ---------------------------------------------------------------------------------------------
# The six areas and the title
# ---------------------------------------------------------------------------------------------

def make_gate():
    scene = Scene([("sky", R("night", (0, 1, 2, 3))), ("stone", R("stone")), ("moon", R("ice", (3, 4))),
                   ("fire", R("fire", (1, 2, 3, 4)))])
    scene.sky(0, 100, "sky", stars=40, moon=(196, 58, 9))
    scene.bricks(0, 100, 256, HORIZON, "stone", base=0.38)
    for x in range(0, 256, 20):                                           # Battlements
        for y in range(88, 100):
            for xx in range(x, x + 12):
                scene.put(xx, y, "stone", 0.45 if y > 89 else 0.6)
    for y in range(102, HORIZON):                                         # Gate arch and portcullis
        half = 22 if y > 112 else 22 * math.sqrt(max(0.0, (y - 102) / 10))
        for x in range(int(128 - half), int(128 + half) + 1):
            bar = (x - 106) % 6 == 0 or (y - 104) % 7 == 0
            scene.put(x, y, "stone", 0.25 if bar else 0.0)
    for x in (82, 174):
        for y in range(116, 124):
            scene.put(x, y, "stone", 0.2)                                 # Torch brackets
        scene.flame(x, 113)
    scene.floor("stone", base=0.25)                                       # Cobbled courtyard
    return scene


def make_garden():
    scene = Scene([("sky", R("night", (1, 2, 3))), ("leaf", R("foliage")), ("stone", R("moss_stone", (1, 2, 3, 4))),
                   ("moon", R("ice", (3, 4)))])
    scene.sky(0, 104, "sky", stars=50, moon=(58, 40, 7))
    for x in range(0, 256, 40):                                           # Hedge wall with leafy texture
        for y in range(96, HORIZON):
            for xx in range(x, x + 38):
                leaf = scene.random.uniform(-0.12, 0.12)
                scene.put(xx, y, "leaf", 0.3 + 0.25 * (1 - (y - 96) / 48) + leaf)
        for y in range(90, 96):
            for xx in range(x + 2, x + 36):
                scene.put(xx, y, "leaf", 0.55 + scene.random.uniform(-0.1, 0.1))
    for side in (-1, 1):                                                  # Stone arbor posts and arch
        scene.pillar(128 + side * 30, 70, HORIZON, 8, "stone", 0.5)
    for x in range(98, 159):
        y = 70 - int(10 * math.sin((x - 98) / 60 * math.pi))
        for yy in range(y, y + 4):
            scene.put(x, yy, "stone", 0.6)
    scene.floor("stone", tiles=False, base=0.2)                           # Gravel path
    for y in range(HORIZON, 256):                                         # Grass verges
        depth = (y - HORIZON) / (256 - HORIZON)
        edge = 30 + 90 * depth
        for x in range(256):
            if abs(x - 128) > edge:
                scene.put(x, y, "leaf", 0.25 + 0.2 * depth + scene.random.uniform(-0.08, 0.08))
    return scene


def make_barracks():
    scene = Scene([("wood", R("leather")), ("stone", R("dark_steel", (1, 2, 3, 4))), ("steel", R("steel", (2, 3, 4))),
                   ("fire", R("fire", (2, 3, 4)))])
    for y in range(0, HORIZON):                                           # Timber wall
        for x in range(256):
            plank = (x // 14) % 2
            grain = 0.04 * math.sin(y * 0.5 + x * 3)
            seam = x % 14 == 0
            scene.put(x, y, "wood", 0.05 if seam else 0.3 + 0.08 * plank + grain - 0.15 * (1 - y / HORIZON))
    for y in (40, 100):                                                   # Cross beams
        for x in range(256):
            for yy in range(y, y + 6):
                scene.put(x, yy, "wood", 0.55 if yy == y else 0.35)
    for rack_x in (36, 186):                                              # Weapon racks with spears
        for x in range(rack_x, rack_x + 34):
            for y in (112, 113, 132, 133):
                scene.put(x, y, "wood", 0.6)
        for spear in range(rack_x + 4, rack_x + 32, 7):
            for y in range(72, 136):
                scene.put(spear, y, "wood", 0.45)
            for y in range(64, 72):
                scene.put(spear, y, "steel", 0.4 + 0.1 * (y - 64) / 8)
    scene.flame(128, 54)                                                  # Hanging lantern
    scene.floor("stone", base=0.3)
    return scene


def make_great_hall():
    scene = Scene([("stone", R("stone")), ("red", R("crimson", (1, 2, 3))), ("gold", R("gold", (3,))),
                   ("glass_a", R("night", (2, 4))), ("glass_b", R("crimson", (3, 4))), ("glass_c", R("gold", (4,)))])
    scene.bricks(0, 0, 256, HORIZON, "stone", width=20, height=10, base=0.3)
    for cx in (72, 184):
        scene.arch_window(cx, 52, 26, 56, ["glass_a", "glass_b", "glass_a", "glass_c"], "stone")
    for x in (20, 128, 236):
        scene.pillar(x, 0, HORIZON, 18, "stone", 0.5)
    for x in (42, 98, 150, 206):
        scene.banner(x, 50, 12, 44, "red", "gold")
    scene.floor("stone", checker=True, base=0.3)
    return scene


def make_throne_approach():
    scene = Scene([("stone", R("stone", (0, 1, 2, 3))), ("red", R("crimson", (1, 2, 3))), ("gold", R("gold", (2, 4))),
                   ("fire", R("fire", (2, 3, 4)))])
    scene.bricks(0, 0, 256, HORIZON, "stone", width=24, height=12, base=0.25)
    for x, width in ((14, 22), (60, 16), (196, 16), (242, 22)):          # Pillars receding to the door
        scene.pillar(x, 0, HORIZON, width, "stone", 0.55)
    for y in range(40, HORIZON):                                          # The doors ahead
        for x in range(110, 147):
            scene.put(x, y, "stone", 0.0 if (x - 110) % 18 else 0.15)
    for x in (92, 164):                                                   # Candelabras
        for y in range(96, HORIZON):
            scene.put(x, y, "gold", 0.3)
        for dx in (-6, 0, 6):
            scene.flame(x + dx, 92 - (0 if dx else 3), 0.6)
    scene.floor("stone", base=0.3)
    for y in range(HORIZON, 256):                                         # Red carpet to the throne
        depth = (y - HORIZON) / (256 - HORIZON)
        half = 18 + 60 * depth
        for x in range(int(128 - half), int(128 + half) + 1):
            scene.put(x, y, "red", 0.4 + 0.3 * depth - 0.2 * abs(x - 128) / half)
        scene.put(128 - half, y, "gold", 0.8)
        scene.put(128 + half, y, "gold", 0.8)
    return scene


def make_throne_room():
    scene = Scene([("wall", R("royal")), ("gold", R("gold", (2, 3, 4))), ("red", R("crimson", (1, 2, 3))),
                   ("glass_a", R("ice", (2, 4))), ("glass_b", R("royal", (4,)))])
    scene.bricks(0, 0, 256, HORIZON, "wall", width=24, height=12, base=0.3)
    scene.arch_window(128, 50, 40, 18, ["glass_a", "glass_b", "glass_a"], "wall")
    for x in range(98, 159):                                              # Throne back and dais
        for y in range(70, 132):
            scene.put(x, y, "wall", 0.08 if 104 < x < 152 else 0.3)
    for x in range(98, 159):
        scene.put(x, 70, "gold", 0.9)
    for spike in (104, 116, 128, 140, 152):                              # Gilded crest on the throne
        for y in range(58, 70):
            for dx in range(-((y - 58) // 4), (y - 58) // 4 + 1):
                scene.put(spike + dx, y, "gold", 0.6)
    for step in range(3):
        for y in range(132 + step * 4, 136 + step * 4):
            for x in range(80 - step * 10, 177 + step * 10):
                scene.put(x, y, "red" if y < 134 + step * 4 else "gold", 0.5)
    for x in (24, 232):
        scene.banner(x - 8, 50, 16, 64, "red", "gold")
    scene.floor("wall", base=0.28)
    return scene


def make_title():
    scene = Scene([("sky", R("night")), ("castle", R("outline", (0, 2, 4))), ("moon", R("ice", (3, 4))),
                   ("window", R("fire", (3, 4)))])
    scene.sky(0, 256, "sky", stars=70, moon=(176, 70, 16))
    # The castle silhouetted against the night: towers, battlements, lit windows.
    towers = [(40, 120, 18), (80, 96, 14), (128, 70, 22), (176, 100, 14), (214, 124, 18)]
    for x, top, half in towers:
        for y in range(top, 256):
            for xx in range(x - half, x + half + 1):
                scene.put(xx, y, "castle", 0.2)
        for y in range(top - 18, top):                                    # Spires
            width = int(half * (y - (top - 18)) / 18)
            for xx in range(x - width, x + width + 1):
                scene.put(xx, y, "castle", 0.2)
        for y in range(top - 10, 256):                                    # Moonlit edge (moon is to the right)
            scene.put(x + half, y, "castle", 0.6)
        for wy in range(top + 14, 220, 22):
            for wx in (x - half // 2, x + half // 2 - 1):
                for yy in range(wy, wy + 7):
                    for dx in range(2):
                        if yy == wy and dx == 1:
                            continue                                      # Rounded window top
                        scene.put(wx + dx, yy, "window", 0.5 if (yy + wx + dx) % 3 else 0.9)
    for y in range(150, 256):                                             # Curtain wall
        for x in range(256):
            if scene.image.get(x, y) in scene.ramps["sky"]:
                scene.put(x, y, "castle", 0.1)
    return scene


AREAS = [
    ("area_gate", make_gate),
    ("area_garden", make_garden),
    ("area_barracks", make_barracks),
    ("area_great_hall", make_great_hall),
    ("area_throne_approach", make_throne_approach),
    ("area_throne_room", make_throne_room),
    ("title", make_title),
]


def main():
    for name, maker in AREAS:
        maker().save(name)
    print("Wrote %d backgrounds to graphics/" % len(AREAS))

    if "--preview" in sys.argv:
        from PIL import Image as PilImage
        from preview_art import load
        output = sys.argv[sys.argv.index("--preview") + 1]
        sheet = PilImage.new("RGB", (240 * 4, 160 * 2))
        for index, (name, _) in enumerate(AREAS):
            image, _ = load(name)
            sheet.paste(image.crop((8, 48, 248, 208)), ((index % 4) * 240, (index // 4) * 160))
        sheet.save(output)
        print("Wrote", output)


if __name__ == "__main__":
    main()
