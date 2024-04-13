#include "NodeImDrawer.h"

namespace CCImEditor
{
    void NodeImDrawerBase::serialize(cocos2d::ValueMap& target)
    {
        _context = Context::SERIALIZE;
        _contextValue = &target;
        draw();
        _contextValue = nullptr;
        _context = Context::DRAW;
    }

    void NodeImDrawerBase::deserialize(const cocos2d::ValueMap& source)
    {
        _context = Context::DESERIALIZE;
        _contextValue = const_cast<cocos2d::ValueMap*>(&source); // save only, deserializer only take const-qualified source
        draw();
        _contextValue = nullptr;
        _context = Context::DRAW;
    }
}
