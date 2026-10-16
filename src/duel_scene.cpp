#include "duel_scene.h"

#include "bn_bg_palette_ptr.h"
#include "bn_bg_palettes.h"
#include "bn_colors.h"
#include "bn_keypad.h"
#include "bn_sstream.h"

#include "bn_regular_bg_items_arena.h"

#include "castle_run.h"
#include "item_roster.h"

namespace igb
{

namespace
{
    // Layer order, back to front: arena (3), enemy (2), hero sprite (1), telegraph and HUD (0).
    constexpr int arena_priority = 3;
}

duel_scene::duel_scene(debug_settings& debug, hero_profile& profile) :
    _tier(tuning::scale_for_cycle(tuning::combine(*tuning::enemy_tiers[debug.tier_index],
                                                  *tuning::enemy_types[debug.type_index]),
                                  debug.type_index, profile.cycle)),
    _debug_settings(debug),
    _profile(profile),
    _arena_bg(bn::regular_bg_items::arena.create_bg(0, 0)),
    _enemy(_tier, *tuning::enemy_types[debug.type_index]),
    _hero(profile, profile.in_run ? profile.run_hp : profile.max_hp()),
    _hud(_tier.name, profile.max_hp(), profile.shield_pips(), profile.magic_charges()),
    _debug(debug, profile),
    _combo(profile.weapon_combo()),
    _ring(profile.ring_spell()),
    _magic_charges(profile.magic_charges()),
    _last_outcome("-"),
    _run_duel(profile.in_run && profile.run_duel_hotspot >= 0 && ! debug.enemy_override)
{
    _arena_bg.set_priority(arena_priority);
    _hero.set_spell(spell_of(_ring));

    // The title and hub dim this same arena art; make sure the duel starts at full brightness.
    bn::bg_palette_ptr arena_palette = _arena_bg.palette();
    arena_palette.set_fade(bn::colors::black, 0);
}

scene_id duel_scene::update()
{
    _result_decided = false;

    // Start pauses (with the debug overlay hidden, during the duel); Start again resumes.
    if(_paused)
    {
        if(bn::keypad::start_pressed())
        {
            _set_paused(false);
        }

        return scene_id::NONE;
    }

    if(bn::keypad::start_pressed() && ! bn::keypad::select_held() && ! _debug_settings.visible &&
       _result == result::NONE)
    {
        _set_paused(true);
        return scene_id::NONE;
    }

    ++_frames;

    // Super first: fired on the impact frame, it still stops the blade.
    if(bn::keypad::l_pressed())
    {
        _try_super();
    }

    // Hero first so input on the impact frame still counts.
    _hero.set_enemy_open(_enemy.state() == enemy_state::OPEN);
    _hero.set_magic_ready(_magic_charges > 0 && _result == result::NONE);
    _hero.set_input_suppressed(bn::keypad::select_held());
    _hero.update();

    if(_hero.channel_started())
    {
        --_magic_charges;
    }

    if(_hero.cast_started())
    {
        _cast_spell();
    }

    if(_hero.attack_started())
    {
        _resolve_attack();
    }

    // While the hero draws a spell, the enemy moves in slow motion.
    bool slowed = _hero.state() == hero_state::CHANNEL;
    _slow_frames = slowed ? _slow_frames + 1 : 0;
    bool enemy_moves = ! slowed || _slow_frames % tuning::magic_slow_divisor == 0;

    if(slowed != _slowed)
    {
        bn::bg_palette_ptr arena_palette = _arena_bg.palette();
        arena_palette.set_fade(bn::color(4, 8, 24), slowed ? bn::fixed(0.3) : bn::fixed(0));
        _slowed = slowed;
    }

    if(enemy_moves)
    {
        _enemy.update();
        _update_burn();

        // Combos and super damage only count inside one opening.
        if(_enemy.state() != enemy_state::OPEN)
        {
            _combo.reset();
            _super_opening = false;
        }

        if(_enemy.impact())
        {
            _resolve_impact();
        }

        if(_enemy.sequence_finished())
        {
            _end_sequence(false);
        }
    }

    _arena_bg.set_position(_hero.arena_nudge());
    _update_prompts();
    _hud.update(_hero.hp(), _hero.max_hp(), _hero.shield_pips(), _super, _magic_charges, _enemy.hp(),
                _enemy.max_hp());

    if(_debug.update(_enemy, _hero, _last_outcome.c_str(), _frames, _result != result::NONE, _magic_charges))
    {
        return scene_id::DUEL;
    }

    if(_debug.refill_requested() && _result == result::NONE)
    {
        _super = tuning::super_meter_max;
        _magic_charges = _profile.magic_charges();
        _hud.show_popup("REFILLED");
    }

    if(_result == result::NONE)
    {
        _check_result();
        return scene_id::NONE;
    }

    ++_result_frames;

    if(_result_frames == tuning::result_prompt_delay)
    {
        if(_profile.in_run || _died_in_run || _god_king_slain)
        {
            _hud.show_prompt("START: continue");
        }
        else
        {
            _hud.show_prompt(_result == result::WON ? "START: back to the gate    A: character" : "START: back to the gate");
        }
    }

    if(_result_frames < tuning::result_prompt_delay)
    {
        return scene_id::NONE;
    }

    if(bn::keypad::start_pressed())
    {
        return _died_in_run ? scene_id::NEW_GENERATION : _god_king_slain ? scene_id::ENDING :
               _profile.in_run ? scene_id::AREA : scene_id::HUB;
    }

    if(_result == result::WON && ! _profile.in_run && ! _god_king_slain && bn::keypad::a_pressed())
    {
        return scene_id::CHARACTER;
    }

    return scene_id::NONE;
}

void duel_scene::_update_prompts()
{
    // Spell pattern while drawing; combo during openings. Both always shown.
    if(_hero.state() == hero_state::CHANNEL)
    {
        _hud.show_spell_prompt(spell_of(_ring), _hero.spell_progress());

        if(_hero.spell_mistake())
        {
            _hud.spell_mistake();
        }
    }
    else
    {
        _hud.hide_spell_prompt();
    }

    if(_enemy.state() == enemy_state::OPEN && _result == result::NONE)
    {
        _hud.show_combo_hint(_combo.current(), _combo.progress());
    }
    else
    {
        _hud.hide_combo_hint();
    }
}

void duel_scene::_set_paused(bool paused)
{
    _paused = paused;

    // Dim the arena and enemy; the HUD and pause text are sprites and stay bright.
    bn::bg_palettes::set_fade(bn::colors::black, paused ? bn::fixed(0.6) : bn::fixed(0));

    if(paused)
    {
        _hud.hide_spell_prompt();
        _hud.hide_combo_hint();
        _hud.show_pause(_profile.worn(item_slot::RING), _profile.worn(item_slot::WEAPON), _magic_charges);
    }
    else
    {
        _hud.hide_pause();
    }
}

void duel_scene::_check_result()
{
    if(_enemy.hp() == 0)
    {
        _result = result::WON;
        _result_decided = true;
        _enemy.defeat();
        _hero.lock_input();
        _rewards = compute_rewards(_tier, _profile, _had_perfect_sequence, _blocked);
        apply_rewards(_rewards, _profile);

        // In a run: the duel's hotspot is won, HP carries over, and elite guards may drop a key.
        // Beating the God King ends the run (the ending and new game plus come later).
        if(_run_duel)
        {
            const castle::area& area = run::current_area(_profile);
            int hotspot = _profile.run_duel_hotspot;
            run::mark_cleared(_profile, hotspot);
            // A little HP comes back after each win; the rest carries into the next duel.
            int heal = _hero.max_hp() * tuning::win_heal_percent / 100;
            int hp = _hero.hp() + heal < _hero.max_hp() ? _hero.hp() + heal : _hero.max_hp();
            _rewards.healed = hp - _hero.hp();
            _profile.run_hp = hp;
            _profile.run_duel_hotspot = -1;

            if(int(run::roll(_profile, hotspot, 5) % 100) < area.key_drop_percent)
            {
                ++_profile.keys;
                _rewards.found_key = true;
            }

            // Beating the God King ends the run and starts the next cycle (new game plus).
            if(_profile.run_area == castle::throne_room)
            {
                run::end(_profile);
                ++_profile.cycle;
                _god_king_slain = true;
            }
        }

        _hud.show_victory(_rewards, _profile);
    }
    else if(_hero.hp() == 0)
    {
        _result = result::LOST;
        _result_decided = true;
        _hero.defeat();
        _enemy.stand_down();
        _hud.show_result("DEFEATED");

        // In a run, death ends it: part of the gold is lost and the next generation takes up
        // the sword at the gate.
        if(_run_duel)
        {
            run::die(_profile);
            _died_in_run = true;
        }
    }
}

void duel_scene::_resolve_impact()
{
    const attack_step& step = _enemy.current_attack();
    // Debug invincibility: hits still land (flash, stun, shield break) but cost no HP.
    int hit_damage = _debug_settings.invincible ? 0 : _profile.damage_taken(damage(step, _tier));

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
        if(parries(_hero.parry_direction(), step) && started <= _profile.parry_window())
        {
            ++_parries;
            _add_super(tuning::super_gain_parry);
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
        _blocked = true;

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
    if(_enemy.state() != enemy_state::OPEN)
    {
        return;
    }

    int previous_progress = _combo.progress();
    bool finisher = _combo.add(_hero.attack_direction());

    // A broken chain flashes the combo hint; a wrong first slash doesn't, so free slashing
    // isn't nagged.
    if(! finisher && previous_progress > 0 && _combo.progress() <= previous_progress)
    {
        _hud.combo_mistake();
    }

    int bonus = finisher ? _combo.current().bonus_damage : 0;
    int damage = _profile.slash_damage() + bonus;

    if(_super_opening)
    {
        damage = damage * tuning::super_damage_percent / 100;
        bonus = bonus * tuning::super_damage_percent / 100;
    }

    _enemy.take_hit(_hero.attack_direction(), damage, finisher);
    _add_super(tuning::super_gain_hit);

    if(finisher)
    {
        bn::string<20> text;
        bn::ostringstream stream(text);
        stream << "COMBO +" << bonus;

        // The weapon's element triggers on the finisher.
        switch(_profile.weapon_element())
        {

        case element::FIRE:
            _burn_ticks_left = tuning::fire_burn_ticks;
            _burn_tick_damage = _profile.slash_damage() * tuning::fire_burn_percent / 100 / tuning::fire_burn_ticks;
            _burn_frames = 0;
            stream << " FIRE";
            break;

        case element::ICE:
            _enemy.extend_opening(tuning::ice_opening_bonus);
            stream << " ICE";
            break;

        case element::SHOCK:
            _enemy.stun_after_opening(tuning::shock_after_stun);
            stream << " SHOCK";
            break;

        default:
            break;
        }

        _hud.show_popup(text.c_str());
    }
}

void duel_scene::_update_burn()
{
    // Fire weapon: small ticks of damage spread evenly over the burn.
    if(_burn_ticks_left <= 0 || _enemy.state() == enemy_state::DEFEATED)
    {
        return;
    }

    if(++_burn_frames >= tuning::fire_burn_frames / tuning::fire_burn_ticks)
    {
        _burn_frames = 0;
        --_burn_ticks_left;
        _enemy.take_damage(_burn_tick_damage);
    }
}

void duel_scene::_try_super()
{
    if(_super < tuning::super_meter_max || _result != result::NONE || ! _hero.can_use_super() ||
       _enemy.state() == enemy_state::DEFEATED)
    {
        return;
    }

    // Stun right away, even mid-wind-up: the sequence is abandoned and the opening is gold.
    _super = 0;
    _super_opening = true;
    _enemy.open(tuning::super_open_frames, open_kind::SUPER);
    _end_sequence_tally();
    _combo.reset();
    _hero.flash();
    _hud.show_popup("SUPER!");
    _set_outcome("Super");
}

void duel_scene::_cast_spell()
{
    // The charge was spent when R was pressed. Every spell knocks the enemy out of his
    // sequence so the hero never comes back from casting into a half-finished swing; cast
    // during an opening, the opening keeps running instead.
    bool open = _enemy.state() == enemy_state::OPEN;

    if(! open)
    {
        _end_sequence_tally();
        _combo.reset();
    }

    bn::string<16> text;
    bn::ostringstream stream(text);

    switch(_ring)
    {

    case spell_id::FIRE:
        {
            if(! open)
            {
                _enemy.interrupt(tuning::fire_stagger_frames);
            }

            int fire = _profile.spell_damage(tuning::fire_damage);
            _enemy.take_hit(dir::UP, fire, true);
            stream << "FIRE -" << fire;
        }
        break;

    case spell_id::ICE:
        // Frozen: an opening.
        _enemy.open(_profile.spell_power(tuning::ice_freeze_frames), open_kind::FROZEN);
        stream << "ICE";
        break;

    case spell_id::HEAL:
        {
            int amount = _profile.spell_power(_hero.max_hp() * tuning::heal_percent / 100);
            _hero.heal(amount);

            if(! open)
            {
                _enemy.interrupt(tuning::heal_reset_frames);
            }

            stream << "HEAL +" << amount;
        }
        break;

    default:    // SHOCK
        {
            if(! open)
            {
                _enemy.interrupt(_profile.spell_power(tuning::shock_stagger_frames));
            }

            int shock = _profile.spell_damage(tuning::shock_damage);
            _enemy.take_hit(dir::DOWN, shock);
            stream << "SHOCK -" << shock;
        }
        break;
    }

    _hud.show_popup(text.c_str());
    _set_outcome("Cast");
}

void duel_scene::_add_super(int amount)
{
    // The meter doesn't fill while a super opening is running.
    if(! _super_opening)
    {
        _super = _super + amount < tuning::super_meter_max ? _super + amount : tuning::super_meter_max;
    }
}

void duel_scene::_end_sequence_tally()
{
    _dodges = 0;
    _parries = 0;
    _blocks = 0;
    _hits = 0;
}

void duel_scene::_end_sequence(bool last_parried)
{
    int size = _enemy.sequence_size();

    if(size >= 2 && _parries == size)
    {
        _had_perfect_sequence = true;
    }

    if(int opening = _earned_opening(last_parried))
    {
        _enemy.open(opening);
    }

    _end_sequence_tally();
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
