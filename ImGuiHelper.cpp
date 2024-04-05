#include "ImGuiHelper.h"
#include "imgui/imgui.h"
#include <string>

namespace CCImEditor
{
    namespace
    {
        static size_t s_menuLevel = 0;
    }

    bool ImGuiHelper::BeginNestedMenu(const char* name)
    {
        IM_ASSERT(s_menuLevel == 0);

        const char* start = name;
        const char* end = strchr(start, '/');

        bool isMenuOpen = true;
        while (end && isMenuOpen)
        {
            std::string menuName(start, end - start);
            if (ImGui::BeginMenu(menuName.c_str()))
            {
                s_menuLevel++;
                start = end + 1;
                end = strchr(start, '/');
            }
            else
            {
                isMenuOpen = false;
            }
        }

        if (isMenuOpen && strlen(start) != 0)
        {
            if (ImGui::BeginMenu(start))
            {
                s_menuLevel++;
            }
            else
            {
                isMenuOpen = false;
            }
        }

        
        if (isMenuOpen)
        {
            return true;
        }
        else
        {
            ImGuiHelper::EndNestedMenu();
            return false;
        }
    }

    void ImGuiHelper::EndNestedMenu()
    {
        while (s_menuLevel > 0)
        {
            ImGui::EndMenu();
            s_menuLevel--;
        }
    }
}
