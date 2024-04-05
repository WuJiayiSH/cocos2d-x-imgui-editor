#ifndef __CCIMEDITOR_NODEFACTORY_H__
#define __CCIMEDITOR_NODEFACTORY_H__

#include "cocos2d.h"

#include <string>
#include <unordered_map>

namespace CCImEditor
{
    enum NodeFlags
    {
        NodeFlags_CanHaveChildren = 1,
        NodeFlags_CanBeRoot = 1 << 1
    };

    class NodeFactory
    {
    public:
        struct NodeType
        {
            friend class NodeFactory;
            
            NodeType(const std::string& name, const std::string& displayName, uint32_t mask, const std::function<cocos2d::Node*()>& constructor)
            : _name(name)
            , _displayName(displayName)
            , _mask(mask)
            , _constructor(constructor)
            {
            }

            const std::string& getName() const { return _name; };
            const std::string& getDisplayName() const { return _displayName; };
            uint32_t getMask() const { return _mask; };

            cocos2d::Node* create() const { return _constructor(); };

        private:
            std::string _name;
            std::string _displayName;
            uint32_t _mask;
            std::function<cocos2d::Node*()> _constructor;
        };
    
        template <typename T>
        void registerNode(const char* name, const char* displayName, uint32_t mask = 0)
        {
            std::function<cocos2d::Node*()> constructor = []() -> cocos2d::Node* {
                T* visitor = new (std::nothrow)T();
                if (!visitor)
                    return nullptr;

                if (!visitor->init())
                {
                    delete visitor;
                    return nullptr;
                }

                return visitor->getOwner();
            };
            NodeFactory::getInstance()->_nodeTypes.emplace(name, NodeType(name, displayName, mask, constructor));
        }

        const std::unordered_map<std::string, NodeType>& getNodeTypes() { return _nodeTypes; };

        cocos2d::Node* createNode(const std::string& name);
        
        static NodeFactory* getInstance();

    private:
        std::unordered_map<std::string, NodeType> _nodeTypes;
    };
}

#endif
