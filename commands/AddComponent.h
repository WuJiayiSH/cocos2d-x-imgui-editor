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
        static AddComponent* create(cocos2d::Node* node, ImPropertyGroup* imPropertyGroup);

    private:
        cocos2d::RefPtr<cocos2d::Node> _node;
        cocos2d::RefPtr<ImPropertyGroup> _imPropertyGroup;

        // AddComponent holds a strong reference to the component
        // because imPropertyGroup only holds a weak reference to it.
        cocos2d::RefPtr<cocos2d::Component> _component;
    };
}

#endif
