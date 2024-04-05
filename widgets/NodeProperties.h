#ifndef __CCIMEDITOR_NODEPROPERTIES_H__
#define __CCIMEDITOR_NODEPROPERTIES_H__

#include <string>
#include "Widget.h"

namespace CCImEditor
{
    class NodeProperties: public Widget
    {
    private:
        void draw(bool* open) override;
    };
}

#endif
