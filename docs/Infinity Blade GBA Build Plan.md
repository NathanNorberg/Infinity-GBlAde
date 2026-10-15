# Infinity Blade GBA: Build Plan

Oct 12, 2026 · @Nathan

## Approach

Build the combat prototype first: one hero, one enemy, every attack type, dodge, block, parry and attack windows, running on hardware or an emulator. Progression, the castle and polish come only after the duel feels right.

The project uses Butano (C++), matching the existing GBA homebrew setup. All tuning values (frame windows, damage, wind-ups) live in one data header so they can be changed without touching logic.

## Milestones

Five milestones, each ending with a test that must pass before the next one starts.

1. **Combat prototype.** One enemy, all attack types, dodge/block/parry, attack openings, HP and shield. *Exit test:* a full duel is winnable, and every loss feels like the player's fault.
2. **Full combat kit.** Combos, super meter, magic via rings, unblockable attacks, feints, rising slashes for late enemies. *Exit test:* three enemy types each feel distinct.
3. **Progression core.** XP, leveling, stat points, five item slots, mastery, gold, shop, save data. *Exit test:* a stat or gear change is noticeable in the next duel.
4. **Castle and bloodline.** Route with the fork, hotspots, chests, keys, God King, death cutscene, 10% gold loss, new game plus. *Exit test:* a full bloodline loop plays start to finish.
5. **Content and polish.** 6 to 8 items per slot, full enemy roster, final pixel art, sound, tuning. *Exit test:* version 1 is complete.

## Project structure

This uses the standard Butano project layout, with combat split into small modules so enemies stay data-driven.

| Path / module | Holds |
| --- | --- |
| `graphics/` | Hero sprite sheet, enemy BG tiles, arena BG, HUD (Butano asset JSON beside each BMP) |
| `audio/` | Placeholder hit, parry and block sounds |
| `include/tuning.h` | Every timing window, damage value and wind-up length |
| `src/main.cpp` | Boot and scene loop |
| `src/duel_scene` | Runs one duel: owns hero, enemy, HUD and the input-to-outcome rules |
| `src/hero` | Hero state machine: idle, dodge, duck, hop, block, parry, attack, hurt |
| `src/enemy` | Enemy state machine: idle, wind-up, strike, recover, staggered, open |
| `src/attack_data` | Attack types as data: telegraph frames, direction, dodge answer, parry answer, blockable |
| `src/hud` | HP bar, shield pips, enemy HP |

## Milestone 1 tasks

The combat prototype breaks into ten tasks, in build order.

- [x] Create the Butano project, confirm it builds and boots in mGBA
- [x] Draw the scene layers: arena BG, enemy BG layer, hero sprite in the lower left
- [x] Add `tuning.h` and `attack_data` with the attack types from the combat spec
- [x] Enemy state machine cycling wind-up, strike and recover with a placeholder telegraph flash
- [x] Hero dodge, duck and hop on the D-pad, with hero sprite and arena nudge
- [x] Block on A with shield pips and stagger when empty
- [x] Parry on B + direction toward the blade, with the 6-frame window
- [x] Openings after dodge, parry and perfect sequences; B + direction attacks during them
- [x] HUD for player HP, shield pips and enemy HP; win and lose screens
- [x] Debug overlay showing the current enemy state and frame count, toggled with Select

## Milestone 2 tasks

The full combat kit breaks into nine tasks, in build order (planned Oct 13).

- [x] Unblockable (heavy) attacks: red glow and trail, per-tier chance, blocking breaks the shield
- [x] Feints: a wind-up that switches into a different swing partway through, per-tier chance
- [x] Rising slashes for late enemies: new poses, parry B + down-left or down-right, dodge away from the starting side
- [x] Three enemy types (Knight, Brute, Duelist) with distinct move sets, HP and speed; tiers scale on top
- [x] Combos: direction sequences inside one opening add bonus damage
- [x] Super meter: parries and landed hits fill it; L stuns the enemy for 3 seconds with double damage
- [x] Magic via rings: R + D-pad pattern casts Fire, Ice, Heal or Shock, with limited charges per duel
- [x] HUD for super meter, magic charges and combo feedback; Start pauses
- [x] Debug overlay switches for enemy type and ring; tuning pass for the exit test

## Milestone 3 tasks

The progression core breaks into eleven tasks, in build order (planned Oct 13).

- [x] Damage rebalance: raise base numbers (starting sword about 20 per slash, enemy HP scaled to match) so stat points give small, steady gains
- [x] Hero profile: level, XP, stats (Health, Attack, Shield, Magic), unspent points and gold, applied to the duel
- [x] Item roster as data: 6 to 8 items per slot with price, XP multiplier, level unlock and stats; drafted for approval before it's built
- [x] Equipped items apply in the duel: weapon damage, element and combo; shield pips and parry window; armor and helmet reduction; ring spell and charges
- [x] Duel rewards and results screen: XP and gold by enemy tier, perfect-sequence and no-block bonuses, item XP
- [x] Leveling and character screen: XP curve, level cap 40, spending stat points
- [x] Item mastery: item XP thresholds by price, first-mastery bonus stat point, mastered items stop granting bonus XP
- [x] Gate hub and scene flow: title, hub (Fight, Character, Equipment, Shop), duel, results; Fight climbs a difficulty ladder until the castle exists
- [x] Equipment screen and shop: equip per slot, buy (level-gated), sell at 50% or 75% if mastered
- [x] Save data: two slots plus a backup copy each, checksum, title Continue/New Game with a slot picker, autosave after duels and shop visits
- [x] Debug grants (XP, gold, items, save reset) and the exit test

## Placeholder art

The prototype uses simple shapes so combat can be judged before any real pixel art exists.

| Asset | Placeholder | Size |
| --- | --- | --- |
| Hero (from behind) | Solid silhouette with a sword shape; 3 poses: idle, dodge, duck | 64×64 sprite |
| Enemy | Blocky figure on a BG layer with one wind-up pose per attack type | About 128×128 in tiles |
| Telegraph | Colored trail along the swing path; red for unblockable | Small sprites |
| Arena | Flat gradient floor and wall | 256×256 BG |
| HUD | Bars and pips from solid tiles | 8×8 tiles |

Final pixel art replaces these in milestone 5, keeping the same sizes and palettes so code doesn't change.

**Art direction (decided Oct 15):** match the visual style of the GBA Castlevania games (Circle of the Moon, Harmony of Dissonance, Aria of Sorrow): gothic castle interiors built from dense, detailed tiles (stone, arches, candles, stained-glass colors); saturated sprites with bold, readable outlines; large, detailed enemies and bosses that dominate the screen; and an ornate HUD with a gold-framed HP bar and jewel-like icons.

## Testing and tuning

The prototype is judged by feel, so testing means playing short sessions and changing one value in `tuning.h` at a time.

- **Where:** mGBA for daily work; real hardware or a flash cart at the end of each milestone, since input timing can feel different on a real GBA.
- **What to watch:** whether each wind-up is readable at a glance, whether the 6-frame parry window feels fair, and whether openings feel worth the risk.
- **Fresh eyes:** after milestone 1, have someone who hasn't seen the spec play one duel with no explanation. If they can't work out dodge and block within a minute or two, the telegraphs need work.

## Open questions

- **Project folder:** `~/Documents/GBA-Projects/Infinity-GBlAde`
- **Working title:** Infinity GBlAde (open to iterations)
- **Hardware testing:** flash cart available

* [x] Which Butano version and toolchain (devkitARM or Wonderful) is installed? Butano 21.7.1 with devkitARM (gcc 16.1.0); the build uses python3.
