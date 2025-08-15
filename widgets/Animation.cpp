#include "Animation.h"

#include "Editor.h"
#include "NodeImDrawer.h"

#include "imgui/imgui.h"
#include "ImSequencer.h"
#include "cocos2d.h"

namespace CCImEditor
{
    namespace Internal
    {
        struct Sequence : public ImSequencer::SequenceInterface
        {
            struct Item
            {
                Item(std::string name, const std::map<int, cocos2d::Value> & values)
                :_name(name)
                ,_values(values)
                {
                    _frameStart = _values.begin()->first;
                    _frameEnd = std::prev(_values.end())->first;
                }

                std::string _name;
                const std::map<int, cocos2d::Value> &_values;
                int _frameStart;
                int _frameEnd;
            };
            
            std::vector<Item> _items;

            int GetFrameMin() const
            {
                return 0;
            }

            int GetFrameMax() const
            {
                int frameMax = 0;
                for (const Item &item : _items)
                    if (item._frameEnd > frameMax)
                        frameMax = item._frameEnd;
                return frameMax;
            }

            void BeginEdit(int index) override
            {
                CCLOG("BeginEdit");
            }

            void EndEdit() override
            {
                CCLOG("EndEdit");
            }

            void prepare(const std::string& animation)
            {
                _items.clear();
                if (cocos2d::Node* editingNode = Editor::getInstance()->getEditingNode())
                {
                    update(editingNode, animation);
                }
            }

            void update(cocos2d::Node *node, const std::string& animation)
            {
                if (NodeImDrawer *drawer = node->getComponent<NodeImDrawer>())
                {
                    ImPropertyGroup *group = drawer->getNodePropertyGroup();
                    auto it = group->_animations.find(animation);
                    if (it != group->_animations.end())
                    {
                        for (const auto &[propertyName, propertyVals] : it->second)
                        {
                            _items.push_back(Item(propertyName, propertyVals));
                        }
                    }

                    for (const auto&[_, group] : drawer->getComponentPropertyGroups())
                    {
                        auto it = group->_animations.find(animation);
                        if (it != group->_animations.end())
                        {
                            for (const auto &[propertyName, propertyVals] : it->second)
                            {
                                _items.push_back(Item(propertyName, propertyVals));
                            }
                        }
                    }
                }

                for (auto child : node->getChildren())
                {
                    update(child, animation);
                }
            }

            int GetItemCount() const override
            {
                return _items.size();
            }

            const char* GetItemLabel(int i) const override
            {
                return _items[i]._name.c_str();
            }

            void Get(int index, int **start, int **end, int *type, unsigned int *color) override
            {
                Item& item = _items[index];
                if (start)
                    *start = &item._frameStart;

                if (end)
                    *end = &item._frameEnd;

                if (type)
                    *type = 0;

                if (color)
                    *color = 0xFFAA8080;
            }
        };
    }

    void Animation::draw(bool *open)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(getWindowName().c_str(), open))
        {
            if (ImGui::Button("Play"))
            {
                _playing = true;
                _elapsed = 0.0f;
            }

            ImGui::SameLine();
            if (ImGui::Button("Stop"))
            {
                _playing = false;
            }
            static Internal::Sequence mySequence;
            static int selectedEntry = -1;
            static int firstFrame = 0;
            static bool expanded = true;
            
            mySequence.prepare("Test");
            ImSequencer::Sequencer(&mySequence, &_currentFrame, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_ALL);

            if (cocos2d::Node* editingNode = Editor::getInstance()->getEditingNode())
            {
                if (NodeImDrawer *drawer = editingNode->getComponent<NodeImDrawer>())
                {
                    drawer->getNodePropertyGroup()->play("Test",_currentFrame );
                    for (const auto&[_, group] : drawer->getComponentPropertyGroups())
                    {
                        group->play("Test",_currentFrame );
                    }
                }
            }
        }

        ImGui::End();
    }

    void Animation::update(float dt)
    {
        if (!_playing)
            return;
        _elapsed += dt;

        if (_elapsed > (1.0f / 30.0f))
        {
            _elapsed -= (1.0f / 30.0f);
            _currentFrame ++;
        }
    }
}
