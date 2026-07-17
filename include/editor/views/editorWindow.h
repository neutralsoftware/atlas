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
class MaterialEditorPanel;
class PostProcessingPanel;
class QMenu;
class QShowEvent;
class QTimer;

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

    void saveLayout();
    void restoreLayout();
    void configureDockSplitters();
    void scheduleLayoutSave();
    void updateWindowTitle(bool dirty);

    EditorDockManager* dockManager = nullptr;
    ads::CDockManager* coreManager = nullptr;
    ViewportPanel* viewportPanel = nullptr;
    InspectorPanel* inspectorPanel = nullptr;
    MaterialEditorPanel* materialEditorPanel = nullptr;
    PostProcessingPanel* postProcessingPanel = nullptr;
    QMenu* viewMenu = nullptr;
    QMenu* windowMenu = nullptr;
    QTimer* layoutSaveTimer = nullptr;
    QByteArray defaultDockState;
    QString projectFile;
    QString projectName;
    bool closing = false;
    bool restoringLayout = false;
    bool startupQueued = false;
    bool startupComplete = false;

    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
};

#endif //ATLAS_EDITORWINDOW_H
