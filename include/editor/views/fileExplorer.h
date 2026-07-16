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

  private:
    void navigateTo(const QString &path, bool recordHistory = true);
    void openIndex(const QModelIndex &index);
    void showContextMenu(const QPoint &position);
    void showCreateMenu(const QPoint &position);
    void createFolder();
    void createScene();
    void createScript();
    void renameSelection();
    void deleteSelection();
    void revealSelection() const;
    void copySelectionPath() const;
    QString selectedPath() const;
    QString uniquePath(const QString &baseName) const;
    bool isInsideProject(const QString &path) const;
    void updateNavigationState();

    QListView *gridView = nullptr;
    QFileSystemModel *model = nullptr;
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
};

#endif
