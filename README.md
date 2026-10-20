# Infinity GBlAde

A Game Boy Advance duel game inspired by the first Infinity Blade, built with [Butano](https://github.com/GValiente/butano).

Milestones 1 to 4 are complete: the full combat kit (dodge, block and diagonal-aware parries; unblockable attacks, feints and rising slashes; Knight, Brute, Duelist and God King enemies; combos, a super meter and ring magic), the progression core (levels and stat points, 36 items with mastery, gold, a shop and two save slots with backups), and the castle and bloodline (five areas with a fork, treasure and keys, HP carried through a run, death passing the sword to the next generation, and new game plus after the God King). Milestone 5 (content and polish) is nearly done: hit-stop, screen shake and sparks, fades between scenes, damage numbers, generated sound effects, and art in the style of the GBA Castlevania games (hero, enemy kits with named looks per area and the God King, area backgrounds, title, HUD and menu frames). What remains is the final tuning pass and a full playthrough on hardware. The design lives in `docs/`, along with the art style guide and preview sheets in `docs/art/`.

## Building

Requirements:

- devkitARM (devkitPro)
- Butano 21.7.1, checked out so that `../Butano/butano` (relative to this folder) is the Butano library folder. Change `LIBBUTANO` in the `Makefile` if yours lives elsewhere.
- `python3`

```bash
make -j8
```

This produces `infinity_gblade.gba`. The folder path must not contain spaces, since Butano's makefiles can't handle them.

## Controls

In a duel:

| Input | Action |
| --- | --- |
| D-pad | Dodge left/right, duck (down), hop (up) |
| A (hold) | Block |
| B + direction | Parry during an enemy wind-up (diagonals for slashes); slash during an opening |
| L | Super, when the gold meter is full |
| R (hold) + D-pad pattern | Cast the equipped ring's spell; the enemy slows while you draw |
| Start | Pause; back to the gate after a win or loss |

Menus (title, gate, character, equipment, shop): D-pad to choose, A to select, B to go back; L/R switch slots on the equipment and shop screens. Castle areas: D-pad moves between hotspots, A fights, takes treasure or follows a path.

Debug controls in a duel:

| Input | Action |
| --- | --- |
| Tap Select | Toggle the debug overlay |
| Select + L / R | Next enemy tier / enemy type (restarts the duel) |
| Select + B, then Select + A | Pick a gear slot, then equip the next item in it (restarts the duel) |
| Select + D-pad | Add a stat point: up Health, right Attack, down Shield, left Magic |
| Select + Start | Refill super meter and magic charges |
| Start (overlay shown) | Toggle invincibility |

Debug controls elsewhere: at the gate, Select + A / B / R grant 1,000 gold / a level / every item; on a castle area, Select + A / B / R / L refill HP / give a key / jump to the next area / raise the cycle; on the title screen, Select + Start erases both save slots.

## Tuning and art

- Every timing window, damage value, enemy tier and type, and progression rule is in `include/tuning.h`; item stats are in `include/item_roster.h`; the castle route, rosters and treasure are in `include/castle.h`.
- Art and sound are generated in code, using the color ramps in `tools/art_style.py` (see `docs/Infinity Blade GBA Art Style Guide.md`). Each script writes its files into `graphics/` or `audio/`:
  - `tools/make_hero_art.py`: the hero's poses
  - `tools/make_enemy_art.py`: enemy kits, poses and named looks, plus the generated `src/enemy_looks.h` and `src/enemy_pose_tables.h`
  - `tools/make_background_art.py`: area backgrounds and the title
  - `tools/make_ui_art.py`: HP bars and caps, shield pips, charge orbs, castle markers and the menu frame
  - `tools/make_placeholder_art.py`: telegraphs, sparks, digits, arrows and the rest of the small sprites
  - `tools/make_sounds.py`: the sound effects
- `tools/preview_art.py` renders preview sheets and mock duel screens, and `tools/png_to_gba.py` converts PNGs to the game's 16-color BMPs.
- `references/` (gitignored) holds reference images for style study only; no art is copied from them.

The 8x8 font in `graphics/` and `include/` comes from Butano's `common` folder (zlib license).
