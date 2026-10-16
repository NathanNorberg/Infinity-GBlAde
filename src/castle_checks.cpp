#include "castle.h"

// Compile-time checks on the castle data, so a bad edit to include/castle.h fails the build
// instead of producing a castle that can't be finished or breaks the teaching order.

namespace igb
{

namespace
{
    using namespace castle;

    constexpr bool areas_are_valid()
    {
        for(int index = 0; index < area_count; ++index)
        {
            const area& area = areas[index];

            if(area.tier_index < 0 || area.tier_index >= tuning::enemy_tier_count ||
               area.type_count < 1 || area.type_count > max_area_types ||
               area.hotspot_count < 1 || area.hotspot_count > max_hotspots ||
               area.gold_min > area.gold_max || area.key_drop_percent < 0 || area.key_drop_percent > 100)
            {
                return false;
            }

            for(int type = 0; type < area.type_count; ++type)
            {
                if(area.types[type] < 0 || area.types[type] >= tuning::enemy_type_count)
                {
                    return false;
                }
            }

            for(int spot = 0; spot < area.hotspot_count; ++spot)
            {
                const hotspot& hotspot = area.hotspots[spot];

                // Exits only lead forward, so a run always moves toward the throne room.
                if(hotspot.kind == hotspot_kind::EXIT && (hotspot.target_area <= index || hotspot.target_area >= area_count))
                {
                    return false;
                }
            }
        }

        return true;
    }

    // Every area but the throne room has a way onward; the throne room has none.
    constexpr bool route_is_complete()
    {
        for(int index = 0; index < area_count; ++index)
        {
            bool has_exit = false;

            for(int spot = 0; spot < areas[index].hotspot_count; ++spot)
            {
                has_exit = has_exit || areas[index].hotspots[spot].kind == hotspot_kind::EXIT;
            }

            if(has_exit == (index == throne_room))
            {
                return false;
            }
        }

        return true;
    }

    constexpr bool god_king_only_in_throne_room()
    {
        for(int index = 0; index < area_count; ++index)
        {
            for(int type = 0; type < areas[index].type_count; ++type)
            {
                if((areas[index].types[type] == tuning::god_king_type_index) != (index == throne_room))
                {
                    return false;
                }
            }
        }

        return true;
    }
}

static_assert(areas_are_valid(), "A castle area has a bad tier, type, hotspot or treasure value");
static_assert(route_is_complete(), "Every area except the throne room needs an exit onward");
static_assert(god_king_only_in_throne_room(), "The God King type belongs in the throne room, and only there");

// Progression spec: 8 duels through the garden, 10 through the barracks, God King included.
static_assert(duel_count(gate) + duel_count(garden) + duel_count(great_hall) + duel_count(throne_approach) +
              duel_count(throne_room) == 8, "The garden route should be 8 duels");
static_assert(duel_count(gate) + duel_count(barracks) + duel_count(great_hall) + duel_count(throne_approach) +
              duel_count(throne_room) == 10, "The barracks route should be 10 duels");

// Teaching order: no Brute at the gate; unblockables only from the great hall on.
static_assert(areas[gate].types[0] != brute && areas[gate].types[1] != brute, "No Brute at the gate");
static_assert(tuning::enemy_tiers[areas[gate].tier_index]->heavy_percent == 0 &&
              tuning::enemy_tiers[areas[garden].tier_index]->heavy_percent == 0 &&
              tuning::enemy_tiers[areas[barracks].tier_index]->heavy_percent == 0,
              "Unblockable attacks start in the great hall");
static_assert(tuning::enemy_tiers[areas[great_hall].tier_index]->heavy_percent > 0,
              "The great hall brings the first unblockable attacks");

}
