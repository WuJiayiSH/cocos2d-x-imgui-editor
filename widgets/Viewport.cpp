#include "Viewport.h"
#include "WidgetFactory.h"
#include "imgui/imgui.h"
#include "Editor.h"

using namespace cocos2d;

namespace CCImEditor
{
    namespace
    {
        const float s_rotationSpeed = 0.005f;
        const float s_panSpeed = 1.0f;
        const float s_zoomSpeed = 50.0f;
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
}
