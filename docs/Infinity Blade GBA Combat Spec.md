# Infinity Blade GBA: Combat Spec

Oct 12, 2026 · @Nathan

## Overview

Every fight is a one-on-one duel where the player reads the enemy's swing and answers it with the D-pad, A and B. Four pillars guide every rule below:

- **Read, then react.** Every enemy attack has a visible wind-up, and the correct answer is always readable from it.
- **Risk buys reward.** Dodging is safe but slow, blocking costs a resource, and parrying is risky but opens the biggest punish.
- **Short and intense.** A duel lasts about 30 to 90 seconds.
- **Progression carries the run.** Bloodline resets, item mastery, gold and stats work like the original Infinity Blade.

## Controls

The D-pad alone dodges; combined with B it aims a parry or an attack.

| Input | Action | Notes |
| --- | --- | --- |
| D-pad (tap) | Dodge | Move out of the blade's path: left, right, duck (down) or hop (up) |
| A (hold) | Block | Stops any attack, drains the shield meter |
| B + direction during enemy wind-up | Parry | Direction points toward where the blade comes from |
| B + direction during an opening | Attack | Directional slash; chains into combos |
| R + D-pad pattern | Magic | Short motion input, stand-in for drawing symbols |
| L | Super | Spends a full super meter; parries and landed hits fill it |
| Start | Pause |  |

## Enemy attacks and dodge rules

The dodge rule is simple: move away from where the blade will land. For diagonal slashes that means away from where the swing finishes, toward the side the weapon is raised on (changed after play-testing, Oct 12). Parries point toward where the blade comes from, so diagonal slashes need diagonal parries (B + up-left or B + up-right). Gate guards, the first two duels, never throw diagonal slashes; hall guards introduce them, so players never learn to parry a slash with a plain left or right (decided Oct 12).

| Attack | Telegraph | Dodge | Parry | Blockable |
| --- | --- | --- | --- | --- |
| Diagonal slash ( / ) | Weapon raised high left | Left | B + up-left | Yes |
| Diagonal slash ( \\ ) | Weapon raised high right | Right | B + up-right | Yes |
| Horizontal slash | Weapon pulled back to one side | Down (duck) | B + the side it comes from | Yes |
| Overhead chop | Weapon raised straight up | Left or right | B + up | Yes |
| Low sweep | Weapon dropped low | Up (hop) | B + down | Yes |
| Thrust | Weapon pulled back at waist | Left or right | Not parryable | Yes |
| Rising slash (late game, final boss) | Weapon dropped low to one side | Away from the side it starts on (changed after play-testing, Oct 13) | B + down-left or B + down-right | Yes |
| Heavy / unblockable | Glow or flash on wind-up | Per base swing | Not parryable | No, breaks shield |

Enemies chain 1 to 4 attacks per sequence before an opening. Later enemies add feints (a wind-up that cancels into a different swing).

## Defense: risk and reward

Each defensive option trades safety for the size of the opening it creates.

| Option | Window | Cost | Reward |
| --- | --- | --- | --- |
| Dodge | Wide (about 12 frames) | None | Small opening after the enemy's full sequence |
| Block | Any time A is held | 1 shield pip per hit; empty shield = stagger | No damage taken, no opening |
| Parry | Tight (about 6 frames) | A miss takes full damage | Enemy staggered, big opening right away |

A perfect sequence (every attack in a chain parried) triggers a longer stun, rewarding mastery like the original.

## Offense

The player attacks only during openings, and the opening's length depends on how it was earned.

- **Opening lengths:** about 1 second after a dodge, about 2 seconds after a parry, about 3 seconds after a perfect sequence or a shield break on the enemy.
- **Directional slashes:** B + any direction is one hit. Direction matters for combos and for enemies with guarded sides.
- **Combos:** a specific direction sequence (for example left, right, up) inside one opening adds bonus damage. Weapons can come with their own combo.
- **Enemy recovery:** when the opening ends, the enemy raises its guard and further hits are blocked.

## Timing and resources

All timings assume the GBA's 60 frames per second and are starting values for tuning.

| Resource / timing | Starting value | Driven by |
| --- | --- | --- |
| Enemy wind-up | 47 to 61 frames for the first enemy (gate guard), down to 23 to 30 for the God King | Enemy tier in tuning.h |
| Parry window | 6 frames before impact | Shield stat can widen it |
| Dodge window | 12 frames before impact | Fixed |
| Player HP | Health stat | Level-up points, armor |
| Shield pips | 3 to 6 | Shield item |
| Player damage | Weapon attack | Weapon, attack stat |
| Damage taken | Enemy attack minus armor | Armor, helmet |

Winning a duel awards XP to the player and to each equipped item toward mastery, plus gold.

## Presentation

The camera sits over the hero's shoulder, built from three layers on the 240×160 screen.

| Layer | Content | Technique |
| --- | --- | --- |
| Foreground | Hero seen from behind, lower left | Large multi-part sprite; shifts on dodge, drops on duck |
| Middle | Enemy, centered and large | Background layer with tile-swap animation; affine scaling for lunges |
| Back | Arena | Background layer; nudged opposite to dodges for a sense of motion |
| HUD | HP, shield pips, enemy HP | Window or sprite overlay along the top edge |

Telegraphs come from the enemy's upper body and weapon so the hero never hides them. Each wind-up gets a short flash or trail along the swing path, and unblockable attacks get a distinct color.

## Decisions

- **Directions:** dodges use 4 directions. Parries use 4 directions plus the diagonals for slashes. Diagonal slashes start with hall guards, and rising slashes come in the late game.
- **Parry aim:** point toward where the blade comes from.
- **Version 1 scope:** the full bloodline loop (climb, God King, death, next generation with gear and levels carried over).
- **L button:** super meter. A full meter stuns the enemy instantly, even mid-wind-up, cancelling his sequence; he stays open for 3 seconds and slashes deal double damage (decided Oct 13).
- **Magic:** included in version 1. Rings offer four spells: Fire (direct damage), Ice (freezes the enemy for about 2 seconds, acting as an opening), Heal (restores about 30% HP) and Shock (the longest stagger, less damage) (decided Oct 13). Casting: pressing R spends a charge and slows the enemy to about 20% speed for up to 5 seconds while the pattern is drawn; releasing R or running out of time fizzles the spell with the charge spent. Every spell that fires knocks the enemy out of his sequence (Ice as an opening, the others as a short stagger), so casting is never the ultimate risk; cast during an opening, the opening keeps running (decided after play-testing, Oct 13).
- **Enemy types:** milestone 2 builds three: Knight (balanced), Brute (slow, high HP, hits hard, more overheads and unblockables) and Duelist (fast, low HP, favors slashes and thrusts, feints). Tiers scale speed and damage on top of the type (decided Oct 13).
- **Art style:** pixel art drawn at GBA scale.
- **Run length:** about 10 duels per bloodline run, varying by path, modeled on the original's castle climb.

## Open questions

None right now. New questions go here as they come up.
