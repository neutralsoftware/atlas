/*
* hierarchyPanel.h
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Hierarchy panel definition
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef ATLAS_HIERARCHYPANEL_H
#define ATLAS_HIERARCHYPANEL_H

#include <QWidget>

class QTreeView;
class QStandardItemModel;

class HierarchyPanel : public QWidget {
    Q_OBJECT

public:
    explicit HierarchyPanel(QWidget* parent = nullptr);

private:
    void populateTemporaryScene();

    QTreeView* treeView = nullptr;
    QStandardItemModel* model = nullptr;
};

#endif //ATLAS_HIERARCHYPANEL_H
