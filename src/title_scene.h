#ifndef IGB_TITLE_SCENE_H
#define IGB_TITLE_SCENE_H

#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "hero_profile.h"
#include "save_data.h"
#include "scene.h"

namespace igb
{

// Title over the dimmed arena with the two save slots. A on a used slot continues it; A on an
// empty slot starts a new game there; B on a used slot starts over after a confirm. Fills in
// the profile and the active slot, then goes to the gate hub. Debug: Select + Start erases
// both slots after a confirm.
class title_scene
{

public:
    title_scene(hero_profile& profile, int& active_slot);

    [[nodiscard]] scene_id update();

private:
    hero_profile& _profile;
    int& _active_slot;
    bn::regular_bg_ptr _arena_bg;
    bn::sprite_text_generator _text_generator;
    bn::vector<bn::sprite_ptr, 48> _sprites;
    save::slot_summary _slots[save::slot_count];
    int _cursor = 0;
    bool _confirming_new_game = false;
    bool _confirming_erase_all = false;

    void _start(bool new_game);
    void _redraw();
};

}

#endif
