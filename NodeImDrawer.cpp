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
}
