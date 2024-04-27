#ifndef __CCIMEDITOR_BASELIGHT_H__
#define __CCIMEDITOR_BASELIGHT_H__

#include "Node3D.h"

namespace CCImEditor
{
    class BaseLight: public Node3D
    {
    public:
        void draw() override;
    };
}

#endif
