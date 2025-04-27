#include "ComponentFactory.h"
#include "CCIMGUI.h"

namespace CCImEditor
{
    ComponentFactory* ComponentFactory::getInstance()
    {
        static ComponentFactory instance;
        return &instance;
    }

    ImPropertyGroup* ComponentFactory::createComponent(const std::string& name)
    {
        std::unordered_map<std::string, ComponentFactory::ComponentType>::iterator it = _componentTypes.find(name);
        if (it == _componentTypes.end())
            return false;

        return it->second.create();
    }
}
