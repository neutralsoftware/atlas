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

#include <QHash>
#include <QString>
#include <QWidget>

class QJsonArray;
class QMenu;
class QPoint;
class QStandardItem;
class QStandardItemModel;
class QToolButton;
class QTreeView;
class ViewportPanel;

class HierarchyPanel : public QWidget {
    Q_OBJECT

  public:
    explicit HierarchyPanel(ViewportPanel *viewport, QWidget *parent = nullptr);

  private:
    void applySceneSnapshot(const QString &snapshot);
    void rebuildScene(const QString &sceneName, const QJsonArray &objects,
                      int selectedId);
    void appendObjects(QStandardItem *parent, const QJsonArray &objects);
    void showAddObjectMenu(const QPoint &position);
    void showContextMenu(const QPoint &position);
    void createObject(const QString &type, const QString &displayName);
    void renameSelectedObject();
    void deleteSelectedObject();
    void focusSelectedObject();
    void moveSelectedObjectToRoot();
    int selectedObjectId() const;
    QString sceneSignature(const QString &sceneName,
                           const QJsonArray &objects) const;

    ViewportPanel *viewport = nullptr;
    QTreeView *treeView = nullptr;
    QStandardItemModel *model = nullptr;
    QToolButton *addButton = nullptr;
    QToolButton *moreButton = nullptr;
    QHash<int, QStandardItem *> itemsById;
    QString lastStructureSignature;
    bool applyingSnapshot = false;
};

#endif
