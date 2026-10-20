#!/usr/bin/env python3
"""Draws the hero (graphics/hero.bmp): seen from behind, over the shoulder, in the style guide's
look (docs/Infinity Blade GBA Art Style Guide.md).

Run from the project folder:  python3 tools/make_hero_art.py [--preview out.png]

64x64 sprite, 5 frames stacked vertically:
  0 idle    sword raised on the right
  1 dodge   leaning right (the game flips it for a left dodge)
  2 duck    crouched low
  3 block   round shield raised on the left
  4 parry   sword swung up and across to the left (also the slash; flipped for right)

Light comes from the top left. Rounded parts are shaded from their surface direction; the cape
gets vertical folds; outlines are tinted, softer on the lit (top and left) edges.
"""

import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import art_style  # noqa: E402
from make_placeholder_art import Image  # noqa: E402

GRAPHICS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "graphics")
SIZE = 64
FRAMES = 5

# Palette: 16 colors from the style guide's ramps.
PALETTE = art_style.palette(
    ("outline", (0, 3)),            # 1 outline, 2 lit-edge outline / deepest shadow
    ("steel", (1, 2, 3, 4)),        # 3-6 armor and blade, dark to bright
    ("crimson", (1, 2, 3, 4)),      # 7-10 cape
    ("leather", (1, 2, 3)),         # 11-13 hair, grip, boots
    ("skin", (2,)),                 # 14 neck
    ("gold", (3,)),                 # 15 crossguard, shield rim
)

OUTLINE, OUTLINE_LIT = 1, 2
STEEL = (3, 4, 5, 6)
CRIMSON = (7, 8, 9, 10)
LEATHER = (11, 12, 13)
SKIN = 14
GOLD = 15

LIGHT = (-0.6, -0.8)                # Toward the light: up and to the left


def shade_index(ramp, value):
    """value 0..1 (dark to lit) -> a ramp color."""
    step = int(value * len(ramp))
    return ramp[max(0, min(len(ramp) - 1, step))]


