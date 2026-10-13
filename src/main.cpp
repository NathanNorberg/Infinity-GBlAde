#include "bn_core.h"
#include "bn_optional.h"

#include "duel_scene.h"

int main()
{
    bn::core::init();

    igb::debug_settings debug;
    bn::optional<igb::duel_scene> scene;
    scene.emplace(debug);

    while(true)
    {
        if(! scene->update())
        {
            // Free the old duel's sprites and backgrounds before building the new one.
            scene.reset();
            scene.emplace(debug);
        }

        bn::core::update();
    }
}
