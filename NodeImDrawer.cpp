#include "NodeImDrawer.h"
#include "NodeFactory.h"

namespace CCImEditor
{
    namespace Internal
    {
        void performRecursively(cocos2d::Node *node, std::function<void(cocos2d::Node*)> func)
        {
            func(node);

            for (auto child : node->getChildren())
            {
                performRecursively(child, func);
            }
        }
    }

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

    void ImPropertyGroup::serializeAnimations(cocos2d::ValueMap& target)
    {
        // For each animation, serialize its properties into the target ValueMap
        for (const auto& [animationName, animationData] : _animations)
        {
            cocos2d::ValueMap animationVals;

            for (const auto& [propertyName, propertyValues] : animationData._values)
            {
                cocos2d::ValueVector frames;

                for (const auto& [frameIndex, val] : propertyValues)
                {
                    cocos2d::ValueMap frame;
                    frame["frame"] = cocos2d::Value(frameIndex);
                    frame["value"] = val;
                    frames.push_back(cocos2d::Value(std::move(frame)));
                }

                animationVals[propertyName] = cocos2d::Value(std::move(frames));
            }

            cocos2d::ValueMap animationRoot;
            animationRoot["maxFrame"] = animationData._maxFrame;
            animationRoot["samples"] = animationData._samples;
            animationRoot["values"] = cocos2d::Value(std::move(animationVals));

            target[animationName] = cocos2d::Value(std::move(animationRoot));
        }
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

            const cocos2d::ValueMap& animationRoot = animationVal.asValueMap();

            cocos2d::ValueMap::const_iterator maxFrameIt = animationRoot.find("maxFrame");
            if (maxFrameIt != animationRoot.end())
            {
                _animations[animationName]._maxFrame = maxFrameIt->second.asInt();
            }
            else
            {
                CCLOGWARN("Missing maxFrame for animation %s, will use default value", animationName.c_str());
            }

            cocos2d::ValueMap::const_iterator samplesIt = animationRoot.find("samples");
            if (samplesIt != animationRoot.end())
            {
                _animations[animationName]._samples = samplesIt->second.asUnsignedInt();
            }
            else
            {
                CCLOGWARN("Missing samples for animation %s, will use default value", animationName.c_str());
            }

            cocos2d::ValueMap::const_iterator valuesIt = animationRoot.find("values");
            if (valuesIt != animationRoot.end() && valuesIt->second.getType() == cocos2d::Value::Type::MAP)
            {
                for (const auto& [propertyName, propertyVals]: valuesIt->second.asValueMap())
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

                        _animations[animationName]._values[propertyName][frameIt->second.asInt()] = valueIt->second;
                    }
                }
            }
        }
    }

    void ImPropertyGroup::sample()
    {
        Context ctx = _context;
        _context = Context::SAMPLE;
        draw();
        _context = ctx;
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

    void NodeImDrawer::play(const std::string& animation, AnimationWrapMode wrapMode)
    {
        const ImPropertyGroup::AnimationData& animationData = _nodePropertyGroup->_animations[animation];
        applyAnimationRecursively(true, animation, 0, animationData._maxFrame, wrapMode, animationData._samples);
    }

    void NodeImDrawer::stop()
    {
        const ImPropertyGroup::AnimationData& animationData = _nodePropertyGroup->_animations[_animationName];
        applyAnimationRecursively(false, _animationName, 0, animationData._maxFrame, _animationWrapMode, animationData._samples);
    }

    void NodeImDrawer::applyAnimationRecursively(bool isPlaying, const std::string& animation, int frame, int maxFrame, CCImEditor::AnimationWrapMode wrapMode, uint16_t sample)
    {
        Internal::performRecursively(getOwner(), [&](cocos2d::Node* node){
            if (NodeImDrawer* drawer = node->getComponent<NodeImDrawer>())
            {
                drawer->_isPlayingAnimation = isPlaying;
                drawer->_animationName = animation;
                drawer->_currentFrame = frame;
                drawer->_animationWrapMode = wrapMode;

                auto it = drawer->_nodePropertyGroup->_animations.find(_animationName);
                if (it != drawer->_nodePropertyGroup->_animations.end())
                {
                    it->second._samples = sample;
                    it->second._maxFrame = maxFrame;
                }
            }
        });
    }

    std::vector<Internal::Animation::SequenceItem> NodeImDrawer::getAnimationSequenceItems(const std::string& animation) const
    {
        std::vector<Internal::Animation::SequenceItem> items;

        Internal::performRecursively(getOwner(), [&](cocos2d::Node* node){
            if (NodeImDrawer* drawer = node->getComponent<NodeImDrawer>())
            {
                ImPropertyGroup *group = drawer->getNodePropertyGroup();
                for (const auto &[animationName, animationData] : group->_animations)
                {
                    if (animationName == animation)
                    {
                        for (const auto &[propertyName, propertyValues] : animationData._values)
                        {
                            items.push_back({node, nullptr, propertyName, propertyValues, animationData._maxFrame, animationData._samples});
                        }
                    }
                }
         
                for (const auto &[componentName, group] : drawer->getComponentPropertyGroups())
                {
                     for (const auto &[animationName, animationData] : group->_animations)
                    {
                        if (animationName == animation)
                        {
                            for (const auto &[propertyName, propertyValues] : animationData._values)
                            {
                                items.push_back({node, node->getComponent(componentName), propertyName, propertyValues, animationData._maxFrame, animationData._samples});
                            }
                        }
                    }
                }
            }
        });

        return items;
    }

    std::unordered_map<std::string, bool> NodeImDrawer::getAnimationNames() const
    {
        std::unordered_map<std::string, bool> animationExists;

        Internal::performRecursively(getOwner(), [&](cocos2d::Node* node){
            if (NodeImDrawer* drawer = node->getComponent<NodeImDrawer>())
            {
                ImPropertyGroup *group = drawer->getNodePropertyGroup();
                for (const auto &[animationName, _] : group->_animations)
                {
                    animationExists[animationName] = true;
                }

                for (const auto &[componentName, group] : drawer->getComponentPropertyGroups())
                {
                     for (const auto &[animationName, _] : group->_animations)
                    {
                        animationExists[animationName] = true;
                    }
                }
            }
        });

        return animationExists;
    }

    void NodeImDrawer::update(float dt)
    {
        if (_animationName.empty())
            return;

        if (_isPlayingAnimation)
        {
            const ImPropertyGroup::AnimationData& animationData = _nodePropertyGroup->_animations[_animationName];
            const float secondPerFrame = 1.0f / animationData._samples;
            const int maxFrame = animationData._maxFrame;
            
            _elapsed += dt;
            int df = static_cast<int>(abs(_elapsed / secondPerFrame));
            switch (_animationWrapMode)
            {
            case AnimationWrapMode::Normal:
                {
                    _currentFrame += df;
                    if (_currentFrame > maxFrame)
                        _currentFrame = maxFrame;
                }
                break;
            case AnimationWrapMode::Loop:
                {
                    _currentFrame += df;
                    if (_currentFrame > maxFrame)
                        _currentFrame = 0;
                }
                break;
            case AnimationWrapMode::Reverse:
                {
                    _currentFrame -= df;
                    if (_currentFrame < 0)
                        _currentFrame = 0;
                }
                break;
            case AnimationWrapMode::ReverseLoop:
                {
                    _currentFrame -= df;
                    if (_currentFrame < 0)
                        _currentFrame = maxFrame;
                }
                break;
            }
          
            _elapsed -= (df * secondPerFrame);
        }

        Internal::performRecursively(getOwner(), [&](cocos2d::Node* node) {
            if (NodeImDrawer* drawer = node->getComponent<NodeImDrawer>())
            {
                ImPropertyGroup *group = drawer->getNodePropertyGroup();
                group->sample();

                for (const auto &[componentName, group] : drawer->getComponentPropertyGroups())
                {
                    group->sample();
                }
            }
        });
    }
}
