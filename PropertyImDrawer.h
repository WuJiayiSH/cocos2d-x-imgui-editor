#ifndef __CCIMEDITOR_PROPERTYIMDRAWER_H__
#define __CCIMEDITOR_PROPERTYIMDRAWER_H__

#include <string>
#include "Widget.h"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
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
            return ImGui::DragInt(label, &v, v_speed, v_min,  v_max, format, flags);
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
            return ImGui::DragFloat(label, &v, v_speed, v_min,  v_max, format, flags);
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

            return false;
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
            return false;
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
            return false;
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
            return false;
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
            return false;
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
            return false;
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
        struct EnumBase {};
        struct MaskBase {};
    }

    template <typename T>
    struct Enum: public Internal::EnumBase
    {
        using Type = T;
    };
    
    template <typename T>
    struct Mask: public Internal::MaskBase
    {
        using Type = T;
    };

    // 64bit mask is not supported because cocos2d::Value does not handle 64bit int.
    template <typename T>
    struct PropertyImDrawer<T, typename std::enable_if<std::is_base_of<Internal::MaskBase, T>::value>::type> {
        static_assert(IM_ARRAYSIZE(T::Type::s_names) == IM_ARRAYSIZE(T::Type::s_values), "Size of enum names and values do not match");
        static_assert(sizeof(typename T::Type::MaskType) <= 4,
            "64bit mask is not supported because cocos2d::Value does not handle 64bit int");

        static bool draw(const char* label, typename T::Type::MaskType& mask) {
            std::stringstream stream;
            bool firstItem = true;
            for (int i = 0; i < IM_ARRAYSIZE(T::Type::s_names); i++)
            {
                if ((mask & T::Type::s_values[i]) > 0)
                {
                    if (!firstItem) {
                        stream << ",";
                    }
                    firstItem = false;
                    stream << T::Type::s_names[i];
                }
            }
            
            bool valueChanged = false;
            if (ImGui::BeginCombo(label, stream.str().c_str()))
            {
                for (int i = 0; i < IM_ARRAYSIZE(T::Type::s_names); i++)
                {
                    bool v = (mask & T::Type::s_values[i]) > 0;
                    if (ImGui::Checkbox(T::Type::s_names[i], &v))
                    {
                        if (v)
                            mask |= T::Type::s_values[i];
                        else
                            mask &= ~T::Type::s_values[i];
                        valueChanged = true;
                    }

                    if (i == 0)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            return valueChanged;
        }

        static bool serialize(cocos2d::Value& target, typename T::Type::MaskType mask) {
            target = mask;
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, typename T::Type::MaskType& mask) {
            mask = source.asUnsignedInt();
            return true;
        }
    };

    template <typename T>
    struct PropertyImDrawer<T, typename std::enable_if<std::is_base_of<Internal::EnumBase, T>::value>::type> {
        static_assert(IM_ARRAYSIZE(T::Type::s_names) == IM_ARRAYSIZE(T::Type::s_values), "Size of enum names and values do not match");

        static bool draw(const char* label, typename T::Type::EnumType& v) {
            bool valueChanged = false;
            auto it = std::find(std::begin(T::Type::s_values), std::end(T::Type::s_values), (decltype(*T::Type::s_values))v);
            const int i = std::distance(T::Type::s_values, it);
            if (ImGui::BeginCombo(label, i >= 0 && i < IM_ARRAYSIZE(T::Type::s_names) ? T::Type::s_names[i] : nullptr))
            {
                for (int j = 0; j < IM_ARRAYSIZE(T::Type::s_names); j++)
                {
                    bool selected = i == j;
                    if (ImGui::Selectable(T::Type::s_names[j], &selected))
                    {
                        v = static_cast<typename T::Type::EnumType>(T::Type::s_values[j]);
                        valueChanged = true;
                    }

                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            return valueChanged;
        }

        static bool serialize(cocos2d::Value& target, typename T::Type::EnumType v) {
            target = static_cast<int>(v);
            return true;
        }

        static bool deserialize(const cocos2d::Value& source, typename T::Type::EnumType& v) {
            v = static_cast<typename T::Type::EnumType>(source.asInt());
            return true;
        }
    };

    struct LightFlag {
        using MaskType = unsigned int;
        using EnumType = cocos2d::LightFlag;
        static constexpr const char* s_names[] = {
            "LIGHT0",
            "LIGHT1",
            "LIGHT2",
            "LIGHT3",
            "LIGHT4",
            "LIGHT5",
            "LIGHT6",
            "LIGHT7",
            "LIGHT8",
            "LIGHT9",
            "LIGHT10",
            "LIGHT11",
            "LIGHT12",
            "LIGHT13",
            "LIGHT14",
            "LIGHT15",
        };

        static constexpr int s_values[] = {
            (int)cocos2d::LightFlag::LIGHT0,
            (int)cocos2d::LightFlag::LIGHT1,
            (int)cocos2d::LightFlag::LIGHT2,
            (int)cocos2d::LightFlag::LIGHT3,
            (int)cocos2d::LightFlag::LIGHT4,
            (int)cocos2d::LightFlag::LIGHT5,
            (int)cocos2d::LightFlag::LIGHT6,
            (int)cocos2d::LightFlag::LIGHT7,
            (int)cocos2d::LightFlag::LIGHT8,
            (int)cocos2d::LightFlag::LIGHT9,
            (int)cocos2d::LightFlag::LIGHT10,
            (int)cocos2d::LightFlag::LIGHT11,
            (int)cocos2d::LightFlag::LIGHT12,
            (int)cocos2d::LightFlag::LIGHT13,
            (int)cocos2d::LightFlag::LIGHT14,
            (int)cocos2d::LightFlag::LIGHT15,
        };
    };

    struct CameraFlag {
        using MaskType = unsigned short;
        static constexpr const char* s_names[] = {
            "DEFAULT",
            "USER1",
            "USER2",
            "USER3",
            "USER4",
            "USER5",
            "USER6",
            "USER7",
            "USER8",
        };

        static constexpr int s_values[] = {
            (int)cocos2d::CameraFlag::DEFAULT,
            (int)cocos2d::CameraFlag::USER1,
            (int)cocos2d::CameraFlag::USER2,
            (int)cocos2d::CameraFlag::USER3,
            (int)cocos2d::CameraFlag::USER4,
            (int)cocos2d::CameraFlag::USER5,
            (int)cocos2d::CameraFlag::USER6,
            (int)cocos2d::CameraFlag::USER7,
            (int)cocos2d::CameraFlag::USER8,
        };
    };
    
    struct ShadowSize {
        using EnumType = cocos2d::ShadowSize;
        static constexpr const char* s_names[] = {
            "Low_256x256",
            "Medium_512x512",
            "High_1024x1024",
            "Ultra_2048x2048",
        };

        static constexpr int s_values[] = {
            (int)cocos2d::ShadowSize::Low_256x256,
            (int)cocos2d::ShadowSize::Medium_512x512,
            (int)cocos2d::ShadowSize::High_1024x1024,
            (int)cocos2d::ShadowSize::Ultra_2048x2048,
        };
    };

    struct BlendSrcDst {
        using EnumType = GLenum;
        static constexpr const char* s_names[] = {
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

        static constexpr int s_values[] = {
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
    };

    struct TextVAlignment {
        using EnumType = cocos2d::TextVAlignment;
        static constexpr const char* s_names[] = {
            "TOP",
            "CENTER",
            "BOTTOM"
        };

        static constexpr int s_values[] = {
            (int)cocos2d::TextVAlignment::TOP,
            (int)cocos2d::TextVAlignment::CENTER,
            (int)cocos2d::TextVAlignment::BOTTOM
        };
    };

    struct TextHAlignment {
        using EnumType = cocos2d::TextHAlignment;
        static constexpr const char* s_names[] = {
            "LEFT",
            "CENTER",
            "RIGHT"
        };

        static constexpr int s_values[] = {
            (int)cocos2d::TextHAlignment::LEFT,
            (int)cocos2d::TextHAlignment::CENTER,
            (int)cocos2d::TextHAlignment::RIGHT
        };
    };

    struct LabelOverflow {
        using EnumType = cocos2d::Label::Overflow;
        static constexpr const char* s_names[] = {
            "NONE",
            "CLAMP",
            "SHRINK",
            "RESIZE_HEIGHT",
        };

        static constexpr int s_values[] = {
            (int)cocos2d::Label::Overflow::NONE,
            (int)cocos2d::Label::Overflow::CLAMP,
            (int)cocos2d::Label::Overflow::SHRINK,
            (int)cocos2d::Label::Overflow::RESIZE_HEIGHT
        };
    };
}

#endif
