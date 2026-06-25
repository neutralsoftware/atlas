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

#include "editor/application/dockManager.h"

namespace ads {
    class CDockManager;
    class CDockWidget;
}

class EditorWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit EditorWindow(QWidget* parent = nullptr);

private:
    void setupWindow();
    void setupMenus();
    void setupDocks();

    void saveLayout();
    void restoreLayout();

    EditorDockManager* dockManager = nullptr;
    ads::CDockManager* coreManager = nullptr;

    void closeEvent(QCloseEvent* event) override;
};

#endif //ATLAS_EDITORWINDOW_H
