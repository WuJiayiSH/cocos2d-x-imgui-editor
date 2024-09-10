#include "Geometry.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void Geometry::draw()
    {
        Node3D::draw();

        if (_context == Context::DRAW && !ImGui::CollapsingHeader(getShortName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            return;

        cocos2d::Sprite3D* owner = dynamic_cast<cocos2d::Sprite3D*>(getOwner());
        CC_ASSERT(owner);

        property<FilePath>("Texture", 
        [this] (cocos2d::Sprite3D*) -> std::string
        {
            auto it = _customValue.find("Texture");
            if (it != _customValue.end())
                return it->second.asString();

            return std::string();
        },
        [this] (cocos2d::Sprite3D* node, const std::string& filePath)
        {
            _customValue["Texture"] = filePath;
            node->setTexture(filePath);
        },
        owner);

        property<Mask<LightFlag>>("LightMask", &cocos2d::Sprite3D::getLightMask, &cocos2d::Sprite3D::setLightMask, owner);

        property("Cast Shadow###CastShadow", &cocos2d::Sprite3D::getCastShadow, &cocos2d::Sprite3D::setCastShadow, owner);
        property("Recieve Shadow###RecieveShadow", &cocos2d::Sprite3D::getRecieveShadow, &cocos2d::Sprite3D::setRecieveShadow, owner);
    }
}
