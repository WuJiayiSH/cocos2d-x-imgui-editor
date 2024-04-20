#ifndef __CCIMEDITOR_SPRITE3D_H__
#define __CCIMEDITOR_SPRITE3D_H__

#include "Node3D.h"

namespace CCImEditor
{
    class Sprite3D: public Node3D
    {
    public:
        void draw() override;
    };
}

#endif
