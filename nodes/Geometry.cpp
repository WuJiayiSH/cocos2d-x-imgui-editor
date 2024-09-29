#include "Geometry.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void Geometry::draw()
    {
        Node3D::draw();

        if (!drawHeader(getShortName().c_str()))
            return;

        cocos2d::Sprite3D* owner = dynamic_cast<cocos2d::Sprite3D*>(getOwner());
        CC_ASSERT(owner);

        property<FilePath>("Material", 
            DefaultGetter<std::string>(),
            [this] (cocos2d::Sprite3D* node, const std::string& filePath)
            {
                if (cocos2d::Material* material = cocos2d::Material::createWithFilename(filePath))
                {
                    node->setMaterial(material);
                }
            },
            owner);

        property<FilePath>("Texture",
            DefaultGetter<std::string>(),
            static_cast<void(cocos2d::Sprite3D::*)(const std::string&)>(&cocos2d::Sprite3D::setTexture),
            owner);

        property<Mask<LightFlag>>("LightMask", &cocos2d::Sprite3D::getLightMask, &cocos2d::Sprite3D::setLightMask, owner);

        property("Cast Shadow###CastShadow", &cocos2d::Sprite3D::getCastShadow, &cocos2d::Sprite3D::setCastShadow, owner);
        property("Recieve Shadow###RecieveShadow", &cocos2d::Sprite3D::getRecieveShadow, &cocos2d::Sprite3D::setRecieveShadow, owner);
    }
}
