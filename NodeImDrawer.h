#ifndef __CCIMEDITOR_NODEIMDRAWER_H__
#define __CCIMEDITOR_NODEIMDRAWER_H__

#include "cocos2d.h"
#include "PropertyImDrawer.h"
#include "commands/CustomCommand.h"

namespace CCImEditor
{
    namespace Internal
    {
        struct DefaultArgumentTag {};

        struct DefaultGetterBase{};
    }

    class ImPropertyGroup : public cocos2d::Ref
    {
    public:
        enum class Context
        {
            DRAW,
            SERIALIZE,
            DESERIALIZE
        };

        friend class NodeFactory;
        friend class ComponentFactory;
        virtual void draw() {};
        void serialize(cocos2d::ValueMap&);
        void deserialize(const cocos2d::ValueMap&);
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
                if (PropertyImDrawer<PropertyOrDrawerType>::draw(label, v, std::forward<Args>(args)...))
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

                    PropertyImDrawer<PropertyOrDrawerType>::serialize(_customValue[key], v);
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
            else if (_context == Context::SERIALIZE)
            {
                const auto& v = getFromCustomValueOrGetter<DrawerType, PropertyType>(key, std::forward<Getter>(getter), std::forward<Object>(object));
                PropertyImDrawer<PropertyOrDrawerType>::serialize((*_contextValue)[key], v);
            }
            else if (_context == Context::DESERIALIZE)
            {
                cocos2d::ValueMap::const_iterator it = _contextValue->find(key);
                if (it != _contextValue->end())
                {
                    PropertyType v;
                    if (PropertyImDrawer<PropertyOrDrawerType>::deserialize(it->second, v))
                    {
                        PropertyImDrawer<PropertyOrDrawerType>::serialize(_customValue[key], v);
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
        std::string _typeName;
        std::string _shortName;
        cocos2d::ValueMap* _contextValue = nullptr;
        std::function<void()> _undo;
        ImGuiID _activeID = 0;
        cocos2d::RefPtr<cocos2d::Ref> _owner;
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

    class NodeImDrawer : public cocos2d::Component
    {
    public:
        friend class NodeFactory;
        static NodeImDrawer* create();
        bool init() override;

        void draw();
        void serialize(cocos2d::ValueMap& target){_nodePropertyGroup->serialize(target);}
        void deserialize(const cocos2d::ValueMap& source){_nodePropertyGroup->deserialize(source);}
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

    private:
        uint32_t getMask() const;
        cocos2d::RefPtr<ImPropertyGroup> _nodePropertyGroup;
        std::map<std::string, cocos2d::RefPtr<ImPropertyGroup>> _componentPropertyGroups;
        std::string _filename;
    };
}

#endif
