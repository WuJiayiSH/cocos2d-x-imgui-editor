#ifndef __CCIMEDITOR_IMGUIDEMO_H__
#define __CCIMEDITOR_IMGUIDEMO_H__

#include "Widget.h"

namespace CCImEditor
{
    class ImGuiDemo: public Widget
    {
    private:
        void draw(bool* open) override;
    };
}

#endif
