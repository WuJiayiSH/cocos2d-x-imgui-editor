#include "AddNode.h"

namespace CCImEditor
{
    void AddNode::undo()
    {
        _parent->removeChild(_child);
        if (_parentBefore)
        {
            _parentBefore->addChild(_child);
        }
    }

    void AddNode::execute()
    {
        if (_parentBefore)
        {
            _parentBefore->removeChild(_child);
        }
        _parent->addChild(_child);
    }

    AddNode* AddNode::create(cocos2d::Node* parent, cocos2d::Node* child)
    {
        if (parent && child)
        {
            if (AddNode* command = new (std::nothrow)AddNode())
            {
                command->_parentBefore = child->getParent();
                command->_parent = parent;
                command->_child = child;
                command->autorelease();
                return command;
            }
        }

        return nullptr;
    }
}
