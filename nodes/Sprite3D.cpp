#include "Sprite3D.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void Sprite3D::draw()
    {
        Node3D::draw();

        if (_context == Context::DRAW && !ImGui::CollapsingHeader("Sprite3D", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        cocos2d::Sprite3D* owner = static_cast<cocos2d::Sprite3D*>(getOwner());
        customProperty<FilePath>("Model", 
        [] (cocos2d::Sprite3D* node, const cocos2d::Value& filePath)
        {
            node->removeAllChildren();
            node->initWithFile(filePath.asString());
        },
        owner);

        customProperty<FilePath>("Texture", 
        [] (cocos2d::Sprite3D* node, const cocos2d::Value& filePath)
        {
            node->setTexture(filePath.asString());
        },
        owner);

        property<Mask<LightFlag>>("LightMask", &cocos2d::Sprite3D::getLightMask, &cocos2d::Sprite3D::setLightMask, owner);

        property("Cast Shadow###CastShadow", &cocos2d::Sprite3D::getCastShadow, &cocos2d::Sprite3D::setCastShadow, owner);
        property("Recieve Shadow###RecieveShadow", &cocos2d::Sprite3D::getRecieveShadow, &cocos2d::Sprite3D::setRecieveShadow, owner);
    }
}
