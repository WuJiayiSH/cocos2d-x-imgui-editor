#include "Label.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void Label::draw()
    {
        Node2D::draw();
        
        if (!drawHeader(getShortName().c_str()))
            return;

        cocos2d::Label* owner = static_cast<cocos2d::Label*>(getOwner());
        property("String", &cocos2d::Label::getString, &cocos2d::Label::setString, owner);

        property("Dimensions", &cocos2d::Label::getDimensions,
        [] (cocos2d::Label* label, const cocos2d::Size& size)
        {
            label->setDimensions(size.width, size.height);
        },
        owner);

        property<Enum<TextHAlignment>>("Horizontal Alignment###HAlign", &cocos2d::Label::getHorizontalAlignment, &cocos2d::Label::setHorizontalAlignment, owner);
        property<Enum<TextVAlignment>>("Vertical Alignment###VAlign", &cocos2d::Label::getVerticalAlignment, &cocos2d::Label::setVerticalAlignment, owner);
        
        property<FilePath>("Font Name###FontName", 
            DefaultGetter<std::string>(), 
            &cocos2d::Label::setSystemFontName,
            owner);

        property("Font Size###FontSize", &cocos2d::Label::getSystemFontSize, &cocos2d::Label::setSystemFontSize, owner);
        property<Enum<LabelOverflow>>("Overflow###Overflow", &cocos2d::Label::getOverflow, &cocos2d::Label::setOverflow, owner);

    }
}
