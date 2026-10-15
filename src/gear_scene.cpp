#include "gear_scene.h"

#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_keypad.h"
#include "bn_sstream.h"
#include "bn_string.h"

#include "common_variable_8x8_sprite_font.h"

namespace igb
{

namespace
{
    // Screen origin is the center.
    constexpr int title_x = -112;
    constexpr int gold_x = 112;
    constexpr int top_y = -70;
    constexpr int tabs_y = -56;
    constexpr int name_x = -104;
    constexpr int stat_x = 16;
    constexpr int status_x = 112;
    constexpr int first_row_y = -42;
    constexpr int row_spacing = 11;
    constexpr int details_y = 48;
    constexpr int details_spacing = 10;
    constexpr int help_y = 70;

    using text_line = bn::string<56>;

    [[nodiscard]] const char* direction_letter(int direction)
    {
        return direction == dir::UP ? "U" : direction == dir::DOWN ? "D" : direction == dir::LEFT ? "L" : "R";
    }

    [[nodiscard]] const char* element_name(element element)
    {
        return element == element::FIRE ? "Fire" : element == element::ICE ? "Ice" :
               element == element::SHOCK ? "Shock" : "";
    }

    // Short stat for the list row.
    void write_row_stat(bn::ostringstream& stream, const item& item)
    {
        switch(item.slot)
        {

        case item_slot::WEAPON:
            stream << item.damage << " dmg";
            break;

        case item_slot::SHIELD:
            stream << item.pips << " pips";
            break;

        case item_slot::ARMOR:
            stream << "-" << item.reduction;
            break;

        case item_slot::HELMET:
            stream << "-" << item.reduction << " +" << item.max_hp_bonus << "hp";
            break;

        default:
            stream << spell_of(item.ring_spell).name << " x" << item.charges;
            break;
        }
    }

