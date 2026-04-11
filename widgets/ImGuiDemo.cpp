#include "ImGuiDemo.h"
#include "WidgetFactory.h"
#include "imgui.h"

namespace CCImEditor
{
    void ImGuiDemo::draw(bool* open)
    {
        ImGui::ShowDemoWindow(open);
    }
}
