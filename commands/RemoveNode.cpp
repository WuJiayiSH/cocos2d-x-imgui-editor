#include "RemoveNode.h"

namespace CCImEditor
{
    void RemoveNode::undo()
    {
        _parent->addChild(_child);
    }

    void RemoveNode::execute()
    {
        _child->removeFromParent();
    }

    RemoveNode* RemoveNode::create(cocos2d::Node* node)
    {
        if (node && node->getParent())
        {
            if (RemoveNode* command = new (std::nothrow)RemoveNode())
            {
                command->_parent = node->getParent();
                command->_child = node;
                command->autorelease();
                return command;
            }
        }

        return nullptr;
    }
}
