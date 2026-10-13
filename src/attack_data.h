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
    COUNT
};

struct attack_type
{
    const char* name;              // Shown in the debug overlay
    int windup_frames;             // Telegraph length, ending at impact
    int damage;
    int dodge_answers;             // dir:: flags that avoid it (any one of them)
    int parry_answer;              // Exact dir:: for B + direction, may be diagonal; NONE = not parryable
    bool blockable;
    bool diagonal;                 // Only thrown by tiers with diagonal_slashes enabled

    // Telegraph trail along the swing path, in screen coordinates (origin at the center).
    bn::fixed_point swing_from;
    bn::fixed_point swing_to;
};

// Indexed by attack_id, in the same order as the enemy pose art. Sides are screen sides.
// Diagonals: dodge away from where the swing finishes, which is toward the side it starts on.
// Parries point toward where the blade comes from, so diagonal slashes need diagonal parries.
constexpr attack_type attack_types[] = {
    { "Slash /", tuning::windup_diagonal_slash, tuning::damage_diagonal_slash,
      dir::LEFT, dir::UP_LEFT, true, true, bn::fixed_point(-62, -70), bn::fixed_point(58, 48) },

    { "Slash \\", tuning::windup_diagonal_slash, tuning::damage_diagonal_slash,
      dir::RIGHT, dir::UP_RIGHT, true, true, bn::fixed_point(62, -70), bn::fixed_point(-58, 48) },

    { "Horiz <", tuning::windup_horizontal_slash, tuning::damage_horizontal_slash,
      dir::DOWN, dir::LEFT, true, false, bn::fixed_point(-98, -14), bn::fixed_point(98, -2) },

    { "Horiz >", tuning::windup_horizontal_slash, tuning::damage_horizontal_slash,
      dir::DOWN, dir::RIGHT, true, false, bn::fixed_point(98, -14), bn::fixed_point(-98, -2) },

    { "Overhead", tuning::windup_overhead_chop, tuning::damage_overhead_chop,
      dir::LEFT | dir::RIGHT, dir::UP, true, false, bn::fixed_point(0, -76), bn::fixed_point(0, 58) },

    { "Sweep", tuning::windup_low_sweep, tuning::damage_low_sweep,
      dir::UP, dir::DOWN, true, false, bn::fixed_point(-90, 50), bn::fixed_point(90, 50) },

    { "Thrust", tuning::windup_thrust, tuning::damage_thrust,
      dir::LEFT | dir::RIGHT, dir::NONE, true, false, bn::fixed_point(0, 0), bn::fixed_point(0, 44) },
};

static_assert(sizeof(attack_types) / sizeof(attack_types[0]) == int(attack_id::COUNT),
              "attack_types must have one entry per attack_id");

// One swing inside an enemy sequence. A heavy swing uses its base attack's dodge rule
// but can't be parried or blocked (blocking it breaks the shield).
struct attack_step
{
    attack_id id;
    bool heavy = false;
};

constexpr const attack_type& attack_type_of(attack_id id)
{
    return attack_types[int(id)];
}

constexpr int windup_frames(attack_step step, const tuning::enemy_tier& tier)
{
    int frames = attack_type_of(step.id).windup_frames * tier.windup_percent / 100;
    return step.heavy ? frames + tuning::windup_heavy_extra : frames;
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