class Canvas:
    """One 64x64 frame inside the sheet, with clipping and an optional lean (shear)."""

    def __init__(self, image, frame, lean=0.0, drop=0):
        self.image = image
        self.top = frame * SIZE
        self.lean = lean
        self.drop = drop

    def put(self, x, y, color):
        y += self.drop
        x = x + round(self.lean * (SIZE - 1 - y) / (SIZE - 1))
        if 0 <= x < SIZE and 0 <= y < SIZE:
            self.image.set(x, self.top + y, color)

    def get(self, x, y):
        if 0 <= x < SIZE and 0 <= y < SIZE:
            return self.image.get(x, self.top + y)
        return 0

    def round_part(self, cx, cy, rx, ry, ramp, bias=0.0):
        """An ellipse shaded like a rounded surface lit from the top left."""
        for y in range(int(cy - ry), int(cy + ry) + 1):
            for x in range(int(cx - rx), int(cx + rx) + 1):
                nx, ny = (x - cx) / rx, (y - cy) / ry
                if nx * nx + ny * ny > 1.0:
                    continue
                facing = -(nx * LIGHT[0] + ny * LIGHT[1])           # -1 away .. 1 toward light
                rim = nx * nx + ny * ny                              # Edges fall off
                value = 0.5 + 0.45 * facing - 0.25 * rim + bias
                self.put(x, y, shade_index(ramp, value))

    def limb(self, x0, y0, x1, y1, width, ramp, bias=0.0):
        """A tube from one point to another, lit across its width."""
        dx, dy = x1 - x0, y1 - y0
        length = max(1.0, math.hypot(dx, dy))
        ux, uy = dx / length, dy / length
        px, py = -uy, ux                                            # Across the tube
        if px * LIGHT[0] + py * LIGHT[1] < 0:
            px, py = -px, -py                                       # px,py points toward the light
        steps = int(length * 2) + 1
        half = width / 2
        for step in range(steps + 1):
            t = step / steps
            cx, cy = x0 + dx * t, y0 + dy * t
            for offset in range(-int(half * 2), int(half * 2) + 1):
                o = offset / 2
                value = 0.45 + 0.5 * (o / half) - 0.15 * abs(o / half) ** 2 + bias
                self.put(round(cx + px * o), round(cy + py * o), shade_index(ramp, value))

    def blade(self, x0, y0, x1, y1):
        """A sword blade: dark spine side, bright lit edge, steel core, a bright tip."""
        dx, dy = x1 - x0, y1 - y0
        length = max(1.0, math.hypot(dx, dy))
        ux, uy = dx / length, dy / length
        px, py = -uy, ux
        if px * LIGHT[0] + py * LIGHT[1] < 0:
            px, py = -px, -py
        for step in range(int(length * 2) + 1):
            t = step / (length * 2)
            cx, cy = x0 + dx * t, y0 + dy * t
            self.put(round(cx - px), round(cy - py), STEEL[1])     # Shadow side
            self.put(round(cx), round(cy), STEEL[2])               # Core
            self.put(round(cx + px), round(cy + py), STEEL[3])     # Lit edge
        self.put(round(x1), round(y1), STEEL[3])

    def hilt(self, hx, hy, ux, uy, guard=7, grip=6):
        """Crossguard across the blade direction (ux, uy) at the hand, grip below it."""
        px, py = -uy, ux
        for o in range(-guard, guard + 1):
            self.put(round(hx + px * o / 2), round(hy + py * o / 2), GOLD)
        for o in range(1, grip + 1):
            self.put(round(hx - ux * o), round(hy - uy * o), LEATHER[1] if o % 2 else LEATHER[0])

    def outline(self):
        """Tinted outline around everything; softer on the lit (top and left) side."""
        marks = []
        for y in range(SIZE):
            for x in range(SIZE):
                if self.get(x, y) != 0:
                    continue
                right, below = self.get(x + 1, y), self.get(x, y + 1)
                left, above = self.get(x - 1, y), self.get(x, y - 1)
                if right or below:
                    marks.append((x, y, OUTLINE_LIT if not (left or above) else OUTLINE))
                elif left or above:
                    marks.append((x, y, OUTLINE))
        for x, y, color in marks:
            self.image.set(x, self.top + y, color)


def draw_cape(canvas, top, bottom, left_top, right_top, left_bottom, right_bottom, bunched=False):
    """A cape hanging from the shoulders, widening, with a few broad folds lit from the left.

    Mostly dark (the style guide keeps bright values for focal points); fold crests catch the
    light, troughs and the right side fall into shadow."""
    fold_width = 8.0 if bunched else 11.0
    for y in range(top, bottom):
        t = (y - top) / max(1, bottom - top)
        x0 = left_top + (left_bottom - left_top) * t
        x1 = right_top + (right_bottom - right_top) * t
        width = max(1.0, x1 - x0)
        for x in range(int(x0), int(x1) + 1):
            across = (x - x0) / width                              # 0 left .. 1 right
            fold = math.sin(2 * math.pi * (x - x0) / fold_width + 0.6 + t * 0.8)
            value = 0.48 - 0.32 * across + 0.2 * fold
            if y < top + 3:
                value -= 0.2                                        # Shadow under the collar
            canvas.put(x, y, shade_index(CRIMSON, value))


