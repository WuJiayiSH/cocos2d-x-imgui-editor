#include "NodeImDrawer.h"

namespace CCImEditor
{
    void NodeImDrawer::serialize(cocos2d::ValueMap& target)
    {
        _context = Context::SERIALIZE;
        _contextValue = &target;
        draw();
        _contextValue = nullptr;
        _context = Context::DRAW;
    }

    void NodeImDrawer::deserialize(const cocos2d::ValueMap& source)
    {
        _context = Context::DESERIALIZE;
        _contextValue = const_cast<cocos2d::ValueMap*>(&source); // save only, deserializer only take const-qualified source
        draw();
        _contextValue = nullptr;
        _context = Context::DRAW;
    }

    bool NodeImDrawer::init()
    {
        if (!cocos2d::Component::init())
            return false;

        setName("CCImEditor.NodeImDrawer");
        return true;
    }
}
