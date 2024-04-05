#ifndef __CCIMEDITOR_NODEIMDRAWER_H__
#define __CCIMEDITOR_NODEIMDRAWER_H__

#include "cocos2d.h"
#include "invoke.hpp/invoke.hpp"
#include "PropertyImDrawer.h"

namespace CCImEditor
{
    class NodeImDrawerBase : public cocos2d::Component
    {
    public:
        virtual void draw() {};
        
    protected:
        template <class Getter, class Setter, class Object, class... Args>
        void property(const char *label, Getter &&getter, Setter &&setter, Object&& object, Args &&...args)
        {
            auto v = invoke_hpp::invoke(std::forward<Getter>(getter), std::forward<Object>(object));
            if (PropertyImDrawer<decltype(v)>::draw(label, v, std::forward<Args>(args)...))
            {
                invoke_hpp::invoke(std::forward<Setter>(setter), std::forward<Object>(object), v);
            }
        }
    };

    template <typename NodeType>
    class NodeImDrawer : public NodeImDrawerBase
    {
    public:
        bool init() override
        {
            if (!cocos2d::Component::init())
                return false;

            NodeType* node = NodeType::create();
            if (!node)
                return false;

            setName("CCImEditor.NodeImDrawer");
            node->addComponent(this);
            node->setCameraMask(node->getCameraMask() | (1 << 15));
            return true;
        }
    };
}

#endif
