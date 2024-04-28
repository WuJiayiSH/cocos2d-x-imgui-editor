#include "SpotLight.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void SpotLight::draw()
    {
        BaseLight::draw();

        cocos2d::SpotLight* owner = static_cast<cocos2d::SpotLight*>(getOwner());
        property<Degrees>("Inner Angle###InnerAngle", &cocos2d::SpotLight::getInnerAngle, &cocos2d::SpotLight::setInnerAngle, owner, 1.0f, 0.0f, 180.0f);
        property<Degrees>("Outer Angle###OuterAngle", &cocos2d::SpotLight::getOuterAngle, &cocos2d::SpotLight::setOuterAngle, owner, 1.0f, 0.0f, 180.0f);
        property("Range", &cocos2d::SpotLight::getRange, &cocos2d::SpotLight::setRange, owner);
    }
}
