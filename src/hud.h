#ifndef IGB_HUD_H
#define IGB_HUD_H

#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "tuning.h"

namespace igb
{

// Top-edge overlay: player HP bar and shield pips on the left, enemy HP bar and name on
// the right. Also shows the win/lose text.
class hud
{

public:
    explicit hud(const char* enemy_name);

    void update(int player_hp, int shield_pips, int enemy_hp);

    // Centered result title; the prompt line under it is shown separately, a moment later.
    void show_result(const char* title);
    void show_prompt(const char* prompt);

private:
    static constexpr int max_pips = 6;              // Spec: 3 to 6 from the shield item
    static constexpr int player_bar_segments = 8;   // 64 px
    static constexpr int enemy_bar_segments = 10;   // 80 px

    static_assert(tuning::shield_pips <= max_pips);

    bn::sprite_text_generator _text_generator;
    bn::vector<bn::sprite_ptr, max_pips> _pips;
    bn::vector<bn::sprite_ptr, player_bar_segments> _player_bar;
    bn::vector<bn::sprite_ptr, enemy_bar_segments> _enemy_bar;
    bn::vector<bn::sprite_ptr, 4> _name_sprites;
    bn::vector<bn::sprite_ptr, 8> _result_sprites;
    bn::vector<bn::sprite_ptr, 8> _prompt_sprites;
    int _shown_pips = -1;
    int _shown_player_hp = -1;
    int _shown_enemy_hp = -1;

    void _update_pips(int shield_pips);
};

}

#endif
