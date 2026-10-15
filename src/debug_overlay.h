#ifndef IGB_DEBUG_OVERLAY_H
#define IGB_DEBUG_OVERLAY_H

#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "hero_profile.h"
#include "tuning.h"

namespace igb
{

class enemy;
class hero;

// Survives duel restarts (owned by main).
struct debug_settings
{
    bool visible = false;
    bool invincible = false;        // Hits still register and flash, but cost no HP
    int tier_index = tuning::first_duel_tier;
    int type_index = tuning::first_duel_type;
    bool enemy_override = false;    // Select + L/R picked the enemy; the hub's Fight clears it
    int gear_slot = 0;              // item_slot that Select + A changes
    bool select_combo_used = false; // Select + L/R was used, so releasing Select doesn't toggle
};

// Tapping Select toggles a text overlay with the enemy and hero state machines, frame counts
// and the result of the last impact. Holding Select (which keeps the hero still):
// L / R cycle the enemy tier / type, B picks a gear slot and A equips the next item in it,
// Start refills the super meter and magic charges, and the D-pad adds a stat point
// (up Health, right Attack, down Shield, left Magic). While the overlay is shown, Start
// toggles invincibility (except on the win/lose screen, where Start restarts).
class debug_overlay
{

public:
    debug_overlay(debug_settings& settings, hero_profile& profile);

    // Returns true when a new tier, type, item or stat point was picked; the duel should
    // restart with it. refill_requested() is set on the frame Select + Start is pressed.
    [[nodiscard]] bool update(const enemy& enemy, const hero& hero, const char* last_outcome, int duel_frames,
                              bool duel_over, int magic_charges);

    [[nodiscard]] bool refill_requested() const
    {
        return _refill_requested;
    }

private:
    static constexpr int lines = 7;
    static constexpr int max_line_sprites = 8;

    using line_text = bn::string<48>;

    debug_settings& _settings;
    hero_profile& _profile;
    bool _refill_requested = false;
    bn::sprite_text_generator _text_generator;
    bn::vector<bn::sprite_ptr, max_line_sprites> _line_sprites[lines];
    line_text _line_texts[lines];

    void _equip_next(item_slot slot);
    void _show_line(int line, const line_text& text);
    void _hide();
};

}

#endif
