#ifndef __CCIMEDITOR_GEOMETRY_H__
#define __CCIMEDITOR_GEOMETRY_H__

#include "Node3D.h"

namespace CCImEditor
{
    class Geometry: public Node3D
    {
    public:
        void draw() override;
    };
}

#endif
