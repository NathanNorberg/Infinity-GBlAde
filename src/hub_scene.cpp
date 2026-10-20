#include "hub_scene.h"

#include "bn_bg_palette_ptr.h"
#include "bn_colors.h"
#include "bn_keypad.h"
#include "bn_sstream.h"
#include "bn_string.h"

#include "bn_regular_bg_items_area_gate.h"
#include "bn_regular_bg_items_menu_frame.h"

#include "common_variable_8x8_sprite_font.h"
#include "castle_run.h"
#include "leveling.h"
#include "save_data.h"
#include "sfx.h"

namespace igb
{

namespace
{
    enum menu_option
    {
        FIGHT,
        CHARACTER,
        EQUIPMENT,
        SHOP,
        OPTION_COUNT
    };

    constexpr const char* option_names[] = { "Enter the castle", "Character", "Equipment", "Shop" };

    // Screen origin is the center.
    constexpr int title_y = -64;
    constexpr int status_y = -48;
    constexpr int points_y = -38;
    constexpr int menu_left_x = -40;
    constexpr int first_option_y = -16;
    constexpr int option_spacing = 13;
    constexpr int description_y = 44;
    constexpr int help_y = 64;

    constexpr int debug_gold = 1000;
}

hub_scene::hub_scene(hero_profile& profile, int active_slot) :
    _profile(profile),
    _active_slot(active_slot),
    _arena_bg(bn::regular_bg_items::area_gate.create_bg(0, 0)),
    _frame_bg(bn::regular_bg_items::menu_frame.create_bg(0, 0)),
    _text_generator(common::variable_8x8_sprite_font)
{
    bn::bg_palette_ptr palette = _arena_bg.palette();
    palette.set_fade(bn::colors::black, bn::fixed(0.6));
    _frame_bg.set_priority(2);         // In front of the gate
    _redraw();
}

scene_id hub_scene::update()
{
    ++_frames;

    if(_update_debug_grants())
    {
        return scene_id::NONE;
    }

    if(bn::keypad::up_pressed())
    {
        _cursor = (_cursor + OPTION_COUNT - 1) % OPTION_COUNT;
        _debug_message = nullptr;
        sfx::play(bn::sound_items::sfx_menu_move);
        _redraw();
    }
    else if(bn::keypad::down_pressed())
    {
        _cursor = (_cursor + 1) % OPTION_COUNT;
        _debug_message = nullptr;
        sfx::play(bn::sound_items::sfx_menu_move);
        _redraw();
    }
    else if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
    {
        sfx::play(bn::sound_items::sfx_menu_select);

        switch(_cursor)
        {

        case FIGHT:
            // A fresh run: full HP, nothing cleared, enemies and treasure rolled from a new seed.
            run::start(_profile, _frames * 2654435761u ^ unsigned(_profile.bloodline) * 40503u ^
                                 unsigned(_profile.gold));
            save::write(_active_slot, _profile);
            return scene_id::AREA;

        case CHARACTER:
            return scene_id::CHARACTER;

        case EQUIPMENT:
            return scene_id::EQUIPMENT;

        default:
            return scene_id::SHOP;
        }
    }

    return scene_id::NONE;
}

bool hub_scene::_update_debug_grants()
{
    if(! bn::keypad::select_held())
    {
        return false;
    }

    if(bn::keypad::a_pressed())
    {
        _profile.gold += debug_gold;
        _debug_message = "DEBUG: +1000 gold";
    }
    else if(bn::keypad::b_pressed())
    {
        if(_profile.level < tuning::level_cap)
        {
            add_xp(_profile, xp_to_next_level(_profile.level) - _profile.xp);
            _debug_message = "DEBUG: level up";
        }
    }
    else if(bn::keypad::r_pressed())
    {
        for(int index = 0; index < roster::item_count; ++index)
        {
            _profile.owned[index] = true;
        }

        _debug_message = "DEBUG: every item owned";
    }
    else
    {
        // Select held: the menu ignores input so a grant doesn't also pick an option.
        return true;
    }

    save::write(_active_slot, _profile);
    sfx::play(bn::sound_items::sfx_coins);
    _redraw();
    return true;
}

void hub_scene::_redraw()
{
    _sprites.clear();

    _text_generator.set_center_alignment();
    {
        bn::string<48> heading;
        bn::ostringstream stream(heading);
        stream << "THE CASTLE GATE    Bloodline " << _profile.bloodline;

        if(_profile.cycle > 0)
        {
            stream << "    Cycle " << (_profile.cycle + 1);
        }
        _text_generator.generate(0, title_y, heading, _sprites);
    }

    bn::string<48> text;

    {
        bn::ostringstream stream(text);
        stream << "Level " << _profile.level << "    ";

        if(_profile.level >= tuning::level_cap)
        {
            stream << "XP max";
        }
        else
        {
            stream << "XP " << _profile.xp << "/" << xp_to_next_level(_profile.level);
        }

        stream << "    " << _profile.gold << " gold    Keys " << _profile.keys;
    }

    _text_generator.generate(0, status_y, text, _sprites);

    if(_profile.unspent_points > 0)
    {
        text.clear();
        bn::ostringstream stream(text);
        stream << _profile.unspent_points << (_profile.unspent_points == 1 ? " stat point" : " stat points")
               << " to spend";
        _text_generator.generate(0, points_y, text, _sprites);
    }

    _text_generator.set_left_alignment();

    for(int option = 0; option < OPTION_COUNT; ++option)
    {
        text.clear();
        bn::ostringstream stream(text);
        stream << (option == _cursor ? "> " : "   ") << option_names[option];
        _text_generator.generate(menu_left_x, first_option_y + option * option_spacing, text, _sprites);
    }

    text.clear();

    if(_debug_message)
    {
        bn::ostringstream stream(text);
        stream << _debug_message;
    }
    else
    {
        bn::ostringstream stream(text);

        switch(_cursor)
        {

        case FIGHT:
            stream << "Climb to the God King";
            break;

        case CHARACTER:
            stream << "Check stats and spend points";
            break;

        case EQUIPMENT:
            stream << "Change your gear";
            break;

        default:
            stream << "Buy and sell gear";
            break;
        }
    }

    _text_generator.set_center_alignment();
    _text_generator.generate(0, description_y, text, _sprites);
    _text_generator.generate(0, help_y, "Up/Down: choose    A: select", _sprites);
}

}
