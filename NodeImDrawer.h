#ifndef __CCIMEDITOR_NODEIMDRAWER_H__
#define __CCIMEDITOR_NODEIMDRAWER_H__

#include "cocos2d.h"
#include "PropertyImDrawer.h"
#include "commands/CustomCommand.h"
#include <type_traits>
#include <utility>

namespace CCImEditor
{
    namespace Internal
    {
        namespace Animation
        {
            struct Sequence;

            struct SequenceItem
            {
                SequenceItem(cocos2d::Node* node, cocos2d::Component* component, std::string name, const std::map<int, cocos2d::Value>& values)
                : _node(node)
                , _component(component)
                , _name(name)
                , _values(values)
                {
                    _frameStart = _values.begin()->first;
                    _frameEnd = std::prev(_values.end())->first;

                    int depth = 0;
                    cocos2d::Node* parent = _node;
                    while (parent = parent->getParent())
                        depth ++;

                    for (int i = 0; i < depth - 1; i++)
                        _label += "  ";

                    _label += node->getName();
                    _label += ": ";
                    if (_component)
                    {
                        _label += _component->getName();
                        _label += ".";
                    }

                    _label += _name;
                }

                cocos2d::Node* _node;
                cocos2d::Component* _component;
                std::string _name;
                const std::map<int, cocos2d::Value>& _values;

                std::string _label;
                int _frameStart;
                int _frameEnd;
            };

            enum class State
            {
                Unset,
                Playing,
                Recording,
            };
        }

        struct DefaultArgumentTag {};

        struct DefaultGetterBase{};

        // Deduce type has a lerp function
        template <typename T, typename U, typename = void>
        struct HasLerp : std::false_type {};

        template <typename T, typename U>
        struct HasLerp<T, U, std::void_t<
            decltype(T::lerp(std::declval<U>(), std::declval<U>(), std::declval<float>()))
        >> : std::true_type {};

        void performRecursively(cocos2d::Node *node, std::function<void(cocos2d::Node*)> func);
    }

    class ImPropertyGroup : public cocos2d::Ref
    {
    public:
        enum class Context
        {
            DRAW,
            SERIALIZE,
            DESERIALIZE,
            PLAY,
        };

        friend class NodeFactory;
        friend class ComponentFactory;
        friend struct Internal::Animation::Sequence;
        friend class NodeImDrawer;
        virtual void draw() {};
        void serialize(cocos2d::ValueMap&);
        void deserialize(const cocos2d::ValueMap&);
        void deserializeAnimations(const cocos2d::ValueMap&);
        void play();
        const std::string& getTypeName() const {return _typeName;}
        const std::string& getShortName() const {return _shortName;}
        virtual bool init();
        cocos2d::Ref* getOwner() const {return _owner;}
    private:
        // Try to get value from _customValue if getter is a DefaultGetterBase.
        // If getter is not a DefaultGetterBase or value not found in _customValue,
        // call the getter and return its result
        template <class DrawerType = Internal::DefaultArgumentTag, class PropertyType, class Getter, class Object>
        PropertyType getFromCustomValueOrGetter(const char *key, Getter &&getter, Object&& object)
        {
            using PropertyOrDrawerType = typename std::conditional<std::is_same<DrawerType, Internal::DefaultArgumentTag>::value, PropertyType, DrawerType>::type;

            if (std::is_base_of<Internal::DefaultGetterBase, Getter>::value)
            {
                auto it = _customValue.find(key);
                if (it != _customValue.end())
                {
                    PropertyType v;
                    if (CCImEditor::PropertyImDrawer<PropertyOrDrawerType>::deserialize(it->second, v))
                    {
                        return v;
                    }
                }
            }
            
            return std::invoke(std::forward<Getter>(getter), std::forward<Object>(object));
        }

        // Return a setter warpper which can be used in command history.
        // The warpper will write value to _customValue too.
        template <class DrawerType = Internal::DefaultArgumentTag, class PropertyType, class Setter, class Object>
        std::function<void()> getSetterWrapper(const char *key, Setter &&setter, Object&& object, const PropertyType& v)
        {
            using PropertyOrDrawerType = typename std::conditional<std::is_same<DrawerType, Internal::DefaultArgumentTag>::value, PropertyType, DrawerType>::type;

            std::string keyStr = key; // cache key
            auto setterWrapper = std::bind(std::forward<Setter>(setter), std::forward<Object>(object), v);
            return [this, keyStr, v, setterWrapper]()
            {
                PropertyImDrawer<PropertyOrDrawerType>::serialize(_customValue[keyStr], v);
                setterWrapper();
            };
        }

