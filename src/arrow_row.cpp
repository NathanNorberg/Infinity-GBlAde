#include "arrow_row.h"

#include "bn_sprite_items_arrows.h"

#include "attack_data.h"

namespace igb
{

namespace
{
    constexpr int priority = 0;
    constexpr int spacing = 18;
    constexpr int mistake_flash_frames = 12;

    // Frames in graphics/arrows.bmp: up, down, left, right for each state.
    constexpr int dim_first_frame = 0;
    constexpr int lit_first_frame = 4;
    constexpr int mistake_first_frame = 8;

    [[nodiscard]] int direction_frame(int direction)
    {
        return direction == dir::UP ? 0 : direction == dir::DOWN ? 1 : direction == dir::LEFT ? 2 : 3;
    }
}

void arrow_row::show(const int* steps, int length, bn::fixed_point center)
{
    if(visible())
    {
        return;
    }

    _length = length < max_arrows ? length : max_arrows;
    _progress = 0;
    _mistake_frames = 0;

    bn::fixed left = center.x() - (spacing * (_length - 1)) / 2;

    for(int index = 0; index < _length; ++index)
    {
        _steps[index] = steps[index];
        bn::sprite_ptr arrow = bn::sprite_items::arrows.create_sprite(left + index * spacing, center.y());
        arrow.set_bg_priority(priority);
        _arrows.push_back(bn::move(arrow));
    }

    _refresh();
}

void arrow_row::hide()
{
    _arrows.clear();
}

void arrow_row::set_progress(int progress)
{
    if(progress != _progress)
    {
        _progress = progress;
        _refresh();
    }
}

void arrow_row::flash_mistake()
{
    _mistake_frames = mistake_flash_frames;
    _refresh();
}

void arrow_row::update()
{
    if(_mistake_frames > 0 && --_mistake_frames == 0)
    {
        _refresh();
    }
}

void arrow_row::_refresh()
{
    for(int index = 0; index < _arrows.size(); ++index)
    {
        int first_frame = _mistake_frames > 0 ? mistake_first_frame :
                          index < _progress ? lit_first_frame : dim_first_frame;
        _arrows[index].set_tiles(bn::sprite_items::arrows.tiles_item(), first_frame + direction_frame(_steps[index]));
    }
}

}
