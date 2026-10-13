#include "attack_data.h"

// Compile-time checks so a bad edit to tuning.h or the attack table fails the build
// instead of producing an unwinnable duel.

namespace igb
{

namespace
{
    constexpr bool windows_fit_every_windup()
    {
        for(const tuning::enemy_tier* tier : tuning::enemy_tiers)
        {
            for(int index = 0; index < int(attack_id::COUNT); ++index)
            {
                int frames = windup_frames(attack_step{ attack_id(index) }, *tier);

                if(frames < tuning::windup_min || frames < tuning::dodge_window)
                {
                    return false;
                }
            }
        }

        return true;
    }

    constexpr bool tiers_are_valid()
    {
        for(const tuning::enemy_tier* tier : tuning::enemy_tiers)
        {
            if(tier->max_attacks < tuning::sequence_min_attacks || tier->max_attacks > tuning::sequence_max_attacks)
            {
                return false;
            }
        }

        return true;
    }

    // Slashes are introduced once: no tier may drop them after an earlier tier used them.
    constexpr bool slashes_never_disappear()
    {
        bool seen = false;

        for(const tuning::enemy_tier* tier : tuning::enemy_tiers)
        {
            if(seen && ! tier->diagonal_slashes)
            {
                return false;
            }

            seen = seen || tier->diagonal_slashes;
        }

        return true;
    }

    // Dodge and parry may share a direction: B + direction parries, the D-pad alone dodges.
    constexpr bool every_attack_has_a_dodge()
    {
        for(const attack_type& type : attack_types)
        {
            if(type.dodge_answers == dir::NONE)
            {
                return false;
            }
        }

        return true;
    }
}

static_assert(tuning::parry_window > 0 && tuning::parry_window <= tuning::dodge_window,
              "Parry window must be positive and no wider than the dodge window");
static_assert(tuning::dodge_window <= tuning::windup_min,
              "Dodge window must fit inside the shortest wind-up");
static_assert(tuning::dodge_duration > tuning::dodge_window,
              "A dodge started at the edge of the window must still cover the impact");
static_assert(tuning::parry_commit > tuning::parry_window,
              "A parry started at the edge of the window must still be active at impact");
static_assert(windows_fit_every_windup(), "A tier's wind-up is shorter than the dodge window or windup_min");
static_assert(tiers_are_valid(), "A tier's max_attacks is outside the sequence limits");
static_assert(slashes_never_disappear(), "Once a tier throws diagonal slashes, every later tier must too");
static_assert(every_attack_has_a_dodge(), "Every attack needs a dodge answer");
static_assert(tuning::sequence_min_attacks >= 1 &&
              tuning::sequence_min_attacks <= tuning::sequence_max_attacks,
              "Sequence length range is invalid");
static_assert(tuning::opening_after_dodge <= tuning::opening_after_parry &&
              tuning::opening_after_parry <= tuning::opening_after_perfect,
              "Openings should grow with risk: dodge, then parry, then perfect");
static_assert(tuning::enemy_hit_flash <= tuning::enemy_hit_frames && tuning::hit_streak_frames <= tuning::enemy_hit_frames,
              "Hit flash and streak must fit inside the hit reaction");
static_assert(tuning::opening_warning_frames < tuning::opening_after_dodge,
              "The closing warning must fit inside the shortest opening");
static_assert(tuning::player_attack_cooldown < tuning::opening_after_dodge,
              "At least one slash must fit in the shortest opening");

static_assert(damage(attack_step{ attack_id::OVERHEAD_CHOP, true }, tuning::tier_elite) == 24, "Heavy damage math");
static_assert(damage(attack_step{ attack_id::OVERHEAD_CHOP }, tuning::tier_gate_guard) == 12, "Gate guard hits softer");
static_assert(tuning::first_duel_tier >= 0 && tuning::first_duel_tier < tuning::enemy_tier_count,
              "first_duel_tier must index enemy_tiers");
static_assert(! blockable(attack_step{ attack_id::OVERHEAD_CHOP, true }), "Heavy attacks are unblockable");
static_assert(parry_answer(attack_step{ attack_id::THRUST }) == dir::NONE, "Thrust is not parryable");
static_assert(parries(dir::UP_LEFT, attack_step{ attack_id::DIAGONAL_SLASH_FROM_LEFT }), "/ needs up-left");
static_assert(! parries(dir::LEFT, attack_step{ attack_id::DIAGONAL_SLASH_FROM_LEFT }), "/ rejects plain left");
static_assert(! parries(dir::UP_LEFT, attack_step{ attack_id::HORIZONTAL_FROM_LEFT }), "Horizontal rejects up-left");

}
