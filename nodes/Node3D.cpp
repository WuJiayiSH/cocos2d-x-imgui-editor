#include "Node3D.h"
#include "cocos2d.h"

using namespace cocos2d;

namespace CCImEditor
{
    void Node3D::draw()
    {
        if (!drawHeader("Node3D"))
            return;

        cocos2d::Node* owner = static_cast<cocos2d::Node*>(getOwner());
        property("Name", &Node::getName, &Node::setName, owner);

        property("Position", &Node::getPosition3D, &Node::setPosition3D, owner);
        property("Rotation", &Node::getRotation3D, &Node::setRotation3D, owner);
        
        property("Scale", &Node::getScale3D, &Node::setScale3D, owner);

        property("Tag", &Node::getTag, &Node::setTag, owner);
        property("Visible", &Node::isVisible, &Node::setVisible, owner);
        
        property<Mask<CameraFlag>>("Camera Mask###CameraMask", &Node::getCameraMask,
        [] (Node* node, unsigned short cameraMask)
        {
            cameraMask |= (1 << 15);
            node->setCameraMask(cameraMask);
        }, owner);
    }
}
