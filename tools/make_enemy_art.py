#!/usr/bin/env python3
"""Draws the enemy knight's poses (graphics/enemy_*.bmp) and the type palette swaps, in the
style guide's look (docs/Infinity Blade GBA Art Style Guide.md).

Run from the project folder:
  python3 tools/make_enemy_art.py --preview-idle out.png   Idle pose only, as a mock duel (no files written)

Each pose is a 256x256 background with the figure centered on column 128, feet on row 184,
about 112 px tall. All poses share one 16-color palette layout so the Brute, Duelist and God
King can be palette swaps:
  1 outline   2-6 armor, darkest to highlight   7 visor glow   8-10 blade, spine to edge
  11-13 cloth (tabard)   14 leather   15 trim
Light comes from the top left; a thin cool rim lights his back-right edge.
"""

import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import art_style  # noqa: E402
from make_placeholder_art import Image  # noqa: E402

GRAPHICS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "graphics")

OUTLINE = 1
ARMOR = (2, 3, 4, 5, 6)
VISOR = 7
BLADE = (8, 9, 10)
CLOTH = (11, 12, 13)
LEATHER = 14
TRIM = 15

LIGHT = (-0.6, -0.8)
CX = 128                                   # Figure's center column
TURN = -2                                  # Front details shift toward the hero (screen left)

# The knight's own colors, in the shared layout.
KNIGHT_PALETTE = art_style.palette(
    ("outline", (1,)),
    ("steel", (0, 1, 2, 3, 4)),
    art_style.gba("#e83828"),              # Red visor glow
    ("dark_steel", (3,)),
    ("steel", (3,)),
    art_style.gba("#f0f8f8"),              # Blade edge: the brightest thing on him
    ("night", (2, 3, 4)),                  # Royal blue tabard
    ("leather", (2,)),
    ("gold", (3,)),
)


# Kits: each fighting style's own drawn look. Arms, hands and the sword's grip and tip stay at
# the same points in every kit, so the telegraphs stay tuned.
KITS = {
    "knight":   dict(leg=0, arm=0, torso=0, pauldron=1.0, spikes=False, helm="great",  cloth="tabard", blade=3),
    "brute":    dict(leg=3, arm=2, torso=3, pauldron=1.2, spikes=True,  helm="horned", cloth="apron", blade=3),
    "duelist":  dict(leg=-2, arm=-1, torso=-2, pauldron=0.8, spikes=False, helm="sallet", cloth="shoulder_cape", blade=3),
    "god_king": dict(leg=1, arm=1, torso=1, pauldron=1.15, spikes=False, helm="crown", cloth="royal_cape", blade=4),
}
KIT = KITS["knight"]


def use_kit(name):
    global KIT
    KIT = KITS[name]


def shade(ramp, value):
    return ramp[max(0, min(len(ramp) - 1, int(value * len(ramp))))]


