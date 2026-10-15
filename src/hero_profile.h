#ifndef IGB_HERO_PROFILE_H
#define IGB_HERO_PROFILE_H

#include "item_roster.h"
#include "tuning.h"

namespace igb
{

// Item XP needed to master an item; scales with its price.
constexpr int mastery_xp(const item& item)
{
    return tuning::mastery_base_xp + item.price / tuning::mastery_price_divisor;
}

// Which items the hero owns, by item index. Starts with the free starter kit.
struct owned_items
{
    bool flags[roster::max_items] = {};

    constexpr owned_items()
    {
        for(int index = 0; index < roster::item_count; ++index)
        {
            flags[index] = roster::items[index].starter();
        }
    }

    constexpr bool& operator[](int index)
    {
        return flags[index];
    }

    constexpr bool operator[](int index) const
    {
        return flags[index];
    }
};

// Everything about the hero that outlives a duel: level, XP, stat points, gold, owned and
// equipped items and item mastery. Keys and the bloodline join it later; save data writes it
// to SRAM.
struct hero_profile
{
    int level = 1;
    int xp = 0;                    // Toward the next level; resets on level-up
    int unspent_points = 0;
    int gold = 0;
    int ladder_step = 0;           // Next duel on the hub's fight ladder

    // Stat points spent.
    int health = 0;
    int attack = 0;
    int shield = 0;
    int magic = 0;

    // Item index (into roster::items) worn in each slot, by item_slot.
    int equipped[int(item_slot::COUNT)] = {
        roster::starter_item(item_slot::WEAPON),
        roster::starter_item(item_slot::SHIELD),
        roster::starter_item(item_slot::ARMOR),
        roster::starter_item(item_slot::HELMET),
        roster::starter_item(item_slot::RING),
    };

    // XP each item has earned, by item index; mastery reads this.
    int item_xp[roster::max_items] = {};

    owned_items owned;

    [[nodiscard]] constexpr const item& worn(item_slot slot) const
    {
        return roster::items[equipped[int(slot)]];
    }

    // Mastered items keep their stats but stop earning item XP.
    [[nodiscard]] constexpr bool mastered(int item_index) const
    {
        return item_xp[item_index] >= mastery_xp(roster::items[item_index]);
    }

    [[nodiscard]] constexpr bool wearing(int item_index) const
    {
        return equipped[int(roster::items[item_index].slot)] == item_index;
    }

    // Shop rules: buy if it's unlocked, affordable and not owned; sell if owned, not worn and
    // not a free starter.
    [[nodiscard]] constexpr bool can_buy(int item_index) const
    {
        const item& item = roster::items[item_index];
        return ! owned[item_index] && level >= item.unlock_level && gold >= item.price;
    }

    [[nodiscard]] constexpr bool can_sell(int item_index) const
    {
        return owned[item_index] && ! wearing(item_index) && ! roster::items[item_index].starter();
    }

    [[nodiscard]] constexpr int sell_price(int item_index) const
    {
        int percent = mastered(item_index) ? tuning::sell_percent_mastered : tuning::sell_percent;
        return roster::items[item_index].price * percent / 100;
    }

    // Combat values for the next duel: equipped items plus the progression spec's stat rules.

    [[nodiscard]] constexpr int max_hp() const
    {
        return tuning::player_max_hp + health * tuning::hp_per_health_point + worn(item_slot::HELMET).max_hp_bonus;
    }

    [[nodiscard]] constexpr int slash_damage() const
    {
        return worn(item_slot::WEAPON).damage + attack * tuning::damage_per_attack_point;
    }

    [[nodiscard]] constexpr const combo& weapon_combo() const
    {
        return *worn(item_slot::WEAPON).weapon_combo;
    }

    [[nodiscard]] constexpr element weapon_element() const
    {
        return worn(item_slot::WEAPON).weapon_element;
    }

    [[nodiscard]] constexpr int shield_pips() const
    {
        int pips = worn(item_slot::SHIELD).pips + shield / tuning::shield_points_per_pip;
        return pips < tuning::max_shield_pips ? pips : tuning::max_shield_pips;
    }

    [[nodiscard]] constexpr int parry_window() const
    {
        int frames = tuning::parry_window + worn(item_slot::SHIELD).parry_frames +
                     shield / tuning::shield_points_per_parry_frame;
        return frames < tuning::max_parry_window ? frames : tuning::max_parry_window;
    }

    [[nodiscard]] constexpr int shield_refill_frames() const
    {
        return worn(item_slot::SHIELD).refill_frames;
    }

    // Damage an enemy hit actually does after armor and helmet; always at least 1.
    [[nodiscard]] constexpr int damage_taken(int hit) const
    {
        int reduced = hit - worn(item_slot::ARMOR).reduction - worn(item_slot::HELMET).reduction;
        return reduced > 1 ? reduced : 1;
    }

    [[nodiscard]] constexpr spell_id ring_spell() const
    {
        return worn(item_slot::RING).ring_spell;
    }

    [[nodiscard]] constexpr int magic_charges() const
    {
        int charges = worn(item_slot::RING).charges + magic / tuning::magic_points_per_charge;
        return charges < tuning::max_magic_charges ? charges : tuning::max_magic_charges;
    }

    // A spell's main effect (freeze time, heal amount, stun time) scaled by the ring's power.
    [[nodiscard]] constexpr int spell_power(int base) const
    {
        return base * worn(item_slot::RING).power_percent / 100;
    }

    // Spell damage (Fire, Shock): the ring's power, plus +5% per Magic point.
    [[nodiscard]] constexpr int spell_damage(int base) const
    {
        return spell_power(base) * (100 + magic * tuning::spell_percent_per_magic_point) / 100;
    }
};

// However many Shield points, a parry at the edge of the window must still be active at impact.
static_assert(tuning::max_parry_window < tuning::parry_commit, "The parry window cap must stay below the parry commit");
static_assert(tuning::max_parry_window >= tuning::parry_window, "The parry window cap is below the base window");
static_assert(tuning::max_parry_window <= tuning::dodge_window, "Parrying should never be easier than dodging");

// A fresh hero matches the duel the game was tuned with before items existed.
static_assert(hero_profile().max_hp() == tuning::player_max_hp);
static_assert(hero_profile().slash_damage() == tuning::player_slash_damage);
static_assert(hero_profile().shield_pips() == tuning::shield_pips);
static_assert(hero_profile().parry_window() == tuning::parry_window);
static_assert(hero_profile().magic_charges() == tuning::magic_charges);

}

#endif
