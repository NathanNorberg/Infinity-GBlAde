# Infinity Blade GBA: Item Roster

Oct 14, 2026 · Draft for approval (milestone 3, task 3)

## How to read this

Every slot has a free starter item and 5 to 7 items to buy. All numbers are starting values that live in data, so they can be tuned without code changes.

- **Price:** gold at the gate shop. A run of about 10 duels earns roughly 1,000 to 1,500 gold, so early upgrades cost about a run and endgame items take many bloodlines.
- **XP ×:** the item's multiplier on duel XP while unmastered (×1 to ×3). Cheap items master fast.
- **Unlock:** the hero level at which the shop starts selling it.
- Item names are original, inspired by the genre rather than taken from the original game.

## Weapons (8)

Damage is per slash and replaces the current 20. The Attack stat adds +2 per point on top. Each weapon brings its own combo; the bonus is added to the finishing slash.

| Weapon | Damage | Element | Combo | Bonus | Price | XP × | Unlock |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Squire's Blade | 20 | None | Cross cut: ← → ↑ | +60 | Starter | ×2 | 1 |
| Iron Longsword | 26 | None | Hammerfall: ↑ ↓ ↑ | +75 | 400 | ×3 | 2 |
| Ember Saber | 30 | Fire | Flicker: → ← → | +90 | 1,200 | ×2 | 5 |
| Frostbite Edge | 34 | Ice | Glacier cut: ↓ ← → | +100 | 2,500 | ×2 | 8 |
| Storm Falchion | 40 | Shock | Tempest: ↑ → ↓ ← | +150 | 5,000 | ×2 | 12 |
| Knight's Greatsword | 48 | None | Twin rend: ← ← → → | +170 | 8,000 | ×2 | 16 |
| Dragonfang | 58 | Fire | Wyrm bite: ↓ ↑ ← → | +220 | 14,000 | ×1 | 22 |
| Godslayer | 70 | Shock | Deicide: ↑ ↓ ← → ↑ | +300 | 25,000 | ×1 | 30 |

Four- and five-step combos don't fit inside the 1-second dodge opening; they need a parry, perfect or super opening. That's deliberate: longer combos pay more and ask for better defense.

## Shields (7)

Pips are the shield's base count; the Shield stat adds +1 per 3 points, and the total stops at 6. Parry is extra parry-window frames (the window stops at 12). Refill is how long one pip takes to come back while not blocking.

| Shield | Pips | Parry | Refill | Price | XP × | Unlock |
| --- | --- | --- | --- | --- | --- | --- |
| Traveler's Shield | 4 | +0 | 4.0 s | Starter | ×2 | 1 |
| Oak Kite Shield | 4 | +0 | 3.0 s | 350 | ×3 | 2 |
| Iron Heater | 5 | +0 | 4.0 s | 1,000 | ×2 | 4 |
| Duelist's Parma | 3 | +2 | 3.0 s | 2,200 | ×2 | 7 |
| Steel Tower | 6 | +0 | 5.0 s | 4,500 | ×2 | 11 |
| Mirror Aegis | 5 | +2 | 3.3 s | 9,000 | ×1 | 18 |
| Bastion of Ages | 6 | +3 | 3.0 s | 18,000 | ×1 | 26 |

The Duelist's Parma and Steel Tower are the two directions a player can lean: fewer pips with an easier parry, or a wall that refills slowly.

## Armor (7)

Reduction is subtracted from every enemy hit (the spec's "enemy attack minus armor"), together with the helmet's. A hit always does at least 1 damage. For scale, enemy hits currently do 8 to 24.

| Armor | Reduction | Price | XP × | Unlock |
| --- | --- | --- | --- | --- |
| Padded Gambeson | 0 | Starter | ×2 | 1 |
| Leather Jerkin | 1 | 300 | ×3 | 2 |
| Chainmail | 2 | 900 | ×2 | 4 |
| Scale Hauberk | 3 | 2,000 | ×2 | 7 |
| Brigandine | 4 | 4,000 | ×2 | 11 |
| Knight's Plate | 6 | 9,000 | ×1 | 17 |
| Royal Warplate | 8 | 18,000 | ×1 | 25 |

## Helmets (6)

Helmets add a little reduction and some max HP (which also grows the HP bar).

| Helmet | Reduction | Max HP | Price | XP × | Unlock |
| --- | --- | --- | --- | --- | --- |
| Cloth Hood | 0 | +0 | Starter | ×2 | 1 |
| Leather Cap | 0 | +10 | 250 | ×3 | 2 |
| Iron Skullcap | 1 | +10 | 800 | ×2 | 4 |
| Nasal Helm | 1 | +25 | 1,800 | ×2 | 8 |
| Great Helm | 2 | +30 | 5,000 | ×2 | 13 |
| Crown of Valor | 3 | +50 | 14,000 | ×1 | 22 |

## Rings (8)

A ring sets the spell and its base charges per duel; the Magic stat adds +1 per 3 points (up to 6) and +5% spell power per point. Power scales the spell's main effect: Fire and Shock damage, Ice freeze time, Heal amount and Shock stun time. Two rings per spell: a starter-tier version and a stronger one.

| Ring | Spell | Charges | Power | Price | XP × | Unlock |
| --- | --- | --- | --- | --- | --- | --- |
| Ember Ring | Fire | 2 | 100% | Starter | ×2 | 1 |
| Frost Band | Ice | 2 | 100% | 500 | ×3 | 3 |
| Mender's Ring | Heal | 2 | 100% | 700 | ×3 | 3 |
| Spark Ring | Shock | 2 | 100% | 900 | ×2 | 5 |
| Blaze Signet | Fire | 3 | 130% | 4,000 | ×2 | 12 |
| Glacier Seal | Ice | 3 | 125% | 6,000 | ×2 | 15 |
| Lifebloom Ring | Heal | 3 | 150% | 7,500 | ×1 | 18 |
| Tempest Ring | Shock | 3 | 150% | 10,000 | ×1 | 21 |

## Totals

36 items: 8 weapons, 7 shields, 7 armor, 6 helmets, 8 rings. Save data has room for up to 100.

## Weapon elements

A weapon's element triggers when its combo completes (decided Oct 14), so it rewards landing the combo and is easy to read:

- **Fire:** the finisher sets him burning: twice the weapon's damage, dealt in small ticks over 3 seconds.
- **Ice:** the finisher adds 1 second to the current opening.
- **Shock:** when the opening ends, he stays stunned for another 0.75 seconds before his next sequence.

## Open questions

None right now. Mark up any item, number or name above before it's built.
