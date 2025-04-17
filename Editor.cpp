#include "Editor.h"
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
        static cocos2d::RefPtr<cocos2d::Node> s_nextEditingNode;
        static std::function<void(std::string)> s_saveFileCallback;
        static std::function<void(std::string)> s_openFileCallback;
        static std::string s_currentFile;
        Internal::FileDialogType s_fileDialogType;
        std::unordered_map<ImGuiID, std::string> s_fileDialogResults;
        ImGuiID s_fileDialogImGuiID = 0;
        cocos2d::ValueMap s_settings;

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

        bool canHaveChildren(cocos2d::Node* node)
        {
            if (node)
            {
                if (NodeImDrawer* drawer = static_cast<NodeImDrawer*>(node->getComponent("CCImEditor.NodeImDrawer")))
                {
                    const NodeFactory::NodeTypeMap& nodeTypes = NodeFactory::getInstance()->getNodeTypes();
                    NodeFactory::NodeTypeMap::const_iterator it = nodeTypes.find(drawer->getTypeName());
                    CC_ASSERT(it != nodeTypes.end());
                    if ((it->second.getMask() & NodeFlags_CanHaveChildren) > 0)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        bool serializeNode(cocos2d::Node* node, cocos2d::ValueMap& target)
        {
            if (!node)
                return false;

            NodeImDrawer* drawer = static_cast<NodeImDrawer*>(node->getComponent("CCImEditor.NodeImDrawer"));
            if (!drawer)
                return false;

            target.emplace("type", drawer->getTypeName());

            cocos2d::ValueMap properties;
            drawer->serialize(properties);
            target.emplace("properties", cocos2d::Value(std::move(properties)));
            
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
            return true;
        }

        bool deserializeNode(cocos2d::Node** node, const cocos2d::ValueMap& source)
        {
            cocos2d::ValueMap::const_iterator typeIt = source.find("type");
            if (typeIt == source.end() || typeIt->second.getType() != cocos2d::Value::Type::STRING)
                return false;

            *node = NodeFactory::getInstance()->createNode(typeIt->second.asString());
            if (!*node)
                return false;

            cocos2d::ValueMap::const_iterator propertiesIt = source.find("properties");
            if (propertiesIt != source.end() && propertiesIt->second.getType() == cocos2d::Value::Type::MAP)
            {
                NodeImDrawer* drawer = static_cast<NodeImDrawer*>((*node)->getComponent("CCImEditor.NodeImDrawer"));
                drawer->deserialize(propertiesIt->second.asValueMap());
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

        void setCurrentFile(const std::string& file)
        {
            s_currentFile = file;

            cocos2d::Value& recentFilesValue = s_settings["recent_files"];
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
            cocos2d::FileUtils::getInstance()->writeToFile(s_settings, settingFile);
        }

        void serializeEditingNodeToFile(const std::string& file)
        {
            if (file.empty())
                return;

            cocos2d::ValueMap root;
            if (serializeNode(Editor::getInstance()->getEditingNode(), root))
            {
                if (cocos2d::FileUtils::getInstance()->writeToFile(root, file))
                {
                    setCurrentFile(file);
                    Editor::getInstance()->getCommandHistory().setSavePoint();
                }
                else
                    Editor::getInstance()->alert("Failed to write to file: %s", file.c_str());
            }
            else
            {
                Editor::getInstance()->alert("Failed to serialize editing node to file:\n %s", file.c_str());
            }
        }

        void drawDockSpace()
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
                                    s_nextEditingNode = nodeType.create();
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
                        Editor::getInstance()->openLoadFileDialog();
                        s_openFileCallback = [](const std::string file)
                        {
                            if (cocos2d::Node* editingNode = Editor::loadFile(file))
                            {
                                Editor::getInstance()->setEditingNode(editingNode);
                                setCurrentFile(file);
                            }
                        };
                    }

                    if (ImGui::BeginMenu("Open Recent"))
                    {
                        auto it = s_settings.find("recent_files");
                        if (it != s_settings.end())
                        {
                            const cocos2d::ValueVector& recentFiles = it->second.asValueVector();
                            for (const cocos2d::Value& recentFile: recentFiles)
                            {
                                const std::string& filename = recentFile.asString();
                                if (ImGui::MenuItem(filename.c_str()))
                                {
                                    if (cocos2d::Node* editingNode = Editor::loadFile(filename))
                                    {
                                        Editor::getInstance()->setEditingNode(editingNode);
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
                        Editor::getInstance()->save();
                    }

                    if (ImGui::MenuItem("Save As..."))
                    {
                        Editor::getInstance()->openSaveFileDialog();
                        s_saveFileCallback = serializeEditingNodeToFile;
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Import File..."))
                    {
                        Editor::getInstance()->openLoadFileDialog();
                        s_openFileCallback = [](const std::string file)
                        {
                            if (cocos2d::Node* importedNode = Editor::loadFile(file))
                            {
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
                        };
                    }

                    ImGui::Separator();

                    if (ImGui::BeginMenu("Preferences"))
                    {
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
                    CommandHistory& commandHistory = Editor::getInstance()->getCommandHistory();
                    if (ImGui::MenuItem("Undo", "CTRL+Z", false, commandHistory.canUndo()))
                        commandHistory.undo();

                    if (ImGui::MenuItem("Redo", "CTRL+Y", false, commandHistory.canRedo()))
                        commandHistory.redo();

                    ImGui::Separator();
                    if (ImGui::MenuItem("Cut", "CTRL+X"))
                        Editor::getInstance()->cut();

                    if (ImGui::MenuItem("Copy", "CTRL+C"))
                        Editor::getInstance()->copy();
                    
                    if (ImGui::MenuItem("Paste", "CTRL+V"))
                        Editor::getInstance()->paste();

                    ImGui::Separator();
                    if (ImGui::MenuItem("Delete", "Delete"))
                        Editor::getInstance()->removeSelectedNode();
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Node"))
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
                                    parent = Editor::getInstance()->getEditingNode();

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
                                if (widgetType.allowMultiple() || !Editor::getInstance()->getWidget(widgetType.getName()))
                                {
                                    if (Widget* widget = widgetType.create())
                                    {
                                        Editor::getInstance()->addWidget(widget);
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
                        if (!s_currentFile.empty())
                        {
                            serializeEditingNodeToFile(s_currentFile);
                            if (cocos2d::Node* node = Editor::loadFile(s_currentFile))
                            {
                                cocos2d::Scene* scene = cocos2d::Scene::create();
                                scene->addChild(node);
                                cocos2d::Director::getInstance()->replaceScene(scene);
                            }
                        }
                        else
                        {
                            Editor::getInstance()->alert("You must save your change before run");
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            // shortcuts
            CommandHistory& commandHistory = Editor::getInstance()->getCommandHistory();
            ImGuiIO& io = ImGui::GetIO();
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false) && commandHistory.canUndo())
                commandHistory.undo();

            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false) && commandHistory.canRedo())
                commandHistory.redo();
            
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
                Editor::getInstance()->save();

            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X, false))
                Editor::getInstance()->cut();

            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C, false))
                Editor::getInstance()->copy();

            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false))
                Editor::getInstance()->paste();

            if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
                Editor::getInstance()->removeSelectedNode();

            ImGui::End();
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
    }

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

        std::string settingFile = fileUtil->getWritablePath() + "cc_imgui_editor/settings.plist";
        s_settings = fileUtil->getValueMapFromFile(settingFile);

        setName("Editor");
        return true;
    }

    void Editor::onEnter()
    {
        Node::onEnter();

        CCIMGUI::getInstance()->addCallback(std::bind(&Editor::callback, this), "CCImEditor.Editor");
        scheduleUpdate();
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

        if (s_fileDialogImGuiID > 0)
        {
            std::string file;
            if (Internal::fileDialog(s_fileDialogType, file))
            {
                // TODO: fileDialogResult is more imgui style but does not work in menubar
                if (s_saveFileCallback && s_fileDialogType == Internal::FileDialogType::SAVE)
                {
                    s_saveFileCallback(file);
                    s_saveFileCallback = nullptr;
                }
                else if (s_openFileCallback && s_fileDialogType == Internal::FileDialogType::LOAD)
                {
                    s_openFileCallback(file);
                    s_openFileCallback = nullptr;
                }
                else
                {
                    s_fileDialogResults[s_fileDialogImGuiID] = std::move(file);
                }
                
                s_fileDialogImGuiID = 0;
            }
        }
        else if(!_alertText.empty())
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

        if (s_nextEditingNode)
        {
            setEditingNode(s_nextEditingNode);
            s_nextEditingNode = nullptr;
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
    }

    void Editor::registerNodes()
    {
        NodeFactory::getInstance()->registerNode<Node2D, cocos2d::Node>("CCImEditor.Node2D", "2D/Node2D", NodeFlags_CanHaveChildren | NodeFlags_CanBeRoot);
        NodeFactory::getInstance()->registerNode<Sprite, cocos2d::Sprite>("CCImEditor.Sprite", "2D/Sprite");
        NodeFactory::getInstance()->registerNode<Label, cocos2d::Label>("CCImEditor.Label", "2D/Label");

        NodeFactory::getInstance()->registerNode<Node3D, cocos2d::Node>("CCImEditor.Node3D", "3D/Node3D", NodeFlags_CanHaveChildren | NodeFlags_CanBeRoot);
        NodeFactory::getInstance()->registerNode<Sprite3D, cocos2d::Sprite3D>("CCImEditor.Sprite3D", "3D/Sprite3D");
        NodeFactory::getInstance()->registerNode<Geometry, QuadProxy>("CCImEditor.Quad", "3D/Quad");
        NodeFactory::getInstance()->registerNode<Geometry, CubeProxy>("CCImEditor.Cube", "3D/Cube");

        NodeFactory::getInstance()->registerNode<DirectionLight, DirectionLightProxy>("CCImEditor.DirectionLight", "3D/Light/Direction Light");
        NodeFactory::getInstance()->registerNode<PointLight, PointLightProxy>("CCImEditor.PointLight", "3D/Light/Point Light");
        NodeFactory::getInstance()->registerNode<BaseLight, AmbientLightProxy>("CCImEditor.AmbientLight", "3D/Light/Ambient Light");
        NodeFactory::getInstance()->registerNode<SpotLight, SpotLightProxy>("CCImEditor.SpotLight", "3D/Light/Spot Light");
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
        if (s_currentFile.empty())
        {
            openSaveFileDialog();
            s_saveFileCallback = serializeEditingNodeToFile;
        }
        else
            serializeEditingNodeToFile(s_currentFile);
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
            if (Editor::getInstance()->getEditingNode() != node && node->getComponent("CCImEditor.NodeImDrawer"))
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

    void Editor::openLoadFileDialog()
    {
        if (ImGuiContext* context = ImGui::GetCurrentContext())
        {
            s_fileDialogImGuiID = context->LastItemData.ID;
            s_fileDialogType = Internal::FileDialogType::LOAD;
            s_fileDialogResults[s_fileDialogImGuiID].clear();
        }
        else
        {
            CCLOGERROR("File dialog has to be opened from a imgui context");
        }
    }

    void Editor::openSaveFileDialog()
    {
        if (ImGuiContext* context = ImGui::GetCurrentContext())
        {
            s_fileDialogImGuiID = context->LastItemData.ID;
            s_fileDialogType = Internal::FileDialogType::SAVE;
            s_fileDialogResults[s_fileDialogImGuiID].clear();
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
            std::string& result = s_fileDialogResults[id];
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
        if (s_currentFile.empty())
        {
            title = glView->getViewName();
        }
        else
        {
            title = cocos2d::StringUtils::format("%s%s - %s", s_currentFile.c_str(), _commandHistory.atSavePoint()?"":"*", glView->getViewName().c_str());
        }

        if (title != _windowTitle)
        {
            _windowTitle = std::move(title);
            glfwSetWindowTitle(static_cast<cocos2d::GLViewImpl*>(glView)->getWindow(), _windowTitle.c_str());
        }
#endif
    }
}