    // Full stats for the details line.
    void write_details(bn::ostringstream& stream, const item& item)
    {
        switch(item.slot)
        {

        case item_slot::WEAPON:
            {
                const combo& combo = *item.weapon_combo;
                stream << "Damage " << item.damage;

                if(item.weapon_element != element::NONE)
                {
                    stream << " " << element_name(item.weapon_element);
                }

                stream << "  " << combo.name << ":";

                for(int step = 0; step < combo.length; ++step)
                {
                    stream << " " << direction_letter(combo.steps[step]);
                }

                stream << " (+" << combo.bonus_damage << ")";
            }
            break;

        case item_slot::SHIELD:
            stream << item.pips << " pips  parry +" << item.parry_frames << "f  refill "
                   << item.refill_frames / 60 << "." << (item.refill_frames % 60) * 10 / 60 << "s";
            break;

        case item_slot::ARMOR:
            stream << "Reduces every hit by " << item.reduction;
            break;

        case item_slot::HELMET:
            stream << "Reduces hits by " << item.reduction << ", +" << item.max_hp_bonus << " max HP";
            break;

        default:
            stream << spell_of(item.ring_spell).name << "  " << item.charges << " charges  power "
                   << item.power_percent << "%";
            break;
        }
    }
}

gear_scene::gear_scene(mode screen_mode, hero_profile& profile) :
    _mode(screen_mode),
    _profile(profile),
    _text_generator(common::variable_8x8_sprite_font)
{
    bn::bg_palettes::set_transparent_color(bn::color(2, 2, 6));
    _list_items();
    _redraw();
}

scene_id gear_scene::update()
{
    int item_index = _row_count > 0 ? _rows[_cursor] : -1;

    if(_confirming)
    {
        if(bn::keypad::a_pressed())
        {
            const item& item = roster::items[item_index];

            if(_profile.owned[item_index])
            {
                _profile.gold += _profile.sell_price(item_index);
                _profile.owned[item_index] = false;
            }
            else
            {
                _profile.gold -= item.price;
                _profile.owned[item_index] = true;
            }

            _confirming = false;
            _redraw();
        }
        else if(bn::keypad::b_pressed())
        {
            _confirming = false;
            _redraw();
        }

        return scene_id::NONE;
    }

    if(bn::keypad::b_pressed())
    {
        return scene_id::HUB;
    }

    if(bn::keypad::l_pressed() || bn::keypad::r_pressed())
    {
        int step = bn::keypad::r_pressed() ? 1 : int(item_slot::COUNT) - 1;
        _slot = (_slot + step) % int(item_slot::COUNT);
        _list_items();
        _redraw();
    }
    else if(bn::keypad::up_pressed() && _row_count > 0)
    {
        _cursor = (_cursor + _row_count - 1) % _row_count;
        _redraw();
    }
    else if(bn::keypad::down_pressed() && _row_count > 0)
    {
        _cursor = (_cursor + 1) % _row_count;
        _redraw();
    }
    else if(bn::keypad::a_pressed() && item_index >= 0)
    {
        if(_mode == mode::EQUIPMENT)
        {
            _profile.equipped[_slot] = item_index;
            _redraw();
        }
        else if(_profile.can_buy(item_index) || _profile.can_sell(item_index))
        {
            _confirming = true;
            _redraw();
        }
    }

    return scene_id::NONE;
}

void gear_scene::_list_items()
{
    // Equipment lists owned items; the shop lists the whole slot. Roster order is price order.
    _row_count = 0;
    _cursor = 0;

    for(int index = 0; index < roster::item_count && _row_count < max_rows; ++index)
    {
        const item& item = roster::items[index];

        if(int(item.slot) == _slot && (_mode == mode::SHOP || _profile.owned[index]))
        {
            if(_profile.wearing(index))
            {
                _cursor = _row_count;
            }

            _rows[_row_count] = index;
            ++_row_count;
        }
    }
}

void gear_scene::_redraw()
{
    _sprites.clear();
    text_line text;

    _text_generator.set_left_alignment();
    _text_generator.generate(title_x, top_y, _mode == mode::SHOP ? "SHOP" : "EQUIPMENT", _sprites);

    {
        bn::ostringstream stream(text);
        stream << _profile.gold << " gold";
    }

    _text_generator.set_right_alignment();
    _text_generator.generate(gold_x, top_y, text, _sprites);

    text.clear();

    {
        bn::ostringstream stream(text);
        stream << "L  < " << slot_names[_slot] << " >  R";
    }

    _text_generator.set_center_alignment();
    _text_generator.generate(0, tabs_y, text, _sprites);

    for(int row = 0; row < _row_count; ++row)
    {
        int index = _rows[row];
        const item& item = roster::items[index];
        int y = first_row_y + row * row_spacing;

        text.clear();

        {
            bn::ostringstream stream(text);
            stream << (row == _cursor ? "> " : "   ") << item.name;
        }

        _text_generator.set_left_alignment();
        _text_generator.generate(name_x, y, text, _sprites);

        text.clear();

        {
            bn::ostringstream stream(text);
            write_row_stat(stream, item);
        }

        _text_generator.generate(stat_x, y, text, _sprites);

        text.clear();

        {
            bn::ostringstream stream(text);

            if(_profile.wearing(index))
            {
                stream << "worn";
            }
            else if(_profile.owned[index])
            {
                stream << "owned";
            }
            else if(_profile.level < item.unlock_level)
            {
                stream << "Lv " << item.unlock_level;
            }
            else
            {
                stream << item.price << "g";
            }

            if(_profile.mastered(index) && _profile.owned[index])
            {
                stream << " M";
            }
        }

        _text_generator.set_right_alignment();
        _text_generator.generate(status_x, y, text, _sprites);
    }

    _text_generator.set_center_alignment();

    if(_row_count == 0)
    {
        return;
    }

    int index = _rows[_cursor];
    const item& item = roster::items[index];
    text.clear();

    {
        bn::ostringstream stream(text);
        write_details(stream, item);
    }

    _text_generator.generate(0, details_y, text, _sprites);
    text.clear();

    {
        bn::ostringstream stream(text);

        if(_profile.mastered(index))
        {
            stream << "Mastered";
        }
        else
        {
            stream << "Mastery " << _profile.item_xp[index] << "/" << mastery_xp(item) << "   XP x" << item.xp_multiplier;
        }

        if(_mode == mode::SHOP && _profile.can_sell(index))
        {
            stream << "   sells for " << _profile.sell_price(index);
        }
    }

    _text_generator.generate(0, details_y + details_spacing, text, _sprites);
    text.clear();

    {
        bn::ostringstream stream(text);

        if(_confirming)
        {
            stream << (_profile.owned[index] ? "Sell for " : "Buy for ")
                   << (_profile.owned[index] ? _profile.sell_price(index) : item.price) << " gold?  A: yes  B: no";
        }
        else if(_mode == mode::EQUIPMENT)
        {
            stream << "A: equip    B: back";
        }
        else if(_profile.can_buy(index))
        {
            stream << "A: buy    B: back";
        }
        else if(_profile.can_sell(index))
        {
            stream << "A: sell    B: back";
        }
        else if(! _profile.owned[index] && _profile.level < item.unlock_level)
        {
            stream << "Unlocks at level " << item.unlock_level << "    B: back";
        }
        else if(! _profile.owned[index])
        {
            stream << "Not enough gold    B: back";
        }
        else
        {
            stream << "B: back";
        }
    }

    _text_generator.generate(0, help_y, text, _sprites);
}

}
