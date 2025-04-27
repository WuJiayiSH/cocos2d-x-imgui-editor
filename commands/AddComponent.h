#ifndef __CCIMEDITOR_ADDCOMPONENT_H__
#define __CCIMEDITOR_ADDCOMPONENT_H__

#include "Command.h"
#include "NodeImDrawer.h"

namespace CCImEditor
{
    class AddComponent: public Command
    {
    public:
        void undo() override;
        void execute() override;
        static AddComponent* create(cocos2d::Node* node, ImPropertyGroup* component);

    private:
        cocos2d::RefPtr<cocos2d::Node> _node;
        cocos2d::RefPtr<ImPropertyGroup> _component;
    };
}

#endif
