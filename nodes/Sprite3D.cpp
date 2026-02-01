#include "Sprite3D.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void Sprite3D::draw()
    {
        Node3D::draw();

        if (!drawHeader("Sprite3D"))
            return;

        cocos2d::Sprite3D* owner = static_cast<cocos2d::Sprite3D*>(getOwner());

        property<FilePath>("Model", 
            DefaultGetter<std::string>(),
            [this] (cocos2d::Sprite3D* node, const std::string& filePath)
            {
                cocos2d::Vec3 position = node->getPosition3D();
                cocos2d::Quaternion rotation = node->getRotationQuat();
                cocos2d::Vec3 scale = {node->getScaleX(), node->getScaleY(), node->getScaleZ()};

                node->removeAllChildren();
                node->initWithFile(filePath);

                // initWithFile may have changed position, rotation and scale. Set them back.
                node->setPosition3D(position);
                node->setRotationQuat(rotation);
                node->setScale3D(scale);
            },
            owner);

        for (ssize_t i = 0; i < owner->getMeshCount(); ++i)
        {
            std::string matName = cocos2d::StringUtils::format("Material (%zd)###Material.%zd", i, i);
            property<FilePath>(matName.c_str(), 
            DefaultGetter<std::string>(),
            [this, i] (cocos2d::Sprite3D* node, const std::string& filePath)
            {
                if (cocos2d::Material* material = cocos2d::Material::createWithFilename(filePath))
                {
                    node->setMaterial(material, i);
                }
            },
            owner);

            std::string texName = cocos2d::StringUtils::format("Texture (%zd)###Texture.%zd", i, i);
            property<FilePath>(texName.c_str(), 
            DefaultGetter<std::string>(),
            [this, i] (cocos2d::Sprite3D* node, const std::string& filePath)
            {
                node->getMeshByIndex(i)->setTexture(filePath);
            },
            owner);
        }

        property<MaskOf<cocos2d::LightFlag>>("LightMask", &cocos2d::Sprite3D::getLightMask, &cocos2d::Sprite3D::setLightMask, owner);

        property("Cast Shadow###CastShadow", &cocos2d::Sprite3D::getCastShadow, &cocos2d::Sprite3D::setCastShadow, owner);
        property("Recieve Shadow###RecieveShadow", &cocos2d::Sprite3D::getRecieveShadow, &cocos2d::Sprite3D::setRecieveShadow, owner);
    }
}
