#include "NodeFactory.h"
#include "CCIMGUI.h"

namespace CCImEditor
{
    NodeFactory* NodeFactory::getInstance()
    {
        static NodeFactory instance;
        return &instance;
    }

    cocos2d::Node* NodeFactory::createNode(const std::string& name)
    {
        std::unordered_map<std::string, NodeFactory::NodeType>::iterator it = _nodeTypes.find(name);
        if (it != _nodeTypes.end())
        {
            return it->second.create();
        }
        
        return nullptr;
    }
}
