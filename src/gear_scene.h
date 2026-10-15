#ifndef IGB_GEAR_SCENE_H
#define IGB_GEAR_SCENE_H

#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "hero_profile.h"
#include "scene.h"

namespace igb
{

// The equipment screen and the shop share one layout: L/R switch slot, Up/Down pick an item,
// details at the bottom, B back to the gate.
//   Equipment: lists owned items; A equips.
//   Shop: lists every item; A buys (unlocked, affordable, not owned) or sells (owned, not worn,
//   not a starter), each after a confirm.
class gear_scene
{

public:
    enum class mode
    {
        EQUIPMENT,
        SHOP
    };

    gear_scene(mode screen_mode, hero_profile& profile);

    [[nodiscard]] scene_id update();

private:
    static constexpr int max_rows = 8;

    mode _mode;
    hero_profile& _profile;
    bn::sprite_text_generator _text_generator;
    bn::vector<bn::sprite_ptr, 112> _sprites;
    int _slot = 0;
    int _cursor = 0;
    int _rows[max_rows] = {};      // Item indices listed for the current slot
    int _row_count = 0;
    bool _confirming = false;

    void _list_items();
    void _redraw();
};

}

#endif
