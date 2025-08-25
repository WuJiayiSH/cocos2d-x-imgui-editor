#include "NodeImDrawer.h"
#include "NodeFactory.h"

namespace CCImEditor
{
    void ImPropertyGroup::serialize(cocos2d::ValueMap& target)
    {
        Context ctx = _context;
        _context = Context::SERIALIZE;
        _contextValue = &target;
        draw();
        _contextValue = nullptr;
        _context = ctx;
    }

    void ImPropertyGroup::deserialize(const cocos2d::ValueMap& source)
    {
        Context ctx = _context;
        _context = Context::DESERIALIZE;
        _contextValue = const_cast<cocos2d::ValueMap*>(&source); // save only, deserializer only take const-qualified source
        draw();
        _contextValue = nullptr;
        _context = ctx;
    }

    void ImPropertyGroup::deserializeAnimations(const cocos2d::ValueMap& source)
    {
        for (const auto& [animationName, animationVal]: source)
        {
            if (animationVal.getType() != cocos2d::Value::Type::MAP)
            {
                CCLOGWARN("Unexpected value type: %d for animation: %s", animationVal.getType(), animationName.c_str());
                continue;
            }

            for (const auto& [propertyName, propertyVals]: animationVal.asValueMap())
            {
                if (propertyVals.getType() != cocos2d::Value::Type::VECTOR)
                {
                    CCLOGWARN("Unexpected value type: %d for properties: %s", propertyVals.getType(), propertyName.c_str());
                    continue;
                }

                for (const cocos2d::Value& propertyVal: propertyVals.asValueVector())
                {
                    if (propertyVal.getType() != cocos2d::Value::Type::MAP)
                    {
                        CCLOGWARN("Unexpected value type: %d in properties: %s", propertyVal.getType(), propertyName.c_str());
                        continue;
                    }

                    const cocos2d::ValueMap& propertyValMap = propertyVal.asValueMap();
                    cocos2d::ValueMap::const_iterator frameIt = propertyValMap.find("frame");
                    if (frameIt == propertyValMap.end())
                    {
                        CCLOGWARN("Missing or invalid frame for property %s", propertyName.c_str());
                        continue;
                    }

                    cocos2d::ValueMap::const_iterator valueIt = propertyValMap.find("value");
                    if (valueIt == propertyValMap.end())
                    {
                        CCLOGWARN("Missing value for property %s", propertyName.c_str());
                        continue;
                    }

                    _animations[animationName][propertyName][frameIt->second.asInt()] = valueIt->second;
                }
            }
        }
    }

    void ImPropertyGroup::play(const std::string& animation, int frame)
    {
//        CC_ASSERT(_animation == nullptr);
        _animation = &animation;
        _frame = frame;

        Context ctx = _context;
        _context = Context::PLAY;
        draw();
        _context = ctx;

        //_animation = nullptr;
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

    void NodeImDrawer::play(const std::string& animation)
    {
        _animation = animation;
        _elapsed = 0.0f;
    }

    void NodeImDrawer::update(float dt)
    {
        if (_animation.empty())
            return;

        _elapsed += dt;
        const int frame = static_cast<int>(_elapsed * _sample);

        _nodePropertyGroup->play(_animation, frame);
        for (const auto &[_, componentPropertyGroup] : _componentPropertyGroups)
        {
            if (componentPropertyGroup.get())
            {
                componentPropertyGroup->play(_animation, frame);
            }
        }
    }
}