    protected:
        template <class DrawerType = Internal::DefaultArgumentTag, class Getter, class Setter, class Object, class... Args>
        void property(const char *label, Getter &&getter, Setter &&setter, Object&& object, Args &&...args)
        {
            using PropertyTypeMaybeQualified = typename std::invoke_result_t<decltype(getter), Object>;
            using PropertyType = typename std::remove_cv<typename std::remove_reference<PropertyTypeMaybeQualified>::type>::type;

            // use PropertyType if DrawerType is not specified
            using PropertyOrDrawerType = typename std::conditional<std::is_same<DrawerType, Internal::DefaultArgumentTag>::value, PropertyType, DrawerType>::type;

            using PropertyImDrawerType = typename PropertyImDrawer<PropertyOrDrawerType>;

            const char* key;
            if (key = strstr(label, "###"))
                key += 3;
            else
                key = label;

            if (_context == Context::DRAW)
            {
                // object is always valid for undo/redo. It will be retained if it is removed form scene by a command.
                using ObjectType = typename std::remove_pointer<typename std::remove_cv<typename std::remove_reference<Object>::type>::type>::type;

                auto v = getFromCustomValueOrGetter<DrawerType, PropertyType>(key, std::forward<Getter>(getter), std::forward<Object>(object));
                if (PropertyImDrawerType::draw(label, v, std::forward<Args>(args)...))
                {
                    if (!_undo)
                    {
                        auto v0 = getFromCustomValueOrGetter<DrawerType, PropertyType>(key, std::forward<Getter>(getter), std::forward<Object>(object));
                        _undo = getSetterWrapper<DrawerType>(key, std::forward<Setter>(setter), std::forward<Object>(object), v0);
                        _activeID = ImGui::GetItemID();
                    }
                    else
                    {
                        CC_ASSERT(_activeID == ImGui::GetItemID());
                    }

                    auto drawer = getDrawer();
                    if (drawer->_animationState == Internal::Animation::State::Recording)
                        PropertyImDrawerType::serialize(_animations[drawer->_animationName][key][drawer->_currentFrame], v);
                    else
                        PropertyImDrawerType::serialize(_customValue[key], v);
                    std::invoke(std::forward<Setter>(setter), std::forward<Object>(object), v);
                }
                else
                {
                    if (_undo && _activeID == ImGui::GetItemID())
                    {
                        CustomCommand* cmd = CustomCommand::create(
                            getSetterWrapper<DrawerType>(key, std::forward<Setter>(setter), std::forward<Object>(object), v),
                            _undo
                        );
                        Editor::getInstance()->getCommandHistory().queue(cmd, false);
                        _undo = nullptr;
                        _activeID = 0;
                    }
                }
            }
            else if (_context == Context::PLAY)
            {
                do
                {
                    auto drawer = getDrawer();
                    auto animation = _animations.find(drawer->_animationName);
                    if (animation == _animations.end())
                        break;

                    auto property = animation->second.find(key);
                    if (property == animation->second.end())
                        break;

                    auto it1 = property->second.upper_bound(drawer->_currentFrame);
                    auto it0 = std::prev(it1);
                    if (it1 != property->second.end() && it0 != property->second.end())
                    {
                        if constexpr (Internal::HasLerp<PropertyImDrawerType, PropertyType>::value)
                        {
                            PropertyType v0;
                            PropertyType v1;
                            if (PropertyImDrawerType::deserialize(it0->second, v0) &&
                                PropertyImDrawerType::deserialize(it1->second, v1))
                            {
                                float offset = (float)(drawer->_currentFrame - it0->first) / (it1->first - it0->first);
                                PropertyType v = PropertyImDrawerType::lerp(v0, v1, offset);
                                std::invoke(std::forward<Setter>(setter), std::forward<Object>(object), v);
                            }
                        }
                        else
                        {
                            PropertyType v0;
                            if (PropertyImDrawerType::deserialize(it0->second, v0))
                            {
                                std::invoke(std::forward<Setter>(setter), std::forward<Object>(object), v0);
                            }
                        }
                    }
                    else if (it0 != property->second.end())
                    {
                        PropertyType v0;
                        if (PropertyImDrawerType::deserialize(it0->second, v0))
                        {
                            std::invoke(std::forward<Setter>(setter), std::forward<Object>(object), v0);
                        }
                    }
                    else if (it1 != property->second.end())
                    {
                        PropertyType v1;
                        if (PropertyImDrawerType::deserialize(it1->second, v1))
                        {
                            std::invoke(std::forward<Setter>(setter), std::forward<Object>(object), v1);
                        }
                    }
                } while (false);
            }
            else if (_context == Context::SERIALIZE)
            {
                const auto& v = getFromCustomValueOrGetter<DrawerType, PropertyType>(key, std::forward<Getter>(getter), std::forward<Object>(object));
                PropertyImDrawerType::serialize((*_contextValue)[key], v);
            }
            else if (_context == Context::DESERIALIZE)
            {
                cocos2d::ValueMap::const_iterator it = _contextValue->find(key);
                if (it != _contextValue->end())
                {
                    PropertyType v;
                    if (PropertyImDrawerType::deserialize(it->second, v))
                    {
                        PropertyImDrawerType::serialize(_customValue[key], v);
                        std::invoke(std::forward<Setter>(setter), std::forward<Object>(object), v);
                    }
                }
            }
        }

