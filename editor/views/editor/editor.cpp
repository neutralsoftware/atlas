/*
* editor.cpp
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Editor view and window
* Copyright (c) 2026 Max Van den Eynde
*/

#include <editor/views/editorWindow.h>

#include <QMenuBar>
#include <QStyle>

#include "DockManager.h"
#include "DockWidget.h"
#include "editor/debug.h"
#include "editor/views/hierarchyPanel.h"

static ads::CDockWidget* makeDock(const QString& title, QWidget* content) {
    auto* dock = new ads::CDockWidget(title);
    dock->setWidget(content);
    return dock;
}

EditorWindow::EditorWindow(QWidget* parent)
    : QMainWindow(parent) {
    setupWindow();
    setupMenus();
    setupDocks();
}

void EditorWindow::setupWindow() {
    setWindowTitle("Atlas Engine");
    resize(1280, 720);
    menuBar()->setNativeMenuBar(false);

    ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DisableStylesheet, true);

    coreManager = new ads::CDockManager(this);
    setCentralWidget(coreManager);
    dockManager = new EditorDockManager(coreManager);
}

void EditorWindow::setupMenus() {
    auto* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction(style()->standardIcon(QStyle::SP_FileIcon), "New");
    fileMenu->addAction(style()->standardIcon(QStyle::SP_DialogOpenButton), "Open");
    fileMenu->addAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "Save");
    fileMenu->addSeparator();
    fileMenu->addAction(style()->standardIcon(QStyle::SP_DialogCloseButton), "Quit", this, &QWidget::close);

    auto* editMenu = menuBar()->addMenu("Edit");
    editMenu->addAction(style()->standardIcon(QStyle::SP_ArrowBack), "Undo");
    editMenu->addAction(style()->standardIcon(QStyle::SP_ArrowForward), "Redo");
    editMenu->addSeparator();
    editMenu->addAction("Preferences");

    auto* viewMenu = menuBar()->addMenu("View");
    viewMenu->addAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView), "Reset Layout");

    auto* windowMenu = menuBar()->addMenu("Window");
    windowMenu->addAction(style()->standardIcon(QStyle::SP_TitleBarNormalButton), "Minimize");
    windowMenu->addAction(style()->standardIcon(QStyle::SP_TitleBarMaxButton), "Zoom");

    auto* helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction(style()->standardIcon(QStyle::SP_MessageBoxQuestion), "About Atlas");
}

void EditorWindow::setupDocks() {
    dockManager->addPanel({
        .id = "hierarchy",
        .title = "Hierarchy Panel",
        .widget = new HierarchyPanel(),
        .area = EditorDockArea::Left,
        .icon = style()->standardIcon(QStyle::SP_DirOpenIcon)
    });

    dockManager->addPanel({
        .id = "debug2",
        .title = "Debug Panel",
        .widget = new DebugComponentsView(),
        .area = EditorDockArea::Right,
        .icon = style()->standardIcon(QStyle::SP_DirOpenIcon)
    });
}
