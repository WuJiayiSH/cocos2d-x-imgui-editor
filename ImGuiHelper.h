#ifndef __CCIMEDITOR_IMGUIHELPER_H__
#define __CCIMEDITOR_IMGUIHELPER_H__

namespace CCImEditor
{
    class ImGuiHelper
    {
    public:
        static bool BeginNestedMenu(const char* name);
        static void EndNestedMenu();
    };
}

#endif
