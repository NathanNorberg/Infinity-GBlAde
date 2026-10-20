#ifndef IGB_TUNING_H
#define IGB_TUNING_H

// Every timing window, damage value and wind-up length in one place. Item stats (weapon
// damage, shield pips, armor, rings) are in include/item_roster.h.
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
constexpr int windup_rising_slash = 42;

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
    int heavy_percent;             // Chance each attack is a heavy (unblockable) swing
    int feint_percent;             // Chance each attack opens with a fake wind-up
    bool rising_slashes;           // Throws rising slashes (late game), parried down-left/down-right
    int xp_reward;                 // Base duel XP for beating him
    int gold_reward;               // Gold for beating him (spec: 50 to 300 by tier)
    int hp_percent = 100;          // Scales the type's HP (the God King grows each cycle)
};

// Gate guards (the first two duels) teach the four straight answers: no diagonal slashes,
// so players never learn to parry a slash with a plain left or right. They also hit softer
// (about 9 to 10 hits to kill a fresh hero) since they're the learning fights. Unblockable
// attacks and feints start with elites (the progression spec puts the first unblockables in
// the great hall; per-area rosters in milestone 4 can refine this).
constexpr enemy_tier tier_gate_guard = { "Gate guard", 140, 28, 70, 2, false, 80, 0, 0, false, 40, 50 };
constexpr enemy_tier tier_hall_guard = { "Hall guard", 115, 20, 55, 3, true, 100, 0, 0, false, 80, 100 };
// Great hall: hall-guard pace with the first unblockable attacks (progression spec).
constexpr enemy_tier tier_hall_captain = { "Hall captain", 110, 18, 50, 3, true, 100, 10, 0, false, 110, 150 };
constexpr enemy_tier tier_elite = { "Elite", 100, 16, 45, 4, true, 100, 20, 15, false, 150, 200 };
constexpr enemy_tier tier_god_king = { "God King", 70, 12, 35, 4, true, 100, 25, 20, true, 400, 300 };

// Easiest first. The debug overlay cycles through these with Select + L.
inline constexpr const enemy_tier* enemy_tiers[] = {
    &tier_gate_guard, &tier_hall_guard, &tier_hall_captain, &tier_elite, &tier_god_king
};

constexpr int enemy_tier_count = sizeof(enemy_tiers) / sizeof(enemy_tiers[0]);
constexpr int first_duel_tier = 0;             // Index into enemy_tiers

// ---------------------------------------------------------------------------------------------
// Enemy types: each one's personality. The tier sets difficulty; the type scales it.
// Type scales multiply the tier's values, so a tier with no unblockables or feints (the
// learning fights) stays that way for every type.
// ---------------------------------------------------------------------------------------------

constexpr int attack_kinds = 9;                // Must match attack_id::COUNT in attack_data.h

struct enemy_type
{
    const char* name;
    int max_hp;
    int windup_percent;            // Scales the tier's wind-ups
    int pace_percent;              // Scales the tier's gaps between attacks and sequences
    int damage_percent;            // Scales the tier's damage
    int heavy_scale_percent;       // Scales the tier's unblockable chance
    int feint_scale_percent;       // Scales the tier's feint chance
    int max_attacks_change;        // Added to the tier's longest sequence (kept within the sequence limits)
    int palette;                   // Kit (drawn look): 0 knight, 1 brute, 2 duelist, 3 God King

    // How often each attack is picked, in attack_id order:
    // slash /, slash \, horizontal <, horizontal >, overhead, sweep, thrust, rising /, rising \.
    // Tier rules still apply: a tier without diagonal or rising slashes never throws them.
    int attack_weights[attack_kinds];

    int reward_percent;            // Scales the tier's XP and gold (tougher types pay more)
};

// Knight: the balanced baseline.
constexpr enemy_type type_knight = { "Knight", 600, 100, 100, 100, 100, 100, 0, 0,
                                     { 2, 2, 2, 2, 2, 2, 2, 2, 2 }, 100 };

