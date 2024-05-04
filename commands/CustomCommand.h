#ifndef __CCIMEDITOR_CUSTOMCOMMAND_H__
#define __CCIMEDITOR_CUSTOMCOMMAND_H__

#include "Command.h"

namespace CCImEditor
{
    class CustomCommand: public Command
    {
    public:
        void undo() override;
        void execute() override;
        static CustomCommand* create(std::function<void()> execute, std::function<void()> undo);

    private:
        std::function<void()> _execute;
        std::function<void()> _undo;
    };
}

#endif
