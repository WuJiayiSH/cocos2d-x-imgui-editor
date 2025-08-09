#include "RemoveComponent.h"

namespace CCImEditor
{
    void RemoveComponent::undo()
    {
        cocos2d::Component* owner = static_cast<cocos2d::Component*>(_component->getOwner());
        _node->addComponent(owner);

        NodeImDrawer* drawer = _node->getComponent<NodeImDrawer>();
        drawer->setComponentPropertyGroup(owner->getName(), _component);
    }

    void RemoveComponent::execute()
    {
        cocos2d::Component* owner = static_cast<cocos2d::Component*>(_component->getOwner());
        _node->removeComponent(owner);

        NodeImDrawer* drawer = _node->getComponent<NodeImDrawer>();
        drawer->setComponentPropertyGroup(owner->getName(), nullptr);
    }

    RemoveComponent* RemoveComponent::create(ImPropertyGroup* component)
    {
        if (!component)
            return nullptr;

        cocos2d::Component* owner = static_cast<cocos2d::Component*>(component->getOwner());
        if (!owner)
            return nullptr;

        cocos2d::Node* node = owner->getOwner();
        if (!node)
            return nullptr;

        RemoveComponent* command = new (std::nothrow)RemoveComponent();
        if (!command)
            return nullptr;

        command->_node = node;
        command->_component = component;
        command->autorelease();
        return command;
    }
}
