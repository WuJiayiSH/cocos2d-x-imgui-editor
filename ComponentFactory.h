#ifndef __CCIMEDITOR_COMPONENTFACTORY_H__
#define __CCIMEDITOR_COMPONENTFACTORY_H__

#include "cocos2d.h"

#include <string>
#include <unordered_map>
#include "NodeImDrawer.h"

namespace CCImEditor
{
    class ComponentFactory
    {
    public:
        struct ComponentType
        {
            friend class ComponentFactory;
            
            ComponentType(const std::string& name, const std::string& displayName, const std::function<ImPropertyGroup*()>& constructor)
            : _name(name)
            , _displayName(displayName)
            , _constructor(constructor)
            {
            }

            const std::string& getName() const { return _name; };
            const std::string& getDisplayName() const { return _displayName; };

            ImPropertyGroup* create() const { return _constructor(); };

        private:
            std::string _name;
            std::string _displayName;
            std::function<ImPropertyGroup*()> _constructor;
        };
    
        template <typename ComponentPropertyGroupType, typename OwnerType>
        void registerComponent(const char* name, const char* displayName)
        {
            std::function<ImPropertyGroup*()> constructor = [name, displayName]() -> ImPropertyGroup* {
                auto owner = OwnerType::create();
                if (!owner)
                    return nullptr;

                ComponentPropertyGroupType* componentPropertyGroup = new (std::nothrow)ComponentPropertyGroupType();
                if (!componentPropertyGroup)
                    return nullptr;

                const char* shortName = displayName;
                while(const char* tmp = strstr(shortName, "/"))
                {
                    shortName = tmp + 1;
                }
                componentPropertyGroup->_shortName = shortName;
                componentPropertyGroup->_typeName = name;

                if (!componentPropertyGroup->init())
                {
                    delete componentPropertyGroup;
                    return nullptr;
                }
                componentPropertyGroup->_owner = owner;
                componentPropertyGroup->autorelease();
                return componentPropertyGroup;
            };
            ComponentFactory::getInstance()->_componentTypes.emplace(name, ComponentType(name, displayName, constructor));
        }

        typedef std::unordered_map<std::string, ComponentType> ComponentTypeMap;
        const ComponentTypeMap& getComponentTypes() { return _componentTypes; };

        ImPropertyGroup* createComponent(const std::string& name);
        
        static ComponentFactory* getInstance();

    private:
        ComponentTypeMap _componentTypes;
    };
}

#endif
