#ifndef IGB_HERO_H
#define IGB_HERO_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"

#include "tuning.h"

namespace igb
{

enum class hero_state
{
    IDLE,
    DODGE,      // Left or right
    DUCK,
    HOP,
    BLOCK,
    PARRY,
    ATTACK,
    HURT,
    STAGGERED,  // Shield broke: can't act
    DEFEATED    // Out of HP: sinks out of view
};

// Hero state machine. Reads the keypad each frame.
class hero
{

public:
    hero();

    void update();

    // Called by the duel scene when an attack lands.
    void hurt(int damage);

    // A blocked hit costs shield pips; the last pip breaks the shield.
    void absorb_block();

    // Empties the shield and staggers the hero; damage is for unblockable hits.
    void break_shield(int damage = 0);

    // Duel over: lost sinks the hero; won just stops input.
    void defeat();
    void lock_input()
    {
        _input_locked = true;
    }

    [[nodiscard]] int hp() const
    {
        return _hp;
    }

    // Ends the parry commit early so the hero can act in the opening.
    void parry_succeeded();

    // Set each frame by the duel scene: while the enemy is open, B + direction slashes
    // instead of parrying.
    void set_enemy_open(bool open)
    {
        _enemy_open = open;
    }

    // True on the frame a slash starts.
    [[nodiscard]] bool attack_started() const
    {
        return _attack_started;
    }

    // dir:: flags of the current slash.
    [[nodiscard]] int attack_direction() const
    {
        return _attack_direction;
    }

    [[nodiscard]] int shield_pips() const
    {
        return _shield_pips;
    }

    [[nodiscard]] hero_state state() const
    {
        return _state;
    }

    // Frames since the current state started (0 on the first frame).
    [[nodiscard]] int state_frames() const
    {
        return _state_frames;
    }

    // dir:: flag of the current dodge, duck (DOWN) or hop (UP); dir::NONE otherwise.
    [[nodiscard]] int evade_direction() const;

    // dir:: flags the current parry points toward (two for a diagonal); dir::NONE when not parrying.
    [[nodiscard]] int parry_direction() const;

    // Arena BG offset that sells the dodge as camera motion.
    [[nodiscard]] bn::fixed_point arena_nudge() const;

private:
    bn::sprite_ptr _sprite;
    hero_state _state = hero_state::IDLE;
    int _state_frames = 0;
    int _evade_direction = 0;
    int _parry_direction = 0;
    int _attack_direction = 0;
    bool _enemy_open = false;
    bool _attack_started = false;
    bool _input_locked = false;
    int _hp = tuning::player_max_hp;
    int _evade_cooldown = 0;
    int _shown_frame = 0;
    int _flash = 0;
    int _block_flash = 0;
    int _shield_pips = tuning::shield_pips;
    int _shield_regen = 0;

    void _enter(hero_state state);
    void _lose_hp(int damage);
    void _read_input();
    [[nodiscard]] static int _read_parry_chord();
    void _start_parry(int direction);
    void _start_attack(int direction);
    void _start_chord(int direction);
    void _settle_parry_direction();
    void _update_shield_regen();
    [[nodiscard]] bn::fixed _evade_amount() const;
    void _update_sprite();
};

}

#endif
