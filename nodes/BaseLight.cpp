#include "BaseLight.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void BaseLight::draw()
    {
        Node3D::draw();
        
        if (_context == Context::DRAW && !ImGui::CollapsingHeader(getShortName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            return;

        cocos2d::BaseLight* owner = static_cast<cocos2d::BaseLight*>(getOwner());
        property("Enabled", &cocos2d::BaseLight::isEnabled, &cocos2d::BaseLight::setEnabled, owner);
        property<Enum<LightFlag>>("Light Flag###LightFlag", &cocos2d::BaseLight::getLightFlag, &cocos2d::BaseLight::setLightFlag, owner);
        property("Intensity", &cocos2d::BaseLight::getIntensity, &cocos2d::BaseLight::setIntensity, owner, 0.01f, 0.0f, 1.0f);
    }
}
