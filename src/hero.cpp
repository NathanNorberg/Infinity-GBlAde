#include "hero.h"

#include "bn_colors.h"
#include "bn_keypad.h"
#include "bn_sprite_palette_ptr.h"

#include "bn_sprite_items_hero.h"

#include "attack_data.h"
#include "sfx.h"
#include "tuning.h"

namespace igb
{

namespace
{
    // Lower-left of the screen, over the shoulder. Screen origin is the center.
    constexpr bn::fixed_point home_position(-84, 52);

    // Draw above the enemy BG (priority 2) and arena BG (priority 3); HUD will use 0.
    constexpr int bg_priority = 1;

    // Frames in graphics/hero.bmp, in sheet order.
    constexpr int idle_frame = 0;
    constexpr int dodge_frame = 1;      // Leans right; flipped for a left dodge
    constexpr int duck_frame = 2;
    constexpr int block_frame = 3;
    constexpr int parry_frame = 4;      // Points up-left; flipped for a right parry

    // Palette fades used as feedback, applied only when they change.
    enum flash_kind
    {
        FLASH_NONE,
        FLASH_HURT,
        FLASH_BLOCK,
        FLASH_STAGGER,
        FLASH_CHANNEL   // Blue glow while drawing a spell
    };

    [[nodiscard]] bool is_evade(hero_state state)
    {
        return state == hero_state::DODGE || state == hero_state::DUCK || state == hero_state::HOP;
    }

    // All held D-pad directions as dir:: flags; opposite pairs cancel out.
    [[nodiscard]] int held_directions()
    {
        int horizontal = (bn::keypad::left_held() ? dir::LEFT : 0) | (bn::keypad::right_held() ? dir::RIGHT : 0);
        int vertical = (bn::keypad::up_held() ? dir::UP : 0) | (bn::keypad::down_held() ? dir::DOWN : 0);

        if(horizontal == (dir::LEFT | dir::RIGHT))
        {
            horizontal = 0;
        }

        if(vertical == (dir::UP | dir::DOWN))
        {
            vertical = 0;
        }

        return horizontal | vertical;
    }

