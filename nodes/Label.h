#ifndef __CCIMEDITOR_LABEL_H__
#define __CCIMEDITOR_LABEL_H__

#include "Node2D.h"

namespace CCImEditor
{
    class Label: public Node2D
    {
    public:
        void draw() override;
    };
}

#endif
