#ifndef IGB_HERO_H
#define IGB_HERO_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"

#include "hero_profile.h"
#include "magic.h"
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
    CHANNEL,    // R held: entering the ring's pattern
    CAST,       // Spell fired: busy for a moment
    DEFEATED    // Out of HP: sinks out of view
};

// Hero state machine. Reads the keypad each frame.
class hero
{

public:
    // Max HP and shield pips come from the profile's stats.
    explicit hero(const hero_profile& profile);

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

    [[nodiscard]] int max_hp() const
    {
        return _max_hp;
    }

    // Ends the parry commit early so the hero can act in the opening.
    void parry_succeeded();

    // Brief white flash, e.g. when the super fires.
    void flash();

    void heal(int amount);

    // The ring's spell; R + its pattern casts it.
    void set_spell(const spell& spell)
    {
        _spell = &spell;
    }

    // Set each frame by the duel scene: R only starts a cast while charges remain.
    void set_magic_ready(bool ready)
    {
        _magic_ready = ready;
    }

    // Set each frame by the duel scene: debug shortcuts (Select held) shouldn't move the hero.
    void set_input_suppressed(bool suppressed)
    {
        _input_suppressed = suppressed;
    }

    // True on the frame R starts drawing a spell (the charge is spent here).
    [[nodiscard]] bool channel_started() const
    {
        return _channel_started;
    }

    // Steps of the spell pattern entered so far, while drawing.
    [[nodiscard]] int spell_progress() const
    {
        return _spell_progress;
    }

    // True on the frame a wrong direction resets the spell pattern.
    [[nodiscard]] bool spell_mistake() const
    {
        return _spell_mistake;
    }

    // True on the frame a spell's pattern completes.
    [[nodiscard]] bool cast_started() const
    {
        return _cast_started;
    }

    // The super can fire unless the hero is hurt, staggered, defeated or locked out.
    [[nodiscard]] bool can_use_super() const
    {
        return ! _input_locked && _state != hero_state::HURT && _state != hero_state::STAGGERED &&
               _state != hero_state::DEFEATED;
    }

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
    bool _input_suppressed = false;
    bool _magic_ready = false;
    bool _cast_started = false;
    bool _channel_started = false;
    bool _spell_mistake = false;
    const spell* _spell = nullptr;
    int _spell_progress = 0;
    int _max_hp;
    int _hp;
    int _max_pips;
    int _refill_frames;
    int _evade_cooldown = 0;
    int _shown_frame = 0;
    int _flash = 0;
    int _block_flash = 0;
    int _shield_pips;
    int _shield_regen = 0;

    void _enter(hero_state state);
    void _lose_hp(int damage);
    void _read_input();
    [[nodiscard]] static int _read_parry_chord();
    void _start_parry(int direction);
    void _start_attack(int direction);
    void _start_chord(int direction);
    void _settle_parry_direction();
    void _update_channel();
    void _update_shield_regen();
    [[nodiscard]] bn::fixed _evade_amount() const;
    void _update_sprite();
};

}

#endif
