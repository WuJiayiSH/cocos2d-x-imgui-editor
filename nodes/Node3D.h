#ifndef __CCIMEDITOR_NODE3D_H__
#define __CCIMEDITOR_NODE3D_H__

#include "NodeImDrawer.h"

namespace CCImEditor
{
    class Node3D: public ImPropertyGroup
    {
    public:
        void draw() override;
    };
}

#endif
