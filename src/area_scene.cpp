#include "area_scene.h"

#include "bn_keypad.h"
#include "bn_sstream.h"

#include "bn_sprite_items_arrows.h"
#include "bn_sprite_items_markers.h"

#include "area_art.h"
#include "castle_run.h"
#include "common_variable_8x8_sprite_font.h"
#include "save_data.h"
#include "sfx.h"

namespace igb
{

namespace
{
    using castle::hotspot_kind;



    // Frames in graphics/markers.bmp.
    constexpr int marker_duel = 0;
    constexpr int marker_duel_won = 1;
    constexpr int marker_gold = 2;
    constexpr int marker_gold_taken = 3;
    constexpr int marker_chest = 4;
    constexpr int marker_chest_open = 5;
    constexpr int marker_locked = 6;
    constexpr int marker_exit = 7;
    constexpr int marker_exit_blocked = 8;
    constexpr int marker_god_king = 9;
    constexpr int marker_elite = 10;

    constexpr int cursor_arrow_frame = 5;   // Lit down arrow in graphics/arrows.bmp
    constexpr int cursor_offset = 15;

    // Screen origin is the center.
    constexpr int left_x = -116;
    constexpr int right_x = 116;
    constexpr int top_y = -72;
    constexpr int status_y = -61;
    constexpr int description_y = 58;
    constexpr int message_y = 70;

    // Roll salts, so each use of a hotspot's random number is independent.
    constexpr unsigned salt_gold = 2;
    constexpr unsigned salt_chest_kind = 3;
    constexpr unsigned salt_chest_item = 4;

