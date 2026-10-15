#include "hud.h"

#include "bn_sstream.h"
#include "bn_string.h"

#include "bn_sprite_items_charge_orb.h"
#include "bn_sprite_items_hp_bar.h"
#include "bn_sprite_items_shield_pip.h"

#include "common_variable_8x8_sprite_font.h"

namespace igb
{

namespace
{
    constexpr int priority = 0;

    // Screen origin is the center; the HUD runs along the top edge.
    constexpr int bar_y = -72;
    constexpr int player_bar_left = -116;
    constexpr int enemy_bar_right = 116;
    constexpr int below_bar_y = -62;
    constexpr int super_bar_y = -52;
    constexpr int pip_spacing = 9;
    constexpr int charges_left = -60;      // Just right of the super meter
    constexpr int charge_spacing = 9;

    constexpr int result_y = -8;

    // Victory screen lines.
    constexpr int victory_title_y = -46;
    constexpr int victory_rewards_y = -32;
    constexpr int victory_bonus_y = -22;
    constexpr int victory_items_y = -8;
    constexpr int victory_item_spacing = 10;
    constexpr int victory_level_y = 44;
    constexpr int victory_prompt_y = 60;
    constexpr int popup_y = -44;
    constexpr int prompt_y = 12;

    // Spell prompt in the middle of the screen; combo hint near the bottom, clear of the hero.
    constexpr int spell_name_y = 4;
    constexpr bn::fixed_point spell_arrows_center(0, 20);
    constexpr bn::fixed_point combo_arrows_center(0, 64);

    // Pause screen lines.
    constexpr int pause_title_y = -40;
    constexpr int pause_ring_y = -26;
    constexpr int pause_ring_effect_y = -15;
    constexpr bn::fixed_point pause_spell_arrows_center(0, 2);
    constexpr int pause_combo_y = 20;
    constexpr bn::fixed_point pause_combo_arrows_center(0, 36);
    constexpr int pause_footer_y = 56;

    // Frames in graphics/hp_bar.bmp: 0..8 player fill, 9..17 enemy fill, 18..26 super fill.
    constexpr int segment_px = 8;
    constexpr int player_bar_first_frame = 0;
    constexpr int enemy_bar_first_frame = 9;
    constexpr int super_bar_first_frame = 18;

    // Frames in graphics/shield_pip.bmp and graphics/charge_orb.bmp.
    constexpr int full_frame = 0;
    constexpr int empty_frame = 1;

    template<int MaxSegments>
    void create_bar(bn::vector<bn::sprite_ptr, MaxSegments>& bar, int segments, int left, int y = bar_y)
    {
        for(int index = 0; index < segments; ++index)
        {
            int x = left + index * segment_px + segment_px / 2;
            bn::sprite_ptr segment = bn::sprite_items::hp_bar.create_sprite(x, y);
            segment.set_bg_priority(priority);
            bar.push_back(bn::move(segment));
        }
    }

