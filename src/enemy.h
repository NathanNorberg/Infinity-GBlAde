#ifndef IGB_ENEMY_H
#define IGB_ENEMY_H

#include "bn_random.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "attack_data.h"

namespace igb
{

enum class enemy_state
{
    IDLE,
    WINDUP,
    STRIKE,
    RECOVER,
    STAGGERED,  // Reeling from a mid-sequence parry; the sequence continues afterwards
    OPEN,       // Guard down: the hero's slashes land
    DEFEATED    // Out of HP: sinks and fades out
};

// Enemy state machine. Runs sequences of 1 to tier.max_attacks attacks:
// IDLE -> (WINDUP -> STRIKE -> RECOVER) per attack -> IDLE.
// The duel scene can interrupt with reel() after a parry and open() to grant an opening.
//
// Impact timing: an attack lands on the first frame of STRIKE. During WINDUP,
// frames_to_impact() counts down to 1 on the last wind-up frame; on the impact frame
// it is 0 and impact() is true.
class enemy
{

public:
    explicit enemy(const tuning::enemy_tier& tier);

    void update();

    // Parried mid-sequence: reel for a moment, then throw the next attack.
    void reel(int frames);

    // Drops the guard for an opening; when it ends the guard comes back up.
    void open(int frames);

    // A hero slash during an opening. direction is the dir:: flags of the slash.
    void take_hit(int direction, int damage);

    // Duel over: defeat() when out of HP, stand_down() when the hero has fallen.
    void defeat();
    void stand_down();

    [[nodiscard]] enemy_state state() const
    {
        return _state;
    }

    [[nodiscard]] int state_frames() const
    {
        return _state_frames;
    }

    [[nodiscard]] int state_length() const
    {
        return _state_length;
    }

    [[nodiscard]] const attack_step& current_attack() const
    {
        return _sequence[_sequence_index];
    }

    [[nodiscard]] int frames_to_impact() const;

    // True on the frame an attack lands.
    [[nodiscard]] bool impact() const
    {
        return _impact;
    }

    // True on the frame the last attack of a sequence finishes its swing.
    [[nodiscard]] bool sequence_finished() const
    {
        return _sequence_finished;
    }

    [[nodiscard]] bool on_last_attack() const
    {
        return _sequence_index + 1 >= _sequence.size();
    }

    [[nodiscard]] int sequence_index() const
    {
        return _sequence_index;
    }

    [[nodiscard]] int sequence_size() const
    {
        return _sequence.size();
    }

    [[nodiscard]] int hp() const
    {
        return _hp;
    }

private:
    static constexpr int trail_dots = 8;

    const tuning::enemy_tier& _tier;
    bn::regular_bg_ptr _bg;
    bn::vector<bn::sprite_ptr, trail_dots> _trail;
    bn::random _random;
    bn::vector<attack_step, tuning::sequence_max_attacks> _sequence;
    bn::vector<attack_id, int(attack_id::COUNT)> _attack_pool;
    int _sequence_index = 0;
    enemy_state _state = enemy_state::IDLE;
    int _state_frames = 0;
    int _state_length = 0;
    int _hp = tuning::enemy_max_hp;
    int _hit_frames = 0;           // Counts down after a hero slash lands
    bool _impact = false;
    bool _sequence_finished = false;
    int _fade = 0;

    void _advance();
    void _enter(enemy_state state, int length);
    void _start_sequence();
    void _place_trail(bn::fixed_point from, bn::fixed_point to, int frame);
    void _update_effects();
    void _update_trail();
};

}

#endif
