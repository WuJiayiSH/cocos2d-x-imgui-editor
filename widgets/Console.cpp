#include "Console.h"
#include "cocos2d.h"

namespace CCImEditor
{
    bool Console::init(const std::string& name, const std::string& windowName, uint32_t mask)
    {
        if (!Widget::init(name, windowName, mask))
            return false;

        cocos2d::FileUtils* fileUtil = cocos2d::FileUtils::getInstance();

        std::string editorLog = fileUtil->getWritablePath() + "cc_imgui_editor/editor.log";
        std::string suitableFullPath = fileUtil->getSuitableFOpen(editorLog);
        _file = fopen(suitableFullPath.c_str(), "rb");
        if (!_file)
            return false;

        return true;
    }

    Console::~Console()
    {
        if (_file)
            fclose(_file);
    }

    void Console::draw(bool* open)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(getWindowName().c_str(), open))
        {
            if (ImGui::Button("Clear"))
            {
                _content.clear();
            }

            ImGui::SameLine();
            if (ImGui::Button("Open Log"))
            {
                std::string f = "file://";
                f += cocos2d::FileUtils::getInstance()->getWritablePath() + "cc_imgui_editor/editor.log";
                cocos2d::Application::getInstance()->openURL(f);
            }

            ImGui::SameLine();
            _filter.Draw("Filter", -175.0f);

            ImGui::SameLine();
            ImGui::Checkbox("Auto-scroll", &_autoScroll);

            ImGui::Separator();

            // Child window for scrolling
            if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
            {
                char buffer[200];
                size_t size = fread(buffer, 1, 200, _file);
                if (size > 0)
                {
                    _content.append(buffer, size);
                }

                // Display filtered content line by line
                size_t start = 0;
                while (start < _content.length())
                {
                    size_t end = _content.find('\n', start);
                    if (end == std::string::npos)
                        end = _content.length();

                    std::string line = _content.substr(start, end - start);
                    if (_filter.PassFilter(line.c_str()))
                        ImGui::TextUnformatted(line.c_str());

                    start = end + 1;
                }

                // Auto-scroll
                if (_autoScroll && size > 0)
                    ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();
        }

        ImGui::End();
    }
}
