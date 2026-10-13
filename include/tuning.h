#ifndef IGB_TUNING_H
#define IGB_TUNING_H

// Every timing window, damage value and wind-up length in one place.
// All times are in frames at 60 fps. Change one value at a time while play-testing.
// Starting values come from docs/Infinity Blade GBA Combat Spec.md.

namespace igb::tuning
{

// ---------------------------------------------------------------------------------------------
// Enemy wind-ups (telegraph length, ending at the moment of impact).
// These are base values; each enemy tier below scales them by its windup_percent.
// ---------------------------------------------------------------------------------------------

constexpr int windup_diagonal_slash = 40;
constexpr int windup_horizontal_slash = 40;
constexpr int windup_overhead_chop = 44;
constexpr int windup_low_sweep = 40;
constexpr int windup_thrust = 34;

// Heavy (unblockable) attacks add this to the base swing's wind-up so the glow can be read.
constexpr int windup_heavy_extra = 8;

// Shortest wind-up any enemy may ever use (late game).
constexpr int windup_min = 18;

// ---------------------------------------------------------------------------------------------
// Enemy swing and sequence pacing.
// ---------------------------------------------------------------------------------------------

constexpr int strike_frames = 8;               // Swing shown after impact
constexpr int sequence_min_attacks = 1;
constexpr int sequence_max_attacks = 4;        // Hard cap for every tier
constexpr int heavy_chance_percent = 0;        // Unblockable attacks arrive in milestone 2

// ---------------------------------------------------------------------------------------------
// Enemy tiers, easiest first. Later areas of the castle use faster tiers.
// ---------------------------------------------------------------------------------------------

struct enemy_tier
{
    const char* name;
    int windup_percent;            // Scales the base wind-ups above
    int recover_between_attacks;   // Gap between attacks in one sequence
    int pause_between_sequences;   // Idle gap when a sequence earns no opening
    int max_attacks;               // Longest sequence this tier throws
    bool diagonal_slashes;         // Throws / and \ slashes, which need diagonal parries
    int damage_percent;            // Scales the attack damage values below
};

// Gate guards (the first two duels) teach the four straight answers: no diagonal slashes,
// so players never learn to parry a slash with a plain left or right. They also hit softer
// (about 9 to 10 hits to kill a fresh hero) since they're the learning fights.
constexpr enemy_tier tier_gate_guard = { "Gate guard", 140, 28, 70, 2, false, 80 };
constexpr enemy_tier tier_hall_guard = { "Hall guard", 115, 20, 55, 3, true, 100 };
constexpr enemy_tier tier_elite = { "Elite", 100, 16, 45, 4, true, 100 };
constexpr enemy_tier tier_god_king = { "God King", 70, 12, 35, 4, true, 100 };

// Easiest first. The debug overlay cycles through these with L and R.
inline constexpr const enemy_tier* enemy_tiers[] = {
    &tier_gate_guard, &tier_hall_guard, &tier_elite, &tier_god_king
};

constexpr int enemy_tier_count = sizeof(enemy_tiers) / sizeof(enemy_tiers[0]);
constexpr int first_duel_tier = 0;             // Index into enemy_tiers

// ---------------------------------------------------------------------------------------------
// Telegraph (placeholder flash and trail).
// ---------------------------------------------------------------------------------------------

constexpr int telegraph_start_flash = 4;       // Enemy flashes when a wind-up begins
constexpr int telegraph_blink_frames = 12;     // Trail blinks during the last frames before impact

// ---------------------------------------------------------------------------------------------
// Player defense windows (frames before impact).
// ---------------------------------------------------------------------------------------------

constexpr int parry_window = 6;
constexpr int dodge_window = 12;

constexpr int dodge_duration = 20;             // How long the hero stays out of the blade's path
constexpr int dodge_cooldown = 8;              // Before another dodge can start
constexpr int parry_commit = 20;               // Hero is locked in after a parry attempt
constexpr int parry_chord_grace = 3;           // A dodge this young turns into a parry if B is pressed
constexpr int parry_diagonal_settle = 2;       // Second D-pad direction this late still makes a diagonal
constexpr int enemy_parry_reel = 24;           // Enemy reels after a mid-sequence parry, then continues

// ---------------------------------------------------------------------------------------------
// Shield.
// ---------------------------------------------------------------------------------------------

constexpr int shield_pips = 4;                 // Spec: 3 to 6, from the shield item later
constexpr int shield_pips_per_block = 1;
constexpr int shield_break_stagger = 60;       // Hero can't act after the shield empties
constexpr int shield_regen_frames = 240;       // One pip back per this many frames not blocking; 0 = off

// ---------------------------------------------------------------------------------------------
// Openings (the player's turn to attack).
// ---------------------------------------------------------------------------------------------

constexpr int opening_after_dodge = 60;
constexpr int opening_after_parry = 120;
constexpr int opening_after_perfect = 180;     // Every attack in the sequence parried
constexpr int opening_after_shield_break = 180; // Enemy shields arrive in milestone 2
constexpr int opening_warning_frames = 30;     // Enemy blinks before raising the guard again

constexpr int player_attack_cooldown = 15;     // Frames between slashes

// ---------------------------------------------------------------------------------------------
// Health and damage.
// ---------------------------------------------------------------------------------------------

constexpr int player_max_hp = 100;
constexpr int enemy_max_hp = 150;

constexpr int player_slash_damage = 5;

constexpr int damage_diagonal_slash = 12;
constexpr int damage_horizontal_slash = 12;
constexpr int damage_overhead_chop = 16;
constexpr int damage_low_sweep = 10;
constexpr int damage_thrust = 14;
constexpr int heavy_damage_percent = 150;      // Heavy attack damage vs. its base swing

constexpr int hero_hurt_frames = 24;           // Hit stun after taking damage

// ---------------------------------------------------------------------------------------------
// Presentation (movement feel). Offsets are in pixels.
// ---------------------------------------------------------------------------------------------

constexpr int evade_out_frames = 4;            // Ease out of the blade's path
constexpr int evade_return_frames = 6;         // Ease back at the end of the dodge
constexpr int dodge_offset = 28;               // Hero slide for a left/right dodge
constexpr int hop_height = 18;
constexpr int duck_drop = 4;                   // On top of the drop drawn into the duck pose
constexpr int arena_nudge = 8;                 // Arena moves opposite to the dodge
constexpr int hurt_flash_frames = 10;
constexpr int block_flash_frames = 6;          // Hero flashes white when a block absorbs a hit
constexpr int stagger_wobble = 2;              // Side-to-side shake while staggered
constexpr int parry_offset = 6;                // Hero leans this far toward the parry direction
constexpr int enemy_stagger_shake = 3;         // Enemy shake after being parried
constexpr int enemy_stagger_shake_frames = 16;
constexpr int enemy_stagger_flash = 6;
constexpr int enemy_open_slump = 4;            // Enemy sags this far while open
constexpr int enemy_hit_frames = 10;           // Reaction to a hero slash
constexpr int enemy_hit_flash = 4;
constexpr int enemy_hit_shake = 2;
constexpr int hit_streak_frames = 6;           // Slash streak across the enemy's body
constexpr int attack_lunge = 6;                // Hero leans toward the enemy on a slash
constexpr int defeat_sink_frames = 40;         // Loser sinks out of view over this many frames
constexpr int defeat_sink_distance = 48;
constexpr int result_prompt_delay = 60;        // Before "Press START" appears on win/lose

}

#endif
