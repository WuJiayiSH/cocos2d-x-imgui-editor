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
                std::vector<SequenceItem> _items;
                int _frameMax;

                int GetFrameMin() const
                {
                    return 0;
                }

                int GetFrameMax() const
                {
                    return _frameMax;
                }

                int GetItemCount() const override
                {
                    return _items.size();
                }

                const char *GetItemLabel(int i) const override
                {
                    return _items[i]._label.c_str();
                }

                void Get(int index, int **start, int **end, int *type, unsigned int *color) override
                {
                    auto &item = _items[index];
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

        // Sequence and arguments
        Internal::Animation::Sequence _sequence;
        bool _expanded = true;
        int _selectedEntry = -1;
        int _firstFrame = 0;
    };
}

#endif
