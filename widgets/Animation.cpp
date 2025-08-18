#include "Animation.h"

#include "Editor.h"
#include "NodeImDrawer.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "ImSequencer.h"
#include "cocos2d.h"
#include "PropertyImDrawer.h"

namespace CCImEditor
{
    namespace Internal
    {
        void Sequence::prepare(const std::string &animation)
        {
            _items.clear();
            if (cocos2d::Node *editingNode = Editor::getInstance()->getEditingNode())
            {
                update(editingNode, animation);
            }
        }

        void Sequence::update(cocos2d::Node *node, const std::string &animation)
        {
            if (NodeImDrawer *drawer = node->getComponent<NodeImDrawer>())
            {
                ImPropertyGroup *group = drawer->getNodePropertyGroup();
                auto it = group->_animations.find(animation);
                if (it != group->_animations.end())
                {
                    for (const auto &[propertyName, propertyVals] : it->second)
                    {
                        _items.push_back(Item(propertyName, propertyVals));
                    }
                }

                for (const auto &[_, group] : drawer->getComponentPropertyGroups())
                {
                    auto it = group->_animations.find(animation);
                    if (it != group->_animations.end())
                    {
                        for (const auto &[propertyName, propertyVals] : it->second)
                        {
                            _items.push_back(Item(propertyName, propertyVals));
                        }
                    }
                }
            }

            for (auto child : node->getChildren())
            {
                update(child, animation);
            }
        }

        bool AnimationButton(const char *str_id)
        {
            using namespace ImGui;
            ImGuiButtonFlags flags = ImGuiButtonFlags_None;
            float sz = GetFrameHeight();
            ImVec2 size(sz * 2, sz);

            ImGuiContext &g = *GImGui;
            ImGuiWindow *window = GetCurrentWindow();
            if (window->SkipItems)
                return false;

            const ImGuiID id = window->GetID(str_id);
            const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
            const float default_size = GetFrameHeight();
            ItemSize(size, (size.y >= default_size) ? g.Style.FramePadding.y : -1.0f);
            if (!ItemAdd(bb, id))
                return false;

            bool hovered, held;
            bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

            // Render
            const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered
                                                                                                 : ImGuiCol_Button);
            const ImU32 text_col = GetColorU32(ImGuiCol_Text);
            RenderNavHighlight(bb, id);
            RenderFrame(bb.Min, bb.Max, bg_col, true, g.Style.FrameRounding);

            if (strcmp("Play", str_id) == 0)
            {
                RenderArrow(window->DrawList, ImVec2(bb.Min.x + ImMax(0.0f, (size.x - g.FontSize) * 0.5f), bb.Min.y + ImMax(0.0f, (size.y - g.FontSize) * 0.5f)), text_col, ImGuiDir_Right);
            }
            else if (strcmp("NextF", str_id) == 0)
            {
                RenderArrow(window->DrawList, ImVec2(bb.Min.x + ImMax(0.0f, (size.x - g.FontSize) * 0.5f), bb.Min.y + ImMax(0.0f, (size.y - g.FontSize) * 0.5f)), text_col, ImGuiDir_Right);

                ImVec2 rect_pos(bb.Min.x + (size.x + g.FontSize) * 0.5f - 2.0f, bb.Min.y + size.y * 0.25f);
                window->DrawList->AddRectFilled(rect_pos, ImVec2(rect_pos.x + 2.0f, rect_pos.y + size.y * 0.5f), text_col);
            }
            else if (strcmp("PrevF", str_id) == 0)
            {
                RenderArrow(window->DrawList, ImVec2(bb.Min.x + ImMax(0.0f, (size.x - g.FontSize) * 0.5f), bb.Min.y + ImMax(0.0f, (size.y - g.FontSize) * 0.5f)), text_col, ImGuiDir_Left);

                ImVec2 rect_pos(bb.Min.x + ImMax(0.0f, (size.x - g.FontSize) * 0.5f), bb.Min.y + size.y * 0.25f);
                window->DrawList->AddRectFilled(rect_pos, ImVec2(rect_pos.x + 2.0f, rect_pos.y + size.y * 0.5f), text_col);
            }
            else if (strcmp("FirstF", str_id) == 0)
            {
                RenderArrow(window->DrawList, ImVec2(bb.Min.x + ImMax(0.0f, (size.x - g.FontSize * 2.0f) * 0.5f) + 2.0f, bb.Min.y + ImMax(0.0f, (size.y - g.FontSize) * 0.5f)), text_col, ImGuiDir_Left);
                RenderArrow(window->DrawList, ImVec2(bb.Min.x + size.x * 0.5f - 2.0f, bb.Min.y + ImMax(0.0f, (size.y - g.FontSize) * 0.5f)), text_col, ImGuiDir_Left);

                ImVec2 rect_pos(bb.Min.x + ImMax(0.0f, (size.x - g.FontSize * 2.0f) * 0.5f) + 2.0f, bb.Min.y + size.y * 0.25f);
                window->DrawList->AddRectFilled(rect_pos, ImVec2(rect_pos.x + 2.0f, rect_pos.y + size.y * 0.5f), text_col);
            }
            else if (strcmp("LastF", str_id) == 0)
            {
                RenderArrow(window->DrawList, ImVec2(bb.Min.x + ImMax(0.0f, (size.x - g.FontSize * 2.0f) * 0.5f) + 2.0f, bb.Min.y + ImMax(0.0f, (size.y - g.FontSize) * 0.5f)), text_col, ImGuiDir_Right);
                RenderArrow(window->DrawList, ImVec2(bb.Min.x + size.x * 0.5f - 2.0f, bb.Min.y + ImMax(0.0f, (size.y - g.FontSize) * 0.5f)), text_col, ImGuiDir_Right);

                ImVec2 rect_pos(bb.Min.x + (size.x + g.FontSize * 2.0f) * 0.5f - 4.0f, bb.Min.y + size.y * 0.25f);
                window->DrawList->AddRectFilled(rect_pos, ImVec2(rect_pos.x + 2.0f, rect_pos.y + size.y * 0.5f), text_col);
            }
            else if (strcmp("Stop", str_id) == 0)
            {
                ImVec2 rect_pos(bb.Min.x + ImMax(0.0f, (size.x - g.FontSize) * 0.5f), bb.Min.y + size.y * 0.25f);
                window->DrawList->AddRectFilled(rect_pos, ImVec2(rect_pos.x + g.FontSize, rect_pos.y + size.y * 0.5f), text_col);
            }

