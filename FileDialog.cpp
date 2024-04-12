#include "FileDialog.h"
#include "imgui/imgui.h"
#include "cocos2d.h"

namespace CCImEditor {
namespace Internal {
    namespace
    {
        std::string s_selectedDirectory;

        std::string s_selectedFile;

        char s_filenameBuf[260];

        std::unordered_map<std::string, std::vector<std::string>> s_fileListCache;

        const std::vector<std::string>& getOrCreateFileListFromCache(const std::string& directory)
        {
            std::unordered_map<std::string, std::vector<std::string>>::iterator it = s_fileListCache.find(directory);
            if (it != s_fileListCache.end())
            {
                return it->second;
            }
            else
            {
                std::vector<std::string>& result = s_fileListCache[directory];
                result = cocos2d::FileUtils::getInstance()->listFiles(directory);
                return result;
            }
        }

        void listDirectories(const std::vector<std::string>& directories)
        {
            cocos2d::FileUtils* fileUtils = cocos2d::FileUtils::getInstance();
            for (size_t i = 0; i < directories.size(); i++)
            {
                const std::string& directory = directories[i];
                if (!fileUtils->isDirectoryExist(directory))
                    continue;
             
                size_t size = directory.size();
                size_t pos = directory.find_last_of('/', size - 2);
                pos = (pos == std::string::npos) ? 0 : pos + 1;
                std::string dirname = directory.substr(pos, size - 1 - pos);
                if (dirname == "." || dirname == "..")
                    continue;

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
                if (s_selectedDirectory == directory)
                {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                const std::vector<std::string>& subDirectories = getOrCreateFileListFromCache(directory);
                const bool hasSubDirectories = subDirectories.size() > 2; // ignore . and ..
                if (!hasSubDirectories)
                {
                    flags |= ImGuiTreeNodeFlags_Leaf;
                    flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }
                
                bool open = ImGui::TreeNodeEx(
                    dirname.c_str(),
                    flags,
                    "%s",
                    dirname.c_str()
                );

                if (ImGui::IsItemClicked())
                    s_selectedDirectory = directory;

                if (open && hasSubDirectories)
                {
                    listDirectories(subDirectories);
                    ImGui::TreePop();
                }
            }
        }
    } // anonymous namespace

    bool saveFile(std::string& outFile)
    {
        if (!ImGui::IsPopupOpen("Save As"))
            ImGui::OpenPopup("Save As");

        ImGui::SetNextWindowSize(ImVec2(640, 360), ImGuiCond_FirstUseEver);

        bool openPopup = true;
        if (ImGui::BeginPopupModal("Save As", &openPopup))
        {
            // Search paths
            cocos2d::FileUtils* fileUtils = cocos2d::FileUtils::getInstance();
            const std::vector<std::string>& searchPaths = fileUtils->getSearchPaths();

            IM_ASSERT(searchPaths.size() > 0);
            if (s_selectedDirectory.empty())
                s_selectedDirectory = searchPaths[0];

            const float footerHeight = ImGui::GetFrameHeightWithSpacing(); // height of filename input and buttons
            if (ImGui::BeginTable("Left", 1, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody, ImVec2(150, -footerHeight)))
            {
                ImGui::TableSetupColumn("Search Path", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                listDirectories(searchPaths);
                ImGui::EndTable();
            }

            ImGui::SameLine();
            if (ImGui::BeginTable("Right", 1, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody, ImVec2(0, -footerHeight)))
            {
                ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                const std::vector<std::string>& files = getOrCreateFileListFromCache(s_selectedDirectory);
                for (const std::string& file: files)
                {
                    if (fileUtils->isFileExist(file))
                    {
                        size_t lastSlash = file.find_last_of('/');
                        const char* filename = file.c_str();
                        if (lastSlash != std::string::npos)
                        {
                            filename = filename + lastSlash + 1;
                        }
                        
                        if (ImGui::Selectable(filename, s_selectedFile == file))
                        {
                            s_selectedFile = file;
                            IM_ASSERT(strlen(filename) < IM_ARRAYSIZE(s_filenameBuf));
                            strcpy(s_filenameBuf, filename);
                        }
                    }
                }

                ImGui::EndTable();
            }

            ImGui::InputText("##filename", s_filenameBuf, IM_ARRAYSIZE(s_filenameBuf));
            ImGui::SameLine();
            ImVec2 buttonSize(ImGui::GetFontSize() * 7.0f, 0.0f);
            bool outFileChanged = false;
            if (ImGui::Button("Save", buttonSize))
            {
                std::string file = s_selectedDirectory + s_filenameBuf;
                if (fileUtils->isFileExist(file))
                {
                    ImGui::OpenPopup("Confrom");
                }
                else
                {
                    outFile = std::move(file);
                    outFileChanged = true;
                }
            }

            bool openConfirm = true;
            if (ImGui::BeginPopupModal("Confrom", &openConfirm))
            {
                ImGui::Text("File already exists. \nDo you want to replace it?");
                if (ImGui::Button("Yes", buttonSize))
                {
                    outFile = s_selectedDirectory + s_filenameBuf;
                    outFileChanged = true;
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::SameLine();
                if (ImGui::Button("No", buttonSize))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel", buttonSize))
            {
                outFile.clear();
                outFileChanged = true;
            }

            if (outFileChanged)
            {
                s_fileListCache.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
            return outFileChanged;
        }
        else
        {
            s_fileListCache.clear();
            outFile.clear();
            return true;
        }
    }
}
}
