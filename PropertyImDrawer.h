#ifndef __CCIMEDITOR_PROPERTYIMDRAWER_H__
#define __CCIMEDITOR_PROPERTYIMDRAWER_H__

#include <string>
#include "Widget.h"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "Editor.h"
#include <magic_enum/magic_enum.hpp>

#define CC_DECLARE_FLAGS(T) \
template <> \
struct magic_enum::customize::enum_range<T> { \
  static constexpr bool is_flags = true; \
}

CC_DECLARE_FLAGS(cocos2d::LightFlag);
CC_DECLARE_FLAGS(cocos2d::CameraFlag);
CC_DECLARE_FLAGS(cocos2d::ShadowSize);

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

        static bool serialize(cocos2d::Value& target, ...)
        {
            CCLOGWARN("Can't serialize property, missing specialization");
            return false;
        }

        static bool deserialize(const cocos2d::Value& source, ...)
        {
            CCLOGWARN("Can't deserialize property, missing specialization");
            return false;
        }
    };

    template <>
    struct PropertyImDrawer<bool> {
        static bool draw(const char* label, bool& v) {
            return ImGui::Checkbox(label, &v);
        }

        static bool serialize(cocos2d::Value& target, bool v) {
            target = v;
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, bool& v) {
            v = source.asBool();
            return true;
        }
    };
    
    template <>
    struct PropertyImDrawer<int> {
        static bool draw(const char* label, int& v, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0) {
            ImGui::DragInt(label, &v, v_speed, v_min,  v_max, format, flags);
            return ImGui::IsItemActive();
        }

        static bool serialize(cocos2d::Value& target, int v) {
            target = v;
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, int& v) {
            v = source.asInt();
            return true;
        }
    };

    template <>
    struct PropertyImDrawer<float> {
        static bool draw(const char* label, float& v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
            ImGui::DragFloat(label, &v, v_speed, v_min,  v_max, format, flags);
            return ImGui::IsItemActive();
        }

        static bool serialize(cocos2d::Value& target, float v) {
            target = v;
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, float& v) {
            v = source.asFloat();
            return true;
        }
    };

    struct Degrees {};
    template <>
    struct PropertyImDrawer<Degrees> {
        static bool draw(const char* label, float& v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
            float degree = CC_RADIANS_TO_DEGREES(v);
            if (ImGui::DragFloat(label, &degree, v_speed, v_min,  v_max, format, flags))
            {
                v = CC_DEGREES_TO_RADIANS(degree);
                return true;
            }

            return ImGui::IsItemActive();
        }

        static bool serialize(cocos2d::Value& target, float v) {
            target = v;
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, float& v) {
            v = source.asFloat();
            return true;
        }
    };

    template <>
    struct PropertyImDrawer<cocos2d::Vec2> {
        static bool draw(const char* label, cocos2d::Vec2& vec, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
            float v[2] = { vec.x, vec.y };
            if (ImGui::DragFloat2(label, v, v_speed, v_min,  v_max, format, flags))
            {
                vec.set(v);
                return true;
            }
            return ImGui::IsItemActive();
        }

        static bool serialize(cocos2d::Value& target, const cocos2d::Vec2& vec) {
            cocos2d::ValueVector v;
            v.push_back(cocos2d::Value(vec.x));
            v.push_back(cocos2d::Value(vec.y));
            target = std::move(v);
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, cocos2d::Vec2& vec) {
            const cocos2d::ValueVector& v = source.asValueVector();
            vec.x = v[0].asFloat();
            vec.y = v[1].asFloat();
            return true;
        }
    };

    template <>
    struct PropertyImDrawer<cocos2d::Size> {
        static bool draw(const char* label, cocos2d::Size& vec, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
            float v[2] = { vec.width, vec.height };
            if (ImGui::DragFloat2(label, v, v_speed, v_min,  v_max, format, flags))
            {
                vec.setSize(v[0], v[1]);
                return true;
            }
            return ImGui::IsItemActive();
        }

        static bool serialize(cocos2d::Value& target, const cocos2d::Size& vec) {
            cocos2d::ValueVector v;
            v.push_back(cocos2d::Value(vec.width));
            v.push_back(cocos2d::Value(vec.height));
            target = std::move(v);
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, cocos2d::Size& vec) {
            const cocos2d::ValueVector& v = source.asValueVector();
            vec.width = v[0].asFloat();
            vec.height = v[1].asFloat();
            return true;
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
            return ImGui::IsItemActive();
        }

        static bool serialize(cocos2d::Value& target, const cocos2d::Vec3& vec) {
            cocos2d::ValueVector v;
            v.push_back(cocos2d::Value(vec.x));
            v.push_back(cocos2d::Value(vec.y));
            v.push_back(cocos2d::Value(vec.z));
            target = std::move(v);
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, cocos2d::Vec3& vec) {
            const cocos2d::ValueVector& v = source.asValueVector();
            vec.x = v[0].asFloat();
            vec.y = v[1].asFloat();
            vec.z = v[2].asFloat();
            return true;
        }
    };

    template <>
    struct PropertyImDrawer<cocos2d::Color3B> {
        static bool draw(const char* label, cocos2d::Color3B& color, ImGuiColorEditFlags flags = 0) {
            float v[3] = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f };
            if (ImGui::ColorEdit3(label, v, flags))
            {
                color.r = static_cast<GLubyte>(v[0] * 255.0f);
                color.g = static_cast<GLubyte>(v[1] * 255.0f);
                color.b = static_cast<GLubyte>(v[2] * 255.0f);
                return true;
            }
            return ImGui::IsItemActive();
        }

        static bool serialize(cocos2d::Value& target, const cocos2d::Color3B& color) {
            cocos2d::ValueVector v;
            v.push_back(cocos2d::Value(color.r));
            v.push_back(cocos2d::Value(color.g));
            v.push_back(cocos2d::Value(color.b));
            target = std::move(v);
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, cocos2d::Color3B& color) {
            const cocos2d::ValueVector& v = source.asValueVector();
            color.r = v[0].asByte();
            color.g = v[1].asByte();
            color.b = v[2].asByte();
            return true;
        }
    };

    template <>
    struct PropertyImDrawer<cocos2d::Color4B> {
        static bool draw(const char* label, cocos2d::Color4B& color, ImGuiColorEditFlags flags = 0) {
            float v[4] = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
            if (ImGui::ColorEdit4(label, v, flags))
            {
                color.r = static_cast<GLubyte>(v[0] * 255.0f);
                color.g = static_cast<GLubyte>(v[1] * 255.0f);
                color.b = static_cast<GLubyte>(v[2] * 255.0f);
                color.a = static_cast<GLubyte>(v[3] * 255.0f);
                return true;
            }
            return ImGui::IsItemActive();
        }

        static bool serialize(cocos2d::Value& target, const cocos2d::Color4B& color) {
            cocos2d::ValueVector v;
            v.push_back(cocos2d::Value(color.r));
            v.push_back(cocos2d::Value(color.g));
            v.push_back(cocos2d::Value(color.b));
            v.push_back(cocos2d::Value(color.a));
            target = std::move(v);
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, cocos2d::Color4B& color) {
            const cocos2d::ValueVector& v = source.asValueVector();
            color.r = v[0].asByte();
            color.g = v[1].asByte();
            color.b = v[2].asByte();
            color.a = v[3].asByte();
            return true;
        }
    };

    template <>
    struct PropertyImDrawer<std::string> {
        static bool draw(const char* label, std::string& v, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr) {
            return ImGui::InputText(label, &v, flags, callback, user_data);
        }

        static bool serialize(cocos2d::Value& target, const std::string& v) {
            target = v;
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, std::string& v) {
            v = source.asString();
            return true;
        }
    };

    struct FilePath {};
    template <>
    struct PropertyImDrawer<FilePath> {
        static bool draw(const char* label, std::string& filePath) {
            ImGui::InputText(label, &filePath, ImGuiInputTextFlags_ReadOnly);
            if (ImGui::IsItemActivated())
            {
                Editor::getInstance()->openLoadFileDialog();
            }

            std::string tmp;
            if (Editor::getInstance()->fileDialogResult(tmp) && !tmp.empty())
            {
                filePath = std::move(tmp);
                return true;
            }

            return false;
        }

        static bool serialize(cocos2d::Value& target, const std::string& filePath) {
            target = filePath;
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, std::string& filePath) {
            filePath = source.asString();
            return true;
        }
    };

    namespace Internal
    {
        struct MaskOfBase {};
    }

    template <typename T>
    struct MaskOf: public Internal::MaskOfBase
    {
        using Type = T;
    };

    // 64bit mask is not supported because cocos2d::Value does not handle 64bit int.
    template <typename T>
    struct PropertyImDrawer<T, typename std::enable_if_t<std::is_base_of_v<Internal::MaskOfBase, T>>> {
        template <typename MaskType>
        static bool draw(const char* label, MaskType& mask) {
            std::string preview;
            preview.reserve(100);
            constexpr auto values = magic_enum::enum_values<T::Type>();
            for (auto enumValue : values)
            {
                if ((mask & (MaskType)enumValue) > 0)
                {
                    if (preview.size() > 0) {
                        preview.append(",");
                    }
                    preview.append(magic_enum::enum_name(enumValue));
                }
            }
            
            bool valueChanged = false;
            if (ImGui::BeginCombo(label, preview.c_str()))
            {
                bool firstItem = true;
                for (auto enumValue : values)
                {
                    auto name = magic_enum::enum_name(enumValue);
                    bool v = (mask & (MaskType)enumValue) > 0;
                    if (ImGui::Checkbox(name.data(), &v))
                    {
                        if (v)
                            mask |= (MaskType)enumValue;
                        else
                            mask &= ~(MaskType)enumValue;
                        valueChanged = true;
                    }

                    if (firstItem == true)
                    {
                        firstItem = false;
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            return valueChanged;
        }

        template <typename MaskType>
        static bool serialize(cocos2d::Value& target, MaskType mask) {
            target = (unsigned int)mask;
            return true;
        }

        template <typename MaskType>
        static bool deserialize(const cocos2d::Value& source, MaskType& mask) {
            mask = (MaskType)source.asUnsignedInt();
            return true;
        }
    };

    template <typename T>
    struct PropertyImDrawer<T, typename std::enable_if_t<std::is_enum_v<T>>> {
        static bool draw(const char* label, T& v) {
            bool valueChanged = false;
        
            // Get enum name for current value
            auto currentName = magic_enum::enum_name(v);
            const char* preview = currentName.empty() ? nullptr : currentName.data();
            
            if (ImGui::BeginCombo(label, preview)) {
                // Iterate through all possible enum values
                constexpr auto values = magic_enum::enum_values<T>();
                for (auto enumValue : values) {
                    auto name = magic_enum::enum_name(enumValue);
                    bool isSelected = (v == enumValue);
                    
                    if (ImGui::Selectable(name.data(), isSelected)) {
                        v = enumValue;
                        valueChanged = true;
                    }
                    
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            return valueChanged;
        }

        static bool serialize(cocos2d::Value& target, T v) {
            target = static_cast<int>(v);
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, T& v) {
            v = static_cast<T>(source.asInt());
            return true;
        }
    };

    template <>
    struct PropertyImDrawer<cocos2d::BlendFunc> {
        static bool draw(const char* label, cocos2d::BlendFunc& func) {
            constexpr const char* names[] = {
                "GL_ZERO",
                "GL_ONE",
                "GL_SRC_COLOR",
                "GL_ONE_MINUS_SRC_COLOR",
                "GL_SRC_ALPHA",
                "GL_ONE_MINUS_SRC_ALPHA",
                "GL_DST_COLOR",
                "GL_ONE_MINUS_DST_COLOR",
                "GL_DST_ALPHA",
                "GL_ONE_MINUS_DST_ALPHA"
            };

            constexpr GLenum values[] = {
                GL_ZERO,
                GL_ONE,
                GL_SRC_COLOR,
                GL_ONE_MINUS_SRC_COLOR,
                GL_SRC_ALPHA,
                GL_ONE_MINUS_SRC_ALPHA,
                GL_DST_COLOR,
                GL_ONE_MINUS_DST_COLOR,
                GL_DST_ALPHA,
                GL_ONE_MINUS_DST_ALPHA
            };

            bool valueChanged = false;
            for (int i = 0; i < 2; i++)
            {
                std::string labelStr;
                if (const char* token = strstr(label, "###"))
                    labelStr.assign(label, token - label);
                else
                    labelStr = label;
                labelStr.append(i == 0 ? " Src" : " Dst");

                GLenum& v = i == 0 ? func.src : func.dst;
                auto it = std::find(std::begin(values), std::end(values), v);
                const int distance = std::distance(values, it);
                
                if (ImGui::BeginCombo(labelStr.c_str(), distance >= 0 && distance < IM_ARRAYSIZE(names) ? names[distance] : nullptr)) {
                    for (int j = 0; j < IM_ARRAYSIZE(values); j++) {
                        auto enumValue = values[j];
                        auto name = names[j];
                        bool isSelected = (v == enumValue);

                        if (ImGui::Selectable(name, isSelected)) {
                            v = enumValue;
                            valueChanged = true;
                        }

                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            return valueChanged;
        }

        static bool serialize(cocos2d::Value& target, const cocos2d::BlendFunc& func) {
            cocos2d::ValueVector v;
            v.push_back(cocos2d::Value(func.src));
            v.push_back(cocos2d::Value(func.dst));
            target = std::move(v);
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, cocos2d::BlendFunc& func) {
            const cocos2d::ValueVector& v = source.asValueVector();
            func.src = v[0].asUnsignedInt();
            func.dst = v[1].asUnsignedInt();
            return true;
        }
    };
}

#endif
