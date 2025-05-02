#include "ComponentLua.h"
#include "cocos2d.h"
#include "cocos/scripting/lua-bindings/manual/CCComponentLua.h"

namespace CCImEditor
{
    void ComponentLua::draw()
    {
        Component::draw();

        cocos2d::ComponentLua* owner = static_cast<cocos2d::ComponentLua*>(getOwner());
        property<FilePath>("Script", 
            DefaultGetter<std::string>(),
            [this] (cocos2d::ComponentLua* component, const std::string& filePath)
            {
                component->loadAndExecuteScript(filePath);
                _loadTime = time(nullptr);
            },
            owner);

        if (_context == Context::DRAW)
            reloadIfModified();
    }

    void ComponentLua::reloadIfModified()
    {
        cocos2d::FileUtils* fileUtils = cocos2d::FileUtils::getInstance();
        cocos2d::ComponentLua* component = static_cast<cocos2d::ComponentLua*>(getOwner());
        auto it = _customValue.find("Script");
        if (it != _customValue.end())
        {
            std::string filePath;
            if (CCImEditor::PropertyImDrawer<FilePath>::deserialize(it->second, filePath))
            {
                if (!fileUtils->isAbsolutePath(filePath))
                {
                    filePath = fileUtils->fullPathForFilename(filePath);
                }

                if (fileUtils->isFileExist(filePath))
                {
                    struct stat st;
                    if (stat(filePath.c_str(), &st) == 0 && st.st_mtime > _loadTime)
                    {
                        component->loadAndExecuteScript(filePath);
                        _loadTime = time(nullptr);
                    }
                }
            }
        }
    }
}
