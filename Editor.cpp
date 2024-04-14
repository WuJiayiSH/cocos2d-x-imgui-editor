#include "Editor.h"
#include "NodeFactory.h"
#include "WidgetFactory.h"
#include "CCIMGUI.h"
#include "imgui_impl_cocos2dx.h"
#include "ImGuiHelper.h"
#include "widgets/ImGuiDemo.h"
#include "widgets/NodeProperties.h"
#include "widgets/NodeTree.h"
#include "widgets/Viewport.h"
#include "nodes/Node3D.h"
#include "FileDialog.h"

namespace CCImEditor
{
    namespace
    {
        static cocos2d::RefPtr<cocos2d::Node> s_nextEditingNode;
        static cocos2d::RefPtr<cocos2d::Node> s_nextAddingNode;
        static std::function<void(std::string)> s_saveFileCallback;
        static std::function<void(std::string)> s_openFileCallback;
        static std::string s_currentFile;

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
                if (NodeImDrawerBase* drawer = static_cast<NodeImDrawerBase*>(node->getComponent("CCImEditor.NodeImDrawer")))
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

            NodeImDrawerBase* drawer = static_cast<NodeImDrawerBase*>(node->getComponent("CCImEditor.NodeImDrawer"));
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

        bool deserializeNode(cocos2d::Node* parent, const cocos2d::ValueMap& source)
        {
            cocos2d::ValueMap::const_iterator typeIt = source.find("type");
            if (typeIt == source.end() || typeIt->second.getType() != cocos2d::Value::Type::STRING)
                return false;

            cocos2d::Node* node = NodeFactory::getInstance()->createNode(typeIt->second.asString());
            if (!node)
                return false;

            cocos2d::ValueMap::const_iterator propertiesIt = source.find("properties");
            if (propertiesIt != source.end() && propertiesIt->second.getType() == cocos2d::Value::Type::MAP)
            {
                NodeImDrawerBase* drawer = static_cast<NodeImDrawerBase*>(node->getComponent("CCImEditor.NodeImDrawer"));
                drawer->deserialize(propertiesIt->second.asValueMap());
            }

            if (parent)
                parent->addChild(node);
            else
                Editor::getInstance()->setEditingNode(node);

            cocos2d::ValueMap::const_iterator childrenIt = source.find("children");
            if (childrenIt != source.end() && childrenIt->second.getType() == cocos2d::Value::Type::VECTOR)
            {
                const cocos2d::ValueVector& childrenVal = childrenIt->second.asValueVector();
                for (const cocos2d::Value& childVal: childrenVal)
                {
                    if (childVal.getType() == cocos2d::Value::Type::MAP)
                        deserializeNode(node, childVal.asValueMap());
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
                        s_openFileCallback = [](const std::string file)
                        {
                            cocos2d::ValueMap source = cocos2d::FileUtils::getInstance()->getValueMapFromFile(file);
                            deserializeNode(nullptr, source);
                        };
                    }

                    ImGui::Separator();
                    if (ImGui::MenuItem("Save"))
                    {
                        if (s_currentFile.empty())
                            s_saveFileCallback = serializeEditingNodeToFile;
                        else
                            serializeEditingNodeToFile(s_currentFile);
                    }

                    if (ImGui::MenuItem("Save As..."))
                    {
                        s_saveFileCallback = serializeEditingNodeToFile;
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
                    if (ImGui::MenuItem("Cut", "CTRL+X"))
                        Editor::getInstance()->cut();

                    if (ImGui::MenuItem("Copy", "CTRL+C"))
                        Editor::getInstance()->copy();
                    
                    if (ImGui::MenuItem("Paste", "CTRL+V"))
                        Editor::getInstance()->paste();

                    ImGui::Separator();
                    if (ImGui::MenuItem("Delete"))
                    {
                        if (cocos2d::Node* node = getSelectedNode())
                        {
                            if (Editor::getInstance()->getEditingNode() != node && node->getComponent("CCImEditor.NodeImDrawer"))
                                node->runAction(cocos2d::RemoveSelf::create());
                        }
                    }
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
                                s_nextAddingNode = nodeType.create();
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

        if (s_saveFileCallback)
        {
            std::string path;
            if (Internal::fileDialog(Internal::FileDialogType::SAVE, path))
            {
                if (!path.empty())
                    s_saveFileCallback(path);
                s_saveFileCallback = nullptr;
            }
        }

        if (s_openFileCallback)
        {
            std::string path;
            if (Internal::fileDialog(Internal::FileDialogType::OPEN, path))
            {
                if (!path.empty())
                    s_openFileCallback(path);
                s_openFileCallback = nullptr;
            }
        }
    }

    void Editor::update(float dt)
    {
        Node::update(dt);

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

        if (s_nextAddingNode)
        {
            // add to selected node if possible
            if (cocos2d::Node* node = getSelectedNode())
            {
                if (NodeImDrawerBase* drawer = static_cast<NodeImDrawerBase*>(node->getComponent("CCImEditor.NodeImDrawer")))
                {
                    const NodeFactory::NodeTypeMap& nodeTypes = NodeFactory::getInstance()->getNodeTypes();
                    NodeFactory::NodeTypeMap::const_iterator it = nodeTypes.find(drawer->getTypeName());
                    CC_ASSERT(it != nodeTypes.end());
                    if ((it->second.getMask() & NodeFlags_CanHaveChildren) > 0)
                    {
                        node->addChild(s_nextAddingNode);
                        s_nextAddingNode = nullptr;
                    }
                }
            }

            // otherwise add to edting node
            if (s_nextAddingNode)
            {
                if (_editingNode)
                {
                    _editingNode->addChild(s_nextAddingNode);
                    s_nextAddingNode = nullptr;
                }
            }

            // cancel if failed
            if (s_nextAddingNode)
            {
                CCLOGERROR("Failed to add node, editing node does not exist!");
                s_nextAddingNode = nullptr;
            }
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
        NodeFactory::getInstance()->registerNode<Node3D>("CCImEditor.Node3D", "3D/Node3D", NodeFlags_CanHaveChildren | NodeFlags_CanBeRoot);
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
        if (_editingNode)
            _editingNode->removeFromParent();
        _editingNode = node;

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
                node->runAction(cocos2d::RemoveSelf::create());
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

            deserializeNode(parent, _clipboardValue);
        }, 0, "paste");
    }
}