def draw_head(canvas, cx, cy):
    """The back of the head: hair lit on the crown and the top left, darker toward the nape."""
    for y in range(cy - 7, cy + 8):
        for x in range(cx - 6, cx + 7):
            nx, ny = (x - cx) / 6, (y - cy) / 7
            if nx * nx + ny * ny > 1.0:
                continue
            facing = -(nx * LIGHT[0] + ny * LIGHT[1])
            value = 0.55 + 0.4 * facing - 0.35 * max(0.0, ny)       # Nape in shadow
            canvas.put(x, y, shade_index(LEATHER, value))
    # Two short swept strands at the sides, so it reads as hair, not a helmet or a face.
    for y in range(cy + 2, cy + 6):
        canvas.put(cx - 5 + (y - cy - 2) // 3, y, LEATHER[0])
        canvas.put(cx + 5 - (y - cy - 2) // 3, y, LEATHER[0])
    for x in range(cx - 1, cx + 2):
        canvas.put(x, cy + 7, SKIN)                                 # Nape of the neck


def draw_idle_like(canvas, sword=True, shield=False, parry=False, crouch=False):
    """A slim build: narrow shoulders, a cape that falls straight, arms close to the body."""
    head_y = 14 if not crouch else 22
    shoulder_y = 25 if not crouch else 32

    # Back arm (screen left): hanging close to the body, or raised with the shield.
    if shield:
        canvas.limb(23, shoulder_y + 2, 18, shoulder_y - 6, 5, STEEL)
    elif not parry:
        canvas.limb(22, shoulder_y + 2, 19, shoulder_y + 20, 5, STEEL, -0.2)
        canvas.round_part(19, shoulder_y + 21, 2.5, 2.5, LEATHER)

    # Sword arm (screen right), drawn before the cape so the cape overlaps its root.
    if parry:
        canvas.limb(40, shoulder_y + 2, 27, shoulder_y + 1, 5, STEEL)
        canvas.limb(22, shoulder_y + 3, 26, shoulder_y + 1, 5, STEEL)
    elif sword:
        canvas.limb(40, shoulder_y + 2, 45, shoulder_y + 14, 5, STEEL, -0.15)

    draw_cape(canvas, shoulder_y - 3, SIZE, 23, 39, 20, 43, bunched=crouch)

    # Pauldrons over the cape's shoulders.
    canvas.round_part(24, shoulder_y - 1, 5.5, 5, STEEL, -0.08)
    canvas.round_part(38, shoulder_y - 1, 5.5, 5, STEEL, -0.15)

    draw_head(canvas, 31, head_y)

    # The sword itself, in front of everything.
    if parry:
        canvas.round_part(26, shoulder_y + 1, 2.5, 2.5, LEATHER)
        canvas.blade(24, shoulder_y - 2, 8, 2)
        canvas.hilt(25, shoulder_y - 1, -0.62, -0.78)
    elif sword:
        canvas.round_part(45, shoulder_y + 15, 2.5, 2.5, LEATHER)
        canvas.blade(46, shoulder_y + 11, 56, 3 if not crouch else 10)
        canvas.hilt(46, shoulder_y + 13, 0.26, -0.97)

    # Round shield raised beside the head: gold rim, steel face, boss.
    if shield:
        canvas.round_part(18, shoulder_y - 8, 11, 12, (GOLD, GOLD, GOLD, GOLD))
        canvas.round_part(18, shoulder_y - 8, 8, 9, STEEL)
        canvas.round_part(17, shoulder_y - 9, 2, 2, (GOLD, GOLD, STEEL[3], STEEL[3]))


def make_hero():
    image = Image(SIZE, SIZE * FRAMES, PALETTE)

    frames = [
        (Canvas(image, 0), dict()),
        (Canvas(image, 1, lean=8), dict()),
        (Canvas(image, 2, drop=4), dict(crouch=True)),
        (Canvas(image, 3), dict(shield=True)),
        (Canvas(image, 4), dict(parry=True)),
    ]

    for canvas, options in frames:
        draw_idle_like(canvas, **options)
        canvas.outline()

    return image


def main():
    image = make_hero()
    image.save_bmp(os.path.join(GRAPHICS_DIR, "hero.bmp"))
    print("Wrote graphics/hero.bmp")

    if "--preview" in sys.argv:
        import subprocess
        output = sys.argv[sys.argv.index("--preview") + 1]
        tools = os.path.dirname(os.path.abspath(__file__))
        subprocess.run([sys.executable, os.path.join(tools, "preview_art.py"), "hero", "-o", output, "--scale", "4"],
                       check=True)


if __name__ == "__main__":
    main()
