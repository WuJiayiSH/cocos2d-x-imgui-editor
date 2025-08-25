#include "Animation.h"

#include "Editor.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "ImSequencer.h"
#include "cocos2d.h"
#include "PropertyImDrawer.h"
#include "CommandHistory.h"

namespace CCImEditor
{
    namespace Internal
    {
        namespace Animation
        {
            void Sequence::performRecursively(cocos2d::Node *node, std::function<void(ImPropertyGroup*)> func)
            {
                if (NodeImDrawer *drawer = node->getComponent<NodeImDrawer>())
                {
                    ImPropertyGroup *group = drawer->getNodePropertyGroup();
                    func(group);

                    for (const auto &[_, group] : drawer->getComponentPropertyGroups())
                    {
                        func(group);
                    }
                }

                for (auto child : node->getChildren())
                {
                    performRecursively(child, func);
                }
            }

            void Sequence::update(ImPropertyGroup* group, const std::string &animation)
            {
                for (const auto &[animationName, properties] : group->_animations)
                {
                    _itemExists[animationName] = true;
                    if (animationName == animation)
                    {
                        for (const auto &[propertyName, propertyValues] : properties)
                        {
                            _items.push_back(Item(propertyName, propertyValues));
                        }
                    }
                }
            }

            void Sequence::record(ImPropertyGroup* group, std::string* animation, int frame)
            {
                group->_animation = animation;
                group->_frame = frame;
            }

            void Sequence::play(ImPropertyGroup* group, const std::string &animation, int frame)
            {
                group->play(animation, frame);
            }

            void Sequence::CustomDrawCompact(int index, ImDrawList *draw_list, const ImRect &rc, const ImRect &clippingRect)
            {
                draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
                const auto &item = _items[index];
                for (const auto &[i, value] : item._values)
                {
                    float r = (i - item._frameStart) / float(item._frameEnd - item._frameStart);
                    float x = ImLerp(rc.Min.x, rc.Max.x, r);
                    draw_list->AddLine(ImVec2(x, rc.Min.y + 6), ImVec2(x, rc.Max.y - 4), 0xAA000000, 4.f);
                }
                draw_list->PopClipRect();
            }

            bool Button(const char *str_id)
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
    }

    void Animation::draw(bool *open)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(getWindowName().c_str(), open))
        {
            _sequence._itemExists.clear();
            _sequence._items.clear();
            if (cocos2d::Node *editingNode = Editor::getInstance()->getEditingNode())
            {
                _sequence.performRecursively(editingNode, std::bind(&Internal::Animation::Sequence::update, &_sequence, std::placeholders::_1, _animation));
            }

            ImGui::SetNextItemWidth(200.0f);
            if (_animation.empty())
            {
                for (const auto &[name, exists] : _sequence._itemExists)
                {
                    if (exists)
                    {
                        _animation = name;
                        break;
                    }
                }
            }

            if (_animation.empty())
                _animation = "unnamed";

            if (ImGui::BeginCombo("###Animations", _animation.c_str()))
            {
                for (const auto &[name, exists] : _sequence._itemExists)
                {
                    if (exists && ImGui::Selectable(name.c_str(), name == _animation))
                    {
                        _animation = name;
                    }
                }

                ImGui::Separator();
                if (ImGui::Selectable("New Animation", false))
                {
                    _animation = "unnamed";
                    int count = 1;
                    while (_sequence._itemExists[_animation])
                        _animation = cocos2d::StringUtils::format("unnamed (%d)", count++);
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            if (Internal::Animation::Button("FirstF"))
            {
                _currentFrame = static_cast<float>(_sequence.GetFrameMin());
            }

            ImGui::SameLine(0.0f, 0.0f);
            if (Internal::Animation::Button("PrevF"))
            {
                _currentFrame--;
            }

            ImGui::SameLine(0.0f, 0.0f);
            if (_state != State::Playing && Internal::Animation::Button("Play"))
            {
                _state = State::Playing;
            }
            else if (_state == State::Playing && Internal::Animation::Button("Stop"))
            {
                _state = State::Idle;
            }

            ImGui::SameLine(0.0f, 0.0f);
            if (Internal::Animation::Button("NextF"))
            {
                _currentFrame++;
            }

            ImGui::SameLine(0.0f, 0.0f);
            if (Internal::Animation::Button("LastF"))
            {
                _currentFrame = static_cast<float>(_sequence.GetFrameMax());
            }

            ImGui::SameLine();
            bool recording = _state == State::Recording;
            if (ImGui::RadioButton("Rec", recording))
            {
                _state = recording ? State::Recording : State::Idle;
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            PropertyImDrawer<WrapMode>::draw("###WrapMode", _wrapMode);
            ImGui::SameLine();
            
            ImGui::SetNextItemWidth(100.0f);
            int samples = _samples;
            if (ImGui::InputInt("Samples", &samples))
            {
                _samples = (uint16_t)samples;
            }

            if (_state == State::Playing)
            {
                float frameRate = cocos2d::Director::getInstance()->getFrameRate();
                float _elapsedFrames = 1.0f / frameRate * _samples;
                if (_wrapMode == WrapMode::Reverse || _wrapMode == WrapMode::ReverseLoop)
                    _currentFrame -= _elapsedFrames;
                else
                    _currentFrame += _elapsedFrames;
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
                    _currentFrame = static_cast<float>(_sequence.GetFrameMax());
            }

            int currentFrame = static_cast<int>(_currentFrame);
            ImSequencer::Sequencer(&_sequence, &currentFrame, &_expanded, &_selectedEntry, &_firstFrame, ImSequencer::SEQUENCER_EDIT_ALL);
            _currentFrame = _currentFrame + (currentFrame - static_cast<int>(_currentFrame));

            if (cocos2d::Node *editingNode = Editor::getInstance()->getEditingNode())
            {
                if (_state == State::Recording)
                {
                    _sequence.performRecursively(editingNode, std::bind(&Internal::Animation::Sequence::record, &_sequence, std::placeholders::_1, &_animation, currentFrame));
                }
                else
                {
                    _sequence.performRecursively(editingNode, std::bind(&Internal::Animation::Sequence::play, &_sequence, std::placeholders::_1, _animation, currentFrame));
                }
            }
        }

        ImGui::End();
    }
}
