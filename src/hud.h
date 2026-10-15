#ifndef IGB_HUD_H
#define IGB_HUD_H

#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "arrow_row.h"
#include "combo.h"
#include "duel_rewards.h"
#include "items.h"
#include "magic.h"
#include "tuning.h"

namespace igb
{

// Top-edge overlay: player HP bar, shield pips, super meter and magic charges on the left,
// enemy HP bar and name on the right. Also shows the spell prompt while R is held, the combo
// hint during openings, popups, the pause screen and the win/lose text.
class hud
{

public:
    // player_max_hp, shield_pips and magic_charges are the hero's maximums for this duel;
    // the HP bar grows with max HP.
    hud(const char* enemy_name, int player_max_hp, int shield_pips, int magic_charges);

    void update(int player_hp, int player_max_hp, int shield_pips, int super_meter, int magic_charges,
                int enemy_hp, int enemy_max_hp);

    // Centered result title; the prompt line under it is shown separately, a moment later.
    void show_result(const char* title);
    void show_prompt(const char* prompt);

    // Victory screen: XP and gold, the bonuses earned and each worn item's XP. The prompt
    // then goes at the bottom instead of under the title.
    void show_victory(const duel_rewards& rewards, const hero_profile& profile);

    // Short-lived text above the enemy, such as "COMBO +15".
    void show_popup(const char* text);

    // Spell name and pattern arrows while R is held; entered steps light up.
    void show_spell_prompt(const spell& spell, int progress);
    void spell_mistake();
    void hide_spell_prompt();

    // Combo arrows during an opening; landed steps light up.
    void show_combo_hint(const combo& combo, int progress);
    void combo_mistake();
    void hide_combo_hint();

    // Pause screen: the equipped ring's spell and the weapon's combo, with their arrows.
    void show_pause(const item& ring, const item& weapon, int magic_charges);
    void hide_pause();

private:
    static constexpr int max_pips = 6;              // Spec: 3 to 6 from the shield item
    static constexpr int max_charges = 6;
    static constexpr int player_bar_min_segments = 8;   // 64 px at the starting 100 HP
    static constexpr int player_bar_max_segments = 13;  // 104 px, stops short of the enemy bar
    static constexpr int enemy_bar_segments = 10;   // 80 px
    static constexpr int super_bar_segments = 6;    // 48 px

    static_assert(tuning::max_shield_pips <= max_pips);
    static_assert(tuning::max_magic_charges <= max_charges);

    bn::sprite_text_generator _text_generator;
    bn::vector<bn::sprite_ptr, max_pips> _pips;
    bn::vector<bn::sprite_ptr, max_charges> _charges;
    bn::vector<bn::sprite_ptr, player_bar_max_segments> _player_bar;
    bn::vector<bn::sprite_ptr, enemy_bar_segments> _enemy_bar;
    bn::vector<bn::sprite_ptr, super_bar_segments> _super_bar;
    bn::vector<bn::sprite_ptr, 4> _name_sprites;
    bn::vector<bn::sprite_ptr, 48> _result_sprites;
    bn::vector<bn::sprite_ptr, 8> _prompt_sprites;
    bn::vector<bn::sprite_ptr, 4> _popup_sprites;
    bn::vector<bn::sprite_ptr, 2> _spell_name_sprites;
    bn::vector<bn::sprite_ptr, 40> _pause_sprites;
    arrow_row _spell_arrows;
    arrow_row _combo_arrows;
    arrow_row _pause_spell_arrows;
    arrow_row _pause_combo_arrows;
    int _popup_frames = 0;
    int _shown_pips = -1;
    int _shown_charges = -1;
    int _shown_player_hp = -1;
    int _shown_enemy_hp = -1;
    int _shown_super = -1;
    int _frames = 0;
    bool _victory_layout = false;

    void _update_pips(int shield_pips);
    void _update_charges(int magic_charges);
    template<int MaxSprites>
    void _write_line(int y, const char* text, bn::vector<bn::sprite_ptr, MaxSprites>& sprites)
    {
        _text_generator.set_center_alignment();
        _text_generator.generate(0, y, text, sprites);
    }
};

}

#endif
