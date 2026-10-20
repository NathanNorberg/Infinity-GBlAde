#ifndef IGB_AREA_ART_H
#define IGB_AREA_ART_H

#include "bn_regular_bg_items_area_barracks.h"
#include "bn_regular_bg_items_area_garden.h"
#include "bn_regular_bg_items_area_gate.h"
#include "bn_regular_bg_items_area_great_hall.h"
#include "bn_regular_bg_items_area_throne_approach.h"
#include "bn_regular_bg_items_area_throne_room.h"

#include "castle.h"

namespace igb
{

// Each castle area's background (tools/make_background_art.py), used both behind its duels and
// as its area screen. Indexed by area.
inline constexpr const bn::regular_bg_item* area_backgrounds[] = {
    &bn::regular_bg_items::area_gate,
    &bn::regular_bg_items::area_garden,
    &bn::regular_bg_items::area_barracks,
    &bn::regular_bg_items::area_great_hall,
    &bn::regular_bg_items::area_throne_approach,
    &bn::regular_bg_items::area_throne_room,
};

static_assert(sizeof(area_backgrounds) / sizeof(area_backgrounds[0]) == castle::area_count);

}

#endif
