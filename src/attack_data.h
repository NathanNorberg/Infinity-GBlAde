#ifndef IGB_ATTACK_DATA_H
#define IGB_ATTACK_DATA_H

#include "bn_fixed_point.h"
#include "bn_span.h"

#include "tuning.h"

namespace igb
{

// D-pad directions as bit flags. Dodges accept any of several flags; a parry must match
// one exact direction, which can be a diagonal (two flags together).
namespace dir
{
    constexpr int NONE = 0;
    constexpr int LEFT = 1 << 0;
    constexpr int RIGHT = 1 << 1;
    constexpr int UP = 1 << 2;
    constexpr int DOWN = 1 << 3;

    constexpr int UP_LEFT = UP | LEFT;
    constexpr int UP_RIGHT = UP | RIGHT;
    constexpr int DOWN_LEFT = DOWN | LEFT;
    constexpr int DOWN_RIGHT = DOWN | RIGHT;
}

enum class attack_id
{
    DIAGONAL_SLASH_FROM_LEFT,      // ( / ) weapon raised high left
    DIAGONAL_SLASH_FROM_RIGHT,     // ( \ ) weapon raised high right
    HORIZONTAL_FROM_LEFT,
    HORIZONTAL_FROM_RIGHT,
    OVERHEAD_CHOP,
    LOW_SWEEP,
    THRUST,
    RISING_FROM_LEFT,              // ( / ) weapon dropped low left, rises to high right
    RISING_FROM_RIGHT,             // ( \ ) weapon dropped low right, rises to high left
    COUNT
};

static_assert(int(attack_id::COUNT) == tuning::attack_kinds, "Update tuning::attack_kinds and every type's attack_weights");

// Which tiers throw an attack: basics always, diagonal slashes from hall guards on,
// rising slashes only for late-game tiers.
enum class attack_family
{
    BASIC,
    DIAGONAL,
    RISING
};

struct attack_type
{
    const char* name;              // Shown in the debug overlay
    int windup_frames;             // Telegraph length, ending at impact
    int damage;
    int dodge_answers;             // dir:: flags that avoid it (any one of them)
    int parry_answer;              // Exact dir:: for B + direction, may be diagonal; NONE = not parryable
    bool blockable;
    attack_family family;

