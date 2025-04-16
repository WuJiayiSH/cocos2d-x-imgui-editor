#include "Viewport.h"
#include "WidgetFactory.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "Editor.h"
#include "ImGuizmo.h"
#include "commands/CustomCommand.h"

using namespace cocos2d;

namespace CCImEditor
{
    namespace
    {
        const float s_rotationSpeed = 0.005f;
        const float s_panSpeed = 1.0f;
        const float s_zoomSpeed = 50.0f;
    }

    std::function<void()> Viewport::getGizmoOperationWrapper(cocos2d::Node* node) const
    {
        CC_ASSERT(node);

        cocos2d::WeakPtr<cocos2d::Node> weak = node;
        if (_gizmoOperation == ImGuizmo::TRANSLATE)
        {
            const cocos2d::Vec3& position = node->getPosition3D();
            return [weak, position] () {
                weak->setPosition3D(position);
            };
        }
        else if (_gizmoOperation == ImGuizmo::ROTATE)
        {
            const cocos2d::Quaternion& rotation = node->getRotationQuat();
            return [weak, rotation] () {
                weak->setRotationQuat(rotation);
            };
        }
        else
        {
            cocos2d::Vec3 scale = { node->getScaleX(), node->getScaleY(), node->getScaleZ() };
            return [weak, scale] () {
                weak->setScale3D(scale);
            };
        }
    }

    void Viewport::drawGizmo()
    {
        cocos2d::Node* selectedNode = dynamic_cast<cocos2d::Node*>(Editor::getInstance()->getUserObject("CCImGuiWidgets.NodeTree.SelectedNode"));
        if (!selectedNode)
            return;

        cocos2d::Component* drawer = selectedNode->getComponent("CCImEditor.NodeImDrawer");
        if (!drawer)
            return;

        if (!_camera)
            return;

        ImGuiIO& io = ImGui::GetIO();
        const ImVec2& windowPos = ImGui::GetWindowPos();
        ImGuizmo::SetRect(windowPos.x, windowPos.y + ImGui::GetWindowHeight() - _targetSize.y, _targetSize.x, _targetSize.y);

        const cocos2d::Mat4& viewMatrix = _camera->getViewMatrix();
        const cocos2d::Mat4& projectionMatrix = _camera->getProjectionMatrix();
        cocos2d::Mat4 transform  = selectedNode->getNodeToParentTransform();

        ImGuizmo::SetOrthographic(!_is3D);
        ImGuizmo::SetAlternativeWindow(ImGui::GetCurrentWindow());
        if (ImGuizmo::Manipulate(viewMatrix.m, projectionMatrix.m, _gizmoOperation, _isGizmoModeLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD, transform.m))
        {
            if (!_gizmoUndo)
            {
                _gizmoUndo = getGizmoOperationWrapper(selectedNode);
            }

            cocos2d::Vec3 position, scale;
            cocos2d::Quaternion rotation;
            transform.decompose(&scale, &rotation, &position);
            if (_gizmoOperation == ImGuizmo::TRANSLATE)
            {
                selectedNode->setPosition3D(position);
            }
            else if (_gizmoOperation == ImGuizmo::ROTATE)
            {
                selectedNode->setRotationQuat(rotation);
            }
            else
            {
                selectedNode->setScale3D(scale);
            }
        }
        
        if (_gizmoUndo && !ImGuizmo::IsUsingAny())
        {
            CustomCommand* cmd = CustomCommand::create(
                getGizmoOperationWrapper(selectedNode),
                _gizmoUndo
            );
            Editor::getInstance()->getCommandHistory().queue(cmd, false);
            _gizmoUndo = nullptr;
        }
    }

