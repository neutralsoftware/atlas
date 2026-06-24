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

#include "DockManager.h"
#include "DockWidget.h"

static ads::CDockWidget* makeDock(const QString& title, QWidget* content) {
    auto* dock = new ads::CDockWidget(title);
    dock->setWidget(content);
    return dock;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Atlas Engine");
    window.resize(1280, 720);

    auto* dockManager = new ads::CDockManager(&window);
    window.setCentralWidget(dockManager);

    auto* label = new QLabel("Hello, World!");
    label->setAlignment(Qt::AlignCenter);

    auto* anotherLabel = new QLabel("Other Label!");

    auto* labelDock = makeDock("Label", label);
    auto* anotherDock = makeDock("Another", anotherLabel);

    dockManager->addDockWidget(ads::LeftDockWidgetArea, anotherDock);
    dockManager->addDockWidget(ads::RightDockWidgetArea, labelDock);

    window.show();

    return app.exec();
}
