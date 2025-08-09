#include "Editor.h"
#include "ComponentFactory.h"
#include "NodeFactory.h"
#include "WidgetFactory.h"
#include "CCIMGUI.h"
#include "imgui_impl_cocos2dx.h"
#include "imgui/imgui_internal.h"
#include "ImGuiHelper.h"
#include "widgets/ImGuiDemo.h"
#include "widgets/NodeProperties.h"
#include "widgets/NodeTree.h"
#include "widgets/Viewport.h"
#include "widgets/Assets.h"
#include "widgets/Console.h"
#include "nodes/Node3D.h"
#include "nodes/Sprite3D.h"
#include "nodes/Node2D.h"
#include "nodes/Sprite.h"
#include "nodes/Label.h"
#include "nodes/DirectionLight.h"
#include "nodes/PointLight.h"
#include "nodes/SpotLight.h"
#include "nodes/Geometry.h"
#include "commands/AddNode.h"
#include "commands/RemoveNode.h"
#include "commands/AddComponent.h"
#include "commands/RemoveComponent.h"
#if CCIME_LUA_ENGINE
#include "components/ComponentLua.h"
#include "cocos/scripting/lua-bindings/manual/CCComponentLua.h"
#endif
#include "FileDialog.h"
#include "ImGuizmo.h"
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX) || (CC_TARGET_PLATFORM == CC_PLATFORM_EMSCRIPTEN)
#include "platform/desktop/CCGLViewImpl-desktop.h"
#endif

namespace CCImEditor
{
    namespace
    {
        cocos2d::RefPtr<Editor> s_instance;

        cocos2d::Node* getSelectedNode()
        {
            if (cocos2d::Ref* obj = Editor::getInstance()->getUserObject("CCImGuiWidgets.NodeTree.SelectedNode"))
            {
                CC_ASSERT(dynamic_cast<cocos2d::Node*>(obj));
                return static_cast<cocos2d::Node*>(obj);
            }
            
            return nullptr;
        }

        void addNode(cocos2d::Node* parent, cocos2d::Node* child)
        {
            AddNode* command = AddNode::create(parent, child);
            if (!command)
                return;

            std::string name = child->getName();
            if (!name.empty())
            {
                int count = 1;
                while (parent->getChildByName(name) != nullptr)
                {
                    name = cocos2d::StringUtils::format("%s (%d)", child->getName().c_str(), count++);
                }
                child->setName(name);
            }

            Editor::getInstance()->getCommandHistory().queue(command);
        }

        void queueAddComponent(cocos2d::Node* parent, ImPropertyGroup* child)
        {
            AddComponent* command = AddComponent::create(parent, child);
            if (!command)
                return;

            cocos2d::Component* component = static_cast<cocos2d::Component*>(child->getOwner());
            std::string name = component->getName();
            if (!name.empty())
            {
                int count = 1;
                while (parent->getComponent(name) != nullptr)
                {
                    name = cocos2d::StringUtils::format("%s (%d)", component->getName().c_str(), count++);
                }
                component->setName(name);
            }

            Editor::getInstance()->getCommandHistory().queue(command);
        }

        bool canHaveChildren(cocos2d::Node* node)
        {
            if (node)
            {
                if (NodeImDrawer* drawer = node->getComponent<NodeImDrawer>())
                {
                    return drawer->canHaveChildren();
                }
            }

            return false;
        }


        bool serializeNode(cocos2d::Node* node, cocos2d::ValueMap& target)
        {
            if (!node)
                return false;

            NodeImDrawer* drawer = node->getComponent<NodeImDrawer>();
            if (!drawer)
                return false;

            target.emplace("type", drawer->getTypeName());

            if (!drawer->getFilename().empty())
            {
                target.emplace("file", drawer->getFilename());
            }

            cocos2d::ValueMap properties;
            drawer->serialize(properties);
            target.emplace("properties", cocos2d::Value(std::move(properties)));

            // For node loaded from file, don't serialize recursively
            if (drawer->getFilename().empty())
            {
                cocos2d::ValueVector childrenVal;
                cocos2d::Vector<cocos2d::Node*>& children = node->getChildren();
                for (cocos2d::Node* child: children)
                {
                    cocos2d::ValueMap childVal;
                    if (serializeNode(child, childVal))
                    {
                        childrenVal.push_back(cocos2d::Value(std::move(childVal)));
                    }
                }

                target.emplace("children", cocos2d::Value(std::move(childrenVal)));
            }

            cocos2d::ValueMap componentsVal;
            const std::map<std::string, cocos2d::RefPtr<ImPropertyGroup>>& components = drawer->getComponentPropertyGroups();
            for (const auto& [name, component]: components)
            {
                cocos2d::ValueMap componentVal;
                componentVal.emplace("type", component->getTypeName());

                cocos2d::ValueMap properties;
                component->serialize(properties);
                componentVal.emplace("properties", cocos2d::Value(std::move(properties)));

                componentsVal.emplace(name, std::move(componentVal));
            }
            target.emplace("components", cocos2d::Value(std::move(componentsVal)));
            return true;
        }

