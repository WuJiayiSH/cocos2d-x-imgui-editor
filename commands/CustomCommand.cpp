#include "CustomCommand.h"

namespace CCImEditor
{
    void CustomCommand::undo()
    {
        _undo();
    }

    void CustomCommand::execute()
    {
        _execute();
    }

    CustomCommand* CustomCommand::create(std::function<void()> execute, std::function<void()> undo)
    {
        if (execute && undo)
        {
            if (CustomCommand* command = new (std::nothrow)CustomCommand())
            {
                command->_execute = execute;
                command->_undo = undo;
                command->autorelease();
                return command;
            }
        }

        return nullptr;
    }
}
