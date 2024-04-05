#include "Node3D.h"
#include "NodeFactory.h"
#include "cocos2d.h"

using namespace cocos2d;

namespace CCImEditor
{
    void Node3D::draw()
    {
        property("Position", &Node::getPosition3D, &Node::setPosition3D, getOwner());
        property("Rotation", &Node::getRotation3D, &Node::setRotation3D, getOwner());
        
        property("Scale", 
        [] (Node* node) -> Vec3
        {
            return { node->getScaleX(), node->getScaleY(), node->getScaleZ() };
        },
        [] (Node* node, const Vec3& scale)
        {
            node->setScaleX(scale.x);
            node->setScaleY(scale.y);
            node->setScaleZ(scale.z);
        },
        getOwner(), 0.01f);

        property("Visible", &Node::isVisible, &Node::setVisible, getOwner());
        property("CastShadow", &Node::getCastShadow, &Node::setCastShadow, getOwner());
        property("RecieveShadow", &Node::getRecieveShadow, &Node::setRecieveShadow, getOwner());
    }
}
