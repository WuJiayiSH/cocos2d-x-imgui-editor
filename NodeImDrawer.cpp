#include "NodeImDrawer.h"
#include "NodeFactory.h"

namespace CCImEditor
{
    void ImPropertyGroup::serialize(cocos2d::ValueMap& target)
    {
        _context = Context::SERIALIZE;
        _contextValue = &target;
        draw();
        _contextValue = nullptr;
        _context = Context::DRAW;
    }

    void ImPropertyGroup::deserialize(const cocos2d::ValueMap& source)
    {
        _context = Context::DESERIALIZE;
        _contextValue = const_cast<cocos2d::ValueMap*>(&source); // save only, deserializer only take const-qualified source
        draw();
        _contextValue = nullptr;
        _context = Context::DRAW;
    }

    bool ImPropertyGroup::init()
    {
        return true;
    }

    NodeImDrawer* NodeImDrawer::create() {
        NodeImDrawer* obj = new (std::nothrow)NodeImDrawer();
        if (obj && obj->init())
        {
            obj->autorelease();
            return obj;
        }

        delete obj;
        return nullptr;
    }

    bool NodeImDrawer::init()
    {
        if (!cocos2d::Component::init())
            return false;

        setName("CCImEditor.NodeImDrawer");
        return true;
    }

    void NodeImDrawer::draw()
    {
        _nodePropertyGroup->draw();

        for (const auto &[name, componentPropertyGroup] : _componentPropertyGroups)
        {
            if (componentPropertyGroup.get())
                componentPropertyGroup->draw();
        }
    }

    void NodeImDrawer::setComponentPropertyGroup(std::string name, ImPropertyGroup* group)
    {
        if (group)
        {
            _componentPropertyGroups[name] = group;
        }
        else
        {
            _componentPropertyGroups.erase(name);
        }
    }

    bool NodeImDrawer::canHaveChildren() const
    {
        return (getMask() & NodeFlags_CanHaveChildren) > 0 && _filename.empty();
    }

    bool NodeImDrawer::canHaveComponents() const
    {
        return (getMask() & NodeFlags_CanHaveComponents) > 0;
    }

    bool NodeImDrawer::canBeRoot() const
    {
        return (getMask() & NodeFlags_CanBeRoot) > 0;
    }

    uint32_t NodeImDrawer::getMask() const
    {
        const NodeFactory::NodeTypeMap& nodeTypes = NodeFactory::getInstance()->getNodeTypes();
        NodeFactory::NodeTypeMap::const_iterator it = nodeTypes.find(getTypeName());
        CC_ASSERT(it != nodeTypes.end());
        return it->second.getMask();
    }
}
