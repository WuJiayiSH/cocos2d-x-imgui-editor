#include "DirectionLight.h"
#include "cocos2d.h"

namespace CCImEditor
{
    void DirectionLight::draw()
    {
        BaseLight::draw();

        cocos2d::DirectionLight* owner = static_cast<cocos2d::DirectionLight*>(getOwner());
        property("Cast Shadow###CastShadow", &cocos2d::DirectionLight::getCastShadow, &cocos2d::DirectionLight::setCastShadow, owner);
        property<Enum<ShadowSize>>("Shadow Map Size###ShadowMapSize", &cocos2d::DirectionLight::getShadowMapSize, &cocos2d::DirectionLight::setShadowMapSize, owner);
        property("Shadow Bias###ShadowBias", &cocos2d::DirectionLight::getShadowBias, &cocos2d::DirectionLight::setShadowBias, owner, 0.001f);
    }
}
