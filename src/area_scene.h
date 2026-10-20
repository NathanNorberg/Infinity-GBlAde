#ifndef IGB_AREA_SCENE_H
#define IGB_AREA_SCENE_H

#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "castle.h"
#include "hero_profile.h"
#include "scene.h"

namespace igb
{

// One castle area between duels: its background, a marker per hotspot (enemy, gold pouch,
// chest, locked chest, path onward) and the hero's run status. The D-pad moves between
// hotspots; A fights, takes treasure or follows a path once every guard is beaten.
// Debug, holding Select: A refills HP, B gives a key, R jumps to the next area, L raises
// the cycle.
class area_scene
{

public:
    area_scene(hero_profile& profile, int active_slot);

    [[nodiscard]] scene_id update();

private:
    hero_profile& _profile;
    int _active_slot;
    bn::regular_bg_ptr _bg;
    bn::regular_bg_ptr _band_bg;       // Dark title band behind the area name (graphics/area_band.bmp)
    bn::sprite_text_generator _text_generator;
    bn::vector<bn::sprite_ptr, castle::max_hotspots> _markers;
    bn::sprite_ptr _cursor_arrow;
    bn::vector<bn::sprite_ptr, 48> _text_sprites;
    bn::string<48> _message;
    int _cursor = 0;
    int _frames = 0;

    [[nodiscard]] scene_id _act();
    [[nodiscard]] scene_id _update_debug();
    void _open_chest(int hotspot, bool locked);
    void _move_cursor(int dx, int dy);
    void _refresh_markers();
    void _redraw_text();
    void _save();
};

}

#endif
