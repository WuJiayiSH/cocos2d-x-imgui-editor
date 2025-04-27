#include "Component.h"
#include "cocos2d.h"
#include "commands/RemoveComponent.h"

using namespace cocos2d;

namespace CCImEditor
{
    void Component::draw()
    {
        bool visible = true;
        bool open = drawHeader(getShortName().c_str(), &visible);
        if (!visible)
        {
            if (RemoveComponent* command = RemoveComponent::create(this))
            {
                Editor::getInstance()->getCommandHistory().queue(command);
            }
        }
        
        if (!open)
            return;
    }
}
