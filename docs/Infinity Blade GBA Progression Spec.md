# Infinity Blade GBA: Progression Spec

Oct 12, 2026 · @Nathan

## Overview

Each bloodline run is a climb of about 10 duels ending at the God King. Winning or losing, the next generation starts at the castle gate with the same level, gear and gold, so every run makes the hero stronger until the God King falls.

Three progress tracks feed each other: player level (stat points), item mastery (pushes the player to swap gear), and gold (buys the next item to master). The God King starts far stronger than the hero and only grows after he is beaten, so each victory raises the bar for the next cycle.

## Castle route

A run follows a mostly fixed path through five areas, with one fork that changes the fight count from 8 to 10 duels including the God King.

| Area | Duels | Notes |
| --- | --- | --- |
| Gate and courtyard | 2 | Easy guards; teaches dodge, block and straight parries (no diagonal slashes) |
| Fork: Garden path | 2 | Fewer fights, more treasure; first diagonal slashes |
| Fork: Barracks path | 4 | More fights and XP, less treasure; first diagonal slashes |
| Great hall | 2 | Mid-tier enemies; first unblockable attacks |
| Throne approach | 1 | Elite guard (mini-boss) |
| Throne room | 1 | God King |

Between duels the player picks from 2 to 4 hotspots per area with the D-pad and A to move along, open chests, and pick up gold. Version 1 can start with one enemy roster per area and add variety later.

## Player leveling

The player earns XP from duels and gets 1 stat point per level, up to a level cap of 40 for version 1.

- **XP curve:** XP needed for the next level = 100 × level^1.5, rounded. Level 2 needs 100 XP; level 20 needs about 8,900.
- **Duel XP:** a base amount set by the enemy, plus bonuses for a perfect parry sequence (+25%) and finishing without blocking (+10%).

| Stat | Effect per point |
| --- | --- |
| Health | +10 max HP |
| Attack | +2 damage per hit |
| Shield | +1 shield pip every 3 points; parry window +1 frame every 5 points |
| Magic | +1 magic charge every 3 points; spell damage +5% |

The super meter isn't a stat; it fills during a duel and empties when the duel ends.

## Items and mastery

The hero wears five items, and each one earns its own XP toward mastery. Mastered items keep their stats but stop granting bonus XP, which pushes the player to buy and equip something new.

| Slot | Main stat | Extra |
| --- | --- | --- |
| Weapon | Damage | Element (fire, ice, shock) and a weapon combo |
| Shield | Pips, block strength | Can widen the parry window |
| Armor | Damage reduction |  |
| Helmet | Damage reduction | Small HP bonus |
| Ring | Spell | Decides which spell R + D-pad casts and its charges |

- **XP multiplier:** each item has a ×1 to ×3 multiplier on duel XP while unmastered. Cheap items master fast; rare ones master slow but hit harder.
- **Mastery reward:** 1 bonus stat point the first time an item is mastered, plus a gold sell bonus.
- **Mastery cost:** item XP needed scales with the item's price, roughly 2 to 6 duels for early gear and 15 or more for endgame gear.

## Gold, shop and treasure

Gold comes from duels, chests and selling gear, and it is spent at a shop at the castle gate before each run.

| Source | Amount (starting values) |
| --- | --- |
| Duel win | 50 to 300 gold, set by enemy tier |
| Gold pouch hotspot | 20 to 100 gold |
| Treasure chest | Gold or an item; some locked chests need a key |
| Selling an item | 50% of price; 75% if mastered |

- **Shop:** opens only between runs. Version 1 ships 6 to 8 items per slot, unlocking as the player's level rises.
- **Keys:** rare drops from elite guards; one opens a locked chest that holds a guaranteed rare item.
- **No gold sinks beyond gear** in version 1, which keeps the economy easy to tune.

## Bloodline carryover

Nearly everything carries over between generations; only the run itself resets.

