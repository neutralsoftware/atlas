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

    ads::CDockManager::setConfigFlag(
        ads::CDockManager::OpaqueSplitterResize,
        true
    );

    auto* dockManager = new ads::CDockManager(&window);
    window.setCentralWidget(dockManager);

    auto* debugView = new DebugComponentsView();
    auto* debugDock = makeDock("Debug Component", debugView);

    dockManager->addDockWidget(ads::RightDockWidgetArea, debugDock);

    window.show();

    return app.exec();
}
