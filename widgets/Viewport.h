#ifndef __CCIMEDITOR_VIEWPORT_H__
#define __CCIMEDITOR_VIEWPORT_H__

#include "Widget.h"
#include "imgui/imgui.h"

namespace CCImEditor
{
    class Viewport: public Widget
    {
    private:
        void draw(bool* open) override;
        void update(float dt) override;

        bool isDirty() const;
        cocos2d::Texture2D* getRenderTexture() const;

        cocos2d::RefPtr<cocos2d::Camera> _camera;
        ImVec2 _targetSize = {1.0f, 1.0f};
        bool _is3D = false;
    };
}

#endif
