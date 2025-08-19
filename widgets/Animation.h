#ifndef __CCIMEDITOR_ANIMATION_H__
#define __CCIMEDITOR_ANIMATION_H__

#include "Widget.h"
#include "imgui/imgui.h"
#include "ImSequencer.h"

namespace CCImEditor
{
    namespace Internal
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
            std::unordered_map<std::string, bool> _animationExists;
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

            void BeginEdit(int index) override
            {
                CCLOG("BeginEdit");
            }

            void EndEdit() override
            {
                CCLOG("EndEdit");
            }

            void prepare(const std::string &animation);

            void update(cocos2d::Node *node, const std::string &animation);
            void rename(cocos2d::Node *node, const std::string &animation, const std::string &newAniamtionName);
            void remove(cocos2d::Node *node, const std::string &animation);
            void record(cocos2d::Node *node, const std::string &animation, int frame);
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
        };

    }

    class Animation: public Widget
    {
    private:
        void draw(bool* open) override;

        void update(float) override;

        enum class WrapMode
        {
            Normal,
            Loop,
            Reverse,
            ReverseLoop,
        };

        bool _playing = false;
        WrapMode _wrapMode = WrapMode::Loop;
        float _elapsed;
        int _currentFrame = 0;
        uint16_t _sample = 30;

        Internal::Sequence _sequence;
        bool _recording = false;
        enum RenameState
        {
            NotActivated,
            Editing,
            Activated,
        };
        RenameState _renameState = RenameState::NotActivated;

        std::string _animation;
        std::string _newAnimationName;
    };
}

#endif
