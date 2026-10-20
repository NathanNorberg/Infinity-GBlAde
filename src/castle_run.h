#ifndef IGB_CASTLE_RUN_H
#define IGB_CASTLE_RUN_H

#include "castle.h"
#include "hero_profile.h"

namespace igb::run
{

// Starts a run at the gate: full HP, nothing cleared, a fresh seed for enemies and treasure.
void start(hero_profile& profile, unsigned seed);

// Ends the run (death or victory); the hero is back at the gate.
void end(hero_profile& profile);

// Death in the castle: the run ends, part of the gold is lost and the next generation of the
// bloodline takes over. Sets profile.last_gold_lost for the death scene.
void die(hero_profile& profile);

// True if the save was made mid-duel (the game was turned off during a castle duel).
[[nodiscard]] bool interrupted_duel(const hero_profile& profile);

[[nodiscard]] const castle::area& current_area(const hero_profile& profile);

[[nodiscard]] bool cleared(const hero_profile& profile, int hotspot);
void mark_cleared(hero_profile& profile, int hotspot);

// Every duel in the current area won: the exits open.
[[nodiscard]] bool duels_cleared(const hero_profile& profile);

// Moves to another area (through an exit).
void enter_area(hero_profile& profile, int area);

// The enemy type this run rolled for a duel hotspot, and its named look in this area.
[[nodiscard]] int duel_type(const hero_profile& profile, int hotspot);
[[nodiscard]] int duel_look(const hero_profile& profile, int hotspot);

// A random number from the run's seed for a hotspot, so contents stay the same after a reload.
[[nodiscard]] unsigned roll(const hero_profile& profile, int hotspot, unsigned salt);

}

#endif
