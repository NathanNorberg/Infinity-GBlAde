#ifndef IGB_HUB_SCENE_H
#define IGB_HUB_SCENE_H

#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "hero_profile.h"
#include "scene.h"

namespace igb
{

// The castle gate between runs: the hero's bloodline, level, XP, gold and keys, and a menu of
// Enter the castle, Character, Equipment and Shop (only available here, between runs).
// Debug grants, holding Select: A +1000 gold, B levels up, R owns every item (saved at once).
class hub_scene
{

public:
    hub_scene(hero_profile& profile, int active_slot);

    [[nodiscard]] scene_id update();

private:
    hero_profile& _profile;
    int _active_slot;
    const char* _debug_message = nullptr;
    bn::regular_bg_ptr _arena_bg;
    bn::sprite_text_generator _text_generator;
    bn::vector<bn::sprite_ptr, 48> _sprites;
    int _cursor = 0;
    unsigned _frames = 0;          // Mixed into the run seed

    [[nodiscard]] bool _update_debug_grants();
    void _redraw();
};

}

#endif
