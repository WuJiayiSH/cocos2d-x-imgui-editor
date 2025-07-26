#ifndef __CCIMEDITOR_CONSOLE_H__
#define __CCIMEDITOR_CONSOLE_H__

#include "Widget.h"
#include "imgui/imgui.h"

namespace CCImEditor
{
    class Console: public Widget
    {
    public:
        Console() {};
        ~Console() override;
        void draw(bool* open) override;

    protected:
        bool init(const std::string& name, const std::string& windowName, uint32_t mask) override;

    private:
        bool _autoScroll = true;  // Keep scrolling if already at the bottom
        ImGuiTextFilter _filter; // Text filter
        std::string _content;
        FILE *_file;
    };
}

#endif
