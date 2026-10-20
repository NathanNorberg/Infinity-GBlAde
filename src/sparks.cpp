#include "sparks.h"

#include "bn_sprite_items_sparks.h"

#include "tuning.h"

namespace igb
{

namespace
{
    constexpr int priority = 0;            // In front of the hero and enemy
    constexpr int frames_per_burst = 4;    // In graphics/sparks.bmp: gold burst 0..3, blue 4..7
}

void sparks::spawn(bn::fixed_point position, color burst_color)
{
    // Oldest spark makes room if all are busy.
    if(_sparks.full())
    {
        _sparks.erase(_sparks.begin());
    }

    int first_frame = burst_color == color::GOLD ? 0 : frames_per_burst;
    bn::sprite_ptr sprite = bn::sprite_items::sparks.create_sprite(position, first_frame);
    sprite.set_bg_priority(priority);
    _sparks.push_back(spark{ bn::move(sprite), first_frame, 0 });
}

void sparks::update()
{
    for(auto it = _sparks.begin(); it != _sparks.end();)
    {
        int step = ++it->age / tuning::spark_frame_time;

        if(step >= frames_per_burst)
        {
            it = _sparks.erase(it);
        }
        else
        {
            it->sprite.set_tiles(bn::sprite_items::sparks.tiles_item(), it->first_frame + step);
            ++it;
        }
    }
}

}