// Brute: slow and tough, hits hard, loves overheads and sweeps, throws more unblockables,
// never feints, shorter chains.
constexpr enemy_type type_brute = { "Brute", 880, 125, 130, 140, 200, 0, -1, 1,
                                    { 1, 1, 2, 2, 5, 3, 1, 1, 1 }, 120 };

// Duelist: fast and fragile, favors slashes and thrusts, feints often, longer chains.
constexpr enemy_type type_duelist = { "Duelist", 440, 80, 75, 80, 50, 200, 1, 2,
                                      { 3, 3, 2, 2, 1, 1, 3, 2, 2 }, 110 };

// God King: his own type for the throne room. Throws everything (rising slashes most of all),
// hits hard, and leans on unblockables and feints. Fixed at "level 50" on the first cycle;
// each cycle after a win makes him stronger (see god_king_*_per_cycle).
constexpr enemy_type type_god_king = { "God King", 1200, 100, 90, 120, 120, 125, 0, 3,
                                       { 2, 2, 2, 2, 3, 2, 2, 3, 3 }, 150 };

// The debug overlay cycles through these with Select + R.
inline constexpr const enemy_type* enemy_types[] = {
    &type_knight, &type_brute, &type_duelist, &type_god_king
};

constexpr int god_king_type_index = 3;
constexpr int god_king_hp_percent_per_cycle = 25;      // New game plus: more HP each cycle
constexpr int god_king_damage_percent_per_cycle = 10;  // ...and harder hits

// Within a castle run, HP carries between duels; each win gives a little back.
constexpr int win_heal_percent = 10;           // Of max HP

// Death (progression spec): the run ends and part of the carried gold is lost.
constexpr int death_gold_loss_percent = 10;

constexpr int enemy_type_count = sizeof(enemy_types) / sizeof(enemy_types[0]);
constexpr int first_duel_type = 0;             // Index into enemy_types

// One enemy's effective settings: the tier scaled by the type, named after the type.
constexpr enemy_tier combine(const enemy_tier& tier, const enemy_type& type)
{
    enemy_tier result = tier;
    result.name = type.name;
    result.windup_percent = tier.windup_percent * type.windup_percent / 100;
    result.recover_between_attacks = tier.recover_between_attacks * type.pace_percent / 100;
    result.pause_between_sequences = tier.pause_between_sequences * type.pace_percent / 100;
    result.damage_percent = tier.damage_percent * type.damage_percent / 100;

    int heavy = tier.heavy_percent * type.heavy_scale_percent / 100;
    result.heavy_percent = heavy > 100 ? 100 : heavy;

    int feint = tier.feint_percent * type.feint_scale_percent / 100;
    result.feint_percent = feint > 100 ? 100 : feint;

    result.xp_reward = tier.xp_reward * type.reward_percent / 100;
    result.gold_reward = tier.gold_reward * type.reward_percent / 100;

    int max_attacks = tier.max_attacks + type.max_attacks_change;
    result.max_attacks = max_attacks < sequence_min_attacks ? sequence_min_attacks :
                         max_attacks > sequence_max_attacks ? sequence_max_attacks : max_attacks;
    return result;
}

// The God King grows stronger with each cycle (new game plus); other enemies don't change.
constexpr enemy_tier scale_for_cycle(enemy_tier enemy, int type_index, int cycle)
{
    if(type_index == god_king_type_index)
    {
        enemy.hp_percent = 100 + cycle * god_king_hp_percent_per_cycle;
        enemy.damage_percent = enemy.damage_percent * (100 + cycle * god_king_damage_percent_per_cycle) / 100;
    }

    return enemy;
}

// ---------------------------------------------------------------------------------------------
// Telegraph (placeholder flash and trail).
// ---------------------------------------------------------------------------------------------

constexpr int telegraph_start_flash = 4;       // Enemy flashes when a wind-up begins
constexpr int telegraph_blink_frames = 12;     // Trail blinks during the last frames before impact
constexpr int heavy_glow_pulse = 6;            // Unblockable wind-ups pulse red, switching every N frames

