#include "Sprite3D.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void Sprite3D::draw()
    {
        Node3D::draw();

        customProperty<FilePath>("Model", 
        [] (cocos2d::Node* node, const cocos2d::Value& filePath)
        {
            node->removeAllChildren();
            static_cast<cocos2d::Sprite3D*>(node)->initWithFile(filePath.asString());
        },
        getOwner());

        customProperty<FilePath>("Texture", 
        [] (cocos2d::Node* node, const cocos2d::Value& filePath)
        {
            static_cast<cocos2d::Sprite3D*>(node)->setTexture(filePath.asString());
        },
        getOwner());
    }
}
