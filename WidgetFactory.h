#ifndef __CCIMEDITOR_WIDGETFACTORY_H__
#define __CCIMEDITOR_WIDGETFACTORY_H__

#include <string>
#include <unordered_map>

#include "Widget.h"

namespace CCImEditor
{
    enum WidgetFlags
    {
        WidgetFlags_DisallowMultiple = 1
    };

    class WidgetFactory
    {
    public:
        struct WidgetType
        {
            friend class WidgetFactory;
            
            WidgetType(const std::string& name, const std::string& displayName, uint32_t mask, const std::function<Widget*()>& constructor)
            : _name(name)
            , _displayName(displayName)
            , _mask(mask)
            , _constructor(constructor)
            {
            }

            const std::string& getName() const { return _name; };
            const std::string& getDisplayName() const { return _displayName; };
            uint32_t getMask() const { return _mask; };
            bool allowMultiple() const { return (_mask & WidgetFlags_DisallowMultiple) == 0; };

            Widget* create() const;

        private:
            std::string _name;
            std::string _displayName;
            uint32_t _mask;
            std::function<Widget*()> _constructor;
        };
    
        template <typename T>
        void registerWidget(const char* name, const char* displayName, uint32_t mask = 0)
        {
            std::function<Widget*()> constructor = []() -> Widget* {return new (std::nothrow)T(); };
            WidgetFactory::getInstance()->_widgetTypes.emplace(name, WidgetType(name, displayName, mask, constructor));
        }

        const std::unordered_map<std::string, WidgetType>& getWidgetTypes() { return _widgetTypes; };

        Widget* createWidget(const std::string& name);

        static WidgetFactory* getInstance();

    private:
        std::unordered_map<std::string, WidgetType> _widgetTypes;
    };
}

#endif
