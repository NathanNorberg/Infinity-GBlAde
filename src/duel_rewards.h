#ifndef IGB_DUEL_REWARDS_H
#define IGB_DUEL_REWARDS_H

#include "leveling.h"

namespace igb
{

// What a won duel pays, computed once at victory and applied to the profile.
struct duel_rewards
{
    int base_xp = 0;               // From the enemy's tier and type
    int xp = 0;                    // After bonuses
    bool perfect_bonus = false;    // At least one perfect parry sequence
    bool no_block_bonus = false;   // Never blocked
    int gold = 0;
    int item_xp[int(item_slot::COUNT)] = {};   // By slot, for the items worn in the duel
    int levels_gained = 0;         // Filled in by apply_rewards
    bool newly_mastered[int(item_slot::COUNT)] = {};  // Filled in by apply_rewards
};

// base_xp and gold come from the combined tier + type; the bonuses from how the duel went.
[[nodiscard]] constexpr duel_rewards compute_rewards(const tuning::enemy_tier& enemy, const hero_profile& profile,
                                                     bool perfect_sequence, bool blocked)
{
    duel_rewards rewards;
    rewards.base_xp = enemy.xp_reward;
    rewards.perfect_bonus = perfect_sequence;
    rewards.no_block_bonus = ! blocked;
    rewards.gold = enemy.gold_reward;

    int bonus_percent = (perfect_sequence ? tuning::perfect_sequence_xp_bonus : 0) +
                        (blocked ? 0 : tuning::no_block_xp_bonus);
    rewards.xp = rewards.base_xp * (100 + bonus_percent) / 100;

    // Each worn, unmastered item earns the duel's XP times its multiplier.
    for(int slot = 0; slot < int(item_slot::COUNT); ++slot)
    {
        bool mastered = profile.mastered(profile.equipped[slot]);
        rewards.item_xp[slot] = mastered ? 0 : rewards.xp * profile.worn(item_slot(slot)).xp_multiplier;
    }

    return rewards;
}

inline void apply_rewards(duel_rewards& rewards, hero_profile& profile)
{
    rewards.levels_gained = add_xp(profile, rewards.xp);
    profile.gold += rewards.gold;

    for(int slot = 0; slot < int(item_slot::COUNT); ++slot)
    {
        int index = profile.equipped[slot];

        if(profile.mastered(index))
        {
            continue;
        }

        // Item XP stops at the mastery line; crossing it the first (and only) time pays stat points.
        int needed = mastery_xp(roster::items[index]);
        int total = profile.item_xp[index] + rewards.item_xp[slot];
        profile.item_xp[index] = total < needed ? total : needed;

        if(profile.mastered(index))
        {
            profile.unspent_points += tuning::mastery_stat_points;
            rewards.newly_mastered[slot] = true;
        }
    }
}

}

#endif
