#include "NodeImDrawer.h"

namespace CCImEditor
{
    void ImPropertyGroup::serialize(cocos2d::ValueMap& target)
    {
        _context = Context::SERIALIZE;
        _contextValue = &target;
        draw();
        _contextValue = nullptr;
        _context = Context::DRAW;
    }

    void ImPropertyGroup::deserialize(const cocos2d::ValueMap& source)
    {
        _context = Context::DESERIALIZE;
        _contextValue = const_cast<cocos2d::ValueMap*>(&source); // save only, deserializer only take const-qualified source
        draw();
        _contextValue = nullptr;
        _context = Context::DRAW;
    }

    bool ImPropertyGroup::init()
    {
        return true;
    }

    NodeImDrawer* NodeImDrawer::create() {
        NodeImDrawer* obj = new (std::nothrow)NodeImDrawer();
        if (obj && obj->init())
        {
            obj->autorelease();
            return obj;
        }

        delete obj;
        return nullptr;
    }

    bool NodeImDrawer::init()
    {
        if (!cocos2d::Component::init())
            return false;

        setName("CCImEditor.NodeImDrawer");
        return true;
    }
}
