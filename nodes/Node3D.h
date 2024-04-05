#ifndef __CCIMEDITOR_NODE3D_H__
#define __CCIMEDITOR_NODE3D_H__

#include "NodeImDrawer.h"

namespace CCImEditor
{
    class Node3D: public NodeImDrawer<cocos2d::Node>
    {
    public:
        void draw() override;
    };
}

#endif
