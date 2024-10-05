#ifndef __CCIMEDITOR_VIEWPORT_H__
#define __CCIMEDITOR_VIEWPORT_H__

#include "Widget.h"
#include "imgui/imgui.h"
#include "ImGuizmo.h"

namespace CCImEditor
{
    class Viewport: public Widget
    {
    private:
        void draw(bool* open) override;
        void update(float dt) override;

        void drawGizmo();
        std::function<void()> getGizmoOperationWrapper(cocos2d::Node* node) const;
        bool isDirty() const;
        cocos2d::Texture2D* getRenderTexture() const;

        cocos2d::RefPtr<cocos2d::Camera> _camera;
        ImVec2 _targetSize = {1.0f, 1.0f};
        bool _is3D = false;
        ImGuizmo::OPERATION _gizmoOperation = ImGuizmo::TRANSLATE;
        bool _isGizmoModeLocal = true;
        std::function<void()> _gizmoUndo;
    };
}

#endif
