/*
* main.cpp
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Main entry point for the editor
* Copyright (c) 2026 Max Van den Eynde
*/

#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QStyle>
#include <QStyleHints>

#include "DockManager.h"
#include "DockWidget.h"
#include "../include/editor/application/styling.h"
#include "editor/debug.h"

static ads::CDockWidget* makeDock(const QString& title, QWidget* content) {
    auto* dock = new ads::CDockWidget(title);
    dock->setWidget(content);
    return dock;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif


    app.setStyle("Fusion");
    styling::applyTheme(app);

    QMainWindow window;
    window.setWindowTitle("Atlas Engine");
    window.resize(1280, 720);
    window.menuBar()->setNativeMenuBar(false);

    auto* fileMenu = window.menuBar()->addMenu("File");
    fileMenu->addAction(window.style()->standardIcon(QStyle::SP_FileIcon), "New");
    fileMenu->addAction(window.style()->standardIcon(QStyle::SP_DialogOpenButton), "Open");
    fileMenu->addAction(window.style()->standardIcon(QStyle::SP_DialogSaveButton), "Save");
    fileMenu->addSeparator();
    fileMenu->addAction(window.style()->standardIcon(QStyle::SP_DialogCloseButton), "Quit", &window, &QWidget::close);

    auto* editMenu = window.menuBar()->addMenu("Edit");
    editMenu->addAction(window.style()->standardIcon(QStyle::SP_ArrowBack), "Undo");
    editMenu->addAction(window.style()->standardIcon(QStyle::SP_ArrowForward), "Redo");
    editMenu->addSeparator();
    editMenu->addAction("Preferences");

    auto* viewMenu = window.menuBar()->addMenu("View");
    viewMenu->addAction(window.style()->standardIcon(QStyle::SP_ComputerIcon), "Debug Components");
    viewMenu->addAction(window.style()->standardIcon(QStyle::SP_FileDialogDetailedView), "Reset Layout");

    auto* windowMenu = window.menuBar()->addMenu("Window");
    windowMenu->addAction(window.style()->standardIcon(QStyle::SP_TitleBarNormalButton), "Minimize");
    windowMenu->addAction(window.style()->standardIcon(QStyle::SP_TitleBarMaxButton), "Zoom");

    auto* helpMenu = window.menuBar()->addMenu("Help");
    helpMenu->addAction(window.style()->standardIcon(QStyle::SP_MessageBoxQuestion), "About Atlas");

    ads::CDockManager::setConfigFlag(
        ads::CDockManager::OpaqueSplitterResize,
        true
    );
    ads::CDockManager::setConfigFlag(
        ads::CDockManager::FocusHighlighting,
        true
    );
    ads::CDockManager::setConfigFlag(
        ads::CDockManager::DisableStylesheet,
        true
    );

    auto* dockManager = new ads::CDockManager(&window);
    window.setCentralWidget(dockManager);

    auto* debugView = new DebugComponentsView();
    auto* debugDock = makeDock("Debug Component", debugView);
    debugDock->setIcon(window.style()->standardIcon(QStyle::SP_ComputerIcon));

    dockManager->addDockWidget(ads::RightDockWidgetArea, debugDock);

    window.show();

    return app.exec();
}
