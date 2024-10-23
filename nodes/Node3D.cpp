#include "Node3D.h"
#include "cocos2d.h"

using namespace cocos2d;

namespace CCImEditor
{
    void Node3D::draw()
    {
        if (!drawHeader("Node3D"))
            return;

        property("Name", &Node::getName, &Node::setName, getOwner());

        property("Position", &Node::getPosition3D, &Node::setPosition3D, getOwner());
        property("Rotation", &Node::getRotation3D, &Node::setRotation3D, getOwner());
        
        property("Scale", &Node::getScale3D, &Node::setScale3D, getOwner());

        property("Tag", &Node::getTag, &Node::setTag, getOwner());
        property("Visible", &Node::isVisible, &Node::setVisible, getOwner());
        
        property<Mask<CameraFlag>>("Camera Mask###CameraMask", &Node::getCameraMask,
        [] (Node* node, unsigned short cameraMask)
        {
            cameraMask |= (1 << 15);
            node->setCameraMask(cameraMask);
        }, getOwner());
    }
}
