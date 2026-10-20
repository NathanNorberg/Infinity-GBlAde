"""Shared art style for Infinity GBlAde: color ramps and rules every art generator uses.

The style follows docs/Infinity Blade GBA Art Style Guide.md (GBA Castlevania-inspired: dark
values, muted midtones with saturated accents, cool shadows and warm highlights, tinted outlines).
These ramps are original; they were tuned against the reference screenshots' measurements, not
copied from them.

Each ramp runs darkest to lightest, 5 steps. Colors are snapped to the GBA's 15-bit palette
(5 bits per channel) when written, so values are kept on multiples of 8 here.
"""


def gba(hex_color):
    """'#rrggbb' -> (r, g, b) snapped to the GBA's 5-bit channels."""
    value = hex_color.lstrip("#")
    return tuple((int(value[i:i + 2], 16) // 8) * 8 for i in (0, 2, 4))


# Materials. Shadows drift cool (blue/purple), highlights drift warm (cream/yellow).
RAMPS = {
    "outline": ["#080810", "#100c18", "#181420", "#201828", "#282030"],   # Tinted near-blacks
    "stone": ["#181820", "#303840", "#485058", "#687070", "#98a090"],     # Cool grey, warm-grey highs
    "moss_stone": ["#102018", "#203028", "#384840", "#586860", "#889880"],
    "steel": ["#202838", "#384060", "#586888", "#90a0b8", "#e0e8e8"],     # Armor and blades
    "dark_steel": ["#101018", "#202030", "#383848", "#585868", "#888890"],
    "gold": ["#382810", "#684818", "#a07830", "#d8b058", "#f8e0a0"],
    "crimson": ["#280810", "#481020", "#781830", "#a83840", "#d87068"],   # Cloth, banners, blood
    "royal": ["#181020", "#302040", "#503060", "#805888", "#b890b8"],     # The God King's purple
    "leather": ["#281810", "#482818", "#684830", "#987048", "#c8a070"],
    "skin": ["#381818", "#683828", "#a06848", "#d09870", "#f0c8a0"],
    "foliage": ["#102010", "#183818", "#305828", "#508040", "#88b058"],
    "night": ["#080810", "#081430", "#182450", "#283870", "#405890"],
    "fire": ["#481008", "#982010", "#d85018", "#f89830", "#f8e080"],
    "ice": ["#102040", "#205080", "#4090c0", "#80c8e8", "#e0f8f8"],
}

# Rules the generators follow (also listed in the style guide).
LIGHT_DIRECTION = "top-left"          # Key light; the hero and enemy share it
ENEMY_RIM = "back-right, cool"        # A thin cool rim separates the enemy from the background
BACKGROUND_VALUE_SHIFT = -1           # Backgrounds use one ramp step darker than characters
MAX_COLORS = 16                       # Per image, with index 0 transparent


def ramp(name, steps=None):
    """A ramp as GBA colors; steps picks a subset by index, e.g. (0, 2, 4)."""
    colors = [gba(value) for value in RAMPS[name]]
    return colors if steps is None else [colors[index] for index in steps]


def palette(*parts):
    """Builds a 16-color palette: transparent magenta at index 0, then the given colors.

    parts are (ramp_name, steps) pairs or raw (r, g, b) colors. Raises if it overflows.
    """
    colors = [(255, 0, 255)]
    for part in parts:
        if isinstance(part, tuple) and len(part) == 2 and isinstance(part[0], str):
            colors.extend(ramp(part[0], part[1]))
        else:
            colors.append(part)
    if len(colors) > MAX_COLORS:
        raise ValueError("Palette has %d colors; the GBA allows %d per image" % (len(colors), MAX_COLORS))
    return colors
