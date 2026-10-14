#ifndef IGB_COMBO_H
#define IGB_COMBO_H

#include "attack_data.h"

namespace igb
{

// A direction sequence that, slashed in order inside one opening, adds bonus damage to the
// final hit. Weapons bring their own combos in milestone 3.
struct combo
{
    static constexpr int max_steps = 5;

    const char* name;
    int steps[max_steps];          // dir:: values, exact (a diagonal doesn't count as a cardinal)
    int length;
    int bonus_damage;
};

// The hero's starting sword.
constexpr combo starting_combo = { "Cross cut", { dir::LEFT, dir::RIGHT, dir::UP }, 3, tuning::combo_bonus_damage };

// Follows slashes inside one opening and reports when the combo completes.
class combo_tracker
{

public:
    explicit combo_tracker(const combo& combo) :
        _combo(combo)
    {
    }

    // Returns true when this slash completes the combo; progress then starts over.
    [[nodiscard]] bool add(int direction)
    {
        if(direction == _combo.steps[_progress])
        {
            ++_progress;
        }
        else
        {
            // A wrong slash breaks the chain but can start a new one.
            _progress = direction == _combo.steps[0] ? 1 : 0;
        }

        if(_progress == _combo.length)
        {
            _progress = 0;
            return true;
        }

        return false;
    }

    void reset()
    {
        _progress = 0;
    }

    [[nodiscard]] int progress() const
    {
        return _progress;
    }

    [[nodiscard]] const combo& current() const
    {
        return _combo;
    }

private:
    const combo& _combo;
    int _progress = 0;
};

static_assert(starting_combo.length >= 2 && starting_combo.length <= combo::max_steps);
static_assert((starting_combo.length - 1) * tuning::player_attack_cooldown < tuning::opening_after_dodge,
              "The combo must fit inside the shortest opening");

}

#endif
