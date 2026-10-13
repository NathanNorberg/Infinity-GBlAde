#include "enemy.h"

#include "bn_bg_palette_ptr.h"
#include "bn_colors.h"

#include "bn_regular_bg_items_enemy_idle.h"
#include "bn_regular_bg_items_enemy_st_horiz_l.h"
#include "bn_regular_bg_items_enemy_st_horiz_r.h"
#include "bn_regular_bg_items_enemy_st_overhead.h"
#include "bn_regular_bg_items_enemy_st_slash_l.h"
#include "bn_regular_bg_items_enemy_st_slash_r.h"
#include "bn_regular_bg_items_enemy_st_sweep.h"
#include "bn_regular_bg_items_enemy_st_thrust.h"
#include "bn_regular_bg_items_enemy_wu_horiz_l.h"
#include "bn_regular_bg_items_enemy_wu_horiz_r.h"
#include "bn_regular_bg_items_enemy_wu_overhead.h"
#include "bn_regular_bg_items_enemy_wu_slash_l.h"
#include "bn_regular_bg_items_enemy_wu_slash_r.h"
#include "bn_regular_bg_items_enemy_wu_sweep.h"
#include "bn_regular_bg_items_enemy_wu_thrust.h"
#include "bn_sprite_items_telegraph.h"

namespace igb
{

namespace
{
    constexpr int bg_priority = 2;          // Behind the hero sprite (1), in front of the arena (3)
    constexpr int telegraph_priority = 0;   // In front of the hero so the trail is never hidden

    // Indexed by attack_id.
    constexpr const bn::regular_bg_item* windup_poses[] = {
        &bn::regular_bg_items::enemy_wu_slash_l,
        &bn::regular_bg_items::enemy_wu_slash_r,
        &bn::regular_bg_items::enemy_wu_horiz_l,
        &bn::regular_bg_items::enemy_wu_horiz_r,
        &bn::regular_bg_items::enemy_wu_overhead,
        &bn::regular_bg_items::enemy_wu_sweep,
        &bn::regular_bg_items::enemy_wu_thrust,
    };

    constexpr const bn::regular_bg_item* strike_poses[] = {
        &bn::regular_bg_items::enemy_st_slash_l,
        &bn::regular_bg_items::enemy_st_slash_r,
        &bn::regular_bg_items::enemy_st_horiz_l,
        &bn::regular_bg_items::enemy_st_horiz_r,
        &bn::regular_bg_items::enemy_st_overhead,
        &bn::regular_bg_items::enemy_st_sweep,
        &bn::regular_bg_items::enemy_st_thrust,
    };

    static_assert(sizeof(windup_poses) / sizeof(windup_poses[0]) == int(attack_id::COUNT));
    static_assert(sizeof(strike_poses) / sizeof(strike_poses[0]) == int(attack_id::COUNT));

    // Telegraph sprite frames.
    constexpr int trail_normal_frame = 0;
    constexpr int trail_heavy_frame = 1;

    // Where hero slashes cross the enemy's body, and how long the slash streak is.
    constexpr bn::fixed_point body_center(0, -12);
    constexpr int hit_streak_half_length = 40;

    // Palette fades, applied only when they change.
    enum fade_kind
    {
        FADE_NONE,
        FADE_FLASH_WHITE,
        FADE_FLASH_RED,
        FADE_DIM,
        FADE_DEFEAT     // Grows every frame while sinking
    };

    // Length for states that only end when the duel scene says so.
    constexpr int forever = 1 << 30;