        bool deserializeNode(cocos2d::Node** node, const cocos2d::ValueMap& source)
        {
            cocos2d::ValueMap::const_iterator typeIt = source.find("type");
            if (typeIt == source.end() || typeIt->second.getType() != cocos2d::Value::Type::STRING)
                return false;

            std::string file;
            cocos2d::ValueMap::const_iterator fileIt = source.find("file");
            if (fileIt != source.end() && fileIt->second.getType() == cocos2d::Value::Type::STRING)
                file = fileIt->second.asString();

            if (!file.empty())
            {
                *node = Editor::loadFile(file);
                if (*node)
                {
                    NodeImDrawer* drawer = (*node)->getComponent<NodeImDrawer>();
                    if (drawer->getTypeName() != typeIt->second.asString())
                    {
                        CCLOGWARN("Types do not match when loading %s, discard", file.c_str());
                        *node = nullptr;
                    }
                    else
                    {
                        drawer->setFilename(file);
                    }
                }
                else
                {
                    CCLOGWARN("Failed to load file %s", file.c_str());
                }
            }

            if (!*node)
                *node = NodeFactory::getInstance()->createNode(typeIt->second.asString());

            if (!*node)
                return false;

            cocos2d::ValueMap::const_iterator propertiesIt = source.find("properties");
            if (propertiesIt != source.end() && propertiesIt->second.getType() == cocos2d::Value::Type::MAP)
            {
                NodeImDrawer* drawer = (*node)->getComponent<NodeImDrawer>();
                drawer->deserialize(propertiesIt->second.asValueMap());
            }

            cocos2d::ValueMap::const_iterator componentsIt = source.find("components");
            if (componentsIt != source.end() && componentsIt->second.getType() == cocos2d::Value::Type::MAP)
            {
                const cocos2d::ValueMap& componentsVal = componentsIt->second.asValueMap();
                for (const auto& [name, componentVal]: componentsVal)
                {
                    if (componentVal.getType() != cocos2d::Value::Type::MAP)
                        continue;

                    const cocos2d::ValueMap& componentValMap = componentVal.asValueMap();

                    cocos2d::ValueMap::const_iterator typeIt = componentValMap.find("type");
                    if (typeIt == componentValMap.end() || typeIt->second.getType() != cocos2d::Value::Type::STRING)
                        continue;

                    ImPropertyGroup* component = ComponentFactory::getInstance()->createComponent(typeIt->second.asString());
                    if (!component)
                        continue;

                    cocos2d::ValueMap::const_iterator propertiesIt = componentValMap.find("properties");
                    if (propertiesIt != componentValMap.end() && propertiesIt->second.getType() == cocos2d::Value::Type::MAP)
                    {
                        component->deserialize(propertiesIt->second.asValueMap());
                    }

                    cocos2d::Component* owner = static_cast<cocos2d::Component*>(component->getOwner());
                    owner->setName(name);
                    (*node)->addComponent(owner);

                    NodeImDrawer* drawer = (*node)->getComponent<NodeImDrawer>();
                    drawer->setComponentPropertyGroup(name, component);
                }
            }

            cocos2d::ValueMap::const_iterator childrenIt = source.find("children");
            if (childrenIt != source.end() && childrenIt->second.getType() == cocos2d::Value::Type::VECTOR)
            {
                const cocos2d::ValueVector& childrenVal = childrenIt->second.asValueVector();
                for (const cocos2d::Value& childVal: childrenVal)
                {
                    if (childVal.getType() == cocos2d::Value::Type::MAP)
                    {
                        cocos2d::Node* child = nullptr;
                        if (deserializeNode(&child, childVal.asValueMap()))
                        {
                            (*node)->addChild(child);
                        }
                    }
                }
            }

            return true;
        }

        void drawImGui()
        {
            if (cocos2d::GLProgram* program = cocos2d::GLProgramCache::getInstance()->getGLProgram(cocos2d::GLProgram::SHADER_NAME_POSITION_COLOR))
            {
                program->use();

                // create frame
                ImGui_ImplCocos2dx_NewFrame();

                ImGuizmo::BeginFrame();

                // draw all gui
                CCIMGUI::getInstance()->update();

                // render
                glUseProgram(0);
                ImGui::Render();
                ImGui_ImplCocos2dx_RenderDrawData(ImGui::GetDrawData());
            }
        }

