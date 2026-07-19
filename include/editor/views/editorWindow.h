/*
* editorWindow.h
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Main View for the editor's window
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef ATLAS_EDITORWINDOW_H
#define ATLAS_EDITORWINDOW_H

#include <QMainWindow>
#include <QByteArray>
#include <QString>

#include "editor/application/dockManager.h"

namespace ads {
    class CDockManager;
    class CDockWidget;
}

class ViewportPanel;
class InspectorPanel;
class HierarchyPanel;
class ContentBrowserPanel;
class ViewportTools;
class MaterialEditorPanel;
class PostProcessingPanel;
class QMenu;
class QButtonGroup;
class QStackedWidget;
class QShowEvent;
class QTimer;
class QFileSystemWatcher;
class QEvent;

class EditorWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit EditorWindow(const QString& projectFile,
                          QWidget* parent = nullptr);

signals:
    void startupStatusChanged(const QString& status);
    void startupReady(bool success, const QString& message);

private:
    void setupWindow();
    void setupMenus();
    void setupDocks();
    void setupWorkspaceBar();
    void activateWorkspace(int index);

    void saveLayout();
    void restoreLayout();
    void configureDockSplitters();
    void scheduleLayoutSave();
    void updateWindowTitle(bool dirty);
    void createScene();
    void openScene();
    void saveSceneAs();
    void showProjectSettings();
    void showInputActions();
    void showExportDialog();
    void showCommandPalette();
    void showGlobalSearch();
    void runProjectCommand(bool buildOnly);
    void takeViewportScreenshot();
    void refreshScriptWatcher();
    bool contentBrowserHasFocus() const;

    EditorDockManager* dockManager = nullptr;
    ads::CDockManager* coreManager = nullptr;
    ViewportPanel* viewportPanel = nullptr;
    InspectorPanel* inspectorPanel = nullptr;
    MaterialEditorPanel* materialEditorPanel = nullptr;
    PostProcessingPanel* postProcessingPanel = nullptr;
    HierarchyPanel* hierarchyPanel = nullptr;
    ContentBrowserPanel* contentBrowser = nullptr;
    ViewportTools* viewportTools = nullptr;
    QStackedWidget* workspaceStack = nullptr;
    QButtonGroup* workspaceModeGroup = nullptr;
    QMenu* viewMenu = nullptr;
    QMenu* windowMenu = nullptr;
    QTimer* layoutSaveTimer = nullptr;
    QFileSystemWatcher* scriptWatcher = nullptr;
    QByteArray defaultDockState;
    QString projectFile;
    QString projectName;
    bool closing = false;
    bool restoringLayout = false;
    bool startupQueued = false;
    bool startupComplete = false;

    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif //ATLAS_EDITORWINDOW_H
