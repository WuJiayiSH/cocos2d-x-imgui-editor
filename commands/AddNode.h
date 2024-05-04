#ifndef __CCIMEDITOR_ADDNODE_H__
#define __CCIMEDITOR_ADDNODE_H__

#include "Command.h"

namespace CCImEditor
{
    class AddNode: public Command
    {
    public:
        void undo() override;
        void execute() override;
        static AddNode* create(cocos2d::Node* parent, cocos2d::Node* child);

    private:
        cocos2d::RefPtr<cocos2d::Node> _parent;
        cocos2d::RefPtr<cocos2d::Node> _child;
    };
}

#endif