        bool drawHeader(const char* label)
        {
            if (_context == Context::DRAW && !ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen))
                return false;

            return true;
        }

        bool drawHeader(const char* label, bool* visible)
        {
            if (_context == Context::DRAW && !ImGui::CollapsingHeader(label, visible, ImGuiTreeNodeFlags_DefaultOpen))
                return false;

            return true;
        }

        Context _context = Context::DRAW;
        cocos2d::ValueMap _customValue;
        
    private:
        NodeImDrawer* getDrawer() const 
        {
            cocos2d::Ref* owner = _owner.get();
            if (cocos2d::Node* node = dynamic_cast<cocos2d::Node*>(owner))
            {
                return node->getComponent<NodeImDrawer>();
            }
            else
            {
                cocos2d::Component* component = static_cast<cocos2d::Component*>(owner);
                return component->getOwner()->getComponent<NodeImDrawer>();
            }
        }

        std::string _typeName;
        std::string _shortName;
        cocos2d::ValueMap* _contextValue = nullptr;
        std::function<void()> _undo;
        ImGuiID _activeID = 0;
        cocos2d::WeakPtr<cocos2d::Ref> _owner;

        std::unordered_map<std::string, std::unordered_map<std::string, std::map<int, cocos2d::Value>>> _animations;
    };

    template <class T>
    struct DefaultGetter : Internal::DefaultGetterBase
    {
        DefaultGetter(const T& v)
            :_defaultValue(v)
        {
        }

        DefaultGetter()
            :_defaultValue{}
        {
        }

        T operator()(...) const
        {
            return _defaultValue;
        }

        T _defaultValue;
    };

    enum class AnimationWrapMode
    {
        Normal,
        Loop,
        Reverse,
        ReverseLoop,
    };
    
    class Animation;
    
    class NodeImDrawer : public cocos2d::Component
    {
    public:
        friend class NodeFactory;
        friend class Animation;
        friend class ImPropertyGroup;
        static NodeImDrawer* create();
        bool init() override;

        void draw();
        void serialize(cocos2d::ValueMap& target){_nodePropertyGroup->serialize(target);}
        void deserialize(const cocos2d::ValueMap& source){_nodePropertyGroup->deserialize(source);}
        void play(const std::string& animation);
        void stop();

        const std::string& getTypeName() const {return _nodePropertyGroup->getTypeName();}
        const std::string& getShortName() const {return _nodePropertyGroup->getShortName();}
        ImPropertyGroup* getNodePropertyGroup() {return _nodePropertyGroup;}

        const std::string& getFilename() const {return _filename;}
        void setFilename(const std::string& filename) {_filename = filename;}

        void setNodePropertyGroup(ImPropertyGroup* group) {_nodePropertyGroup = group;}
        void setComponentPropertyGroup(std::string name, ImPropertyGroup* group);
        ImPropertyGroup* getComponentPropertyGroup(std::string name) {return _componentPropertyGroups[name];}
        const std::map<std::string, cocos2d::RefPtr<ImPropertyGroup>>& getComponentPropertyGroups() const {return _componentPropertyGroups;}

        bool canHaveChildren() const;
        bool canHaveComponents() const;
        bool canBeRoot() const;

        void update(float dt) override;

    private:
        void applyAnimationRecursively(Internal::Animation::State state, const std::string& animation, int frame, AnimationWrapMode wrapMode, uint16_t sample);

        std::vector<Internal::Animation::SequenceItem> getAnimationSequenceItems(const std::string& animation) const;
        std::unordered_map<std::string, bool> getAnimationNames() const;

        uint32_t getMask() const;
        cocos2d::RefPtr<ImPropertyGroup> _nodePropertyGroup;
        std::map<std::string, cocos2d::RefPtr<ImPropertyGroup>> _componentPropertyGroups;
        std::string _filename;

        // animation
        Internal::Animation::State _animationState = Internal::Animation::State::Unset;
        std::string _animationName;
        int _currentFrame;
        AnimationWrapMode _animationWrapMode = AnimationWrapMode::Loop;
        float _elapsed;
        uint16_t _sample = 30;
    };
}

#endif