    // Telegraph trail along the swing path, in screen coordinates (origin at the center).
    bn::fixed_point swing_from;
    bn::fixed_point swing_to;
};

// Indexed by attack_id, in the same order as the enemy pose art. Sides are screen sides.
// Diagonal slashes coming down: dodge toward the side they start on (away from where they finish).
// Rising slashes: dodge away from the side they start on (decided in play-testing, Oct 13).
// Parries point toward where the blade comes from, so both need diagonal parries
// (up-left/up-right for slashes coming down, down-left/down-right for rising ones).
constexpr attack_type attack_types[] = {
    { "Slash /", tuning::windup_diagonal_slash, tuning::damage_diagonal_slash,
      dir::LEFT, dir::UP_LEFT, true, attack_family::DIAGONAL, bn::fixed_point(-62, -70), bn::fixed_point(58, 48) },

    { "Slash \\", tuning::windup_diagonal_slash, tuning::damage_diagonal_slash,
      dir::RIGHT, dir::UP_RIGHT, true, attack_family::DIAGONAL, bn::fixed_point(62, -70), bn::fixed_point(-58, 48) },

    { "Horiz <", tuning::windup_horizontal_slash, tuning::damage_horizontal_slash,
      dir::DOWN, dir::LEFT, true, attack_family::BASIC, bn::fixed_point(-98, -14), bn::fixed_point(98, -2) },

    { "Horiz >", tuning::windup_horizontal_slash, tuning::damage_horizontal_slash,
      dir::DOWN, dir::RIGHT, true, attack_family::BASIC, bn::fixed_point(98, -14), bn::fixed_point(-98, -2) },

    { "Overhead", tuning::windup_overhead_chop, tuning::damage_overhead_chop,
      dir::LEFT | dir::RIGHT, dir::UP, true, attack_family::BASIC, bn::fixed_point(0, -76), bn::fixed_point(0, 58) },

    { "Sweep", tuning::windup_low_sweep, tuning::damage_low_sweep,
      dir::UP, dir::DOWN, true, attack_family::BASIC, bn::fixed_point(-90, 50), bn::fixed_point(90, 50) },

    { "Thrust", tuning::windup_thrust, tuning::damage_thrust,
      dir::LEFT | dir::RIGHT, dir::NONE, true, attack_family::BASIC, bn::fixed_point(0, 0), bn::fixed_point(0, 44) },

    { "Rising /", tuning::windup_rising_slash, tuning::damage_rising_slash,
      dir::RIGHT, dir::DOWN_LEFT, true, attack_family::RISING, bn::fixed_point(-64, 60), bn::fixed_point(62, -66) },

    { "Rising \\", tuning::windup_rising_slash, tuning::damage_rising_slash,
      dir::LEFT, dir::DOWN_RIGHT, true, attack_family::RISING, bn::fixed_point(64, 60), bn::fixed_point(-62, -66) },
};

// True if a tier throws this attack.
constexpr bool tier_throws(const tuning::enemy_tier& tier, attack_id id)
{
    switch(attack_types[int(id)].family)
    {

    case attack_family::DIAGONAL:
        return tier.diagonal_slashes;

    case attack_family::RISING:
        return tier.rising_slashes;

    default:
        return true;
    }
}

static_assert(sizeof(attack_types) / sizeof(attack_types[0]) == int(attack_id::COUNT),
              "attack_types must have one entry per attack_id");

// One swing inside an enemy sequence. A heavy swing uses its base attack's dodge rule
// but can't be parried or blocked (blocking it breaks the shield). A feint opens with
// feint_from's wind-up, then switches to id's; id is the swing that actually lands.
struct attack_step
{
    attack_id id;
    bool heavy = false;
    bool feint = false;
    attack_id feint_from = attack_id::OVERHEAD_CHOP;
};

constexpr const attack_type& attack_type_of(attack_id id)
{
    return attack_types[int(id)];
}

constexpr int swing_windup_frames(attack_id id, bool heavy, const tuning::enemy_tier& tier)
{
    int frames = attack_type_of(id).windup_frames * tier.windup_percent / 100;
    return heavy ? frames + tuning::windup_heavy_extra : frames;
}

// How long a feint shows the fake wind-up before switching.
constexpr int feint_fake_frames(attack_step step, const tuning::enemy_tier& tier)
{
    return swing_windup_frames(step.feint_from, false, tier) * tuning::feint_switch_percent / 100;
}

// Wind-up left for the real swing after a feint switches.
constexpr int feint_followup_frames(attack_step step, const tuning::enemy_tier& tier)
{
    int frames = swing_windup_frames(step.id, step.heavy, tier) * tuning::feint_followup_percent / 100;
    return frames > tuning::feint_min_followup ? frames : tuning::feint_min_followup;
}

// Total wind-up from the first frame to impact, including any fake.
constexpr int windup_frames(attack_step step, const tuning::enemy_tier& tier)
{
    if(step.feint)
    {
        return feint_fake_frames(step, tier) + feint_followup_frames(step, tier);
    }

    return swing_windup_frames(step.id, step.heavy, tier);
}

constexpr int damage(attack_step step, const tuning::enemy_tier& tier)
{
    int base = attack_type_of(step.id).damage * tier.damage_percent / 100;
    return step.heavy ? base * tuning::heavy_damage_percent / 100 : base;
}

constexpr int dodge_answers(attack_step step)
{
    return attack_type_of(step.id).dodge_answers;
}

constexpr int parry_answer(attack_step step)
{
    return step.heavy ? dir::NONE : attack_type_of(step.id).parry_answer;
}

constexpr bool parries(int parry_direction, attack_step step)
{
    int answer = parry_answer(step);
    return answer != dir::NONE && parry_direction == answer;
}

constexpr bool blockable(attack_step step)
{
    return ! step.heavy && attack_type_of(step.id).blockable;
}

}

#endif
