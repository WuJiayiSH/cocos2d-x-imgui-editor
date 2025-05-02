#ifndef __CCIMEDITOR_COMPONENTLUA_H__
#define __CCIMEDITOR_COMPONENTLUA_H__

#include "Component.h"

namespace CCImEditor
{
    class ComponentLua: public Component
    {
    public:
        void draw() override;

    private:
        void reloadIfModified();
        time_t _loadTime;
    };
}

#endif
