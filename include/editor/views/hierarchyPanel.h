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
#include <QList>
#include <QString>
#include <QWidget>

class QJsonArray;
class QEvent;
class QMenu;
class QPoint;
class QStandardItem;
class QStandardItemModel;
class QLineEdit;
class QToolButton;
class QTreeView;
class ViewportPanel;

class HierarchyPanel : public QWidget {
    Q_OBJECT

  public:
    explicit HierarchyPanel(ViewportPanel *viewport, QWidget *parent = nullptr);
    void createObject(const QString &type, const QString &displayName);
    void renameSelectedObject();
    void deleteSelectedObject();
    void focusSelectedObject();
    void moveSelectedObjectToRoot();
    void selectAllObjects();
    void deselectAllObjects();
    void focusSearch();
    void showCreationPopup();
    QList<int> selectedObjectIds() const;

  signals:
    void objectActivated(int id);
    void cameraActivated();
    void environmentActivated();
    void graphiteActivated(const QString &path);

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    void applySceneSnapshot(const QString &snapshot);
    void rebuildScene(const QString &sceneName, const QJsonArray &objects,
                      const QJsonArray &interfaces, int selectedId);
    void appendObjects(QStandardItem *parent, const QJsonArray &objects);
    void showAddObjectMenu(const QPoint &position);
    void showContextMenu(const QPoint &position);
    int selectedObjectId() const;
    QString sceneSignature(const QString &sceneName, const QJsonArray &objects,
                           const QJsonArray &interfaces) const;

    ViewportPanel *viewport = nullptr;
    QTreeView *treeView = nullptr;
    QStandardItemModel *model = nullptr;
    QToolButton *addButton = nullptr;
    QToolButton *moreButton = nullptr;
    QLineEdit *searchField = nullptr;
    QHash<int, QStandardItem *> itemsById;
    QHash<QString, QStandardItem *> specialItems;
    QString lastStructureSignature;
    QString selectedSpecialType;
    int draggedObjectId = -1;
    bool applyingSnapshot = false;
};

#endif
