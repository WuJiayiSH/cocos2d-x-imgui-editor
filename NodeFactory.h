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
    
        template <typename NodePropertyGroupType, typename OwnerType>
        void registerNode(const char* name, const char* displayName, uint32_t mask = 0)
        {
            std::function<cocos2d::Node*()> constructor = [name, displayName]() -> cocos2d::Node* {
                auto owner = OwnerType::create();
                if (!owner)
                    return nullptr;

                NodePropertyGroupType* nodePropertyGroup = new (std::nothrow)NodePropertyGroupType();
                if (!nodePropertyGroup)
                    return nullptr;

                const char* shortName = displayName;
                while(const char* tmp = strstr(shortName, "/"))
                {
                    shortName = tmp + 1;
                }
                nodePropertyGroup->_shortName = shortName;
                nodePropertyGroup->_typeName = name;

                if (!nodePropertyGroup->init())
                {
                    delete nodePropertyGroup;
                    return nullptr;
                }
                nodePropertyGroup->_owner = owner;
                nodePropertyGroup->autorelease();

                NodeImDrawer* drawer = NodeImDrawer::create();
                drawer->_nodePropertyGroup = nodePropertyGroup;
                owner->addComponent(drawer);
                owner->setCameraMask(owner->getCameraMask() | (1 << 15));
                return owner;
            };
            NodeFactory::getInstance()->_nodeTypes.emplace(name, NodeType(name, displayName, mask, constructor));
        }

        typedef std::unordered_map<std::string, NodeType> NodeTypeMap;
        const NodeTypeMap& getNodeTypes() { return _nodeTypes; };

        cocos2d::Node* createNode(const std::string& name);
        
        static NodeFactory* getInstance();

    private:
        NodeTypeMap _nodeTypes;
    };
}

#endif
