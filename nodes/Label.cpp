#include "Label.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void Label::draw()
    {
        Node2D::draw();
        
        if (_context == Context::DRAW && !ImGui::CollapsingHeader(getShortName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
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
        
        customProperty<FilePath>("Font Name###FontName", 
        [] (cocos2d::Label* label, const cocos2d::Value& filePath)
        {
            label->setSystemFontName(filePath.asString());
        },
        owner);
        property("Font Size###FontSize", &cocos2d::Label::getSystemFontSize, &cocos2d::Label::setSystemFontSize, owner);
        property<Enum<LabelOverflow>>("Overflow###Overflow", &cocos2d::Label::getOverflow, &cocos2d::Label::setOverflow, owner);

        property<Enum<BlendSrcDst>>("Blend Source###BlendSrc",  [] (cocos2d::Label* label) -> GLenum
        {
            return label->getBlendFunc().src;
        },
        [] (cocos2d::Label* label, const GLenum& src)
        {
            cocos2d::BlendFunc blendFunc = label->getBlendFunc();
            blendFunc.src = src;
            label->setBlendFunc(blendFunc);
        }, owner);

        property<Enum<BlendSrcDst>>("Blend Destination###BlendDst",  [] (cocos2d::Label* label) -> GLenum
        {
            return label->getBlendFunc().dst;
        },
        [] (cocos2d::Label* label, const GLenum& dst)
        {
            cocos2d::BlendFunc blendFunc = label->getBlendFunc();
            blendFunc.dst = dst;
            label->setBlendFunc(blendFunc);
        }, owner);
    }
}
