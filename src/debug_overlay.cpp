#include "debug_overlay.h"

#include "bn_keypad.h"
#include "bn_sstream.h"

#include "common_variable_8x8_sprite_font.h"
#include "enemy.h"
#include "hero.h"
#include "leveling.h"
#include "magic.h"

namespace igb
{

namespace
{
    constexpr int priority = 0;

    // Left column under the HUD; screen origin is the center.
    constexpr int text_x = -116;
    constexpr int first_line_y = -38;      // Below the super meter
    constexpr int line_spacing = 10;

    // Indexed by enemy_state and hero_state.
    constexpr const char* enemy_state_names[] = {
        "IDLE", "WINDUP", "STRIKE", "RECOVER", "REEL", "OPEN", "DEFEAT"
    };

    constexpr const char* hero_state_names[] = {
        "IDLE", "DODGE", "DUCK", "HOP", "BLOCK", "PARRY", "ATTACK", "HURT", "STAGGER", "CHANNEL", "CAST",
        "DEFEAT"
    };

    static_assert(sizeof(enemy_state_names) / sizeof(enemy_state_names[0]) == int(enemy_state::DEFEATED) + 1);

    static_assert(sizeof(hero_state_names) / sizeof(hero_state_names[0]) == int(hero_state::DEFEATED) + 1);

