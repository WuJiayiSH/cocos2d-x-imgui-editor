#ifndef __CCIMEDITOR_REMOVECOMPONENT_H__
#define __CCIMEDITOR_REMOVECOMPONENT_H__

#include "Command.h"
#include "NodeImDrawer.h"

namespace CCImEditor
{
    class RemoveComponent: public Command
    {
    public:
        void undo() override;
        void execute() override;
        static RemoveComponent* create(ImPropertyGroup* component);

    private:
        cocos2d::RefPtr<cocos2d::Node> _node;
        cocos2d::RefPtr<ImPropertyGroup> _component;
    };
}

#endif
