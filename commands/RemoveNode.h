#ifndef __CCIMEDITOR_REMOVENODE_H__
#define __CCIMEDITOR_REMOVENODE_H__

#include "Command.h"

namespace CCImEditor
{
    class RemoveNode: public Command
    {
    public:
        void undo() override;
        void execute() override;
        static RemoveNode* create(cocos2d::Node* node);

    private:
        cocos2d::RefPtr<cocos2d::Node> _parent;
        cocos2d::RefPtr<cocos2d::Node> _child;
    };
}

#endif
