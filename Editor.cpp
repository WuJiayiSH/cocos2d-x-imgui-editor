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
#include "nodes/Node3D.h"
#include "nodes/Sprite3D.h"
#include "nodes/Node2D.h"
#include "nodes/Sprite.h"
#include "nodes/Label.h"
#include "nodes/DirectionLight.h"
#include "nodes/PointLight.h"
#include "nodes/SpotLight.h"
#include "commands/AddNode.h"
#include "commands/RemoveNode.h"
#include "FileDialog.h"

namespace CCImEditor
{
    namespace
    {
        static cocos2d::RefPtr<cocos2d::Node> s_nextEditingNode;
        static std::function<void(std::string)> s_saveFileCallback;
        static std::function<void(std::string)> s_openFileCallback;
        static std::string s_currentFile;
        Internal::FileDialogType s_fileDialogType;
        std::unordered_map<ImGuiID, std::string> s_fileDialogResults;
        ImGuiID s_fileDialogImGuiID = 0;

        cocos2d::Node* getSelectedNode()
        {
            if (cocos2d::Ref* obj = Editor::getInstance()->getUserObject("CCImGuiWidgets.NodeTree.SelectedNode"))
            {
                CC_ASSERT(dynamic_cast<cocos2d::Node*>(obj));
                return static_cast<cocos2d::Node*>(obj);
            }
            
            return nullptr;
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

        void serializeEditingNodeToFile(const std::string& file)
        {
            cocos2d::ValueMap root;
            if (serializeNode(Editor::getInstance()->getEditingNode(), root))
            {
                cocos2d::FileUtils::getInstance()->writeToFile(root, file);
                s_currentFile = file;
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
                                s_currentFile = file;
                            }
                        };
                    }

                    ImGui::Separator();
                    if (ImGui::MenuItem("Save"))
                    {
                        if (s_currentFile.empty())
                        {
                            Editor::getInstance()->openSaveFileDialog();
                            s_saveFileCallback = serializeEditingNodeToFile;
                        }
                        else
                            serializeEditingNodeToFile(s_currentFile);
                    }

                    if (ImGui::MenuItem("Save As..."))
                    {
                        Editor::getInstance()->openSaveFileDialog();
                        s_saveFileCallback = serializeEditingNodeToFile;
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

                if (ImGui::BeginMenu("Add"))
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
                            if (ImGui::MenuItem(displayName.c_str() + (lastSlash != std::string::npos ? lastSlash + 1 : 0)))
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
                                        AddNode* command = AddNode::create(parent, child);
                                        Editor::getInstance()->getCommandHistory().queue(command);
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
                ImGui::EndMenuBar();
            }

            // shortcuts
            ImGuiIO& io = ImGui::GetIO();
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
    }

    Editor::Editor()
    {
    }
    
    Editor::~Editor()
    {
    }

    Editor* Editor::getInstance()
    {
        static cocos2d::RefPtr<Editor> instance = nullptr;

        if (!instance)
            instance = cocos2d::utils::createHelper(&Editor::init);

        return instance;
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
                if (AddNode* cmd = AddNode::create(parent, node))
                {
                    _commandHistory.queue(cmd);
                }
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
}
