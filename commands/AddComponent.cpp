#include "AddComponent.h"

namespace CCImEditor
{
    void AddComponent::undo()
    {
        _node->removeComponent(_component);

        if (NodeImDrawer* drawer = _node->getComponent<NodeImDrawer>())
        {
            drawer->setComponentPropertyGroup(_component->getName(), nullptr);
        }
    }

    void AddComponent::execute()
    {
        _node->addComponent(_component);

        if (NodeImDrawer* drawer = _node->getComponent<NodeImDrawer>())
        {
            drawer->setComponentPropertyGroup(_component->getName(), _imPropertyGroup);
        }
    }

    AddComponent* AddComponent::create(cocos2d::Node* node, ImPropertyGroup* imPropertyGroup)
    {
        if (node && imPropertyGroup)
        {
            if (AddComponent* command = new (std::nothrow)AddComponent())
            {
                command->_node = node;
                command->_imPropertyGroup = imPropertyGroup;
                command->_component = static_cast<cocos2d::Component*>(imPropertyGroup->getOwner());
                command->autorelease();
                return command;
            }
        }

        return nullptr;
    }
}
