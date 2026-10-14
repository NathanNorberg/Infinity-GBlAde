#ifndef IGB_ARROW_ROW_H
#define IGB_ARROW_ROW_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

namespace igb
{

// A centered row of D-pad arrows for spell patterns and combos: entered steps light up gold,
// and a mistake flashes the whole row red for a moment.
class arrow_row
{

public:
    static constexpr int max_arrows = 5;

    // steps are dir:: cardinal directions. Does nothing if the row already shows them.
    void show(const int* steps, int length, bn::fixed_point center);
    void hide();

    void set_progress(int progress);
    void flash_mistake();

    // Call once per frame while shown.
    void update();

    [[nodiscard]] bool visible() const
    {
        return ! _arrows.empty();
    }

private:
    bn::vector<bn::sprite_ptr, max_arrows> _arrows;
    int _steps[max_arrows] = {};
    int _length = 0;
    int _progress = 0;
    int _mistake_frames = 0;

    void _refresh();
};

}

#endif
