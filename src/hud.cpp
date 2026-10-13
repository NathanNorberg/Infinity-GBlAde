#include "hud.h"

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
    constexpr int pip_spacing = 9;

    constexpr int result_y = -8;
    constexpr int prompt_y = 12;

    // Frames in graphics/hp_bar.bmp: 0..8 player fill, 9..17 enemy fill.
    constexpr int segment_px = 8;
    constexpr int player_bar_first_frame = 0;
    constexpr int enemy_bar_first_frame = 9;

    // Frames in graphics/shield_pip.bmp.
    constexpr int pip_full_frame = 0;
    constexpr int pip_empty_frame = 1;

    template<int Segments>
    void create_bar(bn::vector<bn::sprite_ptr, Segments>& bar, int left)
    {
        for(int index = 0; index < Segments; ++index)
        {
            int x = left + index * segment_px + segment_px / 2;
            bn::sprite_ptr segment = bn::sprite_items::hp_bar.create_sprite(x, bar_y);
            segment.set_bg_priority(priority);
            bar.push_back(bn::move(segment));
        }
    }

    template<int Segments>
    void fill_bar(bn::vector<bn::sprite_ptr, Segments>& bar, int first_frame, int hp, int max_hp)
    {
        // Round up so any HP left shows at least one pixel.
        int total_px = Segments * segment_px;
        int filled_px = (hp * total_px + max_hp - 1) / max_hp;

        for(int index = 0; index < Segments; ++index)
        {
            int segment_fill = filled_px - index * segment_px;
            segment_fill = segment_fill < 0 ? 0 : segment_fill > segment_px ? segment_px : segment_fill;
            bar[index].set_tiles(bn::sprite_items::hp_bar.tiles_item(), first_frame + segment_fill);
        }
    }
}

hud::hud(const char* enemy_name) :
    _text_generator(common::variable_8x8_sprite_font)
{
    _text_generator.set_bg_priority(priority);

    create_bar(_player_bar, player_bar_left);
    create_bar(_enemy_bar, enemy_bar_right - enemy_bar_segments * segment_px);

    for(int index = 0; index < tuning::shield_pips; ++index)
    {
        int x = player_bar_left + 4 + index * pip_spacing;
        bn::sprite_ptr pip = bn::sprite_items::shield_pip.create_sprite(x, below_bar_y);
        pip.set_bg_priority(priority);
        _pips.push_back(bn::move(pip));
    }

    _text_generator.set_right_alignment();
    _text_generator.generate(enemy_bar_right, below_bar_y, enemy_name, _name_sprites);
}

void hud::update(int player_hp, int shield_pips, int enemy_hp)
{
    if(player_hp != _shown_player_hp)
    {
        fill_bar(_player_bar, player_bar_first_frame, player_hp, tuning::player_max_hp);
        _shown_player_hp = player_hp;
    }

    if(enemy_hp != _shown_enemy_hp)
    {
        fill_bar(_enemy_bar, enemy_bar_first_frame, enemy_hp, tuning::enemy_max_hp);
        _shown_enemy_hp = enemy_hp;
    }

    _update_pips(shield_pips);
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
    _text_generator.set_center_alignment();
    _text_generator.generate(0, prompt_y, prompt, _prompt_sprites);
}

void hud::_update_pips(int shield_pips)
{
    if(shield_pips == _shown_pips)
    {
        return;
    }

    for(int index = 0; index < _pips.size(); ++index)
    {
        int frame = index < shield_pips ? pip_full_frame : pip_empty_frame;
        _pips[index].set_tiles(bn::sprite_items::shield_pip.tiles_item(), frame);
    }

    _shown_pips = shield_pips;
}

}
