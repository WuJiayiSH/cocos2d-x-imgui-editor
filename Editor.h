#ifndef __CCIMEDITOR_EDITOR_H__
#define __CCIMEDITOR_EDITOR_H__

#include "cocos2d.h"

#include "Widget.h"
#include "CommandHistory.h"
#include "FileDialog.h"
#include "imgui/imgui.h"

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
        void registerComponents();
        
        void visit(cocos2d::Renderer *renderer, const cocos2d::Mat4 &parentTransform, uint32_t parentFlags) override;

        void openLoadFileDialog(std::function<void(const std::string&)> callback = nullptr);
        void openSaveFileDialog(std::function<void(const std::string&)> callback = nullptr);
        bool fileDialogResult(std::string& outFile);

        void save();
        void copy();
        void cut();
        void paste();
        bool removeSelectedNode();
        void drawCreateNodeMenu();
        void drawCreateComponentMenu();
        static void unpackRecursively(cocos2d::Node* node);

        template<typename ...Args>
        void alert(const char* format, Args&&... args) CC_FORMAT_PRINTF(1, 2) {
            _alertText = cocos2d::StringUtils::format(format, std::forward<Args>(args)...);
        }

        CommandHistory& getCommandHistory() { return _commandHistory; };

        static cocos2d::Node* loadFile(const std::string& file);
        static bool isInstancePresent();
    
        void updateWindowTitle();

        void addRunnable(const std::string& label, std::function<void()> func) {_runnables.emplace_back(label, func);}

        struct ImportRule
        {
            std::function<bool(const std::string& path)> _filter;
            std::string _command;
        };
        void addImportRule(const std::string& path, const std::vector<ImportRule>& rules, bool recursive = false);

    CC_CONSTRUCTOR_ACCESS: 
        Editor();
        ~Editor();

    private:
        Editor(const Editor&) = delete;
        void operator=(const Editor&) = delete;

        bool init() override;
        void update(float) override;
        void callback();

        void drawDockSpace();
        bool drawFileDialog();

        void serializeEditingNodeToFile(const std::string& file);
        void setCurrentFile(const std::string& file);

        void import(const std::string& path, const std::vector<ImportRule>& rules, bool recursive);

        typedef std::pair<std::string, std::function<void()>> Runnable;
        std::vector<Runnable> _runnables;

        std::unordered_map<std::string, cocos2d::WeakPtr<cocos2d::Ref>> _userObjects;
        std::vector<cocos2d::RefPtr<Widget>> _widgets;

        cocos2d::WeakPtr<cocos2d::Node> _editingNode;
        cocos2d::RefPtr<cocos2d::Node> _nextEditingNode;

        bool _isDebugMode = true;
        cocos2d::CustomCommand _command;

        cocos2d::ValueMap _clipboardValue;

        CommandHistory _commandHistory;

        std::string _alertText;
        std::string _windowTitle;

        // file dialog
        std::function<void(const std::string&)> _saveFileCallback;
        std::function<void(const std::string&)> _loadFileCallback;
        Internal::FileDialogType _fileDialogType;
        std::unordered_map<ImGuiID, std::string> _fileDialogResults;
        ImGuiID _fileDialogImGuiID = 0;

        std::string _currentFile;
        cocos2d::ValueMap _settings;
        
        struct ImportRuleSet
        {
            std::string _path;
            bool _recursive;
            std::vector<ImportRule> _rules;
        };
        
        std::vector<ImportRuleSet> _importRuleSets;
    };
}

#endif
