#include "Sprite.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void Sprite::draw()
    {
        Node2D::draw();

        if (_context == Context::DRAW && !ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        cocos2d::Sprite* owner = static_cast<cocos2d::Sprite*>(getOwner());
        customProperty<FilePath>("Texture", 
        [] (cocos2d::Sprite* sprite, const cocos2d::Value& filePath)
        {
            sprite->setTexture(filePath.asString());
        },
        owner);

        property<Enum<BlendSrcDst>>("Blend Source###BlendSrc",  [] (cocos2d::Sprite* sprite) -> GLenum
        {
            return sprite->getBlendFunc().src;
        },
        [] (cocos2d::Sprite* sprite, const GLenum& src)
        {
            cocos2d::BlendFunc blendFunc = sprite->getBlendFunc();
            blendFunc.src = src;
            sprite->setBlendFunc(blendFunc);
        }, owner);

        property<Enum<BlendSrcDst>>("Blend Destination###BlendDst",  [] (cocos2d::Sprite* sprite) -> GLenum
        {
            return sprite->getBlendFunc().dst;
        },
        [] (cocos2d::Sprite* sprite, const GLenum& dst)
        {
            cocos2d::BlendFunc blendFunc = sprite->getBlendFunc();
            blendFunc.dst = dst;
            sprite->setBlendFunc(blendFunc);
        }, owner);

        property("Flipped X###FlippedX", &cocos2d::Sprite::isFlippedX, &cocos2d::Sprite::setFlippedX, owner);
    }
}
