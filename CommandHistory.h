#ifndef __CCIMEDITOR_COMMANDHISTORY_H__
#define __CCIMEDITOR_COMMANDHISTORY_H__

#include <vector>
#include <list>
#include "cocos2d.h"
#include "Command.h"

namespace CCImEditor
{
    class CommandHistory
    {
    public:
        CommandHistory();

        void update(float dt);

        void queue(Command* command, bool execute = true);

        bool canUndo(int step = 1) const;
        bool canRedo(int step = 1) const;
        void undo(int step = 1);
        void redo(int step = 1);

        void reset();

    private:
        std::vector<std::function<void()>> _pendingCallbacks;

        typedef std::list<cocos2d::RefPtr<Command>> CommandList;
        CommandList _commands;
        CommandList::iterator _iterator;
        const size_t _maxSize;
    };
}

#endif
