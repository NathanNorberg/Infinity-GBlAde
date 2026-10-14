#ifndef IGB_MAGIC_H
#define IGB_MAGIC_H

#include "attack_data.h"

namespace igb
{

// The equipped ring decides which spell R casts. Hold R, then enter the spell's D-pad
// pattern (a stand-in for drawing its symbol); the spell fires when the pattern completes.
enum class spell_id
{
    FIRE,       // Direct damage
    ICE,        // Freezes the enemy: an opening that cancels his sequence
    HEAL,       // Restores part of the hero's HP
    SHOCK,      // Interrupts and staggers the enemy, cancelling his sequence
    COUNT
};

struct spell
{
    static constexpr int max_steps = 4;

    const char* name;
    const char* description;       // One line for the pause screen
    int steps[max_steps];          // dir:: cardinal directions, in order
    int length;
};

// Indexed by spell_id.
constexpr spell spells[] = {
    { "Fire", "Burns him and knocks him back", { dir::DOWN, dir::UP, dir::UP }, 3 },          // Flame rising
    { "Ice", "Freezes him: free hits", { dir::LEFT, dir::RIGHT, dir::LEFT }, 3 },             // Frost zigzag
    { "Heal", "Restores HP and resets him", { dir::LEFT, dir::DOWN, dir::RIGHT }, 3 },        // Cup
    { "Shock", "Long stun, light damage", { dir::RIGHT, dir::LEFT, dir::DOWN }, 3 },          // Lightning bolt
};

static_assert(sizeof(spells) / sizeof(spells[0]) == int(spell_id::COUNT), "One spell per spell_id");

constexpr const spell& spell_of(spell_id id)
{
    return spells[int(id)];
}

}

#endif
