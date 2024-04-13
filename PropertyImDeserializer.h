#ifndef __CCIMEDITOR_PROPERTYIMDESERIALIZER_H__
#define __CCIMEDITOR_PROPERTYIMDESERIALIZER_H__

#include "cocos2d.h"

namespace CCImEditor
{
    template <typename T, typename Enabled = void>
    struct PropertyImDeserializer
    {
        static bool deserialize(const cocos2d::ValueMap& source, const char* label, ...)
        {
            CCLOGWARN("Can't deserialize property: %s, missing specialization", label);
            return false;
        }
    };

    template <>
    struct PropertyImDeserializer<bool> {
        static bool deserialize(const cocos2d::ValueMap& source, const char* label, bool& v) {
            cocos2d::ValueMap::const_iterator it = source.find(label);
            if (it != source.end())
            {
                v = it->second.asBool();
                return true;
            }
            
            return false;
        }
    };
    
    template <>
    struct PropertyImDeserializer<cocos2d::Vec3> {
        static bool deserialize(const cocos2d::ValueMap& source, const char* label, cocos2d::Vec3& vec) {
            cocos2d::ValueMap::const_iterator it = source.find(label);
            if (it != source.end())
            {
                const cocos2d::ValueVector& v = it->second.asValueVector();
                vec.x = v[0].asFloat();
                vec.y = v[1].asFloat();
                vec.z = v[2].asFloat();
                return true;
            }
            
            return false;
        }
    };
}

#endif
