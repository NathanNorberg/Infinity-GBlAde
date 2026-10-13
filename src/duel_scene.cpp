#include "duel_scene.h"

#include "bn_keypad.h"
#include "bn_sstream.h"

#include "bn_regular_bg_items_arena.h"

namespace igb
{

namespace
{
    // Layer order, back to front: arena (3), enemy (2), hero sprite (1), telegraph and HUD (0).
    constexpr int arena_priority = 3;
}

duel_scene::duel_scene(debug_settings& debug) :
    _tier(*tuning::enemy_tiers[debug.tier_index]),
    _arena_bg(bn::regular_bg_items::arena.create_bg(0, 0)),
    _enemy(_tier),
    _hud(_tier.name),
    _debug(debug),
    _last_outcome("-")
{
    _arena_bg.set_priority(arena_priority);
}

bool duel_scene::update()
{
    ++_frames;

    // Hero first so input on the impact frame still counts.
    _hero.set_enemy_open(_enemy.state() == enemy_state::OPEN);
    _hero.update();

    if(_hero.attack_started())
    {
        _resolve_attack();
    }

    _enemy.update();

    if(_enemy.impact())
    {
        _resolve_impact();
    }

    if(_enemy.sequence_finished())
    {
        _end_sequence(false);
    }

    _arena_bg.set_position(_hero.arena_nudge());
    _hud.update(_hero.hp(), _hero.shield_pips(), _enemy.hp());

    if(_debug.update(_enemy, _hero, _last_outcome.c_str(), _frames))
    {
        return false;
    }

    if(_result == result::NONE)
    {
        _check_result();
        return true;
    }

    ++_result_frames;

    if(_result_frames == tuning::result_prompt_delay)
    {
        _hud.show_prompt("Press START to duel again");
    }

    return _result_frames < tuning::result_prompt_delay || ! bn::keypad::start_pressed();
}

void duel_scene::_check_result()
{
    if(_enemy.hp() == 0)
    {
        _result = result::WON;
        _enemy.defeat();
        _hero.lock_input();
        _hud.show_result("VICTORY");
    }
    else if(_hero.hp() == 0)
    {
        _result = result::LOST;
        _hero.defeat();
        _enemy.stand_down();
        _hud.show_result("DEFEATED");
    }
}

void duel_scene::_resolve_impact()
{
    const attack_step& step = _enemy.current_attack();
    int hit_damage = damage(step, _tier);

    // Frames before impact that the hero's current move started; the windows count back
    // from the impact frame.
    int started = _hero.state_frames();

    // A dodge counts if it points away from the blade and started no more than
    // dodge_window frames before impact. Too early and the blade catches the hero.
    if(int evade = _hero.evade_direction())
    {
        if(evade & dodge_answers(step))
        {
            if(started <= tuning::dodge_window)
            {
                ++_dodges;
                _set_outcome("Dodge ok", started);
                return;
            }

            _set_outcome("Dodge early", started);
        }
        else
        {
            _set_outcome("Dodge wrong way");
        }

        ++_hits;
        _hero.hurt(hit_damage);
        return;
    }

    // A parry counts if it points toward where the blade comes from and started no more
    // than parry_window frames before impact. A miss takes the full hit.
    if(_hero.state() == hero_state::PARRY)
    {
        if(parries(_hero.parry_direction(), step) && started <= tuning::parry_window)
        {
            ++_parries;
            _set_outcome("Parry ok", started);
            _hero.parry_succeeded();

            if(_enemy.on_last_attack())
            {
                _end_sequence(true);
            }
            else
            {
                _enemy.reel(tuning::enemy_parry_reel);
            }

            return;
        }

        if(parry_answer(step) == dir::NONE)
        {
            _set_outcome("Can't parry");
        }
        else if(parries(_hero.parry_direction(), step))
        {
            _set_outcome("Parry early", started);
        }
        else
        {
            _set_outcome("Parry wrong way");
        }

        ++_hits;
        _hero.hurt(hit_damage);
        return;
    }

    if(_hero.state() == hero_state::BLOCK)
    {
        ++_blocks;

        if(blockable(step))
        {
            _set_outcome("Block");
            _hero.absorb_block();
        }
        else
        {
            // Unblockable attacks break the shield and still land.
            _set_outcome("Shield broken");
            _hero.break_shield(hit_damage);
        }

        return;
    }

    ++_hits;
    _set_outcome("Hit");
    _hero.hurt(hit_damage);
}

void duel_scene::_set_outcome(const char* outcome, int frames_before_impact)
{
    _last_outcome.clear();
    bn::ostringstream stream(_last_outcome);
    stream << outcome;

    if(frames_before_impact >= 0)
    {
        stream << " -" << frames_before_impact << "f";
    }
}

void duel_scene::_resolve_attack()
{
    // Slashes only land while the guard is down; after it comes back up they glance off.
    if(_enemy.state() == enemy_state::OPEN)
    {
        _enemy.take_hit(_hero.attack_direction(), tuning::player_slash_damage);
    }
}

void duel_scene::_end_sequence(bool last_parried)
{
    if(int opening = _earned_opening(last_parried))
    {
        _enemy.open(opening);
    }

    _dodges = 0;
    _parries = 0;
    _blocks = 0;
    _hits = 0;
}

int duel_scene::_earned_opening(bool last_parried) const
{
    int size = _enemy.sequence_size();

    // Perfect sequence: every attack in a chain of two or more parried.
    if(size >= 2 && _parries == size)
    {
        return tuning::opening_after_perfect;
    }

    // Parrying the final attack opens the enemy right away.
    if(last_parried)
    {
        return tuning::opening_after_parry;
    }

    // Every attack avoided without blocking or getting hit: a small opening.
    if(_hits == 0 && _blocks == 0 && _dodges + _parries == size)
    {
        return tuning::opening_after_dodge;
    }

    return 0;
}

}
