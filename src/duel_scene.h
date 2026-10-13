#ifndef IGB_DUEL_SCENE_H
#define IGB_DUEL_SCENE_H

#include "bn_regular_bg_ptr.h"
#include "bn_string.h"

#include "debug_overlay.h"
#include "enemy.h"
#include "hero.h"
#include "hud.h"

namespace igb
{

// Runs one duel: owns the arena, the enemy and the hero, and turns input into outcomes.
class duel_scene
{

public:
    explicit duel_scene(debug_settings& debug);

    // Returns false when the duel should restart: the player asked for a new one after
    // a result, or picked another enemy tier in the debug overlay.
    [[nodiscard]] bool update();

private:
    const tuning::enemy_tier& _tier;
    bn::regular_bg_ptr _arena_bg;
    enemy _enemy;
    hero _hero;
    hud _hud;
    debug_overlay _debug;
    bn::string<24> _last_outcome;      // Result of the last impact, for the debug overlay
    int _frames = 0;

    enum class result
    {
        NONE,
        WON,
        LOST
    };

    result _result = result::NONE;
    int _result_frames = 0;

    // Defensive results in the current enemy sequence; they decide the opening.
    int _dodges = 0;
    int _parries = 0;
    int _blocks = 0;
    int _hits = 0;

    void _check_result();
    void _resolve_impact();
    void _set_outcome(const char* outcome, int frames_before_impact = -1);
    void _resolve_attack();
    void _end_sequence(bool last_parried);
    [[nodiscard]] int _earned_opening(bool last_parried) const;
};

}

#endif