// ---------------------------------------------------------------------------------------------
// Feints: a fake wind-up that switches into the real swing.
// ---------------------------------------------------------------------------------------------

constexpr int feint_switch_percent = 55;       // Fake shown for this share of its own wind-up
constexpr int feint_followup_percent = 80;     // Real swing's wind-up after the switch, vs. normal
constexpr int feint_min_followup = 24;         // Never less than this after the switch
constexpr int feint_read_frames = 12;          // Guaranteed reading time before the dodge window

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

constexpr int shield_pips = 4;                 // Starter shield's pips (the roster's shield sets them)
constexpr int shield_pips_per_block = 1;
constexpr int shield_break_stagger = 60;       // Hero can't act after the shield empties
constexpr int shield_regen_frames = 240;       // Starter shield's refill (the roster's shield sets it)

// ---------------------------------------------------------------------------------------------
// Openings (the player's turn to attack).
// ---------------------------------------------------------------------------------------------

constexpr int opening_after_dodge = 60;
constexpr int opening_after_parry = 150;
constexpr int opening_after_perfect = 180;     // Every attack in the sequence parried
constexpr int opening_after_shield_break = 180; // For enemy shields (not built yet)
constexpr int opening_warning_frames = 30;     // Enemy blinks before raising the guard again

constexpr int player_attack_cooldown = 15;     // Frames between slashes

// Weapon elements, triggered when the weapon's combo completes.
constexpr int fire_burn_percent = 200;         // Burn damage, as a share of the weapon's damage
constexpr int fire_burn_frames = 180;          // Spread over 3 seconds...
constexpr int fire_burn_ticks = 6;             // ...in this many ticks
constexpr int ice_opening_bonus = 60;          // Frames added to the opening
constexpr int shock_after_stun = 45;           // Frames he stays stunned after the opening ends

// ---------------------------------------------------------------------------------------------
// Super meter (L when full). Empties when the duel ends.
// ---------------------------------------------------------------------------------------------

constexpr int super_meter_max = 100;
constexpr int super_gain_parry = 20;           // Per successful parry
constexpr int super_gain_hit = 5;              // Per slash that lands
constexpr int super_open_frames = 180;         // Enemy stunned and open after a super
constexpr int super_damage_percent = 200;      // Slash damage during a super opening

// ---------------------------------------------------------------------------------------------
// Magic (hold R, enter the ring's D-pad pattern). Rings and the Magic stat set charges later.
// ---------------------------------------------------------------------------------------------

// Pressing R spends a charge and slows the enemy while the pattern is drawn. Every spell
// that fires knocks the enemy out of his sequence, so casting never drops the hero back into
// a half-finished swing.
constexpr int magic_charges = 2;               // Starter ring's charges (the roster's ring sets them)
constexpr int magic_input_frames = 300;        // Real time to finish the pattern (5 s) before it fizzles
constexpr int magic_slow_divisor = 5;          // While drawing, the enemy moves 1 frame in every 5 (20%)
constexpr int magic_cast_frames = 20;          // Hero is busy casting after the pattern completes
constexpr int fire_damage = 100;
constexpr int fire_stagger_frames = 45;        // Enemy reels, then starts a new sequence
constexpr int ice_freeze_frames = 120;         // Frozen enemy is open, sequence cancelled
constexpr int heal_percent = 30;               // Of the hero's max HP
constexpr int heal_reset_frames = 45;          // Enemy reels from the flash and resets
constexpr int shock_damage = 40;
constexpr int shock_stagger_frames = 90;       // Longest reset: trades damage for breathing room

// ---------------------------------------------------------------------------------------------
// Health and damage.
// ---------------------------------------------------------------------------------------------

constexpr int player_max_hp = 100;

// Hero damage runs on a larger scale than enemy damage so each Attack point (+2 per hit in the
// progression spec) is a small, steady gain: the starting sword does 20 per slash.
constexpr int player_slash_damage = 20;        // Starter weapon's damage (the roster's weapon sets it)

