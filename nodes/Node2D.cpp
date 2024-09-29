#include "Node2D.h"
#include "cocos2d.h"

using namespace cocos2d;

namespace CCImEditor
{
    void Node2D::draw()
    {
        if (!drawHeader("Node2D"))
            return;

        property("Name", &Node::getName, &Node::setName, getOwner());

        property("Position", 
            static_cast<const cocos2d::Vec2&(cocos2d::Node::*)() const>(&Node::getPosition), 
            static_cast<void(cocos2d::Node::*)(const cocos2d::Vec2&)>(&Node::setPosition),
            getOwner());

        property("Content Size###ContentSize", &Node::getContentSize, &Node::setContentSize, getOwner());

        property("Anchor Point###AnchorPoint", &Node::getAnchorPoint, &Node::setAnchorPoint, getOwner(), 0.01f);

        property("Scale", 
        [] (Node* node) -> Vec2
        {
            return { node->getScaleX(), node->getScaleY() };
        },
        [] (Node* node, const Vec2& scale)
        {
            node->setScaleX(scale.x);
            node->setScaleY(scale.y);
        },
        getOwner(), 0.01f);

        property("Rotation", &Node::getRotation, &Node::setRotation, getOwner());

        property("Skew", 
        [] (Node* node) -> Vec2
        {
            return { node->getSkewX(), node->getSkewY() };
        },
        [] (Node* node, const Vec2& skew)
        {
            node->setSkewX(skew.x);
            node->setSkewY(skew.y);
        },
        getOwner(), 0.1f);

        property("Tag", &Node::getTag, &Node::setTag, getOwner());
        property("Local Z Order###LocalZOrder", &Node::getLocalZOrder, &Node::setLocalZOrder, getOwner());
        property("Visible", &Node::isVisible, &Node::setVisible, getOwner());
        
        property<Mask<CameraFlag>>("Camera Mask###CameraMask", &Node::getCameraMask,
        [] (Node* node, unsigned short cameraMask)
        {
            cameraMask |= (1 << 15);
            node->setCameraMask(cameraMask);
        }, getOwner());

        property("Color", 
        [] (Node* node) -> Color4B
        {
            const Color3B& color = node->getColor(); 
            return Color4B(color.r, color.g, color.b, node->getOpacity());
        },
        [] (Node* node, const Color4B& color)
        {
            node->setColor(Color3B(color.r, color.g, color.b));
            node->setOpacity(color.a);
        },
        getOwner());

        property("Cascade Color Enabled###CascadeColorEnabled", &Node::isCascadeColorEnabled, &Node::setCascadeColorEnabled, getOwner());
        property("Cascade Opacity Enabled###CascadeOpacityEnabled", &Node::isCascadeOpacityEnabled, &Node::setCascadeOpacityEnabled, getOwner());
        property("Opacity Modify RGB###OpacityModifyRGB", &Node::isOpacityModifyRGB, &Node::setOpacityModifyRGB, getOwner());
    }
}
