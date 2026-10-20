#include "bn_bg_palettes.h"
#include "bn_colors.h"
#include "bn_core.h"
#include "bn_optional.h"
#include "bn_sprite_palettes.h"

#include "area_scene.h"
#include "castle_run.h"
#include "character_scene.h"
#include "duel_scene.h"
#include "gear_scene.h"
#include "hero_profile.h"
#include "hub_scene.h"
#include "save_data.h"
#include "story_scene.h"
#include "title_scene.h"

namespace
{
    // Fades every background and sprite toward black over a number of frames. The scenes hold
    // still meanwhile: nothing updates until the fade finishes.
    void fade(bn::fixed from, bn::fixed to, int frames)
    {
        for(int frame = 1; frame <= frames; ++frame)
        {
            bn::fixed intensity = from + (to - from) * frame / frames;
            bn::bg_palettes::set_fade(bn::colors::black, intensity);
            bn::sprite_palettes::set_fade(bn::colors::black, intensity);
            bn::core::update();
        }
    }
}

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
    bn::optional<igb::area_scene> area;
    bn::optional<igb::story_scene> story;

    igb::scene_id current = igb::scene_id::TITLE;
    bn::bg_palettes::set_fade(bn::colors::black, 1);
    bn::sprite_palettes::set_fade(bn::colors::black, 1);
    title.emplace(profile, active_slot);
    bn::core::update();
    fade(1, 0, igb::tuning::transition_fade_frames);

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

        case igb::scene_id::AREA:
            next = area->update();
            break;

        case igb::scene_id::NEW_GENERATION:
        case igb::scene_id::ENDING:
            next = story->update();
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

        // Loading a save made mid-duel (the game was turned off during a castle duel) counts
        // as losing that duel, so turning the power off can't dodge a death.
        if(current == igb::scene_id::TITLE && next == igb::scene_id::HUB && igb::run::interrupted_duel(profile))
        {
            igb::run::die(profile);
            igb::save::write(active_slot, profile);
            next = igb::scene_id::NEW_GENERATION;
        }

        // A saved run resumes in the castle rather than at the gate.
        if(next == igb::scene_id::HUB && profile.in_run)
        {
            next = igb::scene_id::AREA;
        }

        if(next != igb::scene_id::NONE)
        {
            fade(0, 1, igb::tuning::transition_fade_frames);

            area.reset();
            story.reset();
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

            case igb::scene_id::NEW_GENERATION:
            case igb::scene_id::ENDING:
                story.emplace(next, profile);
                break;

            case igb::scene_id::AREA:
                if(current != igb::scene_id::TITLE)
                {
                    igb::save::write(active_slot, profile);
                }

                area.emplace(profile, active_slot);
                break;

            case igb::scene_id::DUEL:
                // A run's duel takes its enemy from the castle; debug restarts keep a
                // Select + L/R override until the next duel picked in the castle.
                if(current == igb::scene_id::AREA)
                {
                    debug.enemy_override = false;
                }

                if(! debug.enemy_override && profile.in_run && profile.run_duel_hotspot >= 0)
                {
                    debug.tier_index = igb::run::current_area(profile).tier_index;
                    debug.type_index = igb::run::duel_type(profile, profile.run_duel_hotspot);
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

            // Show the new scene's first frame, then fade it in.
            bn::core::update();
            fade(1, 0, igb::tuning::transition_fade_frames);
            continue;
        }

        bn::core::update();
    }
}
