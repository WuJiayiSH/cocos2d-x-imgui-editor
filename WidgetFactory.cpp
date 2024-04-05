#include "WidgetFactory.h"
#include <sstream>

namespace CCImEditor
{
    Widget* WidgetFactory::WidgetType::create() const
    {
        Widget* widget = _constructor();

        std::ostringstream oss;

        const std::string& displayName = getDisplayName();
        size_t lastSlash = displayName.find_last_of('/');
        oss << (lastSlash == std::string::npos ? displayName : displayName.substr(lastSlash + 1));
        oss << "###";
        oss << static_cast<void*>(widget); // use pointer address as unique name
        
        if (widget && widget->init(_name, oss.str(), _mask))
        {
            widget->autorelease();
            return widget;
        }

        delete widget;
        return nullptr;
    }

    WidgetFactory* WidgetFactory::getInstance()
    {
        static WidgetFactory instance;
        return &instance;
    }

    Widget* WidgetFactory::createWidget(const std::string& name)
    {
        std::unordered_map<std::string, WidgetType>::iterator it = _widgetTypes.find(name);
        if (it != _widgetTypes.end())
        {
			return it->second.create();
        }
        
        return nullptr;
    }
}
