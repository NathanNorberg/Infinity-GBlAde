#ifndef IGB_ITEMS_H
#define IGB_ITEMS_H

#include "combo.h"
#include "magic.h"

namespace igb
{

// The hero wears one item per slot.
enum class item_slot
{
    WEAPON,
    SHIELD,
    ARMOR,
    HELMET,
    RING,
    COUNT
};

// Indexed by item_slot.
constexpr const char* slot_names[] = { "Weapon", "Shield", "Armor", "Helmet", "Ring" };

static_assert(sizeof(slot_names) / sizeof(slot_names[0]) == int(item_slot::COUNT));

// A weapon's element triggers when its combo completes.
enum class element
{
    NONE,
    FIRE,       // The finisher sets the enemy burning
    ICE,        // The finisher extends the opening
    SHOCK       // The enemy stays stunned after the opening ends
};

// One item. Only the fields for its slot are used.
struct item
{
    const char* name;
    item_slot slot;
    int price;                     // 0 = starter (owned from the beginning, never sold in the shop)
    int xp_multiplier;             // ×1 to ×3 on duel XP while unmastered
    int unlock_level;              // Shop starts selling it at this hero level

    // Weapon
    int damage = 0;                // Per slash, before the Attack stat
    element weapon_element = element::NONE;
    const combo* weapon_combo = nullptr;

    // Shield
    int pips = 0;                  // Base pips, before the Shield stat
    int parry_frames = 0;          // Added to the parry window
    int refill_frames = 0;         // Frames for one pip to come back while not blocking

    // Armor and helmet
    int reduction = 0;             // Subtracted from every enemy hit
    int max_hp_bonus = 0;

    // Ring
    spell_id ring_spell = spell_id::FIRE;
    int charges = 0;               // Base charges per duel, before the Magic stat
    int power_percent = 100;       // Scales the spell's main effect

    [[nodiscard]] constexpr bool starter() const
    {
        return price == 0;
    }
};

constexpr item make_weapon(const char* name, int damage, element weapon_element, const combo& weapon_combo,
                           int price, int xp_multiplier, int unlock_level)
{
    item result{ name, item_slot::WEAPON, price, xp_multiplier, unlock_level };
    result.damage = damage;
    result.weapon_element = weapon_element;
    result.weapon_combo = &weapon_combo;
    return result;
}

constexpr item make_shield(const char* name, int pips, int parry_frames, int refill_frames, int price,
                           int xp_multiplier, int unlock_level)
{
    item result{ name, item_slot::SHIELD, price, xp_multiplier, unlock_level };
    result.pips = pips;
    result.parry_frames = parry_frames;
    result.refill_frames = refill_frames;
    return result;
}

constexpr item make_armor(const char* name, int reduction, int price, int xp_multiplier, int unlock_level)
{
    item result{ name, item_slot::ARMOR, price, xp_multiplier, unlock_level };
    result.reduction = reduction;
    return result;
}

constexpr item make_helmet(const char* name, int reduction, int max_hp_bonus, int price, int xp_multiplier,
                           int unlock_level)
{
    item result{ name, item_slot::HELMET, price, xp_multiplier, unlock_level };
    result.reduction = reduction;
    result.max_hp_bonus = max_hp_bonus;
    return result;
}

constexpr item make_ring(const char* name, spell_id ring_spell, int charges, int power_percent, int price,
                         int xp_multiplier, int unlock_level)
{
    item result{ name, item_slot::RING, price, xp_multiplier, unlock_level };
    result.ring_spell = ring_spell;
    result.charges = charges;
    result.power_percent = power_percent;
    return result;
}

}

#endif
