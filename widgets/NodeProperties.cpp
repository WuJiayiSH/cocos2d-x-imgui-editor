#include "NodeProperties.h"
#include "WidgetFactory.h"
#include "Editor.h"
#include "NodeImDrawer.h"

#include "cocos2d.h"

namespace CCImEditor
{
    void NodeProperties::draw(bool* open)
    {
        ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(getWindowName().c_str(), open))
        {
            if (cocos2d::Node* node = dynamic_cast<cocos2d::Node*>(Editor::getInstance()->getUserObject("CCImGuiWidgets.NodeTree.SelectedNode")))
            {
                if (NodeImDrawerBase* drawer = static_cast<NodeImDrawerBase*>(node->getComponent("CCImEditor.NodeImDrawer")))
                {
                    drawer->draw();
                }
            }
        }

        ImGui::End();
    }
}
