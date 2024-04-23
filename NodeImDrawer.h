#ifndef __CCIMEDITOR_NODEIMDRAWER_H__
#define __CCIMEDITOR_NODEIMDRAWER_H__

#include "cocos2d.h"
#include "invoke.hpp/invoke.hpp"
#include "PropertyImDrawer.h"

namespace CCImEditor
{
    namespace Internal
    {
        struct DefaultArgumentTag {};
    }
    
    class NodeImDrawer : public cocos2d::Component
    {
    public:
        enum class Context
        {
            DRAW,
            SERIALIZE,
            DESERIALIZE
        };

        friend class NodeFactory;
        virtual void draw() {};
        void serialize(cocos2d::ValueMap&);
        void deserialize(const cocos2d::ValueMap&);
        const std::string& getTypeName() const {return _typeName;}
        bool init() override;

    protected:
        template <class PropertyType, class Setter, class Object, class... Args>
        void customProperty(const char *label, Setter &&setter, Object&& object, Args &&...args)
        {
            if (_context == Context::DRAW)
            {
                cocos2d::Value& v = _customValue[label];
                if (PropertyImDrawer<PropertyType>::draw(label, v, std::forward<Args>(args)...))
                {
                    invoke_hpp::invoke(std::forward<Setter>(setter), std::forward<Object>(object), v);
                }
            }
            else if (_context == Context::SERIALIZE)
            {
                const cocos2d::Value& v = _customValue[label];
                PropertyImDrawer<PropertyType>::serialize(*_contextValue, label, v);
            }
            else if (_context == Context::DESERIALIZE)
            {
                cocos2d::Value& v = _customValue[label];
                if (PropertyImDrawer<PropertyType>::deserialize(*_contextValue, label, v))
                {
                    invoke_hpp::invoke(std::forward<Setter>(setter), std::forward<Object>(object), v);
                }
            }
        }

        template <class DrawerType = Internal::DefaultArgumentTag, class Getter, class Setter, class Object, class... Args>
        void property(const char *label, Getter &&getter, Setter &&setter, Object&& object, Args &&...args)
        {
            using PropertyTypeMaybeQualified = typename invoke_hpp::invoke_result_t<decltype(getter), Object>;
            using PropertyType = typename std::remove_cv<std::remove_reference<PropertyTypeMaybeQualified>::type>::type;

            // use PropertyType if DrawerType is not specified
            using PropertyOrDrawerType = typename std::conditional<std::is_same<DrawerType, Internal::DefaultArgumentTag>::value, PropertyType, DrawerType>::type;
                                          
            if (_context == Context::DRAW)
            {
                auto v = invoke_hpp::invoke(std::forward<Getter>(getter), std::forward<Object>(object));
                if (PropertyImDrawer<PropertyOrDrawerType>::draw(label, v, std::forward<Args>(args)...))
                {
                    invoke_hpp::invoke(std::forward<Setter>(setter), std::forward<Object>(object), v);
                }
            }
            else if (_context == Context::SERIALIZE)
            {
                const auto& v = invoke_hpp::invoke(std::forward<Getter>(getter), std::forward<Object>(object));
                PropertyImDrawer<PropertyOrDrawerType>::serialize(*_contextValue, label, v);
            }
            else if (_context == Context::DESERIALIZE)
            {
                PropertyType v;
                if (PropertyImDrawer<PropertyOrDrawerType>::deserialize(*_contextValue, label, v))
                {
                    invoke_hpp::invoke(std::forward<Setter>(setter), std::forward<Object>(object), v);
                }
            }
        }

    protected:
        Context _context = Context::DRAW;

    private:
        std::string _typeName;
        cocos2d::ValueMap* _contextValue = nullptr;
        cocos2d::ValueMap _customValue;
    };

   
}

#endif