    [[nodiscard]] bool any_direction_pressed()
    {
        return bn::keypad::left_pressed() || bn::keypad::right_pressed() ||
               bn::keypad::up_pressed() || bn::keypad::down_pressed();
    }
}

hero::hero(const hero_profile& profile, int start_hp) :
    _sprite(bn::sprite_items::hero.create_sprite(home_position)),
    _max_hp(profile.max_hp()),
    _hp(start_hp > 0 && start_hp < _max_hp ? start_hp : _max_hp),
    _max_pips(profile.shield_pips()),
    _refill_frames(profile.shield_refill_frames()),
    _shield_pips(_max_pips)
{
    _sprite.set_bg_priority(bg_priority);
}

void hero::update()
{
    _attack_started = false;
    _cast_started = false;
    _channel_started = false;
    _spell_mistake = false;
    ++_state_frames;

    if(_evade_cooldown > 0)
    {
        --_evade_cooldown;
    }

    if(_block_flash > 0)
    {
        --_block_flash;
    }

    _update_shield_regen();

    switch(_state)
    {

    case hero_state::DODGE:
    case hero_state::DUCK:
    case hero_state::HOP:
        // Direction pressed a hair before B: treat the chord as a parry, not a dodge.
        if(_state_frames <= tuning::parry_chord_grace && bn::keypad::b_pressed() && ! _input_locked)
        {
            int held = held_directions();
            _start_chord(held ? held : _evade_direction);
        }
        else if(_state_frames >= tuning::dodge_duration)
        {
            _evade_cooldown = tuning::dodge_cooldown;
            _enter(hero_state::IDLE);
        }
        break;

    case hero_state::PARRY:
        if(_state_frames <= tuning::parry_diagonal_settle)
        {
            _settle_parry_direction();
        }

        if(_state_frames >= tuning::parry_commit)
        {
            _enter(hero_state::IDLE);
        }
        break;

    case hero_state::ATTACK:
        if(_state_frames >= tuning::player_attack_cooldown)
        {
            _enter(hero_state::IDLE);
        }
        break;

    case hero_state::BLOCK:
        if(! bn::keypad::a_held())
        {
            _enter(hero_state::IDLE);
        }
        break;

    case hero_state::CHANNEL:
        _update_channel();
        break;

    case hero_state::CAST:
        if(_state_frames >= tuning::magic_cast_frames)
        {
            _enter(hero_state::IDLE);
        }
        break;

    case hero_state::HURT:
        if(_state_frames >= tuning::hero_hurt_frames)
        {
            _enter(hero_state::IDLE);
        }
        break;

    case hero_state::STAGGERED:
        if(_state_frames >= tuning::shield_break_stagger)
        {
            _enter(hero_state::IDLE);
        }
        break;

    default:
        break;
    }

    if(_state == hero_state::IDLE && ! _input_locked && ! _input_suppressed)
    {
        _read_input();
    }

    _chord_buffered = false;

    _update_sprite();
}

void hero::hurt(int damage)
{
    _lose_hp(damage);
    _enter(hero_state::HURT);
}

void hero::defeat()
{
    _input_locked = true;
    _enter(hero_state::DEFEATED);
}

void hero::_lose_hp(int damage)
{
    _hp = _hp > damage ? _hp - damage : 0;
}

void hero::absorb_block()
{
    _shield_pips -= tuning::shield_pips_per_block;
    _block_flash = tuning::block_flash_frames;

    if(_shield_pips <= 0)
    {
        break_shield();
    }
}

void hero::flash()
{
    _block_flash = tuning::block_flash_frames;
}

void hero::heal(int amount)
{
    _hp = _hp + amount < _max_hp ? _hp + amount : _max_hp;
}

void hero::_update_channel()
{
    // Releasing R or running out of time fizzles the spell.
    if(! bn::keypad::r_held() || _state_frames >= tuning::magic_input_frames || ! _spell)
    {
        _enter(hero_state::IDLE);
        return;
    }

    int direction = bn::keypad::left_pressed() ? dir::LEFT :
                    bn::keypad::right_pressed() ? dir::RIGHT :
                    bn::keypad::up_pressed() ? dir::UP :
                    bn::keypad::down_pressed() ? dir::DOWN : dir::NONE;

    if(direction == dir::NONE)
    {
        return;
    }

    if(direction == _spell->steps[_spell_progress])
    {
        ++_spell_progress;
    }
    else
    {
        // A wrong direction restarts the pattern (or starts it, if it's the first step).
        _spell_progress = direction == _spell->steps[0] ? 1 : 0;
        _spell_mistake = true;
    }

    if(_spell_progress == _spell->length)
    {
        _cast_started = true;
        _block_flash = tuning::block_flash_frames;
        _enter(hero_state::CAST);
    }
}

void hero::parry_succeeded()
{
    _block_flash = tuning::block_flash_frames;
    _enter(hero_state::IDLE);
}

void hero::break_shield(int damage)
{
    _lose_hp(damage);
    _shield_pips = 0;
    _shield_regen = 0;
    _enter(hero_state::STAGGERED);
}

int hero::evade_direction() const
{
    return is_evade(_state) ? _evade_direction : dir::NONE;
}

int hero::parry_direction() const
{
    return _state == hero_state::PARRY ? _parry_direction : dir::NONE;
}

bn::fixed_point hero::arena_nudge() const
{
    bn::fixed amount = _evade_amount();

    switch(_state)
    {

    case hero_state::DODGE:
        // Camera follows the hero, so the arena slides the other way.
        return bn::fixed_point(_evade_direction == dir::LEFT ? tuning::arena_nudge * amount :
                                                               -tuning::arena_nudge * amount, 0);

    case hero_state::DUCK:
        return bn::fixed_point(0, -(tuning::arena_nudge / 2) * amount);

    case hero_state::HOP:
        return bn::fixed_point(0, (tuning::arena_nudge / 2) * amount);

    default:
        return bn::fixed_point();
    }
}

void hero::_enter(hero_state state)
{
    _state = state;
    _state_frames = 0;
}

void hero::_read_input()
{
    // R starts drawing a spell while charges remain.
    if(bn::keypad::r_pressed() && _magic_ready)
    {
        _spell_progress = 0;
        _channel_started = true;
        _enter(hero_state::CHANNEL);
        return;
    }

    // Block takes priority over dodging while A is held, as long as the shield has pips.
    if(bn::keypad::a_held() && _shield_pips > 0)
    {
        _enter(hero_state::BLOCK);
        return;
    }

    if(int direction = _read_parry_chord())
    {
        _start_chord(direction);
        return;
    }

    // B + direction is reserved for parries and attacks.
    if(_evade_cooldown > 0 || bn::keypad::b_held())
    {
        return;
    }

    if(bn::keypad::left_pressed())
    {
        _evade_direction = dir::LEFT;
        _enter(hero_state::DODGE);
    }
    else if(bn::keypad::right_pressed())
    {
        _evade_direction = dir::RIGHT;
        _enter(hero_state::DODGE);
    }
    else if(bn::keypad::down_pressed())
    {
        _evade_direction = dir::DOWN;
        _enter(hero_state::DUCK);
    }
    else if(bn::keypad::up_pressed())
    {
        _evade_direction = dir::UP;
        _enter(hero_state::HOP);
    }
    else
    {
        return;
    }

    sfx::play(bn::sound_items::sfx_dodge);
}

void hero::buffer_input()
{
    if(bn::keypad::b_pressed() || (bn::keypad::b_held() && any_direction_pressed()))
    {
        _chord_buffered = true;
    }
}

int hero::_read_parry_chord() const
{
    // B + direction in either order, or both on the same frame (or during a hit-stop freeze,
    // if still held). Reads every held direction so diagonals come through.
    if(bn::keypad::b_pressed() || (bn::keypad::b_held() && (any_direction_pressed() || _chord_buffered)))
    {
        return held_directions();
    }

    return dir::NONE;
}

void hero::_start_parry(int direction)
{
    _parry_direction = direction;
    _enter(hero_state::PARRY);
}

void hero::_start_attack(int direction)
{
    _attack_direction = direction;
    _attack_started = true;
    _enter(hero_state::ATTACK);
}

void hero::_start_chord(int direction)
{
    // B + direction: a slash while the enemy is open, a parry otherwise.
    if(_enemy_open)
    {
        _start_attack(direction);
    }
    else
    {
        _start_parry(direction);
    }
}

void hero::_settle_parry_direction()
{
    // D-pad diagonals often land a frame or two apart. The parry starts on the first
    // direction (so the timing window isn't shortened) and becomes a diagonal if the
    // perpendicular direction arrives within parry_diagonal_settle frames.
    int held = held_directions();
    bool has_horizontal = _parry_direction & (dir::LEFT | dir::RIGHT);
    bool has_vertical = _parry_direction & (dir::UP | dir::DOWN);

    if(! has_horizontal)
    {
        _parry_direction |= held & (dir::LEFT | dir::RIGHT);
    }

    if(! has_vertical)
    {
        _parry_direction |= held & (dir::UP | dir::DOWN);
    }
}

void hero::_update_shield_regen()
{
    bool can_regen = _refill_frames > 0 && _shield_pips < _max_pips &&
                     _state != hero_state::BLOCK && _state != hero_state::STAGGERED;

    if(! can_regen)
    {
        _shield_regen = 0;
        return;
    }

    if(++_shield_regen >= _refill_frames)
    {
        ++_shield_pips;
        _shield_regen = 0;
    }
}

bn::fixed hero::_evade_amount() const
{
    // 0 at rest, 1 fully out of the blade's path: ease out, hold, ease back.
    if(! is_evade(_state))
    {
        return 0;
    }

    int frames_left = tuning::dodge_duration - _state_frames;

    if(_state_frames < tuning::evade_out_frames)
    {
        return bn::fixed(_state_frames + 1) / tuning::evade_out_frames;
    }

    if(frames_left <= tuning::evade_return_frames)
    {
        return bn::fixed(frames_left) / tuning::evade_return_frames;
    }

    return 1;
}

void hero::_update_sprite()
{
    bn::fixed amount = _evade_amount();
    bn::fixed_point position = home_position;
    int frame = idle_frame;
    bool flip = false;

    switch(_state)
    {

    case hero_state::DODGE:
        frame = dodge_frame;
        flip = _evade_direction == dir::LEFT;
        position.set_x(position.x() + (flip ? -tuning::dodge_offset : tuning::dodge_offset) * amount);
        break;

    case hero_state::DUCK:
        frame = duck_frame;
        position.set_y(position.y() + tuning::duck_drop * amount);
        break;

    case hero_state::HOP:
        position.set_y(position.y() - tuning::hop_height * amount);
        break;

    case hero_state::BLOCK:
        frame = block_frame;
        break;

    case hero_state::PARRY:
    case hero_state::ATTACK:
        {
            // Parries and slashes share the swing pose, leaning toward the direction.
            int direction = _state == hero_state::PARRY ? _parry_direction : _attack_direction;
            frame = parry_frame;
            flip = direction & dir::RIGHT;

            if(direction & dir::LEFT)
            {
                position.set_x(position.x() - tuning::parry_offset);
            }
            else if(direction & dir::RIGHT)
            {
                position.set_x(position.x() + tuning::parry_offset);
            }

            if(direction & dir::UP)
            {
                position.set_y(position.y() - tuning::parry_offset);
            }
            else if(direction & dir::DOWN)
            {
                position.set_y(position.y() + tuning::parry_offset);
            }

            if(_state == hero_state::ATTACK)
            {
                position.set_y(position.y() - tuning::attack_lunge);
            }
        }
        break;

    case hero_state::DEFEATED:
        {
            int sink = _state_frames < tuning::defeat_sink_frames ? _state_frames : tuning::defeat_sink_frames;
            position.set_y(position.y() + (tuning::defeat_sink_distance * sink) / tuning::defeat_sink_frames);
        }
        break;

    case hero_state::STAGGERED:
        position.set_x(position.x() + ((_state_frames / 4) % 2 ? tuning::stagger_wobble : -tuning::stagger_wobble));
        break;

    default:
        break;
    }

    if(frame != _shown_frame)
    {
        _sprite.set_tiles(bn::sprite_items::hero.tiles_item(), frame);
        _shown_frame = frame;
    }

    _sprite.set_horizontal_flip(flip);
    _sprite.set_position(position + _screen_offset);

    // Red flash when hit, white flash when a block absorbs a hit, dimmed while staggered.
    int flash = FLASH_NONE;

    if(_state == hero_state::HURT && _state_frames < tuning::hurt_flash_frames)
    {
        flash = FLASH_HURT;
    }
    else if(_block_flash > 0)
    {
        flash = FLASH_BLOCK;
    }
    else if(_state == hero_state::STAGGERED || _state == hero_state::DEFEATED)
    {
        flash = FLASH_STAGGER;
    }
    else if(_state == hero_state::CHANNEL)
    {
        flash = FLASH_CHANNEL;
    }

    if(flash != _flash)
    {
        bn::sprite_palette_ptr palette = _sprite.palette();

        switch(flash)
        {

        case FLASH_HURT:
            palette.set_fade(bn::colors::red, bn::fixed(0.6));
            break;

        case FLASH_BLOCK:
            palette.set_fade(bn::colors::white, bn::fixed(0.5));
            break;

        case FLASH_CHANNEL:
            palette.set_fade(bn::color(8, 16, 31), bn::fixed(0.35));
            break;

        case FLASH_STAGGER:
            palette.set_fade(bn::colors::black, bn::fixed(0.4));
            break;

        default:
            palette.set_fade(bn::colors::black, 0);
            break;
        }

        _flash = flash;
    }
}

}
