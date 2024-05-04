#include "CommandHistory.h"

namespace CCImEditor
{
    CommandHistory::CommandHistory()
    : _maxSize(100)
    {
        reset();
    }

    void CommandHistory::queue(Command* command, bool execute)
    {
        CC_ASSERT(command);

        if (execute)
        {
            cocos2d::RefPtr<Command> cmd = command;
            _pendingCallbacks.push_back(CC_CALLBACK_0(Command::execute, cmd));
        }

        _commands.erase(_iterator, _commands.end());
        _commands.push_back(command);
        if (_commands.size() > _maxSize)
        {
            _commands.pop_front();
        }

        _iterator = _commands.end();
    }

    void CommandHistory::update(float dt)
    {
        for (std::function<void()>& callback: _pendingCallbacks)
        {
            callback();
        }
        _pendingCallbacks.clear();
    }

    void CommandHistory::undo(int step)
    {
        CC_ASSERT(canUndo(step));
        while(step-- > 0)
        {
            _iterator --;
            _pendingCallbacks.push_back(CC_CALLBACK_0(Command::undo, *_iterator));
        }
    }

    void CommandHistory::redo(int step)
    {
        CC_ASSERT(canRedo(step));
        while(step-- > 0)
        {
            _pendingCallbacks.push_back(CC_CALLBACK_0(Command::execute, *_iterator));
            _iterator ++;
        }
    }

    bool CommandHistory::canUndo(int step) const
    {
        CommandList::iterator iterator = _iterator;
        while(step-- > 0)
        {
            if (iterator == _commands.begin())
                return false;

            iterator--;
        }

        return true;
    }
    
    bool CommandHistory::canRedo(int step) const
    {
        CommandList::iterator iterator = _iterator;
        while(step-- > 0)
        {
            if (iterator == _commands.end())
                return false;

            iterator ++;
        }

        return true;
    }

    void CommandHistory::reset()
    {
        _commands.clear();
        _pendingCallbacks.clear();
        _iterator = _commands.end();
    }
}
