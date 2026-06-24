/*
* fileExplorer.h
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Content Browser panel
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef ATLAS_FILEEXPLORER_H
#define ATLAS_FILEEXPLORER_H

#include <QWidget>

class QTreeView;
class QFileSystemModel;

class ContentBrowserPanel : public QWidget {
    Q_OBJECT

public:
    explicit ContentBrowserPanel(QWidget* parent = nullptr);

    void setRootPath(const QString& path);

private:
    QTreeView* treeView = nullptr;
    QFileSystemModel* model = nullptr;
};

#endif //ATLAS_FILEEXPLORER_H