        class DirectionLightProxy
        {
        public:
            static cocos2d::DirectionLight* create()
            {
                return cocos2d::DirectionLight::create(cocos2d::Vec3::UNIT_Z, cocos2d::Color3B::WHITE);
            }
        };

        class PointLightProxy
        {
        public:
            static cocos2d::PointLight* create()
            {
                return cocos2d::PointLight::create(cocos2d::Vec3::ZERO, cocos2d::Color3B::WHITE, 1.0f);
            }
        };

        class AmbientLightProxy
        {
        public:
            static cocos2d::AmbientLight* create()
            {
                return cocos2d::AmbientLight::create(cocos2d::Color3B::WHITE);
            }
        };

        class SpotLightProxy
        {
        public:
            static cocos2d::SpotLight* create()
            {
                return cocos2d::SpotLight::create(cocos2d::Vec3::UNIT_Z, cocos2d::Vec3::ZERO, cocos2d::Color3B::WHITE, 0.0f, CC_DEGREES_TO_RADIANS(30), 5000.0f);
            }
        };

        class QuadProxy
        {
        public:
            static cocos2d::Sprite3D* create()
            {
                cocos2d::Sprite3D* sprite3D = cocos2d::Sprite3D::create();
                if (!sprite3D)
                    return nullptr;

                std::vector<float> vertices = {-0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f};
                std::vector<float> normals = {0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0};
                std::vector<float> uvs = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};
                std::vector<unsigned short> indices = {0, 2, 1, 2, 3, 1};
                cocos2d::Mesh* mesh = cocos2d::Mesh::create(vertices, normals, uvs, indices);
                if (!mesh)
                    return nullptr;

                sprite3D->addMesh(mesh);
                sprite3D->genMaterial();
                return sprite3D;
            }
        };

        class CubeProxy
        {
        public:
            static cocos2d::Sprite3D* create()
            {
                cocos2d::Sprite3D* sprite3D = cocos2d::Sprite3D::create();
                if (!sprite3D)
                    return nullptr;

                std::vector<float> vertices = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f};
                std::vector<float> normals = {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f};
                std::vector<float> uvs = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};
                std::vector<unsigned short> indices = {0, 2, 1, 2, 3, 1, 4, 6, 5, 6, 7, 5, 8, 10, 9, 10, 11, 9, 12, 14, 13, 14, 15, 13, 16, 18, 17, 18, 19, 17, 20, 22, 21, 22, 23, 21};
                cocos2d::Mesh* mesh = cocos2d::Mesh::create(vertices, normals, uvs, indices);
                if (!mesh)
                    return nullptr;

