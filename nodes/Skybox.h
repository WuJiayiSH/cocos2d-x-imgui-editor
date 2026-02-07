#ifndef __CCIMEDITOR_SKYBOX_H__
#define __CCIMEDITOR_SKYBOX_H__

#include "Node3D.h"

namespace CCImEditor
{
    class Skybox: public Node3D
    {
    public:
        void draw() override;
    };
}

#endif