    template<int MaxSegments>
    void fill_bar(bn::vector<bn::sprite_ptr, MaxSegments>& bar, int first_frame, int hp, int max_hp)
    {
        // Round up so any HP left shows at least one pixel.
        int segments = bar.size();
        int total_px = segments * segment_px;
        int filled_px = (hp * total_px + max_hp - 1) / max_hp;

        for(int index = 0; index < segments; ++index)
        {
            int segment_fill = filled_px - index * segment_px;
            segment_fill = segment_fill < 0 ? 0 : segment_fill > segment_px ? segment_px : segment_fill;
            bar[index].set_tiles(bn::sprite_items::hp_bar.tiles_item(), first_frame + segment_fill);
        }
    }
}

hud::hud(const char* enemy_name, int player_max_hp, int shield_pips, int magic_charges) :
    _text_generator(common::variable_8x8_sprite_font)
{
    _text_generator.set_bg_priority(priority);

    // One more segment per hp_per_bar_segment of max HP over the starting 100.
    int extra_hp = player_max_hp - tuning::player_max_hp;
    int player_segments = player_bar_min_segments + (extra_hp > 0 ? extra_hp / tuning::hp_per_bar_segment : 0);
    player_segments = player_segments < player_bar_max_segments ? player_segments : player_bar_max_segments;

    create_bar(_player_bar, player_segments, player_bar_left);
    create_bar(_enemy_bar, enemy_bar_segments, enemy_bar_right - enemy_bar_segments * segment_px);
    create_bar(_super_bar, super_bar_segments, player_bar_left, super_bar_y);

    for(int index = 0; index < shield_pips; ++index)
    {
        int x = player_bar_left + 4 + index * pip_spacing;
        bn::sprite_ptr pip = bn::sprite_items::shield_pip.create_sprite(x, below_bar_y);
        pip.set_bg_priority(priority);
        _pips.push_back(bn::move(pip));
    }

    for(int index = 0; index < magic_charges; ++index)
    {
        int x = charges_left + index * charge_spacing;
        bn::sprite_ptr orb = bn::sprite_items::charge_orb.create_sprite(x, super_bar_y);
        orb.set_bg_priority(priority);
        _charges.push_back(bn::move(orb));
    }

    _text_generator.set_right_alignment();
    _text_generator.generate(enemy_bar_right, below_bar_y, enemy_name, _name_sprites);
}

void hud::update(int player_hp, int player_max_hp, int shield_pips, int super_meter, int magic_charges,
                 int enemy_hp, int enemy_max_hp)
{
    ++_frames;

    if(super_meter != _shown_super)
    {
        fill_bar(_super_bar, super_bar_first_frame, super_meter, tuning::super_meter_max);
        _shown_super = super_meter;
    }

    // A full meter blinks so the player knows L is ready.
    bool super_visible = super_meter < tuning::super_meter_max || (_frames / tuning::super_full_blink) % 2 == 0;

    for(bn::sprite_ptr& segment : _super_bar)
    {
        segment.set_visible(super_visible);
    }

    if(player_hp != _shown_player_hp)
    {
        fill_bar(_player_bar, player_bar_first_frame, player_hp, player_max_hp);
        _shown_player_hp = player_hp;
    }

    if(enemy_hp != _shown_enemy_hp)
    {
        fill_bar(_enemy_bar, enemy_bar_first_frame, enemy_hp, enemy_max_hp);
        _shown_enemy_hp = enemy_hp;
    }

    _update_pips(shield_pips);
    _update_charges(magic_charges);
    _spell_arrows.update();
    _combo_arrows.update();

    if(_popup_frames > 0 && --_popup_frames == 0)
    {
        _popup_sprites.clear();
    }
}

void hud::show_popup(const char* text)
{
    _popup_sprites.clear();
    _text_generator.set_center_alignment();
    _text_generator.generate(0, popup_y, text, _popup_sprites);
    _popup_frames = tuning::popup_frames;
}

void hud::show_result(const char* title)
{
    _result_sprites.clear();
    _text_generator.set_center_alignment();
    _text_generator.generate(0, result_y, title, _result_sprites);
}

void hud::show_prompt(const char* prompt)
{
    _prompt_sprites.clear();
    _write_line(_victory_layout ? victory_prompt_y : prompt_y, prompt, _prompt_sprites);
}

void hud::show_victory(const duel_rewards& rewards, const hero_profile& profile)
{
    _victory_layout = true;
    _result_sprites.clear();
    _write_line(victory_title_y, "VICTORY", _result_sprites);

    bn::string<48> text;

    {
        bn::ostringstream stream(text);
        stream << "+" << rewards.xp << " XP   +" << rewards.gold << " gold";
    }

    _write_line(victory_rewards_y, text.c_str(), _result_sprites);

    if(rewards.perfect_bonus || rewards.no_block_bonus)
    {
        text.clear();
        bn::ostringstream stream(text);

        if(rewards.perfect_bonus)
        {
            stream << "Perfect parry +" << tuning::perfect_sequence_xp_bonus << "%  ";
        }

        if(rewards.no_block_bonus)
        {
            stream << "No block +" << tuning::no_block_xp_bonus << "%";
        }

        _write_line(victory_bonus_y, text.c_str(), _result_sprites);
    }

    for(int slot = 0; slot < int(item_slot::COUNT); ++slot)
    {
        text.clear();
        bn::ostringstream stream(text);
        int index = profile.equipped[slot];
        const item& worn = roster::items[index];
        stream << worn.name << "  ";

        if(rewards.newly_mastered[slot])
        {
            stream << "MASTERED! +" << tuning::mastery_stat_points << " point";
        }
        else if(profile.mastered(index))
        {
            stream << "mastered";
        }
        else
        {
            stream << "+" << rewards.item_xp[slot] << " XP  " << profile.item_xp[index] << "/" << mastery_xp(worn);
        }
        _write_line(victory_items_y + slot * victory_item_spacing, text.c_str(), _result_sprites);
    }

    if(rewards.levels_gained > 0)
    {
        text.clear();
        bn::ostringstream stream(text);
        int points = rewards.levels_gained * tuning::stat_points_per_level;
        stream << "LEVEL UP! Lv " << profile.level << " (+" << points << (points == 1 ? " stat point)" : " stat points)");
        _write_line(victory_level_y, text.c_str(), _result_sprites);
    }
}

void hud::show_spell_prompt(const spell& spell, int progress)
{
    if(! _spell_arrows.visible())
    {
        _text_generator.set_center_alignment();
        _text_generator.generate(0, spell_name_y, spell.name, _spell_name_sprites);
        _spell_arrows.show(spell.steps, spell.length, spell_arrows_center);
    }

    _spell_arrows.set_progress(progress);
}

void hud::spell_mistake()
{
    _spell_arrows.flash_mistake();
}

void hud::hide_spell_prompt()
{
    _spell_name_sprites.clear();
    _spell_arrows.hide();
}

void hud::show_combo_hint(const combo& combo, int progress)
{
    _combo_arrows.show(combo.steps, combo.length, combo_arrows_center);
    _combo_arrows.set_progress(progress);
}

void hud::combo_mistake()
{
    _combo_arrows.flash_mistake();
}

void hud::hide_combo_hint()
{
    _combo_arrows.hide();
}

void hud::show_pause(const item& ring, const item& weapon, int magic_charges)
{
    hide_pause();

    const spell& ring_spell = spell_of(ring.ring_spell);
    const combo& weapon_combo = *weapon.weapon_combo;

    _write_line(pause_title_y, "PAUSED", _pause_sprites);

    bn::string<48> text;

    {
        bn::ostringstream stream(text);
        stream << ring.name << ": " << ring_spell.name << " (hold R)  " << magic_charges << " left";
    }

    _write_line(pause_ring_y, text.c_str(), _pause_sprites);
    _write_line(pause_ring_effect_y, ring_spell.description, _pause_sprites);
    _pause_spell_arrows.show(ring_spell.steps, ring_spell.length, pause_spell_arrows_center);

    text.clear();

    {
        bn::ostringstream stream(text);
        stream << weapon.name << ": " << weapon_combo.name << " (B + dir)";
    }

    _write_line(pause_combo_y, text.c_str(), _pause_sprites);
    _pause_combo_arrows.show(weapon_combo.steps, weapon_combo.length, pause_combo_arrows_center);

    _write_line(pause_footer_y, "START to resume", _pause_sprites);
}

void hud::hide_pause()
{
    _pause_sprites.clear();
    _pause_spell_arrows.hide();
    _pause_combo_arrows.hide();
}

void hud::_update_pips(int shield_pips)
{
    if(shield_pips == _shown_pips)
    {
        return;
    }

    for(int index = 0; index < _pips.size(); ++index)
    {
        int frame = index < shield_pips ? full_frame : empty_frame;
        _pips[index].set_tiles(bn::sprite_items::shield_pip.tiles_item(), frame);
    }

    _shown_pips = shield_pips;
}

void hud::_update_charges(int magic_charges)
{
    if(magic_charges == _shown_charges)
    {
        return;
    }

    for(int index = 0; index < _charges.size(); ++index)
    {
        int frame = index < magic_charges ? full_frame : empty_frame;
        _charges[index].set_tiles(bn::sprite_items::charge_orb.tiles_item(), frame);
    }

    _shown_charges = magic_charges;
}

}
