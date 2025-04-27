#include "AddComponent.h"

namespace CCImEditor
{
    void AddComponent::undo()
    {
        cocos2d::Component* owner = static_cast<cocos2d::Component*>(_component->getOwner());
        _node->removeComponent(owner);

        if (NodeImDrawer* drawer = static_cast<NodeImDrawer*>(_node->getComponent("CCImEditor.NodeImDrawer")))
        {
            drawer->setComponentPropertyGroup(owner->getName(), nullptr);
        }
    }

    void AddComponent::execute()
    {
        cocos2d::Component* owner = static_cast<cocos2d::Component*>(_component->getOwner());
        _node->addComponent(owner);

        if (NodeImDrawer* drawer = static_cast<NodeImDrawer*>(_node->getComponent("CCImEditor.NodeImDrawer")))
        {
            drawer->setComponentPropertyGroup(owner->getName(), _component);
        }
    }

    AddComponent* AddComponent::create(cocos2d::Node* node, ImPropertyGroup* component)
    {
        if (node && component)
        {
            if (AddComponent* command = new (std::nothrow)AddComponent())
            {
                command->_node = node;
                command->_component = component;
                command->autorelease();
                return command;
            }
        }

        return nullptr;
    }
}
