/*
* hierarchy.cpp
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Hierarchy panel definition
* Copyright (c) 2026 Max Van den Eynde
*/

#include <editor/views/hierarchyPanel.h>
#include <QTreeView>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QStandardItem>

#include "editor/application/styling.h"

HierarchyPanel::HierarchyPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    constexpr int margins = 4;
    layout->setContentsMargins(margins, margins, margins, margins);

    treeView = new QTreeView(this);
    model = new QStandardItemModel(this);

    model->setHorizontalHeaderLabels({"Object"});

    treeView->setModel(model);
    treeView->setHeaderHidden(true);
    treeView->setAlternatingRowColors(false);
    treeView->setAnimated(true);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(treeView);

    populateTemporaryScene();
}

void HierarchyPanel::populateTemporaryScene() {
    auto* root = new QStandardItem("Scene");
    root->setIcon(THEME_ICON(QStyle::SP_DesktopIcon));

    auto* camera = new QStandardItem("Main Camera");
    camera->setIcon(THEME_ICON(QStyle::SP_FileIcon));
    auto* light = new QStandardItem("Directional Light");
    light->setIcon(THEME_ICON(QStyle::SP_FileIcon));

    auto* player = new QStandardItem("Player");
    player->appendRow(new QStandardItem("Mesh Renderer"));
    player->appendRow(new QStandardItem("Rigidbody"));
    player->appendRow(new QStandardItem("Player Controller"));

    auto* world = new QStandardItem("World");
    world->appendRow(new QStandardItem("Terrain"));
    world->appendRow(new QStandardItem("Sky"));

    root->appendRow(camera);
    root->appendRow(light);
    root->appendRow(player);
    root->appendRow(world);

    model->appendRow(root);
    treeView->expandAll();
}
