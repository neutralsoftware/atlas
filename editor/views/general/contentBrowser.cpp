/*
* contentBrowser.cpp
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Content Browser / File Explorer Declaration
* Copyright (c) 2026 Max Van den Eynde
*/

#include "editor/views/fileExplorer.h"

#include <QTreeView>
#include <QVBoxLayout>
#include <QFileSystemModel>
#include <QDir>

ContentBrowserPanel::ContentBrowserPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    model = new QFileSystemModel(this);
    model->setFilter(
        QDir::AllDirs |
        QDir::Files |
        QDir::NoDotAndDotDot
    );

    const QString rootPath = QDir::currentPath();
    model->setRootPath(rootPath);

    treeView = new QTreeView(this);
    treeView->setModel(model);
    treeView->setRootIndex(model->index(rootPath));

    treeView->setAnimated(true);
    treeView->setSortingEnabled(true);
    treeView->sortByColumn(0, Qt::AscendingOrder);

    treeView->hideColumn(1);
    treeView->hideColumn(2);
    treeView->hideColumn(3);

    layout->addWidget(treeView);
}

void ContentBrowserPanel::setRootPath(const QString& path) {
    model->setRootPath(path);
    treeView->setRootIndex(model->index(path));
}
