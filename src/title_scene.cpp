#include "title_scene.h"

#include "bn_bg_palette_ptr.h"
#include "bn_colors.h"
#include "bn_keypad.h"
#include "bn_sstream.h"
#include "bn_string.h"

#include "bn_regular_bg_items_title.h"

#include "common_variable_8x8_sprite_font.h"
#include "sfx.h"

namespace igb
{

namespace
{
    // Screen origin is the center.
    constexpr int title_y = -44;
    constexpr int subtitle_y = -30;
    constexpr int slot_left_x = -64;
    constexpr int first_slot_y = -2;
    constexpr int slot_spacing = 14;
    constexpr int help_y = 44;
}

title_scene::title_scene(hero_profile& profile, int& active_slot) :
    _profile(profile),
    _active_slot(active_slot),
    _arena_bg(bn::regular_bg_items::title.create_bg(0, 0)),
    _text_generator(common::variable_8x8_sprite_font)
{
    bn::bg_palette_ptr palette = _arena_bg.palette();
    palette.set_fade(bn::colors::black, bn::fixed(0.25));

    for(int slot = 0; slot < save::slot_count; ++slot)
    {
        _slots[slot] = save::summary(slot);
    }

    _redraw();
}

scene_id title_scene::update()
{
    if(_confirming_erase_all)
    {
        if(bn::keypad::a_pressed())
        {
            for(int slot = 0; slot < save::slot_count; ++slot)
            {
                save::erase(slot);
                _slots[slot] = save::summary(slot);
            }

            _confirming_erase_all = false;
            _redraw();
        }
        else if(bn::keypad::b_pressed())
        {
            _confirming_erase_all = false;
            _redraw();
        }

        return scene_id::NONE;
    }

    if(bn::keypad::select_held() && bn::keypad::start_pressed())
    {
        _confirming_erase_all = true;
        _redraw();
        return scene_id::NONE;
    }

    if(_confirming_new_game)
    {
        if(bn::keypad::a_pressed())
        {
            sfx::play(bn::sound_items::sfx_menu_select);
            _start(true);
            return scene_id::HUB;
        }

        if(bn::keypad::b_pressed())
        {
            _confirming_new_game = false;
            _redraw();
        }

        return scene_id::NONE;
    }

    if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
    {
        sfx::play(bn::sound_items::sfx_menu_move);
        _cursor = (_cursor + 1) % save::slot_count;
        _redraw();
    }
    else if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
    {
        sfx::play(bn::sound_items::sfx_menu_select);
        _start(! _slots[_cursor].used);
        return scene_id::HUB;
    }
    else if(bn::keypad::b_pressed() && _slots[_cursor].used)
    {
        _confirming_new_game = true;
        _redraw();
    }

    return scene_id::NONE;
}

void title_scene::_start(bool new_game)
{
    _active_slot = _cursor;

    if(new_game || ! save::read(_cursor, _profile))
    {
        // A fresh hero, saved right away so the slot shows as used.
        _profile = hero_profile();
        save::write(_cursor, _profile);
    }
}

void title_scene::_redraw()
{
    _sprites.clear();
    _text_generator.set_center_alignment();
    _text_generator.generate(0, title_y, "INFINITY GBLADE", _sprites);
    _text_generator.generate(0, subtitle_y, "A duel at the castle gate", _sprites);

    bn::string<48> text;

    for(int slot = 0; slot < save::slot_count; ++slot)
    {
        text.clear();
        bn::ostringstream stream(text);
        stream << (slot == _cursor ? "> " : "   ") << "Slot " << (slot + 1) << "    ";

        if(_slots[slot].used)
        {
            stream << "Lv " << _slots[slot].level << "    " << _slots[slot].gold << " gold";
        }
        else
        {
            stream << "Empty";
        }

        _text_generator.set_left_alignment();
        _text_generator.generate(slot_left_x, first_slot_y + slot * slot_spacing, text, _sprites);
    }

    text.clear();

    {
        bn::ostringstream stream(text);

        if(_confirming_erase_all)
        {
            stream << "DEBUG: erase both slots?  A: yes  B: no";
        }
        else if(_confirming_new_game)
        {
            stream << "Erase slot " << (_cursor + 1) << " and start over?  A: yes  B: no";
        }
        else if(_slots[_cursor].used)
        {
            stream << "A: continue    B: new game";
        }
        else
        {
            stream << "A: new game";
        }
    }

    _text_generator.set_center_alignment();
    _text_generator.generate(0, help_y, text, _sprites);
}

}
