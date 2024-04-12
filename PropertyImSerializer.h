#ifndef __CCIMEDITOR_PROPERTYIMSERIALIZER_H__
#define __CCIMEDITOR_PROPERTYIMSERIALIZER_H__

#include "imgui/imgui.h"
#include "cocos2d.h"

namespace CCImEditor
{
    template <typename T, typename Enabled = void>
    struct PropertyImSerializer
    {
        static bool serialize(cocos2d::ValueMap& target, const char* label, ...)
        {
            CCLOGWARN("Can't serialize property: %s, missing specialization", label);
            return false;
        }
    };

    template <>
    struct PropertyImSerializer<bool> {
        static bool serialize(cocos2d::ValueMap& target, const char* label, bool v) {
            target.emplace(label, v);
            return true;
        }
    };
    
    template <>
    struct PropertyImSerializer<cocos2d::Vec3> {
        static bool serialize(cocos2d::ValueMap& target, const char* label, const cocos2d::Vec3& vec) {
            cocos2d::ValueVector v;
            v.push_back(cocos2d::Value(vec.x));
            v.push_back(cocos2d::Value(vec.y));
            v.push_back(cocos2d::Value(vec.z));
            target.emplace(label, v);
            return true;
        }
    };
}

#endif