    [[nodiscard]] int roll_range(unsigned roll, int minimum, int maximum)
    {
        return minimum + int(roll % unsigned(maximum - minimum + 1));
    }
}

area_scene::area_scene(hero_profile& profile, int active_slot) :
    _profile(profile),
    _active_slot(active_slot),
    _bg(area_backgrounds[profile.run_area]->create_bg(0, 0)),
    _text_generator(common::variable_8x8_sprite_font),
    _cursor_arrow(bn::sprite_items::arrows.create_sprite(0, 0, cursor_arrow_frame))
{
    const castle::area& area = run::current_area(profile);

    for(int index = 0; index < area.hotspot_count; ++index)
    {
        const castle::hotspot& hotspot = area.hotspots[index];
        _markers.push_back(bn::sprite_items::markers.create_sprite(hotspot.x, hotspot.y));
    }

    // Start on the first hotspot still to do, so the next enemy is usually under the cursor.
    for(int index = 0; index < area.hotspot_count; ++index)
    {
        if(! run::cleared(profile, index) && area.hotspots[index].kind != hotspot_kind::EXIT)
        {
            _cursor = index;
            break;
        }
    }

    _refresh_markers();
    _redraw_text();
}

scene_id area_scene::update()
{
    ++_frames;

    // The cursor arrow bobs over the selected hotspot.
    const castle::hotspot& selected = run::current_area(_profile).hotspots[_cursor];
    _cursor_arrow.set_position(selected.x, selected.y - cursor_offset - (_frames / 15) % 2);

    if(bn::keypad::select_held())
    {
        return _update_debug();
    }

    if(bn::keypad::left_pressed())
    {
        _move_cursor(-1, 0);
    }
    else if(bn::keypad::right_pressed())
    {
        _move_cursor(1, 0);
    }
    else if(bn::keypad::up_pressed())
    {
        _move_cursor(0, -1);
    }
    else if(bn::keypad::down_pressed())
    {
        _move_cursor(0, 1);
    }
    else if(bn::keypad::a_pressed())
    {
        return _act();
    }

    return scene_id::NONE;
}

scene_id area_scene::_act()
{
    const castle::area& area = run::current_area(_profile);
    const castle::hotspot& hotspot = area.hotspots[_cursor];
    bool done = run::cleared(_profile, _cursor);
    _message.clear();
    bn::ostringstream stream(_message);

    switch(hotspot.kind)
    {

    case hotspot_kind::DUEL:
        if(! done)
        {
            sfx::play(bn::sound_items::sfx_menu_select);
            _profile.run_duel_hotspot = _cursor;
            _save();
            return scene_id::DUEL;
        }
        break;

    case hotspot_kind::GOLD:
        if(! done)
        {
            int gold = roll_range(run::roll(_profile, _cursor, salt_gold), area.gold_min, area.gold_max);
            _profile.gold += gold;
            run::mark_cleared(_profile, _cursor);
            stream << "+" << gold << " gold";
            sfx::play(bn::sound_items::sfx_coins);
            _save();
        }
        break;

    case hotspot_kind::CHEST:
        if(! done)
        {
            _open_chest(_cursor, false);
        }
        break;

    case hotspot_kind::LOCKED_CHEST:
        if(! done)
        {
            if(_profile.keys > 0)
            {
                --_profile.keys;
                _open_chest(_cursor, true);
            }
            else
            {
                stream << "Locked. Elite guards may carry keys.";
                sfx::play(bn::sound_items::sfx_menu_back);
            }
        }
        break;

    default:    // EXIT
        if(run::duels_cleared(_profile))
        {
            sfx::play(bn::sound_items::sfx_menu_select);
            run::enter_area(_profile, hotspot.target_area);
            _save();
            return scene_id::AREA;  // Rebuild for the new area
        }

        stream << "Defeat every guard here first";
        sfx::play(bn::sound_items::sfx_menu_back);
        break;
    }

    _refresh_markers();
    _redraw_text();
    return scene_id::NONE;
}

scene_id area_scene::_update_debug()
{
    if(! bn::keypad::a_pressed() && ! bn::keypad::b_pressed() && ! bn::keypad::l_pressed() &&
       ! bn::keypad::r_pressed())
    {
        return scene_id::NONE;
    }

    _message.clear();
    bn::ostringstream stream(_message);

    if(bn::keypad::a_pressed())
    {
        _profile.run_hp = _profile.max_hp();
        stream << "DEBUG: HP refilled";
    }
    else if(bn::keypad::b_pressed())
    {
        ++_profile.keys;
        stream << "DEBUG: +1 key";
    }
    else if(bn::keypad::l_pressed())
    {
        ++_profile.cycle;
        stream << "DEBUG: cycle " << (_profile.cycle + 1);
    }
    else if(bn::keypad::r_pressed())
    {
        // Through the area's first exit (the garden at the fork).
        const castle::area& area = run::current_area(_profile);

        for(int index = 0; index < area.hotspot_count; ++index)
        {
            if(area.hotspots[index].kind == hotspot_kind::EXIT)
            {
                run::enter_area(_profile, area.hotspots[index].target_area);
                _save();
                return scene_id::AREA;
            }
        }

        stream << "DEBUG: no area after this one";
    }
    else
    {
        return scene_id::NONE;
    }

    _save();
    _redraw_text();
    return scene_id::NONE;
}

void area_scene::_open_chest(int hotspot, bool locked)
{
    const castle::area& area = run::current_area(_profile);
    bn::ostringstream stream(_message);

    // Plain chests: sometimes an item up to the area's price cap, otherwise gold.
    // Locked chests: an unowned rare item if any remain, otherwise a pile of gold.
    bool wants_item = locked || int(run::roll(_profile, hotspot, salt_chest_kind) % 100) < area.chest_item_percent;
    int candidates[roster::max_items];
    int candidate_count = 0;

    if(wants_item)
    {
        for(int index = 0; index < roster::item_count; ++index)
        {
            const item& item = roster::items[index];
            bool fits = locked ? item.price >= castle::rare_item_min_price :
                                 item.price > 0 && item.price <= area.chest_max_price;

            if(fits && ! _profile.owned[index])
            {
                candidates[candidate_count++] = index;
            }
        }
    }

    if(candidate_count > 0)
    {
        int index = candidates[run::roll(_profile, hotspot, salt_chest_item) % unsigned(candidate_count)];
        _profile.owned[index] = true;
        stream << "Found: " << roster::items[index].name << "!";
    }
    else
    {
        int gold = locked ? castle::locked_chest_fallback_gold :
                            2 * roll_range(run::roll(_profile, hotspot, salt_gold), area.gold_min, area.gold_max);
        _profile.gold += gold;
        stream << "+" << gold << " gold";
    }

    run::mark_cleared(_profile, hotspot);
    sfx::play(bn::sound_items::sfx_chest);
    _save();
}

void area_scene::_move_cursor(int dx, int dy)
{
    // Nearest hotspot in the pressed direction, preferring ones close to that line.
    const castle::area& area = run::current_area(_profile);
    const castle::hotspot& from = area.hotspots[_cursor];
    int best = -1;
    int best_score = 0;

    for(int index = 0; index < area.hotspot_count; ++index)
    {
        int ox = area.hotspots[index].x - from.x;
        int oy = area.hotspots[index].y - from.y;
        int along = ox * dx + oy * dy;
        int across = ox * dy - oy * dx;

        if(index == _cursor || along <= 0)
        {
            continue;
        }

        int score = along + 2 * (across < 0 ? -across : across);

        if(best < 0 || score < best_score)
        {
            best = index;
            best_score = score;
        }
    }

    if(best >= 0)
    {
        sfx::play(bn::sound_items::sfx_menu_move);
        _cursor = best;
        _message.clear();
        _redraw_text();
    }
}

void area_scene::_refresh_markers()
{
    const castle::area& area = run::current_area(_profile);
    bool exits_open = run::duels_cleared(_profile);

    for(int index = 0; index < area.hotspot_count; ++index)
    {
        bool done = run::cleared(_profile, index);
        int frame = marker_duel;

        switch(area.hotspots[index].kind)
        {

        case hotspot_kind::DUEL:
            frame = done ? marker_duel_won :
                    _profile.run_area == castle::throne_room ? marker_god_king :
                    _profile.run_area == castle::throne_approach ? marker_elite : marker_duel;
            break;

        case hotspot_kind::GOLD:
            frame = done ? marker_gold_taken : marker_gold;
            break;

        case hotspot_kind::CHEST:
            frame = done ? marker_chest_open : marker_chest;
            break;

        case hotspot_kind::LOCKED_CHEST:
            frame = done ? marker_chest_open : marker_locked;
            break;

        default:
            frame = exits_open ? marker_exit : marker_exit_blocked;
            break;
        }

        _markers[index].set_tiles(bn::sprite_items::markers.tiles_item(), frame);
    }
}

void area_scene::_redraw_text()
{
    _text_sprites.clear();
    const castle::area& area = run::current_area(_profile);
    const castle::hotspot& hotspot = area.hotspots[_cursor];
    bool done = run::cleared(_profile, _cursor);
    bn::string<48> text;

    _text_generator.set_left_alignment();
    _text_generator.generate(left_x, top_y, area.name, _text_sprites);

    {
        bn::ostringstream stream(text);
        stream << "Bloodline " << _profile.bloodline;
    }

    _text_generator.set_right_alignment();
    _text_generator.generate(right_x, top_y, text, _text_sprites);
    text.clear();

    {
        bn::ostringstream stream(text);
        stream << "HP " << _profile.run_hp << "/" << _profile.max_hp() << "    Keys " << _profile.keys << "    "
               << _profile.gold << " gold";
    }

    _text_generator.set_left_alignment();
    _text_generator.generate(left_x, status_y, text, _text_sprites);
    text.clear();

    {
        bn::ostringstream stream(text);

        switch(hotspot.kind)
        {

        case hotspot_kind::DUEL:
            if(done)
            {
                stream << "Defeated";
            }
            else
            {
                int type = run::duel_type(_profile, _cursor);

                if(type == tuning::god_king_type_index)
                {
                    stream << "The God King    A: fight";
                }
                else
                {
                    stream << look::looks[run::duel_look(_profile, _cursor)].name << "    A: fight";
                }
            }
            break;

        case hotspot_kind::GOLD:
            stream << (done ? "Gold pouch (taken)" : "Gold pouch    A: take");
            break;

        case hotspot_kind::CHEST:
            stream << (done ? "Chest (opened)" : "Chest    A: open");
            break;

        case hotspot_kind::LOCKED_CHEST:
            if(done)
            {
                stream << "Locked chest (opened)";
            }
            else
            {
                stream << "Locked chest    A: use a key (" << _profile.keys << ")";
            }
            break;

        default:
            stream << "To the " << castle::areas[hotspot.target_area].name;

            if(run::duels_cleared(_profile))
            {
                stream << "    A: go";
            }
            break;
        }
    }

    _text_generator.set_center_alignment();
    _text_generator.generate(0, description_y, text, _text_sprites);

    if(! _message.empty())
    {
        _text_generator.generate(0, message_y, _message, _text_sprites);
    }
}

void area_scene::_save()
{
    save::write(_active_slot, _profile);
}

}
