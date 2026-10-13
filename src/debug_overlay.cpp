#include "debug_overlay.h"

#include "bn_keypad.h"
#include "bn_sstream.h"

#include "common_variable_8x8_sprite_font.h"
#include "enemy.h"
#include "hero.h"

namespace igb
{

namespace
{
    constexpr int priority = 0;

    // Left column under the HUD; screen origin is the center.
    constexpr int text_x = -116;
    constexpr int first_line_y = -48;
    constexpr int line_spacing = 10;

    // Indexed by enemy_state and hero_state.
    constexpr const char* enemy_state_names[] = {
        "IDLE", "WINDUP", "STRIKE", "RECOVER", "REEL", "OPEN", "DEFEAT"
    };

    constexpr const char* hero_state_names[] = {
        "IDLE", "DODGE", "DUCK", "HOP", "BLOCK", "PARRY", "ATTACK", "HURT", "STAGGER", "DEFEAT"
    };

    static_assert(sizeof(enemy_state_names) / sizeof(enemy_state_names[0]) == int(enemy_state::DEFEATED) + 1);
    static_assert(sizeof(hero_state_names) / sizeof(hero_state_names[0]) == int(hero_state::DEFEATED) + 1);

    [[nodiscard]] bool is_attacking(enemy_state state)
    {
        return state == enemy_state::WINDUP || state == enemy_state::STRIKE ||
               state == enemy_state::RECOVER || state == enemy_state::STAGGERED;
    }
}

debug_overlay::debug_overlay(debug_settings& settings) :
    _settings(settings),
    _text_generator(common::variable_8x8_sprite_font)
{
    _text_generator.set_bg_priority(priority);
}

bool debug_overlay::update(const enemy& enemy, const hero& hero, const char* last_outcome, int duel_frames)
{
    if(bn::keypad::select_pressed())
    {
        _settings.visible = ! _settings.visible;
    }

    if(! _settings.visible)
    {
        _hide();
        return false;
    }

    // L / R: previous / next enemy tier.
    if(bn::keypad::l_pressed() || bn::keypad::r_pressed())
    {
        int step = bn::keypad::r_pressed() ? 1 : tuning::enemy_tier_count - 1;
        _settings.tier_index = (_settings.tier_index + step) % tuning::enemy_tier_count;
        return true;
    }

    line_text text;

    {
        bn::ostringstream stream(text);
        stream << "F" << duel_frames << "  L/R: " << tuning::enemy_tiers[_settings.tier_index]->name;
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
        stream << "  " << attack_type_of(step.id).name << (step.heavy ? " HEAVY" : "")
               << "  " << (enemy.sequence_index() + 1) << "/" << enemy.sequence_size();
    }

    _show_line(2, text);
    text.clear();

    {
        bn::ostringstream stream(text);
        stream << "H " << hero_state_names[int(hero.state())] << " " << hero.state_frames();
    }

    _show_line(3, text);
    text.clear();

    {
        bn::ostringstream stream(text);
        stream << "Last: " << last_outcome;
    }

    _show_line(4, text);
    return false;
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
