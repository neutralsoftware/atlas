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

#include <QStringList>
#include <QWidget>

class QFileSystemModel;
class QSortFilterProxyModel;
class QLineEdit;
class QListView;
class QModelIndex;
class QPoint;
class QToolButton;

class ContentBrowserPanel : public QWidget {
    Q_OBJECT

  public:
    explicit ContentBrowserPanel(const QString &projectFile,
                                 QWidget *parent = nullptr);

    void setRootPath(const QString &path);
    void clearSelection();
    void focusSearch();
    void renameSelection();
    void deleteSelection();
    void duplicateSelection();
    void cutSelection();
    void copySelection();
    void pasteSelection();
    void refreshAssets();
    void selectAllAssets();
    QString selectedPath() const;

  signals:
    void selectionChanged(const QString &path);
    void assetActivated(const QString &path);
    void sceneActivated(const QString &path);

  private:
    void navigateTo(const QString &path, bool recordHistory = true);
    void openIndex(const QModelIndex &index);
    void showContextMenu(const QPoint &position);
    void showCreateMenu(const QPoint &position);
    void createFolder();
    void createScene();
    void createScript();
    void createMaterial();
    void revealSelection() const;
    void copySelectionPath() const;
    QString uniquePath(const QString &baseName) const;
    bool isInsideProject(const QString &path) const;
    void updateNavigationState();

    QListView *gridView = nullptr;
    QFileSystemModel *model = nullptr;
    QSortFilterProxyModel *filterModel = nullptr;
    QToolButton *backButton = nullptr;
    QToolButton *forwardButton = nullptr;
    QToolButton *upButton = nullptr;
    QToolButton *createButton = nullptr;
    QToolButton *revealButton = nullptr;
    QToolButton *moreButton = nullptr;
    QLineEdit *pathField = nullptr;
    QLineEdit *searchField = nullptr;
    QString projectRoot;
    QString currentPath;
    QStringList history;
    int historyIndex = -1;
    QStringList clipboardPaths;
    bool cutClipboard = false;
};

#endif
