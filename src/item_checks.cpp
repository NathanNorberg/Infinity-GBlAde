#include "item_roster.h"

// Compile-time checks on the item roster, so a bad edit to include/item_roster.h fails the
// build instead of producing an item that breaks a duel or the save data.

namespace igb
{

namespace
{
    constexpr bool every_slot_has_one_starter()
    {
        for(int slot = 0; slot < int(item_slot::COUNT); ++slot)
        {
            int starters = 0;

            for(const item& item : roster::items)
            {
                if(item.slot == item_slot(slot) && item.starter())
                {
                    if(item.unlock_level != 1)
                    {
                        return false;
                    }

                    ++starters;
                }
            }

            if(starters != 1)
            {
                return false;
            }
        }

        return true;
    }

    constexpr bool common_fields_are_valid()
    {
        for(const item& item : roster::items)
        {
            if(item.price < 0 || item.xp_multiplier < 1 || item.xp_multiplier > 3 ||
               item.unlock_level < 1 || item.unlock_level > tuning::level_cap)
            {
                return false;
            }
        }

        return true;
    }

    constexpr bool slot_fields_are_valid()
    {
        for(const item& item : roster::items)
        {
            switch(item.slot)
            {

            case item_slot::WEAPON:
                if(item.damage <= 0 || ! item.weapon_combo || item.weapon_combo->length < 2 ||
                   item.weapon_combo->length > combo::max_steps || item.weapon_combo->bonus_damage < 0)
                {
                    return false;
                }
                break;

            case item_slot::SHIELD:
                if(item.pips < 1 || item.pips > tuning::max_shield_pips || item.parry_frames < 0 ||
                   item.refill_frames <= 0)
                {
                    return false;
                }
                break;

            case item_slot::ARMOR:
            case item_slot::HELMET:
                if(item.reduction < 0 || item.max_hp_bonus < 0)
                {
                    return false;
                }
                break;

            default:    // RING
                if(item.charges < 1 || item.charges > tuning::max_magic_charges || item.power_percent <= 0)
                {
                    return false;
                }
                break;
            }
        }

        return true;
    }

    // Every 3-step combo fits inside the shortest (dodge) opening; longer ones need a
    // parry, perfect or super opening, which is deliberate.
    constexpr bool short_combos_fit_dodge_openings()
    {
        for(const item& item : roster::items)
        {
            if(item.slot == item_slot::WEAPON && item.weapon_combo->length <= 3 &&
               (item.weapon_combo->length - 1) * tuning::player_attack_cooldown >= tuning::opening_after_dodge)
            {
                return false;
            }
        }

        return true;
    }

    constexpr const item& starter(item_slot slot)
    {
        return roster::items[roster::starter_item(slot)];
    }
}

static_assert(roster::item_count <= roster::max_items, "Save data only has room for max_items items");
static_assert(every_slot_has_one_starter(), "Every slot needs exactly one free starter item, unlocked at level 1");
static_assert(common_fields_are_valid(), "An item has a bad price, XP multiplier or unlock level");
static_assert(slot_fields_are_valid(), "An item has bad stats for its slot");
static_assert(short_combos_fit_dodge_openings(), "A 3-step combo no longer fits inside the dodge opening");

// The starter kit matches the duel the game was tuned with before items existed.
static_assert(starter(item_slot::WEAPON).damage == tuning::player_slash_damage, "Starter weapon damage changed");
static_assert(starter(item_slot::SHIELD).pips == tuning::shield_pips, "Starter shield pips changed");
static_assert(starter(item_slot::SHIELD).refill_frames == tuning::shield_regen_frames, "Starter shield refill changed");
static_assert(starter(item_slot::RING).charges == tuning::magic_charges, "Starter ring charges changed");
static_assert(starter(item_slot::RING).ring_spell == spell_id::FIRE, "The hero starts with a Fire ring");

}
