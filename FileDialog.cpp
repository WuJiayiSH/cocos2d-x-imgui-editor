#include "FileDialog.h"
#include "imgui/imgui.h"
#include "cocos2d.h"

namespace CCImEditor {
namespace Internal {
    namespace
    {
        FileInfo s_fileInfo;

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

        const char* getFilename(const std::string& file)
        {
            size_t lastSlash = file.find_last_of('/');
            const char* filename = file.c_str();
            if (lastSlash != std::string::npos)
            {
                filename = filename + lastSlash + 1;
            }

            return filename;
        }
    } // anonymous namespace

    bool fileDialog(FileDialogType type, std::string& outFile)
    {
        const char* windowName;
        const char* actionName;
        switch(type)
        {
            case FileDialogType::SAVE:
                windowName = "Save As";
                actionName = "Save";
                break;
            default:
                windowName = "Open File";
                actionName = "Open";
                break;
        }

        if (!ImGui::IsPopupOpen(windowName))
            ImGui::OpenPopup(windowName);

        ImGui::SetNextWindowSize(ImVec2(640, 360), ImGuiCond_FirstUseEver);

        if (ImGui::BeginPopupModal(windowName))
        {
            const float footerHeight = ImGui::GetFrameHeightWithSpacing(); // height of filename input and buttons
            if (fileBrowser("File Dialog", s_fileInfo, footerHeight))
            {
                if ((s_fileInfo.getDirtyMask() & FileInfo::FILENAME) > 0)
                {
                    const std::string& filename = s_fileInfo.getFilename();
                    IM_ASSERT(filename.length() < IM_ARRAYSIZE(s_filenameBuf));
                    strcpy(s_filenameBuf, filename.c_str());
                }
            }

            ImGui::InputText("##filename", s_filenameBuf, IM_ARRAYSIZE(s_filenameBuf));
            ImGui::SameLine();
            ImVec2 buttonSize(ImGui::GetFontSize() * 7.0f, 0.0f);
            bool outFileChanged = false;
            if (ImGui::Button(actionName, buttonSize))
            {
                std::string file = s_fileInfo.getDirectory() + s_filenameBuf;
                do
                {
                    if (cocos2d::FileUtils::getInstance()->isFileExist(file) && type == FileDialogType::SAVE)
                    {
                        ImGui::OpenPopup("Confirm###Replace");
                        break;
                    }

                    if (!cocos2d::FileUtils::getInstance()->isFileExist(file) && type == FileDialogType::LOAD)
                    {
                        ImGui::OpenPopup("Open File###FileNotFound");
                        break;
                    }

                    outFile = std::move(file);

                    // TODO: A hack to return relative path to search path when loading
                    if (type == FileDialogType::LOAD)
                        outFile = outFile.substr(s_fileInfo.getRootPath().length());

                    outFileChanged = true;
                } while (false);
            }

            if (ImGui::BeginPopupModal("Open File###FileNotFound"))
            {
                ImGui::Text("File does not exists!");
                if (ImGui::Button("OK", buttonSize))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopupModal("Confirm###Replace"))
            {
                ImGui::Text("File already exists. \nDo you want to replace it?");
                if (ImGui::Button("Yes", buttonSize))
                {
                    outFile = s_fileInfo.getDirectory() + s_filenameBuf;

                    // TODO: A hack to return relative path to search path when loading
                    if (type == FileDialogType::LOAD)
                        outFile = outFile.substr(s_fileInfo.getRootPath().length());

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
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
            return outFileChanged;
        }
        else
        {
            outFile.clear();
            return true;
        }
    }

    FileInfo::FileInfo()
    :_initialized(false)
    ,_dirtyMask(0)
    {
    }

    FileInfo::FileInfo(const std::vector<std::string>& rootPaths)
    :_rootPaths(rootPaths)
    ,_initialized(true)
    ,_dirtyMask(0)
    {
        IM_ASSERT(_rootPaths.size() > 0);

        _directory = _rootPaths[0];
        _rootPath = _rootPaths[0];
    }

    void FileInfo::listDirectories()
    {
        if (!_initialized)
        {
            cocos2d::FileUtils* fileUtils = cocos2d::FileUtils::getInstance();
            IM_ASSERT(fileUtils);

            _rootPaths = fileUtils->getSearchPaths();
            IM_ASSERT(_rootPaths.size() > 0);

            _directory = _rootPaths[0];
            _rootPath = _rootPaths[0];
            _initialized = true;
        }

        listDirectories(_rootPaths);
    }

    void FileInfo::listDirectories(const std::vector<std::string>& directories, const std::string& rootPath)
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
            if (_directory == directory)
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
            {
                if (_rootPath != (rootPath.empty() ? directory : rootPath))
                {
                    _rootPath = rootPath.empty() ? directory : rootPath;
                    _dirtyMask |= FileInfo::ROOT_PATH;
                }
                
                if (_directory != directory)
                {
                    _directory = directory;
                    _dirtyMask |= FileInfo::DIRECTORY;
                }
            }

            if (open && hasSubDirectories)
            {
                listDirectories(subDirectories, rootPath.empty() ? directory : rootPath);
                ImGui::TreePop();
            }
        }
    }

    bool fileBrowser(const char* id, FileInfo& outFileInfo, float marginBottom)
    {
        outFileInfo._dirtyMask = 0;

        if (ImGui::BeginTable("Left", 1, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody, ImVec2(200, -marginBottom)))
        {
            ImGui::TableSetupColumn("Search Path", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            outFileInfo.listDirectories();
            ImGui::EndTable();
        }

        ImGui::SameLine();
        if (ImGui::BeginTable("Right", 1, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody, ImVec2(0, -marginBottom)))
        {
            ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const std::vector<std::string>& files = getOrCreateFileListFromCache(outFileInfo._directory);
            std::string currentFile = outFileInfo._directory + outFileInfo._filename;
            for (const std::string& file: files)
            {
                if (cocos2d::FileUtils::getInstance()->isFileExist(file))
                {
                    const char* filename = getFilename(file);
                    if (ImGui::Selectable(filename, currentFile == file, ImGuiSelectableFlags_AllowDoubleClick))
                    {
                        if (strcmp(outFileInfo._filename.c_str(), filename) != 0)
                        {
                            outFileInfo._filename = filename;
                            outFileInfo._dirtyMask |= FileInfo::FILENAME;
                        }

                        if (ImGui::IsMouseDoubleClicked(0))
                            outFileInfo._dirtyMask |= FileInfo::FILE_DOUBLE_CLICKED;
                    }
                }
            }

            ImGui::EndTable();
        }

        return outFileInfo._dirtyMask > 0;
    }
}
}