    constexpr bn::fixed defeat_fade_max = 0.7;
}

enemy::enemy(const tuning::enemy_tier& tier) :
    _tier(tier),
    _bg(bn::regular_bg_items::enemy_idle.create_bg(0, 0))
{
    _bg.set_priority(bg_priority);

    for(int index = 0; index < int(attack_id::COUNT); ++index)
    {
        attack_id id = attack_id(index);

        if(tier.diagonal_slashes || ! attack_type_of(id).diagonal)
        {
            _attack_pool.push_back(id);
        }
    }

    for(int index = 0; index < trail_dots; ++index)
    {
        bn::sprite_ptr dot = bn::sprite_items::telegraph.create_sprite(0, 0);
        dot.set_bg_priority(telegraph_priority);
        dot.set_visible(false);
        _trail.push_back(bn::move(dot));
    }

    _sequence.push_back(attack_step{ attack_id::OVERHEAD_CHOP });
    _enter(enemy_state::IDLE, _tier.pause_between_sequences);
}

int enemy::frames_to_impact() const
{
    switch(_state)
    {

    case enemy_state::WINDUP:
        return _state_length - _state_frames;

    case enemy_state::STRIKE:
        return _state_frames == 0 ? 0 : -1;

    default:
        return -1;
    }
}

void enemy::update()
{
    _impact = false;
    _sequence_finished = false;
    ++_state_frames;

    if(_hit_frames > 0)
    {
        --_hit_frames;
    }

    if(_state_frames >= _state_length)
    {
        _advance();
    }

    _update_effects();
    _update_trail();
}

void enemy::reel(int frames)
{
    _enter(enemy_state::STAGGERED, frames);
}

void enemy::open(int frames)
{
    _enter(enemy_state::OPEN, frames);
}

void enemy::take_hit(int direction, int damage)
{
    _hp = _hp > damage ? _hp - damage : 0;
    _hit_frames = tuning::enemy_hit_frames;

    // Streak across the body along the slash direction.
    bn::fixed_point step((direction & dir::LEFT) ? -1 : (direction & dir::RIGHT) ? 1 : 0,
                         (direction & dir::UP) ? -1 : (direction & dir::DOWN) ? 1 : 0);
    bn::fixed_point offset = step * hit_streak_half_length;
    _place_trail(body_center - offset, body_center + offset, trail_normal_frame);
}

void enemy::defeat()
{
    _hit_frames = 0;
    _enter(enemy_state::DEFEATED, forever);
}

void enemy::stand_down()
{
    _enter(enemy_state::IDLE, forever);
}

void enemy::_advance()
{
    switch(_state)
    {

    case enemy_state::IDLE:
        _start_sequence();
        _enter(enemy_state::WINDUP, windup_frames(current_attack(), _tier));
        break;

    case enemy_state::WINDUP:
        _enter(enemy_state::STRIKE, tuning::strike_frames);
        _impact = true;
        break;

    case enemy_state::STRIKE:
        if(! on_last_attack())
        {
            ++_sequence_index;
            _enter(enemy_state::RECOVER, _tier.recover_between_attacks);
        }
        else
        {
            // The duel scene may turn this pause into an opening.
            _enter(enemy_state::IDLE, _tier.pause_between_sequences);
            _sequence_finished = true;
        }
        break;

    case enemy_state::RECOVER:
        _enter(enemy_state::WINDUP, windup_frames(current_attack(), _tier));
        break;

    case enemy_state::STAGGERED:
        if(! on_last_attack())
        {
            ++_sequence_index;
            _enter(enemy_state::WINDUP, windup_frames(current_attack(), _tier));
        }
        else
        {
            _enter(enemy_state::IDLE, _tier.pause_between_sequences);
        }
        break;

    default:    // OPEN: guard back up
        _enter(enemy_state::IDLE, _tier.pause_between_sequences);
        break;
    }
}

void enemy::_enter(enemy_state state, int length)
{
    _state = state;
    _state_frames = 0;
    _state_length = length;

    switch(state)
    {

    case enemy_state::WINDUP:
        {
            const attack_step& step = current_attack();
            const attack_type& type = attack_type_of(step.id);
            _bg.set_item(*windup_poses[int(step.id)]);
            _place_trail(type.swing_from, type.swing_to, step.heavy ? trail_heavy_frame : trail_normal_frame);
        }
        break;

    case enemy_state::STRIKE:
        _bg.set_item(*strike_poses[int(current_attack().id)]);
        break;

    default:
        _bg.set_item(bn::regular_bg_items::enemy_idle);
        break;
    }
}

void enemy::_start_sequence()
{
    int count = _random.get_int(tuning::sequence_min_attacks, _tier.max_attacks + 1);
    _sequence.clear();
    _sequence_index = 0;

    for(int index = 0; index < count; ++index)
    {
        attack_step step;
        step.id = _attack_pool[_random.get_int(_attack_pool.size())];
        step.heavy = _random.get_int(100) < tuning::heavy_chance_percent;
        _sequence.push_back(step);
    }
}

void enemy::_place_trail(bn::fixed_point from, bn::fixed_point to, int frame)
{
    bn::fixed_point delta = to - from;

    for(int index = 0; index < trail_dots; ++index)
    {
        bn::sprite_ptr& dot = _trail[index];
        dot.set_position(from + (delta * index) / (trail_dots - 1));
        dot.set_tiles(bn::sprite_items::telegraph.tiles_item(), frame);
    }
}

void enemy::_update_effects()
{
    bool windup_flash = _state == enemy_state::WINDUP && _state_frames < tuning::telegraph_start_flash;
    bool reel_flash = _state == enemy_state::STAGGERED && _state_frames < tuning::enemy_stagger_flash;
    bool hit_flash = _hit_frames > tuning::enemy_hit_frames - tuning::enemy_hit_flash;

    // Open: dimmed and slumped; blinks back to full color as the guard is about to return.
    bool open_dim = false;

    if(_state == enemy_state::OPEN)
    {
        int frames_left = _state_length - _state_frames;
        open_dim = frames_left > tuning::opening_warning_frames || (frames_left / 4) % 2 == 0;
    }

    int fade = FADE_NONE;

    if(_state == enemy_state::DEFEATED)
    {
        int sink = _state_frames < tuning::defeat_sink_frames ? _state_frames : tuning::defeat_sink_frames;
        bn::bg_palette_ptr palette = _bg.palette();
        palette.set_fade(bn::colors::black, (defeat_fade_max * sink) / tuning::defeat_sink_frames);
        _fade = FADE_DEFEAT;
        _bg.set_position(0, (tuning::defeat_sink_distance * sink) / tuning::defeat_sink_frames);
        return;
    }

    if(hit_flash || reel_flash)
    {
        fade = FADE_FLASH_WHITE;
    }
    else if(windup_flash)
    {
        fade = current_attack().heavy ? FADE_FLASH_RED : FADE_FLASH_WHITE;
    }
    else if(open_dim)
    {
        fade = FADE_DIM;
    }

    if(fade != _fade)
    {
        bn::bg_palette_ptr palette = _bg.palette();

        switch(fade)
        {

        case FADE_FLASH_WHITE:
            palette.set_fade(bn::colors::white, bn::fixed(0.5));
            break;

        case FADE_FLASH_RED:
            palette.set_fade(bn::colors::red, bn::fixed(0.5));
            break;

        case FADE_DIM:
            palette.set_fade(bn::colors::black, bn::fixed(0.3));
            break;

        default:
            palette.set_fade(bn::colors::black, 0);
            break;
        }

        _fade = fade;
    }

    // Shake when reeling or hit; slump while open.
    int shake_x = 0;

    if(_state == enemy_state::STAGGERED && _state_frames < tuning::enemy_stagger_shake_frames)
    {
        shake_x = (_state_frames / 2) % 2 ? tuning::enemy_stagger_shake : -tuning::enemy_stagger_shake;
    }
    else if(_hit_frames > 0)
    {
        shake_x = (_hit_frames / 2) % 2 ? tuning::enemy_hit_shake : -tuning::enemy_hit_shake;
    }

    int slump_y = _state == enemy_state::OPEN ? tuning::enemy_open_slump : 0;
    _bg.set_position(shake_x, slump_y);
}

void enemy::_update_trail()
{
    // Hit streak takes priority: shown solid for a few frames after a hero slash.
    if(_hit_frames > tuning::enemy_hit_frames - tuning::hit_streak_frames)
    {
        for(bn::sprite_ptr& dot : _trail)
        {
            dot.set_visible(true);
        }

        return;
    }

    // Telegraph: draws itself along the swing path during the wind-up, blinks just before
    // impact, and stays solid while the swing passes.
    int visible_dots = 0;
    bool blink_off = false;

    if(_state == enemy_state::WINDUP)
    {
        visible_dots = 1 + (_state_frames * trail_dots) / _state_length;

        int to_impact = frames_to_impact();

        if(to_impact <= tuning::telegraph_blink_frames)
        {
            blink_off = (to_impact / 2) % 2 == 1;
        }
    }
    else if(_state == enemy_state::STRIKE)
    {
        visible_dots = trail_dots;
    }

    for(int index = 0; index < trail_dots; ++index)
    {
        _trail[index].set_visible(! blink_off && index < visible_dots);
    }
}

}
