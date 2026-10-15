#include "character_scene.h"

#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_keypad.h"
#include "bn_sstream.h"
#include "bn_string.h"

#include "common_variable_8x8_sprite_font.h"
#include "leveling.h"

namespace igb
{

namespace
{
    constexpr int stat_count = 4;

    // Screen origin is the center.
    constexpr int left_x = -104;
    constexpr int title_y = -64;
    constexpr int level_y = -46;
    constexpr int xp_y = -36;
    constexpr int points_y = -26;
    constexpr int first_stat_y = -6;
    constexpr int stat_spacing = 14;
    constexpr int help_y = 64;

    [[nodiscard]] int& stat_points(hero_profile& profile, int stat)
    {
        return stat == 0 ? profile.health : stat == 1 ? profile.attack : stat == 2 ? profile.shield : profile.magic;
    }
}

character_scene::character_scene(hero_profile& profile) :
    _profile(profile),
    _text_generator(common::variable_8x8_sprite_font)
{
    bn::bg_palettes::set_transparent_color(bn::color(2, 2, 6));
    _redraw();
}

scene_id character_scene::update()
{
    if(bn::keypad::b_pressed() || bn::keypad::start_pressed())
    {
        return scene_id::HUB;
    }

    if(bn::keypad::up_pressed())
    {
        _cursor = (_cursor + stat_count - 1) % stat_count;
        _redraw();
    }
    else if(bn::keypad::down_pressed())
    {
        _cursor = (_cursor + 1) % stat_count;
        _redraw();
    }
    else if(bn::keypad::a_pressed() && _profile.unspent_points > 0)
    {
        ++stat_points(_profile, _cursor);
        --_profile.unspent_points;
        _redraw();
    }

    return scene_id::NONE;
}

void character_scene::_redraw()
{
    _sprites.clear();

    _text_generator.set_center_alignment();
    _text_generator.generate(0, title_y, "CHARACTER", _sprites);
    _text_generator.generate(0, help_y, "Up/Down: choose   A: spend   B: back", _sprites);
    _text_generator.set_left_alignment();

    bn::string<48> text;

    {
        bn::ostringstream stream(text);
        stream << "Level " << _profile.level;
    }

    _text_generator.generate(left_x, level_y, text, _sprites);
    text.clear();

    {
        bn::ostringstream stream(text);

        if(_profile.level >= tuning::level_cap)
        {
            stream << "XP  max level";
        }
        else
        {
            stream << "XP  " << _profile.xp << " / " << xp_to_next_level(_profile.level);
        }
    }

    _text_generator.generate(left_x, xp_y, text, _sprites);
    text.clear();

    {
        bn::ostringstream stream(text);
        stream << "Stat points to spend: " << _profile.unspent_points;
    }

    _text_generator.generate(left_x, points_y, text, _sprites);

    for(int stat = 0; stat < stat_count; ++stat)
    {
        text.clear();
        bn::ostringstream stream(text);
        stream << (stat == _cursor ? "> " : "   ");

        switch(stat)
        {

        case 0:
            stream << "Health " << _profile.health << "   Max HP " << _profile.max_hp();
            break;

        case 1:
            stream << "Attack " << _profile.attack << "   Slash " << _profile.slash_damage();
            break;

        case 2:
            stream << "Shield " << _profile.shield << "   " << _profile.shield_pips() << " pips, parry "
                   << _profile.parry_window() << "f";
            break;

        default:
            stream << "Magic " << _profile.magic << "   " << _profile.magic_charges() << " charges, spells +"
                   << _profile.magic * tuning::spell_percent_per_magic_point << "%";
            break;
        }

        _text_generator.generate(left_x, first_stat_y + stat * stat_spacing, text, _sprites);
    }
}

}
