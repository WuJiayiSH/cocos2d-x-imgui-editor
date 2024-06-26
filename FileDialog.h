#ifndef __CCIMEDITOR_FILEDIALOG_H__
#define __CCIMEDITOR_FILEDIALOG_H__

#include <string>

namespace CCImEditor {
namespace Internal {
    enum class FileDialogType
    {
        SAVE,
        LOAD
    };
    
    bool fileDialog(FileDialogType type, std::string& outFile);
}
}

#endif
