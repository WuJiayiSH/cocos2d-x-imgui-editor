#include "NodeTree.h"
#include "WidgetFactory.h"
#include "Editor.h"
#include "NodeImDrawer.h"
#include "CCIMGUI.h"
#include "cocos2d.h"

using namespace cocos2d;

namespace CCImEditor
{
    namespace
    {
        static const std::string s_selectedNodePath = "CCImGuiWidgets.NodeTree.SelectedNode";

        static WeakPtr<Node> s_selectedNode = nullptr;

        static bool s_shouldExpandSelectedNode = false;

        void drawNode(Node* node)
        {
            const std::string& desc = node->getDescription();
            std::string label = desc.substr(1, desc.find(' '));
            size_t size = node->getName().size();
            if (size)
            {
                label.reserve(label.size() + 2 + size);
                label.append("- ");
                label.append(node->getName());
            }

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            if (s_selectedNode == node)
                flags |= ImGuiTreeNodeFlags_Selected;

            // Hide children if node was loaded from file
            bool hideChildren = false;
            if (NodeImDrawer* drawer = static_cast<NodeImDrawer*>(node->getComponent("CCImEditor.NodeImDrawer")))
            {
                hideChildren = !drawer->getFilename().empty() && !Editor::getInstance()->isDebugMode();
            }

            if (!node->getChildrenCount() || hideChildren)
                flags |= ImGuiTreeNodeFlags_Leaf;

            if (s_shouldExpandSelectedNode)
            {
                Node* current = s_selectedNode;
                while (Node* parent = current->getParent())
                {
                    if (parent == node)
                    {
                        ImGui::SetNextItemOpen(true);
                        break;
                    }

                    current = parent;
                }
            }

            bool open = ImGui::TreeNodeEx(
                (void*)node,
                flags,
                "%s",
                label.c_str()
            );

            if (ImGui::IsItemClicked())
            {
                Editor::getInstance()->setUserObject(s_selectedNodePath, node);
            }

            if (open)
            {
                if (!hideChildren)
                {
                    const Vector<Node*>& children = node->getChildren();
                    for (Node* child : children)
                    {
                        drawNode(child);
                    }
                }

                ImGui::TreePop();
            }
        }
    }

    void NodeTree::draw(bool* open)
    {
        ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(getWindowName().c_str(), open))
        {
            Node* selectedNode = dynamic_cast<Node*>(Editor::getInstance()->getUserObject(s_selectedNodePath));
            if (selectedNode != s_selectedNode)
            {
                s_shouldExpandSelectedNode = true;
            }
            s_selectedNode = selectedNode;

            if (Editor::getInstance()->isDebugMode())
            {   
                if (Scene* scene = Director::getInstance()->getRunningScene())
                {
                    drawNode(scene);
                }
            }
            else if (cocos2d::Node* editingNode = Editor::getInstance()->getEditingNode())
            {
                drawNode(editingNode);
            }

            s_shouldExpandSelectedNode = false;
        }

        ImGui::End();
    }
}
