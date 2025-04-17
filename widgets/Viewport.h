#ifndef __CCIMEDITOR_VIEWPORT_H__
#define __CCIMEDITOR_VIEWPORT_H__

#include "Widget.h"
#include "imgui/imgui.h"
#include "ImGuizmo.h"
#include "tests/cpp-tests/Classes/Sprite3DTest/DrawNode3D.h"

namespace CCImEditor
{
    class Viewport: public Widget
    {
    public:
        Viewport() {};
        ~Viewport() override;

    protected:
        bool init(const std::string& name, const std::string& windowName, uint32_t mask) override;

    private:
        void draw(bool* open) override;
        void update(float dt) override;

        void drawGizmo();
        void drawGrid();
        std::function<void()> getGizmoOperationWrapper(cocos2d::Node* node) const;
        bool isDirty() const;
        cocos2d::Texture2D* getRenderTexture() const;
        void selectRecursively(cocos2d::Node* node, const cocos2d::Ray& ray) const;

        cocos2d::RefPtr<cocos2d::Camera> _camera;
        cocos2d::RefPtr<cocos2d::DrawNode3D> _drawGrid;

        ImVec2 _targetSize = {1.0f, 1.0f};
        bool _is3D = false;
        bool _showGrid = true;
        ImGuizmo::OPERATION _gizmoOperation = ImGuizmo::TRANSLATE;
        bool _isGizmoModeLocal = true;
        std::function<void()> _gizmoUndo;
    };
}

#endif
