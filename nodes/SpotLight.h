#ifndef __CCIMEDITOR_SPOTLIGHT_H__
#define __CCIMEDITOR_SPOTLIGHT_H__

#include "BaseLight.h"

namespace CCImEditor
{
    class SpotLight: public BaseLight
    {
    public:
        void draw() override;
    };
}

#endif
