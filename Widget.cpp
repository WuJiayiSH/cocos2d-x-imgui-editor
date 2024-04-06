#include "Widget.h"

namespace CCImEditor
{
    bool Widget::init(const std::string& typeName, const std::string& windowName, uint32_t mask)
    {
        _typeName = typeName;
        _windowName = windowName;
        _mask = mask;
        return true;
    }
}
