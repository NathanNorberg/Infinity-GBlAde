#include "castle_run.h"

namespace igb::run
{

void start(hero_profile& profile, unsigned seed)
{
    profile.in_run = true;
    profile.run_area = castle::gate;
    profile.run_cleared = 0;
    profile.run_hp = profile.max_hp();
    profile.run_seed = seed;
    profile.run_duel_hotspot = -1;
}

void end(hero_profile& profile)
{
    profile.in_run = false;
    profile.run_area = castle::gate;
    profile.run_cleared = 0;
    profile.run_duel_hotspot = -1;
}

void die(hero_profile& profile)
{
    end(profile);
    profile.last_gold_lost = profile.gold * tuning::death_gold_loss_percent / 100;
    profile.gold -= profile.last_gold_lost;
    ++profile.bloodline;
}

bool interrupted_duel(const hero_profile& profile)
{
    return profile.in_run && profile.run_duel_hotspot >= 0;
}

const castle::area& current_area(const hero_profile& profile)
{
    return castle::areas[profile.run_area];
}

bool cleared(const hero_profile& profile, int hotspot)
{
    return (profile.run_cleared >> hotspot) & 1;
}

void mark_cleared(hero_profile& profile, int hotspot)
{
    profile.run_cleared |= 1 << hotspot;
}

bool duels_cleared(const hero_profile& profile)
{
    const castle::area& area = current_area(profile);

    for(int index = 0; index < area.hotspot_count; ++index)
    {
        if(area.hotspots[index].kind == castle::hotspot_kind::DUEL && ! cleared(profile, index))
        {
            return false;
        }
    }

    return true;
}

void enter_area(hero_profile& profile, int area)
{
    profile.run_area = area;
    profile.run_cleared = 0;
}

unsigned roll(const hero_profile& profile, int hotspot, unsigned salt)
{
    // Small integer hash (xorshift-multiply) of the seed, area, hotspot and purpose.
    unsigned value = profile.run_seed ^ (unsigned(profile.run_area) * 0x9E3779B9u) ^
                     (unsigned(hotspot) * 0x85EBCA6Bu) ^ (salt * 0xC2B2AE35u);
    value ^= value >> 16;
    value *= 0x7FEB352Du;
    value ^= value >> 15;
    value *= 0x846CA68Bu;
    value ^= value >> 16;
    return value;
}

int duel_type(const hero_profile& profile, int hotspot)
{
    const castle::area& area = current_area(profile);
    return area.types[roll(profile, hotspot, 1) % unsigned(area.type_count)];
}

}
