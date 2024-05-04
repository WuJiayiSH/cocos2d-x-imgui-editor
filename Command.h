#ifndef __CCIMEDITOR_COMMAND_H__
#define __CCIMEDITOR_COMMAND_H__

#include "cocos2d.h"

namespace CCImEditor
{
    class Command: public cocos2d::Ref
    {
    public:
        virtual void undo() = 0;
        virtual void execute() = 0;
    };
}

#endif
