#ifndef __CCIMEDITOR_NODETREE_H__
#define __CCIMEDITOR_NODETREE_H__

#include "Widget.h"

namespace CCImEditor
{
    class NodeTree: public Widget
    {
    private:
        void draw(bool* open) override;
    };
}

#endif
