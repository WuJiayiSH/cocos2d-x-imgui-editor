#ifndef __CCIMEDITOR_DIRECTIONLIGHT_H__
#define __CCIMEDITOR_DIRECTIONLIGHT_H__

#include "BaseLight.h"

namespace CCImEditor
{
    class DirectionLight: public BaseLight
    {
    public:
        void draw() override;
    };
}

#endif
