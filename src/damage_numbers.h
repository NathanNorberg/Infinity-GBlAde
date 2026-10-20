#ifndef IGB_DAMAGE_NUMBERS_H
#define IGB_DAMAGE_NUMBERS_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

namespace igb
{

// Numbers that pop up where a hit lands, rise and blink out. White for hits on the enemy, gold
// for big hits (finishers, super, spells, burn), red for damage to the hero, green for healing
// (shown with a plus sign).
class damage_numbers
{

public:
    enum class color
    {
        WHITE,
        GOLD,
        RED,
        GREEN
    };

    void spawn(int value, bn::fixed_point position, color number_color);

    // Call once per frame.
    void update();

private:
    static constexpr int max_numbers = 4;  // Keeps the duel well under the 128-sprite limit
    static constexpr int max_glyphs = 5;   // Plus sign and up to four digits

    struct number
    {
        bn::vector<bn::sprite_ptr, max_glyphs> glyphs;
        bn::fixed_point position;
        int age;
    };

    bn::vector<number, max_numbers> _numbers;
};

}

#endif
