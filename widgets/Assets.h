#ifndef __CCIMEDITOR_ASSETS_H__
#define __CCIMEDITOR_ASSETS_H__

#include "Widget.h"
#include "FileDialog.h"

namespace CCImEditor
{
    class Assets: public Widget
    {
    private:
        void draw(bool* open) override;
        CCImEditor::Internal::FileInfo _fileInfo;
    };
}

#endif
