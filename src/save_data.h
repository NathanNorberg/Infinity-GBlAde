#ifndef IGB_SAVE_DATA_H
#define IGB_SAVE_DATA_H

#include "hero_profile.h"

namespace igb::save
{

constexpr int slot_count = 2;

// What the title screen shows for a slot without loading it into play.
struct slot_summary
{
    bool used = false;
    int level = 0;
    int gold = 0;
};

// Each slot keeps a primary and a backup copy in SRAM, both with a checksum. Writing does the
// primary first, then the backup, so power loss mid-write always leaves one good copy.
void write(int slot, const hero_profile& profile);

// Loads the primary copy if it's valid, otherwise the backup. Returns false if neither is.
[[nodiscard]] bool read(int slot, hero_profile& profile);

[[nodiscard]] slot_summary summary(int slot);

// Makes the slot empty (both copies).
void erase(int slot);

}

#endif