// ---------------------------------------------------------------------------------------------
// Hero stats (progression spec). Points come from levels and first-time item mastery.
// ---------------------------------------------------------------------------------------------

constexpr int level_cap = 40;
constexpr int xp_curve_base = 100;             // XP to the next level = 100 x level^1.5
constexpr int stat_points_per_level = 1;

// Item mastery: XP to master = base + price / divisor, so cheap items master in a few duels
// and endgame items take 15 or more. The first mastery of an item grants stat points.
constexpr int mastery_base_xp = 400;
constexpr int mastery_price_divisor = 5;
constexpr int mastery_stat_points = 1;

// Shop (progression spec): selling returns part of the price.
constexpr int sell_percent = 50;
constexpr int sell_percent_mastered = 75;
constexpr int hp_per_health_point = 10;
constexpr int damage_per_attack_point = 2;
constexpr int shield_points_per_pip = 3;
constexpr int max_shield_pips = 6;             // Spec: 3 to 6
constexpr int shield_points_per_parry_frame = 5;
constexpr int max_parry_window = 12;           // Never wider than the dodge window
constexpr int magic_points_per_charge = 3;
constexpr int max_magic_charges = 6;
constexpr int spell_percent_per_magic_point = 5;
constexpr int hp_per_bar_segment = 50;         // The HP bar grows 8 px per this much max HP over 100

constexpr int damage_diagonal_slash = 12;
constexpr int damage_horizontal_slash = 12;
constexpr int damage_overhead_chop = 16;
constexpr int damage_low_sweep = 10;
constexpr int damage_thrust = 14;
constexpr int damage_rising_slash = 14;
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
constexpr int combo_finisher_shake = 5;        // Enemy shake on the slash that completes a combo
constexpr int burn_flicker_frames = 3;         // Red flicker on each burn tick

// Impact feel: hit-stop freezes the duel for a few frames; shake moves the arena, enemy and
// hero (not the HUD); sparks burst at the point of contact.
constexpr int hitstop_parry = 6;
constexpr int hitstop_combo_finisher = 5;
constexpr int hitstop_super = 8;
constexpr int hitstop_shield_break = 6;
constexpr int hitstop_hurt = 3;
constexpr int shake_hurt = 3;                  // Pixels...
constexpr int shake_hurt_frames = 8;           // ...for this long
constexpr int shake_shield_break = 4;
constexpr int shake_shield_break_frames = 14;
constexpr int shake_super = 2;
constexpr int shake_super_frames = 10;
constexpr int shake_parry = 2;
constexpr int shake_parry_frames = 4;
constexpr int spark_frame_time = 3;            // Frames per spark animation frame

// Damage numbers: rise this far over this long, blinking at the end.
constexpr int damage_number_frames = 36;
constexpr int damage_number_rise = 18;
constexpr int damage_number_blink = 10;

// Sound effects (no music in version 1).
constexpr int sfx_volume_percent = 80;

// Screen transitions: every change of screen fades to black and back.
constexpr int transition_fade_frames = 12;     // Each way
constexpr int popup_frames = 45;               // "COMBO" text and similar popups
constexpr int super_full_blink = 8;            // Full super meter blinks, switching every N frames
constexpr int defeat_sink_frames = 40;         // The hero sinks out of view over this many frames
constexpr int defeat_sink_distance = 48;
constexpr int enemy_kneel_frame = 14;          // A beaten enemy staggers, then kneels at this frame...
constexpr int enemy_fade_start = 40;           // ...then fades out
constexpr int enemy_fade_frames = 40;
constexpr int result_prompt_delay = 60;        // Before "Press START" appears on win/lose

// Duel rewards (progression spec): bonuses on duel XP, earned anywhere in the duel.
constexpr int perfect_sequence_xp_bonus = 25;  // Percent, for at least one perfect parry sequence
constexpr int no_block_xp_bonus = 10;          // Percent, for finishing without blocking

}

#endif