    void Viewport::draw(bool* open)
    {  
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        const bool isWindowOpen = ImGui::Begin(getWindowName().c_str(), open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar();

        if (isWindowOpen)
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("View"))
                {
                    ImGui::Checkbox("3D", &_is3D);
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Node"))
                {
                    if (ImGui::BeginMenu("Gizmo"))
                    {
                        if (ImGui::RadioButton("Move", _gizmoOperation == ImGuizmo::TRANSLATE))
                            _gizmoOperation = ImGuizmo::TRANSLATE;

                        if (ImGui::RadioButton("Rotate", _gizmoOperation == ImGuizmo::ROTATE))
                            _gizmoOperation = ImGuizmo::ROTATE;

                        if (ImGui::RadioButton("Scale", _gizmoOperation == ImGuizmo::SCALE))
                            _gizmoOperation = ImGuizmo::SCALE;

                        ImGui::Separator();
                        ImGui::Checkbox("Local", &_isGizmoModeLocal);
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                
                ImGui::EndMenuBar();
            }

            // get target size before rendering texture
            _targetSize = ImGui::GetContentRegionAvail();
            if (_targetSize.x < 1.0f)_targetSize.x = 1.0f;
            if (_targetSize.y < 1.0f)_targetSize.y = 1.0f;

            if (Texture2D* texture = getRenderTexture())
            {
                const unsigned int wide = texture->getPixelsWide();
                const unsigned int high = texture->getPixelsHigh();
                ImGui::Image((ImTextureID)texture->getName(), ImVec2((float)wide, (float)high), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

                drawGizmo();
            }

            if (ImGui::IsWindowFocused() && _camera)
            {
                const ImGuiIO& io = ImGui::GetIO();
                if (_is3D)
                {
                    const Mat4& transform = _camera->getNodeToWorldTransform();
                    Vec3 pos = _camera->getPosition3D();
                    if (ImGui::IsMouseDown(2))
                    {
                        Vec3 up, left;
                        transform.getUpVector(&up);
                        transform.getLeftVector(&left);
                        pos = pos + io.MouseDelta.x * left * s_panSpeed;
                        pos = pos + io.MouseDelta.y * up * s_panSpeed;
                    }

                    Vec3 forward;
                    transform.getForwardVector(&forward);
                    pos = pos + forward * io.MouseWheel * s_zoomSpeed;
                    _camera->setPosition3D(pos);
    
                    if (ImGui::IsMouseDown(1))
                    {
                        Quaternion quat = _camera->getRotationQuat();
                        quat = Quaternion(Vec3(0, 1, 0), -io.MouseDelta.x * s_rotationSpeed) * quat;
                        quat *= Quaternion(Vec3(1, 0, 0), -io.MouseDelta.y * s_rotationSpeed);
                        _camera->setRotationQuat(quat);
                    }
                }
                else
                {
                    if (ImGui::IsMouseDown(2))
                    {
                        Vec2 pos = _camera->getPosition();
                        pos.x = pos.x - io.MouseDelta.x * s_panSpeed;
                        pos.y = pos.y + io.MouseDelta.y * s_panSpeed;
                        _camera->setPosition(pos);
                    }
                }

                if (ImGui::IsMouseClicked(0))
                {
                    const ImVec2& windowPos = ImGui::GetWindowPos();

                    const Vec2 pointMouse(io.MousePos.x - windowPos.x, windowPos.y + ImGui::GetWindowHeight() - io.MousePos.y);
                    const Vec3 pointNear(pointMouse.x, pointMouse.y, -1), pointFar(pointMouse.x, pointMouse.y, 1);
                    const Size targetSize = Size(_targetSize.x, _targetSize.y);

                    Ray ray;
                    _camera->unprojectGL(targetSize, &pointNear, &ray._origin);
                    _camera->unprojectGL(targetSize, &pointFar, &ray._direction);
                    ray._direction.subtract(ray._origin);
                    ray._direction.normalize();

                    selectRecursively(Editor::getInstance()->getEditingNode(), ray);
                }
            }
        }
        
        ImGui::End();
    }

    void Viewport::update(float)
    {
        if (!isDirty())
            return;

        Camera* camera = nullptr;
        if (_is3D)
            camera = Camera::createPerspective(60.0f, _targetSize.x / _targetSize.y, 1.0f, 10000.0f);
        else
            camera = Camera::createOrthographic(_targetSize.x, _targetSize.y, -1024.0f, 1024.0f);
        
        if (!camera)
            return;

        camera->setCameraFlag(static_cast<CameraFlag>(1 << 15));
        camera->setName(getWindowName().c_str());

        if (_camera)
        {
            if (camera->getType() == _camera->getType())
            {
                camera->setPosition3D(_camera->getPosition3D());
                camera->setRotation3D(_camera->getRotation3D());
            }
            else if(_is3D)
            {
                camera->setPosition3D(Vec3(0.0f, 500.0f, 1000.0f));
                camera->lookAt(Vec3::ZERO);
            }

            _camera->removeFromParent();
        }

        _camera = camera;

        const unsigned int wide = (unsigned int)_targetSize.x;
        const unsigned int high = (unsigned int)_targetSize.y;

        experimental::FrameBuffer* fbo = experimental::FrameBuffer::create(1, wide, high);
        if (!fbo)
            return;
            
        experimental::RenderTarget* renderTarget = experimental::RenderTarget::create(wide, high);
        if (!renderTarget)
            return;

        experimental::RenderTarget* depthStencilTarget = experimental::RenderTarget::create(wide, high, Texture2D::PixelFormat::D24S8);
        if (!depthStencilTarget)
            return;

        fbo->attachRenderTarget(renderTarget);
        fbo->attachDepthStencilTarget(depthStencilTarget);
        _camera->setFrameBufferObject(fbo);

        Editor::getInstance()->addChild(_camera);
    }

    bool Viewport::isDirty() const
    {
        Texture2D* texture = getRenderTexture();
        if (!texture)
            return true;

        const unsigned int wide = texture->getPixelsWide();
        const unsigned int high = texture->getPixelsHigh();
        if (std::abs(_targetSize.x - wide) >= 1.0f || std::abs(_targetSize.y - high) >= 1.0f)
            return true;

        if (_camera->getType() == Camera::Type::PERSPECTIVE && !_is3D)
            return true;

        if (_camera->getType() == Camera::Type::ORTHOGRAPHIC && _is3D)
            return true;
        
        return false;
    }

    cocos2d::Texture2D* Viewport::getRenderTexture() const
    {
        if (!_camera)
            return nullptr;

        experimental::FrameBuffer* fbo = _camera->getFrameBufferObject();
        if (!fbo)
            return nullptr;

        experimental::RenderTargetBase* renderTarget = fbo->getRenderTarget();
        if (!renderTarget)
            return nullptr;

        return renderTarget->getTexture();
    }

    void Viewport::selectRecursively(Node* node, const Ray& ray) const
    {
        Vector<Node*>& children = node->getChildren();
        for (Node* child : children)
        {
            if (child->getComponent("CCImEditor.NodeImDrawer"))
            {
                if (Sprite3D* sprite3D = dynamic_cast<Sprite3D*>(child))
                {
                    if (ray.intersects(sprite3D->getAABB()))
                    {
                        Editor::getInstance()->setUserObject("CCImGuiWidgets.NodeTree.SelectedNode", child);
                    }
                }
                else
                {
                    const Mat4& transform = child->getNodeToWorldTransform();
                    const Size& contentSize = child->getContentSize();
                    AABB aabb;
                    transform.transformPoint(Vec3(0, 0, 0), &aabb._min);
                    transform.transformPoint(Vec3(contentSize.width, contentSize.height, 0), &aabb._max);
                    
                    if (ray.intersects(aabb))
                    {
                        Editor::getInstance()->setUserObject("CCImGuiWidgets.NodeTree.SelectedNode", child);
                    }
                }
            }

            selectRecursively(child, ray);
        }
    }
}
