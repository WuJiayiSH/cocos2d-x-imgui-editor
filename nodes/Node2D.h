#ifndef __CCIMEDITOR_NODE2D_H__
#define __CCIMEDITOR_NODE2D_H__

#include "NodeImDrawer.h"

namespace CCImEditor
{
    class Node2D: public NodeImDrawer
    {
    public:
        void draw() override;
    };
}

#endif
