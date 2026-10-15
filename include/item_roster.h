#ifndef IGB_ITEM_ROSTER_H
#define IGB_ITEM_ROSTER_H

// The item roster: every weapon, shield, armor, helmet and ring with its stats, price,
// XP multiplier and unlock level, plus the weapon combos. Like tuning.h, this is the file to
// edit when tuning items. The design is in docs/Infinity Blade GBA Item Roster.md.

#include "items.h"

namespace igb::roster
{

// ---------------------------------------------------------------------------------------------
// Weapon combos. Bonus damage is added to the finishing slash.
// ---------------------------------------------------------------------------------------------

constexpr combo cross_cut = { "Cross cut", { dir::LEFT, dir::RIGHT, dir::UP }, 3, 60 };
constexpr combo hammerfall = { "Hammerfall", { dir::UP, dir::DOWN, dir::UP }, 3, 75 };
constexpr combo flicker = { "Flicker", { dir::RIGHT, dir::LEFT, dir::RIGHT }, 3, 90 };
constexpr combo glacier_cut = { "Glacier cut", { dir::DOWN, dir::LEFT, dir::RIGHT }, 3, 100 };
constexpr combo tempest = { "Tempest", { dir::UP, dir::RIGHT, dir::DOWN, dir::LEFT }, 4, 150 };
constexpr combo twin_rend = { "Twin rend", { dir::LEFT, dir::LEFT, dir::RIGHT, dir::RIGHT }, 4, 170 };
constexpr combo wyrm_bite = { "Wyrm bite", { dir::DOWN, dir::UP, dir::LEFT, dir::RIGHT }, 4, 220 };
constexpr combo deicide = { "Deicide", { dir::UP, dir::DOWN, dir::LEFT, dir::RIGHT, dir::UP }, 5, 300 };

// ---------------------------------------------------------------------------------------------
// Items. The index in this list is the item's id in save data, so add new items at the end
// of the list rather than in the middle. Refill times are in frames (60 per second).
// ---------------------------------------------------------------------------------------------

constexpr item items[] = {
    //          Name                   Damage Element         Combo        Price  XP× Unlock
    make_weapon("Squire's Blade",      20, element::NONE,  cross_cut,   0,     2, 1),
    make_weapon("Iron Longsword",      26, element::NONE,  hammerfall,  400,   3, 2),
    make_weapon("Ember Saber",         30, element::FIRE,  flicker,     1200,  2, 5),
    make_weapon("Frostbite Edge",      34, element::ICE,   glacier_cut, 2500,  2, 8),
    make_weapon("Storm Falchion",      40, element::SHOCK, tempest,     5000,  2, 12),
    make_weapon("Knight's Greatsword", 48, element::NONE,  twin_rend,   8000,  2, 16),
    make_weapon("Dragonfang",          58, element::FIRE,  wyrm_bite,   14000, 1, 22),
    make_weapon("Godslayer",           70, element::SHOCK, deicide,     25000, 1, 30),

    //          Name                Pips Parry Refill Price  XP× Unlock
    make_shield("Traveler's Shield", 4, 0, 240, 0,     2, 1),
    make_shield("Oak Kite Shield",   4, 0, 180, 350,   3, 2),
    make_shield("Iron Heater",       5, 0, 240, 1000,  2, 4),
    make_shield("Duelist's Parma",   3, 2, 180, 2200,  2, 7),
    make_shield("Steel Tower",       6, 0, 300, 4500,  2, 11),
    make_shield("Mirror Aegis",      5, 2, 200, 9000,  1, 18),
    make_shield("Bastion of Ages",   6, 3, 180, 18000, 1, 26),

    //         Name               Reduction Price  XP× Unlock
    make_armor("Padded Gambeson", 0, 0,     2, 1),
    make_armor("Leather Jerkin",  1, 300,   3, 2),
    make_armor("Chainmail",       2, 900,   2, 4),
    make_armor("Scale Hauberk",   3, 2000,  2, 7),
    make_armor("Brigandine",      4, 4000,  2, 11),
    make_armor("Knight's Plate",  6, 9000,  1, 17),
    make_armor("Royal Warplate",  8, 18000, 1, 25),

    //          Name             Reduction HP Price  XP× Unlock
    make_helmet("Cloth Hood",     0, 0,  0,     2, 1),
    make_helmet("Leather Cap",    0, 10, 250,   3, 2),
    make_helmet("Iron Skullcap",  1, 10, 800,   2, 4),
    make_helmet("Nasal Helm",     1, 25, 1800,  2, 8),
    make_helmet("Great Helm",     2, 30, 5000,  2, 13),
    make_helmet("Crown of Valor", 3, 50, 14000, 1, 22),

    //        Name              Spell            Charges Power Price  XP× Unlock
    make_ring("Ember Ring",     spell_id::FIRE,  2, 100, 0,     2, 1),
    make_ring("Frost Band",     spell_id::ICE,   2, 100, 500,   3, 3),
    make_ring("Mender's Ring",  spell_id::HEAL,  2, 100, 700,   3, 3),
    make_ring("Spark Ring",     spell_id::SHOCK, 2, 100, 900,   2, 5),
    make_ring("Blaze Signet",   spell_id::FIRE,  3, 130, 4000,  2, 12),
    make_ring("Glacier Seal",   spell_id::ICE,   3, 125, 6000,  2, 15),
    make_ring("Lifebloom Ring", spell_id::HEAL,  3, 150, 7500,  1, 18),
    make_ring("Tempest Ring",   spell_id::SHOCK, 3, 150, 10000, 1, 21),
};

constexpr int item_count = sizeof(items) / sizeof(items[0]);
constexpr int max_items = 100;                 // Save data reserves room for this many

// The free item the hero starts with in a slot.
constexpr int starter_item(item_slot slot)
{
    for(int index = 0; index < item_count; ++index)
    {
        if(items[index].slot == slot && items[index].starter())
        {
            return index;
        }
    }

    return -1;
}

}

#endif
