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
            _animationExists.clear();
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
                for (const auto&[animationName, animationValue] : group->_animations)
                {
                    _animationExists[animationName] = true;
                    if (animationName == animation)
                    {
                        for (const auto &[propertyName, propertyVals] : animationValue)
                        {
                            _items.push_back(Item(propertyName, propertyVals));
                        }
                    }
                }

                for (const auto &[_, group] : drawer->getComponentPropertyGroups())
                {
                    for (const auto&[animationName, animationValue] : group->_animations)
                    {
                        _animationExists[animationName] = true;
                        if (animationName == animation)
                        {
                            for (const auto &[propertyName, propertyVals] : animationValue)
                            {
                                _items.push_back(Item(propertyName, propertyVals));
                            }
                        }
                    }
                }
            }

            for (auto child : node->getChildren())
            {
                update(child, animation);
            }
        }

        void Sequence::remove(cocos2d::Node *node, const std::string &animation)
        {
            if (NodeImDrawer *drawer = node->getComponent<NodeImDrawer>())
            {
                ImPropertyGroup *group = drawer->getNodePropertyGroup();
                group->_animations.erase(animation);
                

                for (const auto &[_, group] : drawer->getComponentPropertyGroups())
                {
                    group->_animations.erase(animation);
                }
            }

            for (auto child : node->getChildren())
            {
                remove(child, animation);
            }
        }

        void Sequence::rename(cocos2d::Node *node, const std::string &animation, const std::string &newAniamtionName)
        {
            if (NodeImDrawer *drawer = node->getComponent<NodeImDrawer>())
            {
                ImPropertyGroup *group = drawer->getNodePropertyGroup();
                auto node = group->_animations.extract(animation);
                if (!node.empty())
                {
                    node.key() = newAniamtionName;
                    group->_animations.insert(std::move(node));
                }
                

                for (const auto &[_, group] : drawer->getComponentPropertyGroups())
                {
                    auto node = group->_animations.extract(animation);
                    if (!node.empty())
                    {
                        node.key() = newAniamtionName;
                        group->_animations.insert(std::move(node));
                    }
                }
            }

            for (auto child : node->getChildren())
            {
                rename(child, animation, newAniamtionName);
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
            //static std::string text = "Test";
            //    
            //    ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            if (_renameState == RenameState::NotActivated)
            {
                if (_animation.empty())
                {
                    for (const auto&[animationName, exists] : _sequence._animationExists)
                    {
                        if (exists)
                        {
                            _animation = animationName;
                            break;
                        }
                    }
                }

                if (_animation.empty())
                    _animation = "unnamed";

                if (ImGui::BeginCombo("###AnimationName", _animation.c_str()))
                {
                    for (const auto&[animationName, _] : _sequence._animationExists)
                    {
                        if (ImGui::Selectable(animationName.c_str(), animationName == _animation))
                        {
                            _animation = animationName;
                        }
                    }

                    ImGui::Separator();
                    if (ImGui::Selectable("New Animation", false))
                    {
                        _animation = "unnamed";
                        int count = 1;
                        while (_sequence._animationExists[_animation])
                        {
                            _animation = cocos2d::StringUtils::format("unnamed (%d)", count++);
                        }
                    }

                    if (ImGui::Selectable("Delete Animation", false))
                    {
                        _sequence._animationExists[_animation] = false;
                        if (cocos2d::Node *editingNode = Editor::getInstance()->getEditingNode())
                        {
                            _sequence.remove(editingNode, _animation);
                        }

                        _animation = "unnamed";
                        for (const auto&[animationName, exists] : _sequence._animationExists)
                        {
                            if (exists)
                            {
                                _animation = animationName;
                                break;
                            }
                        }
                    }

                    if (ImGui::Selectable("Rename Animation", false))
                    {
                        _renameState = RenameState::Activated;
                    }

                    ImGui::EndCombo();
                }
            }
            else if(_renameState == RenameState::Activated)
            {
                ImGui::SetKeyboardFocusHere();
                _newAnimationName = _animation;
                ImGui::InputText("###AnimationName", &_newAnimationName, ImGuiInputTextFlags_AutoSelectAll);
                _renameState = RenameState::Editing;
            }
            else if(_renameState == RenameState::Editing)
            {
                ImGui::InputText("###AnimationName", &_newAnimationName);
                if (!ImGui::IsItemActive())
                {
                    _renameState = RenameState::NotActivated;
                    if (_newAnimationName != _animation)
                    {
                        if (cocos2d::Node *editingNode = Editor::getInstance()->getEditingNode())
                        {
                            _sequence.rename(editingNode, _animation, _newAnimationName);
                        }

                        _animation = _newAnimationName;
                    }
                }
            }
            
            ImGui::SameLine();
            if (Internal::AnimationButton("FirstF"))
            {
                _currentFrame = _sequence.GetFrameMin();
            }

            ImGui::SameLine(0.0f, 0.0f);
            if (Internal::AnimationButton("PrevF"))
            {
                _currentFrame--;
            }

            ImGui::SameLine(0.0f, 0.0f);

            if (!_playing && Internal::AnimationButton("Play"))
            {
                _playing = true;
                _elapsed = 0.0f;
            }
            else if (_playing && Internal::AnimationButton("Stop"))
            {
                _playing = false;
            }

            ImGui::SameLine(0.0f, 0.0f);
            if (Internal::AnimationButton("NextF"))
            {
                _currentFrame++;
            }

            ImGui::SameLine(0.0f, 0.0f);
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

            _sequence.prepare(_animation);
            ImSequencer::Sequencer(&_sequence, &_currentFrame, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_ALL);

            if (cocos2d::Node *editingNode = Editor::getInstance()->getEditingNode())
            {
                if (NodeImDrawer *drawer = editingNode->getComponent<NodeImDrawer>())
                {
                    drawer->getNodePropertyGroup()->play(_animation, _currentFrame);
                    for (const auto &[_, group] : drawer->getComponentPropertyGroups())
                    {
                        group->play(_animation, _currentFrame);
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