    [[nodiscard]] bool is_attacking(enemy_state state)
    {
        return state == enemy_state::WINDUP || state == enemy_state::STRIKE ||
               state == enemy_state::RECOVER || state == enemy_state::STAGGERED;
    }
}

debug_overlay::debug_overlay(debug_settings& settings, hero_profile& profile) :
    _settings(settings),
    _profile(profile),
    _text_generator(common::variable_8x8_sprite_font)
{
    _text_generator.set_bg_priority(priority);
}

bool debug_overlay::update(const enemy& enemy, const hero& hero, const char* last_outcome, int duel_frames,
                           bool duel_over, int magic_charges)
{
    // Select + L / R: next enemy tier / type. Select + B: next gear slot. Select + A: next item
    // in that slot. Select + D-pad: stat point. These restart the duel (except picking a slot).
    // Select + Start: refill super and magic.
    _refill_requested = false;

    if(bn::keypad::select_pressed())
    {
        _settings.select_combo_used = false;
    }

    if(bn::keypad::select_held())
    {
        if(bn::keypad::l_pressed())
        {
            _settings.tier_index = (_settings.tier_index + 1) % tuning::enemy_tier_count;
            _settings.enemy_override = true;
            _settings.select_combo_used = true;
            return true;
        }

        if(bn::keypad::r_pressed())
        {
            _settings.type_index = (_settings.type_index + 1) % tuning::enemy_type_count;
            _settings.enemy_override = true;
            _settings.select_combo_used = true;
            return true;
        }

        if(bn::keypad::b_pressed())
        {
            _settings.gear_slot = (_settings.gear_slot + 1) % int(item_slot::COUNT);
            _settings.select_combo_used = true;
        }

        if(bn::keypad::a_pressed())
        {
            _equip_next(item_slot(_settings.gear_slot));
            _settings.select_combo_used = true;
            return true;
        }

        if(bn::keypad::start_pressed())
        {
            _refill_requested = true;
            _settings.select_combo_used = true;
        }

        // Stat points until leveling exists.
        int* stat = bn::keypad::up_pressed() ? &_profile.health :
                    bn::keypad::right_pressed() ? &_profile.attack :
                    bn::keypad::down_pressed() ? &_profile.shield :
                    bn::keypad::left_pressed() ? &_profile.magic : nullptr;

        if(stat)
        {
            ++*stat;
            _settings.select_combo_used = true;
            return true;
        }
    }

    // A plain tap of Select toggles the overlay.
    if(bn::keypad::select_released() && ! _settings.select_combo_used)
    {
        _settings.visible = ! _settings.visible;
    }

    // Hidden on the win/lose screens, which need the sprites for their text.
    if(! _settings.visible || duel_over)
    {
        _hide();
        return false;
    }

    if(bn::keypad::start_pressed() && ! bn::keypad::select_held() && ! duel_over)
    {
        _settings.invincible = ! _settings.invincible;
    }

    line_text text;

    {
        bn::ostringstream stream(text);
        stream << "F" << duel_frames << " " << tuning::enemy_tiers[_settings.tier_index]->name
               << " " << tuning::enemy_types[_settings.type_index]->name;

        if(_settings.invincible)
        {
            stream << "  INV";
        }
    }

    _show_line(0, text);
    text.clear();

    {
        bn::ostringstream stream(text);
        stream << "E " << enemy_state_names[int(enemy.state())] << " " << enemy.state_frames()
               << "/" << enemy.state_length();

        if(enemy.state() == enemy_state::WINDUP)
        {
            stream << "  hit in " << enemy.frames_to_impact();
        }
    }

    _show_line(1, text);
    text.clear();

    if(is_attacking(enemy.state()))
    {
        bn::ostringstream stream(text);
        const attack_step& step = enemy.current_attack();
        stream << "  ";

        if(step.feint)
        {
            stream << "Feint " << attack_type_of(step.feint_from).name << ">";
        }

        stream << attack_type_of(step.id).name << (step.heavy ? " HEAVY" : "")
               << "  " << (enemy.sequence_index() + 1) << "/" << enemy.sequence_size();
    }

    _show_line(2, text);
    text.clear();

    {
        bn::ostringstream stream(text);
        stream << "H " << hero_state_names[int(hero.state())] << " " << hero.state_frames()
               << "  " << spell_of(_profile.ring_spell()).name << " x" << magic_charges;
    }

    _show_line(3, text);
    text.clear();

    {
        bn::ostringstream stream(text);
        stream << "Last: " << last_outcome;
    }

    _show_line(4, text);
    text.clear();

    {
        bn::ostringstream stream(text);
        stream << "Lv" << _profile.level << " " << _profile.xp << "/" << xp_to_next_level(_profile.level)
               << " H" << _profile.health << " A" << _profile.attack
               << " S" << _profile.shield << " M" << _profile.magic << "  " << _profile.gold << "g";
    }

    _show_line(5, text);
    text.clear();

    {
        bn::ostringstream stream(text);
        item_slot slot = item_slot(_settings.gear_slot);
        int index = _profile.equipped[int(slot)];
        stream << slot_names[_settings.gear_slot] << ": " << _profile.worn(slot).name << " ";

        if(_profile.mastered(index))
        {
            stream << "M";
        }
        else
        {
            stream << _profile.item_xp[index] << "/" << mastery_xp(_profile.worn(slot));
        }
    }

    _show_line(6, text);
    return false;
}

void debug_overlay::_equip_next(item_slot slot)
{
    // Next item in the roster that fits this slot, wrapping around.
    int current = _profile.equipped[int(slot)];

    for(int step = 1; step <= roster::item_count; ++step)
    {
        int candidate = (current + step) % roster::item_count;

        if(roster::items[candidate].slot == slot)
        {
            // Debug equipping also gives the item, so the equipment screen can switch back to it.
            _profile.owned[candidate] = true;
            _profile.equipped[int(slot)] = candidate;
            return;
        }
    }
}

void debug_overlay::_show_line(int line, const line_text& text)
{
    if(text == _line_texts[line] && ! _line_sprites[line].empty())
    {
        return;
    }

    _line_texts[line] = text;
    _line_sprites[line].clear();

    if(! text.empty())
    {
        _text_generator.generate(text_x, first_line_y + line * line_spacing, text, _line_sprites[line]);
    }
}

void debug_overlay::_hide()
{
    for(int line = 0; line < lines; ++line)
    {
        _line_sprites[line].clear();
        _line_texts[line].clear();
    }
}

}
