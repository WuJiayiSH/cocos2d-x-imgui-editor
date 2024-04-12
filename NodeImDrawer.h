#ifndef __CCIMEDITOR_NODEIMDRAWER_H__
#define __CCIMEDITOR_NODEIMDRAWER_H__

#include "cocos2d.h"
#include "invoke.hpp/invoke.hpp"
#include "PropertyImDrawer.h"
#include "PropertyImSerializer.h"

namespace CCImEditor
{
    class NodeImDrawerBase : public cocos2d::Component
    {
    public:
        enum class Context
        {
            DRAW,
            SERIALIZE
        };

        friend class NodeFactory;
        virtual void draw() {};
        void serialize(cocos2d::ValueMap&);
        const std::string& getTypeName() const {return _typeName;}
        
    protected:
        template <class Getter, class Setter, class Object, class... Args>
        void property(const char *label, Getter &&getter, Setter &&setter, Object&& object, Args &&...args)
        {
            auto v = invoke_hpp::invoke(std::forward<Getter>(getter), std::forward<Object>(object));

            if (_context == Context::DRAW)
            {
                if (PropertyImDrawer<decltype(v)>::draw(label, v, std::forward<Args>(args)...))
                {
                    invoke_hpp::invoke(std::forward<Setter>(setter), std::forward<Object>(object), v);
                }
            }
            else if (_context == Context::SERIALIZE)
            {
                PropertyImSerializer<decltype(v)>::serialize(*_contextValue, label, v);
            }
        }

    private:
        std::string _typeName;
        Context _context = Context::DRAW;
        cocos2d::ValueMap* _contextValue = nullptr;
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
