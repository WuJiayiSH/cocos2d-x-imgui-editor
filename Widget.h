#ifndef __CCIMEDITOR_WIDGET_H__
#define __CCIMEDITOR_WIDGET_H__

#include "cocos2d.h"

namespace CCImEditor
{
    class Widget: public cocos2d::Ref
    {
    public:
        friend class WidgetFactory;
		friend class Editor;

        const std::string& getName() const {return _name;}
        const std::string& getWindowName() const {return _windowName;}
        
        uint32_t getMask() const { return _mask; };
        
    protected:
        Widget() {};
        virtual ~Widget() {};

        virtual bool init(const std::string& name, const std::string& windowName, uint32_t mask);

    private:
        virtual void draw(bool* open) = 0;
        virtual void update(float) {};

        std::string _name;
        std::string _windowName;
        uint32_t _mask;
    };
}

#endif
