#include "story_scene.h"

#include "bn_keypad.h"
#include "bn_sstream.h"
#include "bn_string.h"

#include "bn_regular_bg_items_area_gate.h"
#include "bn_regular_bg_items_area_throne_room.h"
#include "bn_sprite_items_hero.h"

#include "common_variable_8x8_sprite_font.h"

namespace igb
{

namespace
{
    constexpr int walk_frames = 150;           // The new generation walks away from the camera
    constexpr int skip_after = 60;             // A/Start skips after this many frames
    constexpr int auto_finish = 360;

    // The hero walks from the lower left up the road to the gate, shrinking with distance.
    constexpr bn::fixed_point walk_from(-84, 60);
    constexpr bn::fixed_point walk_to(0, -2);
    constexpr bn::fixed walk_end_scale = 0.35;

    constexpr bn::fixed_point ending_hero(0, 30);
    constexpr bn::fixed ending_scale = 0.7;

    constexpr int first_line_y = -60;
    constexpr int line_spacing = 12;
}

story_scene::story_scene(scene_id which, const hero_profile& profile) :
    _which(which),
    _bg((which == scene_id::ENDING ? bn::regular_bg_items::area_throne_room : bn::regular_bg_items::area_gate)
            .create_bg(0, 0)),
    _hero(bn::sprite_items::hero.create_sprite(which == scene_id::ENDING ? ending_hero : walk_from)),
    _text_generator(common::variable_8x8_sprite_font)
{
    _text_generator.set_center_alignment();
    bn::string<48> text;

    if(which == scene_id::ENDING)
    {
        _hero.set_scale(ending_scale);
        _text_generator.generate(0, first_line_y, "The God King falls.", _sprites);

        bn::ostringstream stream(text);
        stream << "Cycle " << (profile.cycle + 1) << " begins. He returns stronger.";
        _text_generator.generate(0, first_line_y + line_spacing, text, _sprites);
    }
    else
    {
        _text_generator.generate(0, first_line_y, "Your hero has fallen.", _sprites);

        {
            bn::ostringstream stream(text);
            stream << "Bloodline " << profile.bloodline << " takes up the sword.";
        }

        _text_generator.generate(0, first_line_y + line_spacing, text, _sprites);

        if(profile.last_gold_lost > 0)
        {
            text.clear();
            bn::ostringstream stream(text);
            stream << "-" << profile.last_gold_lost << " gold";
            _text_generator.generate(0, first_line_y + 2 * line_spacing, text, _sprites);
        }
    }
}

scene_id story_scene::update()
{
    ++_frames;

    if(_which == scene_id::NEW_GENERATION && _frames <= walk_frames)
    {
        bn::fixed progress = bn::fixed(_frames) / walk_frames;
        _hero.set_position(walk_from + (walk_to - walk_from) * progress);
        _hero.set_scale(1 - (1 - walk_end_scale) * progress);
    }

    bool skip = _frames >= skip_after && (bn::keypad::a_pressed() || bn::keypad::start_pressed());
    return skip || _frames >= auto_finish ? scene_id::HUB : scene_id::NONE;
}

}
