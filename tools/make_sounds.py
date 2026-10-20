#!/usr/bin/env python3
"""Generates the game's sound effects as 8-bit mono WAVs in audio/ (version 1 has no music).

Run from the project folder:  python3 tools/make_sounds.py

Every effect is synthesized from tones and noise, so it can be tweaked here and regenerated.
The random generator is seeded, so the output is the same on every run.
"""

import math
import os
import random
import wave

AUDIO_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "audio")
RATE = 16000

random.seed(1987)


def silence(seconds):
    return [0.0] * int(RATE * seconds)


def envelope(samples, attack, decay_power=1.0):
    """Short linear attack, then a decay to silence (decay_power > 1 dies away faster)."""
    count = len(samples)
    attack_samples = max(1, int(RATE * attack))
    for index in range(count):
        if index < attack_samples:
            gain = index / attack_samples
        else:
            gain = (1 - (index - attack_samples) / max(1, count - attack_samples)) ** decay_power
        samples[index] *= gain
    return samples


def tone(frequency, seconds, shape="sine", slide_to=None):
    """A tone, optionally sliding to another frequency. Shapes: sine, square, triangle."""
    count = int(RATE * seconds)
    phase = 0.0
    out = []
    for index in range(count):
        freq = frequency if slide_to is None else frequency + (slide_to - frequency) * index / count
        phase += freq / RATE
        cycle = phase % 1.0
        if shape == "square":
            value = 1.0 if cycle < 0.5 else -1.0
        elif shape == "triangle":
            value = 4 * abs(cycle - 0.5) - 1
        else:
            value = math.sin(2 * math.pi * phase)
        out.append(value)
    return out


def noise(seconds, smoothing=0.0):
    """White noise; smoothing (0..0.95) darkens it like a low-pass filter."""
    out = []
    last = 0.0
    for _ in range(int(RATE * seconds)):
        last = last * smoothing + random.uniform(-1, 1) * (1 - smoothing)
        out.append(last)
    return out


def mix(*layers):
    length = max(len(layer) for layer, _ in layers)
    out = [0.0] * length
    for layer, gain in layers:
        for index, value in enumerate(layer):
            out[index] += value * gain
    return out


def concat(*parts):
    out = []
    for part in parts:
        out.extend(part)
    return out


def bell(partials, seconds, decay_power=2.5):
    """Inharmonic partials (frequency, gain) for metallic sounds."""
    layers = [(envelope(tone(freq, seconds), 0.001, decay_power), gain) for freq, gain in partials]
    return mix(*layers)