class Figure:
    """Drawing helpers on one 256x256 pose."""

    def __init__(self, palette):
        self.image = Image(256, 256, palette)
        self.dy = 0                        # Shifts everything drawn (used to lower the body)

    def put(self, x, y, color):
        self.image.set(int(round(x)), int(round(y + self.dy)), color)

    def get(self, x, y):
        return self.image.get(int(round(x)), int(round(y + self.dy)))

    def round_part(self, cx, cy, rx, ry, ramp, bias=0.0):
        """Ellipse shaded as a rounded surface lit from the top left."""
        for y in range(int(cy - ry), int(cy + ry) + 1):
            for x in range(int(cx - rx), int(cx + rx) + 1):
                nx, ny = (x - cx) / rx, (y - cy) / ry
                if nx * nx + ny * ny > 1.0:
                    continue
                facing = -(nx * LIGHT[0] + ny * LIGHT[1])
                rim = nx * nx + ny * ny
                value = 0.5 + 0.42 * facing - 0.22 * rim + bias
                # The brightest step is a small glint only, where the surface faces the light.
                if not (facing > 0.75 and rim < 0.35):
                    value = min(value, 0.79)
                self.put(x, y, shade(ramp, value))

    def tube(self, x0, y0, x1, y1, w0, w1, ramp, bias=0.0):
        """A limb segment tapering from width w0 to w1, lit across its width."""
        dx, dy = x1 - x0, y1 - y0
        length = max(1.0, math.hypot(dx, dy))
        px, py = -dy / length, dx / length
        if px * LIGHT[0] + py * LIGHT[1] < 0:
            px, py = -px, -py
        for step in range(int(length * 2) + 1):
            t = step / (length * 2)
            cx, cy = x0 + dx * t, y0 + dy * t
            half = (w0 + (w1 - w0) * t) / 2
            for offset in range(-int(half * 2), int(half * 2) + 1):
                o = offset / 2 / max(0.5, half)
                value = 0.45 + 0.45 * o - 0.15 * o * o + bias
                self.put(cx + px * offset / 2, cy + py * offset / 2, shade(ramp, value))

    def panel(self, top, bottom, left_top, right_top, left_bottom, right_bottom, ramp, bias=0.0, rows=None):
        """A tapered body panel shaded like a cylinder lit from the left; optional row bands."""
        for y in range(int(top), int(bottom)):
            t = (y - top) / max(1, bottom - top)
            x0 = left_top + (left_bottom - left_top) * t
            x1 = right_top + (right_bottom - right_top) * t
            width = max(1.0, x1 - x0)
            for x in range(int(x0), int(x1) + 1):
                across = (x - x0) / width * 2 - 1                    # -1 left .. 1 right
                value = 0.62 - 0.38 * across - 0.22 * across * across - 0.12 * t + bias
                if rows and (y - int(top)) % rows == rows - 1:
                    value -= 0.35                                    # Gap between plates
                value = min(value, 0.79)                             # Flat panels never reach the glint
                self.put(x, y, shade(ramp, value))

    def blade(self, x0, y0, x1, y1, width=3):
        """Sword blade from hilt (x0, y0) to tip: spine, steel core, bright lit edge."""
        dx, dy = x1 - x0, y1 - y0
        length = max(1.0, math.hypot(dx, dy))
        ux, uy = dx / length, dy / length
        px, py = -uy, ux
        if px * LIGHT[0] + py * LIGHT[1] < 0:
            px, py = -px, -py
        for step in range(int(length * 2) + 1):
            t = step / (length * 2)
            cx, cy = x0 + dx * t, y0 + dy * t
            taper = 1 if t < 0.9 else 0
            self.put(cx - px * taper, cy - py * taper, BLADE[0])
            if width >= 3:
                self.put(cx + px * 0.0, cy + py * 0.0, BLADE[1])
            if width >= 4:
                self.put(cx - px * 2 * taper, cy - py * 2 * taper, BLADE[0])   # Broad greatsword
            self.put(cx + px * taper, cy + py * taper, BLADE[2])
        self.put(x1, y1, BLADE[2])

    def hilt(self, hx, hy, ux, uy, guard=8, grip=9):
        """Crossguard across the blade direction (ux, uy) at the hand; grip and pommel behind."""
        px, py = -uy, ux
        for o in range(-guard * 2, guard * 2 + 1):
            self.put(hx + px * o / 2, hy + py * o / 2, TRIM)
            self.put(hx + px * o / 2 - ux, hy + py * o / 2 - uy, TRIM)
        for o in range(2, grip):
            self.put(hx - ux * o, hy - uy * o, LEATHER)
            self.put(hx - ux * o + px, hy - uy * o + py, LEATHER)
        self.round_part(hx - ux * (grip + 1), hy - uy * (grip + 1), 2, 2, (TRIM, TRIM, ARMOR[3], ARMOR[4]))

    def finish(self):
        """Tinted outline around the figure, then a cool rim light on its back-right edge."""
        get = self.image.get
        marks = []
        for y in range(256):
            for x in range(256):
                if get(x, y) != 0:
                    continue
                if any(get(x + dx, y + dy) not in (0, OUTLINE) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                    marks.append((x, y))
        for x, y in marks:
            self.image.set(x, y, OUTLINE)

        rim = []
        for y in range(256):
            for x in range(1, 255):
                color = get(x, y)
                if color in ARMOR[:2] and get(x + 1, y) == OUTLINE and x > CX:
                    rim.append((x, y))
        for x, y in rim:
            self.image.set(x, y, ARMOR[3])


# ---------------------------------------------------------------------------------------------
# The knight's body. Arms and the sword come from the pose; everything else is shared.
# ---------------------------------------------------------------------------------------------

SHOULDER_L = (CX - 25, 104)
SHOULDER_R = (CX + 23, 104)


def elbow_between(shoulder, hand, bend):
    """An elbow between shoulder and hand, pushed sideways by bend (pixels, + = outward)."""
    mx, my = (shoulder[0] + hand[0]) / 2, (shoulder[1] + hand[1]) / 2
    dx, dy = hand[0] - shoulder[0], hand[1] - shoulder[1]
    length = max(1.0, math.hypot(dx, dy))
    px, py = dy / length, -dx / length
    return mx + px * bend, my + py * bend


def draw_foot(figure, ankle_x, side):
    """A pointed sabaton: plates over the instep, toe pointing forward and a little outward."""
    for row, y in enumerate(range(186, 194)):
        reach = 4 + row * 1.5                                              # Longer toward the toe
        back = 4
        for x in range(int(ankle_x - back if side > 0 else ankle_x - reach),
                       int(ankle_x + reach if side > 0 else ankle_x + back) + 1):
            across = (x - ankle_x) / max(1.0, reach)
            value = 0.55 - 0.3 * across * side - 0.06 * row + (0.12 if y == 189 else 0.0)
            figure.put(x, y, shade(ARMOR, min(value, 0.79)))


def draw_legs(figure):
    for side in (-1, 1):
        hip_x = CX + side * 11 + TURN // 2
        knee = (hip_x + side, 164)
        ankle = (hip_x + side * 2, 186)
        bias = -0.05 if side < 0 else -0.18
        leg = KIT["leg"]
        figure.tube(hip_x, 144, knee[0], knee[1], 15 + leg, 12 + leg, ARMOR, bias)
        figure.tube(knee[0], knee[1], ankle[0], ankle[1], 12 + leg, 9 + leg, ARMOR, bias)
        figure.round_part(knee[0], knee[1], 6.5 + leg / 2, 5, ARMOR, 0.05)
        draw_foot(figure, ankle[0], side)


def draw_torso(figure):
    w = KIT["torso"]

    # Armor skirt (faulds): two bands of plates widening downward, short so the legs read long.
    figure.panel(134, 151, CX - 17 - w, CX + 16 + w, CX - 23 - w, CX + 22 + w, ARMOR, -0.05, rows=8)

    # Breastplate: broad at the shoulders, tapering to the waist, a raised center ridge.
    figure.panel(96, 134, CX - 19 - w, CX + 18 + w, CX - 15 - w, CX + 14 + w, ARMOR, 0.05)
    for y in range(100, 130):
        figure.put(CX - 3 + TURN, y, ARMOR[3])
    for x in range(CX - 18 - w, CX + 17 + w):
        if figure.get(x, 113) in ARMOR:
            figure.put(x, 113, ARMOR[1])                                   # Under the chest plate

    cloth = KIT["cloth"]

    if cloth in ("tabard", "royal_cape"):
        # Tabard: a cloth banner hanging from the belt to the knees, trimmed, with a chevron crest.
        top, bottom = 136, 168
        for y in range(top, bottom):
            t = (y - top) / (bottom - top)
            half = 7 + 3 * t
            for x in range(int(CX + TURN - half), int(CX + TURN + half) + 1):
                across = (x - (CX + TURN)) / half
                fold = math.sin((x - CX) * 0.7 + t * 2.0)
                value = 0.6 - 0.35 * across + 0.15 * fold - 0.15 * t
                figure.put(x, y, shade(CLOTH, value))
            figure.put(CX + TURN - half, y, TRIM)
            figure.put(CX + TURN + half, y, TRIM)
        for offset in range(-5, 6):
            figure.put(CX + TURN + offset, 141 + abs(offset) // 1.5, TRIM)      # Chevron
    elif cloth == "apron":
        # Brute: a heavy apron the full width of the hips, riveted along the hem.
        top, bottom = 136, 164
        for y in range(top, bottom):
            t = (y - top) / (bottom - top)
            half = 16 + w + 3 * t
            for x in range(int(CX - half), int(CX + half) + 1):
                across = (x - CX) / half
                value = 0.55 - 0.35 * across - 0.1 * t + (0.1 if (x - CX) % 9 == 0 else 0.0)
                figure.put(x, y, shade(CLOTH, value))
        for x in range(CX - 18 - w, CX + 19 + w, 4):
            figure.put(x, bottom - 2, TRIM)                                     # Rivets
    else:
        # Duelist: a narrow sash hanging from the belt on the left hip.
        for y in range(136, 160):
            for x in range(CX - 12, CX - 6):
                figure.put(x + (y - 136) // 8, y, shade(CLOTH, 0.6 - 0.3 * ((x - CX + 12) / 6)))

    # Belt with a buckle.
    for y in range(132, 136):
        for x in range(CX - 16 - w, CX + 16 + w):
            figure.put(x, y, LEATHER)
    figure.round_part(CX + TURN, 134, 3, 2.5, (TRIM, TRIM, TRIM, ARMOR[4]))


def draw_back_cloth(figure):
    """Cloth behind the body, drawn first: the God King's long cape, or the Duelist's half-cape
    hanging from the left shoulder down the back to mid-thigh."""
    if KIT["cloth"] == "shoulder_cape":
        sx, sy = SHOULDER_L
        top, bottom = sy - 4, 160
        for y in range(top, bottom):
            t = (y - top) / (bottom - top)
            inner = CX - 6 - 2 * t                                      # Tucked behind the body
            outer = sx - 9 - 12 * math.sqrt(t)                          # Flaring out past the arm
            hem = bottom - 6 * (1 - (inner - (CX - 6)) / 8)             # Hem dips toward the outside
            for x in range(int(outer), int(inner) + 1):
                across = (x - outer) / max(1.0, inner - outer)
                if y > bottom - 7 + 6 * across:
                    continue                                            # Angled hem, lower outside
                fold = math.sin((x - outer) * 0.55 - t * 1.2)
                value = 0.25 + 0.35 * across + 0.2 * fold - 0.15 * t
                figure.put(x, y, shade(CLOTH, value))
            if y < bottom - 6:
                figure.put(int(outer), y, TRIM)                         # Gold edge down the outside
        return
    if KIT["cloth"] != "royal_cape":
        return
    for y in range(98, 196):
        t = (y - 98) / 98
        half = 26 + 22 * t
        for x in range(int(CX - half), int(CX + half) + 1):
            across = (x - CX) / half
            fold = math.sin((x - CX) * 0.35 + t * 1.5)
            value = 0.4 - 0.3 * across + 0.15 * fold - 0.15 * t
            figure.put(x, y, shade(CLOTH, value))
        if y > 190:
            for x in range(int(CX - half), int(CX + half) + 1, 3):
                figure.put(x, y, TRIM)                                          # Gold hem


def draw_front_cloth(figure):
    """Cloth over the shoulders, drawn after the pauldrons: the top of the Duelist's half-cape,
    draped over the left pauldron and pinned at the collar with a gold clasp."""
    if KIT["cloth"] != "shoulder_cape":
        return
    sx, sy = SHOULDER_L
    cx, cy, rx, ry = sx + 3, sy - 2, 12, 7
    for y in range(int(cy - ry), int(cy + ry) + 1):
        for x in range(int(cx - rx), int(cx + rx) + 1):
            nx, ny = (x - cx) / rx, (y - cy) / ry
            if nx * nx + ny * ny > 1 or ny > 0.35 - 0.6 * nx:
                continue                                                # Lower edge slopes up toward the collar
            fold = math.sin(x * 0.9 - y * 0.3)
            value = 0.62 - 0.35 * ny - 0.2 * nx + 0.1 * fold
            figure.put(x, y, shade(CLOTH, value))
            if nx * nx + ny * ny > 0.75 and nx < 0:
                figure.put(x, y, TRIM)                                  # Gold edge round the shoulder
    figure.round_part(cx + 8, cy, 2.5, 2.5, (TRIM, TRIM, TRIM, ARMOR[4]))   # Clasp


def draw_head(figure):
    helm = KIT["helm"]

    # Gorget, then the helm: a rounded great helm (most kits) or a sallet (the Duelist).
    figure.round_part(CX - 1, 96, 10, 4.5, ARMOR, -0.1)

    if helm == "sallet":
        # Rounded skull dropping into a tail that flares behind (screen right), plumed.
        figure.round_part(CX - 1, 84, 11, 10, ARMOR, 0.05)
        figure.tube(CX + 6, 86, CX + 17, 93, 7, 3, ARMOR, -0.2)                 # Tail
        for x in range(CX - 10 + TURN, CX + 7 + TURN):
            figure.put(x, 86, ARMOR[0])
        figure.put(CX - 5 + TURN, 86, VISOR)
        figure.put(CX + 2 + TURN, 86, VISOR)
        for step in range(14):                                                  # Plume sweeping back
            px, py = CX + step, 74 - int(6 * math.sin(step / 14 * math.pi))
            for dy in range(3):
                figure.put(px, py + dy, CLOTH[2 - min(2, dy)])
        return

    figure.round_part(CX - 1, 80, 11, 10, ARMOR, 0.05)
    figure.panel(80, 94, CX - 11, CX + 10, CX - 10, CX + 9, ARMOR, 0.0)
    for x in range(CX - 9 + TURN, CX + 7 + TURN):
        figure.put(x, 84, ARMOR[0])
        figure.put(x, 85, ARMOR[0])
    for x in (CX - 5, CX - 4, CX + 2, CX + 3):
        figure.put(x + TURN, 84, VISOR)
    for y in (88, 90):
        for x in (CX + 3 + TURN, CX + 5 + TURN):
            figure.put(x, y, ARMOR[0])                                          # Breath holes

    if helm == "great":
        for y in range(69, 80):
            figure.put(CX - 1 + TURN, y, TRIM)                                  # Crest ridge
            figure.put(CX + TURN, y, ARMOR[4] if y < 74 else TRIM)
    elif helm == "horned":
        for side in (-1, 1):                                                    # Curving horns
            figure.tube(CX + side * 10, 78, CX + side * 19, 70, 5, 4, (TRIM, TRIM, ARMOR[3], ARMOR[4]))
            figure.tube(CX + side * 19, 70, CX + side * 17, 60, 4, 2, (TRIM, TRIM, ARMOR[3], ARMOR[4]))
    elif helm == "crown":
        for x in range(CX - 11, CX + 11):                                       # Crown band
            figure.put(x, 72, TRIM)
            figure.put(x, 73, TRIM)
        for spike_x in (CX - 9, CX - 4, CX + 1, CX + 6):
            for y in range(63, 72):
                width = (y - 63) // 3
                for dx in range(-width, width + 1):
                    figure.put(spike_x + dx, y, TRIM)
            figure.put(spike_x, 63, ARMOR[4])                                   # Glint on each point


def draw_open_hand(figure, x, y, side, bias):
    """A gauntlet hanging at the side, seen edge-on with the palm toward the thigh: the cuff,
    the hand in profile, the index finger curling in at the tip and the thumb along the front.
    side is -1 for the hand on screen left (palm faces right), 1 for screen right."""
    inward = -side                                                            # Toward the body
    figure.tube(x, y - 6, x, y - 1, 8, 9, ARMOR, bias - 0.05)                 # Cuff
    for fy in range(y - 1, y + 9):
        t = (fy - (y - 1)) / 10
        width = 5 if t < 0.5 else 4                                           # Hand narrows to the fingers
        curl = 1 if t > 0.75 else 0                                           # Fingertips curl in
        left = x - width // 2 + inward * curl
        for fx in range(left, left + width):
            across = (fx - left) / max(1, width - 1)
            lit = 0.66 + bias - 0.4 * across - 0.12 * t
            figure.put(fx, fy, shade(ARMOR, min(lit, 0.79)))
        if fy in (y + 3, y + 6):
            figure.put(left + (width - 1 if inward > 0 else 0), fy, ARMOR[1])  # Finger joints
    for fy in range(y, y + 4):
        figure.put(x + inward * 3, fy, shade(ARMOR, 0.5 + bias))              # Thumb along the front
    figure.put(x + inward * 3, y + 4, ARMOR[1])


def draw_fist(figure, x, y, bias):
    """A gauntleted fist closed around a vertical grip: knuckle ridge and finger rows."""
    x, y = int(round(x)), int(round(y))
    figure.round_part(x, y, 5, 4.5, ARMOR, bias)
    for knuckle in range(4):
        figure.put(x - 3 + knuckle * 2, y - 2, shade(ARMOR, 0.7 + bias))
    for row in (y, y + 2):
        for fx in range(x - 4, x + 4):
            if figure.get(fx, row) in ARMOR:
                figure.put(fx, row, ARMOR[1])                                 # Gaps between fingers


def draw_arm(figure, shoulder, hand, bend, bias, open_hand=True):
    elbow = elbow_between(shoulder, hand, bend)
    arm = KIT["arm"]
    figure.tube(shoulder[0], shoulder[1], elbow[0], elbow[1], 10 + arm, 8 + arm, ARMOR, bias)
    figure.tube(elbow[0], elbow[1], hand[0], hand[1] - 6, 8 + arm, 7 + arm, ARMOR, bias)
    figure.round_part(elbow[0], elbow[1], 4.5, 4, ARMOR, bias + 0.05)        # Couter
    if open_hand:
        draw_open_hand(figure, hand[0], hand[1], -1 if hand[0] < CX else 1, bias)


def draw_pauldrons(figure):
    k = KIT["pauldron"]
    for (x, y), bias in ((SHOULDER_L, 0.05), (SHOULDER_R, -0.15)):
        figure.round_part(x, y + 4, 8.5 * k, 5 * k, ARMOR, bias - 0.1)       # Lower lame
        figure.round_part(x, y - 2, 10 * k, 7.5 * k, ARMOR, bias)
        rx, ry = 10.0 * k, 7.5 * k
        for dx in range(-int(rx * 0.7), int(rx * 0.7) + 1):
            ty = y - 2 + int(round(ry * math.sqrt(max(0.0, 1 - (dx / rx) ** 2)))) - 1
            figure.put(x + dx, ty, TRIM)                                    # Trim edge
        if KIT["spikes"]:
            for dx in (-6, 0, 6):
                base = y - 2 - int(ry * math.sqrt(max(0.0, 1 - (dx / rx) ** 2)))
                for h in range(6):
                    figure.put(x + dx, base - h, TRIM if h < 4 else ARMOR[4])
                    if h < 3:
                        figure.put(x + dx + 1, base - h, TRIM)


def draw_sword_in_fist(figure, fist_x, fist_y, tip_y, bias):
    """Upright sword: blade above the crossguard, the fist around the grip, grip and pommel below."""
    guard_y = fist_y - 6
    figure.blade(fist_x, guard_y - 1, fist_x, tip_y, KIT["blade"])
    for o in range(-8, 9):
        figure.put(fist_x + o, guard_y, TRIM)
        figure.put(fist_x + o, guard_y + 1, TRIM)
    for gy in range(guard_y + 2, fist_y + 9):
        figure.put(fist_x, gy, LEATHER)
        figure.put(fist_x + 1, gy, LEATHER)
    figure.round_part(fist_x + 0.5, fist_y + 10, 2.5, 2, (TRIM, TRIM, ARMOR[3], ARMOR[4]))   # Pommel
    draw_fist(figure, fist_x, fist_y, bias)


def draw_idle(figure):
    draw_back_cloth(figure)
    draw_legs(figure)
    draw_torso(figure)
    draw_arm(figure, SHOULDER_L, (CX - 31, 144), -6, 0.0)
    draw_arm(figure, SHOULDER_R, (CX + 31, 144), 8, -0.15, open_hand=False)
    draw_pauldrons(figure)
    draw_front_cloth(figure)
    draw_head(figure)
    # Sword held upright on his left (screen right), the hilt running past his fist.
    draw_sword_in_fist(figure, CX + 31, 144, 62, -0.15)
    figure.finish()


# ---------------------------------------------------------------------------------------------
# Attack poses. Hand (grip) and tip positions come from the placeholder pose table, so every
# swing still lines up with its telegraph trail (src/attack_data.h).
# ---------------------------------------------------------------------------------------------

from make_placeholder_art import ENEMY_SWORD_POSES  # noqa: E402

ARM_REACH = 54                                      # Longest shoulder-to-hand reach


def to_image(point):
    return CX + point[0], 128 + point[1]


def draw_sword(figure, hand, tip):
    """A sword gripped at hand, pointing at tip: blade, crossguard, grip and pommel behind."""
    dx, dy = tip[0] - hand[0], tip[1] - hand[1]
    length = max(1.0, math.hypot(dx, dy))
    ux, uy = dx / length, dy / length
    guard = (hand[0] + ux * 5, hand[1] + uy * 5)
    figure.blade(guard[0] + ux, guard[1] + uy, tip[0], tip[1], KIT["blade"])
    px, py = -uy, ux
    for o in range(-16, 17):
        figure.put(guard[0] + px * o / 2, guard[1] + py * o / 2, TRIM)
        figure.put(guard[0] + px * o / 2 - ux, guard[1] + py * o / 2 - uy, TRIM)
    for o in range(-9, 5):
        figure.put(hand[0] + ux * o, hand[1] + uy * o, LEATHER)
        figure.put(hand[0] + ux * o + px, hand[1] + uy * o + py, LEATHER)
    figure.round_part(hand[0] - ux * 11, hand[1] - uy * 11, 2.5, 2, (TRIM, TRIM, ARMOR[3], ARMOR[4]))
    return ux, uy


def draw_attack_pose(figure, hand_screen, tip_screen):
    hand = to_image(hand_screen)
    tip = to_image(tip_screen)
    dx, dy = tip[0] - hand[0], tip[1] - hand[1]
    length = max(1.0, math.hypot(dx, dy))
    ux, uy = dx / length, dy / length
    grips = (hand, (hand[0] - ux * 7, hand[1] - uy * 7))       # Leading and trailing hand

    draw_back_cloth(figure)
    draw_legs(figure)
    draw_torso(figure)
    draw_head(figure)

    fists = []
    for shoulder, bias in ((SHOULDER_L, 0.0), (SHOULDER_R, -0.15)):
        # Each arm takes the nearer grip; if neither is in reach it swings free to its own side.
        grip = min(grips, key=lambda g: math.hypot(g[0] - shoulder[0], g[1] - shoulder[1]))
        reach = math.hypot(grip[0] - shoulder[0], grip[1] - shoulder[1])
        outward = -1 if shoulder[0] < CX else 1
        if reach <= ARM_REACH:
            bends = (8.0, -8.0)
            elbow_options = [elbow_between(shoulder, grip, bend) for bend in bends]
            best = max(range(2), key=lambda i: abs(elbow_options[i][0] - CX) + elbow_options[i][1] * 0.2)
            draw_arm(figure, shoulder, grip, bends[best], bias, open_hand=False)
            fists.append((grip, bias))
        else:
            free_hand = (shoulder[0] + outward * 12, shoulder[1] + 34)
            draw_arm(figure, shoulder, free_hand, -6 * outward, bias, open_hand=True)

    draw_pauldrons(figure)
    draw_front_cloth(figure)
    draw_sword(figure, hand, tip)
    for grip, bias in fists:
        draw_fist(figure, grip[0], grip[1], bias)
    figure.finish()


# ---------------------------------------------------------------------------------------------
# Defeat: a stagger, then kneeling on the planted sword (the game fades him out after).
# ---------------------------------------------------------------------------------------------

def shift_down(figure, rows, lean=0):
    """Moves everything drawn so far down (and leans it right toward the top)."""
    old = [row[:] for row in figure.image.pixels]
    for y in range(256):
        for x in range(256):
            figure.image.pixels[y][x] = 0
    for y in range(256):
        for x in range(256):
            color = old[y][x]
            if color:
                ny = y + rows
                nx = x + round(lean * (194 - y) / 120)
                if 0 <= ny < 256 and 0 <= nx < 256:
                    figure.image.pixels[ny][nx] = color


def draw_fall_stagger(figure):
    # Reeling back: the whole figure leans, the sword droops to point at the floor in front.
    draw_back_cloth(figure)
    draw_legs(figure)
    draw_torso(figure)
    draw_head(figure)
    draw_arm(figure, SHOULDER_L, (CX - 30, 146), -6, 0.0)
    draw_arm(figure, SHOULDER_R, (CX + 26, 146), 8, -0.15, open_hand=False)
    draw_pauldrons(figure)
    draw_front_cloth(figure)
    draw_sword(figure, (CX + 26, 146), (CX + 44, 196))
    draw_fist(figure, CX + 26, 146, -0.15)
    shift_down(figure, 4, lean=6)
    figure.finish()


def draw_fall_kneel(figure):
    # Down on one knee, both hands on the pommel of the sword planted point-first, head bowed.
    drop = 20
    figure.dy = drop
    draw_back_cloth(figure)
    figure.dy = 0
    for side in (-1, 1):
        hip = (CX + side * 11, 144 + drop)
        if side < 0:
            knee = (hip[0] - 4, 188)                                    # Kneeling knee on the floor
            figure.tube(hip[0], hip[1], knee[0], knee[1], 15, 12, ARMOR, -0.08)
            figure.tube(knee[0], knee[1], knee[0] + 14, 190, 11, 8, ARMOR, -0.25)   # Shin back along the floor
        else:
            knee = (hip[0] + 8, 172)                                    # Other knee up
            figure.tube(hip[0], hip[1], knee[0], knee[1], 15, 12, ARMOR, -0.15)
            figure.tube(knee[0], knee[1], knee[0] + 1, 186, 12, 9, ARMOR, -0.18)
            draw_foot(figure, knee[0] + 1, 1)
        figure.round_part(knee[0], knee[1], 6.5, 5, ARMOR, 0.05)

    figure.dy = drop
    draw_torso(figure)
    figure.dy = drop + 3
    draw_head(figure)                                                   # Head bowed
    figure.dy = drop
    sword_x = CX + 2
    draw_arm(figure, SHOULDER_L, (sword_x - 4, 132), 7, 0.0, open_hand=False)
    draw_arm(figure, SHOULDER_R, (sword_x + 5, 133), -7, -0.15, open_hand=False)
    draw_pauldrons(figure)
    draw_front_cloth(figure)
    figure.dy = 0
    draw_sword(figure, (sword_x, 153), (sword_x, 194))
    draw_fist(figure, sword_x - 3, 151, 0.0)
    draw_fist(figure, sword_x + 4, 153, -0.15)
    figure.finish()


# ---------------------------------------------------------------------------------------------
# Type palettes: the same 16-slot layout recolored (graphics/enemy_palette_*.bmp).
# ---------------------------------------------------------------------------------------------

def type_palette(armor, visor, blade, cloth, leather, trim):
    return art_style.palette(("outline", (1,)), *armor, art_style.gba(visor), *blade, *cloth,
                             art_style.gba(leather), art_style.gba(trim))


TYPE_PALETTES = {
    "brute": type_palette(
        [art_style.gba(c) for c in ("#201410", "#403020", "#685038", "#988058", "#c8b088")],   # Bronze
        "#f87818",
        [art_style.gba(c) for c in ("#484850", "#909098", "#d8d8d8")],                         # Dull iron
        [art_style.gba(c) for c in ("#382010", "#583820", "#805838")],                         # Brown leather
        "#301810", "#808088"),
    "duelist": type_palette(
        [art_style.gba(c) for c in ("#081020", "#182850", "#284078", "#4868a8", "#98b0d8")],   # Navy
        "#40d8f8",
        [art_style.gba(c) for c in ("#586878", "#a8b8c8", "#f8f8f8")],
        art_style.ramp("crimson", (1, 2, 3)),
        "#382030", "#e0b848"),
    "god_king": type_palette(
        art_style.ramp("royal"),                                                               # Royal purple
        "#f8f8f8",
        art_style.ramp("gold", (2, 3, 4)),                                                     # Gilded blade
        art_style.ramp("crimson", (1, 2, 3)),
        "#301018", "#f8c838"),
}


# ---------------------------------------------------------------------------------------------
# Named enemies: each castle area's enemies, as a kit (fighting style's look) plus colors.
# The order here is the look's id in the game (src/enemy_looks.h is generated from it).
# ---------------------------------------------------------------------------------------------

def ramp_of(*hex_colors):
    return [art_style.gba(c) for c in hex_colors]


ARMOR_RAMPS = {
    "steel": art_style.ramp("steel"),
    "dark_iron": art_style.ramp("dark_steel"),
    "bronze": ramp_of("#201410", "#403020", "#685038", "#988058", "#c8b088"),
    "navy": ramp_of("#081020", "#182850", "#284078", "#4868a8", "#98b0d8"),
    "verdigris": ramp_of("#102018", "#204038", "#3a6458", "#68988a", "#b0d0c0"),
    "gilded": ramp_of("#302818", "#584830", "#887850", "#c0b080", "#f0e8c0"),
    "black_iron": ramp_of("#08080c", "#181820", "#2c2c38", "#484858", "#787888"),
}
CLOTH_RAMPS = {
    "blue": art_style.ramp("night", (2, 3, 4)),
    "moss": art_style.ramp("foliage", (1, 2, 3)),
    "leather": ramp_of("#382010", "#583820", "#805838"),
    "crimson": art_style.ramp("crimson", (1, 2, 3)),
    "royal": art_style.ramp("royal", (1, 2, 3)),
    "rose": ramp_of("#480820", "#801838", "#c04060"),
}
BLADE_RAMPS = {
    "steel": [art_style.ramp("dark_steel")[3], art_style.ramp("steel")[3], art_style.gba("#f0f8f8")],
    "iron": ramp_of("#484850", "#909098", "#d8d8d8"),
    "gilded": art_style.ramp("gold", (2, 3, 4)),
}


def look_palette(armor, cloth, visor, trim, blade="steel", leather="#684830"):
    return art_style.palette(("outline", (1,)), *ARMOR_RAMPS[armor], art_style.gba(visor), *BLADE_RAMPS[blade],
                             *CLOTH_RAMPS[cloth], art_style.gba(leather), art_style.gba(trim))


GOLD, IRON = "#d8b058", "#808088"

# id, display name, kit, palette
LOOKS = [
    ("gate_sentry", "Gate Sentry", "knight", look_palette("steel", "moss", "#e83828", IRON)),
    ("gate_skirmisher", "Gate Skirmisher", "duelist", look_palette("dark_iron", "leather", "#40d8f8", IRON)),
    ("garden_warden", "Garden Warden", "knight", look_palette("verdigris", "moss", "#f8c040", GOLD)),
    ("thorn_brute", "Thorn Brute", "brute", look_palette("bronze", "moss", "#f87818", IRON, "iron", "#301810")),
    ("rose_duelist", "Rose Duelist", "duelist", look_palette("navy", "rose", "#f878b8", GOLD)),
    ("barracks_sergeant", "Barracks Sergeant", "knight", look_palette("dark_iron", "leather", "#e83828", IRON)),
    ("barracks_brute", "Barracks Brute", "brute", look_palette("dark_iron", "leather", "#f87818", IRON, "iron", "#301810")),
    ("barracks_fencer", "Barracks Fencer", "duelist", look_palette("steel", "crimson", "#40d8f8", IRON)),
    ("hall_captain", "Hall Captain", "knight", look_palette("steel", "crimson", "#e83828", GOLD)),
    ("hall_brute", "Hall Brute", "brute", look_palette("bronze", "crimson", "#f87818", GOLD, "iron", "#301810")),
    ("hall_blademaster", "Hall Blademaster", "duelist", look_palette("navy", "royal", "#40d8f8", GOLD)),
    ("throne_guard", "Throne Guard", "knight", look_palette("gilded", "royal", "#d8f0f8", GOLD, "gilded")),
    ("royal_executioner", "Royal Executioner", "brute", look_palette("black_iron", "crimson", "#e83828", IRON, "iron", "#200810")),
    ("god_king", "The God King", "god_king", TYPE_PALETTES["god_king"]),
]

# Each kit's own default colors (used when a duel has no named enemy, e.g. debug duels).
KIT_PALETTES = {
    "knight": KNIGHT_PALETTE,
    "brute": TYPE_PALETTES["brute"],
    "duelist": TYPE_PALETTES["duelist"],
    "god_king": TYPE_PALETTES["god_king"],
}
KIT_ORDER = ["knight", "brute", "duelist", "god_king"]     # Matches enemy_type::palette (kit) in tuning.h

POSE_NAMES = ["idle"] + [name for name, _, _ in ENEMY_SWORD_POSES] + ["fall_stagger", "fall_kneel"]


def make_kit(kit):
    """All poses for one kit, in its default colors: [(pose name, figure)]."""
    use_kit(kit)
    palette = KIT_PALETTES[kit]
    poses = []

    figure = Figure(palette)
    draw_idle(figure)
    poses.append(("idle", figure))

    for name, hand, tip in ENEMY_SWORD_POSES:
        figure = Figure(palette)
        draw_attack_pose(figure, hand, tip)
        poses.append((name, figure))

    for name, draw in (("fall_stagger", draw_fall_stagger), ("fall_kneel", draw_fall_kneel)):
        figure = Figure(palette)
        draw(figure)
        poses.append((name, figure))

    use_kit("knight")
    return poses


ATTACK_ORDER = ["slash_l", "slash_r", "horiz_l", "horiz_r", "overhead", "sweep", "thrust", "rise_l", "rise_r"]


def write_headers():
    """src/enemy_looks.h (names and kits, no Butano) and src/enemy_pose_tables.h (asset tables)."""
    src = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src")

    lines = ["#ifndef IGB_ENEMY_LOOKS_H", "#define IGB_ENEMY_LOOKS_H", "",
             "// Generated by tools/make_enemy_art.py: do not edit by hand.", "",
             "namespace igb::look", "{", "",
             "// Named enemies: a display name and the kit (fighting style's drawn look) they wear.",
             "struct named_enemy", "{", "    const char* name;", "    int kit;", "};", "",
             "constexpr int kit_count = %d;" % len(KIT_ORDER), "",
             "inline constexpr named_enemy looks[] = {"]
    for look_id, name, kit, _ in LOOKS:
        lines.append('    { "%s", %d },' % (name, KIT_ORDER.index(kit)))
    lines += ["};", "", "constexpr int look_count = %d;" % len(LOOKS), ""]
    for index, (look_id, _, _, _) in enumerate(LOOKS):
        lines.append("constexpr int %s = %d;" % (look_id, index))
    lines += ["", "}", "", "#endif", ""]
    with open(os.path.join(src, "enemy_looks.h"), "w") as header:
        header.write("\n".join(lines))

    lines = ["#ifndef IGB_ENEMY_POSE_TABLES_H", "#define IGB_ENEMY_POSE_TABLES_H", "",
             "// Generated by tools/make_enemy_art.py: do not edit by hand.", ""]
    for kit in KIT_ORDER:
        for pose in POSE_NAMES:
            lines.append('#include "bn_regular_bg_items_enemy_%s_%s.h"' % (kit, pose))
    for look_id, _, _, _ in LOOKS:
        lines.append('#include "bn_bg_palette_items_enemy_look_%s.h"' % look_id)
    lines += ["", "namespace igb::enemy_art", "{", ""]

    def item(kit, pose):
        return "&bn::regular_bg_items::enemy_%s_%s" % (kit, pose)

    for table, prefix in (("idle_poses", None), ("stagger_poses", "fall_stagger"), ("kneel_poses", "fall_kneel")):
        lines.append("// Indexed by kit.")
        lines.append("constexpr const bn::regular_bg_item* %s[] = {" % table)
        for kit in KIT_ORDER:
            lines.append("    %s," % item(kit, prefix or "idle"))
        lines += ["};", ""]
    for table, prefix in (("windup_poses", "wu"), ("strike_poses", "st")):
        lines.append("// Indexed by kit, then attack_id.")
        lines.append("constexpr const bn::regular_bg_item* %s[][%d] = {" % (table, len(ATTACK_ORDER)))
        for kit in KIT_ORDER:
            lines.append("    { " + ", ".join(item(kit, "%s_%s" % (prefix, attack)) for attack in ATTACK_ORDER) + " },")
        lines += ["};", ""]
    lines.append("// Indexed by look id (src/enemy_looks.h).")
    lines.append("constexpr const bn::bg_palette_item* look_palettes[] = {")
    for look_id, _, _, _ in LOOKS:
        lines.append("    &bn::bg_palette_items::enemy_look_%s," % look_id)
    lines += ["};", "", "}", "", "#endif", ""]
    with open(os.path.join(src, "enemy_pose_tables.h"), "w") as header:
        header.write("\n".join(lines))


def main():
    if "--preview-idle" in sys.argv:
        output = sys.argv[sys.argv.index("--preview-idle") + 1]
        import tempfile
        from preview_art import duel_mock

        figure = Figure(KNIGHT_PALETTE)
        draw_idle(figure)
        with tempfile.TemporaryDirectory() as folder:
            path = os.path.join(folder, "enemy_idle.bmp")
            figure.image.save_bmp(path)
            screen = duel_mock("area_great_hall", path)
            screen.resize((screen.size[0] * 3, screen.size[1] * 3), 0).save(output)
            close = screen.crop((60, 8, 200, 152))
            close.resize((close.size[0] * 4, close.size[1] * 4), 0).save(output.replace(".png", "_close.png"))
        print("Wrote", output)
        return

    # Remove art from the old single-kit layout (enemy_<pose>.bmp, enemy_palette_*).
    for file_name in os.listdir(GRAPHICS_DIR):
        stem = file_name.rsplit(".", 1)[0]
        if file_name.startswith("enemy_") and not any(stem.startswith("enemy_" + kit + "_") for kit in KIT_ORDER) \
                and not stem.startswith("enemy_look_"):
            os.remove(os.path.join(GRAPHICS_DIR, file_name))

    bg_json = '{\n    "type": "regular_bg",\n    "bpp_mode": "bpp_4"\n}\n'
    palette_json = '{\n    "type": "bg_palette",\n    "bpp_mode": "bpp_4"\n}\n'

    for kit in KIT_ORDER:
        for name, figure in make_kit(kit):
            stem = os.path.join(GRAPHICS_DIR, "enemy_%s_%s" % (kit, name))
            figure.image.save_bmp(stem + ".bmp")
            with open(stem + ".json", "w") as json_file:
                json_file.write(bg_json)

    for look_id, _, _, colors in LOOKS:
        stem = os.path.join(GRAPHICS_DIR, "enemy_look_" + look_id)
        Image(8, 8, colors).save_bmp(stem + ".bmp")
        with open(stem + ".json", "w") as json_file:
            json_file.write(palette_json)

    write_headers()
    print("Wrote %d kits x %d poses, %d named looks and the pose tables" % (len(KIT_ORDER), len(POSE_NAMES), len(LOOKS)))


if __name__ == "__main__":
    main()
