#include "bn_core.h"
#include "bn_optional.h"

#include "character_scene.h"
#include "duel_scene.h"
#include "gear_scene.h"
#include "hero_profile.h"
#include "hub_scene.h"
#include "save_data.h"
#include "title_scene.h"

int main()
{
    bn::core::init();

    igb::debug_settings debug;
    igb::hero_profile profile;
    int active_slot = 0;

    // One scene at a time; the old one is freed before the next is built, so their sprites
    // and backgrounds never overlap.
    bn::optional<igb::title_scene> title;
    bn::optional<igb::hub_scene> hub;
    bn::optional<igb::duel_scene> duel;
    bn::optional<igb::character_scene> character;
    bn::optional<igb::gear_scene> gear;

    igb::scene_id current = igb::scene_id::TITLE;
    title.emplace(profile, active_slot);

    while(true)
    {
        igb::scene_id next = igb::scene_id::NONE;

        switch(current)
        {

        case igb::scene_id::TITLE:
            next = title->update();
            break;

        case igb::scene_id::HUB:
            next = hub->update();
            break;

        case igb::scene_id::DUEL:
            next = duel->update();

            // Autosave the moment a duel is decided, before the result screen.
            if(duel->result_decided())
            {
                igb::save::write(active_slot, profile);
            }
            break;

        case igb::scene_id::CHARACTER:
            next = character->update();
            break;

        default:
            next = gear->update();
            break;
        }

        if(next != igb::scene_id::NONE)
        {
            title.reset();
            hub.reset();
            duel.reset();
            character.reset();
            gear.reset();

            switch(next)
            {

            case igb::scene_id::TITLE:
                title.emplace(profile, active_slot);
                break;

            case igb::scene_id::HUB:
                // Autosave on every return to the gate: after duels, shop visits, equipment
                // changes and spent stat points.
                if(current != igb::scene_id::TITLE)
                {
                    igb::save::write(active_slot, profile);
                }

                hub.emplace(profile, active_slot);
                break;

            case igb::scene_id::DUEL:
                // The hub's Fight always takes the ladder's next duel; debug restarts keep a
                // Select + L/R override.
                if(current == igb::scene_id::HUB)
                {
                    debug.enemy_override = false;
                }

                if(! debug.enemy_override)
                {
                    const igb::tuning::ladder_duel& step = igb::tuning::fight_ladder[profile.ladder_step];
                    debug.tier_index = step.tier_index;
                    debug.type_index = step.type_index;
                }

                duel.emplace(debug, profile);
                break;

            case igb::scene_id::CHARACTER:
                character.emplace(profile);
                break;

            case igb::scene_id::EQUIPMENT:
                gear.emplace(igb::gear_scene::mode::EQUIPMENT, profile);
                break;

            default:
                gear.emplace(igb::gear_scene::mode::SHOP, profile);
                break;
            }

            current = next;
        }

        bn::core::update();
    }
}
