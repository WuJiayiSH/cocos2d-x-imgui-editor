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
                node->setScaleX(scale.x);
                node->setScaleY(scale.y);
                node->setScaleZ(scale.z);
            },
            owner);

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
