#ifndef __CCIMEDITOR_EDITOR_H__
#define __CCIMEDITOR_EDITOR_H__

#include "cocos2d.h"

#include "Widget.h"
#include "CommandHistory.h"

namespace CCImEditor
{
    class Editor: public cocos2d::Layer
    {
    public:
        static Editor* getInstance();

        using cocos2d::Layer::getUserObject;
        using cocos2d::Layer::setUserObject;
        Ref* getUserObject(const std::string& path) { return _userObjects[path]; };
        void setUserObject(const std::string& path, Ref* handle) { _userObjects[path] = handle; };

        void onEnter() override;
        void onExit() override;
        
        void addWidget(Widget* widget);
        Widget* getWidget(const std::string& typeName) const;
        
        cocos2d::Node* getEditingNode() const { return _editingNode; };
        void setEditingNode(cocos2d::Node* node);

        void setDebugMode(bool isDebugMode) { _isDebugMode = isDebugMode; }
        bool isDebugMode() const { return _isDebugMode; };

        void registerWidgets();
        void registerNodes();
        
        void visit(cocos2d::Renderer *renderer, const cocos2d::Mat4 &parentTransform, uint32_t parentFlags) override;

        void openLoadFileDialog();
        void openSaveFileDialog();
        bool fileDialogResult(std::string& outFile);

        void copy();
        void cut();
        void paste();
        bool removeSelectedNode();

        CommandHistory& getCommandHistory() { return _commandHistory; };
    
    CC_CONSTRUCTOR_ACCESS: 
        Editor();
        ~Editor();

    private:
        Editor(const Editor&) = delete;
        void operator=(const Editor&) = delete;

        bool init() override;
        void update(float) override;
        void callback();

        std::unordered_map<std::string, cocos2d::WeakPtr<cocos2d::Ref>> _userObjects;
        std::vector<cocos2d::RefPtr<Widget>> _widgets;
        cocos2d::WeakPtr<cocos2d::Node> _editingNode;

        bool _isDebugMode = true;
        cocos2d::CustomCommand _command;

        cocos2d::ValueMap _clipboardValue;

        CommandHistory _commandHistory;
    };
}

#endif
