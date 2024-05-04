#include "AddNode.h"

namespace CCImEditor
{
    void AddNode::undo()
    {
        _child->removeFromParent();
    }

    void AddNode::execute()
    {
        _parent->addChild(_child);
    }

    AddNode* AddNode::create(cocos2d::Node* parent, cocos2d::Node* child)
    {
        if (parent && child)
        {
            if (AddNode* command = new (std::nothrow)AddNode())
            {
                command->_parent = parent;
                command->_child = child;
                command->autorelease();
                return command;
            }
        }

        return nullptr;
    }
}
