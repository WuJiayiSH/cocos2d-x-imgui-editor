#ifndef __CCIMEDITOR_SPRITE_H__
#define __CCIMEDITOR_SPRITE_H__

#include "Node2D.h"

namespace CCImEditor
{
    class Sprite: public Node2D
    {
    public:
        void draw() override;
    };
}

#endif
