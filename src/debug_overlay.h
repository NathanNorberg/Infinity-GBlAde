#ifndef IGB_DEBUG_OVERLAY_H
#define IGB_DEBUG_OVERLAY_H

#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "tuning.h"

namespace igb
{

class enemy;
class hero;

// Survives duel restarts (owned by main).
struct debug_settings
{
    bool visible = false;
    int tier_index = tuning::first_duel_tier;
};

// Select toggles a text overlay with the enemy and hero state machines, frame counts and
// the result of the last impact. While it is shown, L and R switch the enemy tier.
class debug_overlay
{

public:
    explicit debug_overlay(debug_settings& settings);

    // Returns true when L or R picked a new tier; the duel should restart with it.
    [[nodiscard]] bool update(const enemy& enemy, const hero& hero, const char* last_outcome, int duel_frames);

private:
    static constexpr int lines = 5;
    static constexpr int max_line_sprites = 6;

    using line_text = bn::string<40>;

    debug_settings& _settings;
    bn::sprite_text_generator _text_generator;
    bn::vector<bn::sprite_ptr, max_line_sprites> _line_sprites[lines];
    line_text _line_texts[lines];

    void _show_line(int line, const line_text& text);
    void _hide();
};

}

#endif