def save(name, samples, volume=0.9):
    peak = max(abs(value) for value in samples) or 1.0
    path = os.path.join(AUDIO_DIR, name + ".wav")
    with wave.open(path, "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(1)
        out.setframerate(RATE)
        data = bytes(int(128 + 127 * max(-1.0, min(1.0, value / peak * volume))) for value in samples)
        out.writeframes(data)


# ---------------------------------------------------------------------------------------------
# Combat
# ---------------------------------------------------------------------------------------------

def make_combat():
    # Slash landing on armor: a bright noise slice over a short low thump.
    save("sfx_hit", mix((envelope(noise(0.12, 0.3), 0.002, 2.0), 0.8),
                        (envelope(tone(140, 0.12, slide_to=70), 0.002, 2.0), 0.6)))

    # Heavy hit (combo finisher, super): longer, lower and crunchier.
    save("sfx_hit_heavy", mix((envelope(noise(0.28, 0.55), 0.002, 1.6), 0.9),
                              (envelope(tone(110, 0.28, slide_to=45), 0.002, 1.4), 0.8),
                              (envelope(tone(220, 0.1, "square", slide_to=90), 0.001, 2.0), 0.25)))

    # The hero taking a hit: dull thud with a grunt-like low square.
    save("sfx_hurt", mix((envelope(noise(0.2, 0.8), 0.003, 1.5), 0.8),
                         (envelope(tone(95, 0.2, "square", slide_to=60), 0.003, 1.8), 0.35)))

    # Parry: a ringing metallic clang (inharmonic partials) with a sharp click.
    save("sfx_parry", mix((bell([(1180, 1.0), (1760, 0.7), (2630, 0.45), (3510, 0.25)], 0.45), 0.8),
                          (envelope(noise(0.03, 0.0), 0.0005, 3.0), 0.6)))

    # Block: a duller clank, shorter and lower than the parry.
    save("sfx_block", mix((bell([(520, 1.0), (840, 0.6), (1330, 0.3)], 0.22, 3.0), 0.8),
                          (envelope(noise(0.06, 0.4), 0.001, 2.5), 0.5)))

    # Shield break: a crash of noise over a falling tone.
    save("sfx_shield_break", mix((envelope(noise(0.5, 0.35), 0.002, 1.3), 0.8),
                                 (envelope(tone(600, 0.5, "square", slide_to=80), 0.002, 1.5), 0.3),
                                 (bell([(700, 0.6), (1150, 0.4)], 0.3), 0.4)))

    # Dodge: a whoosh, darkened noise that swells and fades.
    whoosh = noise(0.22, 0.85)
    for index in range(len(whoosh)):
        position = index / len(whoosh)
        whoosh[index] *= math.sin(math.pi * position) ** 1.5
    save("sfx_dodge", whoosh, 0.8)


# ---------------------------------------------------------------------------------------------
# Magic
# ---------------------------------------------------------------------------------------------

def make_magic():
    # R pressed: a soft rising shimmer as time slows.
    save("sfx_channel", envelope(mix((tone(400, 0.35, "triangle", slide_to=900), 0.6),
                                     (tone(800, 0.35, "sine", slide_to=1800), 0.3)), 0.05, 1.2), 0.6)

    # Fire: a rising roar.
    roar = noise(0.5, 0.75)
    for index in range(len(roar)):
        position = index / len(roar)
        roar[index] *= min(1.0, position * 4) * (1 - position) ** 0.8
    save("sfx_fire", mix((roar, 1.0), (envelope(tone(90, 0.5, "square", slide_to=180), 0.02, 1.2), 0.25)))

    # Ice: high crystalline chimes, one after another.
    chime = silence(0.5)
    for start, freq in ((0.0, 2093), (0.07, 2637), (0.14, 3136), (0.21, 4186)):
        note = bell([(freq, 1.0), (freq * 2.01, 0.3)], 0.28, 2.0)
        offset = int(RATE * start)
        for index, value in enumerate(note):
            if offset + index < len(chime):
                chime[offset + index] += value
    save("sfx_ice", chime, 0.7)

    # Heal: a gentle rising arpeggio.
    notes = [envelope(tone(freq, 0.12, "triangle"), 0.005, 1.0) for freq in (523, 659, 784, 1047)]
    save("sfx_heal", concat(*notes, envelope(tone(1319, 0.25, "triangle"), 0.005, 1.5)), 0.7)

    # Shock: buzzing crackle bursts.
    crackle = []
    for burst in range(6):
        crackle.extend(envelope(mix((tone(random.choice((180, 220, 260)), 0.05, "square"), 0.6),
                                    (noise(0.05, 0.0), 0.6)), 0.001, 1.5))
        crackle.extend(silence(0.015))
    save("sfx_shock", crackle)


# ---------------------------------------------------------------------------------------------
# Menus, treasure and results
# ---------------------------------------------------------------------------------------------

def make_interface():
    save("sfx_menu_move", envelope(tone(880, 0.04, "square"), 0.002, 1.5), 0.45)
    save("sfx_menu_select", concat(envelope(tone(988, 0.05, "square"), 0.002, 1.0),
                                   envelope(tone(1319, 0.08, "square"), 0.002, 1.5)), 0.45)
    save("sfx_menu_back", concat(envelope(tone(784, 0.05, "square"), 0.002, 1.0),
                                 envelope(tone(523, 0.08, "square"), 0.002, 1.5)), 0.45)

    # Coins: a quick bright double ting.
    coin = concat(bell([(1976, 1.0), (2960, 0.4)], 0.07, 2.0), bell([(2637, 1.0), (3951, 0.4)], 0.22, 2.0))
    save("sfx_coins", coin, 0.7)

    # Chest: a low creak, then the coin sparkle.
    creak = envelope(tone(140, 0.18, "square", slide_to=220), 0.01, 1.0)
    save("sfx_chest", concat(mix((creak, 0.35), (noise(0.18, 0.9), 0.3)), coin), 0.75)

    # Victory: a short rising stinger.
    victory = concat(*[envelope(tone(freq, 0.1, "square"), 0.003, 0.8) for freq in (523, 659, 784)],
                     envelope(tone(1047, 0.4, "square"), 0.003, 1.2))
    save("sfx_victory", victory, 0.55)

    # Level up: a brighter fanfare.
    level_up = concat(*[envelope(tone(freq, 0.08, "square"), 0.003, 0.8) for freq in (659, 784, 988, 1319)],
                      envelope(mix((tone(1568, 0.45, "square"), 0.6), (tone(1976, 0.45, "triangle"), 0.4)), 0.003, 1.2))
    save("sfx_level_up", level_up, 0.55)

    # Defeat: a slow falling tone.
    defeat = concat(envelope(tone(392, 0.25, "triangle"), 0.01, 0.6), envelope(tone(330, 0.25, "triangle"), 0.01, 0.6),
                    envelope(tone(262, 0.6, "triangle", slide_to=196), 0.01, 1.2))
    save("sfx_defeat", defeat, 0.7)


def main():
    os.makedirs(AUDIO_DIR, exist_ok=True)
    make_combat()
    make_magic()
    make_interface()
    print("Wrote sound effects to audio/")


if __name__ == "__main__":
    main()