| Carries over | Resets |
| --- | --- |
| Level, stats, XP | Position in the castle (back to the gate) |
| All owned items and their mastery | Opened chests and gold pouches (refill each run) |
| Gold (minus 10% on death) and keys | Current HP and super meter |
| Bloodline count | Enemy roster (rerolled per area) |

- **Death:** losing any duel ends the run and costs 10% of carried gold (a starting value to tune). A short cutscene shows the next generation getting up and walking down the path to the gate. The hero uses the same sprite every generation.
- **God King scaling:** fixed at level 50 for the first cycle and never adjusts to the player; only leveling and gear close the gap. Each win raises him, and every castle enemy, for the next cycle (decided Oct 20: without it a full clear always paid the same XP and leveling stalled).
- **God King difficulty:** he must feel like a brutal boss. No one should beat him on the first bloodline without exceptional reflexes, and that's intended: do not soften his wind-ups, feints or unblockables to make a fresh hero competitive (decided Oct 13). Testing uses the debug overlay's invincibility switch instead.
- **Victory:** beating the God King shows a short ending, then starts a new, harder cycle with the same hero (new game plus).

## Save data

The full save fits in well under 1 KB of the cartridge's 32 KB SRAM, so two save slots plus a backup copy are easy.

| Data | Size (approx.) |
| --- | --- |
| Header, version, checksum | 16 bytes |
| Level, XP, stat allocation, unspent points | 16 bytes |
| Gold, keys, bloodline count, cycle (new game plus) | 12 bytes |
| Equipped items (5 slots) | 5 bytes |
| Owned items + mastery XP (up to 100 items) | 300 bytes |
| Mid-run state: area, hotspot, current HP | 8 bytes |

The game saves after every duel and at every shop visit. The checksum lets it fall back to the backup copy if a save is corrupted, for example by power loss mid-write.

## Decisions

- **Damage scale:** base numbers rise so the stat rules stay as written: the starting sword does about 20 per slash and enemy HP scales to match, so +2 per Attack point is a small, steady gain (decided Oct 13).
- **Between duels before the castle exists:** a gate hub menu (Fight, Character, Equipment, Shop). Fight starts the next duel on a rising difficulty ladder; milestone 4 replaces it with the castle route (decided Oct 13).
- **Save slots:** two slots, each with a backup copy and checksum, picked from the title screen (decided Oct 13).
- **Item roster:** the full 6 to 8 items per slot is built in milestone 3; milestone 5 adds their art and sound (decided Oct 13).
- **HP within a run:** carries over between duels and refills only at the start of a run, so defense and Heal rings matter across the climb (decided Oct 15). After each win, 10% of max HP comes back, deliberately small (decided Oct 15). Turning the game off mid-duel counts as losing that duel when the save is loaded, so the power switch can't dodge a death (decided Oct 15).
- **The God King:** his own enemy type, with his own colors, HP and move set (every attack including rising slashes, heavy on feints and unblockables), on top of the God King tier (decided Oct 15).
- **Area screens:** each area is one screen with its own background and 2 to 4 hotspot markers; the D-pad moves between them and A acts (decided Oct 15).
- **Teaching order:** the gate throws Knights and Duelists only (no Brute); the fork paths bring diagonal slashes but no unblockables; the great hall brings the first unblockables (decided Oct 15).
- **Castle tuning (milestone 4 starting values, in include/castle.h and include/tuning.h):** the great hall uses a new "Hall captain" tier (hall pace, 10% unblockables); the throne approach's elite guard drops a key half the time, and the great hall holds the locked chest, so keys carry from one bloodline to the next; plain chests hold an item 40% of the time (up to the area's price cap), locked chests an unowned item worth 4,000 or more; each cycle adds +20% HP (+25% for the God King), +10% damage and +30% XP and gold to every enemy (Oct 20); death costs 10% of gold.

## Open questions

None right now. New questions go here as they come up.
