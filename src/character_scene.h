#ifndef IGB_CHARACTER_SCENE_H
#define IGB_CHARACTER_SCENE_H

#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "hero_profile.h"
#include "scene.h"

namespace igb
{

// Level, XP and stats, with what each stat currently gives. Up/Down picks a stat, A spends
// an unspent point on it (permanent), B returns.
class character_scene
{

public:
    explicit character_scene(hero_profile& profile);

    [[nodiscard]] scene_id update();

private:
    hero_profile& _profile;
    bn::regular_bg_ptr _frame_bg;      // Ornate border (graphics/menu_frame.bmp)
    bn::sprite_text_generator _text_generator;
    bn::vector<bn::sprite_ptr, 64> _sprites;
    int _cursor = 0;

    void _redraw();
};

}

#endif
