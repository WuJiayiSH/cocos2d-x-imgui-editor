#ifndef __CCIMEDITOR_PROPERTYIMDRAWER_H__
#define __CCIMEDITOR_PROPERTYIMDRAWER_H__

#include <string>
#include "Widget.h"
#include "imgui/imgui.h"
#include "Editor.h"

namespace CCImEditor
{
    template <typename T, typename Enabled = void>
    struct PropertyImDrawer
    {
        static bool draw(const char* label, ...)
        {
            CCLOGWARN("Can't draw property: %s, missing specialization", label);
            return false;
        }

        static bool serialize(cocos2d::ValueMap& target, const char* label, ...)
        {
            CCLOGWARN("Can't serialize property: %s, missing specialization", label);
            return false;
        }

        static bool deserialize(const cocos2d::ValueMap& source, const char* label, ...)
        {
            CCLOGWARN("Can't deserialize property: %s, missing specialization", label);
            return false;
        }
    };

    template <>
    struct PropertyImDrawer<bool> {
        static bool draw(const char* label, bool& v) {
            return ImGui::Checkbox(label, &v);
        }

        static bool serialize(cocos2d::ValueMap& target, const char* label, bool v) {
            target.emplace(label, v);
            return true;
        }

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
    struct PropertyImDrawer<cocos2d::Vec3> {
        static bool draw(const char* label, cocos2d::Vec3& vec, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
            float v[3] = { vec.x, vec.y, vec.z };
            if (ImGui::DragFloat3(label, v, v_speed, v_min,  v_max, format, flags))
            {
                vec.set(v);
                return true;
            }
            return false;
        }

        static bool serialize(cocos2d::ValueMap& target, const char* label, const cocos2d::Vec3& vec) {
            cocos2d::ValueVector v;
            v.push_back(cocos2d::Value(vec.x));
            v.push_back(cocos2d::Value(vec.y));
            v.push_back(cocos2d::Value(vec.z));
            target.emplace(label, v);
            return true;
        }

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

    struct FilePath {};
    template <>
    struct PropertyImDrawer<FilePath> {
        static bool draw(const char* label, cocos2d::Value& filePath) {
            std::string v = filePath.asString();
            // assume imgui does not modify the c string because of the read-only flag
            ImGui::InputText(label, const_cast<char*>(v.c_str()), v.size(), ImGuiInputTextFlags_ReadOnly);
            if (ImGui::IsItemActivated())
            {
                Editor::getInstance()->openLoadFileDialog();
            }

            if (Editor::getInstance()->fileDialogResult(v) && !v.empty())
            {
                filePath = v;
                return true;
            }

            return false;
        }

        static bool serialize(cocos2d::ValueMap& target, const char* label, const cocos2d::Value& filePath) {
            target.emplace(label, filePath);
            return true;
        }

        static bool deserialize(const cocos2d::ValueMap& source, const char* label, cocos2d::Value& filePath) {
            cocos2d::ValueMap::const_iterator it = source.find(label);
            if (it != source.end())
            {
                filePath = it->second;
                return true;
            }
            
            return false;
        }
    };
}

#endif
