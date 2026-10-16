#ifndef IGB_CASTLE_H
#define IGB_CASTLE_H

// The castle route: five areas with one fork, their enemy rosters, treasure and hotspot
// layout. Like tuning.h, this is the file to edit when tuning the castle. The design is in
// docs/Infinity Blade GBA Progression Spec.md (Castle route).

#include "tuning.h"

namespace igb::castle
{

enum class hotspot_kind
{
    DUEL,           // An enemy blocking the way
    GOLD,           // Gold pouch
    CHEST,          // Gold or an item
    LOCKED_CHEST,   // Needs a key; holds a rare item
    EXIT            // Path onward (open once every duel in the area is won)
};

struct hotspot
{
    hotspot_kind kind;
    int x;                         // Marker position on the area screen (origin at the center)
    int y;
    int target_area = -1;          // EXIT only: where the path leads
};

constexpr int max_hotspots = 7;
constexpr int max_area_types = 3;

struct area
{
    const char* name;
    int tier_index;                // Into tuning::enemy_tiers
    int types[max_area_types];     // Into tuning::enemy_types; each duel rolls one per run
    int type_count;
    int gold_min;                  // Gold pouch range; chests with gold pay double
    int gold_max;
    int chest_item_percent;        // Chance a chest holds an item instead of gold
    int chest_max_price;           // Most expensive item a plain chest can hold
    int key_drop_percent;          // Chance each duel won here drops a key
    hotspot hotspots[max_hotspots];
    int hotspot_count;
};

// Area indices.
constexpr int gate = 0;
constexpr int garden = 1;
constexpr int barracks = 2;
constexpr int great_hall = 3;
constexpr int throne_approach = 4;
constexpr int throne_room = 5;

// Type indices (tuning::enemy_types).
constexpr int knight = 0;
constexpr int brute = 1;
constexpr int duelist = 2;

using kind = hotspot_kind;

inline constexpr area areas[] = {
    // Gate and courtyard: easy guards that teach dodge, block and straight parries. No Brute,
    // which kills a fresh hero in about 7 hits. The fork is chosen here.
    { "Gate and courtyard", 0, { knight, duelist }, 2, 20, 40, 0, 0, 0,
      { { kind::DUEL, -56, 12 }, { kind::DUEL, 24, -12 }, { kind::GOLD, 72, 28 },
        { kind::EXIT, -96, -34, garden }, { kind::EXIT, 96, -34, barracks } }, 5 },

    // Garden path: fewer fights, more treasure; first diagonal slashes.
    { "Garden path", 1, { knight, brute, duelist }, 3, 40, 80, 40, 2500, 0,
      { { kind::DUEL, -64, 16 }, { kind::DUEL, 32, -18 }, { kind::CHEST, -20, 32 }, { kind::CHEST, 80, 22 },
        { kind::GOLD, -92, -24 }, { kind::EXIT, 100, -32, great_hall } }, 6 },

    // Barracks path: more fights and XP, less treasure; first diagonal slashes.
    { "Barracks path", 1, { knight, brute, duelist }, 3, 30, 60, 0, 0, 0,
      { { kind::DUEL, -80, 18 }, { kind::DUEL, -30, -20 }, { kind::DUEL, 30, 22 }, { kind::DUEL, 80, -14 },
        { kind::GOLD, 0, 40 }, { kind::EXIT, 104, 30, great_hall } }, 6 },

    // Great hall: mid-tier enemies and the first unblockable attacks; a locked chest.
    { "Great hall", 2, { knight, brute, duelist }, 3, 50, 100, 40, 5000, 0,
      { { kind::DUEL, -52, 14 }, { kind::DUEL, 52, 14 }, { kind::CHEST, -92, -20 }, { kind::LOCKED_CHEST, 0, -26 },
        { kind::EXIT, 96, -28, throne_approach } }, 5 },

    // Throne approach: the elite guard (mini-boss), who may drop a key.
    { "Throne approach", 3, { knight, brute }, 2, 60, 100, 0, 0, 50,
      { { kind::DUEL, 0, 12 }, { kind::GOLD, -80, 26 }, { kind::EXIT, 0, -34, throne_room } }, 3 },

    // Throne room: the God King.
    { "Throne room", 4, { tuning::god_king_type_index }, 1, 0, 0, 0, 0, 0,
      { { kind::DUEL, 0, 4 } }, 1 },
};

constexpr int area_count = sizeof(areas) / sizeof(areas[0]);

// A locked chest holds a random unowned item at least this expensive (else gold).
constexpr int rare_item_min_price = 4000;
constexpr int locked_chest_fallback_gold = 500;

constexpr int duel_count(int area_index)
{
    int count = 0;

    for(int index = 0; index < areas[area_index].hotspot_count; ++index)
    {
        count += areas[area_index].hotspots[index].kind == hotspot_kind::DUEL;
    }

    return count;
}

}

#endif
