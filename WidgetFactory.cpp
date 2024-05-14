#include "WidgetFactory.h"
#include <sstream>

namespace CCImEditor
{
    namespace
    {
        std::unordered_map<std::string, uint64_t> s_nameIdMap;
    }

    Widget* WidgetFactory::WidgetType::create() const
    {
        Widget* widget = _constructor();

        std::ostringstream oss;

        const std::string& displayName = getDisplayName();
        size_t lastSlash = displayName.find_last_of('/');
        oss << (lastSlash == std::string::npos ? displayName : displayName.substr(lastSlash + 1));
        oss << "###";
        oss << _name;
        oss << ".";
        uint64_t& id = s_nameIdMap[_name];
        oss << id++;
        
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
