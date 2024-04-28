#ifndef __CCIMEDITOR_POINTLIGHT_H__
#define __CCIMEDITOR_POINTLIGHT_H__

#include "BaseLight.h"

namespace CCImEditor
{
    class PointLight: public BaseLight
    {
    public:
        void draw() override;
    };
}

#endif
