#include "Sprite.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void Sprite::draw()
    {
        Node2D::draw();

        if (!drawHeader("Sprite"))
            return;

        cocos2d::Sprite* owner = static_cast<cocos2d::Sprite*>(getOwner());
        property<FilePath>("Texture", 
            DefaultGetter<std::string>(),
            static_cast<void(cocos2d::Sprite::*)(const std::string&)>(&cocos2d::Sprite::setTexture),
            owner);

        property("Blend###BlendFunc", &cocos2d::Sprite::getBlendFunc, &cocos2d::Sprite::setBlendFunc, owner);
        property("Flipped X###FlippedX", &cocos2d::Sprite::isFlippedX, &cocos2d::Sprite::setFlippedX, owner);
    }
}
