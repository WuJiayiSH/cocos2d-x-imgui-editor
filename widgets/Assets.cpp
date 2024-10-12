#include "Assets.h"
#include "imgui/imgui.h"

namespace CCImEditor
{
    void Assets::draw(bool* open)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(getWindowName().c_str(), open))
        {
            if (CCImEditor::Internal::fileBrowser("Assets", _fileInfo, 0))
            {
                if ((_fileInfo.getDirtyMask() & CCImEditor::Internal::FileInfo::FILE_DOUBLE_CLICKED) > 0)
                {
                    std::string f = "file://";
                    f += _fileInfo.getDirectory();
                    f += _fileInfo.getFilename();
                    cocos2d::Application::getInstance()->openURL(f);
                }
            }
        }

        ImGui::End();
    }
}
