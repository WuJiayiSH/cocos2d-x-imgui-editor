#ifndef __CCIMEDITOR_ANIMATION_H__
#define __CCIMEDITOR_ANIMATION_H__

#include "Widget.h"

namespace CCImEditor
{
    class Animation: public Widget
    {
    private:
        void draw(bool* open) override;

        void update(float) override;

        bool _playing = false;
        bool _loop = true;
        float _elapsed;
        int _currentFrame = 0;
    };
}

#endif
