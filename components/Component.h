#ifndef __CCIMEDITOR_COMPONENT_H__
#define __CCIMEDITOR_COMPONENT_H__

#include "NodeImDrawer.h"

namespace CCImEditor
{
    class Component: public ImPropertyGroup
    {
    public:
        void draw() override;
    };
}

#endif
