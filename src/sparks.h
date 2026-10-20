#ifndef IGB_SPARKS_H
#define IGB_SPARKS_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

namespace igb
{

// Short burst animations at points of impact: gold for parries and combo finishers, blue for
// blocks. A few can play at once; each plays its four frames and disappears.
class sparks
{

public:
    enum class color
    {
        GOLD,
        BLUE
    };

    void spawn(bn::fixed_point position, color burst_color);

    // Call once per frame.
    void update();

private:
    static constexpr int max_sparks = 4;

    struct spark
    {
        bn::sprite_ptr sprite;
        int first_frame;
        int age;
    };

    bn::vector<spark, max_sparks> _sparks;
};

}

#endif
