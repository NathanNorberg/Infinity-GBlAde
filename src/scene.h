#ifndef IGB_SCENE_H
#define IGB_SCENE_H

namespace igb
{

// Which screen runs. A scene's update() returns NONE to keep running, or the scene to switch
// to (DUEL from a duel starts a fresh one).
enum class scene_id
{
    NONE,
    TITLE,
    HUB,
    DUEL,
    CHARACTER,
    EQUIPMENT,
    SHOP
};

}

#endif