                sprite3D->addMesh(mesh);
                sprite3D->genMaterial();
                return sprite3D;
            }
        };
    } // namespace

    Editor::Editor()
    {
    }
    
    Editor::~Editor()
    {
    }

    Editor* Editor::getInstance()
    {
        if (!s_instance)
            s_instance = cocos2d::utils::createHelper(&Editor::init);

        return s_instance;
    }

    bool Editor::isInstancePresent()
    {
        return s_instance && s_instance->getParent() != nullptr;
    }

    bool Editor::init()
    {
        if (!Node::init())
            return false;

        if (!CCIMGUI::getInstance())
            return false;

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        cocos2d::FileUtils* fileUtil = cocos2d::FileUtils::getInstance();
        fileUtil->createDirectory(fileUtil->getWritablePath() + "cc_imgui_editor");
        static const std::string lastSessionFile = fileUtil->getWritablePath() + "cc_imgui_editor/last_session.ini";
        io.IniFilename = lastSessionFile.c_str();
        
        const int style = cocos2d::UserDefault::getInstance()->getIntegerForKey("cc_imgui_editor.style", 0);
        switch (style)
        {
            case 0: ImGui::StyleColorsDark(); break;
            case 1: ImGui::StyleColorsLight(); break;
            case 2: ImGui::StyleColorsClassic(); break;
        }

        const bool debug = cocos2d::UserDefault::getInstance()->getBoolForKey("cc_imgui_editor.debug", false);
        setDebugMode(debug);

        std::string settingFile = fileUtil->getWritablePath() + "cc_imgui_editor/settings.plist";
        _settings = fileUtil->getValueMapFromFile(settingFile);

        setName("Editor");
        return true;
    }

    void Editor::onEnter()
    {
        Node::onEnter();

        CCIMGUI::getInstance()->addCallback(std::bind(&Editor::callback, this), "CCImEditor.Editor");
        scheduleUpdate();

        cocos2d::FileUtils* fileUtil = cocos2d::FileUtils::getInstance();
        std::string editorLog = fileUtil->getWritablePath() + "cc_imgui_editor/editor.log";
        freopen(editorLog.c_str(), "w", stdout);
    }

    void Editor::onExit()
    {
        Node::onExit();

        CCIMGUI::getInstance()->removeCallback("CCImEditor.Editor");
        unscheduleUpdate();
    }

    void Editor::callback()
    {
        drawDockSpace();

        for(cocos2d::RefPtr<Widget>& widget : _widgets)
        {
            if (!widget)
                continue;

            bool open = true;
            widget->draw(&open);

            if (!open)
                widget = nullptr;
        }

        bool modal = drawFileDialog();
        if(!modal && !_alertText.empty())
        {
            const char* windowName = "Alert";
            if (!ImGui::IsPopupOpen(windowName))
                ImGui::OpenPopup(windowName);

            if (ImGui::BeginPopupModal(windowName, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text(_alertText.c_str());
                if (ImGui::Button("OK"))
                {
                    _alertText = "";
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }

    void Editor::update(float dt)
    {
        Node::update(dt);

        _commandHistory.update(dt);

        _widgets.erase(std::remove(_widgets.begin(), _widgets.end(), nullptr), _widgets.end());
        for(Widget* widget : _widgets)
        {
            if (widget)
                widget->update(dt);
        }

        if (_nextEditingNode)
        {
            setEditingNode(_nextEditingNode);
            _nextEditingNode = nullptr;
        }

        updateWindowTitle();
    }

    void Editor::addWidget(Widget* widget)
    {
        _widgets.push_back(widget);
    }

    Widget* Editor::getWidget(const std::string& typeName) const
    {
        for(Widget* widget : _widgets)
        {
            if (!widget)
                continue;

            if (widget->getTypeName() == typeName)
                return widget;
        }

        return nullptr;
    }

    void Editor::registerWidgets()
    {
        WidgetFactory::getInstance()->registerWidget<Assets>("CCImEditor.Assets", "Assets");
        WidgetFactory::getInstance()->registerWidget<Viewport>("CCImEditor.Viewport", "Viewport");
        WidgetFactory::getInstance()->registerWidget<NodeTree>("CCImEditor.NodeTree", "Node Tree");
        WidgetFactory::getInstance()->registerWidget<NodeProperties>("CCImEditor.NodeProperties", "Node Properties");
        WidgetFactory::getInstance()->registerWidget<ImGuiDemo>("CCImEditor.ImGuiDemo", "ImGui Demo", WidgetFlags_DisallowMultiple);
        WidgetFactory::getInstance()->registerWidget<Console>("CCImEditor.Console", "Console", WidgetFlags_DisallowMultiple);
    }

    void Editor::registerNodes()
    {
        NodeFactory::getInstance()->registerNode<Node2D, cocos2d::Node>("CCImEditor.Node2D", "2D/Node2D", NodeFlags_CanHaveComponents | NodeFlags_CanHaveChildren | NodeFlags_CanBeRoot);
        NodeFactory::getInstance()->registerNode<Sprite, cocos2d::Sprite>("CCImEditor.Sprite", "2D/Sprite", NodeFlags_CanHaveComponents);
        NodeFactory::getInstance()->registerNode<Label, cocos2d::Label>("CCImEditor.Label", "2D/Label", NodeFlags_CanHaveComponents);

        NodeFactory::getInstance()->registerNode<Node3D, cocos2d::Node>("CCImEditor.Node3D", "3D/Node3D", NodeFlags_CanHaveComponents | NodeFlags_CanHaveChildren | NodeFlags_CanBeRoot);
        NodeFactory::getInstance()->registerNode<Sprite3D, cocos2d::Sprite3D>("CCImEditor.Sprite3D", "3D/Sprite3D", NodeFlags_CanHaveComponents);
        NodeFactory::getInstance()->registerNode<Geometry, QuadProxy>("CCImEditor.Quad", "3D/Quad", NodeFlags_CanHaveComponents);
        NodeFactory::getInstance()->registerNode<Geometry, CubeProxy>("CCImEditor.Cube", "3D/Cube", NodeFlags_CanHaveComponents);

        NodeFactory::getInstance()->registerNode<DirectionLight, DirectionLightProxy>("CCImEditor.DirectionLight", "3D/Light/Direction Light", NodeFlags_CanHaveComponents);
        NodeFactory::getInstance()->registerNode<PointLight, PointLightProxy>("CCImEditor.PointLight", "3D/Light/Point Light", NodeFlags_CanHaveComponents);
        NodeFactory::getInstance()->registerNode<BaseLight, AmbientLightProxy>("CCImEditor.AmbientLight", "3D/Light/Ambient Light", NodeFlags_CanHaveComponents);
        NodeFactory::getInstance()->registerNode<SpotLight, SpotLightProxy>("CCImEditor.SpotLight", "3D/Light/Spot Light", NodeFlags_CanHaveComponents);
    }

    void Editor::registerComponents()
    {
#if CCIME_LUA_ENGINE
        ComponentFactory::getInstance()->registerComponent<ComponentLua, cocos2d::ComponentLua>("CCImEditor.ComponentLua", "ComponentLua");
#endif
    }

    void Editor::visit(cocos2d::Renderer *renderer, const cocos2d::Mat4 &parentTransform, uint32_t parentFlags)
    {
        cocos2d::Layer::visit(renderer, parentTransform, parentFlags);

        if (isVisitableByVisitingCamera())
        {
            _command.init(_globalZOrder);
            _command.func = drawImGui;
            cocos2d::Director::getInstance()->getRenderer()->addCommand(&_command);
        }
    }

    void Editor::setCurrentFile(const std::string& file)
    {
        _currentFile = file;

        cocos2d::Value& recentFilesValue = _settings["recent_files"];
        if (recentFilesValue.getType() != cocos2d::Value::Type::VECTOR)
            recentFilesValue = cocos2d::ValueVector();

        cocos2d::ValueVector& recentFiles = recentFilesValue.asValueVector();
        recentFiles.erase(std::remove_if(recentFiles.begin(), recentFiles.end(),
            [&file](const cocos2d::Value& v)
            {
                return v.asString() == file;
            }),
            recentFiles.end()
        );

        recentFiles.insert(recentFiles.begin(), cocos2d::Value(file));
        if (recentFiles.size() > 10)
            recentFiles.resize(10);

        std::string settingFile = cocos2d::FileUtils::getInstance()->getWritablePath() + "cc_imgui_editor/settings.plist";
        cocos2d::FileUtils::getInstance()->writeToFile(_settings, settingFile);
    }

    void Editor::serializeEditingNodeToFile(const std::string& file)
    {
        if (file.empty())
            return;

        cocos2d::ValueMap root;
        if (serializeNode(getEditingNode(), root))
        {
            if (cocos2d::FileUtils::getInstance()->writeToFile(root, file))
            {
                setCurrentFile(file);
                getCommandHistory().setSavePoint();
            }
            else
                alert("Failed to write to file: %s", file.c_str());
        }
        else
        {
            alert("Failed to serialize editing node to file:\n %s", file.c_str());
        }
    }

    void Editor::drawDockSpace()
    {
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("CCImEditor.Editor", nullptr, windowFlags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspaceId = ImGui::GetID("CCImEditor.Editor");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::BeginMenu("New"))
                {
                    const NodeFactory::NodeTypeMap& nodeTypes = NodeFactory::getInstance()->getNodeTypes();
                    for (const std::pair<std::string, NodeFactory::NodeType>& pair : nodeTypes)
                    {
                        const NodeFactory::NodeType& nodeType = pair.second;
                        if ((nodeType.getMask() & NodeFlags_CanBeRoot) == 0)
                            continue;

                        const std::string& displayName = nodeType.getDisplayName();
                        const size_t lastSlash = displayName.find_last_of('/');
                        
                        bool isMenuOpen = true;
                        if (lastSlash != std::string::npos)
                            isMenuOpen = ImGuiHelper::BeginNestedMenu(displayName.substr(0, lastSlash).c_str());

                        if (isMenuOpen)
                        {
                            if (ImGui::MenuItem(displayName.c_str() + (lastSlash != std::string::npos ? lastSlash + 1 : 0)))
                            {
                                _nextEditingNode = nodeType.create();
                            }
                        }

                        if (lastSlash != std::string::npos)
                            ImGuiHelper::EndNestedMenu();
                    }
                    ImGui::EndMenu();
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Open File..."))
                {
                    openLoadFileDialog([this](const std::string& file)
                    {
                        if (cocos2d::Node* editingNode = Editor::loadFile(file))
                        {
                            setEditingNode(editingNode);
                            setCurrentFile(file);
                        }
                    });
                }

                if (ImGui::BeginMenu("Open Recent"))
                {
                    auto it = _settings.find("recent_files");
                    if (it != _settings.end())
                    {
                        const cocos2d::ValueVector& recentFiles = it->second.asValueVector();
                        for (const cocos2d::Value& recentFile: recentFiles)
                        {
                            const std::string& filename = recentFile.asString();
                            if (ImGui::MenuItem(filename.c_str()))
                            {
                                if (cocos2d::Node* editingNode = Editor::loadFile(filename))
                                {
                                    setEditingNode(editingNode);
                                    setCurrentFile(filename);
                                }
                            }
                        }
                    }

                    ImGui::EndMenu();
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Save", "CTRL+S"))
                {
                    save();
                }

                if (ImGui::MenuItem("Save As..."))
                {
                    openSaveFileDialog([this](const std::string& file){
                        serializeEditingNodeToFile(file);
                    });
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Import File..."))
                {
                    openLoadFileDialog([](const std::string& file)
                    {
                        if (cocos2d::Node* importedNode = Editor::loadFile(file))
                        {
                            if (NodeImDrawer* drawer = importedNode->getComponent<NodeImDrawer>())
                                drawer->setFilename(file);

                            cocos2d::Node* parent = nullptr;
                            if (cocos2d::Node* node = getSelectedNode())
                            {
                                if (canHaveChildren(node))
                                    parent = node;
                            }

                            if (!parent)
                                parent = Editor::getInstance()->getEditingNode();
                                
                            if (parent)
                                addNode(parent, importedNode);
                        }
                    });
                }

                ImGui::Separator();

                if (ImGui::BeginMenu("Preferences"))
                {
                    if (ImGui::Checkbox("Debug Mode", &_isDebugMode))
                    {
                        cocos2d::UserDefault::getInstance()->setBoolForKey("cc_imgui_editor.debug", _isDebugMode);
                    }
                    
                    if (ImGui::BeginMenu("Style"))
                    {
                        const int style = cocos2d::UserDefault::getInstance()->getIntegerForKey("cc_imgui_editor.style", 0);
                        bool selected = style == 0;
                        if (ImGui::Checkbox("Dark", &selected))
                        {
                            ImGui::StyleColorsDark();
                            cocos2d::UserDefault::getInstance()->setIntegerForKey("cc_imgui_editor.style", 0);
                        }

                        selected = style == 1;
                        if (ImGui::Checkbox("Light", &selected))
                        {
                            ImGui::StyleColorsLight();
                            cocos2d::UserDefault::getInstance()->setIntegerForKey("cc_imgui_editor.style", 1);
                        }

                        selected = style == 2;
                        if (ImGui::Checkbox("Classic", &selected))
                        {
                            ImGui::StyleColorsClassic();
                            cocos2d::UserDefault::getInstance()->setIntegerForKey("cc_imgui_editor.style", 2);
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                {
                    cocos2d::Director::getInstance()->end();
                }
                
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                CommandHistory& commandHistory = getCommandHistory();
                if (ImGui::MenuItem("Undo", "CTRL+Z", false, commandHistory.canUndo()))
                    commandHistory.undo();

                if (ImGui::MenuItem("Redo", "CTRL+Y", false, commandHistory.canRedo()))
                    commandHistory.redo();

                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "CTRL+X"))
                    cut();

                if (ImGui::MenuItem("Copy", "CTRL+C"))
                    copy();
                
                if (ImGui::MenuItem("Paste", "CTRL+V"))
                    paste();

                ImGui::Separator();
                if (ImGui::MenuItem("Delete", "Delete"))
                    removeSelectedNode();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Node"))
            {
                if (ImGui::MenuItem("Unpack"))
                {
                    if (cocos2d::Node* node = getSelectedNode())
                    {
                        if (NodeImDrawer* drawer = node->getComponent<NodeImDrawer>())
                            drawer->setFilename("");
                    }
                }

                if (ImGui::MenuItem("Unpack Recursively"))
                {
                    unpackRecursively(getSelectedNode());
                }

                ImGui::Separator();
                this->drawCreateNodeMenu();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Component"))
            {
                this->drawCreateComponentMenu();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Widget"))
            {
                const std::unordered_map<std::string, WidgetFactory::WidgetType>& widgetTypes = WidgetFactory::getInstance()->getWidgetTypes();
                for (const std::pair<std::string, WidgetFactory::WidgetType>& pair : widgetTypes)
                {
                    const WidgetFactory::WidgetType& widgetType = pair.second;
                    const std::string& displayName = widgetType.getDisplayName();
                    const size_t lastSlash = displayName.find_last_of('/');
                    
                    bool isMenuOpen = true;
                    if (lastSlash != std::string::npos)
                        isMenuOpen = ImGuiHelper::BeginNestedMenu(displayName.substr(0, lastSlash).c_str());

                    if (isMenuOpen)
                    {
                        if (ImGui::MenuItem(displayName.c_str() + (lastSlash != std::string::npos ? lastSlash + 1 : 0)))
                        {
                            if (widgetType.allowMultiple() || !getWidget(widgetType.getName()))
                            {
                                if (Widget* widget = widgetType.create())
                                {
                                    addWidget(widget);
                                }
                            }
                        }
                    }

                    if (lastSlash != std::string::npos)
                        ImGuiHelper::EndNestedMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Run"))
            {
                if (ImGui::MenuItem("Run With Empty Scene"))
                {
                    if (_commandHistory.atSavePoint())
                    {
                        if (cocos2d::Node* node = Editor::loadFile(_currentFile))
                        {
                            cocos2d::Scene* scene = cocos2d::Scene::create();
                            scene->addChild(node);
                            cocos2d::Director::getInstance()->replaceScene(scene);
                        }
                    }
                    else
                    {
                        alert("You must save your change before run");
                    }
                }

                for (const Runnable& runnable: _runnables)
                {
                    if (ImGui::MenuItem(runnable.first.c_str()))
                    {
                        runnable.second();
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // shortcuts
        CommandHistory& commandHistory = getCommandHistory();
        ImGuiIO& io = ImGui::GetIO();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false) && commandHistory.canUndo())
            commandHistory.undo();

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false) && commandHistory.canRedo())
            commandHistory.redo();
        
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
            save();

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X, false))
            cut();

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C, false))
            copy();

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false))
            paste();

        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
            removeSelectedNode();

        ImGui::End();
    }

    void Editor::setEditingNode(cocos2d::Node* node)
    {
        if (_editingNode == node)
            return;

        if (_editingNode)
            _editingNode->removeFromParent();

        _editingNode = node;
        _commandHistory.reset();
        cocos2d::Director::getInstance()->getRunningScene()->addChild(node);
    }

    void Editor::save()
    {
        if (_currentFile.empty())
        {
            openSaveFileDialog([this](const std::string& file){
                serializeEditingNodeToFile(file);
            });
        }
        else
            serializeEditingNodeToFile(_currentFile);
    }

    void Editor::copy()
    {
        cocos2d::ValueMap target;
        if (cocos2d::Node* node = getSelectedNode())
        {
            if (serializeNode(node, target))
            {
                _clipboardValue = std::move(target);
            }
        }
    }

    void Editor::cut()
    {
        cocos2d::ValueMap target;
        if (cocos2d::Node* node = getSelectedNode())
        {
            if (node != _editingNode && serializeNode(node, target))
            {
                _clipboardValue = std::move(target);
                if (RemoveNode* cmd = RemoveNode::create(node))
                {
                    _commandHistory.queue(cmd);
                }
            }
        }
    }
    
    void Editor::paste()
    {
        scheduleOnce([this](float)
        {
            cocos2d::Node* parent = nullptr;
            if (cocos2d::Node* node = getSelectedNode())
            {
                while(true)
                {
                    node = node->getParent();
                    if (!node)
                        break;
            
                    if (canHaveChildren(node))
                    {
                        parent = node;
                        break;
                    }
                }
            }

            if (!parent)
                parent = Editor::getInstance()->getEditingNode();

            cocos2d::Node* node = nullptr;
            if (parent && deserializeNode(&node, _clipboardValue))
            {
                addNode(parent, node);
            }
        }, 0, "paste");
    }

    bool Editor::removeSelectedNode()
    {
        if (cocos2d::Node* node = getSelectedNode())
        {
            if (Editor::getInstance()->getEditingNode() != node && node->getComponent<NodeImDrawer>())
            {
                if (RemoveNode* cmd = RemoveNode::create(node))
                {
                    Editor::getInstance()->getCommandHistory().queue(cmd);
                    return true;
                }
            }
        }

        return false;
    }

    void Editor::openLoadFileDialog(std::function<void(const std::string&)> callback)
    {
        if (ImGuiContext* context = ImGui::GetCurrentContext())
        {
            _fileDialogImGuiID = context->LastItemData.ID;
            _fileDialogType = Internal::FileDialogType::LOAD;
            _fileDialogResults[_fileDialogImGuiID].clear();
            _loadFileCallback = callback;
        }
        else
        {
            CCLOGERROR("File dialog has to be opened from a imgui context");
        }
    }

    void Editor::openSaveFileDialog(std::function<void(const std::string&)> callback)
    {
        if (ImGuiContext* context = ImGui::GetCurrentContext())
        {
            _fileDialogImGuiID = context->LastItemData.ID;
            _fileDialogType = Internal::FileDialogType::SAVE;
            _fileDialogResults[_fileDialogImGuiID].clear();
            _saveFileCallback = callback;
        }
        else
        {
            CCLOGERROR("File dialog has to be opened from a imgui context");
        }
    }

    bool Editor::fileDialogResult(std::string& outFile)
    {
        if (ImGuiContext* context = ImGui::GetCurrentContext())
        {
            ImGuiID id = context->LastItemData.ID;
            std::string& result = _fileDialogResults[id];
            if (!result.empty())
            {
                outFile = std::move(result);
                return true;
            }
        }

        return false;
    }

    cocos2d::Node* Editor::loadFile(const std::string& file)
    {
        const cocos2d::ValueMap& valueMap = cocos2d::FileUtils::getInstance()->getValueMapFromFile(file);
        cocos2d::Node* node = nullptr;
        if (deserializeNode(&node, valueMap))
        {
            return node;
        }

        return nullptr;
    }

    void Editor::updateWindowTitle()
    {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX) || (CC_TARGET_PLATFORM == CC_PLATFORM_EMSCRIPTEN)
        cocos2d::GLView* glView  =cocos2d::Director::getInstance()->getOpenGLView();
        std::string title;
        if (_currentFile.empty())
        {
            title = glView->getViewName();
        }
        else
        {
            title = cocos2d::StringUtils::format("%s%s - %s", _currentFile.c_str(), _commandHistory.atSavePoint()?"":"*", glView->getViewName().c_str());
        }

        if (title != _windowTitle)
        {
            _windowTitle = std::move(title);
            glfwSetWindowTitle(static_cast<cocos2d::GLViewImpl*>(glView)->getWindow(), _windowTitle.c_str());
        }
#endif
    }

    bool Editor::drawFileDialog()
    {
        if (_fileDialogImGuiID > 0)
        {
            std::string file;
            if (Internal::fileDialog(_fileDialogType, file))
            {
                // TODO: fileDialogResult is more imgui style but does not work in menubar
                if (_saveFileCallback && _fileDialogType == Internal::FileDialogType::SAVE)
                {
                    _saveFileCallback(file);
                    _saveFileCallback = nullptr;
                }
                else if (_loadFileCallback && _fileDialogType == Internal::FileDialogType::LOAD)
                {
                    _loadFileCallback(file);
                    _loadFileCallback = nullptr;
                }
                else
                {
                    _fileDialogResults[_fileDialogImGuiID] = std::move(file);
                }
                
                _fileDialogImGuiID = 0;
            }

            return true;
        }

        return false;
    }

    void Editor::drawCreateNodeMenu()
    {
        const NodeFactory::NodeTypeMap& nodeTypes = NodeFactory::getInstance()->getNodeTypes();
        for (NodeFactory::NodeTypeMap::const_iterator it = nodeTypes.begin(); it != nodeTypes.end(); it++)
        {
            const NodeFactory::NodeType& nodeType = it->second;
            const std::string& displayName = nodeType.getDisplayName();
            const size_t lastSlash = displayName.find_last_of('/');
            
            bool isMenuOpen = true;
            if (lastSlash != std::string::npos)
                isMenuOpen = ImGuiHelper::BeginNestedMenu(displayName.substr(0, lastSlash).c_str());

            if (isMenuOpen)
            {
                const char* shortName = displayName.c_str() + (lastSlash != std::string::npos ? lastSlash + 1 : 0);
                if (ImGui::MenuItem(shortName))
                {
                    // add to selected node if possible
                    cocos2d::Node* parent = nullptr;
                    if (cocos2d::Node* node = getSelectedNode())
                    {
                        if (canHaveChildren(node))
                            parent = node;
                    }

                    // otherwise add to edting node
                    if (!parent)
                        parent = getEditingNode();

                    if (parent)
                    {
                        if (cocos2d::Node* child = nodeType.create())
                        {
                            child->setName(shortName);
                            addNode(parent, child);
                        }
                    }
                    else
                    {
                        CCLOGERROR("Failed to add node, editing node does not exist!");
                    }
                }
            }

            if (lastSlash != std::string::npos)
                ImGuiHelper::EndNestedMenu();
        }
    }

    void Editor::drawCreateComponentMenu()
    {
        const ComponentFactory::ComponentTypeMap& componentTypes = ComponentFactory::getInstance()->getComponentTypes();
        for (const auto& [_, componentType] : componentTypes)
        {
            const std::string& displayName = componentType.getDisplayName();
            const size_t lastSlash = displayName.find_last_of('/');
            
            bool isMenuOpen = true;
            if (lastSlash != std::string::npos)
                isMenuOpen = ImGuiHelper::BeginNestedMenu(displayName.substr(0, lastSlash).c_str());

            if (isMenuOpen)
            {
                const char* shortName = displayName.c_str() + (lastSlash != std::string::npos ? lastSlash + 1 : 0);
                if (ImGui::MenuItem(shortName))
                {
                    if (cocos2d::Node* node = getSelectedNode())
                    {
                        if (ImPropertyGroup* component = ComponentFactory::getInstance()->createComponent(componentType.getName()))
                        {
                            static_cast<cocos2d::Component*>(component->getOwner())->setName(shortName);
                            queueAddComponent(node, component);
                        }
                    }
                }
            }

            if (lastSlash != std::string::npos)
                ImGuiHelper::EndNestedMenu();
        }
    }

    void Editor::unpackRecursively(cocos2d::Node* node)
    {
        if (!node)
            return;

        if (NodeImDrawer* drawer = node->getComponent<NodeImDrawer>())
        {
            drawer->setFilename("");
        }

        for (cocos2d::Node* child: node->getChildren())
        {
            unpackRecursively(child);
        }
    }
}
