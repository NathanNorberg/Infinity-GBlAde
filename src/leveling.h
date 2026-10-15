#ifndef IGB_LEVELING_H
#define IGB_LEVELING_H

#include "hero_profile.h"

namespace igb
{

namespace leveling_detail
{
    // Rounded integer square root, for the XP curve.
    constexpr long long rounded_sqrt(long long value)
    {
        long long root = 0;

        while((root + 1) * (root + 1) <= value)
        {
            ++root;
        }

        // Round to nearest: (root + 0.5)^2 = root^2 + root + 0.25.
        return value - root * root > root ? root + 1 : root;
    }
}

// XP needed to go from this level to the next: 100 x level^1.5, rounded (progression spec).
// Computed as sqrt(10000 x level^3) to stay in integers.
constexpr int xp_to_next_level(int level)
{
    long long cube = (long long)(level) * level * level;
    return int(leveling_detail::rounded_sqrt(cube * tuning::xp_curve_base * tuning::xp_curve_base));
}

static_assert(xp_to_next_level(1) == 100, "Level 2 needs 100 XP");
static_assert(xp_to_next_level(20) > 8900 && xp_to_next_level(20) < 9000, "Level 20 needs about 8,900 XP");

// Adds XP to the profile, levelling up (with stat points) as many times as it covers.
// XP stops counting at the level cap. Returns the number of levels gained.
inline int add_xp(hero_profile& profile, int xp)
{
    if(profile.level >= tuning::level_cap)
    {
        return 0;
    }

    profile.xp += xp;
    int levels = 0;

    while(profile.level < tuning::level_cap && profile.xp >= xp_to_next_level(profile.level))
    {
        profile.xp -= xp_to_next_level(profile.level);
        ++profile.level;
        profile.unspent_points += tuning::stat_points_per_level;
        ++levels;
    }

    if(profile.level >= tuning::level_cap)
    {
        profile.xp = 0;
    }

    return levels;
}

}

#endif
