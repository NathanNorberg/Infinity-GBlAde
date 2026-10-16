#ifndef IGB_DUEL_SCENE_H
#define IGB_DUEL_SCENE_H

#include "bn_regular_bg_ptr.h"
#include "bn_string.h"

#include "combo.h"
#include "debug_overlay.h"
#include "enemy.h"
#include "hero.h"
#include "hud.h"
#include "scene.h"

namespace igb
{

// Runs one duel: owns the arena, the enemy and the hero, and turns input into outcomes.
class duel_scene
{

public:
    // profile outlives the duel; rewards are written back to it later.
    // A castle run's duel when the profile is in a run with a duel hotspot picked and no
    // debug enemy override; otherwise a stand-alone duel (debug).
    duel_scene(debug_settings& debug, hero_profile& profile);

    // Returns NONE while the duel runs, DUEL to start a fresh one (after a result, or a
    // debug change), or CHARACTER for the character screen from the victory screen.
    [[nodiscard]] scene_id update();

    // True on the frame the duel is won or lost, so the game can autosave right away.
    [[nodiscard]] bool result_decided() const
    {
        return _result_decided;
    }

private:
    tuning::enemy_tier _tier;          // This duel's tier combined with the enemy type
    const debug_settings& _debug_settings;
    hero_profile& _profile;
    bn::regular_bg_ptr _arena_bg;
    enemy _enemy;
    hero _hero;
    hud _hud;
    debug_overlay _debug;
    combo_tracker _combo;
    int _super = 0;
    spell_id _ring;
    int _magic_charges;
    int _slow_frames = 0;              // Counts frames while the enemy is slowed by spell drawing
    bool _slowed = false;
    bool _paused = false;
    bool _super_opening = false;       // The current opening came from the super: double damage
    int _burn_ticks_left = 0;          // Fire weapon burn after a combo
    int _burn_tick_damage = 0;
    int _burn_frames = 0;

    // Duel-wide records for the reward bonuses.
    bool _had_perfect_sequence = false;
    bool _blocked = false;
    duel_rewards _rewards;
    bn::string<24> _last_outcome;      // Result of the last impact, for the debug overlay
    int _frames = 0;

    enum class result
    {
        NONE,
        WON,
        LOST
    };

    result _result = result::NONE;
    bool _result_decided = false;
    bool _run_duel;                    // Part of a castle run: HP carries over, results go back to the area
    bool _died_in_run = false;
    bool _god_king_slain = false;
    int _result_frames = 0;

    // Defensive results in the current enemy sequence; they decide the opening.
    int _dodges = 0;
    int _parries = 0;
    int _blocks = 0;
    int _hits = 0;

    void _check_result();
    void _update_prompts();
    void _set_paused(bool paused);
    void _resolve_impact();
    void _set_outcome(const char* outcome, int frames_before_impact = -1);
    void _resolve_attack();
    void _update_burn();
    void _try_super();
    void _cast_spell();
    void _end_sequence_tally();
    void _add_super(int amount);
    void _end_sequence(bool last_parried);
    [[nodiscard]] int _earned_opening(bool last_parried) const;
};

}

#endif
