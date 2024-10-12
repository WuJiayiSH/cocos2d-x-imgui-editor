#ifndef __CCIMEDITOR_FILEDIALOG_H__
#define __CCIMEDITOR_FILEDIALOG_H__

#include <string>
#include <vector>

namespace CCImEditor {
namespace Internal {
    enum class FileDialogType
    {
        SAVE,
        LOAD
    };
    
    bool fileDialog(FileDialogType type, std::string& outFile);

    class FileInfo
    {
    public:
        enum DirtyFlag
        {
            FILENAME = 1,
            DIRECTORY = 2,
            ROOT_PATH = 4,
            FILE_DOUBLE_CLICKED = 8
        };

        FileInfo();

        FileInfo(const std::vector<std::string>& rootPaths);

        friend bool fileBrowser(const char* id, FileInfo& outFile, float marginBottom);

        const std::string& getFilename() const { return _filename; };

        const std::string& getDirectory() const { return _directory; };

        const std::string& getRootPath() const { return _rootPath; };

        int getDirtyMask() const {return _dirtyMask;};

    private:
        void listDirectories();
        void listDirectories(const std::vector<std::string>& directories, const std::string& rootPath = "");

        std::string _rootPath;
        std::string _filename;
        std::string _directory;
        std::vector<std::string> _rootPaths;
        int _dirtyMask;
        bool _initialized;
    };

    bool fileBrowser(const char* id, FileInfo& outFileInfo, float marginBottom);
}
}

#endif
