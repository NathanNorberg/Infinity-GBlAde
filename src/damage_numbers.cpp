#include "damage_numbers.h"

#include "bn_sprite_items_digits.h"

#include "tuning.h"

namespace igb
{

namespace
{
    constexpr int priority = 0;
    constexpr int glyph_spacing = 6;       // Glyphs are 5 px wide inside their 8x8 cells
    constexpr int glyphs_per_color = 11;   // In graphics/digits.bmp: 0-9, then +
    constexpr int plus_glyph = 10;
}

void damage_numbers::spawn(int value, bn::fixed_point position, color number_color)
{
    if(value <= 0)
    {
        return;
    }

    // Oldest number makes room if all are busy.
    if(_numbers.full())
    {
        _numbers.erase(_numbers.begin());
    }

    // Glyphs, most significant first; healing gets a leading plus sign.
    int glyphs[max_glyphs];
    int count = 0;
    int digits[max_glyphs];
    int digit_count = 0;
    int remaining = value < 9999 ? value : 9999;

    while(remaining > 0 && digit_count < max_glyphs)
    {
        digits[digit_count++] = remaining % 10;
        remaining /= 10;
    }

    if(number_color == color::GREEN)
    {
        glyphs[count++] = plus_glyph;
    }

    while(digit_count > 0 && count < max_glyphs)
    {
        glyphs[count++] = digits[--digit_count];
    }

    number entry;
    entry.position = position;
    entry.age = 0;
    int first_frame = int(number_color) * glyphs_per_color;
    bn::fixed left = position.x() - (glyph_spacing * (count - 1)) / 2;

    for(int index = 0; index < count; ++index)
    {
        bn::sprite_ptr glyph = bn::sprite_items::digits.create_sprite(left + index * glyph_spacing, position.y(),
                                                                      first_frame + glyphs[index]);
        glyph.set_bg_priority(priority);
        entry.glyphs.push_back(bn::move(glyph));
    }

    _numbers.push_back(bn::move(entry));
}

void damage_numbers::update()
{
    for(auto it = _numbers.begin(); it != _numbers.end();)
    {
        int age = ++it->age;

        if(age >= tuning::damage_number_frames)
        {
            it = _numbers.erase(it);
            continue;
        }

        // Rise quickly, then hang; blink before disappearing.
        int rise = tuning::damage_number_rise * age / tuning::damage_number_frames;
        bool visible = age < tuning::damage_number_frames - tuning::damage_number_blink || (age / 2) % 2 == 0;

        for(bn::sprite_ptr& glyph : it->glyphs)
        {
            glyph.set_y(it->position.y() - rise);
            glyph.set_visible(visible);
        }

        ++it;
    }
}

}
