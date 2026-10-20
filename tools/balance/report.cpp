// Balance report: a quick look at how the tuning plays out over many castle runs, built from
// the game's own data headers so it always matches the ROM. Not part of the game build.
//
//   tools/balance/build.sh && tools/balance/report > "docs/Infinity Blade GBA Balance Report.md"
//
// Model (deliberately simple, for spotting outliers rather than predicting play):
// - The hero wins every duel and takes every treasure, alternating the garden and barracks
//   paths, with no XP bonuses. Item drops from chests are ignored; their gold value isn't.
// - Stat points alternate Health and Attack. After each run the hero buys the best unlocked
//   item they can afford for the weapon, armor, helmet and shield, in that order.
// - Enemy hits are the average of the type's attack mix (what its tier throws), with
//   unblockables weighted in, after armor and helmet.

#include <cstdarg>
#include <cstdio>

#include "castle.h"
#include "duel_rewards.h"

using namespace igb;

namespace
{

// Per-run detail is printed for the first runs only; later runs feed the summary tables.
bool detailed = true;

void detail(const char* format, ...)
{
    if(! detailed)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    std::vprintf(format, args);
    va_end(args);
}

constexpr int attack_damage[tuning::attack_kinds] = {
    tuning::damage_diagonal_slash, tuning::damage_diagonal_slash, tuning::damage_horizontal_slash,
    tuning::damage_horizontal_slash, tuning::damage_overhead_chop, tuning::damage_low_sweep,
    tuning::damage_thrust, tuning::damage_rising_slash, tuning::damage_rising_slash,
};

bool throws(const tuning::enemy_tier& tier, int attack)
{
    if(attack <= 1)
    {
        return tier.diagonal_slashes;
    }

    if(attack >= 7)
    {
        return tier.rising_slashes;
    }

    return true;
}

tuning::enemy_tier enemy_for(int tier_index, int type_index, int cycle)
{
    tuning::enemy_tier enemy = tuning::combine(*tuning::enemy_tiers[tier_index], *tuning::enemy_types[type_index]);
    return tuning::scale_for_cycle(enemy, type_index, cycle);
}

// Average damage of one enemy hit on this hero, after armor.
double average_hit(const tuning::enemy_tier& enemy, int type_index, const hero_profile& profile)
{
    const tuning::enemy_type& type = *tuning::enemy_types[type_index];
    double total = 0;
    double weights = 0;

    for(int attack = 0; attack < tuning::attack_kinds; ++attack)
    {
        if(! throws(enemy, attack))
        {
            continue;
        }

        int base = attack_damage[attack] * enemy.damage_percent / 100;
        int heavy = base * tuning::heavy_damage_percent / 100;
        double hit = profile.damage_taken(base) * (100 - enemy.heavy_percent) / 100.0 +
                     profile.damage_taken(heavy) * enemy.heavy_percent / 100.0;
        total += hit * type.attack_weights[attack];
        weights += type.attack_weights[attack];
    }

    return total / weights;
}

int enemy_hp(const tuning::enemy_tier& enemy, int type_index)
{
    return tuning::enemy_types[type_index]->max_hp * enemy.hp_percent / 100;
}

void spend_points(hero_profile& profile, int& next_stat)
{
    while(profile.unspent_points > 0)
    {
        (next_stat % 2 == 0 ? profile.health : profile.attack) += 1;
        --profile.unspent_points;
        ++next_stat;
    }
}

void shop(hero_profile& profile)
{
    const item_slot order[] = { item_slot::WEAPON, item_slot::ARMOR, item_slot::HELMET, item_slot::SHIELD };

    for(item_slot slot : order)
    {
        int best = -1;

        for(int index = 0; index < roster::item_count; ++index)
        {
            const item& candidate = roster::items[index];

            if(candidate.slot == slot && profile.can_buy(index) && candidate.price <= profile.gold &&
               candidate.price > profile.worn(slot).price && (best < 0 || candidate.price > roster::items[best].price))
            {
                best = index;
            }
        }

        if(best >= 0)
        {
            profile.gold -= roster::items[best].price;
            profile.owned[best] = true;
            profile.equipped[int(slot)] = best;
            detail("  - Bought %s for %d gold\n", roster::items[best].name, roster::items[best].price);
        }
    }
}

// One duel row, averaged over the types the area can roll.
void duel_row(const castle::area& area, const hero_profile& profile, int cycle)
{
    for(int index = 0; index < area.type_count; ++index)
    {
        int type_index = area.types[index];
        tuning::enemy_tier enemy = enemy_for(area.tier_index, type_index, cycle);
        double hit = average_hit(enemy, type_index, profile);
        int hp = enemy_hp(enemy, type_index);
        int slash = profile.slash_damage();
        detail("| %s | %s | %d | %d | %.1f | %d | %.1f |\n", area.name, look::looks[area.looks[index]].name, hp,
                    (hp + slash - 1) / slash, hit, profile.max_hp(), profile.max_hp() / hit);
    }
}


// One summary row per run: the hero against the gate's first guard and the God King.
struct run_summary
{
    int cycle;
    int level;
    int max_hp;
    int slash;
    int gate_slashes;
    double gate_hits_to_die;
    int king_slashes;
    double king_hits_to_die;
    int xp;
};

int slashes_to_kill(const tuning::enemy_tier& enemy, int type_index, const hero_profile& profile)
{
    int hp = enemy_hp(enemy, type_index);
    return (hp + profile.slash_damage() - 1) / profile.slash_damage();
}

}

