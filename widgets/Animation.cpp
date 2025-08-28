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
        cocos2d::Node *editingNode = Editor::getInstance()->getEditingNode();

        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(getWindowName().c_str(), open) && editingNode)
        {
            NodeImDrawer *drawer = editingNode->getComponent<NodeImDrawer>();

            std::unordered_map<std::string, bool> animationExists = drawer->getAnimationNames();

            std::string animation = drawer->_animationName;

            ImGui::SetNextItemWidth(250.0f);
            if (animationExists.size() > 0 || !animation.empty())
            {
                if (ImGui::BeginCombo("###Animations", animation.empty() ? "Select an Animation to edit" : animation.c_str()))
                {
                    for (const auto &[name, exists] : animationExists)
                    {
                        if (exists && ImGui::Selectable(name.c_str(), name == animation))
                        {
                            animation = name;
                        }
                    }

                    ImGui::Separator();
                    if (ImGui::Selectable("New Animation", false))
                    {
                        animation = "unnamed";
                        int count = 1;
                        while (animationExists[animation])
                            animation = cocos2d::StringUtils::format("unnamed (%d)", count++);
                    }

                    if (!animation.empty())
                    {
                        ImGui::Separator();
                        if (ImGui::Selectable("Exit", false))
                        {
                            animation = "";
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            else
            {
                if (ImGui::Button("Create a new Animation", ImVec2(250.0f, 0.0f)))
                    animation = "unnamed";
            }

            if (!animation.empty())
            {
                auto items = drawer->getAnimationSequenceItems(animation);
                int currentFrame = drawer->_currentFrame;
                int maxFrame = 0;
                for (const auto &item : items)
                {
                    int end = std::prev(item._values.end())->first;
                    if (end > maxFrame)
                        maxFrame = end;
                }
                if (maxFrame == 0)
                {
                    maxFrame = drawer->_sample;
                }

                const int minFrame = 0;
                auto state = drawer->_animationState;
                auto wrapMode = drawer->_animationWrapMode;
                int sample = drawer->_sample;
                using State = Internal::Animation::State;
                ImGui::SetNextItemWidth(200.0f);
                ImGui::SameLine();
                if (Internal::Animation::Button("FirstF"))
                {
                    currentFrame = minFrame;
                    if (state == State::Playing)
                        state = State::Unset;
                }

                ImGui::SameLine(0.0f, 0.0f);
                if (Internal::Animation::Button("PrevF"))
                {
                    currentFrame--;
                    if (state == State::Playing)
                        state = State::Unset;
                }

                ImGui::SameLine(0.0f, 0.0f);
                if (state != State::Playing && Internal::Animation::Button("Play"))
                {
                    state = State::Playing;
                }
                else if (state == State::Playing && Internal::Animation::Button("Stop"))
                {
                    state = State::Unset;
                }

                ImGui::SameLine(0.0f, 0.0f);
                if (Internal::Animation::Button("NextF"))
                {
                    currentFrame++;
                    if (state == State::Playing)
                        state = State::Unset;
                }

                ImGui::SameLine(0.0f, 0.0f);
                if (Internal::Animation::Button("LastF"))
                {
                    currentFrame = maxFrame;
                    if (state == State::Playing)
                        state = State::Unset;
                }

                ImGui::SameLine();
                bool recording = state == State::Recording;
                if (ImGui::RadioButton("Rec", recording))
                {
                    state = recording ? State::Unset : State::Recording;
                }

                ImGui::SameLine();
                ImGui::SetNextItemWidth(100.0f);
                PropertyImDrawer<AnimationWrapMode>::draw("###WrapMode", wrapMode);

                ImGui::SameLine();
                ImGui::SetNextItemWidth(100.0f);
                ImGui::DragInt("Samples", &sample, 1.0f, 1, 100);

                ImGui::SameLine();
                ImGui::SetNextItemWidth(100.0f);
                ImGui::DragInt("Duration(Frame)", &maxFrame, 1.0f, 1, 100);

                _sequence._items = std::move(items);
                _sequence._frameMax = maxFrame;
                ImSequencer::Sequencer(&_sequence, &currentFrame, &_expanded, &_selectedEntry, &_firstFrame, ImSequencer::SEQUENCER_EDIT_ALL);
                drawer->applyAnimationRecursively(state, animation, currentFrame, wrapMode, (uint16_t)(sample));
            }
        }

        ImGui::End();
    }
}