            IMGUI_TEST_ENGINE_ITEM_INFO(id, str_id, g.LastItemData.StatusFlags);
            return pressed;
        }
    }

    void Animation::draw(bool *open)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(getWindowName().c_str(), open))
        {

            ImGui::SameLine();
            static std::string text = "Test";
            //    
            //    ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);

            
static bool fit = true;
            if (_renaming)
            {
                if(fit)
                {
                    fit = false;
                    ImGui::SetKeyboardFocusHere();
                 ImGui::InputText("###ssss", &text, ImGuiInputTextFlags_AutoSelectAll);
                }
                else
                {
                 ImGui::InputText("###ssss", &text);
                 if (!ImGui::IsItemActive())
                 {
                     _renaming = false;
                 }
                }
                 
                
                // ImGui::SetItemDefaultFocus();
                 

            }
            else
            {
                if (ImGui::BeginCombo("###sss", "Test"))
                {
                    auto isSelected = true;
                    if (ImGui::Selectable("Test", isSelected))
                    {
                    }

                    ImGui::Separator();
                    if (ImGui::Selectable("New Animation", false))
                    {
                    }

                    if (ImGui::Selectable("Delete Animation", false))
                    {
                    }

                    if (ImGui::Selectable("Rename Animation", false))
                    {
                        _renaming = true;
                    }
                    ImGui::EndCombo();
                }
            }
            
            // ImGui::SameLine();
            // if (ImGui::Button("-"))
            // {
            //     _playing = false;
            // }

            // ImGui::SameLine();
            // if (ImGui::Button("+"))
            // {
            //     _playing = false;
            // }

            // ImGui::SameLine();
            // if (ImGui::Button("Rename"))
            // {
            //     _playing = false;
            // }

            float spacing = 0.0f;
            ImGui::SameLine();
            if (Internal::AnimationButton("FirstF"))
            {
                _currentFrame = _sequence.GetFrameMin();
            }

            ImGui::SameLine(0.0f, spacing);
            if (Internal::AnimationButton("PrevF"))
            {
                _currentFrame--;
            }

            ImGui::SameLine(0.0f, spacing);

            if (!_playing && Internal::AnimationButton("Play"))
            {
                _playing = true;
                _elapsed = 0.0f;
            }
            else if (_playing && Internal::AnimationButton("Stop"))
            {
                _playing = false;
            }

            ImGui::SameLine(0.0f, spacing);
            if (Internal::AnimationButton("NextF"))
            {
                _currentFrame++;
            }

            ImGui::SameLine(0.0f, spacing);
            if (Internal::AnimationButton("LastF"))
            {
                _currentFrame = _sequence.GetFrameMax();
            }

            ImGui::SameLine();
            if (ImGui::RadioButton("Rec", _recording))
            {
                _recording = !_recording;
            }

            ImGui::SameLine();

            ImGui::SetNextItemWidth(100.0f);
            PropertyImDrawer<WrapMode>::draw("###WrapMode", _wrapMode);
            ImGui::SameLine();
            int v = 25;
            ImGui::SetNextItemWidth(100.0f);
            ImGui::InputInt("Samples", &v);

            //     ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
            //        ImGui::SameLine();
            // if (ImGui::ArrowButton("##left", ImGuiDir_Left)) { v--; }
            // ImGui::SameLine();

            // ImGui::Text("%d", v);
            // if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { v++; }
            // ImGui::PopItemFlag();
            if (_playing)
            {
                if (_wrapMode == WrapMode::Reverse || _wrapMode == WrapMode::ReverseLoop)
                    _currentFrame--;
                else
                    _currentFrame++;
            }

            if (_currentFrame < 0)
            {
                if (_wrapMode == WrapMode::Reverse)
                    _currentFrame = 0;
                else
                    _currentFrame += _sequence.GetFrameMax();
            }
            else if (_currentFrame > _sequence.GetFrameMax())
            {
                if (_wrapMode == WrapMode::Loop)
                    _currentFrame -= _sequence.GetFrameMax();
                else
                    _currentFrame = _sequence.GetFrameMax();
            }

            static int selectedEntry = -1;
            static int firstFrame = 0;
            static bool expanded = true;

            _sequence.prepare("Test");
            ImSequencer::Sequencer(&_sequence, &_currentFrame, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_ALL);

            if (cocos2d::Node *editingNode = Editor::getInstance()->getEditingNode())
            {
                if (NodeImDrawer *drawer = editingNode->getComponent<NodeImDrawer>())
                {
                    drawer->getNodePropertyGroup()->play("Test", _currentFrame);
                    for (const auto &[_, group] : drawer->getComponentPropertyGroups())
                    {
                        group->play("Test", _currentFrame);
                    }
                }
            }
        }

        ImGui::End();
    }

    void Animation::update(float dt)
    {
        if (!_playing)
            return;
    }
}