int main()
{
    hero_profile profile;
    int next_stat = 0;
    constexpr int detailed_runs = 6;
    constexpr int runs = 40;
    run_summary summaries[runs] = {};

    std::printf("# Infinity Blade GBA: Balance Report\n\n");
    std::printf("Generated by `tools/balance/report` from the game's data headers (tuning.h, item_roster.h, "
                "castle.h). Regenerate after changing them:\n\n```bash\ntools/balance/build.sh && tools/balance/report > "
                "\"docs/Infinity Blade GBA Balance Report.md\"\n```\n\n");
    std::printf("**Model:** the hero wins every duel and takes every treasure, alternating the garden and barracks "
                "paths, with no XP bonuses and no chest items, and beats the God King at the end of every run, so "
                "each run is one cycle further on (every enemy gains %d%% HP, %d%% damage and %d%% XP and gold per "
                "cycle; the God King %d%% HP). Stat points alternate Health and Attack; after each run the hero buys "
                "the best affordable weapon, armor, helmet and shield. It's optimistic (real runs end in deaths, "
                "which don't advance the cycle), so read it for outliers and pacing, not exact numbers.\n\n",
                tuning::enemy_hp_percent_per_cycle, tuning::enemy_damage_percent_per_cycle,
                tuning::enemy_reward_percent_per_cycle, tuning::god_king_hp_percent_per_cycle);
    std::printf("**Columns:** *Slashes* is hits to kill the enemy at the hero's slash damage (combos, supers and "
                "spells shorten it). *Hits to die* is the hero's max HP over the enemy's average hit after armor, "
                "with unblockables weighted in; in a run, HP carries between duels, healing %d%% per win.\n\n",
                tuning::win_heal_percent);

    for(int run = 1; run <= runs; ++run)
    {
        detailed = run <= detailed_runs;
        bool garden = run % 2 == 1;

        run_summary& summary = summaries[run - 1];
        tuning::enemy_tier gate_guard = enemy_for(castle::areas[castle::gate].tier_index, castle::knight, profile.cycle);
        tuning::enemy_tier god_king = enemy_for(castle::areas[castle::throne_room].tier_index,
                                                tuning::god_king_type_index, profile.cycle);
        summary = { profile.cycle + 1, profile.level, profile.max_hp(), profile.slash_damage(),
                    slashes_to_kill(gate_guard, castle::knight, profile),
                    profile.max_hp() / average_hit(gate_guard, castle::knight, profile),
                    slashes_to_kill(god_king, tuning::god_king_type_index, profile),
                    profile.max_hp() / average_hit(god_king, tuning::god_king_type_index, profile), 0 };

        detail("## Run %d (cycle %d, %s path)\n\n", run, profile.cycle + 1, garden ? "garden" : "barracks");
        detail("Start: level %d, %d max HP, %d per slash, %s / %s / %s / %s, %d gold.\n\n", profile.level,
               profile.max_hp(), profile.slash_damage(), profile.worn(item_slot::WEAPON).name,
               profile.worn(item_slot::ARMOR).name, profile.worn(item_slot::HELMET).name,
               profile.worn(item_slot::SHIELD).name, profile.gold);
        detail("| Area | Enemy | HP | Slashes | Avg hit | Hero HP | Hits to die |\n");
        detail("| --- | --- | --- | --- | --- | --- | --- |\n");

        const int route[] = { castle::gate, garden ? castle::garden : castle::barracks, castle::great_hall,
                              castle::throne_approach, castle::throne_room };
        int run_xp = 0;
        int run_gold = 0;
        int levels_before = profile.level;

        for(int area_index : route)
        {
            const castle::area& area = castle::areas[area_index];
            duel_row(area, profile, profile.cycle);

            for(int spot = 0; spot < area.hotspot_count; ++spot)
            {
                const castle::hotspot& hotspot = area.hotspots[spot];

                if(hotspot.kind == castle::hotspot_kind::DUEL)
                {
                    // Average XP and gold over the types this area rolls.
                    int xp = 0;
                    int gold = 0;

                    for(int index = 0; index < area.type_count; ++index)
                    {
                        tuning::enemy_tier enemy = enemy_for(area.tier_index, area.types[index], profile.cycle);
                        xp += enemy.xp_reward;
                        gold += enemy.gold_reward;
                    }

                    duel_rewards rewards = compute_rewards(tuning::enemy_tier { "", 0, 0, 0, 0, false, 0, 0, 0, false,
                                                                                xp / area.type_count, gold / area.type_count },
                                                           profile, false, true);
                    apply_rewards(rewards, profile);
                    run_xp += rewards.xp;
                    run_gold += rewards.gold;
                }
                else if(hotspot.kind == castle::hotspot_kind::GOLD)
                {
                    profile.gold += (area.gold_min + area.gold_max) / 2;
                    run_gold += (area.gold_min + area.gold_max) / 2;
                }
                else if(hotspot.kind == castle::hotspot_kind::CHEST)
                {
                    int gold = (area.gold_min + area.gold_max) * (100 - area.chest_item_percent) / 100;
                    profile.gold += gold;
                    run_gold += gold;
                }
            }
        }

        summary.xp = run_xp;

        // The God King's win starts the next cycle.
        ++profile.cycle;
        detail("\nRun total: %d XP, %d gold, levels %d to %d.\n\n", run_xp, run_gold, levels_before, profile.level);
        spend_points(profile, next_stat);
        shop(profile);
        detail("\n");
    }

    std::printf("## Every run\n\n");
    std::printf("The hero at the start of each run against the gate's first guard (a Gate Sentry) and the God King.\n\n");
    std::printf("| Run | Cycle | Level | Max HP | Per slash | Gate guard slashes | Gate guard hits to die | "
                "God King slashes | God King hits to die | Run XP |\n");
    std::printf("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |\n");

    for(int run = 0; run < runs; ++run)
    {
        const run_summary& row = summaries[run];
        std::printf("| %d | %d | %d | %d | %d | %d | %.1f | %d | %.1f | %d |\n", run + 1, row.cycle, row.level,
                    row.max_hp, row.slash, row.gate_slashes, row.gate_hits_to_die, row.king_slashes,
                    row.king_hits_to_die, row.xp);
    }

    std::printf("\n## Pacing\n\nThe first run the hero starts at each level where new items unlock:\n\n");
    std::printf("| Level | Unlocks | First run |\n| --- | --- | --- |\n");

    for(int level = 2; level <= tuning::level_cap; ++level)
    {
        int unlocks = 0;
        const char* example = "";

        for(int index = 0; index < roster::item_count; ++index)
        {
            if(roster::items[index].unlock_level == level && roster::items[index].price > 0)
            {
                ++unlocks;
                example = roster::items[index].name;
            }
        }

        if(unlocks == 0 && level != tuning::level_cap)
        {
            continue;
        }

        int first_run = 0;

        for(int run = 0; run < runs && ! first_run; ++run)
        {
            first_run = summaries[run].level >= level ? run + 1 : 0;
        }

        if(first_run)
        {
            std::printf("| %d | %s%s | %d |\n", level, unlocks ? example : "(level cap)", unlocks > 1 ? " and more" : "",
                        first_run);
        }
        else
        {
            std::printf("| %d | %s%s | after %d |\n", level, unlocks ? example : "(level cap)",
                        unlocks > 1 ? " and more" : "", runs);
        }
    }

    return 0;
}
