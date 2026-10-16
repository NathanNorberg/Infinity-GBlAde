#ifndef IGB_STORY_SCENE_H
#define IGB_STORY_SCENE_H

#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "hero_profile.h"
#include "scene.h"

namespace igb
{

// Short scenes between runs. NEW_GENERATION: after dying in the castle, the next generation
// walks toward the gate. ENDING: after beating the God King, the next cycle begins.
// A or Start skips once the text is up; both return to the gate.
class story_scene
{

public:
    story_scene(scene_id which, const hero_profile& profile);

    [[nodiscard]] scene_id update();

private:
    scene_id _which;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _hero;
    bn::sprite_text_generator _text_generator;
    bn::vector<bn::sprite_ptr, 40> _sprites;
    int _frames = 0;
};

}

#endif
