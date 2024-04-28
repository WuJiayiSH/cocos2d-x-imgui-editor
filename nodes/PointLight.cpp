#include "PointLight.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void PointLight::draw()
    {
        BaseLight::draw();

        cocos2d::PointLight* owner = static_cast<cocos2d::PointLight*>(getOwner());
        property("Range", &cocos2d::PointLight::getRange, &cocos2d::PointLight::setRange, owner);
    }
}
