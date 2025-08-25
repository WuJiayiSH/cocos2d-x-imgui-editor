#ifndef __CCIMEDITOR_ANIMATION_H__
#define __CCIMEDITOR_ANIMATION_H__

#include "Widget.h"
#include "imgui/imgui.h"
#include "ImSequencer.h"
#include "NodeImDrawer.h"

namespace CCImEditor
{
    namespace Internal
    {
        namespace Animation
        {
            struct Sequence : public ImSequencer::SequenceInterface
            {
                struct Item
                {
                    Item(std::string name, const std::map<int, cocos2d::Value> &values)
                        : _name(name), _values(values)
                    {
                        _frameStart = _values.begin()->first;
                        _frameEnd = std::prev(_values.end())->first;
                    }

                    std::string _name;
                    const std::map<int, cocos2d::Value> &_values;
                    int _frameStart;
                    int _frameEnd;
                };

                std::vector<Item> _items;
                std::unordered_map<std::string, bool> _itemExists;

                void performRecursively(cocos2d::Node *node, std::function<void(ImPropertyGroup*)> func);
                void update(ImPropertyGroup* group, const std::string& animation);
                void record(ImPropertyGroup* group, std::string* animation, int frame);
                void play(ImPropertyGroup* group, const std::string& animation, int frame);

                int GetFrameMin() const
                {
                    return 0;
                }

                int GetFrameMax() const
                {
                    int frameMax = 0;
                    for (const Item &item : _items)
                        if (item._frameEnd > frameMax)
                            frameMax = item._frameEnd;
                    return frameMax;
                }


                int GetItemCount() const override
                {
                    return _items.size();
                }

                const char *GetItemLabel(int i) const override
                {
                    return _items[i]._name.c_str();
                }

                void Get(int index, int **start, int **end, int *type, unsigned int *color) override
                {
                    Item &item = _items[index];
                    if (start)
                        *start = &item._frameStart;

                    if (end)
                        *end = &item._frameEnd;

                    if (type)
                        *type = 0;

                    if (color)
                        *color = 0xFFAA8080;
                }

                void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect) override;
            };
        }
    }

    class Animation: public Widget
    {
    private:
        void draw(bool* open) override;

        std::string _animation;

        enum class State
        {
            Idle,
            Playing,
            Recording,
        };
        State _state = State::Idle;

        enum class WrapMode
        {
            Normal,
            Loop,
            Reverse,
            ReverseLoop,
        };
        WrapMode _wrapMode = WrapMode::Loop;
        uint16_t _samples = 30;

        // Sequence and arguments
        Internal::Animation::Sequence _sequence;
        float _currentFrame = 0.0f;
        bool _expanded = true;
        int _selectedEntry = -1;
        int _firstFrame = 0;
    };
}

#endif
