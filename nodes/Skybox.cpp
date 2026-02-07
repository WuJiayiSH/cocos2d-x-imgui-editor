#include "Skybox.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void Skybox::draw()
    {
        Node3D::draw();
        
        if (!drawHeader(getShortName().c_str()))
            return;

        cocos2d::Skybox* owner = static_cast<cocos2d::Skybox*>(getOwner());

        bool modified = false;
        auto setter = [&modified] (cocos2d::Skybox* node, const std::string& filePath)
        {
            modified = true;
        };

        property<FilePath>("Front",  DefaultGetter<std::string>(), setter, owner);
        property<FilePath>("Back",   DefaultGetter<std::string>(), setter, owner);
        property<FilePath>("Up",     DefaultGetter<std::string>(), setter, owner);
        property<FilePath>("Down",   DefaultGetter<std::string>(), setter, owner);
        property<FilePath>("Right",  DefaultGetter<std::string>(), setter, owner);
        property<FilePath>("Left",   DefaultGetter<std::string>(), setter, owner);

        if (modified)
        {
            std::string front, back, up, down, right, left;
            getCustomValue<FilePath>("Front", front);
            getCustomValue<FilePath>("Back", back);
            getCustomValue<FilePath>("Up", up);
            getCustomValue<FilePath>("Down", down);
            getCustomValue<FilePath>("Right", right);
            getCustomValue<FilePath>("Left", left);

            if (cocos2d::TextureCube* texture = cocos2d::TextureCube::create(left, right, up, down, front, back))
                owner->setTexture(texture);
        }
    }
}
